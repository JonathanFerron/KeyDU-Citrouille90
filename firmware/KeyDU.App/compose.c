/* compose.c — Citrouille90 / AVR64DU32
   SPDX-License-Identifier: GPL-3.0-or-later
   Copyright (C) 2026 Jonathan Ferron

   Host-layout accented and special character primitives.

   Three primitives built on send_mod_key() from keyboard.h:

     compose_direct(kc)         — AltGr + kc
     compose_direct_s(kc)       — AltGr + Shift + kc
     compose_accented(acc, kc)  — AltGr + acc (dead key), then tap kc

   Non-inline so the compiler emits each once.  The static inline wrappers
   in compose.h call through to these; any wrapper never called is dropped
   before linking.  If no compose_* function is used at all, this file
   produces no object code.


*/

#include "compose_internal.h"
#include "keyboard.h"
#include "keycode.h"

/* compose_direct — AltGr + kc */
void compose_direct(uint8_t kc)
{ send_mod_key(MOD_RALT, kc);
}

/* compose_direct_s — AltGr + Shift + kc */
void compose_direct_s(uint8_t kc)
{ send_mod_key(MOD_RALT | MOD_LSFT, kc);
}

/* compose_accented — AltGr + accent_key (dead key), then tap letter_key.
   The host layout resolves the pair to the accented character.
   send_mod_key() calls kbd_stage() after press and after release for each
   key, so four report entries are generated in total. */
void compose_accented(uint8_t accent_key, uint8_t letter_key)
{ send_mod_key(MOD_RALT, accent_key);   /* fire the dead key  */
  send_mod_key(0,        letter_key);   /* tap the base letter */
}


/* execute_compose — called from process_key_press() in keyboard.c
   on any CP_* keycode.  Mirrors execute_macro() in macro.c.
   Unused cases are dead-code eliminated by the compiler. */
void execute_compose(uint16_t keycode)
{ switch(keycode)
  {

    /* Compose — accented characters via CMS host layout dead keys.
       Add CP_* entries to keycode.h and assign them
       in keymap.c as needed.  Unused cases are dead-code eliminated. */

    /* French everyday set */
    //case CP_E_ACU:  compose_e_acute();  break;   /* é */
    //case CP_E_GRV:  compose_e_grave();  break;   /* è */
    //case CP_E_CIR:  compose_e_circ();   break;   /* ê */
    //case CP_E_DIA:  compose_e_diaer();  break;   /* ë */
    //case CP_A_GRV:  compose_a_grave();  break;   /* à */
    //case CP_A_CIR:  compose_a_circ();   break;   /* â */
    //case CP_U_GRV:  compose_u_grave();  break;   /* ù */
    //case CP_U_CIR:  compose_u_circ();   break;   /* û */
    //case CP_U_DIA:  compose_u_diaer();  break;   /* ü */
    //case CP_I_CIR:  compose_i_circ();   break;   /* î */
    //case CP_I_DIA:  compose_i_diaer();  break;   /* ï */
    //case CP_O_CIR:  compose_o_circ();   break;   /* ô */
    //case CP_C_CED:  compose_c_ced();    break;   /* ç */
    //case CP_OE:     compose_oe();       break;   /* œ */
    //case CP_AE:     compose_ae();       break;   /* æ */

    /* Uppercase */
    //case CP_E_ACU_U: compose_E_acute(); break;   /* É */
    //case CP_E_GRV_U: compose_E_grave(); break;   /* È */
    //case CP_C_CED_U: compose_C_ced();   break;   /* Ç */

    default:
      break;
  }
}
