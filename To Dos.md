# Citrouille90 To Dos

## Next steps:

**1. Prove the data path end to end — get a real keypress through.** Enumeration only exercises EP0. Re-enable a minimal `keyboard_task()` (or just hardcode staging one keycode on a timer), then confirm a press actually lands with `evtest` on the input device or by typing into a terminal. This is where you'll flush out the EP1 IN data-toggle and `ep_in_ready()` behavior on a non-control endpoint, plus the LED OUT path. Do this *before* anything else, because almost every later feature rides on this working. Tackle the SOF-ISR vs main-loop `ep_select()` contention here too, since re-enabling the scan loop is exactly what makes that race live — bracket `hid_flush()` with `usb_sof_disable()`/`usb_sof_enable()` or move it off the ISR as you'd planned.

**2. Burn down the post-enumeration correctness bugs you already logged.** You have a clean list from before: `usb_sof_enable()` wiring, `usb_vendor_task()` calling the wrong function, the `process_key_release()` guard, and the EEPROM bootloader magic-flag integration. These are known, scoped, and cheap now that the stack is alive. Knocking them out while the USB layer is fresh in your head is far easier than rediscovering them in three months.

**3. Lock in a regression baseline before you build features on top.** Tag this commit (something like `v0.1-enumeration`), and write down — in your `Hardware_notebook.md` or a `BUGS_FIXED.md` — the three root causes from this session, especially the `EPPTR = &endpoints[0]` offset and the FIFO-lives-before-EPPTR rule. That detail is invisible in the datasheet's prose and you will absolutely need it again for KeyDU v2's six-endpoint layout. A tagged known-good point means that if v2's endpoint expansion breaks enumeration, you can `git diff` against a state you trust instead of re-debugging from scratch.

On Claude Code: it's well suited to steps 2 and 3 — multi-file edits across the `usbcore`/`usbhid`/`bootloader` modules, running your `make` build, and grepping the codebase for the logged bugs. It's less useful for step 1, where the bottleneck is hardware observation (`evtest`, `dmesg`, LED blinks, maybe Bloom/a logic analyzer) rather than code generation. If you do set it up, point it at your existing conventions — snake_case, static module state, no return codes from hardware drivers, the SRAM/flash budgets — so its edits match the codebase rather than drifting.



## To be addressed:

## Nice to have (style issue):

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

## Keep for phase 2:

1. hid_flush()-in-SOF-ISR vs EP0 Control Transfer Race
   hid_flush() is called from usb_event_sof(), which runs inside USB0_BUSEVENT_vect. Meanwhile, usb_ctrl_poll() runs in the main loop and may be executing a control transfer — including ep_write_ctrl_stream() in usb_event_ctrl_request() — at the same moment the SOF ISR fires. Both paths call ep_select() and manipulate the global usb_ep_selected, usb_ep_handle, and usb_ep_fifo pointers. A SOF interrupt mid-control-transfer will clobber those globals, corrupting the in-progress EP0 transfer.
   
    The specific s_kbd_buf reference that triggered a compile error has been removed — HID_REQ_GET_REPORT for the keyboard interface now snapshots via kbd_get_report(), which is safe because s_kbd_report is written exclusively in main-loop context. The s_con_buf read in the consumer branch of HID_REQ_GET_REPORT retains the original exposure.
   
    The correct fix is to move hid_flush() out of the SOF ISR and into the main loop, called unconditionally before usb_ctrl_poll(). The SOF ISR would then only set a flag. This eliminates the re-entrancy entirely. The seqlock on the consumer report and the SPSC queue for the keyboard report were designed with this main-loop flush model in mind — neither requires ISR context.

2. Stale Report on USB Resume (Phantom Keypresses)
   When the device resumes from USB suspend, the current HID report buffers still hold whatever state was staged before suspend. If a key was held at suspend time the host will see it as newly pressed on resume, producing a phantom keypress. The fix is to zero both s_kbd_buf and s_con_buf (and bump their seqlocks) inside usb_event_wakeup() in usb_hid.c, and to call hid_flush() immediately after to push the zero report before the first SOF arrives. This is a small, targeted fix but depends on the SOF race being resolved first, so it naturally slots in second.

3. Enhanced Documentation
   A documentation pass to bring Firmware reference.md current — particularly sections 
   11 (USB stack remarks, currently sparse), 12 (updated with any DFU changes), and 18 (open items, now mostly closed) — and to add inline /* rationale */ comments to any 
   modules that changed significantly in rev 2. Also worth adding a CHANGELOG.md at this stage to capture what changed between rev 1 and rev 2 for your own future reference.

4. Time TCB0_INT_vect to 200us prior to expected SOF arrival (800 us after prior SOF arrival)

5. DFU Bootloader Migration
   The current vendor-class bootloader is functional but requires a custom host-side flashing tool. Migrating to DFU (Device Firmware Upgrade, USB class 0xFE) would allow 
   flashing with standard tools (dfu-util, Microchip's FLIP, or avrdude's DFU backend), which simplifies the developer workflow and eliminates the LibUSB dependency in 
   KeyDU.Flasher. The AVR64DU32 has documented DFU support. This is the largest rev 2 change — it touches KeyDU.BL entirely and requires re-qualifying the flash protocol — so it belongs after the application firmware is fully stable.
