# Citrouille90 To Dos

## Next steps:

1. Burn down the remaining post-enumeration correctness bugs: `usb_sof_enable()` wiring,
   `usb_vendor_task()` calling the wrong function, and the EEPROM bootloader magic-flag
   integration (implemented but not yet tested on hardware). These are known, scoped, and
   cheap now that the stack is alive.

2. Test LED out. Note: the CNano's on-board LED is on PF2, which is a matrix column — not
   suitable for this test. Do on the real Citrouille90 PCB.

## To be addressed:

## Nice to have:

## Keep for phase 2:

1. Stale Report on USB Resume (Phantom Keypresses)
   When the device resumes from USB suspend, the current HID report buffers still hold
   whatever state was staged before suspend. If a key was held at suspend time the host will
   see it as newly pressed on resume, producing a phantom keypress. The fix is to zero both
   s_kbd_buf and s_con_buf (and bump their seqlocks) inside usb_event_wakeup() in usb_hid.c,
   and to call hid_flush() immediately after to push the zero report before the first SOF
   arrives. This is a small, targeted fix but depends on the SOF race being resolved first,
   so it naturally slots in second.

2. Enhanced Documentation
   A documentation pass to bring Firmware reference.md current — particularly sections
   11 (USB stack remarks, currently sparse), 12 (updated with any DFU changes), and 18
   (open items, now mostly closed) — and to add inline /* rationale */ comments to any
   modules that changed significantly in rev 2. Also worth adding a CHANGELOG.md at this
   stage to capture what changed between rev 1 and rev 2 for your own future reference.

3. Time TCB0_INT_vect to 200us prior to expected SOF arrival (800 us after prior SOF arrival)

4. DFU Bootloader Migration
   The current vendor-class bootloader is functional but requires a custom host-side flashing
   tool. Migrating to DFU (Device Firmware Upgrade, USB class 0xFE) would allow flashing
   with standard tools (dfu-util, Microchip's FLIP, or avrdude's DFU backend), which
   simplifies the developer workflow and eliminates the LibUSB dependency in KeyDU.Flasher.
   The AVR64DU32 has documented DFU support. This is the largest rev 2 change — it touches
   KeyDU.BL entirely and requires re-qualifying the flash protocol — so it belongs after the
   application firmware is fully stable.
