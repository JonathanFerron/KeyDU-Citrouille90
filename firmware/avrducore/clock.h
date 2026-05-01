#ifndef CLOCK_H
#define CLOCK_H

/* System clock: 24 MHz OSCHF, no prescaler. Maximum clock speed for AVR64DU32.
   MCLKTIMEBASE set to 24 (0x18) per datasheet §11.5.11: value must equal
   ceil(F_CPU_MHz) for peripherals that use it (e.g. TCA, USB). */
#ifndef F_CPU
  #define F_CPU 24000000UL
#endif

/* Initialise CLKCTRL and CPUINT. Blocks until OSCHF is stable. */
void clock_init(void);

#endif /* CLOCK_H */
