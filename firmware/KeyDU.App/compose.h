/* compose.h — Citrouille90 / AVR64DU32
   SPDX-License-Identifier: GPL-3.0-or-later
   Copyright (C) 2026 Jonathan Ferron

   Host-layout accented and special character helpers.

   Every function emits the HID sequence that produces the named character
   when the host keyboard layout is set to Canadian Multilingual Standard
   (CMS), on both Windows and Linux.

   To retarget to a different host layout (e.g. US International), replace
   the COMPOSE_DK_* constants and direct key mappings below.  All call
   sites in macro.c and the keymap are unchanged.

   Two helpers are declared in compose_internal.h and defined in compose.c:

     compose_direct(kc)         — AltGr + kc
     compose_direct_s(kc)       — AltGr + Shift + kc
     compose_accented(acc, kc)  — AltGr + acc (fires dead key on host),
                                  then tap kc (host resolves accented char)

   All per-character functions are static inline.  The compiler drops any
   function never called — unused entries cost zero flash.

   ── CMS dead-key positions (AltGr + key → dead accent) ──────────────────

     AltGr + ;    → dead acute       ´   (KC_SCLN)
     AltGr + [    → dead circumflex  ^   (KC_LBRC)
     AltGr + '    → dead diaeresis   ¨   (KC_QUOT)
     AltGr + `    → dead grave       `   (KC_GRV)
     AltGr + =    → dead cedilla     ¸   (KC_EQL)
     AltGr + ]    → dead macron/tilde ¯  (KC_RBRC)
     AltGr + 0    → dead ring        ˚   (KC_0)
     AltGr + .    → dead dot above   ·   (KC_DOT)

   ── CMS direct AltGr characters (representative set) ────────────────────

     AltGr + E/e  → œ / Œ      AltGr + A/a  → æ / Æ
     AltGr + O/o  → ø / Ø      AltGr + S    → ß
     AltGr + D/d  → ð / Ð      AltGr + P/p  → þ / Þ
     AltGr + W/w  → ł / Ł      AltGr + T/t  → ŧ / Ŧ
     AltGr + B    → "  (ldquo) AltGr + V    → "  (rdquo)
     AltGr + B(S) → '  (lsquo) AltGr + V(S) → '  (rsquo)
     AltGr + Z(S) → «          AltGr + X(S) → »
     AltGr + Y    → ←          AltGr + U    → ↓
     AltGr + U(S) → ↑          AltGr + Y(S) → ¥
     AltGr + C    → ¢          AltGr + C(S) → ©
     AltGr + R    → ¶          AltGr + R(S) → ®
     AltGr + M    → µ          AltGr + Q(S) → Ω

   Usage in macro.c / execute_macro():

     #include "compose.h"

     case MC_E_ACUTE:  compose_e_acute();  break;   // é
     case MC_E_GRAVE:  compose_e_grave();  break;   // è
     case MC_C_CED:    compose_c_ced();    break;   // ç
*/

#ifndef COMPOSE_H
#define COMPOSE_H

#include "keycode.h"
#include "compose_internal.h"

/* ── Dead-key accent positions on CMS host layout ───────────────────────── */
#define COMPOSE_DK_ACUTE    KC_SCLN   /* AltGr + ;  → dead acute    ´ */
#define COMPOSE_DK_CIRC     KC_LBRC   /* AltGr + [  → dead circ     ^ */
#define COMPOSE_DK_DIAER    KC_QUOT   /* AltGr + '  → dead diaeresis ¨ */
#define COMPOSE_DK_GRAVE    KC_GRV    /* AltGr + `  → dead grave     ` */
#define COMPOSE_DK_CED      KC_EQL    /* AltGr + =  → dead cedilla   ¸ */
#define COMPOSE_DK_MACRON   KC_RBRC   /* AltGr + ]  → dead macron    ¯ */
#define COMPOSE_DK_TILDE    KC_RBRC   /* AltGr + ]  → dead tilde     ~ (same key as macron on CMS) */
#define COMPOSE_DK_RING     KC_0      /* AltGr + 0  → dead ring      ˚ */
#define COMPOSE_DK_DOT      KC_DOT    /* AltGr + .  → dead dot above · */

/* ============================================================================
   Direct AltGr characters — one compose_direct() call each
   ========================================================================= */

void execute_compose(uint16_t keycode);

/* Ligatures */
static inline void compose_oe(void)
{ compose_direct(KC_E);  /* œ */
}
static inline void compose_OE(void)
{ compose_direct_s(KC_E);  /* Œ */
}
static inline void compose_ae(void)
{ compose_direct(KC_A);  /* æ */
}
static inline void compose_AE(void)
{ compose_direct_s(KC_A);  /* Æ */
}

/* Scandinavian / European */
static inline void compose_o_stroke(void)
{ compose_direct(KC_O);  /* ø */
}
static inline void compose_O_stroke(void)
{ compose_direct_s(KC_O);  /* Ø */
}
static inline void compose_sz(void)
{ compose_direct(KC_S);  /* ß */
}
static inline void compose_eth(void)
{ compose_direct(KC_D);  /* ð */
}
static inline void compose_ETH(void)
{ compose_direct_s(KC_D);  /* Ð */
}
static inline void compose_thorn(void)
{ compose_direct(KC_P);  /* þ */
}
static inline void compose_THORN(void)
{ compose_direct_s(KC_P);  /* Þ */
}
static inline void compose_tbar(void)
{ compose_direct(KC_T);  /* ŧ */
}
static inline void compose_TBAR(void)
{ compose_direct_s(KC_T);  /* Ŧ */
}
static inline void compose_lstr(void)
{ compose_direct(KC_W);  /* ł */
}
static inline void compose_LSTR(void)
{ compose_direct_s(KC_W);  /* Ł */
}
static inline void compose_dotless_i(void)
{ compose_direct(KC_I);  /* ı */
}
static inline void compose_eng(void)
{ compose_direct(KC_G);  /* ŋ */
}
static inline void compose_ENG(void)
{ compose_direct_s(KC_G);  /* Ŋ */
}
static inline void compose_hbar(void)
{ compose_direct(KC_H);  /* ħ */
}
static inline void compose_HBAR(void)
{ compose_direct_s(KC_H);  /* Ħ */
}
static inline void compose_ij(void)
{ compose_direct(KC_J);  /* ĳ */
}
static inline void compose_IJ(void)
{ compose_direct_s(KC_J);  /* Ĳ */
}
static inline void compose_kra(void)
{ compose_direct(KC_K);  /* ĸ */
}
static inline void compose_ldot(void)
{ compose_direct(KC_L);  /* ŀ */
}
static inline void compose_LDOT(void)
{ compose_direct_s(KC_L);  /* Ŀ */
}

/* Typography */
static inline void compose_mu(void)
{ compose_direct(KC_M);  /* µ */
}
static inline void compose_pilcrow(void)
{ compose_direct(KC_R);  /* ¶ */
}
static inline void compose_cent(void)
{ compose_direct(KC_C);  /* ¢ */
}
static inline void compose_copyright(void)
{ compose_direct_s(KC_C);  /* © */
}
static inline void compose_reg(void)
{ compose_direct_s(KC_R);  /* ® */
}
static inline void compose_ldquo(void)
{ compose_direct(KC_B);  /* " */
}
static inline void compose_rdquo(void)
{ compose_direct(KC_V);  /* " */
}
static inline void compose_lsquo(void)
{ compose_direct_s(KC_B);  /* ' */
}
static inline void compose_rsquo(void)
{ compose_direct_s(KC_V);  /* ' */
}
static inline void compose_guill_open(void)
{ compose_direct_s(KC_Z);  /* « */
}
static inline void compose_guill_close(void)
{ compose_direct_s(KC_X);  /* » */
}
static inline void compose_ordf(void)
{ compose_direct_s(KC_F);  /* ª */
}
static inline void compose_ordm(void)
{ compose_direct_s(KC_M);  /* º */
}
static inline void compose_omega(void)
{ compose_direct_s(KC_Q);  /* Ω */
}

/* Arrows and symbols */
static inline void compose_arrow_l(void)
{ compose_direct(KC_Y);  /* ← */
}
static inline void compose_arrow_d(void)
{ compose_direct(KC_U);  /* ↓ */
}
static inline void compose_arrow_u(void)
{ compose_direct_s(KC_U);  /* ↑ */
}
static inline void compose_yen(void)
{ compose_direct_s(KC_Y);  /* ¥ */
}

/* ============================================================================
   Acute accent  ´  (AltGr+; then letter)
   ========================================================================= */
static inline void compose_a_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_A);  /* á */
}
static inline void compose_A_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_A);  /* Á */
}
static inline void compose_e_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_E);  /* é */
}
static inline void compose_E_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_E);  /* É */
}
static inline void compose_i_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_I);  /* í */
}
static inline void compose_I_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_I);  /* Í */
}
static inline void compose_o_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_O);  /* ó */
}
static inline void compose_O_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_O);  /* Ó */
}
static inline void compose_u_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_U);  /* ú */
}
static inline void compose_U_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_U);  /* Ú */
}
static inline void compose_y_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_Y);  /* ý */
}
static inline void compose_c_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_C);  /* ć */
}
static inline void compose_n_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_N);  /* ń */
}
static inline void compose_s_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_S);  /* ś */
}
static inline void compose_z_acute(void)
{ compose_accented(COMPOSE_DK_ACUTE, KC_Z);  /* ź */
}

/* ============================================================================
   Grave accent  `  (AltGr+` then letter)
   ========================================================================= */
static inline void compose_a_grave(void)
{ compose_accented(COMPOSE_DK_GRAVE, KC_A);  /* à */
}
static inline void compose_A_grave(void)
{ compose_accented(COMPOSE_DK_GRAVE, KC_A);  /* À */
}
static inline void compose_e_grave(void)
{ compose_accented(COMPOSE_DK_GRAVE, KC_E);  /* è */
}
static inline void compose_E_grave(void)
{ compose_accented(COMPOSE_DK_GRAVE, KC_E);  /* È */
}
static inline void compose_i_grave(void)
{ compose_accented(COMPOSE_DK_GRAVE, KC_I);  /* ì */
}
static inline void compose_o_grave(void)
{ compose_accented(COMPOSE_DK_GRAVE, KC_O);  /* ò */
}
static inline void compose_u_grave(void)
{ compose_accented(COMPOSE_DK_GRAVE, KC_U);  /* ù */
}
static inline void compose_U_grave(void)
{ compose_accented(COMPOSE_DK_GRAVE, KC_U);  /* Ù */
}

/* ============================================================================
   Circumflex  ^  (AltGr+[ then letter)
   ========================================================================= */
static inline void compose_a_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_A);  /* â */
}
static inline void compose_A_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_A);  /* Â */
}
static inline void compose_e_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_E);  /* ê */
}
static inline void compose_E_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_E);  /* Ê */
}
static inline void compose_i_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_I);  /* î */
}
static inline void compose_I_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_I);  /* Î */
}
static inline void compose_o_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_O);  /* ô */
}
static inline void compose_O_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_O);  /* Ô */
}
static inline void compose_u_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_U);  /* û */
}
static inline void compose_U_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_U);  /* Û */
}
static inline void compose_c_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_C);  /* ĉ */
}
static inline void compose_g_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_G);  /* ĝ */
}
static inline void compose_h_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_H);  /* ĥ */
}
static inline void compose_j_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_J);  /* ĵ */
}
static inline void compose_s_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_S);  /* ŝ */
}
static inline void compose_w_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_W);  /* ŵ */
}
static inline void compose_y_circ(void)
{ compose_accented(COMPOSE_DK_CIRC, KC_Y);  /* ŷ */
}

/* ============================================================================
   Diaeresis  ¨  (AltGr+' then letter)
   ========================================================================= */
static inline void compose_a_diaer(void)
{ compose_accented(COMPOSE_DK_DIAER, KC_A);  /* ä */
}
static inline void compose_A_diaer(void)
{ compose_accented(COMPOSE_DK_DIAER, KC_A);  /* Ä */
}
static inline void compose_e_diaer(void)
{ compose_accented(COMPOSE_DK_DIAER, KC_E);  /* ë */
}
static inline void compose_E_diaer(void)
{ compose_accented(COMPOSE_DK_DIAER, KC_E);  /* Ë */
}
static inline void compose_i_diaer(void)
{ compose_accented(COMPOSE_DK_DIAER, KC_I);  /* ï */
}
static inline void compose_I_diaer(void)
{ compose_accented(COMPOSE_DK_DIAER, KC_I);  /* Ï */
}
static inline void compose_o_diaer(void)
{ compose_accented(COMPOSE_DK_DIAER, KC_O);  /* ö */
}
static inline void compose_O_diaer(void)
{ compose_accented(COMPOSE_DK_DIAER, KC_O);  /* Ö */
}
static inline void compose_u_diaer(void)
{ compose_accented(COMPOSE_DK_DIAER, KC_U);  /* ü */
}
static inline void compose_U_diaer(void)
{ compose_accented(COMPOSE_DK_DIAER, KC_U);  /* Ü */
}
static inline void compose_y_diaer(void)
{ compose_accented(COMPOSE_DK_DIAER, KC_Y);  /* ÿ */
}

/* ============================================================================
   Cedilla  ¸  (AltGr+= then letter)
   ========================================================================= */
static inline void compose_c_ced(void)
{ compose_accented(COMPOSE_DK_CED, KC_C);  /* ç */
}
static inline void compose_C_ced(void)
{ compose_accented(COMPOSE_DK_CED, KC_C);  /* Ç */
}
static inline void compose_g_ced(void)
{ compose_accented(COMPOSE_DK_CED, KC_G);  /* ģ */
}
static inline void compose_k_ced(void)
{ compose_accented(COMPOSE_DK_CED, KC_K);  /* ķ */
}
static inline void compose_l_ced(void)
{ compose_accented(COMPOSE_DK_CED, KC_L);  /* ļ */
}
static inline void compose_n_ced(void)
{ compose_accented(COMPOSE_DK_CED, KC_N);  /* ņ */
}
static inline void compose_r_ced(void)
{ compose_accented(COMPOSE_DK_CED, KC_R);  /* ŗ */
}
static inline void compose_s_ced(void)
{ compose_accented(COMPOSE_DK_CED, KC_S);  /* ş */
}
static inline void compose_t_ced(void)
{ compose_accented(COMPOSE_DK_CED, KC_T);  /* ţ */
}

/* ============================================================================
   Tilde  ~  (AltGr+] then letter — shares key with macron on CMS)
   ========================================================================= */
static inline void compose_a_tilde(void)
{ compose_accented(COMPOSE_DK_TILDE, KC_A);  /* ã */
}
static inline void compose_A_tilde(void)
{ compose_accented(COMPOSE_DK_TILDE, KC_A);  /* Ã */
}
static inline void compose_n_tilde(void)
{ compose_accented(COMPOSE_DK_TILDE, KC_N);  /* ñ */
}
static inline void compose_N_tilde(void)
{ compose_accented(COMPOSE_DK_TILDE, KC_N);  /* Ñ */
}
static inline void compose_o_tilde(void)
{ compose_accented(COMPOSE_DK_TILDE, KC_O);  /* õ */
}
static inline void compose_O_tilde(void)
{ compose_accented(COMPOSE_DK_TILDE, KC_O);  /* Õ */
}
static inline void compose_i_tilde(void)
{ compose_accented(COMPOSE_DK_TILDE, KC_I);  /* ĩ */
}
static inline void compose_u_tilde(void)
{ compose_accented(COMPOSE_DK_TILDE, KC_U);  /* ũ */
}

/* ============================================================================
   Macron  ¯  (AltGr+] then letter — shares key with tilde on CMS)
   ========================================================================= */
static inline void compose_a_macron(void)
{ compose_accented(COMPOSE_DK_MACRON, KC_A);  /* ā */
}
static inline void compose_e_macron(void)
{ compose_accented(COMPOSE_DK_MACRON, KC_E);  /* ē */
}
static inline void compose_i_macron(void)
{ compose_accented(COMPOSE_DK_MACRON, KC_I);  /* ī */
}
static inline void compose_o_macron(void)
{ compose_accented(COMPOSE_DK_MACRON, KC_O);  /* ō */
}
static inline void compose_u_macron(void)
{ compose_accented(COMPOSE_DK_MACRON, KC_U);  /* ū */
}

/* ============================================================================
   Ring  ˚  (AltGr+0 then letter)
   ========================================================================= */
static inline void compose_a_ring(void)
{ compose_accented(COMPOSE_DK_RING, KC_A);  /* å */
}
static inline void compose_A_ring(void)
{ compose_accented(COMPOSE_DK_RING, KC_A);  /* Å */
}
static inline void compose_u_ring(void)
{ compose_accented(COMPOSE_DK_RING, KC_U);  /* ů */
}

/* ============================================================================
   Dot above  ·  (AltGr+. then letter)
   ========================================================================= */
static inline void compose_c_dot(void)
{ compose_accented(COMPOSE_DK_DOT, KC_C);  /* ċ */
}
static inline void compose_e_dot(void)
{ compose_accented(COMPOSE_DK_DOT, KC_E);  /* ė */
}
static inline void compose_g_dot(void)
{ compose_accented(COMPOSE_DK_DOT, KC_G);  /* ġ */
}
static inline void compose_i_dot(void)
{ compose_accented(COMPOSE_DK_DOT, KC_I);  /* İ */
}
static inline void compose_z_dot(void)
{ compose_accented(COMPOSE_DK_DOT, KC_Z);  /* ż */
}

#endif /* COMPOSE_H */
