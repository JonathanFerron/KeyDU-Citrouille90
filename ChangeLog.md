# Citrouille90 — ChangeLog

## 13. Stale HID report on USB resume fixed (July 2026)

`keyboard_task()` runs unconditionally off the TCB0 1 ms tick regardless of `usb_device_state`,
so matrix scanning and `kbd_stage()` kept enqueueing reports into `kbd_queue` while the bus was
suspended. `hid_flush()` bails out early whenever `usb_device_state != USB_STATE_CONFIGURED`,
so none of that backlog drained during suspend. On resume, `hid_flush()` would start draining
the queue from the head, sending stale pre-suspend key state to the host as if it had just
happened — a phantom keypress if the physical key state had since changed.

Fixed by adding `usb_event_wakeup()` (overrides the weak stub in `usb_ctrl.c`) in `usb_hid.c`.
It only sets a `volatile bool s_wakeup_pending` flag — `usb_event_wakeup()` runs inside
`USB0_BUSEVENT_vect`, the same ISR `usb_event_sof()` documents as unsafe for `hid_flush()`
(`ep_select()` can clobber an in-flight EP0 control transfer), and `kbd_queue_head`/`tail` are
single-writer by design (head: `hid_flush()`; tail: `kbd_stage()`), so resetting them directly
from the ISR could tear a `flush_kbd()` call already in progress on the main loop. The new
`handle_wakeup()`, called from `hid_flush()` on the main loop, does the actual work when the
flag is set: resets `kbd_queue` to empty, enqueues one zeroed `KBD_QUEUE_REPORT` so it is the
very next thing sent, and clears `s_con_buf` through its existing seqlock (bump odd, memset,
restore `report_id`, bump even) so `flush_consumer()` sends a clean report too. This guarantees
the host's first post-resume report is a known-good all-zero baseline rather than whatever was
staged before or during suspend.

Build verified clean via `make merged` (App: 7924 B flash / 1392 B data), no new warnings.
Not yet verified against an actual USB suspend/resume cycle on hardware — the CNano/host
selective-suspend test is still pending.

(Unrelated to this fix: this commit also carries `make format`/astyle output on several files
with pre-existing style drift — `app_main.c`, `keyboard.c`, `led.c`, `bl_main.c`, `ccp.h`,
`usb_ctrl.c`, `usb_ep.c`, `usb_ep_stream.c` — no logic changes in those files.)

## 12. MCU IDLE sleep re-enabled in App main loop (July 2026)

Uncommented the staged `set_sleep_mode(SLEEP_MODE_IDLE)` + `sleep_enable()` (app_main.c,
before the main loop) and `sleep_cpu()` (bottom of `while(1)`, after the tick-gated block).
This was left commented out pending USB enumeration and HID I/O being proven — both now
work (device→host keyboard reports, host→device status LEDs), so the sleep path was
re-enabled and verified.

IDLE sleep only stops the CPU core clock; all peripheral clocks (USB, TCB0, TCA0) keep
running, so USB enumeration, HID traffic, and PWM are unaffected — this is CPU idle
between interrupts, not USB suspend. Wake sources are TCB0's 1 ms tick interrupt
(unconditionally enabled, guarantees a wake within 1 ms so the CPU can never sleep
indefinitely), USB bus events (enabled unconditionally), and USB SOF (enabled once
configured). All are LVL0 interrupts, which wake the core from IDLE.

Build verified clean via `make merged` (App: 7828 B flash / 1392 B data). Verified on
the CNano breadboard: clean enumeration, k00/k01 test switches and status LEDs both
still working, and correct wake after 30 s idle (blue k01 switch registered KC_1 on
first press with no dropped or phantom keystroke). Current-draw measurement
intentionally skipped (would require an inline ammeter on the CNano's USB 5V line).

## 11. MACRO_ACTION_WAIT busy-wait replaced with kbd_stage_wait() (July 2026)

The MACRO_ACTION_WAIT case in run_macro_sequence() (macro.c) used a busy-wait spin loop
(~1 ms per count at 24 MHz) to implement macro delays. Replaced with a call to
kbd_stage_wait(action.keycode), which enqueues a KBD_QUEUE_WAIT sentinel into the existing
non-blocking HID report queue (usb_hid.c/.h) — the delay is now realised later as
hid_flush() drains the queue, one SOF tick per count, instead of blocking the scan loop.

Note: this path is currently dead code. run_macro_sequence() is still
`__attribute__((unused))`, and its only callers (the MC_XL_* cases in execute_macro())
are commented out, with no keymap entries referencing them. The fix is correct and in
place, but not yet exercised at runtime until a sequence macro is wired into the keymap.

## 10. LED brightness control — verified on CNano breadboard (June 2026)

LD_BRIU / LD_BRID mapped to layer-0 k00/k01 (L11×R3, L11×R4) for testing.
External LEDs (385 Ω / 383 Ω, targeting 7.5 mA) on PA4 (R9) and PA5 (R8).
led_step() confirmed: 4 presses down from default 64 reach 0 (off), steps
of 16 each. LED_BRIGHTNESS_DEFAULT changed from 128 to 64 (25% duty cycle).
Keymap reverted after test.

Also fixed: makefile lacked header dependency tracking — led.o was not
recompiled when led.h changed (LED_BRIGHTNESS_DEFAULT appeared stuck at 128).
Added -MMD -MP to APP_CFLAGS and BL_CFLAGS; added -include of generated .d
files. Future header changes now automatically trigger correct recompilation.

## 9. LED output report — implemented and verified on CNano breadboard (June 2026)

led_apply_host_report() added to led.c: reads the host LED report from
hid_get_led_report() and overrides LED A (Num Lock, PA4/R9) or LED B
(Caps Lock, PA5/R8) to full brightness on top of normal layer feedback.
Called once per 1 kHz tick in keyboard_task() after led_update_layer().

Tested with external LEDs (yellow on R9, white on R8) and KC_CAPS /
KC_NLCK mapped to k00/k01 (L11×R3, L11×R4). xset q and visible LED
brightness change both confirmed for each lock key, individually and
in combination. Both SET_REPORT (EP0) and EP1 OUT paths share the same
s_led_report store — both implicitly verified.

## 8. Consumer report pipeline — verified on CNano breadboard (June 2026)

CC_VOLU and CC_VOLD temporarily mapped to layer-0 k00/k01 (ESC/1 positions,
elec row 0 col 8/7, breadboard L11×R3 and L11×R4). Tested with two push buttons.
Linux volume changed immediately on press and released on release. Confirms:
Interface 1 descriptor, EP2 IN data path, cc_to_hid_usage() PROGMEM lookup,
and the Linux HID driver all working end-to-end. Keymap reverted after test
(CC_VOLU/CC_VOLD remain on layer 1 at k14/k15 = R/T keys).

## 1. hid_flush()-in-SOF-ISR vs EP0 Control Transfer Race

hid_flush() was called from usb_event_sof(), which runs inside USB0_BUSEVENT_vect.
Meanwhile, usb_ctrl_poll() runs in the main loop and may be executing a control transfer at
the same moment the SOF ISR fires. Both paths call ep_select() and manipulate the global
usb_ep_selected, usb_ep_handle, and usb_ep_fifo pointers. A SOF interrupt mid-control-
transfer would clobber those globals, corrupting the in-progress EP0 transfer.

Fixed: hid_flush() moved from usb_event_sof() (SOF ISR) to the main loop, called ungated
before usb_ctrl_poll(). The SOF ISR now only sets a flag. Root cause was ep_select() in the
SOF ISR clobbering the EP0 control-transfer globals during the HID-probe window (SOF goes
live at SET_CONFIGURATION via usb_sof_enable()), corrupting GET_DESCRIPTOR(REPORT)/SET_IDLE
/SET_PROTOCOL and causing intermittent multi-second enumeration stalls and EP1 STALLs.
Verified stable across device #54–62, both replug and reset.

## 2. process_key_release() layer-key guard

process_key_release() was calling layer_key_released(row, col) unconditionally before
untracking, then checking the keycode. This was inconsistent with process_key_press(), which
guards layer_key_pressed() with IS_LAYER_KEY. Layer keys are not stored in s_pressed_keys,
so untrack_pressed_key() returns KC_NO for them — the unconditional call was harmless but
incorrect in structure, and caused unnecessary iteration on every non-layer release.

Fixed: untrack_pressed_key() runs first. If keycode == KC_NO (nothing tracked — likely a
layer key), layer_key_released() is called as a no-op-safe fallback. Otherwise the keycode
drives the consumer/basic release branch. Mirrors the press path exactly.

## 3. Live matrix scan — PORTD-column VPORT collision

PORTD columns (PD5–PD7, cols 6–8) never registered a keypress. PORTF columns (PF0–PF5,
cols 0–5) worked correctly. Root cause: scan_col_raw() drove a column low via
PORTD.OUTCLR (regular PORT register write), waited one NOP, then read VPORTD.IN (VPORT
register read of the same port). This is the PORT→VPORT same-port collision hazard: a
regular PORT write followed immediately by a VPORT read of the same port can return stale
data. PORTF columns escaped because they write PORTF and read VPORTD — different ports.

Diagnosed via paired probes in keyboard_task(): a 'w' probe reading PORTD.IN (regular)
fired immediately on shorting PD2+PD6; a 'z' probe reading VPORTD.IN fired only
sporadically (collision noise making the read oscillate randomly). 0/200 vs 2/30 firing
rate asymmetry confirmed the collision statistically.

Fixed: rows_d now reads PORTD.IN (regular register). VPORTA.IN is kept for PORTA rows
since PORTA is never driven as a column and has no collision hazard.

Verified on Curiosity Nano: clean keypresses with correct OS key-repeat on PORTD cols
(PD2+PD6 → 'a') and PORTF cols (PF5+PD0 → '3', PF4+PD0 → '4'). Rapid taps register
cleanly with no misses or doubles. Commit: 7ca8281.

## 4. End-to-end USB HID pipeline — get a real keypress through

keyboard_init() and keyboard_task() were commented out during USB enumeration debugging.
Re-enabled the full scan path; confirmed the EP1 IN data-toggle, ep_in_ready() behavior,
and the HID report queue all work correctly under real keystroke traffic.

## 5. usb_sof_enable() wiring — keep enabled for planned phase-lock

usb_sof_enable() is called in usb_event_config_changed() (usb_hid.c), so the SOF
interrupt fires every 1 ms once configured — but usb_event_sof() is now empty
(hid_flush() lives in the main loop after the EP0-race fix, see Done #1). This left
an open question: remove the enable as a dead-handler cleanup, or keep it.

Resolved: keep usb_sof_enable() enabled. usb_event_sof() is the intended integration
point for the planned TCB0/SOF phase-lock (snapshot TCB0.CNT across several frames,
nudge TCB0 to fire ~200 us before the next SOF). The empty handler is expected, not
leftover. Two guard rails documented in the usb_event_sof() comment: (a) do NOT remove
usb_sof_enable() as a cleanup — it is load-bearing for the future feature; (b) do NOT
move hid_flush() back into this ISR — ep_select() here clobbers the EP0 control-transfer
globals mid-flight, which was the enumeration race fixed in Done #1. Future-caveat noted:
if USB_OPT_BUS_INT_HIGH is ever set, the TCB0 ISR can be masked during the CNT read
window.

The companion To Do item "usb_vendor_task() calling the wrong function" was found already
resolved — usb_vendor.c calls usb_ctrl_poll() correctly and no usb_task() reference exists
in the tree. Both items removed from To Dos.md "Next steps".

## 6. KeyDU.BL USB enumeration — IVSEL cleared by clock_init()

The bootloader's USB never enumerated: host saw "device descriptor read/64, error -110"
(EP0 silent, NAKing every IN). Root cause: a vector-table relocation ordering bug.

The BL runs from the boot section, so it must set CPUINT.CTRLA.IVSEL=1 to point the
interrupt vectors at its own table at 0x0000. bl_main.c set IVSEL *before* calling
usb_vendor_init(), but usb_vendor_init()'s first action is clock_init() — whose last
line writes `CPUINT.CTRLA = 0`, clearing IVSEL back to 0. With IVSEL=0 and BOOTSIZE=0x10,
USB bus-event interrupts dispatched to the *application's* vector table at 0x2000 (present
in the merged image), so the BL's USB0_BUSEVENT_vect never ran, usb_device_state stayed
UNATTACHED, usb_ctrl_poll() returned early every iteration, and EP0 NAKed forever.

The App is unaffected: it *wants* IVSEL=0 (its table is at 0x2000) and never sets IVSEL,
so clock_init()'s write is the correct end state there.

Fixed: moved the IVSEL set into usb_vendor_init(), immediately after clock_init() and
before usb_init()/sei(), so it can't be clobbered and no interrupt can fire before it is
in effect. Removed the now-redundant (clobbered) IVSEL write from bl_main.c. This was a
real bug in the production boot path, not just the test harness — exposed by the first
isolated BL enumeration test (forced entry). Verified: clean enumeration, device #19,
1209:b4b1 "Citrouille90 Bootloader", lsusb -v descriptors all correct, no dmesg errors.

## 7. EEPROM bootloader magic-flag — app→reset→BL entry verified on hardware

The GPR2/GPR3 → EEPROM migration for the cross-reset bootloader flag was implemented
(keyboard.c SYS_BOOT handler writes BOOT_MAGIC then triggers RSTCTRL.SWRR; bl_main.c
checks RSTFR.SWRF + the EEPROM byte) but had never been exercised end-to-end on hardware.

Tested on the CNano. SYS_BOOT lives on layer 2 (k00), but both momentary-layer keys
needed to reach layer 2 — LY_MO1 (k48, elec col 1 = PF1) and LY_MO2 (k49, elec col 0 =
PF0) — sit on the disconnected crystal columns, so the real layer-stack keystroke is not
reachable on the CNano (only on the real PCB). Worked around by temporarily remapping
layer-0 k00 (ESC position, elec row0/col8 = PD0/PD7 = breadboard L11/R3) to SYS_BOOT,
which exercises the same handler through real key dispatch. Remap reverted after the test.

Result: shorting L11×R3 fired SYS_BOOT → App wrote the magic and issued SWRR → BL detected
SWRF + magic and re-enumerated as 1209:b4b1 "Citrouille90 Bootloader" (clean, no errors).
A subsequent reset-button reset (EXTRF, not SWRF) correctly returned to the App, confirming
the BL clears the magic on entry and that only a software reset + valid magic enters the BL.
