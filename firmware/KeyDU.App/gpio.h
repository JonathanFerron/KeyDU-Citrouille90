/* gpio.h — Lightweight GPIO HAL for AVR64DU32
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
#ifndef GPIO_H
#define GPIO_H

#include <avr/io.h>
#include <stdint.h>

/* -----------------------------------------------------------------------
 * PIN DESCRIPTOR
 * Encode a port pointer + bitmask into a struct for type safety.
 * Usage:  static const gpio_pin_t LED = GPIO_PIN(PORTA, 4);
 * ----------------------------------------------------------------------- */
typedef struct {
    PORT_t  *port;
    uint8_t  mask;
} gpio_pin_t;

#define GPIO_PIN(port, pin_n)   { .port = &(port), .mask = (1u << (pin_n)) }

/* -----------------------------------------------------------------------
 * DIRECTION
 * ----------------------------------------------------------------------- */
#define GPIO_SET_OUTPUT(p)      ((p).port->DIRSET = (p).mask)
#define GPIO_SET_INPUT(p)       ((p).port->DIRCLR = (p).mask)

/* -----------------------------------------------------------------------
 * OUTPUT DRIVE
 * ----------------------------------------------------------------------- */
#define GPIO_HIGH(p)            ((p).port->OUTSET = (p).mask)
#define GPIO_LOW(p)             ((p).port->OUTCLR = (p).mask)
#define GPIO_TOGGLE(p)          ((p).port->OUTTGL = (p).mask)
#define GPIO_WRITE(p, val)      ((val) ? GPIO_HIGH(p) : GPIO_LOW(p))

/* -----------------------------------------------------------------------
 * INPUT READ
 * Returns non-zero if pin is high, zero if low.
 * ----------------------------------------------------------------------- */
#define GPIO_READ(p)            ((p).port->IN & (p).mask)

/* Read a single pin from its VPORT — single-cycle IN instruction.
 * Returns 0 or 1 (not a bitmask). Only valid for ports with a VPORT
 * mapping (PORTA, PORTC, PORTD, PORTF on the AVR64DU32).
 * vport: VPORTA/VPORTC/VPORTD/VPORTF
 * pin_n: pin number 0–7 */
#define GPIO_VPORT_READ(vport, pin_n)    (((vport).IN >> (pin_n)) & 1u)

/* -----------------------------------------------------------------------
 * PIN CONFIG  (PINnCTRL registers)
 * Each pin's config is at port->PIN0CTRL + pin index — standard AVR-Dx
 * layout, so pointer arithmetic on uint8_t* is safe here.
 * ----------------------------------------------------------------------- */
#define _PINCTRL(p) \
    (*(volatile uint8_t *)((uintptr_t)(p).port + \
        offsetof(PORT_t, PIN0CTRL) + __builtin_ctz((p).mask)))

/* Pull-up (input mode) */
#define GPIO_PULLUP_ENABLE(p)   (_PINCTRL(p) |=  PORT_PULLUPEN_bm)
#define GPIO_PULLUP_DISABLE(p)  (_PINCTRL(p) &= ~PORT_PULLUPEN_bm)

/* Invert logic level */
#define GPIO_INVERT_ENABLE(p)   (_PINCTRL(p) |=  PORT_INVEN_bm)
#define GPIO_INVERT_DISABLE(p)  (_PINCTRL(p) &= ~PORT_INVEN_bm)

/* Input sense — pass PORT_ISC_*_gc constants from avr/io.h */
#define GPIO_SET_ISC(p, isc)    (_PINCTRL(p) = \
    (_PINCTRL(p) & ~PORT_ISC_gm) | (isc))

/* Disable digital input buffer entirely (saves power on analog pins) */
#define GPIO_DISABLE_INPUT_BUFFER(p) \
    GPIO_SET_ISC(p, PORT_ISC_INPUT_DISABLE_gc)

/* Configure pull-up on multiple pins of the same port simultaneously.
 * 'port' is a PORT_t reference (e.g. PORTA), 'mask' is a bitmask of pins.
 * PINCONFIG is mirrored across all ports; writing to any port's PINCONFIG
 * is sufficient before applying PINCTRLSET. */
#define GPIO_MULTIPIN_PULLUP(port, mask)  \
    do {                                  \
        (port).PINCONFIG  = PORT_PULLUPEN_bm; \
        (port).PINCTRLSET = (mask);       \
    } while (0)

#define GPIO_MULTIPIN_DISABLE_INPUT_BUFFER(port, mask) \
    do {                                               \
        (port).PINCONFIG  = PORT_ISC_INPUT_DISABLE_gc; \
        (port).PINCTRLSET = (mask);                    \
    } while (0)

#endif /* GPIO_H */
