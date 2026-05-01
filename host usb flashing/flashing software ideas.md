For custom (vendor) class: possibly inspired from avrusbboot (libusb, winusb), see LUFA's GenericHID example code, see V-USB code, see USBasploader (but remove the ‘pretend to be isp’ aspect)

**USBasploader** is probably the least relevant. It's designed around the USBasp protocol, which is an SPI/ISP programming interface. That's an entirely different target architecture (ATmega ISP, not UPDI), and the host-side tooling assumption is avrdude in USBasp mode. The underlying USB is handled by V-USB, which brings up the next point.

**V-USB** is a software USB implementation for AVRs that lack hardware USB — like the ATmega series. Since the AVR64DU32 has native hardware USB, V-USB is completely irrelevant to you. Cross it off entirely.

**avrusbboot** is similarly V-USB based, so same problem — it's a workaround for AVRs without hardware USB, not applicable to your silicon.

**LUFA's GenericHID example** is closer but still not quite right for your stated goal. GenericHID is HID class, not Vendor class. You already decided on Vendor class with bulk transfers, so while the LUFA USB stack architecture is worth understanding, GenericHID specifically isn't the template you want. The more relevant LUFA reference would be its **MassStorage or CDC bootloader examples**, or better yet the **LUFA DFU bootloader** which uses bulk transfers — though that's DFU class rather than pure Vendor.

Given your actual target (AVR64DU32 native USB, Vendor class, bulk transfers, libusb host side), the most directly relevant references are probably the **Microchip Vendor class example** you already have, and looking at how **LUFA structures its bootloader section jump and SPM write routines**, independent of which USB class they use.



To use avrdude with the usbasploader programmer: The `usbasploader` driver in avrdude uses **libusb** to communicate directly with the vendor class USB device — it bypasses the kernel's HID or CDC class drivers entirely and talks raw USB from userspace. That's true for USBasp, USBaspLoader, and any other vendor-class avrdude programmer.



You'll also need a **udev rule** so avrdude can open the device without root:

# /etc/udev/rules.d/99-keydu.rules

SUBSYSTEM=="usb", ATTRS{idVendor}=="xxxx", ATTRS{idProduct}=="yyyy", MODE="0666", GROUP="plugdev"

Replace `xxxx`/`yyyy` with whatever VID/PID you assign to KeyDU.BL. After adding the rule: `sudo udevadm control --reload && sudo udevadm trigger`.
