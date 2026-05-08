#ifndef BOOTMAGIC_H
#define BOOTMAGIC_H

#include <stdint.h>

/* Boot entry magic written to GPR.GPR2/GPR3 by the app's SYS_BOOT handler
 * and checked by the bootloader before any peripheral init.
 * Both values must match for bootloader entry to proceed. */
#define BOOT_MAGIC        0x42u
#define BOOT_MAGIC_COMPL  ((uint8_t)(~BOOT_MAGIC))   /* 0xBD */

#endif /* BOOTMAGIC_H */
