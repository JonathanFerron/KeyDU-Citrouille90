# Citrouille90 — Firmware Session Notes

### Encoder, Scan Loop Timing & USB Architecture

---

## 1. Files Produced This Session

| File                          | Status                          |
| ----------------------------- | ------------------------------- |
| `keymap_alttab_addition_v2.c` | Complete — drop into `keymap.c` |

---

## 2. Integration Checklist

### keyboard.c — one edit required

In `keyboard_task()`, replace:

```c
encoder_read();
encoder_task();
```

With:

```c
encoder_scan();
keymap_tick();
```



---

## 3. send_keyboard_report() — Critical Design Decision

**Must be non-blocking.** Design it as a queue/buffer write that returns immediately.
The USB peripheral handles the actual transaction autonomously when the host polls.

```c
// Intended model — implement when writing USB stack
static hid_keyboard_report_t kb_report_buf;
static volatile uint8_t      kb_report_dirty;

void send_keyboard_report(void) {
    memcpy(&kb_report_buf, &keyboard_report, sizeof(kb_report_buf));
    kb_report_dirty = 1;
    usb_ep_trigger(EP_HID_KEYBOARD);  // signal endpoint — implementation detail TBD
}
```

**Why this matters:**

- `encoder_step()` calls `send_keyboard_report()` multiple times inline for the
  Shift+Tab tap sequence. If blocking, this stalls for several ms mid-scan.
  If non-blocking, each call overwrites the buffer and returns in nanoseconds —
  host gets the most recent state on next poll, which is correct.
- The entire phase-lock timing architecture below depends on send being non-blocking.
  If send blocks, scan timing cannot be controlled.

**Document this decision explicitly in `usb_hid.h` before writing endpoint code.**

---

## 4. Scan Loop Timing — Phase Lock Architecture

### Current state (after main.c fix)

`TCB0` at 24MHz gives a stable 1ms tick independent of USB. Phase offset between
TCB0 tick and host SOF is fixed at enumeration time — constant sub-millisecond
offset, does not drift. Good enough for shipping firmware.

### Future optimisation — SOF phase lock

Once USB stack is stable and scan duration is measured, the latency floor can be
further reduced by aligning the scan trigger to fire just before the expected SOF.

**Concept:**

```
TCB0 fires at (SOF_phase - scan_budget):
    → matrix_scan()
    → encoder_scan()
    → keymap_tick()
    → report ready in queue

SOF ISR fires ~scan_budget ms later:
    → flush queued report to host
```

**How to measure SOF phase at runtime:**

1. In `USB_SOF_vect` ISR, snapshot `TCB0.CNT`
2. That value is the host's clock edge position within your 1ms TCB0 period
3. Subtract scan budget in timer counts:
   `target = sof_cnt - (scan_budget_us * (F_CPU / 1000000UL))`
4. Average across 8–16 consecutive SOF samples before committing
   (SOF is very stable once connected — filtering just rejects startup jitter)
5. Adjust TCB0.CCMP or store target count and compare against TCB0.CNT in main loop

**Scan budget measurement:** toggle a GPIO at scan entry/exit, measure with logic
analyzer. Even a cheap analyzer is sufficient. Target: confirm < 0.2ms (4800 counts
at 24MHz) with margin.

### Prerequisites before attempting phase lock

- [ ] `system_millis` gate in `main.c` working and stable
- [ ] USB stack enumerating and sending reports reliably
- [ ] Scan duration measured on real hardware
- [ ] `send_keyboard_report()` confirmed non-blocking

---

## 5. USB Stack Design Notes

### SOF interrupt role

`USB_SOF_vect` fires every 1ms when host is connected and bus is not suspended.

- **Primary role:** flush queued report to host (report was prepared by scan)
- **Secondary role (phase lock):** snapshot `TCB0.CNT` for phase measurement
- **Not** the scan trigger — scan is triggered by TCB0

### Bus suspend / no USB fallback

SOF stops during suspend and before enumeration. `TCB0` runs unconditionally —
it is the correct fallback scan clock. No special handling needed in scan code;
`keyboard_task()` just keeps running from TCB0 regardless of USB state.

### HID report sequence — correct ordering

**Scan → queue → send on host request**, not the other way around.
Report should reflect the world as it was just before the host asked,
not as it was up to 1ms ago.

---

## 6. keymap_tick() — Where It Lives

`keymap_tick()` must be called once per 1ms tick from `keyboard_task()`, after
`encoder_scan()`. It owns its own `uint32_t s_alt_idle_ticks` counter — no
dependency on `system_millis`, no `cli()/sei()` overhead. Counts scan ticks
directly since it knows it runs at 1KHz.

Do **not** use `millis()` for the alt-release timeout. `millis()` requires an
interrupt gate (`cli()/sei()`) to safely read the 32-bit `volatile` on an 8-bit
CPU — unnecessary overhead for something that can count ticks locally.

---

## 7. Open Items for Next Sessions

- [ ] Write `usb_hid.c` — establish non-blocking `send_keyboard_report()` first

- [ ] Implement `USB_SOF_vect` ISR — flush queued report, snapshot `TCB0.CNT`

- [ ] Fix `main.c` scan loop gate (section 2 above — quick win, do this first)

- [ ] Confirm `keyboard_report` and `add/remove_key_to_report()` declared in `usb_hid.h`
  
      (referenced in `keymap_alttab_addition_v2.c` but declaration not yet visible)

- [ ] Measure scan duration on hardware before attempting phase lock

- [ ] Revisit SOF phase lock after USB stack is stable and measured
