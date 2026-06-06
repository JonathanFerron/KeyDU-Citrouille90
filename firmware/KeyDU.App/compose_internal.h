/* compose_internal.h — Citrouille90 / AVR64DU32
   SPDX-License-Identifier: GPL-3.0-or-later
   Copyright (C) 2026 Jonathan Ferron

   Internal helper declarations for compose.h.
   Not intended for direct inclusion — include compose.h instead.

   Three primitives, each defined in compose.c:

     compose_direct(kc)         — AltGr + kc
     compose_direct_s(kc)       — AltGr + Shift + kc
     compose_accented(acc, kc)  — AltGr + acc (dead key on host),
                                  then tap kc (host resolves accented char)

   These are non-inline so the compiler emits them once.  The static inline
   wrappers in compose.h call through to these; any wrapper never called is
   dropped before linking.

   Host layout assumption: Canadian Multilingual Standard (CMS) on both
   Windows and Linux.  To retarget to a different layout (e.g. US
   International), replace the COMPOSE_DK_* constants and direct key
   mappings in compose.h — the function names and call sites are unchanged.
*/

#ifndef COMPOSE_INTERNAL_H
#define COMPOSE_INTERNAL_H

#include <stdint.h>

void compose_direct(uint8_t kc);
void compose_direct_s(uint8_t kc);
void compose_accented(uint8_t accent_key, uint8_t letter_key);

#endif /* COMPOSE_INTERNAL_H */
