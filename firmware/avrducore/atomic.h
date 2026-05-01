#ifndef ATOMIC_H
#define ATOMIC_H

/* Critical section macros for AVR-GCC.
 *
 * ENTER_CRITICAL / EXIT_CRITICAL save and restore SREG (including I-bit),
 * so they nest correctly with other cli/sei callers.
 *
 * Usage:
 *   ENTER_CRITICAL();
 *   ... shared state access ...
 *   EXIT_CRITICAL();
 *
 * The dummy parameter is accepted but unused — it exists so call sites
 * can give the saved-SREG a name for readability.
 */

#define ENTER_CRITICAL(unused)                          \
    __asm__ __volatile__ (                              \
        "in  __tmp_reg__, __SREG__" "\n\t"              \
        "cli"                       "\n\t"              \
        "push __tmp_reg__"          "\n\t"              \
        ::: "memory"                                    \
    )

#define EXIT_CRITICAL(unused)                           \
    __asm__ __volatile__ (                              \
        "pop __tmp_reg__"           "\n\t"              \
        "out __SREG__, __tmp_reg__" "\n\t"              \
        ::: "memory"                                    \
    )

#define DISABLE_INTERRUPTS()  __asm__ __volatile__ ("cli" ::: "memory")
#define ENABLE_INTERRUPTS()   __asm__ __volatile__ ("sei" ::: "memory")

#endif /* ATOMIC_H */
