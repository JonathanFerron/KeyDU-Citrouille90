#ifndef USB_CTRL_H
#define USB_CTRL_H

#include "usb_types.h"
#include "usb_ep.h"
#include "usb_ep_stream.h"

/*
 * usb_ctrl — standard USB control request handling and device init/task.
 *
 * usb_init() brings up the USB hardware. usb_ctrl_poll() must be called every
 * iteration of the main loop (ungated) to poll EP0 for pending SETUP packets.
 *
 * Three callbacks MUST be implemented by the caller (usb_desc.c for the App,
 * usb_vendor.c for the BL):
 *
 *   uint16_t usb_get_desc(uint16_t w_value, uint16_t w_index,
 *                         const void **addr);
 *     Return size of the requested descriptor and set *addr to its location in
 *     flash (PROGMEM). Return NO_DESCRIPTOR if not found.
 *
 * Four event callbacks have weak stub definitions here and may be overridden
 * by usb_hid.c (or usb_vendor.c for the bootloader):
 *
 *   void usb_event_reset(void)
 *   void usb_event_config_changed(void)
 *   void usb_event_suspend(void)
 *   void usb_event_wakeup(void)
 *
 * Class-specific control requests are forwarded to:
 *   void usb_event_ctrl_request(void)   — weak stub, override in caller
 * If not handled there, the request is automatically stalled.
 */

/* --- Options flags for usb_init --- */

/* Route USB bus-event interrupt to CPU high-priority vector (CPUINT.LVL1VEC) */
#define USB_OPT_BUS_INT_HIGH  (1u << 0)

/* Enable internal 3.3V VUSB regulator (requires VCC >= ~4V) */
#define USB_OPT_VREG_ENABLE   (1u << 2)

/* --- Init and task --- */

/* Initialize the USB peripheral and begin device-mode enumeration.
   options is a bitmask of USB_OPT_* flags.
   Call sei() before or immediately after this. */
void usb_init(uint8_t options);

/* Disable USB peripheral and detach from bus */
void usb_disable(void);

/* Reset USB interface and re-enumerate. Called internally on bus reset. */
void usb_reset_interface(void);

/* Poll EP0 for a pending SETUP packet and dispatch it.
   Must be called every main-loop iteration, ungated. */
void usb_ctrl_poll(void);

/* --- Descriptor callback — implemented in usb_desc.c (App) or usb_vendor_desc.c (BL) --- */

/* Return descriptor size; set *addr to PROGMEM address. Return NO_DESCRIPTOR if absent. */
uint16_t usb_get_desc(uint16_t w_value, uint16_t w_index, const void **addr);

/* --- Event callbacks — weak stubs, override in usb_hid.c / usb_vendor.c --- */

/* Called after bus reset: EP0 already reconfigured, state set to DEFAULT */
void usb_event_reset(void)          USB_WEAK;

/* Called after SET_CONFIGURATION completes; usb_config_num is already updated */
void usb_event_config_changed(void) USB_WEAK;

/* Called on USB bus suspend */
void usb_event_suspend(void)        USB_WEAK;

/* Called on USB bus resume / wakeup */
void usb_event_wakeup(void)         USB_WEAK;

/* Called for every SETUP packet before standard handling.
   If the request is handled here, it must not be left pending (call ep_clear_setup
   and complete the transfer). Unhandled requests fall through to standard dispatch. */
void usb_event_ctrl_request(void)   USB_WEAK;

/* Called each SOF (1 ms when connected). */
void usb_event_sof(void)            USB_WEAK;

/* --- SOF interrupt control --- */

static inline void usb_sof_enable(void)  { USB0.INTCTRLA |=  USB_SOF_bm; }
static inline void usb_sof_disable(void) { USB0.INTCTRLA &= ~USB_SOF_bm; }

#endif /* USB_CTRL_H */
