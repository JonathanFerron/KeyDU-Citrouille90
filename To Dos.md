# Citrouille90 To Dos

To be addressed:
1. Partition Boundary Documentation
The flash layout comment in Firmware reference.md section 14.4 is correct but thin. There's no single place that captures the full picture: fuse values, linker script origins, 
APP_START, the FLMAP boundary at 0x8000, and how the merged hex is assembled. This isn't a code change — it's adding a doc/flash_layout.md or a header block in the Makefile 
that makes the layout self-documenting, including how to verify with avr-objdump and srec_info. The payoff is that any future work on the bootloader (especially rev 2's DFU 
migration) starts from a clear, verified baseline rather than reconstructing the layout from scattered comments.
Suggested order: second, after EEPROM. Pure documentation, zero risk, and it forces you to verify the merged hex actually matches the layout before moving on to anything more complex.

2. Compiler and Linker Flag Optimization
The Makefile currently uses -Os globally. There are several targeted improvements worth evaluating: -flto (link-time optimization, requires avr-gcc 4.9+ and avr-ar — saves 
meaningful flash on small targets), -fno-exceptions and -fno-unwind-tables (already implicit in C but worth being explicit), and explicit -mrelax in CFLAGS to match the 
--relax already in LDFLAGS. The BL target in particular has headroom pressure — verifying .text size after each flag change with avr-size is the discipline here. Add a bear 
target for compile_commands.json generation (useful for clangd/kate LSP) and an astyle target using the existing .astylerc at the same time, since both are one-line Makefile 
additions.
Suggested order: third. Changing flags can perturb object layout and expose latent issues; do this before finalizing the keymap, not after.

3. Compose / Host-Side CMS Keyboard Layout (compose.h/.c)
Currently the firmware sends raw HID keycodes. The plan is to add a compose layer that maps logical characters to the correct HID sequences for a Canadian Multilingual 
Standard (CMS) layout configured on the host. This means encoding the dead-key sequences and shifted variants that CMS defines, likely as a small PROGMEM lookup table keyed 
by logical keycode. This is the most design-intensive rev 1 item — it needs a clear decision on scope (which characters, which compose sequences, how to handle OS differences) 
before any code is written. Keep the interface clean: compose.c should expose a single compose_key(uint16_t logical_keycode) entry point that keyboard.c calls instead of going 
directly to kbd_add_key().
Suggested order: last in rev 1. It has the most design ambiguity and the least urgency for basic functionality. Do it after everything else is stable and tested.

4. bear and astyle Make Targets
bear make generates compile_commands.json for clangd, which enables accurate LSP completion and diagnostics in Kate, Geany, and VSCode across all three dev machines. 
astyle --options=.astylerc run as a Make target enforces the coding style documented in section 2.

Nice to have (style issue):
- Look into fixing the following inconsistency:
    layer_key_released() is called unconditionally on every key release in keyboard.c
    In process_key_release(), layer_key_released(row, col) is called for every released key — including non-layer keys. 
    layer_key_released() will simply fail to find the entry and return harmlessly, but it's a style inconsistency: 
      process_key_press() only calls layer_key_pressed() when IS_LAYER_KEY(keycode) is true. The release path should mirror this, 
      which requires either checking the locked keycode first, or checking IS_LAYER_KEY before calling the layer function. 
      Since layer keys are not tracked in s_pressed_keys, their keycode won't be in keycode_out after untrack_pressed_key — 
      the current code calls layer_key_released before untracking, which is correct for layer keys, but the unconditional call 
      is still inconsistent with the press path.
      Beyond style, there's a subtle correctness edge case: if a layer key is released while the layer table is full 
      (active_layer_key_count == MAX_LAYER_KEYS) and the key wasn't tracked (because it was dropped on press), layer_key_released harmlessly 
      no-ops — but it still iterates the full array every release. Matching the press-path pattern here is the right fix.

Keep for phase 2:
1. Time TCB0_INT_vect to 200us prior to expected SOF arrival (800 us after prior SOF arrival)

2. hid_flush()-in-SOF-ISR vs EP0 Control Transfer Race
This is the most serious correctness issue in the codebase. hid_flush() is called from usb_event_sof(), which runs inside USB0_BUSEVENT_vect. Meanwhile, 
usb_event_ctrl_request() — which handles HID_REQ_GET_REPORT and reads directly from s_kbd_buf/s_con_buf — runs in the main loop via usb_ctrl_poll(). The seqlock in 
hid_kbd_stage() protects against a torn read between the scan loop and the SOF ISR, but it does not protect against ep_write_ctrl_stream(&s_kbd_buf, ...) in the main 
loop executing concurrently with memcpy(&snap, &s_kbd_buf, ...) inside the SOF ISR. The fix requires either moving GET_REPORT handling into the ISR context, or briefly 
disabling the SOF interrupt around the control transfer read. This should be the first rev 2 item because it's a latent data corruption path that could cause intermittent, 
hard-to-reproduce host-side misbehavior.

3. Stale Report on USB Resume (Phantom Keypresses)
When the device resumes from USB suspend, the current HID report buffers still hold whatever state was staged before suspend. If a key was held at suspend time the host 
will see it as newly pressed on resume, producing a phantom keypress. The fix is to zero both s_kbd_buf and s_con_buf (and bump their seqlocks) inside usb_event_wakeup() 
in usb_hid.c, and to call hid_flush() immediately after to push the zero report before the first SOF arrives. This is a small, targeted fix but depends on the SOF race 
being resolved first, so it naturally slots in second.

4. DFU Bootloader Migration
The current vendor-class bootloader is functional but requires a custom host-side flashing tool. Migrating to DFU (Device Firmware Upgrade, USB class 0xFE) would allow 
flashing with standard tools (dfu-util, Microchip's FLIP, or avrdude's DFU backend), which simplifies the developer workflow and eliminates the LibUSB dependency in 
KeyDU.Flasher. The AVR64DU32 has documented DFU support. This is the largest rev 2 change — it touches KeyDU.BL entirely and requires re-qualifying the flash protocol — 
so it belongs after the application firmware is fully stable.

5. Enhanced Documentation
By this point the firmware will have gone through two significant revision cycles. A documentation pass to bring Firmware reference.md current — particularly sections 
11 (USB stack remarks, currently sparse), 12 (updated with any DFU changes), and 18 (open items, now mostly closed) — and to add inline /* rationale */ comments to any 
modules that changed significantly in rev 2. Also worth adding a CHANGELOG.md at this stage to capture what changed between rev 1 and rev 2 for your own future reference.



