# Citrouille90 To Dos

- Look into fixing the following inconsistency:
    layer_key_released() is called unconditionally on every key release in keyboard.c
    In process_key_release(), layer_key_released(row, col) is called for every released key — including non-layer keys. 
    layer_key_released() will simply fail to find the entry and return harmlessly, but it's a style inconsistency: 
      process_key_press() only calls layer_key_pressed() when IS_LAYER_KEY(keycode) is true. The release path should mirror this, 
      which requires either checking the locked keycode first, or checking IS_LAYER_KEY before calling the layer function. 
      Since layer keys are not tracked in s_pressed_keys, their keycode won't be in keycode_out after untrack_pressed_key — 
      the current code calls layer_key_released before untracking, which is correct for layer keys, but the unconditional call 
      is still inconsistent with the press path.
- Determine if this comment is still applicable:
    Establish the double-buffer for send_keyboard_report(). 
    The ATOMIC_BLOCK approach you documented in §11.5 is the right call — 8 bytes, ~16 cycles, negligible. 
    Do this before you write any endpoint flush code, not after. The race window is small but real.
- Implement keyboard.c key processing loop — the key_state_t locked-keycode design is already fully specified. 
    The SRAM cost is 180 bytes (90 keys × 2 bytes), which is negligible.
- keymap.c — fill in the actual keymap array based on kicad wiring
- Time TCB0_INT_vect to 200us prior to expected SOF arrival (800 us after prior SOF arrival)
- look into an opportunity to move the boot magic define constants to a central location, perhaps under KeyDU.BL/bootmagic.h
- look into the following opportunity to fix an inconsistency: 
    layer.c uses shift-down removal instead of swap-with-last
      layer_key_released() shifts remaining entries down one slot — O(n) with a nested loop. 
      keyboard.c's untrack_pressed_key() does the cleaner O(1) swap-with-last pattern. 
      Since both arrays are tiny (max 4 vs max 90 entries) this is a style inconsistency 
      rather than a performance problem, but it's worth making them match.
