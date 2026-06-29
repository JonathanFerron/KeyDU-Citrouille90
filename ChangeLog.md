# Citrouille90 — ChangeLog

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
