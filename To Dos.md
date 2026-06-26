# Citrouille90 To Dos

## Next steps:

1. Test KeyDU.BL USB enumeration in isolation (forced BL entry in bl_main.c).
   Confirm `lsusb` shows 1209:b4b1 Citrouille90 Bootloader + clean dmesg. Proves the
   vendor descriptors + EP0-only setup on the already-proven usbcore. No host tooling
   needed. Canary for step 2.
   
```
   In bl_main.c, could use the following bit of code to help test the above:
   
      bool soft = (rstfr & RSTCTRL_SWRF_bm) != 0u;
      bool magic_valid = (magic == BOOT_MAGIC);
   
   +  /* TEST ONLY — revert: force BL entry to test USB enumeration in isolation,
   +     bypassing the soft-reset + magic boot decision. IVSEL still set below. */
   +  soft = true; magic_valid = true;
   +
      if(soft && magic_valid)
      { ccp_write_ioreg((void*)&CPUINT.CTRLA, CPUINT_IVSEL_bm);  // sets interrupt vector to bootcode
      usb_vendor_init();   /* clock, USB hardware, state init, sei() */
      usb_vendor_task();   /* bare loop — never returns */
      }
```

2. Test the EEPROM bootloader magic-flag integration on hardware. Code is implemented
   in keyboard.c (SYS_BOOT handler writes the magic) and bl_main.c (reads RSTFR for
   SWRF + checks the EEPROM byte), but the full app→reset→bootloader-entry path has not
   yet been exercised on the CNano. Confirms the GPR2/GPR3 → EEPROM migration actually
   lands you in the BL.

3. Test LED out. Note: the CNano's on-board LED is on PF2, which is a matrix column — not
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
