# Custom UPDI and DFU host programming tools

### Strategy — Fork and strip avrdude

Clone the avrdude repo and delete everything you don't need, keeping only the serialUPDI programmer + the AVR64DU32 part definition. This is far less work than it sounds, and you end up with something that is still a recognizable avrdude build but trivially small.

---

## What to keep and what to delete

### Source files to keep from `src/`

These are the files that actually implement the serialUPDI path:

**UPDI protocol stack (core):**
- `serialupdi.c/.h` — top-level programmer vtable
- `updi_link.c/.h` — serial framing, double-break, UART open/close
- `updi_state.c/.h` — programmer state struct
- `updi_readwrite.c/.h` — CS/DS read/write primitives
- `updi_nvm.c/.h` — NVM operations (erase, write page) — this is where NVM model dispatch lives
- `updi_nvm_v0.c/.h`, `updi_nvm_v2.c/.h` — model implementations (AVR64DU32 uses v2)
- `updi_constants.h` — protocol constants

**Serial port layer:**
- `ser_posix.c` — POSIX serial (Linux only, drop `ser_win32.c` entirely)
- `serdev.h`

**Part/programmer registry (unavoidable but can be gutted):**
- `avrdude.c` — main, CLI parsing
- `avrdude.h`, `libavrdude.h`
- `config.c/.h`, `config_gram.y`, `lexer.l` — conf file parser (or replace with hardcoded part struct)
- `pgm.c/.h`, `pgm_type.c/.h` — programmer vtable
- `avrpart.c/.h` — part descriptor
- `avr.c/.h` — generic memory read/write dispatch
- `mem.c/.h` — memory type structs
- `fileio.c/.h` — Intel HEX read/write
- `ihex.c/.h` (or `ihex.h` depending on version)

**Utilities:**
- `avrintel.c` — AVR part intelligence/signature lookup  
- `dryrun.c/.h` — safe no-op (used internally)
- `term.c/.h` — **delete this** (terminal mode you don't need)
- `bitbang.c/.h` — **delete** (ISP only)
- Everything named `stk500*`, `jtag*`, `isp*`, `jtagmkI*`, `dragon*`, `usbasp*`, `usbtiny*`, `flip*`, `micronucleus*`, etc. — **delete all of these**

### What to do about the conf file

`avrdude.conf.in` is ~25,000 lines. You need exactly one part entry. After stripping, replace the whole conf with a minimal hand-edited file containing:

```
programmer
  id = "serialupdi";
  desc = "SerialUPDI";
  type = "serialupdi";
  prog_modes = PM_UPDI;
  connection_type = serial;
  hvupdi_support = 1;
;

part
  id = "avr64du32";
  # ... copy the avr64du32 block verbatim from avrdude.conf.in ...
;
```

This avoids rebuilding the yacc/lex parser for the conf, or you can hardcode the part struct in C and eliminate the parser entirely.

---

## Build system

avrdude uses CMake. For a stripped fork, replace it with a plain Makefile. The full build with all programmers pulls in libelf, libusb, libhidapi, libftdi — none of which you need. Your stripped build needs only:

```
-lm          # math (used internally)
```

No external libraries. No CMake. Pure gcc on Linux.

---

## Practical steps

```
# 1. Clone
git clone https://github.com/avrdudes/avrdude.git myupdi
cd myupdi/src

# 2. Delete irrelevant programmer files
rm -f stk500*.c stk500*.h
rm -f jtag*.c jtag*.h
rm -f isp.c isp.h
rm -f dragon*.c dragon*.h
rm -f usbasp.c usbasp.h usbtiny*.c usbtiny*.h
rm -f flip*.c flip*.h
rm -f micronucleus*.c micronucleus*.h
rm -f dfu.c dfu.h
rm -f ppi.c ppi.h linuxgpio.c linuxgpio.h
rm -f ser_win32.c  # Linux only build
rm -f term.c term.h  # no interactive terminal
# ... and so on for every programmer you don't need

# 3. Gut avrdude.conf.in down to serialupdi + avr64du32

# 4. Write a Makefile
```

---

One thing to verify: the `64du32` part entry in `avrdude.conf.in` should have `nvmver = 2;` or equivalent.

---

## Summary recommendation

1. Clone avrdude at the latest v8.x tag
2. Delete all programmer backends except the `updi_*` and `serialupdi` files and `ser_posix.c`
3. Replace `avrdude.conf.in` with a 50-line minimal conf
4. Write a plain Makefile targeting Linux only
5. Keep the yacc/lex parser (it's not worth replacing unless binary size matters)

The result will be a ~50KB binary that does exactly what you need, with zero external library dependencies, and you can put it in your project repo alongside the firmware Makefile.

---

## DFU programmer

**dfu-util is the right reference**, and it's specifically designed around USB-IF DFU 1.1 (and DFU suffix validation, dfuse extensions if you ever want them). More importantly for your use case, its USB transfer logic is relatively cleanly separated from the rest of the tool. The files you'd actually care about are:

- `src/dfu.c` / `dfu.h` — the core DFU state machine: DNLOAD, UPLOAD, GETSTATUS, CLRSTATUS, ABORT. This is the part you'd lift directly.
- `src/usb_dfu.h` — the DFU descriptor and request constants. Take this verbatim.
- `src/dfuse.c` — only relevant if you implement DfuSe (ST Micro's addressing extension). You almost certainly won't, since AVR DU flash is straightforward.

What you would **not** need from dfu-util: the device enumeration/scanning logic (`main.c`, `dfu_load.c`), the suffix handling (`suffix.c`), and the quirk table — all of that gets replaced by your own minimal open-by-VID/PID logic.

One thing worth knowing about implementing DFU in KeyDU.BL: standard DFU 1.1 has a specific expectation about transfer size negotiation via the `wTransferSize` field in the functional descriptor, and the host is supposed to respect it. Since you control both sides, you can set `wTransferSize = 512` (your page size) and write the host tool to match, which makes the download loop trivially clean — one DFU_DNLOAD per page, poll GETSTATUS until dfuDNBUSY clears, repeat.

The official repository is at:

**https://sourceforge.net/p/dfu-util/dfu-util/ci/master/tree/**

There's also a GitHub mirror that most people find easier to browse:

**https://github.com/Stefan-Schmidt/dfu-util**

Though for the canonical, up-to-date source the Sourceforge repo is authoritative. The project's homepage is **http://dfu-util.sourceforge.net/** which has release tarballs if you prefer that over cloning.

To clone:
```bash
git clone https://git.code.sf.net/p/dfu-util/dfu-util
```

Or from the GitHub mirror:
```bash
git clone https://github.com/Stefan-Schmidt/dfu-util
```
