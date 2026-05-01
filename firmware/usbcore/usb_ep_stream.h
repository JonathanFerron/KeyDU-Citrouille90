#ifndef USB_EP_STREAM_H
#define USB_EP_STREAM_H

#include "usb_ep.h"

/*
 * usb_ep_stream — bulk/interrupt endpoint stream I/O.
 *
 * These functions transfer multi-byte buffers through non-control endpoints,
 * handling packet boundaries automatically. For control endpoints, use the
 * ep_write_ctrl_stream / ep_read_ctrl_stream variants.
 *
 * All stream functions return an ep_ready_result_t. EP_READY_OK means the
 * full transfer completed. If bytes_done is non-NULL, an incomplete transfer
 * returns EP_READY_OK incrementally; call again with the same parameters to
 * continue until bytes_done reaches length.
 *
 * Control stream functions do not take a bytes_done parameter — the entire
 * transfer must complete in one call. The caller is responsible for clearing
 * the status-stage packet after these return.
 */

/* --- Non-control endpoint streams --- */

/* Discard length bytes from the current OUT endpoint */
ep_ready_result_t ep_discard_stream(uint16_t length, uint16_t *bytes_done);

/* Write length zero-bytes to the current IN endpoint */
ep_ready_result_t ep_write_null_stream(uint16_t length, uint16_t *bytes_done);

/* Write length bytes from RAM buffer to the current IN endpoint (little-endian) */
ep_ready_result_t ep_write_stream(const void *buf, uint16_t length,
                                  uint16_t *bytes_done);

/* Read length bytes from the current OUT endpoint into RAM buffer */
ep_ready_result_t ep_read_stream(void *buf, uint16_t length,
                                 uint16_t *bytes_done);

/* Write length bytes from PROGMEM (flash) buffer to the current IN endpoint */
ep_ready_result_t ep_write_stream_P(const void *buf, uint16_t length,
                                    uint16_t *bytes_done);

/* --- Control endpoint streams --- */

/* Send length bytes from RAM buffer through EP0 IN (descriptor responses etc.) */
ep_ready_result_t ep_write_ctrl_stream(const void *buf, uint16_t length);

/* Send length bytes from PROGMEM through EP0 IN */
ep_ready_result_t ep_write_ctrl_stream_P(const void *buf, uint16_t length);

/* Receive length bytes from EP0 OUT into RAM buffer */
ep_ready_result_t ep_read_ctrl_stream(void *buf, uint16_t length);

#endif /* USB_EP_STREAM_H */
