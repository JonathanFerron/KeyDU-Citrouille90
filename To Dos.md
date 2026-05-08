# Citrouille90 To Dos

- Time TCB0_INT_vect to 200us prior to expected SOF arrival (800 us after prior SOF arrival)
- look into an opportunity to move the boot magic define constants to a central location, perhaps under KeyDU.BL/bootmagic.h
- look into the following opportunity to fix an inconsistency: 
    layer.c uses shift-down removal instead of swap-with-last
      layer_key_released() shifts remaining entries down one slot — O(n) with a nested loop. 
      keyboard.c's untrack_pressed_key() does the cleaner O(1) swap-with-last pattern. 
      Since both arrays are tiny (max 4 vs max 90 entries) this is a style inconsistency 
      rather than a performance problem, but it's worth making them match.
