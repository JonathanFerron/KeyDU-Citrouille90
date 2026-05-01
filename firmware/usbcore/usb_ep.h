#ifndef USB_EP_H
#define USB_EP_H

#include "usb_types.h"

/*
 * usb_ep — AVR64DU32 USB endpoint hardware layer.
 *
 * Manages the hardware endpoint table, per-endpoint FIFO pairs (software
 * buffers), and all primitive read/write/clear/stall operations.
 *
 * All operations act on the currently selected endpoint set by ep_select().
 * Callers that switch endpoints must save and restore the selection.
 */

/* --- Internal types (used by usb_ep_stream.c and usb_ctrl.c) --- */

/* Software FIFO for one direction of one endpoint */
typedef struct {
    uint8_t data[EP_MAX_SIZE];
    uint8_t length;    /* bytes available (OUT) or bank size (IN) */
    uint8_t position;  /* current read/write cursor */
} ep_fifo_t;

/* Paired FIFOs for both directions of one endpoint number */
typedef struct {
    ep_fifo_t out;
    ep_fifo_t in;
} ep_fifo_pair_t;

/* Hardware endpoint table layout for AVR DU (pointed to by USB0.EPPTR) */
typedef struct USB_PACKED {
    USB_EP_PAIR_t  endpoints[EP_TABLE_COUNT];
    uint16_t       frame_num;
} ep_hw_table_t;

/* Aligned storage for the hardware endpoint table */
extern uint8_t  usb_ep_table[sizeof(ep_hw_table_t)] USB_ALIGNED(2);

/* Software FIFO pairs (one per endpoint slot) */
extern ep_fifo_pair_t  usb_ep_fifos[EP_TABLE_COUNT];

/* Currently selected endpoint address (includes direction bit) */
extern volatile uint8_t   usb_ep_selected;
extern volatile USB_EP_t *usb_ep_handle;   /* pointer into hw table */
extern volatile ep_fifo_t *usb_ep_fifo;    /* pointer into sw fifo  */

/* --- Endpoint selection --- */

/* Select an endpoint by address (e.g. EP_DIR_IN | 1). All subsequent
   operations act on this endpoint until ep_select() is called again. */
void ep_select(uint8_t address);

/* --- Endpoint configuration --- */

/* Configure one endpoint. Returns true on success.
   Type is EP_TYPE_*. size must be <= EP_MAX_SIZE.
   Control endpoints auto-configure both IN and OUT directions. */
bool ep_configure(uint8_t address, uint8_t type, uint16_t size, uint8_t banks);

/* Configure a table of endpoints in one call. Zero-address entries are
   skipped. Returns false if any configuration fails. */
typedef struct {
    uint8_t  address;
    uint16_t size;
    uint8_t  type;
    uint8_t  banks;
} ep_table_entry_t;
bool ep_configure_table(const ep_table_entry_t *table, uint8_t count);

/* Reset all endpoint hardware entries to unconfigured state */
void ep_clear_all(void);

/* --- Packet management --- */

/* True if selected IN endpoint is ready for a new packet (BUSNAK set) */
bool ep_in_ready(void);

/* True if selected OUT endpoint has a received packet (TRNCOMPL set).
   Also loads fifo->length from hardware CNT register. */
bool ep_out_received(void);

/* True if selected CONTROL endpoint has a received SETUP packet */
bool ep_setup_received(void);

/* Clear the SETUP condition, reset data toggles, zero FIFO cursors */
void ep_clear_setup(void);

/* Commit the IN FIFO to hardware and clear for next packet */
void ep_clear_in(void);

/* Acknowledge the received OUT packet and reset FIFO cursor */
void ep_clear_out(void);

/* Stall the current endpoint (and paired direction if CONTROL type) */
void ep_stall(void);

/* Clear the STALL condition on the current endpoint */
static inline void ep_clear_stall(void)
{
    usb_ep_handle->CTRL &= ~USB_DOSTALL_bm;
}

/* True if the current endpoint is stalled */
static inline bool ep_is_stalled(void)
{
    return (usb_ep_handle->CTRL & USB_DOSTALL_bm) != 0;
}

/* Reset the data toggle bit on the current endpoint */
static inline void ep_reset_toggle(void)
{
    /* Requires RMW-safe write via STATUS CLR register */
    while (USB0.INTFLAGSB & USB_RMWBUSY_bm);
    if (usb_ep_selected & EP_DIR_IN)
        USB0.STATUS[usb_ep_selected & EP_NUM_MASK].INCLR  = USB_TOGGLE_bm;
    else
        USB0.STATUS[usb_ep_selected & EP_NUM_MASK].OUTCLR = USB_TOGGLE_bm;
}

/* Reset the FIFO cursor for a specific endpoint address (not just selected) */
static inline void ep_reset_fifo(uint8_t address)
{
    if (address & EP_DIR_IN)
        usb_ep_fifos[address & EP_NUM_MASK].in.position  = 0;
    else
        usb_ep_fifos[address & EP_NUM_MASK].out.position = 0;
}

/* Return the current selected endpoint address */
static inline uint8_t ep_get_current(void)
{
    return usb_ep_selected;
}

/* Direction of selected endpoint: EP_DIR_IN or EP_DIR_OUT */
static inline uint8_t ep_get_direction(void)
{
    return usb_ep_selected & EP_DIR_MASK;
}

/* True if the selected endpoint is configured (has a non-zero TYPE field) */
static inline bool ep_is_configured(void)
{
    return (usb_ep_handle->CTRL & USB_TYPE_gm) != 0;
}

/* Bytes remaining to read (OUT) or space used so far (IN) in the FIFO */
static inline uint16_t ep_bytes_in_fifo(void)
{
    if (usb_ep_selected & EP_DIR_IN)
        return usb_ep_fifo->position;
    else
        return (uint16_t)(usb_ep_fifo->length - usb_ep_fifo->position);
}

/* True if FIFO position is within the buffer (more data to read/write) */
static inline bool ep_rw_allowed(void)
{
    return usb_ep_fifo->position < usb_ep_fifo->length;
}

/* --- Primitive byte read/write --- */

uint8_t ep_read_u8(void);
void    ep_write_u8(uint8_t data);

static inline void ep_discard_u8(void)  { usb_ep_fifo->position++; }

static inline uint16_t ep_read_u16_le(void)
{
    uint16_t lo = ep_read_u8();
    uint16_t hi = ep_read_u8();
    return (hi << 8) | lo;
}

static inline void ep_write_u16_le(uint16_t data)
{
    ep_write_u8((uint8_t)(data));
    ep_write_u8((uint8_t)(data >> 8));
}

static inline void ep_discard_u16(void) { ep_discard_u8(); ep_discard_u8(); }

/* --- Wait for endpoint ready (non-control endpoints) --- */

typedef enum {
    EP_READY_OK           = 0,
    EP_READY_STALLED      = 1,
    EP_READY_DISCONNECTED = 2,
    EP_READY_SUSPENDED    = 3,
    EP_READY_TIMEOUT      = 4,
} ep_ready_result_t;

ep_ready_result_t ep_wait_ready(void);

/* Complete the status stage of a CONTROL transfer (send/receive ZLP) */
void ep_complete_ctrl_status(void);

/* Current USB frame number (from hardware endpoint table FrameNum field) */
static inline uint16_t ep_get_frame_number(void)
{
    return ((ep_hw_table_t *)USB0.EPPTR)->frame_num;
}

/* Remote wakeup — assert URESUME bit */
static inline void usb_send_remote_wakeup(void)
{
    USB0.CTRLB |= USB_URESUME_bm;
}

#endif /* USB_EP_H */
