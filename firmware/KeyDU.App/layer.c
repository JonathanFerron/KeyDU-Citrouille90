// ============================================================================
// layer.c
// ============================================================================

#include "layer.h"

uint8_t current_layer = 0;
layer_key_t active_layer_keys[MAX_LAYER_KEYS];
uint8_t active_layer_key_count = 0;

void layer_init(void) {
    current_layer = 0;
    active_layer_key_count = 0;
    
    // Clear tracking array
    for (uint8_t i = 0; i < MAX_LAYER_KEYS; i++) {
        active_layer_keys[i].row = 0xFF;
        active_layer_keys[i].col = 0xFF;
        active_layer_keys[i].target_layer = 0;
    }
}

// Calculate which layer should be active based on held layer keys
// Only one layer is active at a time - the highest priority one
void layer_update(void) {
    if (active_layer_key_count == 0) {
        current_layer = 0;
        return;
    }
    
    // Find the highest layer being held
    uint8_t highest = 0;
    for (uint8_t i = 0; i < active_layer_key_count; i++) {
        if (active_layer_keys[i].target_layer > highest) {
            highest = active_layer_keys[i].target_layer;
        }
    }
    
    current_layer = highest;
}

// Called when a layer key is pressed
void layer_key_pressed(uint8_t row, uint8_t col, uint8_t target_layer) {
    // Add to tracking array if space available
    if (active_layer_key_count < MAX_LAYER_KEYS) {
        active_layer_keys[active_layer_key_count].row = row;
        active_layer_keys[active_layer_key_count].col = col;
        active_layer_keys[active_layer_key_count].target_layer = target_layer;
        active_layer_key_count++;
        layer_update();
    }
}

// Called when a layer key is released
void layer_key_released(uint8_t row, uint8_t col) {
    // Find and remove this key from the array
    for (uint8_t i = 0; i < active_layer_key_count; i++) {
        if (active_layer_keys[i].row == row && 
            active_layer_keys[i].col == col) {
            // Shift remaining keys down
            for (uint8_t j = i; j < active_layer_key_count - 1; j++) {
                active_layer_keys[j] = active_layer_keys[j + 1];
            }
            
            // Clear the last slot
            active_layer_key_count--;
            active_layer_keys[active_layer_key_count].row = 0xFF;
            active_layer_keys[active_layer_key_count].col = 0xFF;
            active_layer_keys[active_layer_key_count].target_layer = 0;
            
            layer_update();
            return;
        }
    }
}
