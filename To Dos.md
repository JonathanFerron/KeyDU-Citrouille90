# Citrouille90 To Dos

## To be addressed urgently:
- Nothing for the moment

## Next steps:

1. Replace MACRO_ACTION_WAIT busy-wait in macro.c with kbd_stage_wait()
   Infrastructure is ready: kbd_stage_wait() is declared in usb_hid.h and implemented in usb_hid.c.
   Only the MACRO_ACTION_WAIT case body in run_macro_sequence() needs updating — replace the
   spin loop with kbd_stage_wait(action.keycode).

2. Test LED out. Wire external LEDs with resistors on the CNano breadboard (CNano's on-board
   LED on PF2 is a matrix column — use separate LEDs on the appropriate TCA0 WO pins instead).

3. Sleep mode — enable AVR IDLE sleep in the main loop. The structure is already staged:
   - app_main.c lines 62–63: sleep_enable() + set_sleep_mode(SLEEP_MODE_IDLE)
   - app_main.c line 99: sleep_cpu() call (commented out)
   Uncomment and verify: MCU should sleep between ticks and wake cleanly on TCB0, USB SOF,
   and USB bus events. Untested.

4. Stale Report on USB Resume (Phantom Keypresses)
   When the device resumes from USB suspend, the current HID report buffers still hold
   whatever state was staged before suspend. If a key was held at suspend time the host will
   see it as newly pressed on resume, producing a phantom keypress. The fix is to flush the
   kbd_queue (reset head/tail to 0) and zero s_con_buf (bumping s_con_seq) inside
   usb_event_wakeup() in usb_hid.c, then call hid_flush() immediately to push a clean zero
   report before the first real report arrives.

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
