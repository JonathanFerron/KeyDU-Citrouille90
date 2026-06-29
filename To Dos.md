# Citrouille90 To Dos

## To be addressed urgently:
- Nothing for the moment

## Next steps:

1. Enhanced Documentation
   A documentation pass to bring Firmware reference.md current — particularly sections
   11 (USB stack remarks, currently sparse), 12 (updated with any DFU changes), and 18
   (open items, now mostly closed) — and to add inline /* rationale */ comments to any
   modules that changed significantly in rev 2. Also worth adding a CHANGELOG.md at this
   stage to capture what changed between rev 1 and rev 2 for your own future reference.
 
2. Implement a flash_fuses target in the makefile so that fuses get programmed by avrdude once we start using it on the real Citrouille90 board. Fuses are present in the merged hex file, and programming the avr64du32 via the cnano programmer / debugger via the mass-storage device approach programs both the flash (BL and App) and the fuses. Avrdude with only the 'flash' command will skip the fuses: we need to also use the fuses command / option / section.

3. Test LED out. Note: the CNano's on-board LED is on PF2, which is a matrix column — not
   suitable for this test. Do on the real Citrouille90 PCB.


## Nice to have:
- Nothing for the moment

## Keep for phase 2:

1. Stale Report on USB Resume (Phantom Keypresses)
   When the device resumes from USB suspend, the current HID report buffers still hold
   whatever state was staged before suspend. If a key was held at suspend time the host will
   see it as newly pressed on resume, producing a phantom keypress. The fix is to zero both
   s_kbd_buf and s_con_buf (and bump their seqlocks) inside usb_event_wakeup() in usb_hid.c,
   and to call hid_flush() immediately after to push the zero report before the first SOF
   arrives. This is a small, targeted fix but depends on the SOF race being resolved first,
   so it naturally slots in second.

2. Time TCB0_INT_vect to 200us prior to expected SOF arrival (800 us after prior SOF arrival)

3. DFU Bootloader Migration
   The current vendor-class bootloader is functional but requires a custom host-side flashing
   tool. Migrating to DFU (Device Firmware Upgrade, USB class 0xFE) would allow flashing
   with standard tools (dfu-util, Microchip's FLIP, or avrdude's DFU backend), which
   simplifies the developer workflow and eliminates the LibUSB dependency in KeyDU.Flasher.
   The AVR64DU32 has documented DFU support. This is the largest rev 2 change — it touches
   KeyDU.BL entirely and requires re-qualifying the flash protocol — so it belongs after the
   application firmware is fully stable.
