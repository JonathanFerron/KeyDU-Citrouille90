# Citrouille90 — Scanning & Debounce Decisions

---

## 1. Matrix Scanning Frequency — 1 KHz

**Decision:** Scan the keyboard matrix at 1000Hz (1ms interval), gated by a timer in the main loop.

**Rationale:**
- USB Full Speed HID polling occurs every 1ms — scanning at 1KHz aligns cleanly with the USB report deadline
- At 24MHz, a full 90-key matrix scan completes well under 100µs, leaving ample headroom within the 1ms window
- TCA0 PWM at 1465Hz runs entirely in hardware (CMP2/CMP3); the CPU has no per-cycle obligation there, so no contention with the scan loop
- 1KHz is disciplined and deterministic — a deliberate architectural choice suited to purpose-built bare-metal firmware, unlike QMK's "scan as fast as possible" approach which is a historical artifact of its generalist framework design
- Returns diminish sharply above 1KHz for switch debouncing purposes, as all target switches settle well within 1ms

---

## 2. Encoder Pin Scanning — Part of the Regular 1KHz Loop

**Decision:** Scan encoder pins A and B as part of the same 1KHz matrix scan loop, not via pin-change interrupts.

**Rationale:**
- With hardware RC filtering in place (see section 3), the signal is already clean and stable well within the 1ms polling window — the primary argument for interrupt-driven sampling (catching fast edges) is moot
- Integrating encoder scanning into the existing loop keeps firmware architecture unified and deterministic — no additional interrupt vectors competing with USB and PWM obligations
- The quadrature state machine runs at a known, fixed rate which simplifies reasoning about debounce timing
- At the PES12's maximum rated speed (60 RPM, 24 detents/rev), detent-to-detent time is ~41ms — vastly longer than the 1ms scan interval, so no transitions will be missed by polling

---

## 3. Hardware RC Filter on Encoder Pins

**Decision:** Implement the Bourns-recommended RC filter on encoder terminals A and B.

**Circuit (per pin):**
- 10kΩ pull-up resistor to VDD (5V): note, configure the AVR GPIO pins as plain input, no internal pull-up, as otherwise the RC filter time constant may shift away from the ideal value
- 10kΩ series resistor between terminal and MCU GPIO
- 10nF capacitor from GPIO pin to GND
- Terminal C to GND

**RC time constant:** 10kΩ × 10nF = **100µs**

**Impact:**
- Suppresses contact bounce electrically before it reaches the MCU — bounce spikes faster than ~300–500µs rise/fall are filtered out
- Substantially reduces reliance on software debounce logic
- Allows the quadrature state machine to operate with minimal additional software layers
- 100µs time constant is well below the 1ms polling interval — legitimate quadrature transitions are unaffected
- No level shifting required — board VDD is 5V, AVR64DU32 VDD is 5V (1.8–5.5V supported per datasheet), USB 3.3V derived internally via on-chip regulator

---

## 4. Bourns PES12 Encoder — Relevant Specifications

| Parameter | Value |
|---|---|
| Max contact bounce | 5ms |
| Max rotation speed | 60 RPM |
| Detents per revolution | 24 |
| Pulses per revolution | 24 (1 full quadrature cycle per detent) |
| Detent-to-detent time at 60 RPM | ~41.7ms |
| Quadrature transitions per detent | 4 |
| Min time between transitions at max speed | ~10.4ms |

---

## 5. Kailh Choc V1 Switch Debouncing

**Decision:** Eager press with deferred release, or simple N-scan successive confirmation.

**Rationale:**
- Kailh Choc V1 datasheet specifies bounce time under 5ms — at 1KHz scanning, 4 successive identical readings = 3ms debounce window, within spec with margin
- Eager press (accept keydown on first detected edge, debounce release with a short confirmation window) gives best perceived responsiveness with no risk of spurious re-triggers
- Simple 4-scan confirmation is a valid and simpler alternative if implementation uniformity is preferred over minimum latency

**Recommended:** 4 successive identical readings (including the first) for both press and release, giving a **3ms debounce window**. Revisit empirically during firmware testing.

---

## 6. Encoder Quadrature Decoding Algorithm

**Approach:** 16-entry lookup table quadrature state machine, with accumulator threshold, accumulator reset on illegal transitions, and post-step lockout timer. Configure the AVR GPIO encoder pins as plain input, no internal pull-up.

**Hardware RC filter eliminates the need for:** successive confirmation scans, integrator/counter debounce, direction consistency filtering, majority voting / moving average approaches.

**Retained software layers:**
- 16-entry lookup table — core quadrature decoding (non-optional)
- ±4 accumulator threshold — requires full valid quadrature cycle before emitting a step
- Accumulator reset on illegal transition — belt-and-suspenders against residual noise
- Post-step lockout timer — guards against ringing immediately after a valid step

### Final Pseudocode

```
CONSTANT LOCKOUT_MS = 2

// 16-entry lookup table
// index = (prev_ab << 2) | curr_ab
// +1 = CW, -1 = CCW, 0 = no change or illegal
lookup[16] = {
     0,  // 0000: 00→00 no change
    -1,  // 0001: 00→01 valid CCW
    +1,  // 0010: 00→10 valid CW
     0,  // 0011: 00→11 illegal
    +1,  // 0100: 01→00 valid CW
     0,  // 0101: 01→01 no change
     0,  // 0110: 01→10 illegal
    -1,  // 0111: 01→11 valid CCW
    -1,  // 1000: 10→00 valid CCW
     0,  // 1001: 10→01 illegal
     0,  // 1010: 10→10 no change
    +1,  // 1011: 10→11 valid CW
     0,  // 1100: 11→00 illegal
    +1,  // 1101: 11→01 valid CCW
    -1,  // 1110: 11→10 valid CW
     0,  // 1111: 11→11 no change
}

// State (persistent across calls)
prev_ab       = read pins A,B        // initialize to current pin state at boot
accumulator   = 0
lockout_timer = 0


FUNCTION encoder_scan():             // called once every 1ms from main scan loop

    // Decrement lockout timer
    IF lockout_timer > 0 THEN
        lockout_timer -= 1
    END IF

    // Early exit during lockout — prev_ab held at last known good state
    IF lockout_timer > 0 THEN
        RETURN
    END IF

    // Read current pin state
    curr_ab = read pins A,B

    // No change — early exit
    IF curr_ab == prev_ab THEN
        RETURN
    END IF

    // Build lookup index
    index = (prev_ab << 2) | curr_ab

    // Look up transition value
    delta = lookup[index]

    // Illegal transition — reset accumulator, stay anchored to last good state
    IF delta == 0 THEN
        accumulator = 0
        RETURN
    END IF

    // Valid transition — accumulate and update state
    accumulator += delta
    prev_ab = curr_ab

    // Check for completed full quadrature cycle
    IF accumulator >= +4 THEN
        accumulator = 0
        emit_step(CW)
        lockout_timer = LOCKOUT_MS

    ELSE IF accumulator <= -4 THEN
        accumulator = 0
        emit_step(CCW)
        lockout_timer = LOCKOUT_MS

    END IF
```

### Key Design Notes

**prev_ab not updated on illegal transition:** anchors the state machine to the last known good state so the next valid reading is evaluated against a clean baseline.

**Lockout timer decremented before the lockout check:** if the timer reaches zero in step 1, execution falls through to full decode logic immediately in the same scan cycle — no one-cycle delay.

**Accumulator reset before emit:** a new cycle always starts clean, whether or not a step was emitted (e.g. if lockout were ever active at that point — it cannot be given step 3, but the reset is unconditional for clarity).

**emit_step() dispatches to layer/keymap logic:** CW and CCW map to whatever the active layer specifies — volume up/down, alt-tab direction, LED brightness, etc. The encoder decode layer is fully agnostic to the assigned action.
