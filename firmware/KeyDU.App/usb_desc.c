#include "usb_desc.h"

/* ════════════════════════════════════════════════════════════════════════
 *  HID report descriptors
 * ════════════════════════════════════════════════════════════════════════ */

/* Boot-compatible keyboard — 8 bytes: modifier(1) reserved(1) keycodes(6)
   Includes LED output report for Num/Caps/Scroll/Compose/Kana             */
static const uint8_t PROGMEM rd_kbd[] = {
    0x05,0x01, 0x09,0x06, 0xA1,0x01,       /* Usage Page Generic Desktop, Keyboard  */
    /* Modifier byte */
    0x05,0x07, 0x19,0xE0, 0x29,0xE7,
    0x15,0x00, 0x25,0x01, 0x75,0x01, 0x95,0x08, 0x81,0x02,
    /* Reserved byte */
    0x95,0x01, 0x75,0x08, 0x81,0x01,
    /* Keycodes[6] */
    0x19,0x00, 0x29,0xDD,
    0x15,0x00, 0x25,0xDD, 0x75,0x08, 0x95,0x06, 0x81,0x00,
    /* LED output report: Num(0) Caps(1) Scroll(2) Compose(3) Kana(4) + 3 pad */
    0x05,0x08, 0x19,0x01, 0x29,0x05,
    0x15,0x00, 0x25,0x01, 0x75,0x01, 0x95,0x05, 0x91,0x02,
    0x75,0x03, 0x95,0x01, 0x91,0x01,
    0xC0
};

/* Consumer/system — 4 bytes: report-id(1) consumer-usage(2) sys-bits(1)   */
static const uint8_t PROGMEM rd_consumer[] = {
    0x05,0x0C, 0x09,0x01, 0xA1,0x01,       /* Usage Page Consumer Control           */
    0x85,0x01,                              /*   Report ID 1                         */
    /* 16-bit consumer usage (0x0000 = none) */
    0x15,0x00, 0x26,0xFF,0x03,
    0x19,0x00, 0x2A,0xFF,0x03,
    0x75,0x10, 0x95,0x01, 0x81,0x00,
    /* System byte: b0 = System Sleep (GD 0x82), b1-7 padding */
    0x05,0x01, 0x09,0x82,
    0x15,0x00, 0x25,0x01, 0x75,0x01, 0x95,0x01, 0x81,0x02,
    0x75,0x07, 0x95,0x01, 0x81,0x01,
    0xC0
};

/* ════════════════════════════════════════════════════════════════════════
 *  Device descriptor
 * ════════════════════════════════════════════════════════════════════════ */
static const usb_desc_device_t PROGMEM dev_desc = {
    .b_length            = sizeof(usb_desc_device_t),
    .b_descriptor_type   = DTYPE_DEVICE,
    .bcd_usb             = VERSION_BCD(2,0,0),
    .b_device_class      = 0x00,    /* class defined at interface level      */
    .b_device_sub_class  = 0x00,
    .b_device_protocol   = 0x00,
    .b_max_packet_size0  = 8,       /* FIXED_CONTROL_ENDPOINT_SIZE           */
    .id_vendor           = USB_VID,
    .id_product          = USB_PID,
    .bcd_device          = USB_DEVICE_VERSION,
    .i_manufacturer      = STRIDX_MANUFACTURER,
    .i_product           = STRIDX_PRODUCT,
    .i_serial_number     = 0xDC,    /* triggers internal serial in usb_ctrl  */
    .b_num_configurations = 1,
};

/* ════════════════════════════════════════════════════════════════════════
 *  Configuration descriptor
 * ════════════════════════════════════════════════════════════════════════ */
static const usb_config_desc_t PROGMEM cfg_desc = {

    .cfg = {
        .b_length              = sizeof(usb_desc_config_t),
        .b_descriptor_type     = DTYPE_CONFIGURATION,
        .w_total_length        = sizeof(usb_config_desc_t),
        .b_num_interfaces      = HID_IFACE_COUNT,
        .b_configuration_value = 1,
        .i_configuration       = 0,
        .bm_attributes         = USB_CFG_ATTR_RESERVED,
        .b_max_power           = USB_CONFIG_POWER_MA(100),
    },

    /* ── Interface 0: boot keyboard ── */
    .iface_kbd = {
        .b_length              = sizeof(usb_desc_interface_t),
        .b_descriptor_type     = DTYPE_INTERFACE,
        .b_interface_number    = HID_IFACE_KBD,
        .b_alternate_setting   = 0,
        .b_num_endpoints       = 2,    /* EP1 IN + EP1 OUT                   */
        .b_interface_class     = HID_CLASS,
        .b_interface_sub_class = HID_SUBCLASS_BOOT,
        .b_interface_protocol  = HID_PROTOCOL_KEYBOARD,
        .i_interface           = 0,
    },
    .hid_kbd = {
        .b_length                   = sizeof(usb_desc_hid_t),
        .b_descriptor_type          = HID_DTYPE_HID,
        .bcd_hid                    = HID_SPEC_VERSION,
        .b_country_code             = 0,
        .b_num_descriptors          = 1,
        .b_report_descriptor_type   = HID_DTYPE_REPORT,
        .w_report_descriptor_length = sizeof(rd_kbd),
    },
    .ep_kbd_in = {
        .b_length            = sizeof(usb_desc_endpoint_t),
        .b_descriptor_type   = DTYPE_ENDPOINT,
        .b_endpoint_address  = HID_EP_KBD_IN,
        .bm_attributes       = EP_TYPE_INTERRUPT | EP_ATTR_NO_SYNC | EP_USAGE_DATA,
        .w_max_packet_size   = HID_EP_SIZE_KBD,
        .b_interval          = HID_EP_INTERVAL_MS,
    },
    .ep_kbd_out = {
        .b_length            = sizeof(usb_desc_endpoint_t),
        .b_descriptor_type   = DTYPE_ENDPOINT,
        .b_endpoint_address  = HID_EP_KBD_OUT,
        .bm_attributes       = EP_TYPE_INTERRUPT | EP_ATTR_NO_SYNC | EP_USAGE_DATA,
        .w_max_packet_size   = HID_EP_SIZE_KBD,
        .b_interval          = HID_EP_INTERVAL_MS,
    },

    /* ── Interface 1: consumer/system ── */
    .iface_consumer = {
        .b_length              = sizeof(usb_desc_interface_t),
        .b_descriptor_type     = DTYPE_INTERFACE,
        .b_interface_number    = HID_IFACE_CONSUMER,
        .b_alternate_setting   = 0,
        .b_num_endpoints       = 1,
        .b_interface_class     = HID_CLASS,
        .b_interface_sub_class = HID_SUBCLASS_NONE,
        .b_interface_protocol  = HID_PROTOCOL_NONE,
        .i_interface           = 0,
    },
    .hid_consumer = {
        .b_length                   = sizeof(usb_desc_hid_t),
        .b_descriptor_type          = HID_DTYPE_HID,
        .bcd_hid                    = HID_SPEC_VERSION,
        .b_country_code             = 0,
        .b_num_descriptors          = 1,
        .b_report_descriptor_type   = HID_DTYPE_REPORT,
        .w_report_descriptor_length = sizeof(rd_consumer),
    },
    .ep_consumer_in = {
        .b_length            = sizeof(usb_desc_endpoint_t),
        .b_descriptor_type   = DTYPE_ENDPOINT,
        .b_endpoint_address  = HID_EP_CONSUMER_IN,
        .bm_attributes       = EP_TYPE_INTERRUPT | EP_ATTR_NO_SYNC | EP_USAGE_DATA,
        .w_max_packet_size   = HID_EP_SIZE_CONSUMER,
        .b_interval          = HID_EP_INTERVAL_MS,
    },
};

/* ════════════════════════════════════════════════════════════════════════
 *  String descriptors
 *  Declared as raw byte arrays so we can use USB_STRING_LEN() and wchar_t
 *  literals without depending on a LUFA-style USB_STRING_DESCRIPTOR macro.
 * ════════════════════════════════════════════════════════════════════════ */
static const struct USB_PACKED { uint8_t bl; uint8_t bt; uint16_t lang; }
PROGMEM str_lang = {
    .bl   = USB_STRING_LEN(1),
    .bt   = DTYPE_STRING,
    .lang = LANG_ID_ENG,
};

static const struct USB_PACKED {
    uint8_t bl; uint8_t bt;
    wchar_t s[sizeof(USB_STR_MANUFACTURER)/sizeof(wchar_t) - 1];
} PROGMEM str_mfr = {
    .bl = USB_STRING_LEN(sizeof(USB_STR_MANUFACTURER)/sizeof(wchar_t) - 1),
    .bt = DTYPE_STRING,
    .s  = USB_STR_MANUFACTURER,
};

static const struct USB_PACKED {
    uint8_t bl; uint8_t bt;
    wchar_t s[sizeof(USB_STR_PRODUCT)/sizeof(wchar_t) - 1];
} PROGMEM str_prod = {
    .bl = USB_STRING_LEN(sizeof(USB_STR_PRODUCT)/sizeof(wchar_t) - 1),
    .bt = DTYPE_STRING,
    .s  = USB_STR_PRODUCT,
};

/* ════════════════════════════════════════════════════════════════════════
 *  usb_get_desc() — required usbcore callback
 * ════════════════════════════════════════════════════════════════════════ */
uint16_t usb_get_desc(uint16_t w_value, uint16_t w_index, const void **addr)
{
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
        case STRIDX_LANG:
            *addr = &str_lang;
            return pgm_read_byte(&str_lang.bl);
        case STRIDX_MANUFACTURER:
            *addr = &str_mfr;
            return pgm_read_byte(&str_mfr.bl);
        case STRIDX_PRODUCT:
            *addr = &str_prod;
            return pgm_read_byte(&str_prod.bl);
        default:
            return NO_DESCRIPTOR;
        }

    case HID_DTYPE_HID:
        /* Host requests the HID descriptor block by interface number        */
        switch (w_index) {
        case HID_IFACE_KBD:
            *addr = &cfg_desc.hid_kbd;
            return sizeof(usb_desc_hid_t);
        case HID_IFACE_CONSUMER:
            *addr = &cfg_desc.hid_consumer;
            return sizeof(usb_desc_hid_t);
        default:
            return NO_DESCRIPTOR;
        }

    case HID_DTYPE_REPORT:
        switch (w_index) {
        case HID_IFACE_KBD:
            *addr = rd_kbd;
            return sizeof(rd_kbd);
        case HID_IFACE_CONSUMER:
            *addr = rd_consumer;
            return sizeof(rd_consumer);
        default:
            return NO_DESCRIPTOR;
        }

    default:
        return NO_DESCRIPTOR;
    }
}
