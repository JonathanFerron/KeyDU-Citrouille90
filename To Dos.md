# Citrouille90 To Dos

To be addressed:

1. Compiler and Linker Flag Optimization

2. bear and astyle Make Targets

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

1. hid_flush()-in-SOF-ISR vs EP0 Control Transfer Race
   This is the most serious correctness issue in the codebase. hid_flush() is called from usb_event_sof(), which runs inside USB0_BUSEVENT_vect. Meanwhile, 
   usb_event_ctrl_request() — which handles HID_REQ_GET_REPORT and reads directly from s_kbd_buf/s_con_buf — runs in the main loop via usb_ctrl_poll(). The seqlock in 
   hid_kbd_stage() protects against a torn read between the scan loop and the SOF ISR, but it does not protect against ep_write_ctrl_stream(&s_kbd_buf, ...) in the main 
   loop executing concurrently with memcpy(&snap, &s_kbd_buf, ...) inside the SOF ISR. The fix requires either moving GET_REPORT handling into the ISR context, or briefly 
   disabling the SOF interrupt around the control transfer read. This should be the first rev 2 item because it's a latent data corruption path that could cause intermittent, 
   hard-to-reproduce host-side misbehavior.

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
