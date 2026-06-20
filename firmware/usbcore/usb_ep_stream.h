#ifndef USB_EP_STREAM_H
#define USB_EP_STREAM_H

#include "usb_ep.h"

/*
   usb_ep_stream — endpoint stream I/O.

   ep_write_stream: write a RAM buffer to a non-control IN endpoint,
   handling packet boundaries. Returns ep_ready_result_t; EP_READY_OK on
   success. Pass bytes_done non-NULL for incremental (resumable) transfers.

   Control stream functions do not take a bytes_done parameter — the entire
   transfer must complete in one call. The caller is responsible for clearing
   the status-stage packet after these return.
*/

/* Write length bytes from RAM buffer to the current non-control IN endpoint */
ep_ready_result_t ep_write_stream(const void* buf, uint16_t length,
                                  uint16_t* bytes_done);

/* --- Control endpoint streams --- */

/* Send length bytes from RAM buffer through EP0 IN (descriptor responses etc.) */
ep_ready_result_t ep_write_ctrl_stream(const void* buf, uint16_t length);

/* Send length bytes from PROGMEM through EP0 IN */
ep_ready_result_t ep_write_ctrl_stream_P(const void* buf, uint16_t length);

/* Receive length bytes from EP0 OUT into RAM buffer */
ep_ready_result_t ep_read_ctrl_stream(void* buf, uint16_t length);

#endif /* USB_EP_STREAM_H */
