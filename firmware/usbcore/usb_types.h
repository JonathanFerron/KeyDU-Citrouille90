#ifndef USB_TYPES_H
#define USB_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

/* --- Compiler helpers --- */

#define USB_PACKED      __attribute__((packed))
#define USB_ALIGNED(n)  __attribute__((aligned(n)))
#define USB_WEAK        __attribute__((weak))
#define USB_NOINLINE    __attribute__((noinline))
#define USB_ALWAYS_INLINE __attribute__((always_inline))

/* cpu_to_le16: AVR64DU32 is little-endian, this is a no-op */
#define cpu_to_le16(x)  (x)

/* GCC memory barrier — prevents compiler reordering across volatile accesses */
#define USB_MEMORY_BARRIER()  __asm__ __volatile__("" ::: "memory")

/* --- Endpoint address direction bits --- */

#define EP_DIR_OUT      0x00u
#define EP_DIR_IN       0x80u
#define EP_DIR_MASK     0x80u
#define EP_NUM_MASK     0x0Fu

/* --- Endpoint type constants (match AVR DU USB_TYPE_* hardware encoding) --- */

#define EP_TYPE_CONTROL     0x00u
#define EP_TYPE_ISOCHRONOUS 0x01u
#define EP_TYPE_BULK        0x02u
#define EP_TYPE_INTERRUPT   0x03u

/* Control EP address */
#define EP_CTRL         0u

/* Default control EP bank size before descriptor is read */
#define EP_CTRL_SIZE_DEFAULT  8u

/* Max non-ISO endpoint bank size on AVR DU */
#define EP_MAX_SIZE     64u

/* Stream timeout in ms (used by ep_wait_ready) */
#ifndef USB_STREAM_TIMEOUT_MS
#define USB_STREAM_TIMEOUT_MS  100u
#endif

/* Total endpoint slots in hardware table (EP0..EP15) */
#define EP_TABLE_COUNT  16u

// ============================================================================
// USB Configuration
// ============================================================================
#define USB_ENDPOINT_SIZE   64  // Endpoint size in bytes: is this even used / needed?
#define USB_POLLING_RATE    1   // 1ms polling rate (1000Hz): is this even used / needed ?

/* --- USB device state machine states --- */

typedef enum {
    USB_STATE_UNATTACHED  = 0,
    USB_STATE_POWERED     = 1,
    USB_STATE_DEFAULT     = 2,
    USB_STATE_ADDRESSED   = 3,
    USB_STATE_CONFIGURED  = 4,
    USB_STATE_SUSPENDED   = 5,
} usb_dev_state_t;

/* --- Standard control request types --- */

/* bmRequestType direction field */
#define REQ_DIR_HOST_TO_DEV  (0u << 7)
#define REQ_DIR_DEV_TO_HOST  (1u << 7)
#define REQ_DIR_MASK         0x80u

/* bmRequestType type field */
#define REQ_TYPE_STANDARD    (0u << 5)
#define REQ_TYPE_CLASS       (1u << 5)
#define REQ_TYPE_VENDOR      (2u << 5)
#define REQ_TYPE_MASK        0x60u

/* bmRequestType recipient field */
#define REQ_REC_DEVICE       (0u << 0)
#define REQ_REC_INTERFACE    (1u << 0)
#define REQ_REC_ENDPOINT     (2u << 0)
#define REQ_REC_OTHER        (3u << 0)
#define REQ_REC_MASK         0x1Fu

/* Standard bRequest codes */
#define REQ_GET_STATUS        0u
#define REQ_CLEAR_FEATURE     1u
#define REQ_SET_FEATURE       3u
#define REQ_SET_ADDRESS       5u
#define REQ_GET_DESCRIPTOR    6u
#define REQ_SET_DESCRIPTOR    7u
#define REQ_GET_CONFIGURATION 8u
#define REQ_SET_CONFIGURATION 9u
#define REQ_GET_INTERFACE     10u
#define REQ_SET_INTERFACE     11u

/* Feature selector values */
#define FEAT_EP_HALT          0x00u
#define FEAT_REMOTE_WAKEUP    0x01u

/* Internal status response bits */
#define STATUS_SELF_POWERED   (1u << 0)
#define STATUS_REMOTE_WAKEUP  (1u << 1)

/* Packed SETUP packet received in usb_ctrl_req */
typedef struct USB_PACKED {
    uint8_t  bm_request_type;
    uint8_t  b_request;
    uint16_t w_value;
    uint16_t w_index;
    uint16_t w_length;
} usb_request_t;

_Static_assert(sizeof(usb_request_t) == 8, "usb_request_t must be 8 bytes");

/* --- Standard USB descriptor type codes --- */

#define DTYPE_DEVICE            0x01u
#define DTYPE_CONFIGURATION     0x02u
#define DTYPE_STRING            0x03u
#define DTYPE_INTERFACE         0x04u
#define DTYPE_ENDPOINT          0x05u
#define DTYPE_DEVICE_QUALIFIER  0x06u
#define DTYPE_IAD               0x0Bu

/* Sentinel for "descriptor not found" */
#define NO_DESCRIPTOR  0u

/* Helper macros for descriptor construction */
#define USB_CONFIG_POWER_MA(mA)        ((mA) >> 1)
#define USB_STRING_LEN(n_chars)        (2u + ((n_chars) << 1))
#define VERSION_BCD(maj, min, rev)     cpu_to_le16(((uint16_t)((maj) & 0xFF) << 8) | \
                                                   (((min) & 0x0F) << 4)           | \
                                                   ((rev) & 0x0F))

/* Config descriptor bmAttributes bits */
#define USB_CFG_ATTR_RESERVED     0x80u
#define USB_CFG_ATTR_SELF_POWERED 0x40u
#define USB_CFG_ATTR_REMOTE_WKUP  0x20u

/* Endpoint descriptor bmAttributes sync/usage bits */
#define EP_ATTR_NO_SYNC  (0u << 2)
#define EP_USAGE_DATA    (0u << 4)

/* English language ID for string descriptor 0 */
#define LANG_ID_ENG  0x0409u

/* --- Standard descriptor structs --- */
/* Using USB-IF field names (bLength, bDescriptorType, etc.)
   to match what you will write in usb_desc.c */

typedef struct USB_PACKED {
    uint8_t b_length;
    uint8_t b_descriptor_type;
} usb_desc_header_t;

typedef struct USB_PACKED {
    uint8_t  b_length;
    uint8_t  b_descriptor_type;
    uint16_t bcd_usb;
    uint8_t  b_device_class;
    uint8_t  b_device_sub_class;
    uint8_t  b_device_protocol;
    uint8_t  b_max_packet_size0;
    uint16_t id_vendor;
    uint16_t id_product;
    uint16_t bcd_device;
    uint8_t  i_manufacturer;
    uint8_t  i_product;
    uint8_t  i_serial_number;
    uint8_t  b_num_configurations;
} usb_desc_device_t;

typedef struct USB_PACKED {
    uint8_t  b_length;
    uint8_t  b_descriptor_type;
    uint16_t w_total_length;
    uint8_t  b_num_interfaces;
    uint8_t  b_configuration_value;
    uint8_t  i_configuration;
    uint8_t  bm_attributes;
    uint8_t  b_max_power;
} usb_desc_config_t;

typedef struct USB_PACKED {
    uint8_t b_length;
    uint8_t b_descriptor_type;
    uint8_t b_interface_number;
    uint8_t b_alternate_setting;
    uint8_t b_num_endpoints;
    uint8_t b_interface_class;
    uint8_t b_interface_sub_class;
    uint8_t b_interface_protocol;
    uint8_t i_interface;
} usb_desc_interface_t;

typedef struct USB_PACKED {
    uint8_t  b_length;
    uint8_t  b_descriptor_type;
    uint8_t  b_endpoint_address;
    uint8_t  bm_attributes;
    uint16_t w_max_packet_size;
    uint8_t  b_interval;
} usb_desc_endpoint_t;

/* String descriptor — variable length; use USB_STRING_LEN() for b_length */
typedef struct USB_PACKED {
    uint8_t b_length;
    uint8_t b_descriptor_type;
    wchar_t unicode_string[];
} usb_desc_string_t;

/* --- Shared globals (defined in usb_ctrl.c, declared here) --- */

extern volatile usb_dev_state_t  usb_device_state;
extern usb_request_t             usb_ctrl_req;

/* Configuration number set by SET_CONFIGURATION; 0 = unconfigured */
extern uint8_t  usb_config_num;

/* Remote wakeup enabled by host via SET_FEATURE */
extern bool     usb_remote_wakeup_enabled;

/* Control EP bank size — read from device descriptor at init time */
extern uint8_t  usb_ctrl_ep_size;

/* Set true once usb_init() has run */
extern volatile bool usb_is_initialized;

/* --- Internal serial number support (AVR64DU32 SIGROW) --- */

/* 16 bytes × 2 nibbles = 32 hex chars = 32 Unicode chars */
#define USB_SERIAL_NUM_CHARS  32u
#define USB_SERIAL_ADDR       (&SIGROW.SERNUM0)

#endif /* USB_TYPES_H */
