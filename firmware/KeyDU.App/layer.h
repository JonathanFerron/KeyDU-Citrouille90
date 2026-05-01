// layer.h - Citrouille90 Keyboard Firmware
// Layer management

#ifndef LAYER_H
#define LAYER_H

#include <stdint.h>
#include "config.h"

// ============================================================================
// Layer Configuration
// ============================================================================
#define MAX_LAYER_KEYS      4  // Max simultaneous layer hold keys

// Track which physical keys are holding which layers
typedef struct {
    uint8_t row;
    uint8_t col;
    uint8_t target_layer;
} layer_key_t;

extern uint8_t current_layer;
extern layer_key_t active_layer_keys[MAX_LAYER_KEYS];
extern uint8_t active_layer_key_count;

void layer_init(void);
void layer_update(void);
void layer_key_pressed(uint8_t row, uint8_t col, uint8_t target_layer);
void layer_key_released(uint8_t row, uint8_t col);

#endif // LAYER_H
