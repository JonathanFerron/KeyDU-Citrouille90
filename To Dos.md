# Citrouille90 To Dos

- Fix the following issues:

## Outstanding Firmware Issues — Implementation Order

The grouping logic: fix what prevents the build first, then what breaks USB enumeration, then runtime logic bugs.

---

### Build Blockers (fix before anything compiles)

**#20 — `usb_init()` argument mismatch**
`usb_ctrl.c` declares `void usb_init(uint8_t options)` but `keyboard.c` calls `usb_init()` with no argument. Add `USB_OPT_VREG_ENABLE` (or whatever the appropriate flag is) to the call site in `keyboard.c`.

**#6 — Missing `led_brightness_increase()` / `led_brightness_decrease()`**
`keyboard.c` calls these two functions; `led.h` only exports `led_step(bool dir, uint8_t step)`. Change the two call sites in `keyboard.c` to `led_step(true, LED_BRIGHTNESS_STEP)` and `led_step(false, LED_BRIGHTNESS_STEP)`.

**#5 — `led_update_for_layer()` name mismatch**
`keyboard.c` calls `led_update_for_layer(current_layer)`; the function defined in `led.c` is `led_update_layer(uint8_t layer)`. Rename one to match the other — `led_update_layer()` is the more consistent name.

---

### Silent Hardware Failures (build succeeds, device doesn't work)

**#3 / #4 — `clock_init()` never called; `MCLKTIMEBASE` never set**
These two are one fix: call `clock_init()` as the very first statement in `system_init()` in `main.c`, before TCB0 or anything else is configured. Without it, `CLKCTRL.MCLKTIMEBASE` is never written (USB will malfunction) and the TCB0 `CCMP` value is computed against the wrong clock frequency.

---

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
