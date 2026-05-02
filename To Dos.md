# Citrouille90 To Dos

## Outstanding Firmware Issues — Implementation Order

### USB Protocol Correctness

**#11 — Address spin loop lacks disconnect guard in `usb_ctrl.c`**
`ctrl_set_address()` spins on `while (!ep_in_ready())` with no escape. Add a `usb_device_state == USB_STATE_UNATTACHED` check inside the loop, consistent with the guards used elsewhere in that file.

---

### Housekeeping (last, low risk)

**#19 — `ep_configure_prv()` always returns `true` in `usb_ep.c`**
The return value is meaningless — every code path returns `true`. Either add real validation and propagate errors to the caller, or change the return type to `void` and update the call site.


- consider enabling autotuning to USB SOF
- consider timing the TCB0_INT_vect (that wakes up the MCU and sets the tick flag to the moment the SOF arrives (do this after enumeration) minus 200 microseconds)
- consider enhancing documentation of hid report descriptors to make them easier to read
- consider using software reset instead of watchdog reset for the entry to bootloader reset, and reducing the wdt wait time for the plain (not bootloader) SYS_Reset keycode.
