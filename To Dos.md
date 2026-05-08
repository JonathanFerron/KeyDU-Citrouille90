# Citrouille90 To Dos

To be addressed:
- keymap.c — fill in the actual keymap array based on kicad wiring

Nice to have (style issue):
- Look into fixing the following inconsistency:
    layer_key_released() is called unconditionally on every key release in keyboard.c
    In process_key_release(), layer_key_released(row, col) is called for every released key — including non-layer keys. 
    layer_key_released() will simply fail to find the entry and return harmlessly, but it's a style inconsistency: 
      process_key_press() only calls layer_key_pressed() when IS_LAYER_KEY(keycode) is true. The release path should mirror this, 
      which requires either checking the locked keycode first, or checking IS_LAYER_KEY before calling the layer function. 
      Since layer keys are not tracked in s_pressed_keys, their keycode won't be in keycode_out after untrack_pressed_key — 
      the current code calls layer_key_released before untracking, which is correct for layer keys, but the unconditional call 
      is still inconsistent with the press path.
      Beyond style, there's a subtle correctness edge case: if a layer key is released while the layer table is full 
      (active_layer_key_count == MAX_LAYER_KEYS) and the key wasn't tracked (because it was dropped on press), layer_key_released harmlessly 
      no-ops — but it still iterates the full array every release. Matching the press-path pattern here is the right fix.

Keep for phase 2:
- Time TCB0_INT_vect to 200us prior to expected SOF arrival (800 us after prior SOF arrival)


