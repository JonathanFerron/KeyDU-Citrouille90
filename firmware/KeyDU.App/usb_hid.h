#ifndef USB_HID_H
#define USB_HID_H

#include <stdint.h>
#include <stdbool.h>
#include "usb_desc.h"

/* ════════════════════════════════════════════════════════════════════════
 *  HID report structs (§11.2, §11.4)
 *  Sizes must match HID_EP_SIZE_* in usb_desc.h.
 * ════════════════════════════════════════════════════════════════════════ */

/* Interface 0 — boot keyboard, 8 bytes, no report-id prefix (§11.2) */
typedef struct __attribute__((packed)) {
    uint8_t modifier;       /* HID modifier bitmask                         */
    uint8_t reserved;       /* always 0x00                                  */
    uint8_t keycode[6];     /* up to 6 simultaneous keycodes (6KRO)         */
} hid_kbd_report_t;

/* Interface 0 — LED output report, 1 byte, no report-id (§11.3)
   bit0=NumLock bit1=CapsLock bit2=ScrollLock bit3=Compose bit4=Kana        */
typedef uint8_t hid_led_report_t;

/* Interface 1 — consumer/system, 4 bytes, report-id prefix = 0x01 (§11.4) */
typedef struct __attribute__((packed)) {
    uint8_t  report_id;     /* always 0x01                                  */
    uint16_t consumer;      /* HID Consumer page usage (0x0000 = none)      */
    uint8_t  system;        /* bit0 = System Sleep (GD page 0x82)           */
} hid_consumer_report_t;

_Static_assert(sizeof(hid_kbd_report_t)      == HID_EP_SIZE_KBD,      "kbd report size mismatch");
_Static_assert(sizeof(hid_consumer_report_t) == HID_EP_SIZE_CONSUMER,  "consumer report size mismatch");

/* ════════════════════════════════════════════════════════════════════════
 *  HID protocol state (per interface, toggled by SET_PROTOCOL)
 * ════════════════════════════════════════════════════════════════════════ */
typedef enum { HID_PROTO_BOOT = 0, HID_PROTO_REPORT = 1 } hid_protocol_t;
extern hid_protocol_t hid_protocol[HID_IFACE_COUNT];

/* ════════════════════════════════════════════════════════════════════════
 *  API
 *
 *  Lifecycle:
 *    hid_configure() — call from usb_event_config_changed() to set up EPs
 *
 *  Report submission (scan-loop / main context):
 *    hid_kbd_stage()      — latch new keyboard report into back-buffer
 *    hid_consumer_stage() — latch new consumer report into back-buffer
 *
 *  Report flush (SOF ISR or tight main loop, before usb_task()):
 *    hid_flush()          — push staged reports to USB IN endpoints
 *
 *  LED output report (called from usb_event_config_changed or OUT poll):
 *    hid_get_led_report() — last LED report received from host
 *
 *  Non-blocking guarantee:
 *    hid_kbd_stage() and hid_consumer_stage() MUST NOT block. They write
 *    to a back-buffer protected by a seqlock and return immediately. This
 *    is required because encoder_step() may call send_keyboard_report()
 *    multiple times inline during the 1 ms scan window. (§11.5)
 * ════════════════════════════════════════════════════════════════════════ */

/* Configure HID endpoints — call from usb_event_config_changed() */
void hid_configure(void);

/* Stage a new keyboard report. Non-blocking. */
void hid_kbd_stage(const hid_kbd_report_t *r);

/* Stage a new consumer/system report. Non-blocking. */
void hid_consumer_stage(const hid_consumer_report_t *r);

/* Flush staged reports to IN endpoints. Call once per USB frame. */
void hid_flush(void);

/* Return last LED state received from host (via SET_REPORT or EP1 OUT) */
hid_led_report_t hid_get_led_report(void);

/* ════════════════════════════════════════════════════════════════════════
 *  usb_event_ctrl_request() — overrides the weak stub in usb_ctrl.c
 *  Handles all HID class requests for both interfaces.
 * ════════════════════════════════════════════════════════════════════════ */
void usb_event_ctrl_request(void);

/* usb_event_config_changed() — overrides the weak stub in usb_ctrl.c */
void usb_event_config_changed(void);

#endif /* USB_HID_H */
