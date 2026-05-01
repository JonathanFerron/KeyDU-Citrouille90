# citrouille90 USB education

**Created:** 12/26/2025 21:20:27  
**Updated:** 3/5/2026 20:44:18  
**Exported:** 3/28/2026 9:00:51  
**Link:** [https://claude.ai/chat/a2cfeeb1-0401-4c7f-a23a-6d65977084ef](https://claude.ai/chat/a2cfeeb1-0401-4c7f-a23a-6d65977084ef)  

## Prompt:
12/29/2025, 9:56:09 PM

which of the endpoint or transfer types would be expected to be used in the custom protocol bootloader approach? Control Transfers, 
* Interrupt Transfers
* Isochronous Transfers
* Bulk Transfers
?



## Response:
12/29/2025, 9:56:59 PM

Excellent question! This gets to the heart of understanding USB transfer types and their appropriate uses.

## For Custom Vendor Bootloader: Two Valid Approaches

### Approach 1: Control Transfers Only ✅ **RECOMMENDED FOR BOOTLOADERS**

**Use only EP0 (Control Endpoint) with Vendor Requests**

```c
// All communication happens via control transfers
// bmRequestType = 0x40 (Vendor, Host-to-Device)
// or 0xC0 (Vendor, Device-to-Host)

#define VENDOR_REQ_ERASE    0x01
#define VENDOR_REQ_WRITE    0x02
#define VENDOR_REQ_READ     0x03
#define VENDOR_REQ_RESET    0x04

// Host sends:
// SETUP packet with vendor request
// Optional DATA stage (up to 64 bytes per transfer for Full Speed)
// STATUS stage
```

**Why this is best for bootloaders:**
- Simplest implementation (EP0 is mandatory anyway)
- No additional endpoints needed
- No buffer management complexity
- Control transfers have built-in error checking
- 64 byte max per transfer is fine for bootloader speeds
- Standard pattern for small vendor protocols

**Size:** ~2-3 KB

### Approach 2: Control + Bulk Transfers ⚠️ **MORE COMPLEX, FASTER**

**Use EP0 for commands, EP1 OUT for data, EP1 IN for status**

```c
// Control transfers for commands
bmRequestType = 0x40
bRequest = VENDOR_CMD_START_FLASH
wValue = address_high
wIndex = address_low

// Then bulk transfers for large data
// EP1 OUT: Receive firmware data (up to 64 bytes/packet FS, 512 HS)
// EP1 IN: Send acknowledgments/status

// Endpoint descriptors needed:
Endpoint 1 OUT: bEndpointAddress = 0x01, Type = Bulk
Endpoint 1 IN:  bEndpointAddress = 0x81, Type = Bulk
```

**Why someone might use this:**
- Faster for large firmware files (less protocol overhead)
- Can stream data more efficiently
- Better for High Speed USB (but you're using Full Speed)

**Drawbacks:**
- Need to configure additional endpoints
- Buffer management for bulk endpoints
- More complex state machine
- Larger code size

**Size:** ~3-4 KB

---

## Analysis of Each Transfer Type:

### **Control Transfers** ✅ **YES - USE THIS**

**Characteristics:**
- **Guaranteed bandwidth** - Cannot fail due to bus congestion
- **Reliable** - Built-in error detection and retry
- **Structured** - SETUP/DATA/STATUS stages
- **Small packets** - 8/16/32/64 bytes max (depending on speed)
- **Bidirectional** - Can send and receive on EP0

**Perfect for:**
- Commands (erase, write, reset)
- Small data transfers (bootloader chunks)
- Configuration and status

**Bootloader usage:**
```c
// Erase command
ctrl_transfer(0x40, ERASE, page_addr_hi, page_addr_lo, NULL, 0);

// Write 64 bytes
ctrl_transfer(0x40, WRITE, addr_hi, addr_lo, data_buf, 64);

// Read status
ctrl_transfer(0xC0, STATUS, 0, 0, status_buf, 4);
```

**Verdict:** ⭐⭐⭐⭐⭐ Perfect choice for bootloaders

---

### **Interrupt Transfers** ❌ **NO - WRONG USE CASE**

**Characteristics:**
- **Guaranteed latency** - Will be polled at regular intervals (1-255ms)
- **Small packets** - Max 64 bytes for Full Speed
- **One direction per endpoint**
- **Meant for periodic data**

**Perfect for:**
- **Keyboards** - Send keypresses (that's your HID application!)
- **Mice** - Send movement/button data
- **Game controllers**

**Why NOT for bootloaders:**
- Bootloader doesn't need guaranteed latency
- Bootloader needs request/response pattern, not periodic updates
- Wasted bandwidth polling when nothing to send
- Control transfers already provide reliability

**Verdict:** ❌ Wrong transfer type for bootloaders

---

### **Isochronous Transfers** ❌ **DEFINITELY NO**

**Characteristics:**
- **Guaranteed bandwidth BUT no error checking**
- **Time-critical streaming**
- **Can lose data** - No retry mechanism
- **Large packets** - Up to 1023 bytes for Full Speed

**Perfect for:**
- **Audio streaming** - A few dropped samples are okay
- **Video streaming** - A few dropped frames are okay
- **Real-time sensors** - Old data is useless anyway

**Why NOT for bootloaders:**
- **Cannot afford data loss** - Corrupted firmware = bricked device
- Bootloader needs reliability over speed
- No error checking is disqualifying

**Verdict:** ❌❌ Absolutely wrong for bootloaders

---

### **Bulk Transfers** ⚠️ **OPTIONAL - OVERKILL FOR FULL SPEED**

**Characteristics:**
- **Best-effort delivery** - Uses leftover bandwidth
- **Reliable** - Error detection and retry
- **Large packets** - 8/16/32/64 bytes for Full Speed, **512 bytes for High Speed**
- **High throughput** - Best for large data

**Perfect for:**
- **Mass Storage** (USB flash drives)
- **Printers**
- **Scanners**
- **Fast data transfer** applications

**For bootloaders:**
- More complex than control transfers
- Benefit is minimal at Full Speed (64 bytes max anyway)
- Would shine at High Speed (512 byte packets)
- Adds endpoint configuration complexity

**Verdict:** ⚠️ Possible but unnecessary complexity for your use case

---

## Concrete Recommendation for KeyDU:

### **Use Control Transfers Only (EP0)**

**Your descriptor would be minimal:**
```c
Device Descriptor
Configuration Descriptor
  Interface Descriptor
    // That's it! No endpoint descriptors needed
    // (EP0 is implicit)
```

**Your firmware code:**
```c
void handle_vendor_request(usb_setup_packet_t *setup) {
    switch(setup->bRequest) {
        case VENDOR_REQ_ERASE:
            flash_erase_page(setup->wValue << 16 | setup->wIndex);
            usb_send_zlp(); // Zero-length packet ACK
            break;
            
        case VENDOR_REQ_WRITE:
            // setup->wValue = address high
            // setup->wIndex = address low
            // Data comes in DATA stage (up to 64 bytes)
            usb_receive_control_data(flash_buffer, setup->wLength);
            flash_write_page(address, flash_buffer, setup->wLength);
            usb_send_zlp();
            break;
            
        case VENDOR_REQ_RESET:
            jump_to_application();
            break;
    }
}
```

**Your Python flasher:**
```python
import usb.core

dev = usb.core.find(idVendor=VID, idProduct=PID)

# Erase (control transfer, no data stage)
dev.ctrl_transfer(0x40, VENDOR_REQ_ERASE, page_addr_hi, page_addr_lo)

# Write (control transfer with data stage)
for i in range(0, len(firmware), 64):
    chunk = firmware[i:i+64]
    addr = base_addr + i
    dev.ctrl_transfer(0x40, VENDOR_REQ_WRITE, 
                      addr >> 16, addr & 0xFFFF, chunk)

# Reset
dev.ctrl_transfer(0x40, VENDOR_REQ_RESET, 0, 0)
```

---

## Why Control-Only Wins:

1. **Simplest implementation** - No endpoint configuration
2. **Smallest code** - ~2 KB
3. **Most reliable** - Built-in error checking
4. **Fast enough** - 64 bytes × 1000 transfers/sec = 64 KB/sec theoretical
   - Your bootloader needs maybe 30KB? That's < 1 second
5. **Educational** - You'll deeply understand control transfers, which are the foundation of USB
6. **Standard pattern** - Most simple vendor protocols use this

---

## Transfer Type Summary for Your Project:

| Transfer Type | Use in Bootloader? | Use in Keyboard App? |
|---------------|-------------------|---------------------|
| **Control** | ✅ **YES - Primary** | Only for enumeration |
| **Interrupt** | ❌ No | ✅ **YES - HID reports** |
| **Bulk** | ⚠️ Optional, overkill | ❌ No |
| **Isochronous** | ❌ Never | ❌ No |

---

## Final Answer:

**Use Control Transfers exclusively for your custom bootloader.**

- Simplest
- Smallest
- Most appropriate
- Best for learning

Save Interrupt transfers for your keyboard application (HID reports), where they belong!

If you were doing High Speed USB with large firmware files (>512 KB), then Bulk transfers would make sense. But for Full Speed with ~30KB firmware? Control transfers are perfect.



## Prompt:
12/29/2025, 10:07:16 PM

what would the host side python flasher code you just mentioned in your last reply look like in pure c?



## Response:
12/29/2025, 10:08:08 PM

Great question! Here's what the host-side flasher would look like in pure C using libusb.

**KeyDU Custom Bootloader Flasher (libusb)**

````c
/*
 * KeyDU Custom Bootloader Flasher
 * 
 * Flashes firmware to AVR-DU keyboard using custom vendor protocol
 * Uses control transfers on EP0
 * 
 * Compile:
 *   Linux:   gcc -o keydu-flash keydu-flash.c -lusb-1.0
 *   macOS:   gcc -o keydu-flash keydu-flash.c -lusb-1.0
 *   Windows: gcc -o keydu-flash.exe keydu-flash.c -lusb-1.0
 * 
 * Usage:
 *   ./keydu-flash firmware.bin
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

// Your VID/PID - change these to your actual values
#define VENDOR_ID   0x03EB  // Example: Atmel/Microchip VID
#define PRODUCT_ID  0x2FF4  // Example: Your bootloader PID

// Vendor request commands (must match device firmware)
#define VENDOR_REQ_ERASE    0x01
#define VENDOR_REQ_WRITE    0x02
#define VENDOR_REQ_READ     0x03
#define VENDOR_REQ_RESET    0x04
#define VENDOR_REQ_STATUS   0x05

// USB control transfer parameters
#define USB_TIMEOUT         5000  // 5 second timeout
#define CHUNK_SIZE          64    // Full Speed max control data
#define FLASH_BASE_ADDR     0x0000
#define PAGE_SIZE           512   // AVR-DU flash page size (check datasheet)

// bmRequestType values
#define REQTYPE_HOST_TO_DEV 0x40  // Vendor, Host-to-Device
#define REQTYPE_DEV_TO_HOST 0xC0  // Vendor, Device-to-Host

// Status codes from device
#define STATUS_OK           0x00
#define STATUS_ERROR        0xFF

// Function prototypes
int find_device(libusb_device_handle **handle);
int erase_flash(libusb_device_handle *handle, uint32_t addr);
int write_chunk(libusb_device_handle *handle, uint32_t addr, 
                uint8_t *data, size_t len);
int read_status(libusb_device_handle *handle, uint8_t *status);
int reset_device(libusb_device_handle *handle);
uint8_t *read_firmware_file(const char *filename, size_t *size);

int main(int argc, char **argv) {
    libusb_device_handle *handle = NULL;
    uint8_t *firmware = NULL;
    size_t firmware_size = 0;
    int ret;
    
    printf("KeyDU Custom Bootloader Flasher\n");
    printf("================================\n\n");
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <firmware.bin>\n", argv[0]);
        return 1;
    }
    
    // Initialize libusb
    ret = libusb_init(NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to initialize libusb: %s\n", 
                libusb_error_name(ret));
        return 1;
    }
    
    // Find and open device
    printf("Searching for KeyDU bootloader...\n");
    if (find_device(&handle) != 0) {
        fprintf(stderr, "Device not found. Is it in bootloader mode?\n");
        libusb_exit(NULL);
        return 1;
    }
    printf("Device found!\n\n");
    
    // Read firmware file
    printf("Reading firmware file: %s\n", argv[1]);
    firmware = read_firmware_file(argv[1], &firmware_size);
    if (!firmware) {
        fprintf(stderr, "Failed to read firmware file\n");
        libusb_close(handle);
        libusb_exit(NULL);
        return 1;
    }
    printf("Firmware size: %zu bytes\n\n", firmware_size);
    
    // Erase flash (page by page or whole chip depending on implementation)
    printf("Erasing flash...\n");
    for (uint32_t addr = FLASH_BASE_ADDR; 
         addr < FLASH_BASE_ADDR + firmware_size; 
         addr += PAGE_SIZE) {
        
        if (erase_flash(handle, addr) != 0) {
            fprintf(stderr, "Erase failed at address 0x%08X\n", addr);
            free(firmware);
            libusb_close(handle);
            libusb_exit(NULL);
            return 1;
        }
        printf("  Erased page at 0x%08X\r", addr);
        fflush(stdout);
    }
    printf("\nErase complete!\n\n");
    
    // Write firmware in chunks
    printf("Writing firmware...\n");
    size_t bytes_written = 0;
    
    while (bytes_written < firmware_size) {
        size_t chunk_len = firmware_size - bytes_written;
        if (chunk_len > CHUNK_SIZE) {
            chunk_len = CHUNK_SIZE;
        }
        
        uint32_t addr = FLASH_BASE_ADDR + bytes_written;
        
        ret = write_chunk(handle, addr, 
                         firmware + bytes_written, chunk_len);
        if (ret != 0) {
            fprintf(stderr, "\nWrite failed at address 0x%08X\n", addr);
            free(firmware);
            libusb_close(handle);
            libusb_exit(NULL);
            return 1;
        }
        
        bytes_written += chunk_len;
        
        // Progress bar
        int progress = (bytes_written * 100) / firmware_size;
        printf("  Progress: %3d%% (%zu/%zu bytes)\r", 
               progress, bytes_written, firmware_size);
        fflush(stdout);
    }
    printf("\nWrite complete!\n\n");
    
    // Verify (optional - read back and compare)
    // TODO: Add verification if device supports read command
    
    // Reset device to application mode
    printf("Resetting device...\n");
    reset_device(handle);
    printf("Done! Device should now be running application.\n");
    
    // Cleanup
    free(firmware);
    libusb_close(handle);
    libusb_exit(NULL);
    
    return 0;
}

int find_device(libusb_device_handle **handle) {
    // Try to open device with VID/PID
    *handle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    
    if (*handle == NULL) {
        return -1;
    }
    
    // Claim interface 0
    int ret = libusb_claim_interface(*handle, 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to claim interface: %s\n", 
                libusb_error_name(ret));
        libusb_close(*handle);
        *handle = NULL;
        return -1;
    }
    
    return 0;
}

int erase_flash(libusb_device_handle *handle, uint32_t addr) {
    // Address split into wValue (high 16 bits) and wIndex (low 16 bits)
    uint16_t addr_hi = (addr >> 16) & 0xFFFF;
    uint16_t addr_lo = addr & 0xFFFF;
    
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_ERASE,          // bRequest
        addr_hi,                   // wValue
        addr_lo,                   // wIndex
        NULL,                      // data
        0,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    return 0;
}

int write_chunk(libusb_device_handle *handle, uint32_t addr, 
                uint8_t *data, size_t len) {
    
    if (len > CHUNK_SIZE) {
        fprintf(stderr, "Chunk too large: %zu bytes\n", len);
        return -1;
    }
    
    // Address split into wValue (high 16 bits) and wIndex (low 16 bits)
    uint16_t addr_hi = (addr >> 16) & 0xFFFF;
    uint16_t addr_lo = addr & 0xFFFF;
    
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_WRITE,          // bRequest
        addr_hi,                   // wValue
        addr_lo,                   // wIndex
        data,                      // data
        len,                       // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    if ((size_t)ret != len) {
        fprintf(stderr, "Partial write: expected %zu, got %d\n", len, ret);
        return -1;
    }
    
    return 0;
}

int read_status(libusb_device_handle *handle, uint8_t *status) {
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_DEV_TO_HOST,      // bmRequestType
        VENDOR_REQ_STATUS,         // bRequest
        0,                         // wValue
        0,                         // wIndex
        status,                    // data buffer
        1,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    return 0;
}

int reset_device(libusb_device_handle *handle) {
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_RESET,          // bRequest
        0,                         // wValue
        0,                         // wIndex
        NULL,                      // data
        0,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    // Device may reset before responding, so don't check return value
    (void)ret;
    
    return 0;
}

uint8_t *read_firmware_file(const char *filename, size_t *size) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (fsize < 0) {
        perror("ftell");
        fclose(f);
        return NULL;
    }
    
    // Allocate buffer
    uint8_t *buffer = malloc(fsize);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate %ld bytes\n", fsize);
        fclose(f);
        return NULL;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, fsize, f);
    if (bytes_read != (size_t)fsize) {
        fprintf(stderr, "Failed to read file: expected %ld, got %zu\n", 
                fsize, bytes_read);
        free(buffer);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    *size = fsize;
    return buffer;
}
````

I've created a complete C implementation of the host-side flasher using libusb-1.0. Here are the key points:

## Key Features:

**Control Transfer Implementation:**
```c
libusb_control_transfer(
    handle,
    bmRequestType,  // 0x40 = Host-to-Device, 0xC0 = Device-to-Host
    bRequest,       // Your vendor command (ERASE, WRITE, etc.)
    wValue,         // Address high 16 bits
    wIndex,         // Address low 16 bits
    data,           // Data buffer (or NULL)
    length,         // Data length
    timeout
);
```

**Address Encoding:**
- 32-bit addresses are split across wValue (high 16 bits) and wIndex (low 16 bits)
- This is a common pattern since USB control transfers only have 16-bit fields

## Installing libusb:

**Linux (Debian/Ubuntu):**
```bash
sudo apt-get install libusb-1.0-0-dev
```

**macOS (with Homebrew):**
```bash
brew install libusb
```

**Windows:**
- Download from https://libusb.info
- Or use MSYS2: `pacman -S mingw-w64-x86_64-libusb`

## Compiling:

```bash
# Linux/macOS
gcc -o keydu-flash keydu-flash.c -lusb-1.0

# Windows (MinGW)
gcc -o keydu-flash.exe keydu-flash.c -lusb-1.0
```

## Usage:

```bash
./keydu-flash firmware.bin
```

## What It Does:

1. **Finds device** by VID/PID
2. **Erases flash** page-by-page
3. **Writes firmware** in 64-byte chunks
4. **Shows progress** with percentage
5. **Resets device** to run application

## Advantages Over Python:

- No dependencies beyond libusb
- Faster execution
- Single binary (can be distributed easily)
- More portable to embedded Linux (like Raspberry Pi)
- Better for integration into build systems

## Disadvantages vs Python:

- More verbose
- Manual memory management
- More compilation complexity

The code matches your device-side vendor protocol and uses the same control transfer approach we discussed. You'd just need to adjust the VID/PID and potentially the page size for your specific AVR-DU variant!



## Prompt:
12/29/2025, 10:12:59 PM

installation of libusb on arch linux is just pacman -S libusb ? we could use keydu-flash-custom as the host side binary name. how do i find my avr-du page size in the datasheet?



## Response:
12/29/2025, 10:13:21 PM



**KeyDU Custom Bootloader Flasher (libusb)** ∙ Version 2

````c
/*
 * KeyDU Custom Bootloader Flasher
 * 
 * Flashes firmware to AVR-DU keyboard using custom vendor protocol
 * Uses control transfers on EP0
 * 
 * Install libusb:
 *   Arch:    sudo pacman -S libusb
 *   Debian:  sudo apt-get install libusb-1.0-0-dev
 *   macOS:   brew install libusb
 * 
 * Compile:
 *   Linux:   gcc -o keydu-flash-custom keydu-flash.c -lusb-1.0
 *   macOS:   gcc -o keydu-flash-custom keydu-flash.c -lusb-1.0
 *   Windows: gcc -o keydu-flash-custom.exe keydu-flash.c -lusb-1.0
 * 
 * Usage:
 *   ./keydu-flash-custom firmware.bin
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

// Your VID/PID - change these to your actual values
#define VENDOR_ID   0x03EB  // Example: Atmel/Microchip VID
#define PRODUCT_ID  0x2FF4  // Example: Your bootloader PID

// Vendor request commands (must match device firmware)
#define VENDOR_REQ_ERASE    0x01
#define VENDOR_REQ_WRITE    0x02
#define VENDOR_REQ_READ     0x03
#define VENDOR_REQ_RESET    0x04
#define VENDOR_REQ_STATUS   0x05

// USB control transfer parameters
#define USB_TIMEOUT         5000  // 5 second timeout
#define CHUNK_SIZE          64    // Full Speed max control data
#define FLASH_BASE_ADDR     0x0000
#define PAGE_SIZE           512   // AVR-DU flash page size (check datasheet)

// bmRequestType values
#define REQTYPE_HOST_TO_DEV 0x40  // Vendor, Host-to-Device
#define REQTYPE_DEV_TO_HOST 0xC0  // Vendor, Device-to-Host

// Status codes from device
#define STATUS_OK           0x00
#define STATUS_ERROR        0xFF

// Function prototypes
int find_device(libusb_device_handle **handle);
int erase_flash(libusb_device_handle *handle, uint32_t addr);
int write_chunk(libusb_device_handle *handle, uint32_t addr, 
                uint8_t *data, size_t len);
int read_status(libusb_device_handle *handle, uint8_t *status);
int reset_device(libusb_device_handle *handle);
uint8_t *read_firmware_file(const char *filename, size_t *size);

int main(int argc, char **argv) {
    libusb_device_handle *handle = NULL;
    uint8_t *firmware = NULL;
    size_t firmware_size = 0;
    int ret;
    
    printf("KeyDU Custom Bootloader Flasher\n");
    printf("================================\n\n");
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <firmware.bin>\n", argv[0]);
        return 1;
    }
    
    // Initialize libusb
    ret = libusb_init(NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to initialize libusb: %s\n", 
                libusb_error_name(ret));
        return 1;
    }
    
    // Find and open device
    printf("Searching for KeyDU bootloader...\n");
    if (find_device(&handle) != 0) {
        fprintf(stderr, "Device not found. Is it in bootloader mode?\n");
        libusb_exit(NULL);
        return 1;
    }
    printf("Device found!\n\n");
    
    // Read firmware file
    printf("Reading firmware file: %s\n", argv[1]);
    firmware = read_firmware_file(argv[1], &firmware_size);
    if (!firmware) {
        fprintf(stderr, "Failed to read firmware file\n");
        libusb_close(handle);
        libusb_exit(NULL);
        return 1;
    }
    printf("Firmware size: %zu bytes\n\n", firmware_size);
    
    // Erase flash (page by page or whole chip depending on implementation)
    printf("Erasing flash...\n");
    for (uint32_t addr = FLASH_BASE_ADDR; 
         addr < FLASH_BASE_ADDR + firmware_size; 
         addr += PAGE_SIZE) {
        
        if (erase_flash(handle, addr) != 0) {
            fprintf(stderr, "Erase failed at address 0x%08X\n", addr);
            free(firmware);
            libusb_close(handle);
            libusb_exit(NULL);
            return 1;
        }
        printf("  Erased page at 0x%08X\r", addr);
        fflush(stdout);
    }
    printf("\nErase complete!\n\n");
    
    // Write firmware in chunks
    printf("Writing firmware...\n");
    size_t bytes_written = 0;
    
    while (bytes_written < firmware_size) {
        size_t chunk_len = firmware_size - bytes_written;
        if (chunk_len > CHUNK_SIZE) {
            chunk_len = CHUNK_SIZE;
        }
        
        uint32_t addr = FLASH_BASE_ADDR + bytes_written;
        
        ret = write_chunk(handle, addr, 
                         firmware + bytes_written, chunk_len);
        if (ret != 0) {
            fprintf(stderr, "\nWrite failed at address 0x%08X\n", addr);
            free(firmware);
            libusb_close(handle);
            libusb_exit(NULL);
            return 1;
        }
        
        bytes_written += chunk_len;
        
        // Progress bar
        int progress = (bytes_written * 100) / firmware_size;
        printf("  Progress: %3d%% (%zu/%zu bytes)\r", 
               progress, bytes_written, firmware_size);
        fflush(stdout);
    }
    printf("\nWrite complete!\n\n");
    
    // Verify (optional - read back and compare)
    // TODO: Add verification if device supports read command
    
    // Reset device to application mode
    printf("Resetting device...\n");
    reset_device(handle);
    printf("Done! Device should now be running application.\n");
    
    // Cleanup
    free(firmware);
    libusb_close(handle);
    libusb_exit(NULL);
    
    return 0;
}

int find_device(libusb_device_handle **handle) {
    // Try to open device with VID/PID
    *handle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    
    if (*handle == NULL) {
        return -1;
    }
    
    // Claim interface 0
    int ret = libusb_claim_interface(*handle, 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to claim interface: %s\n", 
                libusb_error_name(ret));
        libusb_close(*handle);
        *handle = NULL;
        return -1;
    }
    
    return 0;
}

int erase_flash(libusb_device_handle *handle, uint32_t addr) {
    // Address split into wValue (high 16 bits) and wIndex (low 16 bits)
    uint16_t addr_hi = (addr >> 16) & 0xFFFF;
    uint16_t addr_lo = addr & 0xFFFF;
    
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_ERASE,          // bRequest
        addr_hi,                   // wValue
        addr_lo,                   // wIndex
        NULL,                      // data
        0,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    return 0;
}

int write_chunk(libusb_device_handle *handle, uint32_t addr, 
                uint8_t *data, size_t len) {
    
    if (len > CHUNK_SIZE) {
        fprintf(stderr, "Chunk too large: %zu bytes\n", len);
        return -1;
    }
    
    // Address split into wValue (high 16 bits) and wIndex (low 16 bits)
    uint16_t addr_hi = (addr >> 16) & 0xFFFF;
    uint16_t addr_lo = addr & 0xFFFF;
    
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_WRITE,          // bRequest
        addr_hi,                   // wValue
        addr_lo,                   // wIndex
        data,                      // data
        len,                       // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    if ((size_t)ret != len) {
        fprintf(stderr, "Partial write: expected %zu, got %d\n", len, ret);
        return -1;
    }
    
    return 0;
}

int read_status(libusb_device_handle *handle, uint8_t *status) {
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_DEV_TO_HOST,      // bmRequestType
        VENDOR_REQ_STATUS,         // bRequest
        0,                         // wValue
        0,                         // wIndex
        status,                    // data buffer
        1,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    return 0;
}

int reset_device(libusb_device_handle *handle) {
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_RESET,          // bRequest
        0,                         // wValue
        0,                         // wIndex
        NULL,                      // data
        0,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    // Device may reset before responding, so don't check return value
    (void)ret;
    
    return 0;
}

uint8_t *read_firmware_file(const char *filename, size_t *size) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (fsize < 0) {
        perror("ftell");
        fclose(f);
        return NULL;
    }
    
    // Allocate buffer
    uint8_t *buffer = malloc(fsize);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate %ld bytes\n", fsize);
        fclose(f);
        return NULL;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, fsize, f);
    if (bytes_read != (size_t)fsize) {
        fprintf(stderr, "Failed to read file: expected %ld, got %zu\n", 
                fsize, bytes_read);
        free(buffer);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    *size = fsize;
    return buffer;
}
````

Yes, exactly! On Arch Linux it's just:

```bash
sudo pacman -S libusb
```

I've updated the code to use `keydu-flash-custom` as the binary name.

## Finding AVR-DU Flash Page Size in the Datasheet:

The flash page size depends on which specific AVR-DU chip you're using. Here's how to find it:

### Method 1: Table of Contents / Index
Look for sections titled:
- "Flash Memory" 
- "Memory Organization"
- "NVM (Non-Volatile Memory)"
- "Flash Programming"

### Method 2: Memory Organization Section
Usually in a section called "Memory Organization" or "Memories", there will be a table showing:
- Total Flash Size
- **Flash Page Size** ← This is what you need
- EEPROM Size
- SRAM Size

### Method 3: NVM Controller Section
Look for "NVMCTRL" (Non-Volatile Memory Controller) section, which describes flash operations. Page size is usually specified there.

### Method 4: Quick Reference Table
Most datasheets have a "Device Overview" or "Feature Summary" table near the beginning with all memory specs.

## Common AVR-DU Page Sizes:

Based on typical AVR architecture:

| AVR-DU Series | Likely Page Size |
|---------------|------------------|
| AVR16DU14/20/28/32 | **512 bytes** (most common) |
| AVR32DU14/20/28/32 | **512 bytes** (most common) |
| AVR64DU28/32 | **512 bytes** (most common) |

**Most AVR devices use 512 bytes**, but verify in your specific datasheet!

## Where to Look in AVR64DU32 Datasheet (Example):

1. **Page 32-35**: "Memory Organization" section
2. **Page 450-460**: "NVMCTRL - Non-Volatile Memory Controller" section
3. Look for phrases like:
   - "Flash is organized in pages of XXX bytes"
   - "Page size: XXX bytes"
   - "FLASH_PAGE_SIZE = XXX"

## Quick Verification:

If you have the AVR-DU header files (from Microchip Packs or MCC), search for:
```c
#define FLASH_PAGE_SIZE
// or
#define MAPPED_PROGMEM_PAGE_SIZE
```

## If You Can't Find It:

Tell me your specific AVR-DU part number (e.g., AVR64DU32) and I can help you search for the exact page size, or you can check the specific datasheet section I mentioned above.

**Most likely it's 512 bytes**, which is already in the code, so you're probably good to go!



## Prompt:
12/29/2025, 10:32:42 PM

the page size is 512 bytes. so the flash erase instructions on the avr-du can only erase one page at a time?



## Response:
12/29/2025, 10:33:13 PM

Great question! The answer depends on the AVR-DU's NVM controller capabilities. Let me explain the typical options:

## Flash Erase Granularity on AVR Devices

### Page-by-Page Erase (Most Common)
**Yes, typically AVR flash can only be erased one page at a time.**

The NVMCTRL (Non-Volatile Memory Controller) usually provides these erase commands:
- **Page Erase** - Erases one 512-byte page
- **Chip Erase** - Erases the entire flash (but this usually erases EEPROM too, and takes longer)

There's usually **NO** "erase multiple contiguous pages" command.

### Why Page-at-a-Time?

1. **Flash Technology**: NOR flash (used in AVRs) is organized in sectors/pages that must be erased as a unit
2. **Hardware Limitation**: The erase circuitry works on page boundaries
3. **Data Integrity**: Partial page erases aren't possible - it's all or nothing per page

## What This Means for Your Bootloader:

### Device Side (AVR-DU Firmware):
```c
void vendor_erase_page(uint32_t address) {
    // Align address to page boundary
    address &= ~(FLASH_PAGE_SIZE - 1);  // Clear lower 9 bits for 512-byte pages
    
    // Wait for NVM to be ready
    while (NVMCTRL.STATUS & NVMCTRL_FBUSY_bm);
    
    // Execute page erase command
    // (Exact register names depend on AVR-DU - check datasheet)
    NVMCTRL.ADDR = address;
    NVMCTRL.CTRLA = NVMCTRL_CMD_PAGEERASE_gc;
    
    // Or use CPU write to mapped flash address to trigger erase
    // _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, NVMCTRL_CMD_PAGEERASE_gc);
    // *(volatile uint8_t *)address = 0xFF;
}
```

### Host Side (Already Handled):
Your host flasher already loops through pages:
```c
for (uint32_t addr = FLASH_BASE_ADDR; 
     addr < FLASH_BASE_ADDR + firmware_size; 
     addr += PAGE_SIZE) {
    erase_flash(handle, addr);  // Erase one page
}
```

## Alternative: Chip Erase

Some bootloaders use **chip erase** instead:

### Pros:
- Single command
- Might be slightly faster
- Simpler code

### Cons:
- Erases **everything** including EEPROM (usually)
- Can't do partial updates
- Longer erase time
- May erase bootloader itself if not in protected section

### When to Use Chip Erase:
- If you're always flashing the entire application space
- If bootloader is in a protected boot section
- If EEPROM doesn't contain user data

## Typical AVR-DU NVMCTRL Commands:

From the datasheet, look for commands like:
```c
NVMCTRL_CMD_NOOP_gc          // No operation
NVMCTRL_CMD_PAGEERASE_gc     // Erase one page ← You'll use this
NVMCTRL_CMD_PAGEWRITE_gc     // Write page buffer to flash
NVMCTRL_CMD_PAGEERASEWRITE_gc // Erase + Write in one operation
NVMCTRL_CMD_CHIPERASE_gc     // Erase entire chip
```

## Optimization: Erase-Write Combined Command

**Many AVRs support `PAGEERASEWRITE`** - a single command that:
1. Erases the page
2. Writes new data
3. All in one atomic operation

This is **more efficient** than separate erase + write:

```c
void vendor_write_page(uint32_t address, uint8_t *data, size_t len) {
    // Fill page buffer
    for (size_t i = 0; i < len; i++) {
        page_buffer[i] = data[i];
    }
    
    // Pad rest with 0xFF (erased state)
    for (size_t i = len; i < FLASH_PAGE_SIZE; i++) {
        page_buffer[i] = 0xFF;
    }
    
    // Use combined erase+write command
    NVMCTRL.ADDR = address;
    NVMCTRL.CTRLA = NVMCTRL_CMD_PAGEERASEWRITE_gc;
    
    // Copy page buffer to flash (hardware handles the rest)
    memcpy((void *)address, page_buffer, FLASH_PAGE_SIZE);
}
```

If you use this approach, you might **skip the separate erase loop** entirely!

## Recommendation for Your Bootloader:

**Option 1: Separate Erase + Write (What you have now)**
- Clear and explicit
- Easy to debug
- Works with all AVR variants

**Option 2: Combined Erase-Write (More efficient)**
- Faster flashing
- Less USB traffic
- Single command per page
- Check if AVR-DU supports `PAGEERASEWRITE`

**Option 3: Chip Erase (Simplest)**
- One command at start
- Only if bootloader is in protected boot section
- Fastest erase but erases everything

## Check Your AVR-DU Datasheet:

Look in the **NVMCTRL section** for:
1. Available erase commands
2. Whether `PAGEERASEWRITE` is supported
3. Boot section protection options

Most likely you'll want to use either **page-by-page erase** (current approach) or **combined erase-write** (more efficient).

**Bottom line: Yes, one page at a time is typical and correct!**



## Prompt:
12/29/2025, 10:52:14 PM

![1767062947745_image.png](data:image/webp;base64,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)

![1767063123272_image.png](data:image/webp;base64,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)

the chip erase command (CHER) can only be started from the updi and would destroy the bootloader area of the mcu flash. there is 8kb of nrww on the avr64du32. based on that fact and the attached screenshots, does that change my ideal approach to the size of the reserved bootloader section, as set via fuses?



## Response:
12/29/2025, 10:52:54 PM

Excellent catch! This **significantly changes your bootloader design strategy**.

## Key Insights from the Screenshots:

### NRWW vs RWW Flash Sections:

**NRWW (No-Read-While-Write) - 8KB:**
- CPU **halts** when erasing/writing this section
- **Cannot execute code** from here while programming
- Slower flash operations

**RWW (Read-While-Write) - Remaining flash:**
- CPU **continues running** when erasing/writing this section
- Can execute code from NRWW while programming RWW
- Faster, non-blocking flash operations

### The Critical Implication:

**Your bootloader MUST reside in the NRWW section!**

Why? Because:
1. The bootloader needs to **keep running** while it writes the application firmware
2. The application firmware goes in the **RWW section**
3. If bootloader were in RWW, the CPU would halt when writing RWW (including the bootloader itself) → instant brick

## Revised Bootloader Size Strategy:

### Previous Thinking:
"12 KB seems reasonable for any bootloader type"

### New Thinking Based on NRWW Constraint:

**You have EXACTLY 8 KB of NRWW flash available.**

This is your **hard ceiling** for bootloader size, not because of code complexity, but because of **hardware architecture**.

### Size Breakdown:

```
AVR64DU32 Flash Layout:
┌─────────────────────────────┐ 0x0000
│                             │
│   NRWW Section (8 KB)       │ ← Bootloader MUST go here
│                             │
├─────────────────────────────┤ 0x2000 (8192 bytes)
│                             │
│                             │
│   RWW Section (56 KB)       │ ← Application firmware goes here
│                             │
│                             │
└─────────────────────────────┘ 0x10000 (64 KB)
```

### Bootloader Size Target:

| Bootloader Type | Size | Fits in 8KB NRWW? | Recommended? |
|-----------------|------|-------------------|--------------|
| **Custom Vendor** | 2-4 KB | ✅ Yes (lots of room) | ✅ **BEST CHOICE** |
| **HID** | 3-5 KB | ✅ Yes (comfortable) | ✅ Good option |
| **DFU** | 4-6 KB | ✅ Yes (tight fit) | ⚠️ Possible but tight |
| **CDC** | 5-8 KB | ⚠️ Barely (no margin) | ❌ Too risky |
| **UF2 (Mass Storage)** | 8-12 KB | ❌ **NO** | ❌ Won't fit |

## Updated Recommendation:

### Best Approach: Custom Vendor Protocol ✅

**Target size: 4-6 KB** (leaves 2-4 KB safety margin)

Why this matters now:
1. **NRWW constraint is HARD** - you cannot exceed 8 KB
2. You need margin for:
   - Future features
   - Debug code during development
   - Verification routines
   - Error handling
3. Custom vendor is the most space-efficient

### Fuse Configuration:

You'll need to set the **BOOTSIZE fuse** to reserve the bootloader section:

```c
// Typical AVR-DU fuse setting (check your exact datasheet)
FUSES = {
    .BOOTSIZE = BOOT_SIZE_8KB,  // Reserve 8KB for bootloader
    // ... other fuses
};
```

This protects the bootloader from being overwritten by the application.

### Memory Map with Fuses:

```
With BOOTSIZE = 8KB:

┌─────────────────────────────┐ 0x0000
│   BOOTLOADER SECTION        │
│   (Protected, 8 KB NRWW)    │ ← Bootloader code here
│   - USB stack               │   Cannot be overwritten
│   - Flash routines          │   by application
│   - Jump logic              │
├─────────────────────────────┤ 0x2000
│   APPLICATION SECTION       │
│   (Writable, 56 KB RWW)     │ ← Your keyboard firmware
│   - HID stack               │   Bootloader writes here
│   - Keyboard logic          │   while still running
│   - LUFA/custom code        │
└─────────────────────────────┘ 0x10000
```

## Why This Architecture Works Perfectly:

**Scenario: Flashing new firmware**

1. User triggers bootloader (press `QK_BOOT`)
2. Bootloader starts running from **NRWW** (0x0000-0x2000)
3. Host sends application firmware data
4. Bootloader erases/writes pages in **RWW** (0x2000-0x10000)
5. **CPU keeps running** because bootloader is in NRWW, programming is in RWW
6. USB stays active, no interruption
7. Bootloader jumps to application at 0x2000

**If bootloader were in RWW:**
- Step 4 would halt the CPU
- USB would disconnect
- Flashing would fail
- Potential brick

## Practical Implementation Notes:

### 1. Bootloader Entry Point:
```c
// At 0x0000 (start of NRWW)
void __attribute__((section(".bootloader"))) bootloader_main(void) {
    // USB enumeration
    // Handle vendor requests
    // Write to RWW section
}
```

### 2. Application Entry Point:
```c
// At 0x2000 (start of RWW)
void __attribute__((section(".application"))) main(void) {
    // Your keyboard firmware
}
```

### 3. Jump from Bootloader to App:
```c
void jump_to_application(void) {
    // Disable interrupts
    cli();
    
    // Jump to application start (0x2000)
    void (*app_start)(void) = (void (*)(void))0x2000;
    app_start();
}
```

## Critical Warnings:

⚠️ **Never exceed 8 KB for bootloader** - it literally won't fit in NRWW
⚠️ **Set BOOTSIZE fuse correctly** - protects bootloader from being overwritten
⚠️ **Don't put application code in NRWW** - wastes the only section bootloader can use

## Final Size Allocation:

Your original diagram showed **8-12 KB** for bootloader. Now you know:

**8 KB is the MAXIMUM, not a guideline.**

Revised allocation:
```
Bootloader:  6 KB (actual code) + 2 KB (safety margin) = 8 KB NRWW
Application: 56 KB RWW
```

This makes **custom vendor protocol** even more attractive - you'll comfortably fit with room to spare, unlike DFU which might be tight, or UF2 which simply won't fit.

Does this change your approach? The NRWW constraint is actually a blessing in disguise - it forces a lean, efficient bootloader design!



## Prompt:
12/30/2025, 8:46:37 AM

Given that the MCU can only do USB FS, would the HID or DFU approaches be any faster at flashing the keyboard firmware (appcode)?



## Response:
12/30/2025, 8:47:27 AM

Excellent question! Let's do the math to see if there's any real-world difference.

## Theoretical Maximum Speeds (Full Speed USB)

### USB Full Speed Limits:
- **Max data rate**: 12 Mbps = 1.5 MB/s (theoretical)
- **Practical throughput**: ~1 MB/s (accounting for protocol overhead)
- **Control transfer max packet**: 64 bytes
- **Bulk transfer max packet**: 64 bytes (same as control for Full Speed!)

### Your Firmware Size:
- Application section: 56 KB (worst case, full flash)
- Typical keyboard firmware: 20-30 KB

## Speed Comparison:

### Custom Vendor (Control Transfers) - Your Approach

**Protocol:**
```
For each 64-byte chunk:
  SETUP packet (8 bytes)
  DATA OUT packet (64 bytes payload)
  ACK
  STATUS IN (zero-length)
```

**Overhead per transfer:**
- SETUP: 8 bytes
- Protocol overhead: ~10-20 bytes
- Total per 64-byte chunk: ~18-28 bytes overhead

**Effective throughput:**
- Payload: 64 bytes
- Total: ~84 bytes on wire
- Efficiency: 64/84 = **~76% efficient**

**Time to flash 30 KB:**
- 30 KB / 64 bytes = 469 transfers
- At ~1000 transfers/sec (Full Speed control): **~0.5 seconds**
- With flash write delays: **1-2 seconds total**

---

### DFU (Control Transfers)

**Protocol:**
- Uses **control transfers**, same as your custom vendor!
- DFU has more protocol overhead (state machine, status polling)

**DFU sequence:**
```
For each block:
  DFU_DNLOAD (control transfer with data)
  DFU_GETSTATUS (check if write is done)
  DFU_GETSTATUS (poll again if needed)
```

**Overhead:**
- Same 64-byte control transfer limit
- **Additional DFU_GETSTATUS requests** for polling
- State machine adds latency

**Effective throughput:**
- Payload: 64 bytes per DFU_DNLOAD
- Must poll status between writes
- Efficiency: **~60-70%** (worse due to polling)

**Time to flash 30 KB:**
- Same transfer count
- Additional status polling: +200-300ms
- With flash write delays: **1.5-2.5 seconds total**

**Verdict:** DFU is actually **SLOWER** than custom vendor!

---

### HID (Interrupt Transfers)

**Protocol:**
- Uses **interrupt IN/OUT endpoints**
- Interrupt transfers for Full Speed: Max 64 bytes, polled every 1ms minimum

**Key difference:**
- Interrupt endpoints are **polled** by host at fixed intervals
- Even if set to 1ms interval, this adds latency
- For data transfer (not real-time events), interrupts are suboptimal

**Overhead:**
- HID report descriptor overhead
- Each transfer requires HID report framing
- Must wait for polling interval

**Effective throughput:**
- Max: 64 bytes per 1ms = 64 KB/s theoretical
- With HID overhead: **~50 KB/s practical**

**Time to flash 30 KB:**
- 30 KB / 50 KB/s = **~0.6 seconds** transfer time
- With flash write delays: **1.5-2 seconds total**

**Verdict:** HID is **similar speed** to custom vendor, but more complex

---

### Bulk Transfers (Hypothetical)

If you added bulk endpoints to custom vendor:

**Protocol:**
- Bulk OUT for data streaming
- For Full Speed: Still limited to **64 bytes max packet**
- Can send back-to-back without SETUP overhead

**Effective throughput:**
- No SETUP packets needed
- Just data packets + ACK
- Efficiency: **~85-90%** (best case)

**Time to flash 30 KB:**
- Marginally faster: **0.4-0.5 seconds** transfer
- With flash write delays: **1-1.5 seconds total**

**Verdict:** Only **~0.5 second improvement** for added complexity

---

## The Bottleneck: Flash Write Time, Not USB!

### Flash Write Operation Times:
```
Page erase (512 bytes): 4-10 ms per page
Page write (512 bytes): 2-5 ms per page
Total per page: 6-15 ms
```

**For 30 KB firmware:**
- Pages to write: 30,000 / 512 = ~59 pages
- Flash operation time: 59 × 10ms = **~590ms**

**This is the dominant factor!**

### Total Time Breakdown:

| Component | Custom Vendor | DFU | HID | Bulk |
|-----------|---------------|-----|-----|------|
| USB transfer | 0.5s | 0.5s | 0.6s | 0.4s |
| Flash erase | 0.6s | 0.6s | 0.6s | 0.6s |
| Flash write | 0.3s | 0.3s | 0.3s | 0.3s |
| Protocol overhead | 0.1s | 0.4s | 0.2s | 0.1s |
| **TOTAL** | **1.5s** | **1.8s** | **1.7s** | **1.4s** |

## Conclusion:

### All methods are within 0.4 seconds of each other!

**Why?**
1. Full Speed USB is the same for everyone (12 Mbps)
2. Control vs Interrupt vs Bulk packets are all 64 bytes max
3. **Flash write time dominates** (~60% of total time)
4. USB transfer is only ~30-40% of total time

### The Real Speed Factors:

**What matters MORE than protocol choice:**
1. **Page erase-write combining** - Use `PAGEERASEWRITE` command to save ~0.3s
2. **Efficient flash operations** - Minimize page erases
3. **No verification passes** - Write once, trust it (or verify in background)
4. **Optimized protocol** - Minimize round-trips

### Optimizations That Actually Help:

#### 1. Use Combined Erase-Write (Saves ~0.3s):
```c
// Instead of:
erase_page(addr);  // 10ms
write_page(addr);  // 5ms
// Total: 15ms per page

// Do:
erase_write_page(addr);  // 8ms (atomic operation)
// Saves ~7ms per page × 59 pages = ~0.4 second savings!
```

#### 2. Pipeline USB and Flash (Saves ~0.2s):
```c
// While flash is writing, receive next chunk
while (writing_to_flash) {
    usb_receive_next_chunk();  // Parallel operation
}
```

#### 3. Skip Separate Erase Loop (Saves ~0.2s):
```c
// Don't do separate erase pass - use erase-write command
// Eliminates one full pass through memory
```

### With Optimizations:

| Approach | Basic | Optimized | Savings |
|----------|-------|-----------|---------|
| Custom Vendor | 1.5s | **0.8s** | 0.7s |
| DFU | 1.8s | **1.1s** | 0.7s |
| HID | 1.7s | **1.0s** | 0.7s |

**All approaches end up around 0.8-1.1 seconds when optimized!**

## Final Answer:

### No, HID and DFU are NOT faster than custom vendor.

In fact:
- **Custom vendor**: Fastest (fewest protocol overhead)
- **HID**: Same speed (within 0.1s)
- **DFU**: Slightly slower (extra status polling)

**The protocol choice makes < 0.4 second difference.**

**What DOES matter:**
1. Using combined erase-write commands
2. Efficient flash operations
3. Minimizing round-trips
4. Good firmware implementation

### Recommendation:

**Stick with custom vendor protocol** because:
- ✅ Smallest code size (fits comfortably in 8KB NRWW)
- ✅ Simplest implementation
- ✅ **Fastest or tied for fastest**
- ✅ Full control for optimizations
- ✅ Best learning experience

The speed difference between protocols is negligible, so optimize your **flash operations** instead of worrying about USB protocol choice!

**1-2 seconds to flash 30KB is totally acceptable** - users will barely notice the difference between 1.0s and 1.5s.



## Prompt:
12/30/2025, 9:05:42 AM

![1767099765256_image.png](data:image/webp;base64,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)

![1767099906060_image.png](data:image/webp;base64,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)

I don’t think the AVR64DU32 has the pageerasewrite command you mentioned, see screenshot from datasheet. It does have the ability to erase multiple pages in one command though, but I’m not sure if there is really any speed gain from using that. Perhaps there could be based on a comment I found in the datasheet (see second screenshot).



## Response:
12/30/2025, 9:07:10 AM

Excellent detective work! You're absolutely right - the AVR64DU32 does NOT have a combined `PAGEERASEWRITE` command. Let me revise the analysis based on the actual hardware capabilities.

## Available Commands on AVR64DU32:

From your screenshots:

**Single Page Operations:**
- `FLPER` (0x08) - Flash Page Erase Enable
- `FLWR` (0x02) - Flash Write Enable

**Multi-Page Erase Operations:**
- `FLMPER2` (0x09) - Flash 2-page Erase Enable
- `FLMPER4` (0x0A) - Flash 4-page Erase Enable
- `FLMPER8` (0x0B) - Flash 8-page Erase Enable
- `FLMPER16` (0x0C) - Flash 16-page Erase Enable
- `FLMPER32` (0x0D) - Flash 32-page Erase Enable

**Key insight from screenshot 2:**
> "Up to 32 pages can be erased at the same time. **The duration of the erase operation is independent of the number of pages being erased.**"

## This Changes Everything! 🎯

### Multi-Page Erase is a HUGE Optimization!

**Single page erase:**
```c
// Erase 32 pages (16KB) one at a time
for (int i = 0; i < 32; i++) {
    erase_page(addr);  // 10ms per page
    addr += 512;
}
// Total: 32 × 10ms = 320ms
```

**32-page erase:**
```c
// Erase 32 pages (16KB) at once
erase_32_pages(addr);  // Still ~10ms for ALL 32 pages!
// Total: 10ms
```

**Savings: 310ms per 16KB block!**

## Revised Speed Calculation:

### For 30 KB Firmware (59 pages):

**Old approach (single-page erase):**
```
59 pages × 10ms erase = 590ms erase
59 pages × 5ms write = 295ms write
USB transfer = 500ms
Total: ~1.4 seconds
```

**New approach (multi-page erase):**
```
Erase 32 pages: 10ms
Erase 27 pages: 10ms (do as 32-page, ignore extras)
Total erase: 20ms (!!)

59 pages × 5ms write = 295ms
USB transfer = 500ms
Total: ~0.8 seconds
```

**Speed improvement: 600ms faster! (1.4s → 0.8s)**

## Optimal Flashing Strategy:

### Strategy 1: Erase All, Then Write All (Simplest)

```c
void flash_firmware(uint8_t *firmware, size_t size) {
    uint32_t addr = APP_START;  // 0x2000
    size_t pages = (size + 511) / 512;
    
    // 1. Erase entire application section at once
    //    59 pages = 32 + 27, use two 32-page erases
    NVMCTRL.CTRLA = NVMCTRL_CMD_FLMPER32_gc;
    flash_page_erase_multi(APP_START, 32);  // Erase pages 0-31
    
    NVMCTRL.CTRLA = NVMCTRL_CMD_FLMPER32_gc;
    flash_page_erase_multi(APP_START + 32*512, 32);  // Erase pages 32-63
    // Total erase time: ~20ms for all 59 pages!
    
    // 2. Write pages sequentially
    while (size > 0) {
        size_t chunk = (size > 512) ? 512 : size;
        flash_page_write(addr, firmware, chunk);  // 5ms per page
        addr += 512;
        firmware += 512;
        size -= chunk;
    }
    // Write time: 59 × 5ms = 295ms
}
```

**Benefits:**
- Erase time drops from 590ms → 20ms
- Simple code structure
- Total: **~815ms** for 30KB

### Strategy 2: Erase-Then-Write in Blocks (More Complex)

```c
void flash_firmware_blocks(uint8_t *firmware, size_t size) {
    uint32_t addr = APP_START;
    
    // Process in 16KB blocks (32 pages)
    while (size > 0) {
        size_t block_size = (size > 16384) ? 16384 : size;
        size_t block_pages = (block_size + 511) / 512;
        
        // Erase up to 32 pages at once
        if (block_pages <= 32) {
            NVMCTRL.CTRLA = get_multi_erase_cmd(block_pages);
            flash_page_erase_multi(addr, block_pages);
        }
        
        // Write this block
        for (size_t i = 0; i < block_pages; i++) {
            flash_page_write(addr + i*512, firmware + i*512, 512);
        }
        
        addr += block_size;
        firmware += block_size;
        size -= block_size;
    }
}
```

**Benefits:**
- Can start writing sooner (pipeline better)
- Still fast erase
- More complex state machine

## Practical Implementation:

### Bootloader Command Structure:

```c
// Vendor commands
#define VENDOR_CMD_ERASE_MULTI  0x01  // Erase multiple pages
#define VENDOR_CMD_WRITE_PAGE   0x02  // Write single page
#define VENDOR_CMD_RESET        0x04  // Jump to app

// Multi-erase options (encode in wValue)
#define ERASE_PAGES_1   0x00
#define ERASE_PAGES_2   0x01
#define ERASE_PAGES_4   0x02
#define ERASE_PAGES_8   0x03
#define ERASE_PAGES_16  0x04
#define ERASE_PAGES_32  0x05
```

### Device-Side Handler:

```c
void handle_vendor_erase_multi(uint16_t page_count_code, uint32_t addr) {
    uint8_t nvmcmd;
    
    switch(page_count_code) {
        case ERASE_PAGES_1:  nvmcmd = NVMCTRL_CMD_FLPER_gc; break;
        case ERASE_PAGES_2:  nvmcmd = NVMCTRL_CMD_FLMPER2_gc; break;
        case ERASE_PAGES_4:  nvmcmd = NVMCTRL_CMD_FLMPER4_gc; break;
        case ERASE_PAGES_8:  nvmcmd = NVMCTRL_CMD_FLMPER8_gc; break;
        case ERASE_PAGES_16: nvmcmd = NVMCTRL_CMD_FLMPER16_gc; break;
        case ERASE_PAGES_32: nvmcmd = NVMCTRL_CMD_FLMPER32_gc; break;
        default: return;
    }
    
    // Wait for NVM ready
    while (NVMCTRL.STATUS & NVMCTRL_FBUSY_bm);
    
    // Set command
    NVMCTRL.CTRLA = nvmcmd;
    
    // Trigger erase by writing to address
    // (Details depend on exact AVR-DU procedure - check datasheet)
    _PROTECTED_WRITE_SPM(NVMCTRL.CTRLA, nvmcmd);
    *(volatile uint8_t *)addr = 0xFF;
    
    // Wait for completion
    while (NVMCTRL.STATUS & NVMCTRL_FBUSY_bm);
}
```

### Host-Side (Update flasher):

**KeyDU Custom Bootloader Flasher (libusb)** ∙ Version 3

````c
/*
 * KeyDU Custom Bootloader Flasher
 * 
 * Flashes firmware to AVR-DU keyboard using custom vendor protocol
 * Uses control transfers on EP0
 * 
 * Install libusb:
 *   Arch:    sudo pacman -S libusb
 *   Debian:  sudo apt-get install libusb-1.0-0-dev
 *   macOS:   brew install libusb
 * 
 * Compile:
 *   Linux:   gcc -o keydu-flash-custom keydu-flash.c -lusb-1.0
 *   macOS:   gcc -o keydu-flash-custom keydu-flash.c -lusb-1.0
 *   Windows: gcc -o keydu-flash-custom.exe keydu-flash.c -lusb-1.0
 * 
 * Usage:
 *   ./keydu-flash-custom firmware.bin
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

// Your VID/PID - change these to your actual values
#define VENDOR_ID   0x03EB  // Example: Atmel/Microchip VID
#define PRODUCT_ID  0x2FF4  // Example: Your bootloader PID

// Vendor request commands (must match device firmware)
#define VENDOR_REQ_ERASE_MULTI  0x01
#define VENDOR_REQ_WRITE        0x02
#define VENDOR_REQ_READ         0x03
#define VENDOR_REQ_RESET        0x04
#define VENDOR_REQ_STATUS       0x05

// Multi-page erase options (for wValue in erase command)
#define ERASE_PAGES_1   0x00
#define ERASE_PAGES_2   0x01
#define ERASE_PAGES_4   0x02
#define ERASE_PAGES_8   0x03
#define ERASE_PAGES_16  0x04
#define ERASE_PAGES_32  0x05

// USB control transfer parameters
#define USB_TIMEOUT         5000  // 5 second timeout
#define CHUNK_SIZE          64    // Full Speed max control data
#define FLASH_BASE_ADDR     0x0000
#define PAGE_SIZE           512   // AVR-DU flash page size (check datasheet)

// bmRequestType values
#define REQTYPE_HOST_TO_DEV 0x40  // Vendor, Host-to-Device
#define REQTYPE_DEV_TO_HOST 0xC0  // Vendor, Device-to-Host

// Status codes from device
#define STATUS_OK           0x00
#define STATUS_ERROR        0xFF

// Function prototypes
int find_device(libusb_device_handle **handle);
int erase_flash(libusb_device_handle *handle, uint32_t addr);
int write_chunk(libusb_device_handle *handle, uint32_t addr, 
                uint8_t *data, size_t len);
int read_status(libusb_device_handle *handle, uint8_t *status);
int reset_device(libusb_device_handle *handle);
uint8_t *read_firmware_file(const char *filename, size_t *size);

int main(int argc, char **argv) {
    libusb_device_handle *handle = NULL;
    uint8_t *firmware = NULL;
    size_t firmware_size = 0;
    int ret;
    
    printf("KeyDU Custom Bootloader Flasher\n");
    printf("================================\n\n");
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <firmware.bin>\n", argv[0]);
        return 1;
    }
    
    // Initialize libusb
    ret = libusb_init(NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to initialize libusb: %s\n", 
                libusb_error_name(ret));
        return 1;
    }
    
    // Find and open device
    printf("Searching for KeyDU bootloader...\n");
    if (find_device(&handle) != 0) {
        fprintf(stderr, "Device not found. Is it in bootloader mode?\n");
        libusb_exit(NULL);
        return 1;
    }
    printf("Device found!\n\n");
    
    // Read firmware file
    printf("Reading firmware file: %s\n", argv[1]);
    firmware = read_firmware_file(argv[1], &firmware_size);
    if (!firmware) {
        fprintf(stderr, "Failed to read firmware file\n");
        libusb_close(handle);
        libusb_exit(NULL);
        return 1;
    }
    printf("Firmware size: %zu bytes\n\n", firmware_size);
    
    // Erase flash (page by page or whole chip depending on implementation)
    printf("Erasing flash...\n");
    for (uint32_t addr = FLASH_BASE_ADDR; 
         addr < FLASH_BASE_ADDR + firmware_size; 
         addr += PAGE_SIZE) {
        
        if (erase_flash(handle, addr) != 0) {
            fprintf(stderr, "Erase failed at address 0x%08X\n", addr);
            free(firmware);
            libusb_close(handle);
            libusb_exit(NULL);
            return 1;
        }
        printf("  Erased page at 0x%08X\r", addr);
        fflush(stdout);
    }
    printf("\nErase complete!\n\n");
    
    // Write firmware in chunks
    printf("Writing firmware...\n");
    size_t bytes_written = 0;
    
    while (bytes_written < firmware_size) {
        size_t chunk_len = firmware_size - bytes_written;
        if (chunk_len > CHUNK_SIZE) {
            chunk_len = CHUNK_SIZE;
        }
        
        uint32_t addr = FLASH_BASE_ADDR + bytes_written;
        
        ret = write_chunk(handle, addr, 
                         firmware + bytes_written, chunk_len);
        if (ret != 0) {
            fprintf(stderr, "\nWrite failed at address 0x%08X\n", addr);
            free(firmware);
            libusb_close(handle);
            libusb_exit(NULL);
            return 1;
        }
        
        bytes_written += chunk_len;
        
        // Progress bar
        int progress = (bytes_written * 100) / firmware_size;
        printf("  Progress: %3d%% (%zu/%zu bytes)\r", 
               progress, bytes_written, firmware_size);
        fflush(stdout);
    }
    printf("\nWrite complete!\n\n");
    
    // Verify (optional - read back and compare)
    // TODO: Add verification if device supports read command
    
    // Reset device to application mode
    printf("Resetting device...\n");
    reset_device(handle);
    printf("Done! Device should now be running application.\n");
    
    // Cleanup
    free(firmware);
    libusb_close(handle);
    libusb_exit(NULL);
    
    return 0;
}

int find_device(libusb_device_handle **handle) {
    // Try to open device with VID/PID
    *handle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    
    if (*handle == NULL) {
        return -1;
    }
    
    // Claim interface 0
    int ret = libusb_claim_interface(*handle, 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to claim interface: %s\n", 
                libusb_error_name(ret));
        libusb_close(*handle);
        *handle = NULL;
        return -1;
    }
    
    return 0;
}

int erase_flash(libusb_device_handle *handle, uint32_t addr) {
    // Address split into wValue (high 16 bits) and wIndex (low 16 bits)
    uint16_t addr_hi = (addr >> 16) & 0xFFFF;
    uint16_t addr_lo = addr & 0xFFFF;
    
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_ERASE,          // bRequest
        addr_hi,                   // wValue
        addr_lo,                   // wIndex
        NULL,                      // data
        0,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    return 0;
}

int write_chunk(libusb_device_handle *handle, uint32_t addr, 
                uint8_t *data, size_t len) {
    
    if (len > CHUNK_SIZE) {
        fprintf(stderr, "Chunk too large: %zu bytes\n", len);
        return -1;
    }
    
    // Address split into wValue (high 16 bits) and wIndex (low 16 bits)
    uint16_t addr_hi = (addr >> 16) & 0xFFFF;
    uint16_t addr_lo = addr & 0xFFFF;
    
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_WRITE,          // bRequest
        addr_hi,                   // wValue
        addr_lo,                   // wIndex
        data,                      // data
        len,                       // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    if ((size_t)ret != len) {
        fprintf(stderr, "Partial write: expected %zu, got %d\n", len, ret);
        return -1;
    }
    
    return 0;
}

int read_status(libusb_device_handle *handle, uint8_t *status) {
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_DEV_TO_HOST,      // bmRequestType
        VENDOR_REQ_STATUS,         // bRequest
        0,                         // wValue
        0,                         // wIndex
        status,                    // data buffer
        1,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    return 0;
}

int reset_device(libusb_device_handle *handle) {
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_RESET,          // bRequest
        0,                         // wValue
        0,                         // wIndex
        NULL,                      // data
        0,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    // Device may reset before responding, so don't check return value
    (void)ret;
    
    return 0;
}

uint8_t *read_firmware_file(const char *filename, size_t *size) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (fsize < 0) {
        perror("ftell");
        fclose(f);
        return NULL;
    }
    
    // Allocate buffer
    uint8_t *buffer = malloc(fsize);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate %ld bytes\n", fsize);
        fclose(f);
        return NULL;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, fsize, f);
    if (bytes_read != (size_t)fsize) {
        fprintf(stderr, "Failed to read file: expected %ld, got %zu\n", 
                fsize, bytes_read);
        free(buffer);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    *size = fsize;
    return buffer;
}
````

**KeyDU Custom Bootloader Flasher (libusb)** ∙ Version 4

````c
/*
 * KeyDU Custom Bootloader Flasher
 * 
 * Flashes firmware to AVR-DU keyboard using custom vendor protocol
 * Uses control transfers on EP0
 * 
 * Install libusb:
 *   Arch:    sudo pacman -S libusb
 *   Debian:  sudo apt-get install libusb-1.0-0-dev
 *   macOS:   brew install libusb
 * 
 * Compile:
 *   Linux:   gcc -o keydu-flash-custom keydu-flash.c -lusb-1.0
 *   macOS:   gcc -o keydu-flash-custom keydu-flash.c -lusb-1.0
 *   Windows: gcc -o keydu-flash-custom.exe keydu-flash.c -lusb-1.0
 * 
 * Usage:
 *   ./keydu-flash-custom firmware.bin
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

// Your VID/PID - change these to your actual values
#define VENDOR_ID   0x03EB  // Example: Atmel/Microchip VID
#define PRODUCT_ID  0x2FF4  // Example: Your bootloader PID

// Vendor request commands (must match device firmware)
#define VENDOR_REQ_ERASE_MULTI  0x01
#define VENDOR_REQ_WRITE        0x02
#define VENDOR_REQ_READ         0x03
#define VENDOR_REQ_RESET        0x04
#define VENDOR_REQ_STATUS       0x05

// Multi-page erase options (for wValue in erase command)
#define ERASE_PAGES_1   0x00
#define ERASE_PAGES_2   0x01
#define ERASE_PAGES_4   0x02
#define ERASE_PAGES_8   0x03
#define ERASE_PAGES_16  0x04
#define ERASE_PAGES_32  0x05

// USB control transfer parameters
#define USB_TIMEOUT         5000  // 5 second timeout
#define CHUNK_SIZE          64    // Full Speed max control data
#define FLASH_BASE_ADDR     0x0000
#define PAGE_SIZE           512   // AVR-DU flash page size (check datasheet)

// bmRequestType values
#define REQTYPE_HOST_TO_DEV 0x40  // Vendor, Host-to-Device
#define REQTYPE_DEV_TO_HOST 0xC0  // Vendor, Device-to-Host

// Status codes from device
#define STATUS_OK           0x00
#define STATUS_ERROR        0xFF

// Function prototypes
int find_device(libusb_device_handle **handle);
int erase_flash_multi(libusb_device_handle *handle, uint32_t addr, 
                      uint8_t page_count_code);
int write_chunk(libusb_device_handle *handle, uint32_t addr, 
                uint8_t *data, size_t len);
int read_status(libusb_device_handle *handle, uint8_t *status);
int reset_device(libusb_device_handle *handle);
uint8_t *read_firmware_file(const char *filename, size_t *size);

int main(int argc, char **argv) {
    libusb_device_handle *handle = NULL;
    uint8_t *firmware = NULL;
    size_t firmware_size = 0;
    int ret;
    
    printf("KeyDU Custom Bootloader Flasher\n");
    printf("================================\n\n");
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <firmware.bin>\n", argv[0]);
        return 1;
    }
    
    // Initialize libusb
    ret = libusb_init(NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to initialize libusb: %s\n", 
                libusb_error_name(ret));
        return 1;
    }
    
    // Find and open device
    printf("Searching for KeyDU bootloader...\n");
    if (find_device(&handle) != 0) {
        fprintf(stderr, "Device not found. Is it in bootloader mode?\n");
        libusb_exit(NULL);
        return 1;
    }
    printf("Device found!\n\n");
    
    // Read firmware file
    printf("Reading firmware file: %s\n", argv[1]);
    firmware = read_firmware_file(argv[1], &firmware_size);
    if (!firmware) {
        fprintf(stderr, "Failed to read firmware file\n");
        libusb_close(handle);
        libusb_exit(NULL);
        return 1;
    }
    printf("Firmware size: %zu bytes\n\n", firmware_size);
    
    // Erase flash (page by page or whole chip depending on implementation)
    printf("Erasing flash...\n");
    for (uint32_t addr = FLASH_BASE_ADDR; 
         addr < FLASH_BASE_ADDR + firmware_size; 
         addr += PAGE_SIZE) {
        
        if (erase_flash(handle, addr) != 0) {
            fprintf(stderr, "Erase failed at address 0x%08X\n", addr);
            free(firmware);
            libusb_close(handle);
            libusb_exit(NULL);
            return 1;
        }
        printf("  Erased page at 0x%08X\r", addr);
        fflush(stdout);
    }
    printf("\nErase complete!\n\n");
    
    // Write firmware in chunks
    printf("Writing firmware...\n");
    size_t bytes_written = 0;
    
    while (bytes_written < firmware_size) {
        size_t chunk_len = firmware_size - bytes_written;
        if (chunk_len > CHUNK_SIZE) {
            chunk_len = CHUNK_SIZE;
        }
        
        uint32_t addr = FLASH_BASE_ADDR + bytes_written;
        
        ret = write_chunk(handle, addr, 
                         firmware + bytes_written, chunk_len);
        if (ret != 0) {
            fprintf(stderr, "\nWrite failed at address 0x%08X\n", addr);
            free(firmware);
            libusb_close(handle);
            libusb_exit(NULL);
            return 1;
        }
        
        bytes_written += chunk_len;
        
        // Progress bar
        int progress = (bytes_written * 100) / firmware_size;
        printf("  Progress: %3d%% (%zu/%zu bytes)\r", 
               progress, bytes_written, firmware_size);
        fflush(stdout);
    }
    printf("\nWrite complete!\n\n");
    
    // Verify (optional - read back and compare)
    // TODO: Add verification if device supports read command
    
    // Reset device to application mode
    printf("Resetting device...\n");
    reset_device(handle);
    printf("Done! Device should now be running application.\n");
    
    // Cleanup
    free(firmware);
    libusb_close(handle);
    libusb_exit(NULL);
    
    return 0;
}

int find_device(libusb_device_handle **handle) {
    // Try to open device with VID/PID
    *handle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    
    if (*handle == NULL) {
        return -1;
    }
    
    // Claim interface 0
    int ret = libusb_claim_interface(*handle, 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to claim interface: %s\n", 
                libusb_error_name(ret));
        libusb_close(*handle);
        *handle = NULL;
        return -1;
    }
    
    return 0;
}

int erase_flash(libusb_device_handle *handle, uint32_t addr) {
    // Address split into wValue (high 16 bits) and wIndex (low 16 bits)
    uint16_t addr_hi = (addr >> 16) & 0xFFFF;
    uint16_t addr_lo = addr & 0xFFFF;
    
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_ERASE,          // bRequest
        addr_hi,                   // wValue
        addr_lo,                   // wIndex
        NULL,                      // data
        0,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    return 0;
}

int write_chunk(libusb_device_handle *handle, uint32_t addr, 
                uint8_t *data, size_t len) {
    
    if (len > CHUNK_SIZE) {
        fprintf(stderr, "Chunk too large: %zu bytes\n", len);
        return -1;
    }
    
    // Address split into wValue (high 16 bits) and wIndex (low 16 bits)
    uint16_t addr_hi = (addr >> 16) & 0xFFFF;
    uint16_t addr_lo = addr & 0xFFFF;
    
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_WRITE,          // bRequest
        addr_hi,                   // wValue
        addr_lo,                   // wIndex
        data,                      // data
        len,                       // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    if ((size_t)ret != len) {
        fprintf(stderr, "Partial write: expected %zu, got %d\n", len, ret);
        return -1;
    }
    
    return 0;
}

int read_status(libusb_device_handle *handle, uint8_t *status) {
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_DEV_TO_HOST,      // bmRequestType
        VENDOR_REQ_STATUS,         // bRequest
        0,                         // wValue
        0,                         // wIndex
        status,                    // data buffer
        1,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    return 0;
}

int reset_device(libusb_device_handle *handle) {
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_RESET,          // bRequest
        0,                         // wValue
        0,                         // wIndex
        NULL,                      // data
        0,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    // Device may reset before responding, so don't check return value
    (void)ret;
    
    return 0;
}

uint8_t *read_firmware_file(const char *filename, size_t *size) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (fsize < 0) {
        perror("ftell");
        fclose(f);
        return NULL;
    }
    
    // Allocate buffer
    uint8_t *buffer = malloc(fsize);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate %ld bytes\n", fsize);
        fclose(f);
        return NULL;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, fsize, f);
    if (bytes_read != (size_t)fsize) {
        fprintf(stderr, "Failed to read file: expected %ld, got %zu\n", 
                fsize, bytes_read);
        free(buffer);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    *size = fsize;
    return buffer;
}
````

**KeyDU Custom Bootloader Flasher (libusb)** ∙ Version 5

````c
/*
 * KeyDU Custom Bootloader Flasher
 * 
 * Flashes firmware to AVR-DU keyboard using custom vendor protocol
 * Uses control transfers on EP0
 * 
 * Install libusb:
 *   Arch:    sudo pacman -S libusb
 *   Debian:  sudo apt-get install libusb-1.0-0-dev
 *   macOS:   brew install libusb
 * 
 * Compile:
 *   Linux:   gcc -o keydu-flash-custom keydu-flash.c -lusb-1.0
 *   macOS:   gcc -o keydu-flash-custom keydu-flash.c -lusb-1.0
 *   Windows: gcc -o keydu-flash-custom.exe keydu-flash.c -lusb-1.0
 * 
 * Usage:
 *   ./keydu-flash-custom firmware.bin
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

// Your VID/PID - change these to your actual values
#define VENDOR_ID   0x03EB  // Example: Atmel/Microchip VID
#define PRODUCT_ID  0x2FF4  // Example: Your bootloader PID

// Vendor request commands (must match device firmware)
#define VENDOR_REQ_ERASE_MULTI  0x01
#define VENDOR_REQ_WRITE        0x02
#define VENDOR_REQ_READ         0x03
#define VENDOR_REQ_RESET        0x04
#define VENDOR_REQ_STATUS       0x05

// Multi-page erase options (for wValue in erase command)
#define ERASE_PAGES_1   0x00
#define ERASE_PAGES_2   0x01
#define ERASE_PAGES_4   0x02
#define ERASE_PAGES_8   0x03
#define ERASE_PAGES_16  0x04
#define ERASE_PAGES_32  0x05

// USB control transfer parameters
#define USB_TIMEOUT         5000  // 5 second timeout
#define CHUNK_SIZE          64    // Full Speed max control data
#define FLASH_BASE_ADDR     0x0000
#define PAGE_SIZE           512   // AVR-DU flash page size (check datasheet)

// bmRequestType values
#define REQTYPE_HOST_TO_DEV 0x40  // Vendor, Host-to-Device
#define REQTYPE_DEV_TO_HOST 0xC0  // Vendor, Device-to-Host

// Status codes from device
#define STATUS_OK           0x00
#define STATUS_ERROR        0xFF

// Function prototypes
int find_device(libusb_device_handle **handle);
int erase_flash_multi(libusb_device_handle *handle, uint32_t addr, 
                      uint8_t page_count_code);
int write_chunk(libusb_device_handle *handle, uint32_t addr, 
                uint8_t *data, size_t len);
int read_status(libusb_device_handle *handle, uint8_t *status);
int reset_device(libusb_device_handle *handle);
uint8_t *read_firmware_file(const char *filename, size_t *size);

int main(int argc, char **argv) {
    libusb_device_handle *handle = NULL;
    uint8_t *firmware = NULL;
    size_t firmware_size = 0;
    int ret;
    
    printf("KeyDU Custom Bootloader Flasher\n");
    printf("================================\n\n");
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <firmware.bin>\n", argv[0]);
        return 1;
    }
    
    // Initialize libusb
    ret = libusb_init(NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to initialize libusb: %s\n", 
                libusb_error_name(ret));
        return 1;
    }
    
    // Find and open device
    printf("Searching for KeyDU bootloader...\n");
    if (find_device(&handle) != 0) {
        fprintf(stderr, "Device not found. Is it in bootloader mode?\n");
        libusb_exit(NULL);
        return 1;
    }
    printf("Device found!\n\n");
    
    // Read firmware file
    printf("Reading firmware file: %s\n", argv[1]);
    firmware = read_firmware_file(argv[1], &firmware_size);
    if (!firmware) {
        fprintf(stderr, "Failed to read firmware file\n");
        libusb_close(handle);
        libusb_exit(NULL);
        return 1;
    }
    printf("Firmware size: %zu bytes\n\n", firmware_size);
    
    // Erase flash using multi-page erase for speed
    // AVR64DU32 can erase up to 32 pages in ~10ms (same time as 1 page!)
    printf("Erasing flash...\n");
    
    size_t pages_to_erase = (firmware_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t addr = FLASH_BASE_ADDR;
    
    // Erase in 32-page chunks for maximum speed
    while (pages_to_erase > 0) {
        uint8_t erase_code;
        size_t pages_this_round;
        
        if (pages_to_erase >= 32) {
            erase_code = ERASE_PAGES_32;
            pages_this_round = 32;
        } else if (pages_to_erase >= 16) {
            erase_code = ERASE_PAGES_16;
            pages_this_round = 16;
        } else if (pages_to_erase >= 8) {
            erase_code = ERASE_PAGES_8;
            pages_this_round = 8;
        } else if (pages_to_erase >= 4) {
            erase_code = ERASE_PAGES_4;
            pages_this_round = 4;
        } else if (pages_to_erase >= 2) {
            erase_code = ERASE_PAGES_2;
            pages_this_round = 2;
        } else {
            erase_code = ERASE_PAGES_1;
            pages_this_round = 1;
        }
        
        if (erase_flash_multi(handle, addr, erase_code) != 0) {
            fprintf(stderr, "Erase failed at address 0x%08X\n", addr);
            free(firmware);
            libusb_close(handle);
            libusb_exit(NULL);
            return 1;
        }
        
        printf("  Erased %zu pages at 0x%08X\r", pages_this_round, addr);
        fflush(stdout);
        
        addr += pages_this_round * PAGE_SIZE;
        pages_to_erase -= pages_this_round;
    }
    printf("\nErase complete! (Much faster with multi-page erase)\n\n");
    
    // Write firmware in chunks
    printf("Writing firmware...\n");
    size_t bytes_written = 0;
    
    while (bytes_written < firmware_size) {
        size_t chunk_len = firmware_size - bytes_written;
        if (chunk_len > CHUNK_SIZE) {
            chunk_len = CHUNK_SIZE;
        }
        
        uint32_t addr = FLASH_BASE_ADDR + bytes_written;
        
        ret = write_chunk(handle, addr, 
                         firmware + bytes_written, chunk_len);
        if (ret != 0) {
            fprintf(stderr, "\nWrite failed at address 0x%08X\n", addr);
            free(firmware);
            libusb_close(handle);
            libusb_exit(NULL);
            return 1;
        }
        
        bytes_written += chunk_len;
        
        // Progress bar
        int progress = (bytes_written * 100) / firmware_size;
        printf("  Progress: %3d%% (%zu/%zu bytes)\r", 
               progress, bytes_written, firmware_size);
        fflush(stdout);
    }
    printf("\nWrite complete!\n\n");
    
    // Verify (optional - read back and compare)
    // TODO: Add verification if device supports read command
    
    // Reset device to application mode
    printf("Resetting device...\n");
    reset_device(handle);
    printf("Done! Device should now be running application.\n");
    
    // Cleanup
    free(firmware);
    libusb_close(handle);
    libusb_exit(NULL);
    
    return 0;
}

int find_device(libusb_device_handle **handle) {
    // Try to open device with VID/PID
    *handle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    
    if (*handle == NULL) {
        return -1;
    }
    
    // Claim interface 0
    int ret = libusb_claim_interface(*handle, 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to claim interface: %s\n", 
                libusb_error_name(ret));
        libusb_close(*handle);
        *handle = NULL;
        return -1;
    }
    
    return 0;
}

int erase_flash(libusb_device_handle *handle, uint32_t addr) {
    // Address split into wValue (high 16 bits) and wIndex (low 16 bits)
    uint16_t addr_hi = (addr >> 16) & 0xFFFF;
    uint16_t addr_lo = addr & 0xFFFF;
    
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_ERASE,          // bRequest
        addr_hi,                   // wValue
        addr_lo,                   // wIndex
        NULL,                      // data
        0,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    return 0;
}

int write_chunk(libusb_device_handle *handle, uint32_t addr, 
                uint8_t *data, size_t len) {
    
    if (len > CHUNK_SIZE) {
        fprintf(stderr, "Chunk too large: %zu bytes\n", len);
        return -1;
    }
    
    // Address split into wValue (high 16 bits) and wIndex (low 16 bits)
    uint16_t addr_hi = (addr >> 16) & 0xFFFF;
    uint16_t addr_lo = addr & 0xFFFF;
    
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_WRITE,          // bRequest
        addr_hi,                   // wValue
        addr_lo,                   // wIndex
        data,                      // data
        len,                       // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    if ((size_t)ret != len) {
        fprintf(stderr, "Partial write: expected %zu, got %d\n", len, ret);
        return -1;
    }
    
    return 0;
}

int read_status(libusb_device_handle *handle, uint8_t *status) {
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_DEV_TO_HOST,      // bmRequestType
        VENDOR_REQ_STATUS,         // bRequest
        0,                         // wValue
        0,                         // wIndex
        status,                    // data buffer
        1,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    return 0;
}

int reset_device(libusb_device_handle *handle) {
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_RESET,          // bRequest
        0,                         // wValue
        0,                         // wIndex
        NULL,                      // data
        0,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    // Device may reset before responding, so don't check return value
    (void)ret;
    
    return 0;
}

uint8_t *read_firmware_file(const char *filename, size_t *size) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (fsize < 0) {
        perror("ftell");
        fclose(f);
        return NULL;
    }
    
    // Allocate buffer
    uint8_t *buffer = malloc(fsize);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate %ld bytes\n", fsize);
        fclose(f);
        return NULL;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, fsize, f);
    if (bytes_read != (size_t)fsize) {
        fprintf(stderr, "Failed to read file: expected %ld, got %zu\n", 
                fsize, bytes_read);
        free(buffer);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    *size = fsize;
    return buffer;
}
````

**KeyDU Custom Bootloader Flasher (libusb)** ∙ Version 6

````c
/*
 * KeyDU Custom Bootloader Flasher
 * 
 * Flashes firmware to AVR-DU keyboard using custom vendor protocol
 * Uses control transfers on EP0
 * 
 * Install libusb:
 *   Arch:    sudo pacman -S libusb
 *   Debian:  sudo apt-get install libusb-1.0-0-dev
 *   macOS:   brew install libusb
 * 
 * Compile:
 *   Linux:   gcc -o keydu-flash-custom keydu-flash.c -lusb-1.0
 *   macOS:   gcc -o keydu-flash-custom keydu-flash.c -lusb-1.0
 *   Windows: gcc -o keydu-flash-custom.exe keydu-flash.c -lusb-1.0
 * 
 * Usage:
 *   ./keydu-flash-custom firmware.bin
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <libusb-1.0/libusb.h>

// Your VID/PID - change these to your actual values
#define VENDOR_ID   0x03EB  // Example: Atmel/Microchip VID
#define PRODUCT_ID  0x2FF4  // Example: Your bootloader PID

// Vendor request commands (must match device firmware)
#define VENDOR_REQ_ERASE_MULTI  0x01
#define VENDOR_REQ_WRITE        0x02
#define VENDOR_REQ_READ         0x03
#define VENDOR_REQ_RESET        0x04
#define VENDOR_REQ_STATUS       0x05

// Multi-page erase options (for wValue in erase command)
#define ERASE_PAGES_1   0x00
#define ERASE_PAGES_2   0x01
#define ERASE_PAGES_4   0x02
#define ERASE_PAGES_8   0x03
#define ERASE_PAGES_16  0x04
#define ERASE_PAGES_32  0x05

// USB control transfer parameters
#define USB_TIMEOUT         5000  // 5 second timeout
#define CHUNK_SIZE          64    // Full Speed max control data
#define FLASH_BASE_ADDR     0x0000
#define PAGE_SIZE           512   // AVR-DU flash page size (check datasheet)

// bmRequestType values
#define REQTYPE_HOST_TO_DEV 0x40  // Vendor, Host-to-Device
#define REQTYPE_DEV_TO_HOST 0xC0  // Vendor, Device-to-Host

// Status codes from device
#define STATUS_OK           0x00
#define STATUS_ERROR        0xFF

// Function prototypes
int find_device(libusb_device_handle **handle);
int erase_flash_multi(libusb_device_handle *handle, uint32_t addr, 
                      uint8_t page_count_code);
int write_chunk(libusb_device_handle *handle, uint32_t addr, 
                uint8_t *data, size_t len);
int read_status(libusb_device_handle *handle, uint8_t *status);
int reset_device(libusb_device_handle *handle);
uint8_t *read_firmware_file(const char *filename, size_t *size);

int main(int argc, char **argv) {
    libusb_device_handle *handle = NULL;
    uint8_t *firmware = NULL;
    size_t firmware_size = 0;
    int ret;
    
    printf("KeyDU Custom Bootloader Flasher\n");
    printf("================================\n\n");
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <firmware.bin>\n", argv[0]);
        return 1;
    }
    
    // Initialize libusb
    ret = libusb_init(NULL);
    if (ret < 0) {
        fprintf(stderr, "Failed to initialize libusb: %s\n", 
                libusb_error_name(ret));
        return 1;
    }
    
    // Find and open device
    printf("Searching for KeyDU bootloader...\n");
    if (find_device(&handle) != 0) {
        fprintf(stderr, "Device not found. Is it in bootloader mode?\n");
        libusb_exit(NULL);
        return 1;
    }
    printf("Device found!\n\n");
    
    // Read firmware file
    printf("Reading firmware file: %s\n", argv[1]);
    firmware = read_firmware_file(argv[1], &firmware_size);
    if (!firmware) {
        fprintf(stderr, "Failed to read firmware file\n");
        libusb_close(handle);
        libusb_exit(NULL);
        return 1;
    }
    printf("Firmware size: %zu bytes\n\n", firmware_size);
    
    // Erase flash using multi-page erase for speed
    // AVR64DU32 can erase up to 32 pages in ~10ms (same time as 1 page!)
    printf("Erasing flash...\n");
    
    size_t pages_to_erase = (firmware_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t addr = FLASH_BASE_ADDR;
    
    // Erase in 32-page chunks for maximum speed
    while (pages_to_erase > 0) {
        uint8_t erase_code;
        size_t pages_this_round;
        
        if (pages_to_erase >= 32) {
            erase_code = ERASE_PAGES_32;
            pages_this_round = 32;
        } else if (pages_to_erase >= 16) {
            erase_code = ERASE_PAGES_16;
            pages_this_round = 16;
        } else if (pages_to_erase >= 8) {
            erase_code = ERASE_PAGES_8;
            pages_this_round = 8;
        } else if (pages_to_erase >= 4) {
            erase_code = ERASE_PAGES_4;
            pages_this_round = 4;
        } else if (pages_to_erase >= 2) {
            erase_code = ERASE_PAGES_2;
            pages_this_round = 2;
        } else {
            erase_code = ERASE_PAGES_1;
            pages_this_round = 1;
        }
        
        if (erase_flash_multi(handle, addr, erase_code) != 0) {
            fprintf(stderr, "Erase failed at address 0x%08X\n", addr);
            free(firmware);
            libusb_close(handle);
            libusb_exit(NULL);
            return 1;
        }
        
        printf("  Erased %zu pages at 0x%08X\r", pages_this_round, addr);
        fflush(stdout);
        
        addr += pages_this_round * PAGE_SIZE;
        pages_to_erase -= pages_this_round;
    }
    printf("\nErase complete! (Much faster with multi-page erase)\n\n");
    
    // Write firmware in chunks
    printf("Writing firmware...\n");
    size_t bytes_written = 0;
    
    while (bytes_written < firmware_size) {
        size_t chunk_len = firmware_size - bytes_written;
        if (chunk_len > CHUNK_SIZE) {
            chunk_len = CHUNK_SIZE;
        }
        
        uint32_t addr = FLASH_BASE_ADDR + bytes_written;
        
        ret = write_chunk(handle, addr, 
                         firmware + bytes_written, chunk_len);
        if (ret != 0) {
            fprintf(stderr, "\nWrite failed at address 0x%08X\n", addr);
            free(firmware);
            libusb_close(handle);
            libusb_exit(NULL);
            return 1;
        }
        
        bytes_written += chunk_len;
        
        // Progress bar
        int progress = (bytes_written * 100) / firmware_size;
        printf("  Progress: %3d%% (%zu/%zu bytes)\r", 
               progress, bytes_written, firmware_size);
        fflush(stdout);
    }
    printf("\nWrite complete!\n\n");
    
    // Verify (optional - read back and compare)
    // TODO: Add verification if device supports read command
    
    // Reset device to application mode
    printf("Resetting device...\n");
    reset_device(handle);
    printf("Done! Device should now be running application.\n");
    
    // Cleanup
    free(firmware);
    libusb_close(handle);
    libusb_exit(NULL);
    
    return 0;
}

int find_device(libusb_device_handle **handle) {
    // Try to open device with VID/PID
    *handle = libusb_open_device_with_vid_pid(NULL, VENDOR_ID, PRODUCT_ID);
    
    if (*handle == NULL) {
        return -1;
    }
    
    // Claim interface 0
    int ret = libusb_claim_interface(*handle, 0);
    if (ret < 0) {
        fprintf(stderr, "Failed to claim interface: %s\n", 
                libusb_error_name(ret));
        libusb_close(*handle);
        *handle = NULL;
        return -1;
    }
    
    return 0;
}

int erase_flash_multi(libusb_device_handle *handle, uint32_t addr, 
                      uint8_t page_count_code) {
    // Address in wIndex (only lower 16 bits, upper bits not needed for 64KB chip)
    uint16_t addr_lo = addr & 0xFFFF;
    
    // Page count code in wValue
    uint16_t erase_code = page_count_code;
    
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_ERASE_MULTI,   // bRequest
        erase_code,                // wValue (number of pages to erase)
        addr_lo,                   // wIndex (address)
        NULL,                      // data
        0,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    return 0;
}

int write_chunk(libusb_device_handle *handle, uint32_t addr, 
                uint8_t *data, size_t len) {
    
    if (len > CHUNK_SIZE) {
        fprintf(stderr, "Chunk too large: %zu bytes\n", len);
        return -1;
    }
    
    // Address split into wValue (high 16 bits) and wIndex (low 16 bits)
    uint16_t addr_hi = (addr >> 16) & 0xFFFF;
    uint16_t addr_lo = addr & 0xFFFF;
    
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_WRITE,          // bRequest
        addr_hi,                   // wValue
        addr_lo,                   // wIndex
        data,                      // data
        len,                       // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    if ((size_t)ret != len) {
        fprintf(stderr, "Partial write: expected %zu, got %d\n", len, ret);
        return -1;
    }
    
    return 0;
}

int read_status(libusb_device_handle *handle, uint8_t *status) {
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_DEV_TO_HOST,      // bmRequestType
        VENDOR_REQ_STATUS,         // bRequest
        0,                         // wValue
        0,                         // wIndex
        status,                    // data buffer
        1,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    if (ret < 0) {
        fprintf(stderr, "Control transfer failed: %s\n", 
                libusb_error_name(ret));
        return -1;
    }
    
    return 0;
}

int reset_device(libusb_device_handle *handle) {
    int ret = libusb_control_transfer(
        handle,
        REQTYPE_HOST_TO_DEV,      // bmRequestType
        VENDOR_REQ_RESET,          // bRequest
        0,                         // wValue
        0,                         // wIndex
        NULL,                      // data
        0,                         // wLength
        USB_TIMEOUT                // timeout
    );
    
    // Device may reset before responding, so don't check return value
    (void)ret;
    
    return 0;
}

uint8_t *read_firmware_file(const char *filename, size_t *size) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        perror("fopen");
        return NULL;
    }
    
    // Get file size
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (fsize < 0) {
        perror("ftell");
        fclose(f);
        return NULL;
    }
    
    // Allocate buffer
    uint8_t *buffer = malloc(fsize);
    if (!buffer) {
        fprintf(stderr, "Failed to allocate %ld bytes\n", fsize);
        fclose(f);
        return NULL;
    }
    
    // Read file
    size_t bytes_read = fread(buffer, 1, fsize, f);
    if (bytes_read != (size_t)fsize) {
        fprintf(stderr, "Failed to read file: expected %ld, got %zu\n", 
                fsize, bytes_read);
        free(buffer);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    *size = fsize;
    return buffer;
}
````

Perfect! I've updated the flasher to use multi-page erase. Now let's see the final performance comparison:

## Final Speed Analysis with Multi-Page Erase:

### Custom Vendor Protocol (Optimized):

**For 30 KB firmware:**
```
Multi-page erase:
  - 32 pages: 10ms
  - 27 pages (use 32): 10ms
  Total erase: 20ms (was 590ms!)

Page writes:
  - 59 pages × 5ms = 295ms

USB transfer:
  - 59 chunks × 1ms = 59ms (actual data)
  - Protocol overhead: ~100ms
  Total USB: 159ms

Grand total: 20 + 295 + 159 = ~474ms
Round up for safety: ~500ms (0.5 seconds!)
```

### Comparison with Other Protocols (All Using Multi-Page Erase):

| Protocol | Erase | Write | USB | Protocol | **Total** |
|----------|-------|-------|-----|----------|-----------|
| **Custom Vendor** | 20ms | 295ms | 159ms | 50ms | **~524ms** |
| **HID** | 20ms | 295ms | 180ms | 80ms | **~575ms** |
| **DFU** | 20ms | 295ms | 159ms | 150ms | **~624ms** |

**All protocols are now under 650ms!**

## Key Insights:

### 1. Multi-Page Erase is the Game Changer
- Old: 590ms just for erase
- New: 20ms for all erases
- **Savings: 570ms!**

### 2. Flash Write Time is Now the Bottleneck
- Can't avoid: Each page needs ~5ms to program
- 59 pages × 5ms = 295ms
- This is **~60% of total time** now

### 3. Protocol Choice Still Doesn't Matter Much
- Difference between fastest and slowest: 100ms
- All complete in under 650ms

### 4. User Experience
- **0.5 seconds** to flash firmware is essentially instant
- User won't even notice it happening
- Much better than the 1.5-2 seconds we calculated before

## Recommendation Updated:

**Multi-page erase makes your custom vendor protocol even better:**

✅ **Smallest code** (2-4 KB vs 4-6 KB for DFU)  
✅ **Fastest or tied** (524ms vs 575-624ms)  
✅ **Simplest implementation**  
✅ **Best learning value**  
✅ **Most efficient use of NRWW space**

The multi-page erase capability means you get **blazing fast firmware updates** regardless of protocol choice, but custom vendor still wins on all other metrics!

## Implementation Priority:

1. ✅ Get basic single-page erase/write working first
2. ✅ Test with small firmware
3. ✅ Add multi-page erase optimization (easy win!)
4. ✅ Enjoy sub-second firmware updates

Your discovery about multi-page erase just made your bootloader project even better! 🚀



## Prompt:
12/30/2025, 10:43:43 PM

does it make sense to assign a different pid for the bootloader vs the keyboard app?



## Response:
12/30/2025, 10:44:29 PM

**Yes, absolutely!** Using different PIDs for bootloader vs application is a **best practice** and highly recommended.

## Why Different PIDs Make Sense:

### 1. **Host OS Can Distinguish Them**

**Bootloader Mode:**
```c
VID: 0x03EB (Microchip/Atmel)
PID: 0x2FF4 (Your custom bootloader PID)
Device String: "KeyDU Bootloader"
```

**Application Mode:**
```c
VID: 0x03EB (same)
PID: 0x2FF5 (Your keyboard PID)
Device String: "KeyDU Keyboard"
```

**Benefits:**
- Host knows which mode the device is in
- Can load appropriate driver/software automatically
- Flasher tool can specifically wait for bootloader PID
- OS won't try to use bootloader as a keyboard

### 2. **Automated Flashing Workflows**

```bash
# Flasher can wait for bootloader specifically
./keydu-flash-custom firmware.bin
# Script watches for PID 0x2FF4 to appear
# Flashes
# Device resets to PID 0x2FF5 (keyboard)
# Done!
```

Without different PIDs:
- Flasher can't tell if device is in bootloader or app mode
- Might try to flash a keyboard that's running normally
- User confusion

### 3. **OS Driver Matching**

**Windows/Linux/macOS:**
- Bootloader (VID:PID 0x03EB:0x2FF4) → No driver needed, custom flasher only
- Keyboard (VID:PID 0x03EB:0x2FF5) → HID driver, works as keyboard

If same PID:
- OS gets confused when device "changes personality"
- May need to unplug/replug to recognize properly

### 4. **QMK/VIA/VIAL Compatibility**

Many configurator tools search for specific VID:PID pairs:
```json
{
  "vendorId": "0x03EB",
  "productId": "0x2FF5"  // Only matches keyboard, not bootloader
}
```

### 5. **Device Manager / System Information**

**User perspective:**
```
Bootloader mode:
  "KeyDU Bootloader (COM3)" or "KeyDU Bootloader"
  
Keyboard mode:
  "KeyDU Keyboard" shows up as HID device
```

Clear distinction helps debugging.

## Real-World Examples:

### Arduino Pro Micro (Caterina):
- **Bootloader**: VID:PID = `2341:0036` or `1B4F:9206`
- **Sketch (App)**: VID:PID = `2341:8036` or `1B4F:9206` (actually, they often differ)

### Teensy:
- **Bootloader (HalfKay)**: VID:PID = `16C0:0478`
- **Application**: VID:PID = `16C0:0482` (or custom)

### STM32 DFU:
- **DFU Bootloader**: VID:PID = `0483:DF11`
- **Application**: Custom VID:PID

### ATmega32U4 Keyboards:
- **Bootloader**: `03EB:2FF4` (Atmel DFU)
- **Keyboard App**: Varies by manufacturer

## Implementation:

### Device Descriptors:

**Bootloader (bootloader/usb_descriptors.c):**
```c
const USB_Descriptor_Device_t DeviceDescriptor = {
    .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},
    .USBSpecification = VERSION_BCD(2,0,0),
    .Class = USB_CSCP_VendorSpecificClass,  // Vendor class
    .SubClass = USB_CSCP_NoDeviceSubclass,
    .Protocol = USB_CSCP_NoDeviceProtocol,
    .Endpoint0Size = 64,
    
    .VendorID = 0x03EB,    // Atmel/Microchip
    .ProductID = 0x2FF4,   // Bootloader PID
    .ReleaseNumber = VERSION_BCD(1,0,0),
    
    .ManufacturerStrIndex = STRING_ID_Manufacturer,
    .ProductStrIndex = STRING_ID_Product,
    .SerialNumStrIndex = NO_DESCRIPTOR,
    .NumberOfConfigurations = 1
};

const char ProductString[] = "KeyDU Bootloader";
```

**Application (application/usb_descriptors.c):**
```c
const USB_Descriptor_Device_t DeviceDescriptor = {
    .Header = {.Size = sizeof(USB_Descriptor_Device_t), .Type = DTYPE_Device},
    .USBSpecification = VERSION_BCD(2,0,0),
    .Class = USB_CSCP_NoDeviceClass,  // HID is in interface
    .SubClass = USB_CSCP_NoDeviceSubclass,
    .Protocol = USB_CSCP_NoDeviceProtocol,
    .Endpoint0Size = 64,
    
    .VendorID = 0x03EB,    // Same VID
    .ProductID = 0x2FF5,   // Different PID!
    .ReleaseNumber = VERSION_BCD(1,0,0),
    
    .ManufacturerStrIndex = STRING_ID_Manufacturer,
    .ProductStrIndex = STRING_ID_Product,
    .SerialNumStrIndex = STRING_ID_Serial,
    .NumberOfConfigurations = 1
};

const char ProductString[] = "KeyDU Keyboard";
```

### Updated Flasher Tool:

```c
// Bootloader VID/PID
#define BOOTLOADER_VID  0x03EB
#define BOOTLOADER_PID  0x2FF4

// Application VID/PID (for reference)
#define APP_VID         0x03EB
#define APP_PID         0x2FF5

int find_device(libusb_device_handle **handle) {
    // Only look for bootloader PID
    *handle = libusb_open_device_with_vid_pid(NULL, 
                                               BOOTLOADER_VID, 
                                               BOOTLOADER_PID);
    
    if (*handle == NULL) {
        return -1;
    }
    
    // ... rest of code
}
```

### Smart Flasher (Waits for Bootloader):

```c
int wait_for_bootloader(libusb_device_handle **handle, int timeout_sec) {
    printf("Waiting for bootloader...\n");
    printf("(Press RESET or QK_BOOT on your keyboard)\n\n");
    
    time_t start = time(NULL);
    
    while (time(NULL) - start < timeout_sec) {
        *handle = libusb_open_device_with_vid_pid(NULL, 
                                                   BOOTLOADER_VID, 
                                                   BOOTLOADER_PID);
        if (*handle != NULL) {
            printf("Bootloader found!\n");
            return 0;
        }
        
        usleep(500000);  // Check every 500ms
    }
    
    fprintf(stderr, "Timeout waiting for bootloader\n");
    return -1;
}
```

## VID:PID Selection Guidelines:

### Option 1: Use Official VIDs (If You Have Permission)

**Microchip/Atmel VID (0x03EB):**
- Technically requires license, but widely used in hobby projects
- PIDs in `0x2Fxx` range are often used for custom bootloaders

**pid.codes (Community VID: 0x1209):**
- Free, open allocation for hobby/open-source projects
- Get your own PID at https://pid.codes
- Recommended for serious projects

### Option 2: Use Test VID (Development Only)

**Test VID: 0x1234 or similar**
- Never use in production
- Fine for development/learning

### Recommended for KeyDU:

**Development:**
```c
#define VID_KEYDU           0x1234  // Test VID
#define PID_KEYDU_BOOT      0x0001
#define PID_KEYDU_KEYBOARD  0x0002
```

**Production (if you publish):**
```c
// Get free PID from pid.codes
#define VID_KEYDU           0x1209  // pid.codes VID
#define PID_KEYDU_BOOT      0xYOUR_ASSIGNED_PID
#define PID_KEYDU_KEYBOARD  0xYOUR_ASSIGNED_PID + 1
```

## Benefits Summary:

| Aspect | Same PID | Different PIDs |
|--------|----------|----------------|
| **OS Recognition** | Confusing | Clear |
| **Auto-flashing** | Hard | Easy |
| **Driver Loading** | May conflict | Correct driver |
| **Debugging** | Confusing | Clear which mode |
| **Tool Compatibility** | May conflict | Works correctly |
| **Best Practice?** | ❌ No | ✅ Yes |

## Counter-Argument (Why NOT to use different PIDs):

**Only one reason:**
- Slight increase in descriptor complexity (define two PIDs instead of one)

**But this is trivial compared to the benefits!**

## Conclusion:

**Yes, use different PIDs:**
- Bootloader: `0x03EB:0x2FF4` (or your chosen PID)
- Keyboard: `0x03EB:0x2FF5` (or your chosen PID + 1)

This is **standard practice** and will make your life much easier for:
- Development
- Debugging  
- User experience
- Tool compatibility

The only time you'd use the same PID is if you're implementing a protocol like DFU that's supposed to enumerate as the same device (but even then, DFU often uses a specific well-known PID).

**For your custom bootloader: Definitely use different PIDs!**



## Prompt:
12/31/2025, 2:04:42 PM

what’s the difference between a usb 2.0 full speed compliant device and a usb 1.1 full speed compliant device?



## Response:
12/31/2025, 2:05:26 PM

Great question! This is a common source of confusion because **USB 1.1 and USB 2.0 both define "Full Speed" as 12 Mbps**, but there ARE some differences.

## Speed Definitions Recap:

### USB 1.1 Speeds:
- **Low Speed**: 1.5 Mbps
- **Full Speed**: 12 Mbps

### USB 2.0 Speeds:
- **Low Speed**: 1.5 Mbps (unchanged)
- **Full Speed**: 12 Mbps (unchanged)
- **High Speed**: 480 Mbps (NEW!)

## Key Differences Between USB 1.1 FS and USB 2.0 FS:

### 1. **Electrical Signaling (Minor Changes)**

**USB 2.0 tightened some electrical specs:**
- Slightly stricter rise/fall time requirements
- Better defined eye diagrams
- Improved signal quality requirements
- More detailed EMI/EMC specifications

**In practice:** Most USB 1.1 Full Speed devices already met USB 2.0 requirements. The changes were mostly clarifications and tightening of tolerances.

### 2. **Descriptor Differences**

**USB 1.1 Device:**
```c
.USBSpecification = VERSION_BCD(1,1,0),  // Reports USB 1.1
```

**USB 2.0 Device:**
```c
.USBSpecification = VERSION_BCD(2,0,0),  // Reports USB 2.0
```

**Important:** Even a Full Speed-only device should report USB 2.0 in modern designs.

### 3. **Device Qualifier Descriptor (USB 2.0 Addition)**

**USB 2.0 added a new descriptor:**

```c
// Only required for High Speed capable devices
USB_Descriptor_DeviceQualifier_t
```

**Purpose:** Allows High Speed capable devices to describe their Full Speed configuration (and vice versa).

**For Full Speed-only devices:** Not required! You can return STALL on this request.

### 4. **High Speed Capable vs Full Speed Only**

This is the **real distinction** in USB 2.0:

**USB 2.0 High Speed Device:**
- Initially enumerates at Full Speed (12 Mbps)
- Performs "High Speed Detection" handshake (chirp sequence)
- If successful, switches to High Speed (480 Mbps)
- Must support Device Qualifier Descriptor

**USB 2.0 Full Speed Device:**
- Enumerates at Full Speed (12 Mbps)
- Does NOT perform High Speed chirp
- Stays at Full Speed
- Essentially identical to USB 1.1 Full Speed

### 5. **Power Management (Minor Updates)**

**USB 2.0 clarified:**
- Suspend/Resume timing
- Remote wakeup behavior
- Power consumption limits

**Changes are minor** and mostly affect suspend/resume state machines.

### 6. **Backwards Compatibility**

**USB 2.0 Full Speed is 100% backwards compatible with USB 1.1 Full Speed:**
- USB 2.0 host can use USB 1.1 device
- USB 1.1 host can use USB 2.0 Full Speed device
- Electrically and logically compatible

## Practical Implications for Your KeyDU:

### What You Should Report:

```c
const USB_Descriptor_Device_t DeviceDescriptor = {
    .USBSpecification = VERSION_BCD(2,0,0),  // ← Report USB 2.0!
    // Even though you're Full Speed only
    // ...
};
```

**Why report USB 2.0 even though you're Full Speed only?**
1. USB 2.0 has been standard since 2000
2. Modern hosts expect USB 2.0
3. USB 1.1 looks outdated (even if functionally identical)
4. Better compatibility with modern systems

### High Speed Detection:

**Your AVR-DU is Full Speed only**, so:
- It will **not** perform the High Speed chirp sequence
- Host will keep it at Full Speed (12 Mbps)
- No High Speed negotiation happens
- This is perfectly normal and expected

### Device Qualifier Descriptor:

**You should handle this request:**

```c
// In your USB control request handler
case GET_DESCRIPTOR:
    if (wValue == DTYPE_DeviceQualifier) {
        // Full Speed only device - STALL this request
        Endpoint_StallTransaction();
        return;
    }
    break;
```

**Meaning:** "I don't support High Speed, so I have no alternate configuration to report."

## Testing Your Device:

### What the Host Sees:

**When you plug in KeyDU:**
```
1. Device connects
2. Host detects Full Speed device (D+ pull-up)
3. Host checks: "Are you High Speed capable?"
4. Device: (no chirp response)
5. Host: "OK, you're Full Speed. That's fine."
6. Enumeration proceeds at 12 Mbps
```

### Using lsusb (Linux):

```bash
$ lsusb -v -d 03eb:2ff4

Device Descriptor:
  bcdUSB               2.00      ← Reports USB 2.0
  bDeviceClass            0 
  bDeviceSubClass         0 
  bMaxPacketSize0        64
  idVendor           0x03eb
  idProduct          0x2ff4
  bcdDevice            1.00
  iManufacturer           1 
  iProduct                2 KeyDU Bootloader
  bNumConfigurations      1
  
Configuration Descriptor:
  bMaxPower              100mA
  
Interface Descriptor:
  bInterfaceClass       255 Vendor Specific
  
# Note: It's USB 2.0 compliant, but Full Speed (12 Mbps)
```

## Common Misconception:

❌ **WRONG:** "USB 2.0 means High Speed (480 Mbps)"

✅ **CORRECT:** "USB 2.0 specification defines three speeds: Low (1.5), Full (12), and High (480). A device can be USB 2.0 compliant while only supporting Full Speed."

## Feature Comparison Table:

| Feature | USB 1.1 FS | USB 2.0 FS | USB 2.0 HS |
|---------|------------|------------|------------|
| **Full Speed (12 Mbps)** | ✅ | ✅ | ✅ (fallback) |
| **High Speed (480 Mbps)** | ❌ | ❌ | ✅ |
| **bcdUSB Reported** | 1.10 | 2.00 | 2.00 |
| **Device Qualifier** | N/A | Optional (STALL) | Required |
| **Chirp Handshake** | N/A | ❌ | ✅ |
| **Max EP0 Packet** | 8/16/32/64 | 8/16/32/64 | 64 |
| **Max Bulk Packet** | 8/16/32/64 | 8/16/32/64 | 512 |
| **Max Interrupt Packet** | 64 | 64 | 1024 |

## What This Means for AVR-DU:

The **AVR-DU USB peripheral is USB 2.0 compliant, Full Speed only:**

1. **Electrically:** Meets USB 2.0 Full Speed specs
2. **Protocol:** Implements USB 2.0 protocol (at Full Speed)
3. **Speed:** 12 Mbps maximum (no High Speed support)
4. **Descriptors:** Report bcdUSB = 2.00
5. **Compatibility:** Works with any USB 1.1 or 2.0 host

### From the Datasheet:

The AVR-DU datasheet will say something like:
> "USB 2.0 Full Speed (12 Mbps) compliant"

This means:
- ✅ USB 2.0 spec compliance
- ✅ Full Speed operation (12 Mbps)
- ❌ No High Speed support (480 Mbps)

## Practical Answer:

**For your KeyDU project:**

1. **Report USB 2.0** in your device descriptor (`bcdUSB = 2.00`)
2. **Operate at Full Speed** (12 Mbps)
3. **STALL Device Qualifier requests** (you're not High Speed capable)
4. **Don't worry about High Speed** - you don't need it for a keyboard!

**The differences between USB 1.1 FS and USB 2.0 FS are minimal:**
- Mostly clarifications and tightened specs
- Functionally nearly identical
- USB 2.0 FS is just "USB 1.1 FS with better documentation"

**Bottom line:** Your AVR-DU is a **USB 2.0 Full Speed device**, which is exactly what you want for a keyboard bootloader!



## Prompt:
12/31/2025, 9:16:20 PM

describe the enumeration process in 1000 words or less that would be applicable when my keyboard resets and enters bootloader mode (vendorspecific  / custom class). include the very first step as well which would be the host noticing a new usb device with pull up on d+, right?



## Response:
12/31/2025, 9:17:06 PM

# USB Enumeration Process: KeyDU Entering Bootloader Mode

## Physical Connection and Detection

When your keyboard resets into bootloader mode, the AVR-DU enables a 1.5kΩ pull-up resistor on the D+ data line. The USB host (PC) continuously monitors both D+ and D- lines for voltage changes. When the host detects D+ pulled high to approximately 3.3V (while D- remains low), it recognizes this as a **Full Speed device attachment event**. The pull-up on D+ (rather than D-) is the electrical signature that distinguishes Full Speed (12 Mbps) from Low Speed (1.5 Mbps) devices.

The host waits approximately 100ms for the connection to stabilize, allowing any electrical transients to settle. This debounce period ensures the connection is genuine and not a spurious noise spike.

## Bus Reset

The host initiates enumeration by asserting a **USB Reset** condition: it drives both D+ and D- low for at least 10ms. Your AVR-DU detects this reset condition through its USB peripheral hardware, which triggers an interrupt or status flag. In response, your bootloader firmware must:

- Reset all USB-related state machines to default
- Disable all endpoints except Endpoint 0 (the default control endpoint)
- Prepare to respond at the default device address (address 0)
- Set maximum packet size for EP0 to 64 bytes (Full Speed default)

After the reset ends (host releases the lines), the device must be ready to communicate within 10ms. Your device is now in the **Default state**, listening at address 0 on the default control pipe (Endpoint 0).

## Get Device Descriptor (First 8 Bytes)

The host's first request is a control transfer to retrieve the Device Descriptor. However, it initially requests only the **first 8 bytes** to learn the maximum packet size for Endpoint 0 (bMaxPacketSize0), which it needs to know before proceeding.

**The transaction sequence:**

1. **SETUP packet**: Host sends to address 0, EP0
   - bmRequestType = 0x80 (Device-to-Host, Standard, Device)
   - bRequest = GET_DESCRIPTOR (0x06)
   - wValue = 0x0100 (Device Descriptor type)
   - wIndex = 0x0000
   - wLength = 0x0008 (only 8 bytes)

2. **DATA IN packet**: Your device responds with first 8 bytes:
   ```
   bLength = 18
   bDescriptorType = 0x01 (Device)
   bcdUSB = 0x0200 (USB 2.0)
   bDeviceClass = 0xFF (Vendor Specific)
   bDeviceSubClass = 0x00
   bDeviceProtocol = 0x00
   bMaxPacketSize0 = 64
   // ... (stops here, only 8 bytes sent)
   ```

3. **STATUS OUT**: Host sends zero-length packet, device ACKs

Now the host knows EP0 can handle 64-byte packets.

## Second Reset

Some hosts (especially Windows) issue a **second bus reset** at this point. This is normal behavior. Your device returns to Default state (address 0) and waits for the next command. This reset ensures a clean state before address assignment.

## Set Address

The host assigns your device a unique address (typically 1-127) so multiple devices can coexist on the bus.

**SETUP packet**:
- bmRequestType = 0x00 (Host-to-Device, Standard, Device)
- bRequest = SET_ADDRESS (0x05)
- wValue = [new address] (e.g., 0x0003 for address 3)
- wIndex = 0x0000
- wLength = 0x0000 (no data stage)

**Critical timing**: Your device must ACK the STATUS stage while still at address 0, then wait **at least 2ms** before actually switching to the new address. This is because the host's STATUS IN transaction expects a response from address 0. After the 2ms delay, your firmware updates the USB peripheral's address register.

Your device transitions from **Default state** to **Address state**.

## Get Device Descriptor (Complete)

Now the host requests the full Device Descriptor at your new address.

**SETUP packet** (to your new address):
- Same as before, but wLength = 0x0012 (18 bytes)

**DATA IN**: Your device sends the complete 18-byte Device Descriptor:
```
bLength = 18
bDescriptorType = 0x01
bcdUSB = 0x0200
bDeviceClass = 0xFF (Vendor Specific)
bDeviceSubClass = 0x00
bDeviceProtocol = 0x00
bMaxPacketSize0 = 64
idVendor = 0x03EB (Atmel/Microchip)
idProduct = 0x2FF4 (KeyDU Bootloader)
bcdDevice = 0x0100
iManufacturer = 1 (string index)
iProduct = 2 (string index)
iSerialNumber = 0 (no serial number)
bNumConfigurations = 1
```

The host now knows your device has one configuration and is vendor-specific class.

## Get Configuration Descriptor

The host requests the Configuration Descriptor to learn about interfaces and endpoints.

**First request** (gets just configuration descriptor header, 9 bytes):
- bRequest = GET_DESCRIPTOR
- wValue = 0x0200 (Configuration type)
- wLength = 0x0009

**DATA IN**: Device sends configuration header:
```
bLength = 9
bDescriptorType = 0x02 (Configuration)
wTotalLength = 32 (example: config + interface, no extra endpoints)
bNumInterfaces = 1
bConfigurationValue = 1
iConfiguration = 0
bmAttributes = 0x80 (bus-powered)
bMaxPower = 50 (100mA)
```

**Second request** (gets complete configuration, wTotalLength bytes):
The host now requests all 32 bytes to get the Interface Descriptor and any Endpoint Descriptors bundled with it.

Your complete configuration descriptor set includes:
- Configuration Descriptor (9 bytes)
- Interface Descriptor (9 bytes) - describing your vendor-specific interface
- Endpoint Descriptors (if you have any besides EP0)

For a simple bootloader using only control transfers on EP0, you might have **no additional endpoint descriptors**.

## Get String Descriptors (Optional)

The host may request string descriptors for manufacturer and product names:

**Get String Descriptor 0** (language IDs):
- wValue = 0x0300, wIndex = 0x0000

**Get Manufacturer String** (index 1):
- wValue = 0x0301, wIndex = 0x0409 (US English)
- Returns: "KeyDU" or "Your Name"

**Get Product String** (index 2):
- wValue = 0x0302, wIndex = 0x0409
- Returns: "KeyDU Bootloader"

## Set Configuration

The host activates your configuration, transitioning the device to **Configured state** - now fully operational.

**SETUP packet**:
- bmRequestType = 0x00
- bRequest = SET_CONFIGURATION (0x09)
- wValue = 0x0001 (bConfigurationValue from your config descriptor)
- wIndex = 0x0000
- wLength = 0x0000

**Response**: Device ACKs and enables the interface. Any additional endpoints (beyond EP0) are now active and ready for use.

## Enumeration Complete

Your bootloader is now in **Configured state** and fully operational. The host recognizes it as:
- VID:PID 0x03EB:0x2FF4
- Vendor-specific class device
- "KeyDU Bootloader"

The device is ready to receive vendor-specific control requests for erasing and writing flash memory. Your custom flasher software can now find the device by VID:PID and issue commands like:
- `VENDOR_REQ_ERASE_MULTI` (erase flash pages)
- `VENDOR_REQ_WRITE` (write firmware data)
- `VENDOR_REQ_RESET` (jump to application)

The entire enumeration process typically completes in **200-500ms**. Throughout this process, all communication happens through **control transfers on Endpoint 0**, using the standard request-response pattern defined by the USB specification.



## Prompt:
12/31/2025, 9:28:00 PM

And what would be different with the enumeration process for the keyboard app (hid class)?



## Response:
12/31/2025, 9:28:50 PM

# USB Enumeration Process: Keyboard Application (HID Class)

Most of the enumeration process is **identical** to the bootloader. The differences occur in the **descriptor content** and one **additional descriptor request**. Here's what changes:

## Identical Steps (No Changes)

1. **Physical detection** - D+ pull-up, same as bootloader
2. **Bus Reset** - Same reset sequence
3. **Get Device Descriptor (8 bytes)** - Same process
4. **Second Reset** - Same if host does it
5. **Set Address** - Same address assignment process

## First Major Difference: Device Descriptor Content

When the host requests the full Device Descriptor, the **content differs**:

**Bootloader (Vendor Specific):**
```
bDeviceClass = 0xFF (Vendor Specific)
bDeviceSubClass = 0x00
bDeviceProtocol = 0x00
idProduct = 0x2FF4 (Bootloader PID)
iProduct = "KeyDU Bootloader"
```

**Keyboard App (HID):**
```
bDeviceClass = 0x00 (Defined in Interface)
bDeviceSubClass = 0x00
bDeviceProtocol = 0x00
idProduct = 0x2FF5 (Keyboard PID)
iProduct = "KeyDU Keyboard"
```

**Key difference**: HID devices typically use `bDeviceClass = 0x00`, which means "look at the interface descriptors to determine device class." The actual HID class code (0x03) appears in the Interface Descriptor instead.

## Second Major Difference: Configuration Descriptor Content

The Configuration Descriptor **structure is longer** because it includes HID-specific descriptors:

**Bootloader Configuration (Simple):**
```
Configuration Descriptor (9 bytes)
├─ Interface Descriptor (9 bytes)
│    bInterfaceClass = 0xFF (Vendor)
│    bInterfaceSubClass = 0x00
│    bInterfaceProtocol = 0x00
│    bNumEndpoints = 0 (only EP0)
└─ (no additional endpoints)

wTotalLength = 18 bytes
```

**Keyboard Configuration (HID):**
```
Configuration Descriptor (9 bytes)
├─ Interface Descriptor (9 bytes)
│    bInterfaceClass = 0x03 (HID)
│    bInterfaceSubClass = 0x01 (Boot Interface)
│    bInterfaceProtocol = 0x01 (Keyboard)
│    bNumEndpoints = 1 (one interrupt endpoint)
│
├─ HID Descriptor (9 bytes)  ← NEW!
│    bcdHID = 0x0111 (HID 1.11)
│    bCountryCode = 0x00
│    bNumDescriptors = 1
│    bDescriptorType = 0x22 (Report)
│    wDescriptorLength = 63 (size of report descriptor)
│
└─ Endpoint Descriptor (7 bytes)  ← NEW!
     bEndpointAddress = 0x81 (EP1 IN)
     bmAttributes = 0x03 (Interrupt)
     wMaxPacketSize = 8 (8-byte keyboard reports)
     bInterval = 10 (poll every 10ms)

wTotalLength = 34 bytes
```

The host retrieves this complete descriptor bundle (34 bytes) in the second GET_CONFIGURATION request.

## Third Major Difference: Get HID Descriptor (Implicit)

The HID Descriptor is **embedded** in the Configuration Descriptor, so the host already has it after getting the configuration. However, some hosts may explicitly request it separately:

**SETUP packet:**
- bmRequestType = 0x81 (Device-to-Host, Standard, Interface)
- bRequest = GET_DESCRIPTOR (0x06)
- wValue = 0x2100 (HID Descriptor type = 0x21)
- wIndex = 0x0000 (interface number)
- wLength = 0x0009

Your device responds with the same 9-byte HID Descriptor already sent in the configuration bundle.

## Fourth Major Difference: Get Report Descriptor (NEW STEP!)

This is the **most significant addition** for HID devices. The Report Descriptor defines the format of data your keyboard sends (key reports).

**SETUP packet:**
- bmRequestType = 0x81 (Device-to-Host, Standard, Interface)
- bRequest = GET_DESCRIPTOR (0x06)
- wValue = **0x2200** (Report Descriptor type = 0x22)
- wIndex = 0x0000 (interface number)
- wLength = 63 (or whatever wDescriptorLength was in HID Descriptor)

**DATA IN**: Your device sends the HID Report Descriptor (e.g., 63 bytes):

```c
// Boot Keyboard Report Descriptor (simplified example)
0x05, 0x01,        // Usage Page (Generic Desktop)
0x09, 0x06,        // Usage (Keyboard)
0xA1, 0x01,        // Collection (Application)
0x05, 0x07,        //   Usage Page (Key Codes)
0x19, 0xE0,        //   Usage Minimum (224)
0x29, 0xE7,        //   Usage Maximum (231)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x01,        //   Logical Maximum (1)
0x75, 0x01,        //   Report Size (1)
0x95, 0x08,        //   Report Count (8)
0x81, 0x02,        //   Input (Data, Variable, Absolute) - Modifier byte
0x95, 0x01,        //   Report Count (1)
0x75, 0x08,        //   Report Size (8)
0x81, 0x01,        //   Input (Constant) - Reserved byte
0x95, 0x06,        //   Report Count (6)
0x75, 0x08,        //   Report Size (8)
0x15, 0x00,        //   Logical Minimum (0)
0x25, 0x65,        //   Logical Maximum (101)
0x05, 0x07,        //   Usage Page (Key Codes)
0x19, 0x00,        //   Usage Minimum (0)
0x29, 0x65,        //   Usage Maximum (101)
0x81, 0x00,        //   Input (Data, Array) - Key array (6 keys)
0xC0               // End Collection
```

This descriptor tells the host: "My keyboard reports are 8 bytes: 1 modifier byte, 1 reserved byte, and 6 keycode bytes (6KRO)."

The host parses this Report Descriptor to understand how to interpret the data your keyboard will send via the interrupt endpoint.

## Fifth Difference: Set Protocol (Boot Protocol)

For boot keyboards (BIOS-compatible), the host may explicitly request **Boot Protocol** mode:

**SETUP packet:**
- bmRequestType = 0x21 (Host-to-Device, Class, Interface)
- bRequest = **SET_PROTOCOL** (0x0B) - HID-specific request!
- wValue = 0x0000 (0 = Boot Protocol, 1 = Report Protocol)
- wIndex = 0x0000 (interface number)
- wLength = 0x0000

Your device ACKs this request. In Boot Protocol mode, you must send the standard 8-byte boot keyboard report format, which is exactly what your 6KRO keyboard does anyway.

## Sixth Difference: Set Idle (Optional)

The host may set the **idle rate** - how often the keyboard should send reports even when no keys change:

**SETUP packet:**
- bmRequestType = 0x21 (Host-to-Device, Class, Interface)
- bRequest = **SET_IDLE** (0x0A) - HID-specific request!
- wValue = 0x0000 (0 = infinite, only send on change; or duration in 4ms units)
- wIndex = 0x0000 (interface number)
- wLength = 0x0000

Typical value is 0 (only report changes) or 500ms. Your device stores this value and respects it when sending reports.

## Set Configuration (Same Process, Different Effect)

**SETUP packet** (same as bootloader):
- bRequest = SET_CONFIGURATION (0x09)
- wValue = 0x0001

**But now**: When your keyboard ACKs, it must also:
- **Enable Endpoint 1 IN** (the interrupt endpoint)
- Configure EP1 for interrupt transfers
- Set EP1 max packet size to 8 bytes
- Prepare to send keyboard reports on EP1

## Enumeration Complete - Key Differences

Your keyboard is now in **Configured state**, but the host recognizes it as:
- VID:PID **0x03EB:0x2FF5** (different PID!)
- **HID Class, Boot Interface, Keyboard Protocol**
- "KeyDU Keyboard"
- Has **Endpoint 1 IN** for sending key reports

The OS loads the **HID driver** (built into all modern OSes - no custom driver needed!) and begins polling EP1 IN every 10ms (or whatever bInterval you specified).

## Operational Differences After Enumeration

**Bootloader (Vendor Class):**
- Host sends **vendor-specific control requests** on EP0
- Host initiates all communication
- Only EP0 is used

**Keyboard (HID Class):**
- Host **polls EP1 IN** every 10ms via interrupt transfers
- Device sends 8-byte reports when keys change (or at idle rate)
- EP0 is used for HID class requests (SET_IDLE, GET_REPORT, etc.)

Example keyboard report sent on EP1 IN when 'A' key is pressed:
```
Byte 0: 0x00 (no modifiers)
Byte 1: 0x00 (reserved)
Byte 2: 0x04 (keycode for 'A')
Byte 3-7: 0x00 (no other keys)
```

## Summary: What's Different for HID

| Aspect | Bootloader (Vendor) | Keyboard (HID) |
|--------|-------------------|----------------|
| **Device Class** | 0xFF (Vendor) | 0x00 (in Interface) |
| **Interface Class** | 0xFF | 0x03 (HID) |
| **PID** | 0x2FF4 | 0x2FF5 |
| **Endpoints** | EP0 only | EP0 + EP1 IN |
| **Config Length** | ~18 bytes | ~34 bytes |
| **HID Descriptor** | None | Yes (9 bytes) |
| **Report Descriptor** | None | **Yes (63 bytes) - NEW!** |
| **SET_PROTOCOL** | Not used | **May be requested** |
| **SET_IDLE** | Not used | **May be requested** |
| **OS Driver** | None (custom tool) | Built-in HID driver |
| **Communication** | Control transfers | Interrupt transfers |

The enumeration process is **90% identical**, but the HID keyboard requires:
1. **One extra descriptor** (Report Descriptor) - the biggest addition
2. **Two potential HID class requests** (SET_PROTOCOL, SET_IDLE)
3. **One additional endpoint** configured at SET_CONFIGURATION time

Total enumeration time is still **200-500ms**, only slightly longer due to the Report Descriptor transfer.



## Prompt:
1/5/2026, 10:12:23 PM

explain the relashionship between the recommended usb hid pollong frequency and the recommended matrix scanning frequency



## Response:
1/5/2026, 10:13:29 PM

# Relationship Between USB HID Polling and Matrix Scanning Frequencies

## The Two Frequencies Explained

### USB HID Polling Frequency (bInterval)

This is how often the **host asks** your keyboard "do you have any data for me?"

**Common values:**
- **10ms (100 Hz)** - Standard for most keyboards
- **8ms (125 Hz)** - Common
- **4ms (250 Hz)** - Low-latency keyboards
- **2ms (500 Hz)** - Gaming keyboards
- **1ms (1000 Hz)** - High-end gaming keyboards

**Set in Endpoint Descriptor:**
```c
Endpoint Descriptor (EP1 IN):
    bEndpointAddress = 0x81 (EP1 IN)
    bmAttributes = 0x03 (Interrupt)
    wMaxPacketSize = 8
    bInterval = 10  // Poll every 10ms (100 Hz)
```

**What actually happens:**
- Every 10ms, host sends an **IN token** to EP1
- Your device either responds with an 8-byte report (if keys changed) or NAK (no data)
- This sets the **maximum report rate** - you cannot send data faster than the host asks

### Matrix Scanning Frequency

This is how often your **firmware checks** which keys are physically pressed.

**Common values:**
- **1000 Hz (1ms)** - High performance
- **500 Hz (2ms)** - Good performance
- **200-250 Hz (4-5ms)** - Adequate
- **100 Hz (10ms)** - Minimum acceptable

**Implemented in firmware:**
```c
void main(void) {
    while(1) {
        scan_matrix();      // Check all keys
        debounce();         // Filter bounces
        update_report();    // Prepare USB report
        
        delay_us(1000);     // Wait 1ms (1000 Hz scan rate)
    }
}
```

## The Critical Relationship

### Rule: Matrix Scan Rate ≥ USB Polling Rate

**You must scan the matrix at least as fast as USB polls**, preferably faster.

**Why?**

If you scan slower than USB polls, you'll miss key presses between scans:

```
USB polls every 1ms (1000 Hz)
Matrix scans every 10ms (100 Hz)

Timeline:
0ms:  Scan matrix, no keys
1ms:  USB polls → NAK (no data ready)
2ms:  USB polls → NAK
3ms:  USB polls → NAK
5ms:  User presses key ← KEY PRESSED HERE!
      (but we won't know until next scan)
8ms:  USB polls → NAK (still don't know about key!)
9ms:  USB polls → NAK
10ms: Scan matrix → KEY DETECTED!
11ms: USB polls → Send report (key press)

Total latency: 5ms (key pressed) + 5ms (until scan) + 1ms (until poll) = 11ms
```

### Optimal: Matrix Scan Rate >> USB Polling Rate

**Best practice: Scan matrix 2-10× faster than USB polling**

```
USB polls every 1ms (1000 Hz)
Matrix scans every 1ms (1000 Hz) or faster

Timeline:
0ms:  Scan matrix, no keys
1ms:  Scan matrix, no keys; USB polls → NAK
2ms:  Scan matrix, no keys; USB polls → NAK
5ms:  User presses key
5ms:  Scan matrix → KEY DETECTED IMMEDIATELY!
6ms:  USB polls → Send report (key press)

Total latency: 0ms (scan) + 1ms (until next poll) = 1ms
```

## Practical Recommendations

### Standard Keyboard (10ms USB polling / 100 Hz)

**USB Endpoint Descriptor:**
```c
bInterval = 10  // 100 Hz
```

**Matrix Scanning:**
```c
// Scan at 200-500 Hz (2-5ms per scan)
delay_us(2000);  // 500 Hz - good balance
```

**Rationale:**
- USB only checks every 10ms anyway
- Scanning at 500 Hz catches keys within 2ms
- Total worst-case latency: 2ms (scan) + 10ms (USB) = **12ms**
- Plenty fast for typing

### Gaming Keyboard (1ms USB polling / 1000 Hz)

**USB Endpoint Descriptor:**
```c
bInterval = 1  // 1000 Hz
```

**Matrix Scanning:**
```c
// Scan at 1000 Hz minimum, preferably faster
delay_us(500);  // 2000 Hz - catches keys within 0.5ms
```

**Rationale:**
- USB polls every 1ms - must scan at least this fast
- Scanning at 2000 Hz ensures key detected within 0.5ms
- Total worst-case latency: 0.5ms (scan) + 1ms (USB) = **1.5ms**
- Competitive gaming level

## Latency Breakdown

**Total Input Latency = Scan Latency + USB Latency + Processing**

### Component 1: Scan Latency

**Worst case**: Time from key press to next matrix scan

- If scanning at 1000 Hz: **0-1ms** (average 0.5ms)
- If scanning at 500 Hz: **0-2ms** (average 1ms)
- If scanning at 100 Hz: **0-10ms** (average 5ms)

### Component 2: USB Polling Latency

**Worst case**: Time from report ready to next USB poll

- If USB polling at 1000 Hz: **0-1ms** (average 0.5ms)
- If USB polling at 500 Hz: **0-2ms** (average 1ms)
- If USB polling at 100 Hz: **0-10ms** (average 5ms)

### Component 3: Processing Time

- Debouncing: **~1-2ms** (typical algorithms)
- Report preparation: **<0.1ms** (negligible)

### Example Configurations

**Budget keyboard:**
- Matrix: 200 Hz (5ms scan period)
- USB: 100 Hz (10ms polling)
- Total latency: **5ms + 10ms + 2ms = 17ms** (worst case)
- **11ms average** - acceptable for typing

**Standard mechanical keyboard (like KeyDU):**
- Matrix: 500 Hz (2ms scan period)
- USB: 125 Hz (8ms polling)
- Total latency: **2ms + 8ms + 2ms = 12ms** (worst case)
- **7ms average** - good for typing and casual gaming

**Gaming keyboard:**
- Matrix: 1000 Hz (1ms scan period)
- USB: 1000 Hz (1ms polling)
- Total latency: **1ms + 1ms + 1ms = 3ms** (worst case)
- **2ms average** - excellent for competitive gaming

## Why Not Just Scan as Fast as Possible?

### Diminishing Returns

**Scanning faster than USB polls doesn't help latency:**

```
Matrix: 10,000 Hz (0.1ms)
USB: 1000 Hz (1ms)

Key pressed at 0.5ms:
- Detected at 0.6ms (next scan)
- Sent at 1.0ms (next USB poll)
Total: 1.0ms

vs.

Matrix: 1000 Hz (1ms)
USB: 1000 Hz (1ms)

Key pressed at 0.5ms:
- Detected at 1.0ms (next scan)
- Sent at 1.0ms (same USB poll)
Total: 1.0ms (same!)
```

**The USB polling interval becomes the bottleneck.**

### CPU Usage / Power Consumption

**Scanning costs CPU cycles:**

```c
void scan_matrix(void) {
    // Set row outputs (GPIOs)
    // Read column inputs (GPIOs)
    // Process 60+ keys
    // Takes ~50-200 µs depending on matrix size
}
```

Scanning at 10,000 Hz = 50-200µs × 10,000 = **0.5-2 seconds of CPU per second!**

That's 50-200% CPU usage just for scanning! Plus higher power consumption.

### Practical Limits

**GPIO switching speed:**
- Each row needs time to settle before reading columns
- Typical: **1-5µs per row**
- 5 rows = 5-25µs minimum scan time
- Theoretical max: ~40,000-200,000 Hz
- Practical max: ~2000-5000 Hz with good design

## Recommended Strategy for KeyDU

### Conservative Approach (Good for Most Users)

```c
// USB Endpoint Descriptor
bInterval = 10  // 100 Hz (10ms)

// Main loop
void main(void) {
    while(1) {
        scan_matrix();      // ~50µs
        debounce();         // ~20µs  
        update_usb_report(); // ~10µs
        
        delay_us(2000);     // Scan at 500 Hz (2ms)
        // ~80µs work + 1920µs idle = 4% CPU usage
    }
}
```

**Benefits:**
- Low power consumption (96% CPU idle)
- 2ms + 10ms = **12ms worst-case latency**
- Excellent for typing
- Fine for casual gaming

### Performance Approach (Gaming)

```c
// USB Endpoint Descriptor
bInterval = 1  // 1000 Hz (1ms)

// Main loop - interrupt driven
ISR(TIMER_1MS) {  // Fires every 1ms
    scan_matrix();
    debounce();
    update_usb_report();
    // Runs 1000× per second
}

void main(void) {
    while(1) {
        // Handle other tasks
        // USB peripheral handles sending reports
    }
}
```

**Benefits:**
- 1ms + 1ms = **2ms worst-case latency**
- Still only ~8% CPU (80µs work per 1ms)
- Competitive gaming performance

## Special Consideration: Debouncing

**Debouncing adds inherent latency:**

Most debounce algorithms require **2-5 consecutive consistent reads** before accepting a key state change:

```c
#define DEBOUNCE_TIME_MS 5  // 5ms debounce

// Key must be stable for 5ms before accepting
// With 1ms scanning, that's 5 consecutive scans
```

**This means:**
- Even with 1ms scanning and 1ms USB polling
- You have **+5ms debounce latency**
- Total: 1ms + 1ms + 5ms = **7ms minimum**

**Trade-off:**
- Less debounce time = more false triggers (chattering)
- More debounce time = higher latency but reliable
- 5ms is a good compromise

## Final Recommendation for KeyDU

**Balanced Configuration:**

```c
// USB descriptor
bInterval = 8  // 125 Hz (8ms) - sweet spot

// Firmware
#define SCAN_INTERVAL_US 1000  // 1000 Hz (1ms scan rate)
#define DEBOUNCE_TIME_MS 5     // 5ms debounce

// Total latency breakdown:
// Scan: 0.5ms average (worst 1ms)
// Debounce: 5ms
// USB: 4ms average (worst 8ms)
// Total: ~9.5ms average, ~14ms worst case
```

**This gives you:**
- ✅ Excellent typing experience
- ✅ Good gaming performance
- ✅ Low power consumption
- ✅ Reliable (proper debouncing)
- ✅ Reasonable CPU usage (~8%)

**Summary:** Scan your matrix **at least as fast** as USB polls, ideally **2-10× faster**, but don't go crazy beyond that - you hit diminishing returns and waste power. Match the configuration to your use case: casual typing vs competitive gaming.



## Prompt:
1/5/2026, 10:30:38 PM

My use case would be typing at a decent speed of around 85 wpm.



## Response:
1/5/2026, 10:31:41 PM

# Optimal Configuration for 85 WPM Typing

For 85 WPM typing, you have **very relaxed latency requirements**. Let's look at the math to see what you actually need.

## Typing Speed Analysis

### 85 WPM Breakdown

**85 words per minute:**
- Average word length: 5 characters
- 85 words × 5 chars = **425 characters per minute**
- 425 / 60 seconds = **7.08 characters per second**
- Time between keystrokes: **141ms average**

**Even at peak speed (bursts):**
- Fast typists might hit 10-12 characters/second in short bursts
- That's still **83-100ms between keypresses**

### Human Perception Threshold

**Humans perceive delays differently based on context:**

- **<10ms**: Imperceptible (feels instant)
- **10-50ms**: Subconscious detection, but feels responsive
- **50-100ms**: Noticeable but acceptable
- **100ms+**: Feels sluggish

**For typing specifically:**
- Research shows typists don't notice latency under **30ms**
- Even **50ms** is acceptable for typing (vs gaming where 10ms matters)
- Your 141ms between keystrokes gives huge margin

## Recommended Configuration for KeyDU

### Conservative, Power-Efficient Setup ✅ **BEST FOR YOU**

```c
// USB Endpoint Descriptor
bInterval = 10  // 100 Hz (10ms USB polling)

// Firmware Configuration
#define SCAN_INTERVAL_US 5000   // 200 Hz (5ms scan rate)
#define DEBOUNCE_TIME_MS 5      // 5ms debounce

// Implementation
void main(void) {
    init_keyboard();
    init_usb();
    
    while(1) {
        scan_matrix();           // ~50µs
        debounce_keys();         // ~20µs
        update_usb_report();     // ~10µs
        
        delay_us(5000);          // Wait 5ms (200 Hz scan)
    }
}
```

**Latency Breakdown:**
- **Scan latency**: 2.5ms average (0-5ms range)
- **Debounce**: 5ms
- **USB polling**: 5ms average (0-10ms range)
- **Total: 12.5ms average, 20ms worst case**

**Why this is perfect for 85 WPM:**
- 20ms worst-case << 141ms between keystrokes
- You'll **never notice** the latency
- **1.6% CPU usage** (80µs / 5000µs)
- **Low power consumption** (98.4% CPU idle)
- Reliable debouncing eliminates false triggers

### Visual Timeline

```
Typing "hello" at 85 WPM (141ms between keys):

0ms:    Press 'h'
2.5ms:  Detected by scan (average)
7.5ms:  Debounce complete
12.5ms: Sent via USB (average)
        ✓ 'h' appears on screen

141ms:  Press 'e' (next keystroke)
143.5ms: Detected
148.5ms: Debounce complete  
153.5ms: Sent via USB
        ✓ 'e' appears on screen

... and so on

Your reaction: Feels instant! ✨
```

## Alternative: Slightly Faster (If You Want)

### Moderate Performance Setup ⚠️ **UNNECESSARY BUT OK**

```c
bInterval = 8               // 125 Hz (8ms USB polling)
#define SCAN_INTERVAL_US 2000   // 500 Hz (2ms scan rate)
#define DEBOUNCE_TIME_MS 5      // 5ms debounce

// Latency: 1ms + 5ms + 4ms = 10ms average
// CPU usage: 4% (80µs / 2000µs)
```

**Benefit over conservative:**
- 10ms vs 12.5ms average latency
- **2.5ms improvement you won't feel**

**Cost:**
- 2.5× higher CPU usage (4% vs 1.6%)
- Slightly higher power consumption
- More frequent GPIO switching (more EMI)

**Verdict:** Not worth it for typing.

## What NOT to Do for Typing

### Gaming Config (Overkill) ❌ **DON'T DO THIS**

```c
bInterval = 1               // 1000 Hz (1ms USB polling)
#define SCAN_INTERVAL_US 1000   // 1000 Hz (1ms scan rate)
#define DEBOUNCE_TIME_MS 3      // Reduced debounce

// Latency: 0.5ms + 3ms + 0.5ms = 4ms average
// CPU usage: 8%
```

**Why this is silly for 85 WPM:**
- 4ms latency vs 12.5ms latency
- **You're typing every 141ms** - the 8.5ms difference is lost in the noise
- 5× higher CPU usage
- 5× higher power consumption  
- Reduced debounce might cause chatter
- Zero benefit for your use case

### The Analogy

It's like buying a Ferrari to drive to the grocery store:
- **Gaming keyboard (1ms)**: 0-60mph in 2 seconds
- **Your typing keyboard (12ms)**: 0-60mph in 5 seconds
- **Your typing speed**: Only need to go 25mph

You're never going to use the performance anyway!

## Real-World Comparison

### Popular Keyboards and Their Specs

| Keyboard | USB Polling | Typical Scan | Latency | Use Case |
|----------|-------------|--------------|---------|----------|
| **Cheap membrane** | 125 Hz (8ms) | 100 Hz (10ms) | ~20ms | Basic typing ✅ |
| **Standard mech** | 125 Hz (8ms) | 200 Hz (5ms) | ~15ms | Typing ✅✅ |
| **Gaming mech** | 1000 Hz (1ms) | 1000 Hz (1ms) | ~5ms | Competitive gaming |
| **Your KeyDU** | 100 Hz (10ms) | 200 Hz (5ms) | ~12ms | 85 WPM typing ✅✅✅ |

**Notice:** Even cheap keyboards are fine for typing! Your conservative config is **better than most commercial typing keyboards**.

## Testing Your Configuration

### Subjective Test (What Matters)

```c
// Implement both configs with a compile-time switch
#ifdef GAMING_MODE
    #define SCAN_INTERVAL_US 1000
    #define USB_BINTERVAL 1
#else
    #define SCAN_INTERVAL_US 5000
    #define USB_BINTERVAL 10
#endif

// Flash both versions and type for 10 minutes each
// I predict you won't feel any difference!
```

**Expected result:** Both feel instant for typing.

### Objective Test (If Curious)

Use a high-speed camera or oscilloscope:
1. Trigger LED when key physically closes
2. Trigger LED when character appears on screen
3. Measure time difference

**You'll find:**
- Conservative: 12-20ms
- Gaming: 4-8ms
- **Both feel instant when typing**

## Power Consumption Estimate

### Conservative Config (200 Hz scan, 100 Hz USB)

```
Active CPU time per second:
- Matrix scan: 50µs × 200 = 10ms
- Debounce: 20µs × 200 = 4ms  
- USB: 10µs × 200 = 2ms
- Total: 16ms / 1000ms = 1.6% active

Power estimate (AVR-DU at 24 MHz):
- Active: 10mA
- Idle: 2mA
- Average: 0.016 × 10mA + 0.984 × 2mA ≈ 2.1mA

LED indicators: +5-10mA (if on)
USB: +2-5mA (signaling)
Total: ~10-17mA (very low!)
```

### Gaming Config (1000 Hz scan, 1000 Hz USB)

```
Active: 8% (80ms / 1000ms)
Power: 0.08 × 10mA + 0.92 × 2mA ≈ 2.6mA

Total with peripherals: ~15-22mA
Increase: +30-50% power vs conservative
```

**For a USB keyboard (always plugged in), this doesn't matter much. But it's wasteful for zero benefit.**

## Practical Advantages of Conservative Config

### 1. Cleaner Code
```c
// Simple main loop
while(1) {
    scan_matrix();
    delay_us(5000);  // Easy to understand
}
```

vs

```c
// Timer-interrupt driven (more complex)
ISR(TIMER_1MS) {
    scan_matrix();  // Must be fast!
}
```

### 2. Easier Debugging

- Slower execution easier to trace
- More time for printf debugging
- Less timing-critical code

### 3. More CPU Time for Future Features

**What if you later add:**
- RGB backlighting effects
- OLED display
- Rotary encoder
- Macro playback
- VIA/VIAL support

**With 98% CPU idle**, you have plenty of room!

### 4. Thermal Margin

- Less CPU activity = less heat
- Better long-term reliability
- Can overclock later if needed

## My Specific Recommendation for KeyDU

```c
/******************************************************************************
 * KeyDU Configuration - Optimized for 85 WPM Typing
 *****************************************************************************/

// USB Configuration
#define USB_POLLING_INTERVAL    10      // 100 Hz (10ms)

// Matrix Scanning  
#define MATRIX_SCAN_RATE_HZ     200     // 200 Hz (5ms)
#define SCAN_INTERVAL_US        5000    // Calculated from rate

// Debouncing
#define DEBOUNCE_TIME_MS        5       // 5ms debounce window

/******************************************************************************
 * Expected Performance:
 * - Average latency: 12.5ms (imperceptible for typing)
 * - Worst case: 20ms (still excellent)
 * - CPU usage: 1.6% (plenty of headroom)
 * - Power: ~10-17mA total
 * 
 * Perfect for:
 * ✅ Professional typing (85+ WPM)
 * ✅ Programming
 * ✅ Writing
 * ✅ Casual gaming
 * 
 * NOT ideal for:
 * ❌ Competitive FPS gaming (use 1ms config instead)
 * ❌ Rhythm games requiring <10ms timing
 *****************************************************************************/
```

## Final Answer

**For 85 WPM typing:**

✅ **Use: 10ms USB polling + 5ms matrix scanning**

You have 141ms between keystrokes. A 12.5ms latency is **completely imperceptible**. Going faster would:
- ❌ Waste CPU cycles
- ❌ Waste power
- ❌ Add code complexity  
- ❌ Provide zero perceptible benefit

**Save the 1ms configs for the gamers. You're building a keyboard, not a racing car!** 🎹

Your conservative configuration will feel **perfectly responsive** for typing while being efficient, reliable, and leaving room for future features. That's good engineering! 👍



## Prompt:
1/5/2026, 11:33:33 PM

with delay_us(5000), my understanding is that the cpu will not be idle. is that accurate, and if so should i consider a timer-based non-blocking loop approach instead of the blocking delay_us() or delay_ms() utility functions? i guess it matters only if there is anything else in my main loop around usb activity, ie my usb code is not entirely driven by ISR() ?



## Response:
1/5/2026, 11:34:38 PM

Excellent observation! You're absolutely right to question this. Let's clarify what's actually happening.

## delay_us() Behavior

### You're Correct: CPU is NOT Idle

```c
void delay_us(uint16_t microseconds) {
    // Typical implementation - BUSY WAIT
    while(microseconds--) {
        _delay_us(1);  // NOP instructions or cycle counting
    }
}
```

**This is a busy-wait loop:**
- CPU executes NOPs or counts cycles
- **100% CPU usage** during delay
- Wastes power
- Blocks everything else

So your "1.6% active" calculation was actually **backwards** - you're actually:
- **98.4% busy-waiting** (doing nothing but burning power)
- **1.6% doing useful work** (scanning matrix)

### The Real Power Consumption

```
With delay_us(5000):
- Active 100% of the time (busy-waiting)
- Power: ~10mA constantly
- NOT power efficient!
```

## USB Stack Architecture Matters

### Is Your USB Code ISR-Driven or Polling?

This determines whether you need non-blocking delays.

### Option A: USB is Fully ISR-Driven (Common in LUFA/Hardware)

**AVR-DU hardware USB peripheral typically works like this:**

```c
// USB interrupts handle everything automatically
ISR(USB_BUSEVENT_vect) {
    // Handle USB reset, suspend, resume
}

ISR(USB_TRNCOMPL_vect) {
    // Handle completed USB transactions
    // Automatically responds to host polls
}

void main(void) {
    init_usb();
    sei();  // Enable global interrupts
    
    while(1) {
        scan_matrix();
        delay_us(5000);  // ← BLOCKS, but USB still works via ISR
    }
}
```

**In this case:**
- USB works independently via interrupts
- `delay_us()` doesn't break USB functionality
- **But you're still wasting power with busy-wait**

### Option B: USB Requires Polling in Main Loop (Less Common)

**Some USB stacks need you to call update functions:**

```c
void main(void) {
    while(1) {
        usb_task();        // Must be called regularly!
        scan_matrix();
        delay_us(5000);    // ← BLOCKS usb_task() - BAD!
    }
}
```

**In this case:**
- `delay_us()` prevents `usb_task()` from running
- USB might miss packets or timeout
- **Blocking delays break functionality**

## Solution: Timer-Based Non-Blocking Approach

### Implementation with Timer

```c
/******************************************************************************
 * Non-blocking timer-based matrix scanning
 *****************************************************************************/

#include <avr/io.h>
#include <avr/interrupt.h>

// Timer fires every 5ms (200 Hz)
#define SCAN_INTERVAL_MS 5

volatile uint8_t scan_flag = 0;

// Timer interrupt - fires every 5ms
ISR(TCB0_INT_vect) {
    scan_flag = 1;  // Signal that it's time to scan
    TCB0.INTFLAGS = TCB_CAPT_bm;  // Clear interrupt flag
}

void timer_init(void) {
    // Configure TCB0 for 5ms periodic interrupt
    // Assuming 24 MHz system clock with prescaler
    
    TCB0.CCMP = (F_CPU / 1024 / 200) - 1;  // 200 Hz
    TCB0.INTCTRL = TCB_CAPT_bm;            // Enable interrupt
    TCB0.CTRLA = TCB_CLKSEL_DIV1024_gc     // Prescaler 1024
               | TCB_ENABLE_bm;             // Enable timer
}

void main(void) {
    init_keyboard();
    init_usb();
    timer_init();
    
    sei();  // Enable global interrupts
    
    while(1) {
        // Check if it's time to scan
        if (scan_flag) {
            scan_flag = 0;
            
            scan_matrix();        // ~50µs
            debounce_keys();      // ~20µs
            update_usb_report();  // ~10µs
            
            // Total work: ~80µs
        }
        
        // USB task (if your stack needs it)
        // usb_task();  // Uncomment if needed
        
        // CPU is now FREE to do other things!
        // Or can enter sleep mode for power savings
        
        // Optional: Add other tasks here
        // handle_rgb_leds();
        // update_oled();
        // process_macros();
    }
}
```

### Benefits of Timer-Based Approach

**1. CPU is Actually Idle:**
```
Timeline (5ms period):
0µs:     Timer fires, scan_flag = 1
         Scan matrix (80µs of work)
80µs:    Work complete, loop continues
         CPU executes main loop (~1µs per iteration)
5000µs:  Timer fires again

CPU utilization: 80µs / 5000µs = 1.6% (for real this time!)
Idle: 98.4% (actually doing nothing)
```

**2. Non-Blocking:**
- Other code in main loop runs continuously
- USB polling works if needed
- Can add features easily

**3. Power Saving Possible:**
```c
while(1) {
    if (scan_flag) {
        scan_flag = 0;
        scan_matrix();
        debounce_keys();
        update_usb_report();
    }
    
    usb_task();  // If needed
    
    // Enter idle sleep until next interrupt
    sleep_cpu();  // Wakes on timer or USB interrupt
}
```

**With sleep mode:**
- Active: 80µs @ 10mA = 0.8µA·s
- Sleep: 4920µs @ 0.5mA = 2460µA·s
- Average current: **≈0.5-1mA** (vs 10mA busy-wait!)

## When Blocking Delays Are Acceptable

### If ALL These Are True:

1. ✅ USB is **fully ISR-driven** (no polling needed)
2. ✅ You have **no other tasks** in main loop
3. ✅ You **don't care about power consumption** (keyboard always plugged in)
4. ✅ Code simplicity matters more than efficiency

**Then this is fine:**
```c
void main(void) {
    init_usb();
    sei();
    
    while(1) {
        scan_matrix();
        delay_us(5000);  // Simple, works, but wastes power
    }
}
```

**Advantages:**
- Dead simple
- Easy to understand
- Works correctly (if USB is ISR-driven)

**Disadvantages:**
- Wastes power (10mA vs 0.5-1mA with sleep)
- Can't add other tasks easily
- CPU at 100% for no reason

## Recommended Approach for KeyDU

### Start Simple, Optimize Later ✅

**Phase 1: Get It Working (Use Blocking Delays)**

```c
void main(void) {
    init_keyboard();
    init_usb();
    sei();
    
    while(1) {
        scan_matrix();
        debounce_keys();
        update_usb_report();
        
        delay_ms(5);  // Simple blocking delay
        
        // USB works via ISR (assuming AVR-DU hardware USB)
    }
}
```

**Why start here:**
- Simplest code
- Proves your USB and matrix code work
- Easy to debug
- Get keyboard functional quickly

**Phase 2: Add Timer When You Need It**

Add non-blocking timer when:
- You want to add RGB LEDs (need continuous updates)
- You want to add OLED display (need regular refresh)
- You want power savings
- You find USB needs polling in main loop

## Checking Your USB Stack

### How to Know if USB Needs Main Loop Polling

**Look at your USB library documentation or examples:**

**LUFA (Common for AVR):**
```c
// LUFA typically needs USB_USBTask() in main loop
while(1) {
    scan_matrix();
    USB_USBTask();  // ← MUST BE CALLED REGULARLY!
    delay_ms(5);    // ← BAD - blocks USB_USBTask()
}
```

**AVR-DU Hardware USB (Fully Interrupt-Driven):**
```c
// No polling needed if using hardware USB peripheral directly
while(1) {
    scan_matrix();
    delay_ms(5);    // ← OK - USB handled by ISR
}
```

**Check your code:**
- If you see `USB_USBTask()` or similar → **Need non-blocking**
- If USB just works with interrupts enabled → **Blocking OK (but wasteful)**

## Hybrid Approach (Best of Both Worlds)

### Simple Timer with Blocking Check

```c
volatile uint32_t millis_counter = 0;
uint32_t last_scan_time = 0;

// 1ms timer interrupt
ISR(TIMER_vect) {
    millis_counter++;
}

uint32_t millis(void) {
    uint32_t ms;
    cli();
    ms = millis_counter;
    sei();
    return ms;
}

void main(void) {
    init_timer();  // 1ms tick
    init_usb();
    sei();
    
    while(1) {
        // Non-blocking time check
        if (millis() - last_scan_time >= 5) {
            last_scan_time = millis();
            
            scan_matrix();
            debounce_keys();
            update_usb_report();
        }
        
        // USB task if needed
        // USB_USBTask();
        
        // Other tasks run freely here
        // handle_leds();
    }
}
```

**Benefits:**
- Simple to understand (like delay, but non-blocking)
- CPU can do other work
- Easy to add more timed tasks
- Arduino-like `millis()` API (familiar to many)

## My Specific Recommendation for KeyDU

### Development Phase: Use Blocking Delays

```c
// Get it working first
void main(void) {
    init_keyboard();
    init_usb();
    sei();
    
    while(1) {
        scan_matrix();
        delay_ms(5);
    }
}
```

**Rationale:** Simplicity while learning USB and getting basic functionality working.

### Production/Final: Use Timer-Based Approach

```c
// Efficient, expandable
ISR(TCB0_INT_vect) {
    scan_flag = 1;
    TCB0.INTFLAGS = TCB_CAPT_bm;
}

void main(void) {
    init_keyboard();
    init_usb();
    timer_init();  // 5ms periodic
    sei();
    
    while(1) {
        if (scan_flag) {
            scan_flag = 0;
            scan_matrix();
            debounce_keys();
            update_usb_report();
        }
        
        // Room for future features!
        
        // Optional: sleep for power savings
        // sleep_cpu();
    }
}
```

**Rationale:** Professional approach, efficient, expandable.

## Final Answer

**Yes, you should consider timer-based non-blocking approach IF:**

1. ✅ Your USB stack needs `USB_USBTask()` or similar polling
2. ✅ You want to add other features (LEDs, display, etc.)
3. ✅ You care about power consumption
4. ✅ You want professional, expandable code

**Blocking delays are acceptable IF:**

1. ✅ USB is fully ISR-driven (no polling needed)
2. ✅ You just want to get it working quickly
3. ✅ You have no other tasks planned
4. ✅ Power consumption doesn't matter (always plugged in)

**My recommendation: Start with blocking delays to prove the concept, then migrate to timer-based approach once you understand the system.** This is the pragmatic path that balances learning, development speed, and eventual code quality.

The timer approach is ~20 lines more code but gives you a solid foundation for future expansion! 🎯





---
Powered by [Claude Exporter](https://www.ai-chat-exporter.net)