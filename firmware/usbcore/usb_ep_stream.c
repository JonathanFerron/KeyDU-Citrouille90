#include "usb_ep_stream.h"
#include <avr/pgmspace.h>

/* --- Non-control endpoint stream helper --- */
/*
 * All four non-control streams share the same loop structure. Rather than
 * LUFA's template-macro abuse, we write them directly. Each is short enough
 * to be readable and distinct.
 */

ep_ready_result_t ep_discard_stream(uint16_t length, uint16_t *bytes_done)
{
    uint16_t done = bytes_done ? *bytes_done : 0;
    ep_ready_result_t err = ep_wait_ready();
    if (err) return err;

    while (done < length) {
        if (!ep_rw_allowed()) {
            ep_clear_out();
            if (bytes_done) { *bytes_done = done; return EP_READY_OK; }
            err = ep_wait_ready();
            if (err) return err;
        } else {
            ep_discard_u8();
            done++;
        }
    }
    return EP_READY_OK;
}

ep_ready_result_t ep_write_null_stream(uint16_t length, uint16_t *bytes_done)
{
    uint16_t done = bytes_done ? *bytes_done : 0;
    ep_ready_result_t err = ep_wait_ready();
    if (err) return err;

    while (done < length) {
        if (!ep_rw_allowed()) {
            ep_clear_in();
            if (bytes_done) { *bytes_done = done; return EP_READY_OK; }
            err = ep_wait_ready();
            if (err) return err;
        } else {
            ep_write_u8(0);
            done++;
        }
    }
    return EP_READY_OK;
}

ep_ready_result_t ep_write_stream(const void *buf, uint16_t length,
                                  uint16_t *bytes_done)
{
    uint16_t       done = bytes_done ? *bytes_done : 0;
    const uint8_t *p    = (const uint8_t *)buf + done;
    ep_ready_result_t err = ep_wait_ready();
    if (err) return err;

    while (done < length) {
        if (!ep_rw_allowed()) {
            ep_clear_in();
            if (bytes_done) { *bytes_done = done; return EP_READY_OK; }
            err = ep_wait_ready();
            if (err) return err;
        } else {
            ep_write_u8(*p++);
            done++;
        }
    }
    return EP_READY_OK;
}

ep_ready_result_t ep_read_stream(void *buf, uint16_t length,
                                 uint16_t *bytes_done)
{
    uint16_t  done = bytes_done ? *bytes_done : 0;
    uint8_t  *p    = (uint8_t *)buf + done;
    ep_ready_result_t err = ep_wait_ready();
    if (err) return err;

    while (done < length) {
        if (!ep_rw_allowed()) {
            ep_clear_out();
            if (bytes_done) { *bytes_done = done; return EP_READY_OK; }
            err = ep_wait_ready();
            if (err) return err;
        } else {
            *p++ = ep_read_u8();
            done++;
        }
    }
    return EP_READY_OK;
}

ep_ready_result_t ep_write_stream_P(const void *buf, uint16_t length,
                                    uint16_t *bytes_done)
{
    uint16_t       done = bytes_done ? *bytes_done : 0;
    const uint8_t *p    = (const uint8_t *)buf + done;
    ep_ready_result_t err = ep_wait_ready();
    if (err) return err;

    while (done < length) {
        if (!ep_rw_allowed()) {
            ep_clear_in();
            if (bytes_done) { *bytes_done = done; return EP_READY_OK; }
            err = ep_wait_ready();
            if (err) return err;
        } else {
            ep_write_u8(pgm_read_byte(p++));
            done++;
        }
    }
    return EP_READY_OK;
}

/* --- Control endpoint stream helpers --- */
/*
 * Control streams differ from bulk streams in two ways:
 * (1) They handle host-abort (SETUP received mid-transfer) as a normal exit.
 * (2) They auto-send the last partial packet rather than waiting for a full bank.
 * The caller is always responsible for clearing the status-stage packet after
 * these functions return.
 */

ep_ready_result_t ep_write_ctrl_stream(const void *buf, uint16_t length)
{
    const uint8_t *p   = (const uint8_t *)buf;
    uint16_t       rem = length;

    /* Clamp to wLength if host asked for less */
    if (rem > usb_ctrl_req.w_length)
        rem = usb_ctrl_req.w_length;

    while (rem) {
        /* Wait for IN endpoint to be ready */
        while (!ep_in_ready()) {
            if (usb_device_state == USB_STATE_UNATTACHED) return EP_READY_DISCONNECTED;
            if (ep_setup_received())                      return EP_READY_OK; /* host aborted */
        }

        /* Fill up to the bank size */
        while (rem && ep_rw_allowed()) {
            ep_write_u8(*p++);
            rem--;
        }
        ep_clear_in();
    }

    /* If transfer was an exact multiple of bank size, send a ZLP */
    if (length && (length % EP_MAX_SIZE == 0)) {
        while (!ep_in_ready()) {
            if (usb_device_state == USB_STATE_UNATTACHED) return EP_READY_DISCONNECTED;
        }
        ep_clear_in();
    }

    return EP_READY_OK;
}

ep_ready_result_t ep_write_ctrl_stream_P(const void *buf, uint16_t length)
{
    const uint8_t *p   = (const uint8_t *)buf;
    uint16_t       rem = length;

    if (rem > usb_ctrl_req.w_length)
        rem = usb_ctrl_req.w_length;

    while (rem) {
        while (!ep_in_ready()) {
            if (usb_device_state == USB_STATE_UNATTACHED) return EP_READY_DISCONNECTED;
            if (ep_setup_received())                      return EP_READY_OK;
        }

        while (rem && ep_rw_allowed()) {
            ep_write_u8(pgm_read_byte(p++));
            rem--;
        }
        ep_clear_in();
    }

    if (length && (length % EP_MAX_SIZE == 0)) {
        while (!ep_in_ready()) {
            if (usb_device_state == USB_STATE_UNATTACHED) return EP_READY_DISCONNECTED;
        }
        ep_clear_in();
    }

    return EP_READY_OK;
}

ep_ready_result_t ep_read_ctrl_stream(void *buf, uint16_t length)
{
    uint8_t  *p   = (uint8_t *)buf;
    uint16_t  rem = length;

    while (rem) {
        while (!ep_out_received()) {
            if (usb_device_state == USB_STATE_UNATTACHED) return EP_READY_DISCONNECTED;
            if (ep_setup_received())                      return EP_READY_OK;
        }

        while (rem && ep_rw_allowed()) {
            *p++ = ep_read_u8();
            rem--;
        }
        ep_clear_out();
    }

    return EP_READY_OK;
}
