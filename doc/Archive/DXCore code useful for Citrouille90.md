# DxCore Information Relevant to AVR64DU32 (Citrouille90)

## Overview

This document extracts key information from the SpenceKonde/DxCore project that will be useful for developing firmware for the Citrouille90 keyboard using the AVR64DU32 microcontroller.

**DxCore Repository:** https://github.com/SpenceKonde/DxCore

---

### Clock Initialization Pattern

Based on DxCore approach:

```c
// Simplified from DxCore clock initialization
void init_clock(void) {
    // Protected write sequence required for CLKCTRL changes
    _PROTECTED_WRITE(CLKCTRL.OSCHFCTRLA, 
                     CLKCTRL_FRQSEL_20M_gc);  // 20 MHz for USB, but pick 24 instead 
    
    // Wait for clock to stabilize
    while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSCHFS_bm));
    
    // Main clock prescaler (typically disabled for max speed)
    _PROTECTED_WRITE(CLKCTRL.MCLKCTRLB, 0);  // No prescaling
}
```

---

## 4. Reset and Startup Handling

### DxCore Reset Flag Handling

**Critical pattern from DxCore:**

```c
// DxCore saves reset flags in GPIOR0 very early (.init3)
// This must be done before clearing RSTCTRL.RSTFR
// KeyDU.BL could do that at the beginning of main()

void init_reset_flags(void) __attribute__((naked)) 
                             __attribute__((used)) 
                             __attribute__((section(".init3")));

void init_reset_flags(void) {
    // Save reset flags to GPIOR0 (user-accessible), note can't use the same as the bootloader magic flags
    GPIOR0 = RSTCTRL.RSTFR;
    
    // Clear reset flags (required for proper operation)
    RSTCTRL.RSTFR = RSTCTRL.RSTFR;  // Write 1s to clear
}
```

**Why this matters:**
- Reset flags must be cleared after reading
- If not cleared, certain peripherals may not function correctly
- GPIOR0 preserves the information for user code

### Dirty Reset Detection

DxCore implements **dirty reset detection** to catch software failures:

```c
// In .init3, before main(), but KeyDU.BL could probably do that at the beginning of main()
if (RSTCTRL.RSTFR == 0) {
    // Dirty reset! Registers weren't cleared
    // Force a clean software reset
    _PROTECTED_WRITE(RSTCTRL.SWRR, 1);
}
```
---

## 7. Interrupt Handling

### DxCore Interrupt Patterns

From DxCore Ref_Interrupts.md:

**Port Interrupts:**
- All pins can generate interrupts
- Pins 2 and 6 of each port are "async" (can wake from deep sleep)

```c
// Enable pin interrupt (DxCore attachInterrupt style)
void setup_pin_interrupt(void) {
    // Configure pin for interrupt
    PORTA.PIN6CTRL = PORT_PULLUPEN_bm | PORT_ISC_RISING_gc;
    
    // Define ISR
    ISR(PORTA_PORT_vect) {
        // Read interrupt flags
        uint8_t flags = PORTA.INTFLAGS;
        
        // Clear flags by writing 1s
        PORTA.INTFLAGS = flags;
        
        // Handle interrupt
        if (flags & PIN6_bm) {
            // PA6 interrupt occurred
        }
    }
}
```

**For AmberClick90:**
- may Use port interrupts for encoder (PA6, PA7), but not planning on
- Matrix scanning typically polled, not interrupt-driven

### Interrupt Priority

AVR64DU32 supports interrupt priority levels:
- **Level 0:** Normal priority (default)
- **Level 1:** High priority

```c
// Set high priority (from CPUINT peripheral)
CPUINT.LVL1VEC = PORTA_PORT_vect_num;
```

---

## 8. UPDI Programming Considerations

### SerialUPDI (Recommended Programmer)

DxCore strongly recommends SerialUPDI:

**Configuration:**
```
Baud rates supported:
- 57600  (slow, for Vcc < 2.7V)
- 230400 (default, works most places)
- 460800 (fast, requires Vcc ≥ 4.5V)
```

---

## 9. Fuse Configuration

### Critical Fuses for AVR64DU32

From DxCore tools menu patterns:

```c
// SYSCFG0: System Configuration 0
// Bits:
// - CRCSRC: CRC source (typically disabled)
// - RSTPINCFG: Reset pin configuration

// SYSCFG1: System Configuration 1

// BOOTSIZE: Bootloader size
// - For 8KB bootloader: BOOTSIZE = 0x10

// CODESIZE: Not typically changed

// OSCCFG: Oscillator configuration (set at compile time via clock menu)
```

### Fuse Setting via DxCore

**Important:** DxCore sets fuses during every updi upload:

```c
// Fuses are set automatically based on Tools menu:
// - BOD voltage
// - BOD mode
// - Startup time
// - EEPROM retention
// - Reset pin function
```

---

## 10. Memory Layout Recommendations

### Flash Memory Organization

From DxCore memory management docs:

```
|------------------------| 0x0000
|  Bootloader (8 KB)     |
|                        |
|------------------------| 0x2000
|                        |
|  Application Firmware  |
|  (remaining flash)     |
|                        |
|------------------------| End of Flash
```

### PROGMEM Considerations

**For 64KB parts (like AVR64DU32):**

DxCore provides mapped flash access:

```c
// Constants in flash can be accessed two ways:

// 1. Traditional PROGMEM with pgm_read_byte()
const uint8_t table[] PROGMEM = {1, 2, 3, ...};
uint8_t value = pgm_read_byte(&table[index]);

// 2. Memory-mapped access (easier)
const uint8_t table[] PROGMEM_MAPPED = {1, 2, 3, ...};
uint8_t value = table[index];  // Direct access!
```

**For AmberClick90:**
- Store keymaps in PROGMEM_MAPPED
- Store USB descriptors in PROGMEM_MAPPED
- Store macro definitions in PROGMEM_MAPPED

### SRAM Usage

```c
// Typical SRAM allocation (8 KB total):
// - Stack: ~512 bytes
// - Global variables: varies
// - Heap (if used): remainder
```

---

## 11. Watchdog Timer (WDT)

### DxCore WDT Patterns

```c
// Enable watchdog with timeout
void wdt_enable(uint8_t period) {
    _PROTECTED_WRITE(WDT.CTRLA, period);
}

// Feed/reset watchdog
void wdt_reset(void) {
    __asm__ __volatile__ ("wdr");
}

// Disable watchdog
void wdt_disable(void) {
    _PROTECTED_WRITE(WDT.CTRLA, 0);
}
```

**Timeout periods:**
```c
WDT_PERIOD_8CLK_gc    // 8 ms
WDT_PERIOD_16CLK_gc   // 16 ms
WDT_PERIOD_32CLK_gc   // 32 ms
WDT_PERIOD_64CLK_gc   // 64 ms
WDT_PERIOD_128CLK_gc  // 128 ms
WDT_PERIOD_256CLK_gc  // 256 ms
WDT_PERIOD_512CLK_gc  // 512 ms
WDT_PERIOD_1KCLK_gc   // 1 s
WDT_PERIOD_2KCLK_gc   // 2 s
WDT_PERIOD_4KCLK_gc   // 4 s
WDT_PERIOD_8KCLK_gc   // 8 s
```

---

## 12. Protected Write Sequence

### Critical for Configuration Registers

Many AVR DU registers require a protected write:

```c
// DxCore implementation
static inline void _PROTECTED_WRITE(volatile uint8_t *reg, uint8_t value) {
    // Disable interrupts
    uint8_t sreg = SREG;
    cli();
    
    // CCP signature for protected write
    CCP = CCP_IOREG_gc;
    
    // Write must occur within 4 cycles
    *reg = value;
    
    // Restore interrupt state
    SREG = sreg;
}
```

**Used for:**
- CLKCTRL configuration
- WDT configuration
- RSTCTRL software reset
- NVMCTRL operations

---

## 15. DxCore Callbacks for Customization

### Available Weak Functions

DxCore provides callbacks you can override:

```c
// Called very early in initialization (.init3)
void initVariant() __attribute__((weak));

```

**For AmberClick90:**
- Use `initVariant()` for early hardware setup
- Preserve for future customization needs

---

## 18. Recommended File Structure

Based on DxCore organization:

```
/src
  main.c                  # Entry point
  startup.S               # Startup code (.init sections)
  config.h                # Pin mappings, constants
  hal/
    hal_gpio.c/h          # GPIO abstraction (DxCore patterns)
    hal_timer.c/h         # Timer abstraction
  keyboard/
    matrix.c/h            # Matrix scanning
    encoder.c/h           # Encoder handling
    layers.c/h            # Layer logic
  usb/
    usb_hid.c/h           # USB HID stack (from LUFA/MCC/TinyUSB)
    descriptors.c/h       # USB descriptors
```

---

## 19. Build System Recommendations

### From DxCore Makefile Patterns

```makefile
# Compiler flags (from DxCore)
MCU = avr64du32
F_CPU = 24000000UL
CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -Wextra
LDFLAGS = -mmcu=$(MCU)

# Optimization
CFLAGS += -ffunction-sections -fdata-sections
LDFLAGS += -Wl,--gc-sections
```