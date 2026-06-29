# Citrouille90 To Dos

## To be addressed urgently:
- Nothing for the moment

## Next steps:

1. Implement a flash_fuses target in the makefile (for use with the UPDI Friend on the real
   Citrouille90 PCB — the CNano mass-storage path already programs fuses automatically).

   Exact diff to apply:

   .PHONY line — add flash_fuses:
     - flash_usb flasher bear format help merged
     + flash_fuses flash_usb flasher bear format help merged

   After the flash_bl target, add:
     # UPDI fuses-only — program the fuse row from the BL hex. fuses.c emits the
     # FUSES struct to the .fuse section at 0x820000 in KeyDU.BL.elf; avrdude 8.x
     # recognises the multi-memory hex and writes only the fuse segment. Run once
     # at bring-up and after any fuses.c change. No -D needed: a fuses-only write
     # performs no flash erase. (Requires avrdude >= 8.0 for multi-memory hex.)
     flash_fuses: $(BL_HEX)
         @echo "Flashing fuse row via UPDI..."
         $(AVRDUDE) -c $(PROG_ID) -p $(MCU) -P $(PROG_PORT) -U fuses:w:$<:i

   In the header comment block, after the flash_bl line:
     + #   make flash_fuses   — program fuse row via UPDI (from BL hex)

   In the help target echo block, after flash_bl:
     + @echo "  flash_fuses  Program fuse row only via UPDI (from BL hex)"

   Notes:
   - Sources fuses from $(BL_HEX), not $(MERGED_HEX) — leaner, and flash_fuses
     can run after just make bl without building the App.
   - -D intentionally omitted: it suppresses erase before flash writes; a
     fuse-only -U touches only the fuse row and needs no erase flag.
   - To verify after writing: -U fuses:v:$(BL_HEX):i reads the row back and
     diffs against the hex.
   - Fuse bytes confirmed correct in the current build (verified June 2026):
     820000–82000B in the merged hex, decodes to WDTCFG=0x00, BODCFG=0x00,
     OSCCFG=0x00, SYSCFG0=0xD8, SYSCFG1=0x08, CODESIZE=0x00, BOOTSIZE=0x10,
     PDICFG=0x0003 — matches fuses.c and the cheatsheet KeyDU column exactly.

2. Extend consumer/system keycodes — add the following missing codes to keycode.h and
   keyboard.c, then assign them to keys in keymap.c as desired.

   A) Consumer page (0x0C) — add CC_ defines to keycode.h and corresponding entries to
      cc_usage_table[] in keyboard.c (PROGMEM, format: { cc_low_byte, hid_usage_16bit }):

      #define CC_MSEL  (CC_BASE | 0x08)   // 0x0C:0x0183  Media Select
      #define CC_MAIL  (CC_BASE | 0x09)   // 0x0C:0x018A  Launch Mail
      #define CC_CALC  (CC_BASE | 0x0A)   // 0x0C:0x0192  Launch Calculator (App2)
      #define CC_MYPC  (CC_BASE | 0x0B)   // 0x0C:0x0194  Launch My Computer (App1)
      #define CC_WSCH  (CC_BASE | 0x0C)   // 0x0C:0x0221  WWW Search
      #define CC_WHOM  (CC_BASE | 0x0D)   // 0x0C:0x0223  WWW Home
      #define CC_WBAK  (CC_BASE | 0x0E)   // 0x0C:0x0224  WWW Back
      #define CC_WFWD  (CC_BASE | 0x0F)   // 0x0C:0x0225  WWW Forward
      #define CC_WSTP  (CC_BASE | 0x10)   // 0x0C:0x0226  WWW Stop
      #define CC_WRFR  (CC_BASE | 0x11)   // 0x0C:0x0227  WWW Refresh
      #define CC_WFAV  (CC_BASE | 0x12)   // 0x0C:0x022A  WWW Favorites

      cc_usage_table entries to add (after the existing CC_MSTP row):
        { 0x08, 0x0183 },   /* CC_MSEL — Media Select         */
        { 0x09, 0x018A },   /* CC_MAIL — Launch Mail          */
        { 0x0A, 0x0192 },   /* CC_CALC — Launch Calculator    */
        { 0x0B, 0x0194 },   /* CC_MYPC — My Computer          */
        { 0x0C, 0x0221 },   /* CC_WSCH — WWW Search           */
        { 0x0D, 0x0223 },   /* CC_WHOM — WWW Home             */
        { 0x0E, 0x0224 },   /* CC_WBAK — WWW Back             */
        { 0x0F, 0x0225 },   /* CC_WFWD — WWW Forward          */
        { 0x10, 0x0226 },   /* CC_WSTP — WWW Stop             */
        { 0x11, 0x0227 },   /* CC_WRFR — WWW Refresh          */
        { 0x12, 0x022A },   /* CC_WFAV — WWW Favorites        */

   B) System Sleep (GD page 0x01, usage 0x82) — special case, NOT a consumer code:
      The hid_consumer_report_t struct already has a system byte with bit0 = System Sleep
      (already in the HID descriptor). The infrastructure exists but no keycode drives it.

      Add to keycode.h (SYS_ range):
        #define SYS_SLEEP  (SYS_BASE | 0x03)

      Add to the SYS_ dispatch in keyboard.c process_key_press() / process_key_release():
        case SYS_SLEEP:
          s_con_report.system |= 0x01;   // press: assert sleep bit
          hid_consumer_stage(&s_con_report);
          break;
      And in process_key_release():
        case SYS_SLEEP:
          s_con_report.system &= ~0x01;  // release: clear sleep bit
          hid_consumer_stage(&s_con_report);
          break;
      Note: s_con_report is static in keyboard.c and is not exposed directly — the
      dispatch will need to either use the existing kbd_consumer_set/clear pattern or
      add a dedicated kbd_system_set/clear pair similar to kbd_consumer_set().

3. Replace MACRO_ACTION_WAIT busy-wait in macro.c with kbd_stage_wait()
   Infrastructure is ready: kbd_stage_wait() is declared in usb_hid.h and implemented in usb_hid.c.
   Only the MACRO_ACTION_WAIT case body in run_macro_sequence() needs updating — replace the
   spin loop with kbd_stage_wait(action.keycode).

4. Test LED out. Wire external LEDs with resistors on the CNano breadboard (CNano's on-board
   LED on PF2 is a matrix column — use separate LEDs on the appropriate TCA0 WO pins instead).

5. Sleep mode — enable AVR IDLE sleep in the main loop. The structure is already staged:
   - app_main.c lines 62–63: sleep_enable() + set_sleep_mode(SLEEP_MODE_IDLE)
   - app_main.c line 99: sleep_cpu() call (commented out)
   Uncomment and verify: MCU should sleep between ticks and wake cleanly on TCB0, USB SOF,
   and USB bus events. Untested.

6. Stale Report on USB Resume (Phantom Keypresses)
   When the device resumes from USB suspend, the current HID report buffers still hold
   whatever state was staged before suspend. If a key was held at suspend time the host will
   see it as newly pressed on resume, producing a phantom keypress. The fix is to flush the
   kbd_queue (reset head/tail to 0) and zero s_con_buf (bumping s_con_seq) inside
   usb_event_wakeup() in usb_hid.c, then call hid_flush() immediately to push a clean zero
   report before the first real report arrives.

7. Get flashing to work from KeyDU.BL using the vendor protocol and custom host side program (libusb supported)

8. Time TCB0_INT_vect to 200us prior to expected SOF arrival (800 us after prior SOF arrival)


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
