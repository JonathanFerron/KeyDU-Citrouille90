The typical flow with libusb-1.0:

```c
#include <libusb-1.0/libusb.h>

int main() {
    libusb_context *ctx = NULL;
    libusb_device_handle *dev = NULL;

    // Initialize
    libusb_init(&ctx);

    // Open device by VID/PID
    dev = libusb_open_device_with_vid_pid(ctx, 0xVVVV, 0xPPPP);
    if (!dev) {
        // not found or permission denied
        libusb_exit(ctx);
        return 1;
    }

    // Claim the interface (interface 0 for a simple vendor device)
    libusb_claim_interface(dev, 0);

    // Bulk or interrupt transfer — adjust endpoint address to match your descriptor
    uint8_t buf[64];
    int transferred;

    // Send (OUT endpoint, e.g. 0x01)
    libusb_bulk_transfer(dev, 0x01, buf, sizeof(buf), &transferred, 1000);

    // Receive (IN endpoint, e.g. 0x81)
    libusb_bulk_transfer(dev, 0x81, buf, sizeof(buf), &transferred, 1000);

    // Control transfer (vendor-class request)
    libusb_control_transfer(dev,
        LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT,
        0x01,        // bRequest — your vendor command
        0x0000,      // wValue
        0x0000,      // wIndex
        buf,         // data
        sizeof(buf), // length
        1000         // timeout ms
    );

    libusb_release_interface(dev, 0);
    libusb_close(dev);
    libusb_exit(ctx);
    return 0;
}
```

Compile with:
```bash
gcc main.c -o main $(pkg-config --cflags --libs libusb-1.0)
```

A few practical notes:

- **VID/PID**: find yours with `lsusb`
- **Endpoints**: check your device's USB descriptor with `lsusb -v -d VVVV:PPPP` — look for `bEndpointAddress` and `bmAttributes` (bulk vs interrupt)
- **Permissions**: by default you'll need `sudo`, or add a udev rule to allow your user access without it:
  ```
  SUBSYSTEM=="usb", ATTR{idVendor}=="VVVV", ATTR{idProduct}=="PPPP", MODE="0666"
  ```
  Drop that in `/etc/udev/rules.d/99-mydevice.rules` and run `sudo udevadm control --reload-rules`
- **Kernel driver**: if the OS has claimed the interface already, you may need `libusb_detach_kernel_driver(dev, 0)` before `claim_interface`