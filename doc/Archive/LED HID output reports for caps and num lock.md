# Getting and using LED output reports

Add the EP OUT to **Interface 0 (Boot Keyboard)**.

This is the correct choice because the HID LED output report (Num Lock, Caps Lock, Scroll Lock, Compose, Kana) is part of the standard boot keyboard protocol. The host sends LED state via a **HID Output Report** on the keyboard interface, and BIOS/UEFI also knows to use Interface 0 for this since it's the boot-class interface.

Your endpoint allocation would become:

```
Endpoint 0:    Control (bidirectional)
Endpoint 1 IN: Keyboard reports (Interface 0)
Endpoint 1 OUT: LED output reports (Interface 0)  ← add this
Endpoint 2 IN: Mouse reports (Interface 1)
Endpoint 3 IN: Extended control reports (Interface 2)
```

Note that EP1 OUT shares the endpoint number with EP1 IN — IN and OUT are separate pipes on the same endpoint number, which is normal and saves you an endpoint number.

**The output report format** (1 byte, no Report ID since Interface 0 has no report IDs):

```
Bit 0: Num Lock
Bit 1: Caps Lock
Bit 2: Scroll Lock
Bit 3: Compose
Bit 4: Kana
Bits 5-7: Padding
```

The host can deliver this either via the EP1 OUT endpoint or via a `SET_REPORT` control transfer on EP0 — you should support both. Many OSes use `SET_REPORT` (EP0) rather than the interrupt OUT endpoint, so if you want to keep things minimal you could skip the physical OUT endpoint and rely solely on `SET_REPORT`, but declaring the OUT endpoint in your descriptor is the more complete and correct implementation.