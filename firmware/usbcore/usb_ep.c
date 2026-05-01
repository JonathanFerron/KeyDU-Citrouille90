#include "usb_ep.h"
#include "usb_types.h"

/* --- Storage --- */

uint8_t       usb_ep_table[sizeof(ep_hw_table_t)] USB_ALIGNED(2);
ep_fifo_pair_t usb_ep_fifos[EP_TABLE_COUNT];

volatile uint8_t   usb_ep_selected;
volatile USB_EP_t *usb_ep_handle;
volatile ep_fifo_t *usb_ep_fifo;

/* --- Status register RMW helpers --- */

static inline void ep_status_clr(uint8_t bits)
{
    while (USB0.INTFLAGSB & USB_RMWBUSY_bm);
    if (usb_ep_selected & EP_DIR_IN)
        USB0.STATUS[usb_ep_selected & EP_NUM_MASK].INCLR  = bits;
    else
        USB0.STATUS[usb_ep_selected & EP_NUM_MASK].OUTCLR = bits;
}

static inline void ep_status_set(uint8_t bits)
{
    while (USB0.INTFLAGSB & USB_RMWBUSY_bm);
    if (usb_ep_selected & EP_DIR_IN)
        USB0.STATUS[usb_ep_selected & EP_NUM_MASK].INSET  = bits;
    else
        USB0.STATUS[usb_ep_selected & EP_NUM_MASK].OUTSET = bits;
}

static inline uint8_t ep_status_get(void)
{
    return usb_ep_handle->STATUS;
}

/* --- ep_select --- */

void ep_select(uint8_t address)
{
    uint8_t num = address & EP_NUM_MASK;
    usb_ep_selected = address;

    ep_fifo_pair_t *pair    = &usb_ep_fifos[num];
    ep_hw_table_t  *hw_tbl  = (ep_hw_table_t *)USB0.EPPTR;

    if (address & EP_DIR_IN) {
        usb_ep_fifo   = &pair->in;
        usb_ep_handle = &hw_tbl->endpoints[num].IN;
    } else {
        usb_ep_fifo   = &pair->out;
        usb_ep_handle = &hw_tbl->endpoints[num].OUT;
    }
}

/* --- ep_configure_prv: internal helper used by ep_configure --- */

static bool ep_configure_prv(uint8_t address, uint8_t config, uint8_t size)
{
    ep_select(address);

    usb_ep_handle->CTRL    = 0;
    ep_status_clr(0xFF);

    if (address & EP_DIR_IN)
        ep_status_set(USB_BUSNAK_bm);   /* IN ep starts ready for firmware to fill */

    usb_ep_handle->CTRL    = config;
    usb_ep_handle->CNT     = 0;
    usb_ep_handle->DATAPTR = (intptr_t)usb_ep_fifo->data;

    /* IN: length = bank size. OUT: length starts 0 (filled by hardware on receive) */
    usb_ep_fifo->length   = (address & EP_DIR_IN) ? size : 0;
    usb_ep_fifo->position = 0;

    return true;
}

/* Convert byte count to AVR DU BUFSIZE field value */
static uint8_t ep_size_to_mask(uint16_t size)
{
    if (size == 1023)
        return USB_BUFSIZE_ISO_BUF1023_gc;

    uint8_t  mask  = 0;
    uint16_t check = 8;
    while (check < size) {
        mask++;
        check <<= 1;
    }
    return (uint8_t)(mask << USB_BUFSIZE_DEFAULT_gp);
}

bool ep_configure(uint8_t address, uint8_t type, uint16_t size, uint8_t banks)
{
    (void)banks;    /* AVR DU does not use the banks parameter */

    if ((address & EP_NUM_MASK) >= EP_TABLE_COUNT) return false;
    if (size > EP_MAX_SIZE)                         return false;

    uint8_t cfg = (uint8_t)(USB_TCDSBL_bm | ep_size_to_mask(size));

    switch (type) {
        case EP_TYPE_CONTROL:     cfg |= USB_TYPE_CONTROL_gc;  break;
        case EP_TYPE_ISOCHRONOUS: cfg |= USB_TYPE_ISO_gc;      break;
        default:                  cfg |= USB_TYPE_BULKINT_gc;  break;
    }

    /* Control endpoints need both IN and OUT configured */
    if (type == EP_TYPE_CONTROL)
        ep_configure_prv((uint8_t)(address ^ EP_DIR_IN), cfg, (uint8_t)size);

    return ep_configure_prv(address, cfg, (uint8_t)size);
}

bool ep_configure_table(const ep_table_entry_t *table, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++) {
        if (!table[i].address) continue;
        if (!ep_configure(table[i].address, table[i].type,
                          table[i].size, table[i].banks))
            return false;
    }
    return true;
}

void ep_clear_all(void)
{
    ep_hw_table_t *tbl = (ep_hw_table_t *)USB0.EPPTR;
    for (uint8_t i = 0; i < EP_TABLE_COUNT; i++) {
        tbl->endpoints[i].IN.CTRL  = 0;
        tbl->endpoints[i].OUT.CTRL = 0;
    }
}

/* --- Packet state predicates --- */

bool ep_in_ready(void)
{
    ep_select((uint8_t)(usb_ep_selected | EP_DIR_IN));
    return (ep_status_get() & USB_BUSNAK_bm) != 0;
}

bool ep_out_received(void)
{
    ep_select((uint8_t)(usb_ep_selected & ~EP_DIR_IN));
    if (ep_status_get() & USB_TRNCOMPL_bm) {
        usb_ep_fifo->length = (uint8_t)usb_ep_handle->CNT;
        return true;
    }
    return false;
}

bool ep_setup_received(void)
{
    ep_select((uint8_t)(usb_ep_selected & ~EP_DIR_IN));
    if (ep_status_get() & USB_EPSETUP_bm) {
        usb_ep_fifo->length = (uint8_t)usb_ep_handle->CNT;
        return true;
    }
    return false;
}

/* --- Packet clear operations --- */

void ep_clear_setup(void)
{
    /* OUT side: clear setup flag, transaction complete, NAK, over/underflow;
       set toggle to sync data phase */
    ep_select((uint8_t)(usb_ep_selected & ~EP_DIR_IN));
    ep_status_clr(USB_EPSETUP_bm | USB_TRNCOMPL_bm | USB_BUSNAK_bm | USB_UNFOVF_bm);
    ep_status_set(USB_TOGGLE_bm);
    usb_ep_fifo->position = 0;

    /* IN side: clear setup flag, set toggle */
    ep_select((uint8_t)(usb_ep_selected | EP_DIR_IN));
    ep_status_clr(USB_EPSETUP_bm);
    ep_status_set(USB_TOGGLE_bm);
    usb_ep_fifo->position = 0;
}

void ep_clear_in(void)
{
    usb_ep_handle->CNT = usb_ep_fifo->position;   /* tell HW how many bytes to send */
    ep_status_clr(USB_TRNCOMPL_bm | USB_BUSNAK_bm | USB_UNFOVF_bm);
    usb_ep_fifo->position = 0;
}

void ep_clear_out(void)
{
    ep_status_clr(USB_TRNCOMPL_bm | USB_BUSNAK_bm | USB_UNFOVF_bm);
    usb_ep_fifo->position = 0;
}

void ep_stall(void)
{
    usb_ep_handle->CTRL |= USB_DOSTALL_bm;
    /* For CONTROL type, stall both directions */
    if ((usb_ep_handle->CTRL & USB_TYPE_gm) == USB_TYPE_CONTROL_gc) {
        ep_select((uint8_t)(usb_ep_selected ^ EP_DIR_IN));
        usb_ep_handle->CTRL |= USB_DOSTALL_bm;
    }
}

/* --- Primitive byte I/O --- */

uint8_t ep_read_u8(void)
{
    return usb_ep_fifo->data[usb_ep_fifo->position++];
}

void ep_write_u8(uint8_t data)
{
    usb_ep_fifo->data[usb_ep_fifo->position++] = data;
}

/* --- Status stage completion for CONTROL transfers --- */

void ep_complete_ctrl_status(void)
{
    if (usb_ctrl_req.bm_request_type & REQ_DIR_DEV_TO_HOST) {
        /* Data was IN; status stage is a zero-length OUT from host */
        while (!ep_out_received()) {
            if (usb_device_state == USB_STATE_UNATTACHED) return;
        }
        ep_clear_out();
    } else {
        /* Data was OUT (or no data); status stage is a zero-length IN */
        while (!ep_in_ready()) {
            if (usb_device_state == USB_STATE_UNATTACHED) return;
        }
        ep_clear_in();
    }
}

/* --- ep_wait_ready (non-control endpoints) --- */

ep_ready_result_t ep_wait_ready(void)
{
    uint16_t timeout = USB_STREAM_TIMEOUT_MS;
    uint16_t prev_frame = ep_get_frame_number();

    for (;;) {
        if (ep_get_direction() == EP_DIR_IN) {
            if (ep_in_ready())    return EP_READY_OK;
        } else {
            if (ep_out_received()) return EP_READY_OK;
        }

        usb_dev_state_t state = usb_device_state;
        if (state == USB_STATE_UNATTACHED) return EP_READY_DISCONNECTED;
        if (state == USB_STATE_SUSPENDED)  return EP_READY_SUSPENDED;
        if (ep_is_stalled())               return EP_READY_STALLED;

        uint16_t cur_frame = ep_get_frame_number();
        if (cur_frame != prev_frame) {
            prev_frame = cur_frame;
            if (!timeout--) return EP_READY_TIMEOUT;
        }
    }
}
