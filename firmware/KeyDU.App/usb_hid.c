#include "usb_hid.h"
#include "../usbcore/usb_types.h"
#include "../usbcore/usb_ep.h"
#include "../usbcore/usb_ep_stream.h"
#include "../usbcore/usb_ctrl.h"
#include "../../avrducore/clock.h"
#include <string.h>

/* ════════════════════════════════════════════════════════════════════════
 *  State
 * ════════════════════════════════════════════════════════════════════════ */

hid_protocol_t hid_protocol[HID_IFACE_COUNT] = {
    [HID_IFACE_KBD]      = HID_PROTO_REPORT,
    [HID_IFACE_CONSUMER] = HID_PROTO_REPORT,
};

/* SET_IDLE period per interface (units = 4 ms; 0 = send only on change)    */
static uint8_t s_idle_rate[HID_IFACE_COUNT];

/* LED output report: last value received from host via SET_REPORT or EP OUT */
static volatile hid_led_report_t s_led_report;

/* ── Seqlock double-buffers ─────────────────────────────────────────────
 * seq even = stable snapshot; seq odd = write in progress.
 * hid_kbd_stage / hid_consumer_stage run in main context.
 * hid_flush runs in SOF ISR (or tight main loop just before usb_task).
 * On AVR there is no out-of-order execution, but the compiler can reorder
 * across volatile — USB_MEMORY_BARRIER() (defined in usb_types.h as a GCC
 * memory-clobber asm) prevents that without a real memory barrier instruction.
 * ──────────────────────────────────────────────────────────────────────── */
static volatile uint8_t       s_kbd_seq;
static volatile uint8_t       s_con_seq;
static hid_kbd_report_t      s_kbd_buf;
static hid_consumer_report_t s_con_buf;

/* ════════════════════════════════════════════════════════════════════════
 *  hid_configure — call from usb_event_config_changed()
 * ════════════════════════════════════════════════════════════════════════ */
void hid_configure(void)
{
    ep_configure(HID_EP_KBD_IN,      EP_TYPE_INTERRUPT, HID_EP_SIZE_KBD,      1);
    ep_configure(HID_EP_KBD_OUT,     EP_TYPE_INTERRUPT, HID_EP_SIZE_KBD,      1);
    ep_configure(HID_EP_CONSUMER_IN, EP_TYPE_INTERRUPT, HID_EP_SIZE_CONSUMER, 1);

    /* Clear report buffers and seqlocks on (re)configuration               */
    memset(&s_kbd_buf, 0, sizeof(s_kbd_buf));
    memset(&s_con_buf, 0, sizeof(s_con_buf));
    s_con_buf.report_id = 0x01;
    s_kbd_seq = 0;
    s_con_seq = 0;
    s_led_report = 0;
}

/* ════════════════════════════════════════════════════════════════════════
 *  Stage helpers (main-loop / scan context)
 * ════════════════════════════════════════════════════════════════════════ */
void hid_kbd_stage(const hid_kbd_report_t *r)
{
    s_kbd_seq++;                        /* odd  — write in progress          */
    USB_MEMORY_BARRIER();
    memcpy(&s_kbd_buf, r, sizeof(s_kbd_buf));
    USB_MEMORY_BARRIER();
    s_kbd_seq++;                        /* even — stable                     */
}

void hid_consumer_stage(const hid_consumer_report_t *r)
{
    s_con_seq++;
    USB_MEMORY_BARRIER();
    memcpy(&s_con_buf, r, sizeof(s_con_buf));
    USB_MEMORY_BARRIER();
    s_con_seq++;
}

/* ════════════════════════════════════════════════════════════════════════
 *  hid_flush — call once per USB frame (SOF ISR or just before usb_task)
 *
 *  For each IN endpoint:
 *    1. Read seqlock; skip if odd (write in progress)
 *    2. Copy report to local stack snapshot
 *    3. Re-check seqlock; skip if changed (torn read)
 *    4. ep_select + ep_in_ready check; if ready, write snapshot
 *
 *  Also polls EP1 OUT for incoming LED reports.
 * ════════════════════════════════════════════════════════════════════════ */

static void flush_kbd(void)
{
    uint8_t s0 = s_kbd_seq;
    if (s0 & 1u) return;
    USB_MEMORY_BARRIER();
    hid_kbd_report_t snap;
    memcpy(&snap, &s_kbd_buf, sizeof(snap));
    USB_MEMORY_BARRIER();
    if (s_kbd_seq != s0) return;        /* torn read — skip this frame       */

    ep_select(HID_EP_KBD_IN);
    if (!ep_in_ready()) return;
    ep_write_stream(&snap, sizeof(snap), NULL);
    ep_clear_in();
}

static void flush_consumer(void)
{
    uint8_t s0 = s_con_seq;
    if (s0 & 1u) return;
    USB_MEMORY_BARRIER();
    hid_consumer_report_t snap;
    memcpy(&snap, &s_con_buf, sizeof(snap));
    USB_MEMORY_BARRIER();
    if (s_con_seq != s0) return;

    ep_select(HID_EP_CONSUMER_IN);
    if (!ep_in_ready()) return;
    ep_write_stream(&snap, sizeof(snap), NULL);
    ep_clear_in();
}

static void poll_led_out(void)
{
    ep_select(HID_EP_KBD_OUT);
    if (!ep_out_received()) return;
    s_led_report = (hid_led_report_t)ep_read_u8();
    ep_clear_out();
}

void hid_flush(void)
{
    if (usb_device_state != USB_STATE_CONFIGURED) return;
    flush_kbd();
    flush_consumer();
    poll_led_out();
}

hid_led_report_t hid_get_led_report(void)
{
    return s_led_report;
}

/* ════════════════════════════════════════════════════════════════════════
 *  usb_event_config_changed — overrides weak stub in usb_ctrl.c
 * ════════════════════════════════════════════════════════════════════════ */
void usb_event_config_changed(void)
{
    hid_configure();
    clock_autotune_enable();
}

/* ════════════════════════════════════════════════════════════════════════
 *  usb_event_ctrl_request — overrides weak stub in usb_ctrl.c
 *
 *  Handles all six mandatory HID class requests for both interfaces.
 *  If the request is handled, ep_clear_setup() + ep_complete_ctrl_status()
 *  are called so usb_ctrl.c's dispatcher sees SETUP cleared and skips stall.
 *  Unrecognised requests fall through without touching SETUP — the standard
 *  dispatcher will stall them.
 * ════════════════════════════════════════════════════════════════════════ */
void usb_event_ctrl_request(void)
{
    /* Class requests to an interface only */
    if ((usb_ctrl_req.bm_request_type & REQ_TYPE_MASK) != REQ_TYPE_CLASS) return;

    const uint8_t iface = (uint8_t)(usb_ctrl_req.w_index & 0xFFu);
    if (iface >= HID_IFACE_COUNT) return;

    switch (usb_ctrl_req.b_request) {

    case HID_REQ_GET_REPORT: {
        /* Return current staged report for the requested interface          */
        ep_select(EP_CTRL);
        ep_clear_setup();
        if (iface == HID_IFACE_KBD) {
            ep_write_ctrl_stream(&s_kbd_buf, sizeof(s_kbd_buf));
        } else {
            ep_write_ctrl_stream(&s_con_buf, sizeof(s_con_buf));
        }
        ep_complete_ctrl_status();
        break;
    }

    case HID_REQ_SET_REPORT: {
        /* Interface 0 only: receive 1-byte LED report                      */
        ep_select(EP_CTRL);
        ep_clear_setup();
        if (iface == HID_IFACE_KBD) {
            uint8_t led = 0;
            ep_read_ctrl_stream(&led, 1);
            s_led_report = (hid_led_report_t)led;
        } else {
            /* No output report on interface 1 — accept and discard         */
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
}
