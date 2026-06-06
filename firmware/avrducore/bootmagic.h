#ifndef BOOTMAGIC_H
#define BOOTMAGIC_H

#include <avr/eeprom.h>
#include <stdint.h>

/* Boot entry magic written to EEPROM by the app's SYS_BOOT handler
   and checked by the bootloader before any peripheral init.
   0x42 = ASCII 'B' for Bootloader. */
#define BOOT_MAGIC             0x42u

/* EEPROM byte address for the boot magic.
   Offset 0x00 from EEPROM base — change the offset and reflash both
   targets if this byte accumulates significant wear (e.g. hundreds of
   boot-trigger cycles per day during intensive dev). Any offset 0x00–0xFF
   is valid; no other EEPROM data is currently allocated. */
#define BOOT_MAGIC_EEPROM_ADDR ((uint8_t *)(EEPROM_START + 0x00))

#endif /* BOOTMAGIC_H */
