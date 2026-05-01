#include "usb_ctrl.h"
#include <avr/interrupt.h>

/* --- Globals (declared extern in usb_types.h) --- */

volatile usb_dev_state_t  usb_device_state    = USB_STATE_UNATTACHED;
usb_request_t             usb_ctrl_req;
uint8_t                   usb_config_num      = 0;
bool                      usb_remote_wakeup_enabled = false;
uint8_t                   usb_ctrl_ep_size    = EP_CTRL_SIZE_DEFAULT;
volatile bool             usb_is_initialized  = false;

/* --- Weak event stubs --- */

void usb_event_reset(void)          {}
void usb_event_config_changed(void) {}
void usb_event_suspend(void)        {}
void usb_event_wakeup(void)         {}
void usb_event_ctrl_request(void)   {}
void usb_event_sof(void)            {}

/* --- Internal helpers --- */

/* Build serial string descriptor from SIGROW into a local buffer and send it */
static void send_internal_serial(void)
{
    struct {
        uint8_t  b_length;
        uint8_t  b_descriptor_type;
        uint16_t unicode[USB_SERIAL_NUM_CHARS];
    } desc;

    desc.b_length          = USB_STRING_LEN(USB_SERIAL_NUM_CHARS);
    desc.b_descriptor_type = DTYPE_STRING;

    /* Disable interrupts briefly to ensure consistent SIGROW read */
    uint8_t sreg = SREG;
    cli();
    const uint8_t *src = (const uint8_t *)USB_SERIAL_ADDR;
    for (uint8_t i = 0; i < USB_SERIAL_NUM_CHARS; i++) {
        uint8_t byte = *src;
        uint8_t nibble = (i & 1u) ? (byte >> 4) : (byte & 0x0Fu);
        if (i & 1u) src++;
        desc.unicode[i] = cpu_to_le16(nibble >= 10
                          ? ('A' - 10 + nibble)
                          : ('0' + nibble));
    }
    SREG = sreg;

    ep_clear_setup();
    ep_write_ctrl_stream(&desc, sizeof(desc));
    ep_clear_out();
}

/* --- Standard request handlers --- */

static void ctrl_get_descriptor(void)
{
    /* Internal serial number shortcut */
    if (usb_ctrl_req.w_value == ((uint16_t)(DTYPE_STRING << 8) | 0xDCu)) {
        send_internal_serial();
        return;
    }

    const void *addr;
    uint16_t size = usb_get_desc(usb_ctrl_req.w_value,
                                 usb_ctrl_req.w_index, &addr);
    if (size == NO_DESCRIPTOR) return;

    ep_clear_setup();
    ep_write_ctrl_stream_P(addr, size);
    ep_clear_out();
}

static void ctrl_set_address(void)
{
    uint8_t addr = (uint8_t)(usb_ctrl_req.w_value & 0x7Fu);

    /* AVR DU: set address register but do not enable it yet —
       address takes effect after the status IN packet completes */
    (void)addr;  /* USB0.ADDR written in ep_complete_ctrl_status callback below */
    ep_clear_setup();
    ep_complete_ctrl_status();

    /* Now the status IN has been sent; activate the new address */
    while (!ep_in_ready());
    USB0.ADDR = addr;
    usb_device_state = addr ? USB_STATE_ADDRESSED : USB_STATE_DEFAULT;
}

static void ctrl_get_configuration(void)
{
    ep_clear_setup();
    ep_write_u8(usb_config_num);
    ep_clear_in();
    ep_complete_ctrl_status();
}

static void ctrl_set_configuration(void)
{
    usb_config_num = (uint8_t)usb_ctrl_req.w_value;
    ep_clear_setup();
    ep_complete_ctrl_status();
    usb_device_state = usb_config_num ? USB_STATE_CONFIGURED
                     : (USB0.ADDR ? USB_STATE_ADDRESSED : USB_STATE_POWERED);
    usb_event_config_changed();
}

static void ctrl_get_status(void)
{
    uint16_t status = 0;

    if (usb_ctrl_req.bm_request_type ==
            (REQ_DIR_DEV_TO_HOST | REQ_TYPE_STANDARD | REQ_REC_DEVICE)) {
        if (usb_remote_wakeup_enabled)
            status |= STATUS_REMOTE_WAKEUP;
    } else if (usb_ctrl_req.bm_request_type ==
            (REQ_DIR_DEV_TO_HOST | REQ_TYPE_STANDARD | REQ_REC_ENDPOINT)) {
        uint8_t ep_num = (uint8_t)(usb_ctrl_req.w_index & EP_NUM_MASK);
        if (ep_num < EP_TABLE_COUNT) {
            uint8_t saved = ep_get_current();
            ep_select(ep_num);
            status = ep_is_stalled() ? 1u : 0u;
            ep_select(saved);
        }
    } else {
        return;  /* unsupported recipient — will be stalled below */
    }

    ep_clear_setup();
    ep_write_u16_le(status);
    ep_clear_in();
    ep_complete_ctrl_status();
}

static void ctrl_clear_set_feature(void)
{
    bool set = (usb_ctrl_req.b_request == REQ_SET_FEATURE);
    uint8_t recipient = usb_ctrl_req.bm_request_type & REQ_REC_MASK;

    if (recipient == REQ_REC_DEVICE) {
        if (usb_ctrl_req.w_value == FEAT_REMOTE_WAKEUP) {
            usb_remote_wakeup_enabled = set;
        } else {
            return;
        }
    } else if (recipient == REQ_REC_ENDPOINT) {
        uint8_t ep_num = (uint8_t)(usb_ctrl_req.w_index & EP_NUM_MASK);
        if (ep_num == EP_CTRL || ep_num >= EP_TABLE_COUNT) return;

        uint8_t saved = ep_get_current();
        ep_select(ep_num);
        if (set) {
            ep_stall();
        } else {
            ep_clear_stall();
            ep_reset_fifo(ep_num);
            ep_reset_toggle();
        }
        ep_select(saved);
    } else {
        return;
    }

    ep_select(EP_CTRL);
    ep_clear_setup();
    ep_complete_ctrl_status();
}

/* --- Main control request dispatcher --- */

void usb_process_ctrl_request(void)
{
    /* Read the 8-byte SETUP packet from the FIFO into usb_ctrl_req */
    uint8_t *req = (uint8_t *)&usb_ctrl_req;
    for (uint8_t i = 0; i < sizeof(usb_request_t); i++)
        req[i] = ep_read_u8();

    /* Give the application first look (HID GET/SET_REPORT, vendor requests…) */
    usb_event_ctrl_request();

    /* If the application handled it, SETUP is no longer pending — done */
    if (!ep_setup_received()) return;

    uint8_t type = usb_ctrl_req.bm_request_type;

    switch (usb_ctrl_req.b_request) {
        case REQ_GET_STATUS:
            if ((type == (REQ_DIR_DEV_TO_HOST | REQ_TYPE_STANDARD | REQ_REC_DEVICE)) ||
                (type == (REQ_DIR_DEV_TO_HOST | REQ_TYPE_STANDARD | REQ_REC_ENDPOINT)))
                ctrl_get_status();
            break;
        case REQ_CLEAR_FEATURE:
        case REQ_SET_FEATURE:
            if ((type == (REQ_DIR_HOST_TO_DEV | REQ_TYPE_STANDARD | REQ_REC_DEVICE)) ||
                (type == (REQ_DIR_HOST_TO_DEV | REQ_TYPE_STANDARD | REQ_REC_ENDPOINT)))
                ctrl_clear_set_feature();
            break;
        case REQ_SET_ADDRESS:
            if (type == (REQ_DIR_HOST_TO_DEV | REQ_TYPE_STANDARD | REQ_REC_DEVICE))
                ctrl_set_address();
            break;
        case REQ_GET_DESCRIPTOR:
            if ((type == (REQ_DIR_DEV_TO_HOST | REQ_TYPE_STANDARD | REQ_REC_DEVICE)) ||
                (type == (REQ_DIR_DEV_TO_HOST | REQ_TYPE_STANDARD | REQ_REC_INTERFACE)))
                ctrl_get_descriptor();
            break;
        case REQ_GET_CONFIGURATION:
            if (type == (REQ_DIR_DEV_TO_HOST | REQ_TYPE_STANDARD | REQ_REC_DEVICE))
                ctrl_get_configuration();
            break;
        case REQ_SET_CONFIGURATION:
            if (type == (REQ_DIR_HOST_TO_DEV | REQ_TYPE_STANDARD | REQ_REC_DEVICE))
                ctrl_set_configuration();
            break;
        default:
            break;
    }

    /* Stall any unhandled request that left SETUP still pending */
    if (ep_setup_received()) {
        ep_clear_setup();
        ep_stall();
    }
}

/* --- usb_task (polled EP0) --- */

void usb_task(void)
{
    if (usb_device_state == USB_STATE_UNATTACHED) return;

    uint8_t saved = ep_get_current();
    ep_select(EP_CTRL);

    if (ep_setup_received())
        usb_process_ctrl_request();

    ep_select(saved);
}

/* --- usb_init_device: reset state machine and configure EP0 --- */

static void usb_init_device(void)
{
    usb_device_state           = USB_STATE_UNATTACHED;
    usb_config_num             = 0;
    usb_remote_wakeup_enabled  = false;

    /* Read control EP size from device descriptor (PROGMEM) */
    const usb_desc_device_t *dev_desc;
    uint16_t size = usb_get_desc((uint16_t)(DTYPE_DEVICE << 8), 0,
                                 (const void **)&dev_desc);
    if (size != NO_DESCRIPTOR)
        usb_ctrl_ep_size = pgm_read_byte(&dev_desc->b_max_packet_size0);

    ep_configure(EP_CTRL, EP_TYPE_CONTROL, usb_ctrl_ep_size, 1);

    /* Enable bus-event interrupts: suspend, resume, reset */
    USB0.INTCTRLA |= USB_SUSPEND_bm | USB_RESUME_bm | USB_RESET_bm;

    USB0.CTRLB |= USB_ATTACH_bm;
}

/* --- usb_init --- */

void usb_init(uint8_t options)
{
    uint8_t sreg = SREG;
    cli();

    USB0.EPPTR = (intptr_t)&usb_ep_table[0];
    USB0.CTRLA = (uint8_t)(USB_STFRNUM_bm |
                           ((EP_TABLE_COUNT - 1u) << USB_MAXEP_gp));

    if (options & USB_OPT_BUS_INT_HIGH)
        CPUINT.LVL1VEC = USB0_BUSEVENT_vect_num;

    if (options & USB_OPT_VREG_ENABLE)
        SYSCFG.VUSBCTRL = SYSCFG_USBVREG_ENABLE_gc;
    else
        SYSCFG.VUSBCTRL = SYSCFG_USBVREG_DISABLE_gc;

    SREG = sreg;

    usb_is_initialized = true;
    usb_reset_interface();
}

void usb_disable(void)
{
    USB0.INTCTRLA = 0;
    USB0.INTCTRLB = 0;
    USB0.INTFLAGSA = 0xFF;
    USB0.INTFLAGSB = 0xFF;
    USB0.CTRLB &= ~USB_ATTACH_bm;
    USB0.CTRLA &= ~USB_ENABLE_bm;
    usb_is_initialized = false;
}

void usb_reset_interface(void)
{
    USB0.ADDR = 0;
    USB0.INTCTRLA = 0;
    USB0.INTCTRLB = 0;
    USB0.INTFLAGSA = 0xFF;
    USB0.INTFLAGSB = 0xFF;
    /* Reset: disable then re-enable the controller */
    USB0.CTRLA &= ~USB_ENABLE_bm;
    USB0.CTRLA |=  USB_ENABLE_bm;
    usb_init_device();
}

/* --- USB0_BUSEVENT_vect: SOF, suspend, resume, reset --- */

ISR(USB0_BUSEVENT_vect)
{
    /* SOF — fires every 1 ms when connected */
    if (USB0.INTFLAGSA & USB_SOF_bm) {
        USB0.INTFLAGSA = USB_SOF_bm;
        usb_event_sof();
    }

    /* Suspend */
    if (USB0.INTFLAGSA & USB_SUSPEND_bm) {
        USB0.INTFLAGSA = USB_SUSPEND_bm;
        usb_device_state = USB_STATE_SUSPENDED;
        usb_event_suspend();
    }

    /* Resume */
    if (USB0.INTFLAGSA & USB_RESUME_bm) {
        USB0.INTFLAGSA = USB_RESUME_bm;
        if (usb_config_num)
            usb_device_state = USB_STATE_CONFIGURED;
        else
            usb_device_state = USB0.ADDR ? USB_STATE_ADDRESSED : USB_STATE_POWERED;
        usb_event_wakeup();
    }

    /* Bus reset */
    if (USB0.INTFLAGSA & USB_RESET_bm) {
        USB0.INTFLAGSA = USB_RESET_bm;

        usb_device_state = USB_STATE_DEFAULT;
        usb_config_num   = 0;
        USB0.ADDR        = 0;

        ep_clear_all();
        ep_configure(EP_CTRL, EP_TYPE_CONTROL, usb_ctrl_ep_size, 1);

        usb_event_reset();
    }
}
