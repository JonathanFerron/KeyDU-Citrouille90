# CMS Layout Reference — Linux `ca(multix)`

**Host layout:** Canadian Multilingual Standard (CSA Z243.200).
**Source:** `symbols/ca`, `xkb_symbols "multix"`, from **`xkb-data 2.41`** (the package
Kubuntu 24.04 LTS ships). Extracted from the `.deb`, not transcribed from a chart.
**In the KDE layout selector this appears as "Canadian (CSA)".** If your screenshots
show a different name (e.g. "Canadian (multilingual, first part)" = `ca(multi)`, a
*different* variant), these tables do **not** apply — re-pull before trusting them.

This documents the **Linux flavour only**. The firmware targets the Linux chords for
tilde/ñ and otherwise the Base/Shift/AltGr intersection (see Portability below).

---

## 1. How each plane maps to a firmware emission

CMS populates five planes. Right Ctrl is bound to `ISO_Level5_Shift`, which is why two
of the planes are reachable on Linux but **not** by the same chord on Windows.

| Plane (column) | XKB level | Firmware emits | compose.h primitive | Portable to Win CMS |
| --- | --- | --- | --- | --- |
| **Base**   | 1 | `tap_key(KC_x)`                     | (plain tap)          | ✅ yes |
| **Shift**  | 2 | `send_mod_key(MOD_LSFT, KC_x)`      | `compose_shift`      | ✅ yes |
| **AltGr**  | 3 | `send_mod_key(MOD_RALT, KC_x)`      | `compose_direct`     | ⚠ see §6 |
| **RCtrl**  | 5 | `send_mod_key(MOD_RCTL, KC_x)`      | *(new, Linux-only)*  | ❌ Linux only |
| **⇧RCtrl** | 6 | `send_mod_key(MOD_RCTL\|MOD_LSFT, KC_x)` | *(new, Linux-only)* | ❌ Linux only |

The **AltGr+Shift** plane (level 4) is **empty on every key** — the plane the current
`compose_direct_s()` targets. Nothing lives there; re-aim or retire that primitive.
Levels 7–8 are unused on this layout.

`◌` = dead key (fires an accent, then the next letter resolves). Every emission above
goes through the report ring (non-blocking); a dead-key compose stages 4 reports, so it
must never run from a busy-wait path.

---

## 2. Positional matrices

Physical key = US-QWERTY position (the `KC_` your matrix scans). Columns are the five
planes from §1. `—` = unassigned.

### Number row

| Key | KC_ | Base | Shift | AltGr | RCtrl | ⇧RCtrl |
| --- | --- | --- | --- | --- | --- | --- |
| `` ` `` | `KC_GRV`  | /  | \  | \| | — | -  |
| 1 | `KC_1`    | 1  | !  | ±  | ¹ | ¡  |
| 2 | `KC_2`    | 2  | @  | @  | ² | —  |
| 3 | `KC_3`    | 3  | #  | £  | ³ | £  |
| 4 | `KC_4`    | 4  | $  | ¤  | ¼ | €  |
| 5 | `KC_5`    | 5  | %  | —  | ½ | ⅜  |
| 6 | `KC_6`    | 6  | ?  | —  | ¾ | ⅝  |
| 7 | `KC_7`    | 7  | &  | {  | — | ⅞  |
| 8 | `KC_8`    | 8  | *  | }  | — | ™  |
| 9 | `KC_9`    | 9  | (  | [  | — | ±  |
| 0 | `KC_0`    | 0  | )  | ]  | — | —  |
| `-` | `KC_MINS` | -  | _  | ½  | — | ¿  |
| `=` | `KC_EQL`  | =  | +  | ¬  | ◌¸ | ◌˛ |

### Top row

| Key | KC_ | Base | Shift | AltGr | RCtrl | ⇧RCtrl |
| --- | --- | --- | --- | --- | --- | --- |
| Q | `KC_Q`    | q | Q | — | — | Ω |
| W | `KC_W`    | w | W | — | ł | Ł |
| E | `KC_E`    | e | E | €  | œ | Œ |
| R | `KC_R`    | r | R | — | ¶ | ® |
| T | `KC_T`    | t | T | — | ŧ | Ŧ |
| Y | `KC_Y`    | y | Y | — | ← | ¥ |
| U | `KC_U`    | u | U | — | ↓ | ↑ |
| I | `KC_I`    | i | I | — | → | ı |
| O | `KC_O`    | o | O | §  | ø | Ø |
| P | `KC_P`    | p | P | ¶  | þ | Þ |
| `[` | `KC_LBRC` | ◌ˆ | ◌¨ | ◌` | — | ◌˚ |
| `]` | `KC_RBRC` | ç | Ç | ~  | ◌~ | ◌¯ |

### Home row

| Key | KC_ | Base | Shift | AltGr | RCtrl | ⇧RCtrl |
| --- | --- | --- | --- | --- | --- | --- |
| A | `KC_A`    | a | A | — | æ | Æ |
| S | `KC_S`    | s | S | — | ß | § |
| D | `KC_D`    | d | D | — | ð | Ð |
| F | `KC_F`    | f | F | — | — | ª |
| G | `KC_G`    | g | G | — | ŋ | Ŋ |
| H | `KC_H`    | h | H | — | ħ | Ħ |
| J | `KC_J`    | j | J | — | ĳ | Ĳ |
| K | `KC_K`    | k | K | — | ĸ | — |
| L | `KC_L`    | l | L | — | ŀ | Ŀ |
| `;` | `KC_SCLN` | ; | : | °  | ◌´ | ◌˝ |
| `'` | `KC_QUOT` | è | È | {  | — | ◌ˇ |
| `\` | `KC_BSLS` | à | À | }  | — | ◌˘ |

### Bottom row

| Key | KC_ | Base | Shift | AltGr | RCtrl | ⇧RCtrl |
| --- | --- | --- | --- | --- | --- | --- |
| ISO `<` | `KC_NUBS` | ù | Ù | °  | — | ¦ |
| Z | `KC_Z`    | z | Z | «  | — | — |
| X | `KC_X`    | x | X | »  | — | — |
| C | `KC_C`    | c | C | — | ¢ | © |
| V | `KC_V`    | v | V | — | " (U+201C) | ' (U+2018) |
| B | `KC_B`    | b | B | — | " (U+201D) | ' (U+2019) |
| N | `KC_N`    | n | N | — | ŉ | ♪ |
| M | `KC_M`    | m | M | µ  | µ | º |
| `,` | `KC_COMM` | , | ' | <  | ― | × |
| `.` | `KC_DOT`  | . | " | >  | · | ÷ |
| `/` | `KC_SLSH` | é | É | ◌´ | — | ◌˙ |

Also: **AltGr + Space = NBSP** (U+00A0).

`KC_NUBS` (0x64) is the extra ISO key between Left Shift and Z. It carries `ù`/`Ù` —
relevant only if the Citrouille90 build physically wires that position.

---

## 3. Dead keys

Fire the dead key (chord below), then tap the base letter; the host resolves the pair.

| Accent | Chord | KC_ | Portable |
| --- | --- | --- | --- |
| ◌ˆ circumflex | **plain tap** | `KC_LBRC` | ✅ both |
| ◌¨ diaeresis  | Shift | `KC_LBRC` | ✅ both |
| ◌` grave      | AltGr | `KC_LBRC` | ✅ both |
| ◌˚ ring       | ⇧RCtrl | `KC_LBRC` | ❌ Linux |
| ◌´ acute      | AltGr | `KC_SLSH` | ✅ both |
| ◌´ acute (alt)| RCtrl | `KC_SCLN` | ❌ Linux |
| ◌˝ dbl-acute  | ⇧RCtrl | `KC_SCLN` | ❌ Linux |
| ◌¸ cedilla    | RCtrl | `KC_EQL`  | ❌ Linux |
| ◌˛ ogonek     | ⇧RCtrl | `KC_EQL`  | ❌ Linux |
| ◌~ tilde      | RCtrl | `KC_RBRC` | ❌ Linux (your chosen flavour) |
| ◌¯ macron     | ⇧RCtrl | `KC_RBRC` | ❌ Linux |
| ◌ˇ caron      | ⇧RCtrl | `KC_QUOT` | ❌ Linux |
| ◌˘ breve      | ⇧RCtrl | `KC_BSLS` | ❌ Linux |
| ◌˙ dot-above  | ⇧RCtrl | `KC_SLSH` | ❌ Linux |

The three you need for French (**circumflex, diaeresis, grave**) are all on the
**portable** `KC_LBRC` planes. `COMPOSE_DK_CIRC = KC_LBRC` in the current header is the
right *key*; the current header has the wrong *plane* (it says AltGr; circumflex is a
**plain tap**).

---

## 4. French / everyday emit reference (stitching cheat-sheet)

Glyph-first, for hand-stitching `compose.h`. **Note how much is a direct key** — the
dedicated accented-letter keys mean most of the everyday set needs no compose sequence.

### Direct keys — no compose, just a tap

| Glyph | Emit | Glyph | Emit |
| --- | --- | --- | --- |
| é | `tap_key(KC_SLSH)` | É | `compose_shift(KC_SLSH)` |
| è | `tap_key(KC_QUOT)` | È | `compose_shift(KC_QUOT)` |
| à | `tap_key(KC_BSLS)` | À | `compose_shift(KC_BSLS)` |
| ù | `tap_key(KC_NUBS)` | Ù | `compose_shift(KC_NUBS)` |
| ç | `tap_key(KC_RBRC)` | Ç | `compose_shift(KC_RBRC)` |

### Composed (dead key + letter) — portable set

Pattern: `compose_accented(dead_key_chord, letter)`. The three dead keys below are all
on `KC_LBRC`; only the *plane* differs.

| Dead key | Firmware fires | then letter → |
| --- | --- | --- |
| circumflex | `tap_key(KC_LBRC)` (plain) | â ê î ô û → tap KC_A/E/I/O/U |
| diaeresis  | `compose_shift(KC_LBRC)`   | ä ë ï ö ü ÿ |
| grave      | `compose_direct(KC_LBRC)`  | (also reaches à è ù, but those are direct keys) |

So `ê` = plain `KC_LBRC` tap, then `KC_E`. Uppercase `Ê` = same dead key, then
`compose_shift(KC_E)` — this is the "case follows real shift inside compose" hook: the
compose action reads the live shift bit and shifts only the **letter** tap, not the dead
key.

---

## 5. Portable AltGr symbol bank (Feature 1 candidates)

AltGr-plane glyphs that are safe cross-platform (CSA-standard programmer symbols,
confirmed on the Windows chart for the bracket/brace/bar set):

| Glyph | Emit | Glyph | Emit |
| --- | --- | --- | --- |
| \| | `compose_direct(KC_GRV)`  | ¬ | `compose_direct(KC_EQL)` |
| {  | `compose_direct(KC_7)`    | }  | `compose_direct(KC_8)` |
| [  | `compose_direct(KC_9)`    | ]  | `compose_direct(KC_0)` |
| €  | `compose_direct(KC_E)`    | §  | `compose_direct(KC_O)` |

Note `{` `}` also appear at AltGr+`'` / AltGr+`\` (`KC_QUOT`/`KC_BSLS`) — duplicates;
prefer the number-row positions above for consistency with the Windows chart.

Everything in the **RCtrl / ⇧RCtrl** columns (arrows, Ω, œ/Œ, ß, «/», curly quotes,
µ, etc.) is **Linux-only by chord**. Fine for a Linux-first `CP_` entry, but tag each so
a future Windows/intersection pass knows to re-map or drop it.

---

## 6. Rules & caveats

1. **Right Ctrl is NOT a Ctrl modifier under CMS** on either OS — it's the group
   selector (`ISO_Level5_Shift` on Linux; the equivalent selector on Windows). Macros
   must use `MOD_LCTL` / `MOD_LALT` only. Never emit `MOD_RCTL` expecting "Ctrl";
   here it's used purely to reach the level-5/6 glyph planes.
2. **AltGr+Shift plane is empty** on every key. The old `compose_direct_s()`
   (AltGr+Shift) targets dead air — re-aim at `MOD_RCTL|MOD_LSFT` (⇧RCtrl) or retire it.
3. **AltGr plane is not fully identical to Windows.** The bracket/brace/bar/notsign set
   (§5) is portable; the AltGr *duplicates* (`± @ £ ½` on the number row, `° µ` etc.)
   reach the same glyph by a **different** chord on Windows, so treat them as Linux-only
   unless verified against a Windows CMS chart.
4. **`€` portable route is AltGr+E**, not ⇧RCtrl+4 (the `KC_4` slot is Linux-only).
5. **`~` / dead-tilde is Linux-only by design here.** On Linux it's RCtrl+`KC_RBRC`;
   Windows swaps tilde and dead-tilde across the AltGr/selector planes, so no single
   chord gives `~` on both. Per project decision, firmware caters to the Linux chord and
   accepts `~`/`ñ` not working on the locked Windows work machines.
6. **Non-blocking:** every compose emission stages multiple reports into the SPSC ring;
   with the ring at 16 slots a worst-case burst fits. No compose path may be reached from
   a busy-wait (relevant once `tap_keycode()` can route to `execute_compose()`).

---

*Cross-checked key-by-key against the extracted `symbols/ca`. Spot-check the Base/Shift
columns against your Kubuntu screenshots; if any cell disagrees, the shipped file on your
machine differs from `xkb-data 2.41` and this doc should be regenerated from it.*
