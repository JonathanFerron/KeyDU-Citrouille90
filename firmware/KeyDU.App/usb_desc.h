#ifndef USB_DESC_H
#define USB_DESC_H

#include "../usbcore/usb_types.h"
#include <avr/pgmspace.h>

/* ── Identity — edit here only ─────────────────────────────────────────── */
// ============================================================================
// Keyboard Information
// ============================================================================
#define USB_VID                 0x1209u          /* pid.codes open-source VID  */
#define USB_PID                 0xB4B0u          /* KeyDU.App: not yet assigned by pid.codes */
#define USB_DEVICE_VERSION      VERSION_BCD(1,0,0)

#define USB_STR_MANUFACTURER    L"KeyDU project"
#define USB_STR_PRODUCT         L"Citrouille90"
/* Serial number: auto-generated from AVR SIGROW by usb_ctrl.c              */

/* ── String descriptor indices ─────────────────────────────────────────── */
#define STRIDX_LANG             0u
#define STRIDX_MANUFACTURER     1u
#define STRIDX_PRODUCT          2u
/* 0xDC = internal serial shortcut handled directly in usb_ctrl.c           */

/* ── Interface numbers (§11.1) ─────────────────────────────────────────── */
#define HID_IFACE_KBD           0u   /* boot keyboard  — EP1 IN + EP1 OUT   */
#define HID_IFACE_CONSUMER      1u   /* consumer/sys   — EP2 IN             */
#define HID_IFACE_COUNT         2u

/* ── Endpoint addresses ─────────────────────────────────────────────────── */
#define HID_EP_KBD_IN           (EP_DIR_IN  | 1u)
#define HID_EP_KBD_OUT          (EP_DIR_OUT | 1u)   /* LED output report     */
#define HID_EP_CONSUMER_IN      (EP_DIR_IN  | 2u)

/* ── Endpoint bank sizes ────────────────────────────────────────────────── */
#define HID_EP_SIZE_KBD         8u    /* 8-byte boot keyboard report         */
#define HID_EP_SIZE_CONSUMER    4u    /* report-id(1) + consumer(2) + sys(1) */
#define HID_EP_INTERVAL_MS      1u    /* 1 ms polling interval               */

/* ── HID class constants (not in usb_types.h) ───────────────────────────── */
#define HID_CLASS               0x03u
#define HID_SUBCLASS_BOOT       0x01u
#define HID_SUBCLASS_NONE       0x00u
#define HID_PROTOCOL_KEYBOARD   0x01u
#define HID_PROTOCOL_NONE       0x00u
#define HID_DTYPE_HID           0x21u
#define HID_DTYPE_REPORT        0x22u
#define HID_SPEC_VERSION        VERSION_BCD(1,1,1)

/* ── HID class request codes ────────────────────────────────────────────── */
#define HID_REQ_GET_REPORT      0x01u
#define HID_REQ_GET_IDLE        0x02u
#define HID_REQ_GET_PROTOCOL    0x03u
#define HID_REQ_SET_REPORT      0x09u
#define HID_REQ_SET_IDLE        0x0Au
#define HID_REQ_SET_PROTOCOL    0x0Bu

/* ── HID descriptor struct (USB-IF HID 1.11 §6.2.1) ────────────────────── */
typedef struct USB_PACKED {
    uint8_t  b_length;
    uint8_t  b_descriptor_type;          /* HID_DTYPE_HID                    */
    uint16_t bcd_hid;
    uint8_t  b_country_code;
    uint8_t  b_num_descriptors;          /* always 1                         */
    uint8_t  b_report_descriptor_type;   /* HID_DTYPE_REPORT                 */
    uint16_t w_report_descriptor_length;
} usb_desc_hid_t;

/* ── Full configuration descriptor layout ──────────────────────────────── */
typedef struct USB_PACKED {
    usb_desc_config_t    cfg;
    /* Interface 0 — boot keyboard */
    usb_desc_interface_t iface_kbd;
    usb_desc_hid_t       hid_kbd;
    usb_desc_endpoint_t  ep_kbd_in;
    usb_desc_endpoint_t  ep_kbd_out;     /* LED output report                */
    /* Interface 1 — consumer/system */
    usb_desc_interface_t iface_consumer;
    usb_desc_hid_t       hid_consumer;
    usb_desc_endpoint_t  ep_consumer_in;
} usb_config_desc_t;

/* ── Required usbcore callback — implemented in usb_desc.c ─────────────── */
uint16_t usb_get_desc(uint16_t w_value, uint16_t w_index, const void **addr);

#endif /* USB_DESC_H */
