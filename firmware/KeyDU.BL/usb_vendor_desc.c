/*
 * usb_vendor_desc.c — USB descriptor tables for KeyDU.BL
 * SPDX-License-Identifier: GPL-3.0-or-later
 * Copyright (C) 2026 Jonathan Ferron
 *
 * Vendor class, single interface, EP0 control transfers only.
 * No bulk endpoints declared — the flash protocol is entirely EP0-based.
 *
 * usb_get_desc() is the required usbcore callback; it is called by
 * usb_ctrl.c's GET_DESCRIPTOR handler.
 */

#include "usb_vendor_desc.h"
#include "usb_vendor.h"          /* VENDOR_VID, VENDOR_PID, string literals */
#include "../../usbcore/usb_types.h"
#include <avr/pgmspace.h>

/* ============================================================================
 * Device descriptor
 * ========================================================================= */
static const usb_desc_device_t PROGMEM dev_desc = {
    .b_length            = sizeof(usb_desc_device_t),
    .b_descriptor_type   = DTYPE_DEVICE,
    .bcd_usb             = VERSION_BCD(2,0,0),
    .b_device_class      = 0xFF,   /* Vendor-defined at device level          */
    .b_device_sub_class  = 0xFF,
    .b_device_protocol   = 0xFF,
    .b_max_packet_size0  = EP_CTRL_SIZE_DEFAULT,   /* 8 bytes                 */
    .id_vendor           = VENDOR_VID,
    .id_product          = VENDOR_PID,
    .bcd_device          = VERSION_BCD(0,1,0),
    .i_manufacturer      = VENDOR_STRIDX_MANUFACTURER,
    .i_product           = VENDOR_STRIDX_PRODUCT,
    .i_serial_number     = 0xDC,   /* triggers internal SIGROW serial in usb_ctrl.c */
    .b_num_configurations = 1,
};

/* ============================================================================
 * Configuration descriptor layout
 * Single interface, zero non-control endpoints.
 * ========================================================================= */
typedef struct USB_PACKED {
    usb_desc_config_t    cfg;
    usb_desc_interface_t iface;
} usb_vendor_config_desc_t;

static const usb_vendor_config_desc_t PROGMEM cfg_desc = {

    .cfg = {
        .b_length              = sizeof(usb_desc_config_t),
        .b_descriptor_type     = DTYPE_CONFIGURATION,
        .w_total_length        = sizeof(usb_vendor_config_desc_t),
        .b_num_interfaces      = 1,
        .b_configuration_value = 1,
        .i_configuration       = 0,
        .bm_attributes         = USB_CFG_ATTR_RESERVED,
        .b_max_power           = USB_CONFIG_POWER_MA(100),
    },

    .iface = {
        .b_length              = sizeof(usb_desc_interface_t),
        .b_descriptor_type     = DTYPE_INTERFACE,
        .b_interface_number    = 0,
        .b_alternate_setting   = 0,
        .b_num_endpoints       = 0,   /* EP0 only                              */
        .b_interface_class     = 0xFF,
        .b_interface_sub_class = 0xFF,
        .b_interface_protocol  = 0xFF,
        .i_interface           = 0,
    },
};

/* ============================================================================
 * String descriptors
 * ========================================================================= */
static const struct USB_PACKED { uint8_t bl; uint8_t bt; uint16_t lang; }
PROGMEM str_lang = {
    .bl   = USB_STRING_LEN(1),
    .bt   = DTYPE_STRING,
    .lang = LANG_ID_ENG,
};

static const struct USB_PACKED {
    uint8_t bl; uint8_t bt;
    wchar_t s[sizeof(VENDOR_STR_MANUFACTURER)/sizeof(wchar_t) - 1];
} PROGMEM str_mfr = {
    .bl = USB_STRING_LEN(sizeof(VENDOR_STR_MANUFACTURER)/sizeof(wchar_t) - 1),
    .bt = DTYPE_STRING,
    .s  = VENDOR_STR_MANUFACTURER,
};

static const struct USB_PACKED {
    uint8_t bl; uint8_t bt;
    wchar_t s[sizeof(VENDOR_STR_PRODUCT)/sizeof(wchar_t) - 1];
} PROGMEM str_prod = {
    .bl = USB_STRING_LEN(sizeof(VENDOR_STR_PRODUCT)/sizeof(wchar_t) - 1),
    .bt = DTYPE_STRING,
    .s  = VENDOR_STR_PRODUCT,
};

/* ============================================================================
 * usb_get_desc — required usbcore callback
 * Called by usb_ctrl.c GET_DESCRIPTOR handler.
 * ========================================================================= */
uint16_t usb_get_desc(uint16_t w_value, uint16_t w_index, const void **addr)
{
    (void)w_index;

    const uint8_t dtype = (uint8_t)(w_value >> 8);
    const uint8_t didx  = (uint8_t)(w_value & 0xFFu);

    switch (dtype) {

    case DTYPE_DEVICE:
        *addr = &dev_desc;
        return sizeof(dev_desc);

    case DTYPE_CONFIGURATION:
        *addr = &cfg_desc;
        return sizeof(cfg_desc);

    case DTYPE_STRING:
        switch (didx) {
        case VENDOR_STRIDX_LANG:
            *addr = &str_lang;
            return pgm_read_byte(&str_lang.bl);
        case VENDOR_STRIDX_MANUFACTURER:
            *addr = &str_mfr;
            return pgm_read_byte(&str_mfr.bl);
        case VENDOR_STRIDX_PRODUCT:
            *addr = &str_prod;
            return pgm_read_byte(&str_prod.bl);
        default:
            return NO_DESCRIPTOR;
        }

    default:
        return NO_DESCRIPTOR;
    }
}
