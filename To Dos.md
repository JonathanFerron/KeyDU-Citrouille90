# Citrouille90 To Dos

## To be addressed urgently:
- Nothing for the moment

## Next steps:

4. Verify the USB suspend/resume fix (ChangeLog #13) on real hardware.
   Build-verified only so far (`make merged`, no warnings). Needs an actual USB
   selective-suspend/resume cycle on the CNano or real PCB — e.g. hold a key, force the host
   to suspend the device (Linux: `echo suspend > /sys/bus/usb/devices/<dev>/power/level` or
   let the host idle-suspend it), then resume and confirm no phantom keypress/release is seen
   by the host (`evtest` or similar) and normal typing resumes cleanly afterward.

5. Get flashing to work from KeyDU.BL using the vendor protocol and custom host side program (libusb supported)

6. Time TCB0_INT_vect to 200us prior to expected SOF arrival (800 us after prior SOF arrival)


## Nice to have:
- Nothing for the moment

## Keep for phase 2:
-  DFU Bootloader Migration
   The current vendor-class bootloader is functional but requires a custom host-side flashing
   tool. Migrating to DFU (Device Firmware Upgrade, USB class 0xFE) would allow flashing
   with standard tools (dfu-util, Microchip's FLIP, or avrdude's DFU backend), which
   simplifies the developer workflow and eliminates the LibUSB dependency in KeyDU.Flasher.
   The AVR64DU32 has documented DFU support. This is the largest rev 2 change — it touches
   KeyDU.BL entirely and requires re-qualifying the flash protocol — so it belongs after the
   application firmware is fully stable.
