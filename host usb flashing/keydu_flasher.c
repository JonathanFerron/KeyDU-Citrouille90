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