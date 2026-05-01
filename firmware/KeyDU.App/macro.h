// macro.h - Citrouille90 Keyboard Firmware
// Macro execution with hybrid approach

#ifndef MACRO_H
#define MACRO_H

#include <stdint.h>
#include "keycode.h"

// ============================================================================
// Macro Configuration
// ============================================================================
#define MAX_HELD_MACROS     6  // Max simultaneous held macros

// Macro action types
typedef enum {
    MACRO_ACTION_DOWN,      // Press key/modifier
    MACRO_ACTION_UP,        // Release key/modifier
    MACRO_ACTION_TAP,       // Press and release
    MACRO_ACTION_WAIT,      // Delay in ms
    MACRO_ACTION_END        // End of macro sequence
} macro_action_type_t;

// Macro action structure
typedef struct {
    uint8_t type;           // macro_action_type_t
    uint8_t keycode;        // Key or modifier to act on
} macro_action_t;

// Public macros for defining sequences (properly namespaced)
#define MACRO_DOWN(kc)      {MACRO_ACTION_DOWN, kc}
#define MACRO_UP(kc)        {MACRO_ACTION_UP, kc}
#define MACRO_TAP(kc)       {MACRO_ACTION_TAP, kc}
#define MACRO_WAIT(ms)      {MACRO_ACTION_WAIT, ms}
#define MACRO_END           {MACRO_ACTION_END, 0}

// Execute a macro
void execute_macro(uint16_t keycode);

#endif // MACRO_H
