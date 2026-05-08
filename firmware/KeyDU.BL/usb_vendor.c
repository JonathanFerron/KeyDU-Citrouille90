/*
 * usb_vendor.c — Vendor-class USB for KeyDU.BL
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * Flash protocol over EP0 control transfers only.
 * No bulk endpoints — EP0-only keeps the bootloader simple and small.
 *
 * State machine:
 *   s_page_buf[]    — 512-byte assembly buffer; filled chunk by chunk
 *   s_chunk_count   — chunks received into current page (0–7)
 *   s_page_addr     — flash byte address of page being assembled
 *   s_status        — STATUS_OK / STATUS_ERROR, returned by VCMD_STATUS
 *   s_reset_pending — set by VCMD_RESET; jump fires in usb_vendor_task()
 *
 * Address validation: any write target < APP_START is rejected.
 * Page flush: fires automatically when s_chunk_count reaches CHUNKS_PER_PAGE.
 *
 * Caller model:
 *   usb_vendor_init() — call once at BL startup
 *   usb_vendor_task() — call in a bare while(1); returns only on VCMD_RESET
 *                       after jumping to the application
 */

#include "usb_vendor.h"
#include "usb_vendor_desc.h"
#include "flash.h"
#include "usb_ctrl.h"
#include "usb_ep.h"
#include "usb_ep_stream.h"
#include "clock.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>

/* ============================================================================
 * Module state
 * ========================================================================= */

static uint8_t  s_page_buf[PAGE_SIZE];
static uint8_t  s_chunk_count;
static uint32_t s_page_addr;
static uint8_t  s_status;
static volatile uint8_t s_reset_pending;

/* ============================================================================
 * Internal helpers
 * ========================================================================= */

/* Decode VCMD_ERASE wValue page-count code → actual page count.
 * Codes: 0→1, 1→2, 2→4, 3→8, 4→16, 5→32.  Invalid code → 1. */
static uint8_t decode_page_count(uint8_t code)
{
    if (code > 5u) return 1u;
    return (uint8_t)(1u << code);
}

/* Return byte address of the page containing addr (aligned down). */
static uint32_t page_base(uint32_t addr)
{
    return addr & ~((uint32_t)(PAGE_SIZE - 1u));
}

/* True if addr is within the writable application partition. */
static bool addr_valid(uint32_t addr)
{
    return addr >= (uint32_t)APP_START;
}

/* ============================================================================
 * VCMD handlers — called from usb_event_ctrl_request()
 * ========================================================================= */

static void handle_erase(uint8_t page_count_code, uint32_t addr)
{
    ep_select(EP_CTRL);
    ep_clear_setup();

    if (!addr_valid(addr)) {
        s_status = STATUS_ERROR;
    } else {
        flash_erase_pages(addr, decode_page_count(page_count_code));
        s_status = STATUS_OK;
    }

    ep_complete_ctrl_status();
}

static void handle_write(uint32_t addr)
{
    uint8_t chunk_buf[CHUNK_SIZE];

    ep_select(EP_CTRL);
    ep_clear_setup();
    ep_read_ctrl_stream(chunk_buf, CHUNK_SIZE);

    if (!addr_valid(addr)) {
        s_status = STATUS_ERROR;
        ep_complete_ctrl_status();
        return;
    }

    uint32_t target_page = page_base(addr);

    /* If this chunk starts a new page (or is the very first), reset state. */
    if (s_chunk_count == 0u || target_page != s_page_addr) {
        s_page_addr   = target_page;
        s_chunk_count = 0u;
        memset(s_page_buf, 0xFF, PAGE_SIZE);
    }

    uint16_t offset = (uint16_t)(addr - s_page_addr);
    memcpy(s_page_buf + offset, chunk_buf, CHUNK_SIZE);
    s_chunk_count++;

    if (s_chunk_count >= CHUNKS_PER_PAGE) {
        flash_write_page(s_page_addr, s_page_buf);
        s_chunk_count = 0u;
    }

    s_status = STATUS_OK;
    ep_complete_ctrl_status();
}

static void handle_read(uint32_t addr)
{
    ep_select(EP_CTRL);
    ep_clear_setup();

    if (!addr_valid(addr)) {
        s_status = STATUS_ERROR;
        ep_stall();
        return;
    }

    /* AVR64DU32: application flash is memory-mapped, read directly via pointer. */
    ep_write_ctrl_stream((const void *)(uintptr_t)addr, CHUNK_SIZE);
    ep_complete_ctrl_status();
}

static void handle_status(void)
{
    ep_select(EP_CTRL);
    ep_clear_setup();
    ep_write_ctrl_stream(&s_status, sizeof(s_status));
    ep_complete_ctrl_status();
}

static void handle_reset(void)
{
    ep_select(EP_CTRL);
    ep_clear_setup();
    ep_complete_ctrl_status();
    s_reset_pending = 1u;   /* defer jump until ACK is on the wire */
}

/* ============================================================================
 * usb_event_ctrl_request — overrides weak stub in usb_ctrl.c
 *
 * Called for every SETUP packet before standard dispatch.
 * We handle vendor-class requests here; anything else falls through
 * and usb_ctrl.c will stall it.
 * ========================================================================= */
void usb_event_ctrl_request(void)
{
    /* Only handle vendor-class requests. */
    if ((usb_ctrl_req.bm_request_type & REQ_TYPE_MASK) != REQ_TYPE_VENDOR)
        return;

    const uint8_t  cmd  = usb_ctrl_req.b_request;
    const uint16_t wval = usb_ctrl_req.w_value;
    const uint16_t widx = usb_ctrl_req.w_index;

    /* WRITE / READ address: wValue = addr_hi16, wIndex = addr_lo16 */
    const uint32_t addr = ((uint32_t)wval << 16) | widx;

    switch (cmd) {
        case VCMD_ERASE:
            /* wValue = page_count_code, wIndex = erase base address (lo16 only) */
            handle_erase((uint8_t)wval, (uint32_t)widx);
            break;

        case VCMD_WRITE:
            handle_write(addr);
            break;

        case VCMD_READ:
            handle_read(addr);
            break;

        case VCMD_STATUS:
            handle_status();
            break;

        case VCMD_RESET:
            handle_reset();
            break;

        default:
            /* Unrecognised vendor command — fall through; usb_ctrl.c stalls. */
            break;
    }
}

void usb_event_config_changed(void)
{
    clock_autotune_enable();
}

void usb_event_suspend(void)
{
    clock_autotune_disable();
}

void usb_event_reset(void)
{
    clock_autotune_disable();
}

/* ============================================================================
 * usb_vendor_init — call once at BL startup, before sei()
 * ========================================================================= */
void usb_vendor_init(void)
{
    s_chunk_count   = 0u;
    s_page_addr     = 0u;
    s_status        = STATUS_OK;
    s_reset_pending = 0u;
    memset(s_page_buf, 0xFF, PAGE_SIZE);

    clock_init();   /* 24 MHz OSCHF, TIMEBASE=24 — required before USB */

    sei();

    /* USB_OPT_VREG_ENABLE: internal 3.3V VUSB regulator is used on this board. */
    usb_init(USB_OPT_VREG_ENABLE);
}

/* ============================================================================
 * usb_vendor_task — bare loop; call after usb_vendor_init()
 *
 * Polls USB and, when s_reset_pending is set (VCMD_RESET was ACKed),
 * disables USB and jumps to the application entry point.
 * ========================================================================= */
void usb_vendor_task(void)
{
    while (1) {
        usb_task();

        if (s_reset_pending) {
            /* Short spin so the ACK handshake completes before detach */
            for (volatile uint16_t d = 0; d < 4000u; d++) {}

            usb_disable();
            clock_autotune_disable();   /* leave clock in a clean known state for app */

            /* Clear GPR magic so a plain power-on reset stays in the app */
            GPR.GPR2 = 0x00u;
            GPR.GPR3 = 0x00u;
            
            ccp_write_ioreg((void *)&RSTCTRL.SWRR, RSTCTRL_SWRST_bm);
            while (1) {} /* Should not be reached */
            
            //ccp_write_ioreg((void *)&CPUINT.CTRLA, 0x00);

            /* Jump to application reset vector (byte addr → word addr). */
            //void (*app_entry)(void) = (void (*)(void))((uintptr_t)APP_START >> 1);
            //app_entry();

            /* Should not be reached */
            //while (1) {}
        }
    }
}
