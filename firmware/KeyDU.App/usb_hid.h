#ifndef USB_HID_H
#define USB_HID_H

#include <stdint.h>
#include <stdbool.h>
#include "usb_desc.h"

/* ════════════════════════════════════════════════════════════════════════
    HID report structs (§11.2, §11.4)
    Sizes must match HID_EP_SIZE_* in usb_desc.h.
   ════════════════════════════════════════════════════════════════════════ */

/* Interface 0 — boot keyboard, 8 bytes, no report-id prefix (§11.2) */
typedef struct __attribute__((packed))
{ uint8_t modifier;       /* HID modifier bitmask                         */
  uint8_t reserved;       /* always 0x00                                  */
  uint8_t keycode[6];     /* up to 6 simultaneous keycodes (6KRO)         */
}
hid_kbd_report_t;

/* Interface 0 — LED output report, 1 byte, no report-id (§11.3)
   bit0=NumLock bit1=CapsLock bit2=ScrollLock bit3=Compose bit4=Kana        */
typedef uint8_t hid_led_report_t;

/* Interface 1 — consumer/system, 4 bytes, report-id prefix = 0x01 (§11.4) */
typedef struct __attribute__((packed))
{ uint8_t  report_id;     /* always 0x01                                  */
  uint16_t consumer;      /* HID Consumer page usage (0x0000 = none)      */
  uint8_t  system;        /* bit0 = System Sleep (GD page 0x82)           */
}
hid_consumer_report_t;

_Static_assert(sizeof(hid_kbd_report_t)      == HID_EP_SIZE_KBD,      "kbd report size mismatch");
_Static_assert(sizeof(hid_consumer_report_t) == HID_EP_SIZE_CONSUMER,  "consumer report size mismatch");

/* ════════════════════════════════════════════════════════════════════════
    HID protocol state (per interface, toggled by SET_PROTOCOL)
   ════════════════════════════════════════════════════════════════════════ */
typedef enum { HID_PROTO_BOOT = 0, HID_PROTO_REPORT = 1 } hid_protocol_t;
extern hid_protocol_t hid_protocol[HID_IFACE_COUNT];

/* ════════════════════════════════════════════════════════════════════════
    Keyboard report queue

    kbd_stage() enqueues one entry; hid_flush() dequeues and sends one
    entry per call (rate-limited by ep_in_ready()).

    Design constraints:
      - head and tail are uint8_t — AVR 8-bit R/W is atomic, no lock needed
      - depth must be a power of 2 — index wraps with bitmask, no modulo
      - SPSC: kbd_stage() (main context) writes tail only;
              hid_flush() (main loop) reads head only
      - full queue: new entry is silently dropped (zero flash overhead)
      - empty queue: hid_flush() checks head==tail before reading any slot

    Sizing: count kbd_stage() calls for your worst-case sequence, add 2
    for headroom, round up to next power of 2.  Each slot is
    sizeof(kbd_queue_entry_t) bytes — see below.

    Entry types:
      KBD_QUEUE_REPORT — normal keyboard report, sent on next hid_flush()
      KBD_QUEUE_WAIT   — sentinel: hid_flush() decrements wait_sofs each
                         call and discards the entry when it reaches zero,
                         sending nothing to the host during the countdown.
                         Intended replacement for MACRO_ACTION_WAIT busy-wait.
   ════════════════════════════════════════════════════════════════════════ */

/* Queue depth — power of 2.  Adjust after counting worst-case sequence. */
#define KBD_QUEUE_DEPTH   8u
#define KBD_QUEUE_MASK    (KBD_QUEUE_DEPTH - 1u)

typedef enum
{ KBD_QUEUE_REPORT = 0,   /* normal keyboard report      */
  KBD_QUEUE_WAIT   = 1,   /* wait sentinel — N SOF ticks */
} kbd_queue_entry_type_t;

typedef struct
{ kbd_queue_entry_type_t type;   /* entry type tag              */
  union
  { hid_kbd_report_t report;   /* KBD_QUEUE_REPORT: 8 bytes   */
    uint8_t          wait_sofs;/* KBD_QUEUE_WAIT: countdown   */
  };
} kbd_queue_entry_t;

/* The queue itself — defined in usb_hid.c.
   head: read by hid_flush() (main loop).
   tail: written by kbd_stage() (main context).
   Both are uint8_t — atomic on AVR. */
extern kbd_queue_entry_t  kbd_queue[KBD_QUEUE_DEPTH];
extern volatile uint8_t   kbd_queue_head;
extern volatile uint8_t   kbd_queue_tail;

/* Enqueue a wait sentinel (N SOF ticks of silence).
   Called from run_macro_sequence() in place of _delay_ms().
   If the queue is full the sentinel is silently dropped — the macro
   delay will be shorter than requested but no deadlock occurs. */
void kbd_stage_wait(uint8_t n_sofs);

/* ════════════════════════════════════════════════════════════════════════
    API

    Lifecycle:
      hid_configure() — call from usb_event_config_changed() to set up EPs

    Report submission (scan-loop / main context):
      hid_kbd_stage()      — latch new keyboard report into back-buffer
      hid_consumer_stage() — latch new consumer report into back-buffer

    Report flush (main loop, before usb_ctrl_poll()):
      hid_flush()          — push staged reports to USB IN endpoints

    LED output report (called from usb_event_config_changed or OUT poll):
      hid_get_led_report() — last LED report received from host

    Non-blocking guarantee:
      hid_kbd_stage() and hid_consumer_stage() MUST NOT block.
      hid_kbd_stage() enqueues into kbd_queue; hid_consumer_stage() writes
      to a seqlock-protected back-buffer. Both return immediately.
      Required because encoder_step() may call kbd_stage() multiple times
      within a single 1 ms scan tick.
   ════════════════════════════════════════════════════════════════════════ */

/* Configure HID endpoints — call from usb_event_config_changed() */
void hid_configure(void);

/* Stage a new keyboard report. Non-blocking. */
void hid_kbd_stage(const hid_kbd_report_t* r);

/* Stage a new consumer/system report. Non-blocking. */
void hid_consumer_stage(const hid_consumer_report_t* r);

/* Flush staged reports to IN endpoints. Call once per USB frame. */
void hid_flush(void);

/* Return last LED state received from host (via SET_REPORT or EP1 OUT) */
hid_led_report_t hid_get_led_report(void);

/* ════════════════════════════════════════════════════════════════════════
    usb_event_ctrl_request() — overrides the weak stub in usb_ctrl.c
    Handles all HID class requests for both interfaces.
   ════════════════════════════════════════════════════════════════════════ */
void usb_event_ctrl_request(void);

/* usb_event_config_changed() — overrides the weak stub in usb_ctrl.c */
void usb_event_config_changed(void);

#endif /* USB_HID_H */
