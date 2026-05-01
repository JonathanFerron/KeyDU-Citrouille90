/*
 * flash.h — Flash erase/write interface for KeyDU.BL
 *
 * All operations are blocking (polls NVMCTRL.STATUS.FBUSY).
 * Addresses are byte addresses in code space (0x0000 base).
 * Both functions enforce APP_START guard — calls with addr < APP_START
 * are silently ignored (second line of defence after usb_vendor.c check).
 *
 * FLMAP note: the AVR64DU32 maps only 32 KB of flash into data space at a
 * time. flash_write_page() switches FLMAP automatically when the target page
 * crosses the 32 KB boundary (0x8000 code-space byte address).
 */

#ifndef FLASH_H
#define FLASH_H

#include <stdint.h>

/* Erase n pages starting at the page containing addr.
   n must be 1, 2, 4, 8, 16, or 32 — matches VCMD_ERASE page_count_code
   after decode_page_count(). Behaviour for other values is undefined. */
void flash_erase_pages(uint32_t addr, uint8_t n_pages);

/* Write a full 512-byte page to addr (must be page-aligned).
   buf must point to PAGE_SIZE bytes. */
void flash_write_page(uint32_t addr, const uint8_t *buf);

#endif /* FLASH_H */
