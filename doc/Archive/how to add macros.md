## How to Add New Macros

### For Simple Modifier+Key Macros (like Alt+F8):

1. Add to `keycode.h`:
```c
#define MC_XL_VBA       (MC_BASE | 0x08)  // Alt+F8
```

2. Add case in `macro.c`:
```c
case MC_XL_VBA:
    send_mod_key(MOD_LALT, KC_F8);
    break;
```

### For Complex Sequence Macros (like Alt, E, S):

1. Add to `keycode.h`:
```c
#define MC_XL_PSTSP     (MC_BASE | 0x09)  // Alt,E,S
```

2. Define sequence in `macro.c`:
```c
static const macro_action_t PROGMEM macro_excel_paste_special[] = {
    MT(KC_LALT), MT(KC_E), MT(KC_S), MACRO_END
};
```

3. Add case in `execute_macro()`:
```c
case MC_XL_PSTSP:
    run_macro_sequence(macro_excel_paste_special);
    break;
```