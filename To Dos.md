# Citrouille90 To Dos

## Outstanding Firmware Issues — Implementation Order

### USB Protocol Correctness

**#14 — ZLP check uses wrong EP bank size in `usb_ep_stream.c`**
`length % EP_MAX_SIZE` uses the hardcoded constant 64, which is wrong for the control endpoint's 8-byte bank. Change the divisor to the actual control EP bank size. While here, verify that the wLength clamping at the top of the function doesn't cause a spurious ZLP when `rem` was already shortened.

**#12 — Serial descriptor path missing status stage in `usb_ctrl.c`**
`send_internal_serial()` ends with `ep_clear_out()` and returns without completing the status stage. Audit every handled request path through `usb_process_ctrl_request()` together and ensure each one reaches `ep_complete_ctrl_status()`.

**#11 — Address spin loop lacks disconnect guard in `usb_ctrl.c`**
`ctrl_set_address()` spins on `while (!ep_in_ready())` with no escape. Add a `usb_device_state == USB_STATE_UNATTACHED` check inside the loop, consistent with the guards used elsewhere in that file.

---

### Runtime Logic Bugs

**#13 — Encoder bypasses seqlock in `keymap.c`**
`encoder_step()` writes directly to `keyboard_report.mods` rather than staging through `hid_kbd_stage()`. This creates a potential partial-update race if `hid_flush()` fires mid-step. Rework the encoder path to build a full `hid_kbd_report_t` and route it through the same staging mechanism as key events. Decide at the same time whether to give `keymap.c` a read-back of the current staged report or maintain a local shadow for the modifier state.

**#7 — `send_mod_key()` writes `keys[0]` directly in `macro.c`**
Bypasses `add_key_to_report()` and clobbers slot 0 if any other key is held. Replace the direct struct write with `add_key_to_report()` / `remove_key_from_report()` calls, consistent with all other key event handling.

---

### Housekeeping (last, low risk)

**#19 — `ep_configure_prv()` always returns `true` in `usb_ep.c`**
The return value is meaningless — every code path returns `true`. Either add real validation and propagate errors to the caller, or change the return type to `void` and update the call site.
