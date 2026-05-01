# TinyUSB Reference for AmberClick90 Firmware

This document extracts relevant USB vendor class and HID protocol information from the TinyUSB stack that will be useful for implementing the AmberClick90 keyboard firmware on the AVR64DU32.

## Table of Contents

1. [Overview](#overview)
2. [HID Keyboard Implementation](#hid-keyboard-implementation)
3. [HID Mouse Implementation](#hid-mouse-implementation)
4. [HID Consumer Controls](#hid-consumer-controls)
5. [Vendor Class Implementation](#vendor-class-implementation)
6. [USB Descriptor Structure](#usb-descriptor-structure)
7. [Boot Protocol Support](#boot-protocol-support)
8. [Implementation Callbacks](#implementation-callbacks)
9. [Report Descriptor Macros](#report-descriptor-macros)
10. [Key Code Definitions](#key-code-definitions)

---

## Overview

TinyUSB is a memory-safe, thread-safe USB stack that emphasizes:

- No dynamic allocation
- Deferred interrupt handling (ISR → task context)
- Portability across MCU families
- Support for multiple device classes

The AmberClick90 will implement:

- **Bootloader mode**: USB Vendor class (for firmware updates)
- **Application mode**: USB HID composite device (keyboard + mouse + consumer controls)

---

## HID Keyboard Implementation

### Boot Keyboard Report Structure

```c
/// Standard HID Boot Protocol Keyboard Report
typedef struct TU_ATTR_PACKED {
    uint8_t modifier;   // Keyboard modifier (KEYBOARD_MODIFIER_* masks)
    uint8_t reserved;   // Reserved for OEM use, always set to 0
    uint8_t keycode[6]; // Key codes (6KRO - 6 simultaneous keys)
} hid_keyboard_report_t;
```

### Keyboard Modifier Bits

```c
// Keyboard modifier masks (bitmask for modifier field)
#define KEYBOARD_MODIFIER_LEFTCTRL   0x01
#define KEYBOARD_MODIFIER_LEFTSHIFT  0x02
#define KEYBOARD_MODIFIER_LEFTALT    0x04
#define KEYBOARD_MODIFIER_LEFTGUI    0x08
#define KEYBOARD_MODIFIER_RIGHTCTRL  0x10
#define KEYBOARD_MODIFIER_RIGHTSHIFT 0x20
#define KEYBOARD_MODIFIER_RIGHTALT   0x40
#define KEYBOARD_MODIFIER_RIGHTGUI   0x80
```

### Sending Keyboard Reports

```c
// Send keyboard report (instance 0, report_id, modifier, 6 keycodes)
bool tud_hid_keyboard_report(uint8_t report_id, uint8_t modifier, const uint8_t keycode[6]);

// Example usage:
uint8_t keycode[6] = {HID_KEY_A, HID_KEY_NONE, HID_KEY_NONE, 
                      HID_KEY_NONE, HID_KEY_NONE, HID_KEY_NONE};
tud_hid_keyboard_report(REPORT_ID_KEYBOARD, KEYBOARD_MODIFIER_LEFTSHIFT, keycode);

// Release all keys:
uint8_t keycode[6] = {HID_KEY_NONE};
tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
```

### HID Keyboard Report Descriptor Macro

```c
// TinyUSB provides convenient macros for report descriptors
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(REPORT_ID_KEYBOARD))
};

// For boot keyboard (no report ID):
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD()
};
```

---

## HID Mouse Implementation

### Boot Mouse Report Structure

```c
/// Standard HID Boot Protocol Mouse Report
typedef struct TU_ATTR_PACKED {
    uint8_t buttons;    // Button mask (bit 0=left, 1=right, 2=middle)
    int8_t  x;          // X movement delta (-127 to +127)
    int8_t  y;          // Y movement delta (-127 to +127)
    int8_t  vertical;   // Vertical scroll wheel delta
    int8_t  horizontal; // Horizontal scroll wheel delta (optional)
} hid_mouse_report_t;
```

### Mouse Button Bits

```c
#define MOUSE_BUTTON_LEFT    0x01
#define MOUSE_BUTTON_RIGHT   0x02
#define MOUSE_BUTTON_MIDDLE  0x04
#define MOUSE_BUTTON_BACKWARD 0x08  // Optional
#define MOUSE_BUTTON_FORWARD 0x10   // Optional
```

### Sending Mouse Reports

```c
// Send mouse report
bool tud_hid_mouse_report(uint8_t report_id, uint8_t buttons, 
                          int8_t x, int8_t y, 
                          int8_t vertical, int8_t horizontal);

// Example: scroll wheel only (encoder use case)
tud_hid_mouse_report(REPORT_ID_MOUSE, 0, 0, 0, 1, 0); // Scroll up
tud_hid_mouse_report(REPORT_ID_MOUSE, 0, 0, 0, -1, 0); // Scroll down
```

### HID Mouse Report Descriptor Macro

```c
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(REPORT_ID_MOUSE))
};
```

---

## HID Consumer Controls

### Consumer Control Report Structure

Consumer controls handle media keys, volume, browser controls, etc.

```c
// Consumer control uses 16-bit usage codes
// Report structure (example):
typedef struct TU_ATTR_PACKED {
    uint16_t usage_code; // Consumer control usage (0x0000 = none)
} hid_consumer_report_t;
```

### Common Consumer Control Codes

```c
// Volume controls
#define HID_USAGE_CONSUMER_VOLUME_INCREMENT   0x00E9
#define HID_USAGE_CONSUMER_VOLUME_DECREMENT   0x00EA
#define HID_USAGE_CONSUMER_MUTE               0x00E2

// Media controls
#define HID_USAGE_CONSUMER_PLAY_PAUSE         0x00CD
#define HID_USAGE_CONSUMER_STOP               0x00B7
#define HID_USAGE_CONSUMER_SCAN_NEXT_TRACK    0x00B5
#define HID_USAGE_CONSUMER_SCAN_PREVIOUS_TRACK 0x00B6

// Browser controls
#define HID_USAGE_CONSUMER_AC_HOME            0x0223
#define HID_USAGE_CONSUMER_AC_BACK            0x0224
#define HID_USAGE_CONSUMER_AC_FORWARD         0x0225
#define HID_USAGE_CONSUMER_AC_REFRESH         0x0227
#define HID_USAGE_CONSUMER_AC_SEARCH          0x0221
#define HID_USAGE_CONSUMER_AC_BOOKMARKS       0x022A

// Application launch
#define HID_USAGE_CONSUMER_AL_EMAIL           0x018A
#define HID_USAGE_CONSUMER_AL_CALCULATOR      0x0192
#define HID_USAGE_CONSUMER_AL_LOCAL_BROWSER   0x0194 // My Computer
```

### Sending Consumer Reports

```c
// Send consumer control report (generic report sending)
uint16_t usage = HID_USAGE_CONSUMER_VOLUME_INCREMENT;
tud_hid_report(REPORT_ID_CONSUMER, &usage, sizeof(usage));

// Release (send 0x0000)
uint16_t usage = 0x0000;
tud_hid_report(REPORT_ID_CONSUMER, &usage, sizeof(usage));
```

### HID Consumer Report Descriptor Macro

```c
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(REPORT_ID_CONSUMER))
};
```

---

## Vendor Class Implementation

### Vendor Class Overview

Vendor class devices use custom protocols and require LibUSB on the host side. This is ideal for bootloader implementations.

### Vendor Class Configuration

```c
// In tusb_config.h (or equivalent):
#define CFG_TUD_VENDOR              1  // Enable vendor class
#define CFG_TUD_VENDOR_RX_BUFSIZE   64 // RX buffer size
#define CFG_TUD_VENDOR_TX_BUFSIZE   64 // TX buffer size
```

### Vendor Descriptor Structure

```c
// Vendor interface descriptor
// bInterfaceClass:    0xFF (Vendor Specific)
// bInterfaceSubClass: 0x00 (or custom)
// bInterfaceProtocol: 0x00 (or custom)

// Endpoint configuration:
// - One bulk IN endpoint (device to host)
// - One bulk OUT endpoint (host to device)
// - Endpoint size: typically 64 bytes for full-speed
```

### Vendor Class Callbacks

```c
// Invoked when vendor device is mounted
void tud_vendor_rx_cb(uint8_t itf);

// Optional TX complete callback
void tud_vendor_tx_cb(uint8_t itf, uint32_t sent_bytes);

// Check if vendor interface is ready
bool tud_vendor_mounted(void);

// Read data from host
uint32_t tud_vendor_read(void* buffer, uint32_t bufsize);

// Check available bytes to read
uint32_t tud_vendor_available(void);

// Write data to host
uint32_t tud_vendor_write(void const* buffer, uint32_t bufsize);

// Flush TX buffer
bool tud_vendor_write_flush(void);
```

### Example Vendor Protocol Implementation

```c
// Bootloader vendor class example:

void tud_vendor_rx_cb(uint8_t itf) {
    uint8_t buf[64];
    uint32_t count = tud_vendor_read(buf, sizeof(buf));

    if (count > 0) {
        // Process command
        uint8_t cmd = buf[0];

        switch(cmd) {
            case CMD_FLASH_ERASE:
                // Erase flash page
                break;
            case CMD_FLASH_WRITE:
                // Write flash data
                break;
            case CMD_FLASH_READ:
                // Read flash and send back
                tud_vendor_write(data, len);
                tud_vendor_write_flush();
                break;
            case CMD_RESET:
                // Reset to application
                break;
        }
    }
}
```

---

## USB Descriptor Structure

### Device Descriptor Template

```c
tusb_desc_device_t const desc_device = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,  // USB 2.0

    // For composite device:
    .bDeviceClass       = 0x00,    // Defined at interface level
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,

    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE, // 64 bytes

    .idVendor           = 0xCAFE,  // Your VID
    .idProduct          = 0x4090,  // Your PID
    .bcdDevice          = 0x0100,  // Device version

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};
```

### Configuration Descriptor for Composite HID Device

```c
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + \
                           TUD_HID_DESC_LEN + \
                           TUD_HID_DESC_LEN + \
                           TUD_HID_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Config descriptor
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface 0: Boot Keyboard
    TUD_HID_DESCRIPTOR(ITF_NUM_KEYBOARD, 0, HID_ITF_PROTOCOL_KEYBOARD,
                       sizeof(desc_hid_report_keyboard), EPNUM_HID_KBD,
                       CFG_TUD_HID_EP_BUFSIZE, 10),

    // Interface 1: Boot Mouse
    TUD_HID_DESCRIPTOR(ITF_NUM_MOUSE, 0, HID_ITF_PROTOCOL_MOUSE,
                       sizeof(desc_hid_report_mouse), EPNUM_HID_MOUSE,
                       CFG_TUD_HID_EP_BUFSIZE, 10),

    // Interface 2: Consumer Controls
    TUD_HID_DESCRIPTOR(ITF_NUM_CONSUMER, 0, HID_ITF_PROTOCOL_NONE,
                       sizeof(desc_hid_report_consumer), EPNUM_HID_CONSUMER,
                       CFG_TUD_HID_EP_BUFSIZE, 10)
};
```

### Configuration Descriptor for Vendor Class (Bootloader)

```c
#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)

uint8_t const desc_configuration[] = {
    // Config descriptor
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, 0x00, 100),

    // Vendor interface descriptor
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 0, EPNUM_VENDOR_OUT,
                          EPNUM_VENDOR_IN, CFG_TUD_VENDOR_EPSIZE)
};
```

---

## Boot Protocol Support

### Boot Protocol vs Report Protocol

- **Boot Protocol**: Simplified, fixed-format reports for BIOS/UEFI compatibility
- **Report Protocol**: Full-featured HID reports with report IDs

### Enabling Boot Protocol

```c
// In interface descriptor:
// bInterfaceSubClass = HID_SUBCLASS_BOOT (0x01)
// bInterfaceProtocol = HID_ITF_PROTOCOL_KEYBOARD (0x01) or 
//                      HID_ITF_PROTOCOL_MOUSE (0x02)

// Example interface descriptor for boot keyboard:
TUD_HID_DESCRIPTOR(
    ITF_NUM_KEYBOARD,          // Interface number
    0,                         // String index
    HID_ITF_PROTOCOL_KEYBOARD, // Boot protocol keyboard
    sizeof(desc_report),       // Report descriptor length
    EPNUM_HID,                 // Endpoint
    CFG_TUD_HID_EP_BUFSIZE,    // Endpoint size
    10                         // Polling interval (ms)
)
```

### Boot Protocol Callbacks

```c
// Invoked when received SET_PROTOCOL request
// protocol is either HID_PROTOCOL_BOOT (0) or HID_PROTOCOL_REPORT (1)
void tud_hid_set_protocol_cb(uint8_t instance, uint8_t protocol);

// Optional: boot mode callback
void tud_hid_boot_mode_cb(uint8_t boot_mode);
```

### Boot Protocol Report Format

```c
// Boot keyboard: 8 bytes (no report ID)
// [modifier][reserved][key1][key2][key3][key4][key5][key6]

// Boot mouse: 5 bytes (no report ID)
// [buttons][x][y][wheel][reserved]
```

---

## Implementation Callbacks

### Required HID Callbacks

```c
// Invoked when received GET_REPORT_DESCRIPTOR request
// Application returns pointer to descriptor
uint8_t const* tud_hid_descriptor_report_cb(uint8_t instance) {
    // Return appropriate report descriptor based on instance
    switch(instance) {
        case 0: return desc_hid_report_keyboard;
        case 1: return desc_hid_report_mouse;
        case 2: return desc_hid_report_consumer;
        default: return NULL;
    }
}

// Invoked when received GET_REPORT control request
// Application fills buffer and returns length (0 = STALL)
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                                hid_report_type_t report_type,
                                uint8_t* buffer, uint16_t reqlen) {
    // Return current report state
    return 0;  // Or actual report length
}

// Invoked when received SET_REPORT control request
// or received data on OUT endpoint
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                            hid_report_type_t report_type,
                            uint8_t const* buffer, uint16_t bufsize) {
    // Handle LED state for keyboard (Num Lock, Caps Lock, Scroll Lock)
    if (report_type == HID_REPORT_TYPE_OUTPUT) {
        // buffer[0] contains LED state bits
        uint8_t leds = buffer[0];
        // Update LED states
    }
}

// Invoked when received SET_IDLE request
bool tud_hid_set_idle_cb(uint8_t idle_rate) {
    // idle_rate is in 4ms units (0 = send only on change)
    return true;  // Accept request
}
```

### Device-Level Callbacks

```c
// Invoked when device is mounted
void tud_mount_cb(void) {
    // Device enumeration complete
}

// Invoked when device is unmounted
void tud_umount_cb(void) {
    // USB cable disconnected
}

// Invoked when USB bus is suspended
void tud_suspend_cb(bool remote_wakeup_en) {
    // Enter low power mode if needed
}

// Invoked when USB bus is resumed
void tud_resume_cb(void) {
    // Exit low power mode
}
```

---

## Report Descriptor Macros

### TinyUSB Provides Convenient Macros

```c
// Keyboard (boot protocol compatible)
TUD_HID_REPORT_DESC_KEYBOARD(report_id)

// Mouse (boot protocol compatible)
TUD_HID_REPORT_DESC_MOUSE(report_id)

// Consumer controls (media, volume, etc.)
TUD_HID_REPORT_DESC_CONSUMER(report_id)

// Gamepad
TUD_HID_REPORT_DESC_GAMEPAD(report_id)

// Leave report_id empty for single-report devices:
TUD_HID_REPORT_DESC_KEYBOARD()
```

### Composite Report Descriptor Example

```c
// Multiple report types in single interface
uint8_t const desc_hid_report[] = {
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(2)),
    TUD_HID_REPORT_DESC_CONSUMER(HID_REPORT_ID(3))
};
```

---

## Key Code Definitions

### Common HID Keyboard Usage Codes

TinyUSB defines standard HID keyboard codes in `hid.h`:

```c
// Alphabetic keys
#define HID_KEY_A              0x04
#define HID_KEY_B              0x05
// ... through ...
#define HID_KEY_Z              0x1D

// Number keys
#define HID_KEY_1              0x1E
#define HID_KEY_2              0x1F
// ... through ...
#define HID_KEY_0              0x27

// Function keys
#define HID_KEY_F1             0x3A
#define HID_KEY_F2             0x3B
// ... through ...
#define HID_KEY_F24            0x73

// Special keys
#define HID_KEY_ESCAPE         0x29
#define HID_KEY_BACKSPACE      0x2A
#define HID_KEY_TAB            0x2B
#define HID_KEY_SPACE          0x2C
#define HID_KEY_ENTER          0x28
#define HID_KEY_DELETE         0x4C
#define HID_KEY_INSERT         0x49

// Navigation
#define HID_KEY_HOME           0x4A
#define HID_KEY_PAGE_UP        0x4B
#define HID_KEY_PAGE_DOWN      0x4E
#define HID_KEY_END            0x4D
#define HID_KEY_ARROW_RIGHT    0x4F
#define HID_KEY_ARROW_LEFT     0x50
#define HID_KEY_ARROW_DOWN     0x51
#define HID_KEY_ARROW_UP       0x52

// Modifiers (use in modifier field, not keycode array)
#define HID_KEY_CONTROL_LEFT   0xE0
#define HID_KEY_SHIFT_LEFT     0xE1
#define HID_KEY_ALT_LEFT       0xE2
#define HID_KEY_GUI_LEFT       0xE3
#define HID_KEY_CONTROL_RIGHT  0xE4
#define HID_KEY_SHIFT_RIGHT    0xE5
#define HID_KEY_ALT_RIGHT      0xE6
#define HID_KEY_GUI_RIGHT      0xE7

// Special value
#define HID_KEY_NONE           0x00

// System power control
#define HID_KEY_POWER          0x66  // Keyboard Power
```

### Keypad Codes

```c
#define HID_KEY_KEYPAD_DIVIDE      0x54
#define HID_KEY_KEYPAD_MULTIPLY    0x55
#define HID_KEY_KEYPAD_SUBTRACT    0x56
#define HID_KEY_KEYPAD_ADD         0x57
#define HID_KEY_KEYPAD_ENTER       0x58
#define HID_KEY_KEYPAD_1           0x59
#define HID_KEY_KEYPAD_2           0x5A
// ... through ...
#define HID_KEY_KEYPAD_0           0x62
#define HID_KEY_KEYPAD_DECIMAL     0x63
```

---

## Implementation Notes for AmberClick90

### For Bootloader (Vendor Class)

1. Configure vendor class with bulk endpoints
2. Implement simple command protocol:
   - Flash erase/write/read commands
   - Reset to application command
   - Device info query
3. Use LibUSB on host side for communication

### For Keyboard Application (HID Composite)

1. Three separate HID interfaces:
   
   - Interface 0: Boot keyboard (6KRO, BIOS compatible)
   - Interface 1: Boot mouse (for encoder scroll, BIOS compatible)
   - Interface 2: Consumer controls (media keys, OS only)

2. Use report IDs to distinguish reports in callbacks

3. Matrix scanning → keycode array conversion:
   
   ```c
   uint8_t modifier = 0;
   uint8_t keycode[6] = {0};
   int key_count = 0;
   
   // Scan matrix, build modifier and keycode array
   for (each pressed key) {
       if (is_modifier(key)) {
           modifier |= modifier_bit(key);
       } else if (key_count < 6) {
           keycode[key_count++] = hid_keycode(key);
       }
   }
   
   // Send report
   tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode);
   ```

4. Encoder → mouse scroll:
   
   ```c
   // On encoder CW rotation:
   tud_hid_mouse_report(REPORT_ID_MOUSE, 0, 0, 0, 1, 0);
   
   // On encoder CCW rotation:
   tud_hid_mouse_report(REPORT_ID_MOUSE, 0, 0, 0, -1, 0);
   ```

5. Media keys → consumer reports:
   
   ```c
   // Volume up key pressed:
   uint16_t usage = HID_USAGE_CONSUMER_VOLUME_INCREMENT;
   tud_hid_report(REPORT_ID_CONSUMER, &usage, sizeof(usage));
   
   // Volume up key released:
   usage = 0x0000;
   tud_hid_report(REPORT_ID_CONSUMER, &usage, sizeof(usage));
   ```

### Main Loop Structure

```c
int main(void) {
    board_init();
    tusb_init();

    while (1) {
        tud_task(); // TinyUSB device task (must be called frequently)

        // Your keyboard logic
        keyboard_task();
    }
}

void keyboard_task(void) {
    // Only send reports if HID is ready
    if (!tud_hid_ready()) return;

    // Scan matrix
    scan_matrix();

    // Send reports based on state
    send_keyboard_report();
    send_mouse_report();      // If encoder moved
    send_consumer_report();   // If media key pressed
}
```

---

## References

- TinyUSB GitHub: https://github.com/hathach/tinyusb
- TinyUSB Documentation: https://docs.tinyusb.org
- HID Usage Tables: https://usb.org/sites/default/files/hut1_5.pdf
- USB HID Specification: https://www.usb.org/hid
- Boot Protocol Specification: Section 4 of Device Class Definition for HID

---

## Additional Resources

### Example Projects

- TinyUSB HID Composite Example: `examples/device/hid_composite/`
- TinyUSB Vendor Example: https://github.com/piersfinlayson/tinyusb-vendor-example
- Adafruit TinyUSB Arduino Library: https://github.com/adafruit/Adafruit_TinyUSB_Arduino

### Key Files to Study

- `src/class/hid/hid.h` - HID constants and structures
- `src/class/hid/hid_device.h` - HID device API and report descriptor macros
- `src/class/hid/hid_device.c` - HID device implementation
- `src/class/vendor/vendor_device.h` - Vendor class API
- `src/class/vendor/vendor_device.c` - Vendor class implementation
- `examples/device/hid_composite/src/usb_descriptors.c` - Example descriptors

---

**End of TinyUSB Reference for AmberClick90**
