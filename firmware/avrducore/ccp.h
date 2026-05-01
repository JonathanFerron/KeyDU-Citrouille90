#ifndef CCP_H
#define CCP_H

#include <avr/io.h>
#include <stdint.h>

/* CCP-protected register write. Unlocks the register for 4 cycles using the
   IOREG or SPM signature, then writes value before the window closes. */
extern void ccp_write_io(void *addr, uint8_t magic, uint8_t value);

static inline void ccp_write_ioreg(void *addr, uint8_t value)
{
    ccp_write_io(addr, CCP_IOREG_gc, value);
}

static inline void ccp_write_spm(void *addr, uint8_t value)
{
    ccp_write_io(addr, CCP_SPM_gc, value);
}

#endif /* CCP_H */
