#include <avr/io.h>
#include "clock.h"
#include "ccp.h"

void clock_init(void)
{
    /* Select OSCHF as main clock source, no clock output on pin */
    ccp_write_ioreg((void *)&CLKCTRL.MCLKCTRLA,
        CLKCTRL_CLKSEL_OSCHF_gc);

    /* Prescaler disabled — run OSCHF directly at 24 MHz */
    ccp_write_ioreg((void *)&CLKCTRL.MCLKCTRLB,
        0);

    /* OSCHF: 24 MHz, binary algorithm, autotune off, no standby run */
    ccp_write_ioreg((void *)&CLKCTRL.OSCHFCTRLA,
        CLKCTRL_FRQSEL_24M_gc |
        CLKCTRL_ALGSEL_BIN_gc |
        CLKCTRL_AUTOTUNE_OFF_gc);

    /* TIMEBASE must equal ceil(F_CPU / 1e6) = 24 for USB and TCA */
    ccp_write_ioreg((void *)&CLKCTRL.MCLKTIMEBASE, 0x18);

    /* Wait for OSCHF to lock before any peripheral init */
    while (!(CLKCTRL.MCLKSTATUS & CLKCTRL_OSCHFS_bm))
        ;

    /* CPUINT: default priority, no round-robin, no compact vectors */
    ccp_write_ioreg((void *)&CPUINT.CTRLA, 0);
}
