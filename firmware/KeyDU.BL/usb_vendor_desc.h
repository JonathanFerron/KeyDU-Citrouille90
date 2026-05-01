/*
 * usb_vendor_desc.h — Descriptor tables for KeyDU.BL
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 */

#ifndef USB_VENDOR_DESC_H
#define USB_VENDOR_DESC_H

#include <stdint.h>

/* usb_get_desc — required usbcore callback, implemented in usb_vendor_desc.c.
   Called by usb_ctrl.c to service GET_DESCRIPTOR requests. */
uint16_t usb_get_desc(uint16_t w_value, uint16_t w_index, const void **addr);

#endif /* USB_VENDOR_DESC_H */
