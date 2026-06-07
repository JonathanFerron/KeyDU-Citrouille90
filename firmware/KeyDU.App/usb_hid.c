#include "usb_hid.h"
#include "keyboard.h"
#include "../usbcore/usb_types.h"
#include "../usbcore/usb_ep.h"
#include "../usbcore/usb_ep_stream.h"
#include "../usbcore/usb_ctrl.h"
#include "../avrducore/clock.h"
#include <string.h>

/* ════════════════════════════════════════════════════════════════════════
    State
   ════════════════════════════════════════════════════════════════════════ */

hid_protocol_t hid_protocol[HID_IFACE_COUNT] =
{ [HID_IFACE_KBD]      = HID_PROTO_REPORT,
  [HID_IFACE_CONSUMER] = HID_PROTO_REPORT,
};

/* SET_IDLE period per interface (units = 4 ms; 0 = send only on change)    */
static uint8_t s_idle_rate[HID_IFACE_COUNT];

/* LED output report: last value received from host via SET_REPORT or EP OUT */
static volatile hid_led_report_t s_led_report;

/* ── Keyboard report queue (Phase 2) ────────────────────────────────────

   Replaces the seqlock double-buffer for keyboard reports.
   SPSC lockless ring buffer — head touched only by hid_flush() (SOF ISR),
   tail touched only by kbd_stage() (main context).
   uint8_t indices are written atomically on AVR — no lock needed.

   Consumer report retains its seqlock — it is staged infrequently and
   its 4-byte report does not need the multi-entry queue.
   ──────────────────────────────────────────────────────────────────────── */
kbd_queue_entry_t  kbd_queue[KBD_QUEUE_DEPTH];
volatile uint8_t   kbd_queue_head = 0;
volatile uint8_t   kbd_queue_tail = 0;

/* Consumer report retains seqlock from phase 1 */
static volatile uint8_t       s_con_seq;
static hid_consumer_report_t  s_con_buf;

/* ════════════════════════════════════════════════════════════════════════
    hid_configure — call from usb_event_config_changed()
   ════════════════════════════════════════════════════════════════════════ */
void hid_configure(void)
{ ep_configure(HID_EP_KBD_IN,      EP_TYPE_INTERRUPT, HID_EP_SIZE_KBD,      1);
  ep_configure(HID_EP_KBD_OUT,     EP_TYPE_INTERRUPT, HID_EP_SIZE_KBD,      1);
  ep_configure(HID_EP_CONSUMER_IN, EP_TYPE_INTERRUPT, HID_EP_SIZE_CONSUMER, 1);

  /* Reset queue — safe here since hid_flush() cannot preempt during init */
  kbd_queue_head = 0;
  kbd_queue_tail = 0;
  memset(kbd_queue, 0, sizeof(kbd_queue));

  memset(&s_con_buf, 0, sizeof(s_con_buf));
  s_con_buf.report_id = 0x01;
  s_con_seq = 0;
  s_led_report = 0;
}

/* ════════════════════════════════════════════════════════════════════════
    kbd_stage — enqueue one keyboard report (main-loop context)

    Replaces the seqlock write from phase 1.
    Full queue: silently drop — see usb_hid.h for sizing guidance.
    Entry is written before tail is advanced so hid_flush() never sees
    a partially-written slot.
   ════════════════════════════════════════════════════════════════════════ */
/* Internal enqueue — shared by kbd_stage() (keyboard.c) and kbd_stage_wait() */
static void kbd_enqueue(const kbd_queue_entry_t* entry)
{ uint8_t next_tail = (kbd_queue_tail + 1u) & KBD_QUEUE_MASK;
  if(next_tail == kbd_queue_head) return;    /* full — drop */
  kbd_queue[kbd_queue_tail] = *entry;        /* write data first */
  USB_MEMORY_BARRIER();
  kbd_queue_tail = next_tail;                /* then advance tail */
}

/* hid_kbd_stage — called from keyboard.c / kbd_stage() */
void hid_kbd_stage(const hid_kbd_report_t* r)
{ kbd_queue_entry_t entry;
  entry.type   = KBD_QUEUE_REPORT;
  memcpy(&entry.report, r, sizeof(entry.report));
  kbd_enqueue(&entry);
}

/* kbd_stage_wait — enqueue a wait sentinel (N SOF ticks of silence).
   Called from run_macro_sequence() in phase 2, replacing _delay_ms(). */
void kbd_stage_wait(uint8_t n_sofs)
{ kbd_queue_entry_t entry;
  entry.type      = KBD_QUEUE_WAIT;
  entry.wait_sofs = n_sofs;
  kbd_enqueue(&entry);
}

/* ════════════════════════════════════════════════════════════════════════
   Phase 2 — macro.c MACRO_ACTION_WAIT update

   In macro.c, replace the MACRO_ACTION_WAIT case:

     // Phase 1 (remove):
     for (volatile uint8_t w = action.keycode; w; w--)
         for (volatile uint16_t d = 0; d < 2400u; d++) {}

     // Phase 2 (replace with):
     kbd_stage_wait(action.keycode);

   Also add #include "usb_hid.h" to macro.c (or expose kbd_stage_wait
   via keyboard.h — either is acceptable).
   ════════════════════════════════════════════════════════════════════════ */

/* Consumer report retains seqlock — unchanged from phase 1 */
void hid_consumer_stage(const hid_consumer_report_t* r)
{ s_con_seq++;
  USB_MEMORY_BARRIER();
  memcpy(&s_con_buf, r, sizeof(s_con_buf));
  USB_MEMORY_BARRIER();
  s_con_seq++;
}

/* ════════════════════════════════════════════════════════════════════════
    hid_flush — call once per SOF from usb_event_sof()

    Keyboard: dequeues one entry from kbd_queue.
      KBD_QUEUE_REPORT: sends report to EP1 IN this SOF.
      KBD_QUEUE_WAIT:   decrements wait_sofs; discards when zero,
                        sending nothing to host this SOF.
    Consumer: unchanged seqlock path from phase 1.
    LED OUT: unchanged from phase 1.

    For each IN endpoint:
      1. Read seqlock; skip if odd (write in progress)
      2. Copy report to local stack snapshot
      3. Re-check seqlock; skip if changed (torn read)
      4. ep_select + ep_in_ready check; if ready, write snapshot

    Also polls EP1 OUT for incoming LED reports.
   ════════════════════════════════════════════════════════════════════════ */

static void flush_kbd(void)
{ /* Empty queue — nothing to send */
  if(kbd_queue_head == kbd_queue_tail) return;

  kbd_queue_entry_t* entry = &kbd_queue[kbd_queue_head];

  if(entry->type == KBD_QUEUE_WAIT)
  { /* Countdown sentinel — decrement and discard when expired */
    if(--entry->wait_sofs == 0)
    { USB_MEMORY_BARRIER();
      kbd_queue_head = (kbd_queue_head + 1u) & KBD_QUEUE_MASK;
    }
    return;   /* send nothing to host this SOF */
  }

  /* KBD_QUEUE_REPORT — send and pop */
  ep_select(HID_EP_KBD_IN);
  if(!ep_in_ready()) return;    /* endpoint busy — retry next SOF */
  ep_write_stream(&entry->report, sizeof(entry->report), NULL);
  ep_clear_in();
  USB_MEMORY_BARRIER();
  kbd_queue_head = (kbd_queue_head + 1u) & KBD_QUEUE_MASK;
}

static void flush_consumer(void)
{ uint8_t s0 = s_con_seq;
  if(s0 & 1u) return;
  USB_MEMORY_BARRIER();
  hid_consumer_report_t snap;
  memcpy(&snap, &s_con_buf, sizeof(snap));
  USB_MEMORY_BARRIER();
  if(s_con_seq != s0) return;

  ep_select(HID_EP_CONSUMER_IN);
  if(!ep_in_ready()) return;
  ep_write_stream(&snap, sizeof(snap), NULL);
  ep_clear_in();
}

static void poll_led_out(void)
{ ep_select(HID_EP_KBD_OUT);
  if(!ep_out_received()) return;
  s_led_report = (hid_led_report_t)ep_read_u8();
  ep_clear_out();
}

void hid_flush(void)
{ if(usb_device_state != USB_STATE_CONFIGURED) return;
  flush_kbd();
  flush_consumer();
  poll_led_out();
}

hid_led_report_t hid_get_led_report(void)
{ return s_led_report;
}

/* ════════════════════════════════════════════════════════════════════════
    usb_event_config_changed — overrides weak stub in usb_ctrl.c
   ════════════════════════════════════════════════════════════════════════ */
void usb_event_config_changed(void)
{ hid_configure();
  clock_autotune_enable();
  usb_sof_enable();
}

/* ════════════════════════════════════════════════════════════════════════
    usb_event_ctrl_request — overrides weak stub in usb_ctrl.c

    Handles all six mandatory HID class requests for both interfaces.
    If the request is handled, ep_clear_setup() + ep_complete_ctrl_status()
    are called so usb_ctrl.c's dispatcher sees SETUP cleared and skips stall.
    Unrecognised requests fall through without touching SETUP — the standard
    dispatcher will stall them.
   ════════════════════════════════════════════════════════════════════════ */
void usb_event_ctrl_request(void)
{ /* Class requests to an interface only */
  if((usb_ctrl_req.bm_request_type & REQ_TYPE_MASK) != REQ_TYPE_CLASS) return;

  const uint8_t iface = (uint8_t)(usb_ctrl_req.w_index & 0xFFu);
  if(iface >= HID_IFACE_COUNT) return;

  switch(usb_ctrl_req.b_request)
  {

    case HID_REQ_GET_REPORT:
    { /* Return current staged report for the requested interface          */
      ep_select(EP_CTRL);
      ep_clear_setup();
      if(iface == HID_IFACE_KBD)
      { hid_kbd_report_t snap;
        kbd_get_report(&snap);
        ep_write_ctrl_stream(&snap, sizeof(snap));
      }
      else
        ep_write_ctrl_stream(&s_con_buf, sizeof(s_con_buf));
      ep_complete_ctrl_status();
      break;
    }

    case HID_REQ_SET_REPORT:
    { /* Interface 0 only: receive 1-byte LED report                      */
      ep_select(EP_CTRL);
      ep_clear_setup();
      if(iface == HID_IFACE_KBD)
      { uint8_t led = 0;
        ep_read_ctrl_stream(&led, 1);
        s_led_report = (hid_led_report_t)led;
      }
      else
      { /* No output report on interface 1 — accept and discard         */
        uint8_t dummy;
        ep_read_ctrl_stream(&dummy, usb_ctrl_req.w_length);
      }
      ep_complete_ctrl_status();
      break;
    }

    case HID_REQ_SET_IDLE:
      s_idle_rate[iface] = (uint8_t)(usb_ctrl_req.w_value >> 8);
      ep_select(EP_CTRL);
      ep_clear_setup();
      ep_complete_ctrl_status();
      break;

    case HID_REQ_GET_IDLE:
      ep_select(EP_CTRL);
      ep_clear_setup();
      ep_write_ctrl_stream(&s_idle_rate[iface], 1);
      ep_complete_ctrl_status();
      break;

    case HID_REQ_SET_PROTOCOL:
      hid_protocol[iface] = (hid_protocol_t)(usb_ctrl_req.w_value & 0xFFu);
      ep_select(EP_CTRL);
      ep_clear_setup();
      ep_complete_ctrl_status();
      break;

    case HID_REQ_GET_PROTOCOL:
      ep_select(EP_CTRL);
      ep_clear_setup();
      ep_write_ctrl_stream(&hid_protocol[iface], 1);
      ep_complete_ctrl_status();
      break;

    default:
      break;  /* unrecognised — fall through, usb_ctrl.c will stall       */
  }
} // usb_event_ctrl_request

/* Called from USB0_BUSEVENT_vect once per 1 ms SOF when the bus is active.
   Flushes staged keyboard and consumer reports to their IN endpoints.
   hid_flush() is a no-op if the device is not in USB_STATE_CONFIGURED,
   so no guard is needed here.

   Note: do not gate this on tick_flag or any scan-loop state.
   The SOF ISR and the scan loop are independent — hid_flush() reads
   the seqlock double-buffer, so concurrent access is safe. */
void usb_event_sof(void)
{ hid_flush();
}
