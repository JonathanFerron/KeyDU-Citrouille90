# Citrouille90 — Fixed / Done

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
and the HID report seqlock queue all work correctly under real keystroke traffic.
