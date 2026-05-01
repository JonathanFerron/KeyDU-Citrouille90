Direct register access using the AVR-provided headers:

```c
#include <avr/io.h>

// Write magic value
GPR.GPR0 = 0xAB;
GPR.GPR1 = 0xCD;

// Read and check
if (GPR.GPR0 == 0xAB && GPR.GPR1 == 0xCD) {
    // bootloader entry confirmed
}

// Clear after use
GPR.GPR0 = 0x00;
GPR.GPR1 = 0x00;
```

The `GPR` struct is defined in `<avr/io.h>` → pulled in via `ioavr64du32.h` for your target. The members are `GPR0` through `GPR3`.

For your bootloader magic, I'd define the values as named constants in a shared header that both `kb.app` and `KeyDU.BL` include:

```c
#define BL_MAGIC_0  0x4B    // 'K'
#define BL_MAGIC_1  0x44    // 'D'
```

Then your app-side trigger:

```c
void reboot_to_bootloader(void) {
    GPR.GPR0 = BL_MAGIC_0;
    GPR.GPR1 = BL_MAGIC_1;
    _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
}
```

And your bootloader-side check at the top of `main()`, before any hardware init:

```c
if (RSTCTRL.RSTFR & RSTCTRL_SWRF_bm) {
    if (GPR.GPR0 == BL_MAGIC_0 && GPR.GPR1 == BL_MAGIC_1) {
        GPR.GPR0 = 0;
        GPR.GPR1 = 0;
        // proceed with bootloader
    }
}
// otherwise jump to kb.app
```

Note `_PROTECTED_WRITE` — RSTCTRL.SWRR is a protected register, so it requires the CCP unlock sequence. The macro handles that for you.

The check needs to happen **before the C runtime zeros SRAM**, because a zeroing pass would wipe GPR0/GPR1 if they were in RAM — but since they're I/O registers, the C runtime never touches them. So you actually don't have that problem here.

That means a check at the top of `main()` before any peripheral init is perfectly fine and sufficient. The GPR registers are untouched by startup code.

The `__attribute__((naked)) __attribute__((section(".init3")))` early-init pattern is only necessary when you're using a RAM variable for the magic value (like LUFA does with `ATTR_NO_INIT`), because you need to read it before `.bss` zeroing runs. Since you chose GPR registers instead of RAM, you dodged that entire complexity.

So no, don't bother with the init section approach. `main()` first line is clean and correct for your case.

That's a fair challenge — let me search for it rather than just asserting it.

The `euboot` result is the most directly relevant find. Let me fetch it.

I have to be honest with you: I didn't find a Microchip-authored example that explicitly demonstrates GPR survival across SWRST with code you can point to. The euboot project is the closest real-world AVR-DU bootloader example, and it uses a **GPIO pin state** (SW0 low during SWRST) rather than GPR magic values as its entry condition.

So here's where things stand honestly:

**What the datasheet table proves:** SWRST does not reset "Other Volatile Logic" — but the datasheet doesn't explicitly enumerate what is and isn't in that category for GPR0–3 specifically. It's an inference, albeit a well-founded one.

**The most rigorous thing you can do** is test it yourself — it's a 10-line program:

```c
#include <avr/io.h>

int main(void) {
    if (GPR.GPR0 == 0xAB && GPR.GPR1 == 0xCD) {
        // survived reset — signal with LED or UART
        GPR.GPR0 = 0;
        GPR.GPR1 = 0;
        // blink LED fast
    } else {
        GPR.GPR0 = 0xAB;
        GPR.GPR1 = 0xCD;
        _PROTECTED_WRITE(RSTCTRL.SWRR, RSTCTRL_SWRE_bm);
    }
}
```

Flash it, observe behavior. That settles the question definitively for your specific silicon revision, which is more trustworthy than any LLM including me.

Exactly. And it's a trivial test to write and run — you've got the Adafruit UPDI Friend and avrdude ready to go, so flashing a 10-line proof-of-concept is low friction.

One thing worth noting when you run it: check `RSTCTRL.RSTFR` at the top of main before anything else and save the value, because the act of reading it doesn't clear it — you need to explicitly write 1s to clear the flags. If you're not careful about that in the final bootloader, stale reset flags from a previous POR could interfere with your magic value check logic on a subsequent SWRST. Standard practice is to read it, save it to a local variable, then clear it immediately:

```c
uint8_t reset_flags = RSTCTRL.RSTFR;
RSTCTRL.RSTFR = reset_flags; // clear all flags
```

Then make your bootloader entry decision based on the saved value.