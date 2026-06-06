# KeyDU Development Cheat Sheet

AVR64DU32 · Curiosity Nano (pkobn_updi) · avrdude 8.x

---

## avrdude — Programmer Setup

```
Programmer : pkobn_updi   (Curiosity Nano nEDBG, UPDI mode)
Part       : avr64du32
Port       : usb
```

### Flash

```bash
# Full flash (BL + App merged) — use after any bootloader change
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U flash:w:build/KeyDU.merged.hex:i

# App only (no erase, BL already present)
avrdude -c pkobn_updi -p avr64du32 -P usb -D \
  -U flash:w:build/app/KeyDU.App.hex:i

# BL only (no erase)
avrdude -c pkobn_updi -p avr64du32 -P usb -D \
  -U flash:w:build/bl/KeyDU.BL.hex:i

# Read back full flash
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U flash:r:flash_readback.hex:i

# Verify flash matches hex file
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U flash:v:build/KeyDU.merged.hex:i
```

### Fuses

> **CRITICAL:** fuse numbering matches datasheet offset, NOT byte index in the 16-byte dump.
> avrdude also accepts alias names (e.g. `bootsize` instead of `fuse8`).

```bash
# Read all fuses (hex, one byte per line)
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U fuses:r:-:h

# Read all fuses to binary file + inspect
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U fuses:r:fuses.bin:r
xxd fuses.bin

# Write BOOTSIZE = 0x10 (8KB boot partition, 16 × 512B pages)
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U fuse8:w:0x10:m

# Write all project fuses in one shot
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U fuse0:w:0x00:m \
  -U fuse1:w:0x00:m \
  -U fuse2:w:0x00:m \
  -U fuse5:w:0xD8:m \
  -U fuse6:w:0x08:m \
  -U fuse7:w:0x00:m \
  -U fuse8:w:0x10:m

# Verify BOOTSIZE was written correctly
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U fuse8:v:0x10:m
```

### EEPROM

```bash
# Read EEPROM
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U eeprom:r:eeprom.bin:r
xxd eeprom.bin | head -4

# Clear EEPROM byte 0 (boot magic)
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U eeprom:w:0xFF:m

# Write full EEPROM from file
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U eeprom:w:eeprom.bin:r
```

### Other memories

```bash
# Read signature / serial number
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U sernum:r:-:h

# Read userrow (extra EEPROM page, survives chip erase)
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U userrow:r:userrow.bin:r

# Read bootrow (BL-private, accessible only from BOOT section)
avrdude -c pkobn_updi -p avr64du32 -P usb \
  -U bootrow:r:bootrow.bin:r
```

---

## AVR64DU32 Fuse Reference

From datasheet section 8.10. All are R/W unless noted.

| avrdude name | Alias      | DS offset | Default | KeyDU value | Description                                              |
| ------------ | ---------- | --------- | ------- | ----------- | -------------------------------------------------------- |
| `fuse0`      | `wdtcfg`   | 0x00      | 0x00    | 0x00        | Watchdog config (off)                                    |
| `fuse1`      | `bodcfg`   | 0x01      | 0x00    | 0x00        | Brownout detection (off)                                 |
| `fuse2`      | `osccfg`   | 0x02      | 0x00    | 0x00        | Oscillator: CLKSEL=0000=OSCHF                            |
| `fuse5`      | `syscfg0`  | 0x05      | 0xD0    | 0xD8        | See below                                                |
| `fuse6`      | `syscfg1`  | 0x06      | 0x00    | 0x08        | USBSINK=1 (bit3), SUT=0ms                                |
| `fuse7`      | `codesize` | 0x07      | 0x00    | 0x00        | APPCODE size in 512B blocks (0=APPCODE runs to FLASHEND) |
| `fuse8`      | `bootsize` | 0x08      | 0x00    | **0x10**    | BOOT size in 512B blocks (0x10 = 16×512 = 8KB)           |
| `fusea`      | `pdicfg`   | 0x0A      | 0x0003  | 0x0003      | UPDI lock (2 bytes)                                      |

### SYSCFG0 (fuse5) bit breakdown — KeyDU value 0xD8 = 1101\_1000

| Bits | Field      | Value | Meaning                                     |
| ---- | ---------- | ----- | ------------------------------------------- |
| 7:6  | CRCSRC     | 11    | NOCRC — no CRC scan on reset                |
| 5    | CRCSEL     | 0     | CRC16-CCITT                                 |
| 4    | UPDIPINCFG | 1     | PF7 = UPDI pin (factory default, must keep) |
| 3    | RSTPINCFG  | 1     | PF6 = external RESET with pull-up           |
| 1    | BROWSAVE   | 0     | Boot row erased on chip erase               |
| 0    | EESAVE     | 0     | EEPROM erased on chip erase                 |

> **Warning:** Factory default is 0xD0 (RSTPINCFG=0 = PF6 is GPIO). KeyDU sets RSTPINCFG=1 to enable the reset button on the Curiosity Nano and Citrouille90.

### Flash sections (BOOTSIZE=0x10, CODESIZE=0x00)

```
BOOTEND  = BOOTSIZE × 512  = 0x10 × 512 = 0x2000
CODEEND  = FLASHEND (CODESIZE=0 means APPCODE runs to end)

0x0000 ─── BOOT section   (8KB)  ← KeyDU.BL, IVSEL=1 for BL interrupts
0x2000 ─── APPCODE section (56KB) ← KeyDU.App, vectors at 0x2000 by default
0xFFFF ────────────────────────────
```

> After reset, the default vector table is at the **start of APPCODE** (0x2000).
> The BL sets `CPUINT.CTRLA |= CPUINT_IVSEL_bm` to relocate vectors to BOOT (0x0000).

### AVR64DU32 memory types summary

| avrdude name            | Size | Notes                                                       |
| ----------------------- | ---- | ----------------------------------------------------------- |
| `flash`                 | 64KB | Main flash: BOOT + APPCODE + APPDATA                        |
| `eeprom`                | 256B | Survives reset; erased by chip erase (unless EESAVE=1)      |
| `fuses`                 | 16B  | All fuseX as one logical block                              |
| `fuse0`–`fuse8`,`fusea` | 1–2B | Individual fuse bytes (see table above)                     |
| `prodsig` / `sigrow`    | —    | Factory read-only: signature, calibration, serial           |
| `sernum`                | 16B  | Unique device serial number (subset of prodsig)             |
| `bootrow`               | 256B | Extra page; BL-only write access; not erased by chip erase  |
| `userrow`               | 512B | Extra EEPROM page; read/write; **not** erased by chip erase |
| `sib`                   | —    | System info block: AVR family, silicon revision             |
| `lock`                  | —    | Lock bits                                                   |
| `signature`             | 3B   | Device signature: 0x1E 0x96 0x21 for AVR64DU32              |

---

## AVR Inspection Tools

### avr-nm — symbol table

```bash
# Show all symbols with addresses
avr-nm build/bl/KeyDU.BL.elf

# Find main, init, ISR symbols
avr-nm build/bl/KeyDU.BL.elf | grep -i "main\|init\|isr\|vect"

# Sort by address
avr-nm -n build/bl/KeyDU.BL.elf | head -30
```

### avr-objdump — disassembly and inspection

```bash
# Full disassembly
avr-objdump -D build/bl/KeyDU.BL.elf | head -60

# Disassemble only code sections
avr-objdump -d build/bl/KeyDU.BL.elf

# Show all section headers
avr-objdump -h build/bl/KeyDU.BL.elf

# Show specific section content
avr-objdump -s -j .fuse build/bl/KeyDU.BL.elf
avr-objdump -s -j .text build/bl/KeyDU.BL.elf

# Disassemble a hex file (use avr:6 for AVR Dx/Ex/DU family)
avr-objdump -D -m avr:6 flash_readback.hex | head -40

# Show symbol addresses around a specific address
avr-objdump -D build/bl/KeyDU.BL.elf | grep -A 20 "^.*2fe:"
```

### avr-size — memory usage

```bash
avr-size --format=avr --mcu=avr64du32 build/bl/KeyDU.BL.elf
avr-size --format=avr --mcu=avr64du32 build/app/KeyDU.App.elf
```

---

## Hex / Binary Inspection

### xxd

```bash
xxd fuses.bin               # hex dump
xxd fuses.bin | head -4     # first 4 lines only
xxd -l 32 fuses.bin         # first 32 bytes only
```

### srec_info — Intel HEX file analysis

```bash
# Show address ranges and format
srec_info build/KeyDU.merged.hex -Intel

# Expected output for a good merged hex:
# Data:   0000 - 02FE    (BL code)
# Data:   2000 - 25CB    (App code)
```

---

## Linux Shell Tools

### dmesg — kernel messages

```bash
dmesg -w &              # watch in background, see USB events live
dmesg | tail -20        # last 20 lines
dmesg | grep -i "usb\|hid\|error"
```

### lsusb — list USB devices

```bash
lsusb                   # quick list
lsusb -t                # tree view showing bus topology
lsusb -v 2>/dev/null | grep -A 5 "idVendor"   # verbose, suppress errors
lsusb | grep -i "1209\|03eb\|keydu\|atmel"     # filter by known VID
```

### find

```bash
find /usr/lib/avr -name "*64du*"              # find avr64du32 support files
find /usr/lib/avr -name "crt*.o" | grep xmega2   # find CRT objects
find . -name "*.c" | xargs grep -l "BOOTSIZE" # find files containing a symbol
```

### grep

```bash
grep -i "pattern"           # case insensitive
grep -r "pattern" dir/      # recursive
grep -A 10 "pattern"        # 10 lines after match
grep -B 2 "pattern"         # 2 lines before match
grep -v "pattern"           # invert: show lines NOT matching
grep -l "pattern" *.c       # show only filenames
grep -n "pattern" file.c    # show line numbers
```

### Redirect / suppress

```bash
command 2>/dev/null         # suppress stderr
command 2>&1 | grep foo     # merge stderr into stdout, then grep
command 2>&1 | head -20     # merge and show first 20 lines
```

### head / tail

```bash
head -20 file               # first 20 lines
tail -20 file               # last 20 lines
command | tail -5           # last 5 lines of command output
```

---

## USB Monitoring (usbmon + Wireshark)

```bash
# Load usbmon kernel module
sudo modprobe usbmon

# Verify it loaded
lsmod | grep usbmon

# Find which USB bus the device is on
lsusb -t

# Install Wireshark
sudo apt install wireshark
sudo usermod -aG wireshark $USER
newgrp wireshark            # activate group without logout
```

In Wireshark:

- Capture interface: `usbmon1` (match bus number from `lsusb -t`)
- Display filter for enumeration: `usb.addr contains "1."` or no filter
- Look for: `GET DESCRIPTOR Request DEVICE` → `Response` → `SET ADDRESS` → `GET DESCRIPTOR CONFIGURATION`
- Failure modes:

| dmesg message                          | Cause                                                             |
| -------------------------------------- | ----------------------------------------------------------------- |
| No dmesg at all                        | D+ never pulled high — USB_ATTACH_bm not set, or clock/VREG issue |
| `device descriptor read/8, error -71`  | Device not responding — USB peripheral not initialised            |
| `device not accepting address`         | EP0 not responding — descriptor or stack issue                    |
| `device descriptor read/64, error -32` | Device reset mid-transfer                                         |

---

## Make Targets (KeyDU)

```bash
make                    # build both BL and App
make bl                 # build bootloader only
make app                # build app only
make merged             # build and merge into single hex
make clean              # remove all build artefacts
make size               # build and print section sizes
make flash_merged       # UPDI: program merged hex (full reflash)
make flash_app          # UPDI: program App only (no erase)
make flash_bl           # UPDI: program BL only (no erase)
make flash_usb          # USB vendor protocol: program App via BL
make flash_fuses        # write fuses via avrdude
make format             # run astyle on all source files
make bear               # regenerate compile_commands.json
```

---

# Kate for development

For push/pull, the plugin has basic support but many people find it more natural to just use the built-in terminal panel for those two operations.

## Git workflow recommendation

Use Kate's Git panel for the visual diff/stage/commit cycle, and use the terminal for push, pull, clone, and anything branchy. The commands you'll use 95% of the time are maybe 6-8 total.

The rough mental model coming from GitHub Desktop:

| GitHub Desktop action | Terminal equivalent       |
| --------------------- | ------------------------- |
| Clone                 | `git clone <url>`         |
| Pull                  | `git pull`                |
| Stage all             | `git add .`               |
| Stage file            | `git add filename`        |
| Commit                | `git commit -m "message"` |
| Push                  | `git push`                |
| See status            | `git status`              |
| See log               | `git log --oneline`       |

## Your actual workflow in the terminal is just:

```bash
git pull
# do your work
git diff
git add .        # stage everything (what GitHub Desktop did silently)
git status
git commit -m "message"
git push
```

## One-time git config

```bash
git config --global user.name "Your Name"
git config --global user.email "you@example.com"
git config --global credential.helper store   # saves your token after first use
```

For GitHub auth, use a **Personal Access Token** (classic, with repo scope) as your password the first time you push — `credential.helper store` will cache it so you never type it again.
