/*
 * flash.c — NVMCTRL flash erase/write for KeyDU.BL (AVR64DU32)
 *
 * Programming model (from datasheet §11.3.2.3):
 *   1. Poll NVMCTRL.STATUS.FBUSY until clear.
 *   2. ccp_write_spm(&NVMCTRL.CTRLA, cmd)  — unlocks within 4 cycles.
 *   3. ST* write(s) to the mapped data-space address trigger the operation.
 *   4. Poll FBUSY again.
 *   5. ccp_write_spm(&NVMCTRL.CTRLA, NOCMD) to clear the command register.
 *
 * Data-space flash mapping (FLMAP in NVMCTRL.CTRLB, CCP_IOREG_gc key):
 *   SECTION0 (FLMAP=0): code-space 0x0000–0x7FFF → data-space 0x8000–0xFFFF
 *   SECTION1 (FLMAP=1): code-space 0x8000–0xFFFF → data-space 0x8000–0xFFFF
 *   flash_write_page() switches sections as needed.
 *
 * Page erase for multi-page (FLMPER2/4/8/16/32): a single dummy ST write
 * anywhere in the target range suffices — the hardware erases the whole
 * aligned group. Same busy-poll sequence as single-page erase.
 */

#include "flash.h"
#include "ccp.h"
#include "usb_vendor.h"
#include <avr/io.h>
#include <stddef.h>

/* --- NVMCTRL command codes (CTRLA CMD[6:0], datasheet Table 11-4) --- */
#define CMD_NOCMD    NVMCTRL_CMD_NOCMD_gc     /* 0x00 — clear command      */
#define CMD_FLWR     NVMCTRL_CMD_FLWR_gc      /* 0x02 — flash write enable */
#define CMD_FLPER    NVMCTRL_CMD_FLPER_gc     /* 0x08 — page erase         */
#define CMD_FLMPER2  NVMCTRL_CMD_FLMPER2_gc   /* 0x09 — 2-page erase       */
#define CMD_FLMPER4  NVMCTRL_CMD_FLMPER4_gc   /* 0x0A — 4-page erase       */
#define CMD_FLMPER8  NVMCTRL_CMD_FLMPER8_gc   /* 0x0B — 8-page erase       */
#define CMD_FLMPER16 NVMCTRL_CMD_FLMPER16_gc  /* 0x0C — 16-page erase      */
#define CMD_FLMPER32 NVMCTRL_CMD_FLMPER32_gc  /* 0x0D — 32-page erase      */

/* Data-space base for flash window (LD/ST address origin) */
#define FLASH_DATA_BASE  0x8000u

/* FLMAP section boundary in code-space byte addresses */
#define FLMAP_SECTION1_ADDR  0x8000u

/* --- Internal helpers --- */

static void nvm_wait(void)
{
    while (NVMCTRL.STATUS & NVMCTRL_FBUSY_bm) {}
}

static void nvm_cmd(uint8_t cmd)
{
    ccp_write_spm((void *)&NVMCTRL.CTRLA, cmd);
}

/* Set FLMAP to the section containing code-space byte address addr.
   NVMCTRL.CTRLB is CCP_IOREG_gc protected. Current CTRLB value is
   preserved for all bits except FLMAP[1:0]. */
static void flmap_set(uint32_t addr)
{
    uint8_t section = (addr >= FLMAP_SECTION1_ADDR) ? 1u : 0u;
    uint8_t ctrlb   = NVMCTRL.CTRLB;
    ctrlb = (ctrlb & ~NVMCTRL_FLMAP_gm) | ((section << NVMCTRL_FLMAP_gp) & NVMCTRL_FLMAP_gm);
    ccp_write_ioreg((void *)&NVMCTRL.CTRLB, ctrlb);
}

/* Convert a code-space byte address to its data-space ST target address.
   Within the active 32KB section, the offset within the section maps
   linearly onto the data-space window at FLASH_DATA_BASE. */
static volatile uint8_t *flash_ds_ptr(uint32_t code_addr)
{
    uint16_t offset = (uint16_t)(code_addr & 0x7FFFu);   /* offset within section */
    return (volatile uint8_t *)(uintptr_t)(FLASH_DATA_BASE + offset);
}

/* Map page_count_code (VCMD_ERASE wValue after decode) → NVMCTRL erase cmd.
   Codes 0–5 → 1/2/4/8/16/32 pages. Code 0 (single page) uses FLPER. */
static uint8_t erase_cmd_for_count(uint8_t n_pages)
{
    switch (n_pages) {
        case  2: return CMD_FLMPER2;
        case  4: return CMD_FLMPER4;
        case  8: return CMD_FLMPER8;
        case 16: return CMD_FLMPER16;
        case 32: return CMD_FLMPER32;
        default: return CMD_FLPER;     /* 1 page, or any unexpected value */
    }
}

/* --- Public API --- */

void flash_erase_pages(uint32_t addr, uint8_t n_pages)
{
    if (addr < APP_START) return;    /* guard: never touch bootloader section */

    flmap_set(addr);
    nvm_wait();
    nvm_cmd(erase_cmd_for_count(n_pages));
    *flash_ds_ptr(addr) = 0xFFu;     /* dummy ST write arms the erase */
    nvm_wait();
    nvm_cmd(CMD_NOCMD);
}

void flash_write_page(uint32_t addr, const uint8_t *buf)
{
    if (addr < APP_START) return;    /* guard */

    /* A 512-byte page may straddle the SECTION0/SECTION1 boundary at 0x8000.
       If the page is entirely within one section, one pass suffices.
       If it crosses (addr < 0x8000 and addr+PAGE_SIZE > 0x8000), split:
       write the lower portion in SECTION0, then the upper in SECTION1.
       In practice APP_START = 0x2000 and pages are 512B-aligned, so the
       only straddle would be at exactly 0x7E00–0x8200 — possible but rare. */
    uint32_t split = FLMAP_SECTION1_ADDR;
    uint16_t low_len  = (addr < split && (addr + PAGE_SIZE) > split)
                        ? (uint16_t)(split - addr) : 0u;
    uint16_t high_off = low_len;                    /* offset into buf        */
    uint16_t high_len = (uint16_t)(PAGE_SIZE - high_off);

    /* Write lower portion (or full page if no split) in SECTION0 */
    uint16_t first_len = low_len ? low_len : (uint16_t)PAGE_SIZE;
    flmap_set(addr);
    nvm_wait();
    nvm_cmd(CMD_FLWR);
    volatile uint8_t *dst = flash_ds_ptr(addr);
    for (uint16_t i = 0; i < first_len; i++) dst[i] = buf[i];
    nvm_wait();
    nvm_cmd(CMD_NOCMD);

    /* Write upper portion if page crossed section boundary */
    if (low_len) {
        flmap_set(split);
        nvm_wait();
        nvm_cmd(CMD_FLWR);
        dst = flash_ds_ptr(split);
        for (uint16_t i = 0; i < high_len; i++) dst[i] = buf[high_off + i];
        nvm_wait();
        nvm_cmd(CMD_NOCMD);
    }
}
