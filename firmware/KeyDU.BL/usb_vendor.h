/*
   usb_vendor.h — Vendor-class USB interface for KeyDU.BL
   SPDX-License-Identifier: GPL-3.0-or-later
   Copyright (C) 2026 Jonathan Ferron

   EP0 control-transfer flash protocol only.
   No bulk endpoints.

   Protocol commands (bRequest):
     VCMD_ERASE  0x01  wValue=page_count_code, wIndex=addr_lo16, no data
     VCMD_WRITE  0x02  wValue=addr_hi16, wIndex=addr_lo16, data=CHUNK_SIZE bytes
     VCMD_READ   0x03  wValue=addr_hi16, wIndex=addr_lo16, data=CHUNK_SIZE bytes
     VCMD_RESET  0x04  no args — triggers application jump after status ACK
     VCMD_STATUS 0x05  no args — returns 1-byte status (0x00=OK, 0xFF=ERR)

   Write flow:
     Host sends CHUNKS_PER_PAGE × VCMD_WRITE chunks (CHUNK_SIZE bytes each),
     filling a PAGE_SIZE-byte buffer.  On the last chunk flash_write_page()
     is called automatically.  VCMD_ERASE must precede writes on any page.

   Address space:
     Application area only — [APP_START, FLASH_END).
     Any request outside this range is rejected with STATUS_ERROR.
*/

#ifndef USB_VENDOR_H
#define USB_VENDOR_H

#include <stdint.h>
#include <stdbool.h>

/* ── USB identity ──────────────────────────────────────────────────────── */
#define VENDOR_VID              0x1209u          /* pid.codes open-source VID  */
#define VENDOR_PID              0xB4B1u          /* KeyDU.BL: not yet assigned by pid.codes. Must proceed with registration over the course of the next few months. */

#define VENDOR_DEVICE_VERSION      VERSION_BCD(1,0,0)  /* hw rev 1, fw 0 : use the minor byte for the hardware revision and the patch nibble for firmware revision. Bump the major when we go to a new PCB spin, minor for significant firmware changes. This doesn't affect OS behavior but makes it easier to tell from lsusb which build you're looking at without a serial read. */

#define VENDOR_STR_MANUFACTURER L"KeyDU"
#define VENDOR_STR_PRODUCT      L"Citrouille90 Bootloader"

/* ── String descriptor indices ─────────────────────────────────────────── */
#define VENDOR_STRIDX_LANG          0u
#define VENDOR_STRIDX_MANUFACTURER  1u
#define VENDOR_STRIDX_PRODUCT       2u

/* ── Protocol command codes (bRequest) ─────────────────────────────────── */
#define VCMD_ERASE      0x01u
#define VCMD_WRITE      0x02u
#define VCMD_READ       0x03u
#define VCMD_RESET      0x04u
#define VCMD_STATUS     0x05u

/* ── Status byte values (returned by VCMD_STATUS) ──────────────────────── */
#define STATUS_OK       0x00u
#define STATUS_ERROR    0xFFu

/* ── Erase count codes (wValue for VCMD_ERASE) ─────────────────────────── */
#define ERASE_PAGES_1   0x00u
#define ERASE_PAGES_2   0x01u
#define ERASE_PAGES_4   0x02u
#define ERASE_PAGES_8   0x03u
#define ERASE_PAGES_16  0x04u
#define ERASE_PAGES_32  0x05u

/* ── Flash geometry ─────────────────────────────────────────────────────── */
#define APP_START       0x2000u              /* 8KB bootloader partition      */
/* PARTITION BOUNDARY — bootloader occupies 0x0000–0x1FFF (8KB)
     8KB = 8192 bytes = 0x2000 bytes  (0x2000 = 2 × 16³ = 2 × 4096)
     → application starts at byte address 0x2000
     8KB = 16 pages of 512B → BOOTSIZE = 0x10

   To change the bootloader size, use a calculator or:
     python3 -c "n=10240; print(f'APP_START=0x{n:04X}  BOOTSIZE=0x{n//512:02X}')"

   If you change this, also update:
     usb_vendor.h  : APP_START       (byte address = bootloader size in bytes)
     KeyDU.BL.ld   : flash LENGTH    (bootloader size in bytes)
     KeyDU.App.ld  : flash ORIGIN    (= APP_START = bootloader size in bytes)
     fuses.c       : BOOTSIZE        (page count = bootloader size / 512) */


#define PAGE_SIZE       512u                 /* AVR64DU32 flash page size      */
#define CHUNK_SIZE      64u                  /* EP0 transfer chunk             */
#define CHUNKS_PER_PAGE (PAGE_SIZE / CHUNK_SIZE)   /* 8                       */

/* ── Public API ─────────────────────────────────────────────────────────── */

/* Call once at BL startup. Initialises clock, USB hardware, and state. */
void usb_vendor_init(void);

/* Bare polling loop. Handles USB tasks and jumps to app on VCMD_RESET. */
void usb_vendor_task(void);

/* usb_event_ctrl_request() — overrides weak stub in usb_ctrl.c.
   Handles all five VCMD_* vendor requests. */
void usb_event_ctrl_request(void);

#endif /* USB_VENDOR_H */
