# Citrouille90 Git Repository Structure

```
git/
│
├── README.md                           # Project overview, quick start
├── LICENSE                             # Open source license
├── .gitignore                          # Ignore build artifacts, hex files
│
├── docs/                               # Documentation
│   ├── README.md                       # Documentation index
│   ├── TECHNICAL_SPEC.md               # Complete technical specification
│   ├── BUILDING.md                     # Firmware build instructions
│   ├── FLASHING.md                     # Bootloader entry, flashing guide
│   ├── CUSTOMIZATION.md                # How to customize keymaps
│   ├── API.md                          # Keyboard firmware API reference
│   ├── RECOVERY.md                     # UPDI unbrick procedure
│   ├── BOM.md                          # Bill of materials
│   ├── ASSEMBLY.md                     # PCB assembly instructions
│   └── images/                         # Photos, diagrams
│       ├── pcb_layout.png
│       ├── assembled.jpg
│       └── schematic_overview.png
│
├── hardware/                           # PCB design files
│   ├── README.md                       # Hardware documentation
│
├── case/                               # 3D printed case
│   ├── README.md                       # Case assembly instructions
│   ├── stl/                            # STL files for printing
│   │   ├── top_case.stl
│   │   ├── bottom_case.stl
│   │   ├── plate.stl
│   │   └── encoder_knob.stl
│   ├── cad/                            # CAD source files
│   │   ├── AmberClick90_case.f3d       # Fusion 360 source
│   │   └── AmberClick90_case.step      # STEP export
│   └── printing_notes.md               # Filament, print settings notes
│
├── firmware/                           # Keyboard firmware
│   ├── README.md                       # Firmware overview
│   ├── Makefile                        # Build system
│   ├── config.mk                       # Build configuration
│   │
│   ├── src/                            # Active source code
│   │   ├── main.c                      # Entry point, main loop
│   │   ├── config.h                    # Firmware configuration constants
│   │   ├── keymap.c                    # User keymap definitions
│   │   ├── macros.c                    # User active macros
│   │   ├── encoder.c                   # Encoder behavior implementation
│   │   ├── matrix.c                    # Matrix scanning logic
│   │   ├── matrix.h
│   │   ├── debounce.c                  # Debouncing algorithm
│   │   ├── debounce.h
│   │   ├── layers.c                    # Layer switching logic
│   │   ├── layers.h
│   │   ├── keyboard.c                  # Keyboard state management
│   │   ├── keyboard.h
│   │   ├── usb_hid.c                   # USB HID keyboard implementation
│   │   ├── usb_hid.h
│   │   ├── bootloader.c                # Bootloader entry logic
│   │   ├── bootloader.h
│   │   ├── hal_gpio.c                  # GPIO hardware abstraction
│   │   ├── hal_gpio.h
│   │   ├── hal_timer.c                 # Timer/PWM abstraction
│   │   ├── hal_timer.h
│   │   ├── led.c                       # LED control, PWM
│   │   ├── led.h
│   │   └── util.c                      # Utility functions
│   │       └── util.h
│   │
│   ├── mcc_generated/                  # MCC USB stack
│   │   ├── usb/                        # USB stack (trimmed)
│   │   │   ├── usb_device.c
│   │   │   ├── usb_device.h
│   │   │   ├── usb_hid.c
│   │   │   └── usb_hid.h
│   │   └── system/                     # System initialization
│   │       ├── clock.c
│   │       └── pins.c
│   │
│   ├── bootloader/                     # Bootloader firmware
│   │   ├── README.md                   # Bootloader documentation
│   │   ├── Makefile                    # Bootloader build
│   │   ├── main.c                      # Bootloader entry point
│   │   ├── usb_vendor.c                # USB vendor protocol
│   │   ├── usb_vendor.h
│   │   ├── flash.c                     # Flash write routines
│   │   ├── flash.h
│   │   └── linker.ld                   # Bootloader linker script
│   │
│   ├── lib/                            # Macro/encoder library (NOT compiled)
│   │   ├── README.md                   # Library usage guide
│   │   │
│   │   ├── macros/                     # Macro library
│   │   │   ├── README.md               # Macro library index
│   │   │   ├── basic/
│   │   │   │   ├── copy_paste.c        # Ctrl+C, Ctrl+V, Ctrl+X
│   │   │   │   ├── text_editing.c      # Line operations, word nav
│   │   │   │   └── navigation.c        # Home/End combos
│   │   │   ├── media/
│   │   │   │   ├── playback.c          # Play/pause, next/prev
│   │   │   │   └── volume.c            # Volume up/down, mute
│   │   │   ├── window_management/
│   │   │   │   ├── alt_tab.c           # Alt-Tab implementations
│   │   │   │   ├── virtual_desktops.c  # Desktop switching
│   │   │   │   └── tiling.c            # Window snapping
│   │   │   ├── development/
│   │   │   │   ├── ide_shortcuts.c     # VSCode, IntelliJ shortcuts
│   │   │   │   ├── terminal.c          # Terminal commands
│   │   │   │   └── git.c               # Git shortcuts
│   │   │   ├── productivity/
│   │   │   │   ├── email.c             # Email signatures
│   │   │   │   ├── zoom.c              # Zoom meeting shortcuts
│   │   │   │   └── browser.c           # Tab management
│   │   │   └── custom/
│   │   │       └── template.c          # Empty macro template
│   │   │
│   │   ├── encoders/                   # Encoder library
│   │   │   ├── README.md               # Encoder library index
│   │   │   ├── basic_volume.c          # Simple volume control
│   │   │   ├── smart_alt_tab.c         # Alt-Tab with hold
│   │   │   ├── scroll_wheel.c          # Mouse wheel emulation
│   │   │   ├── zoom_control.c          # Browser zoom
│   │   │   ├── brightness.c            # Screen brightness
│   │   │   ├── timeline.c              # Media scrubbing
│   │   │   └── template.c              # Empty encoder template
│   │   │
│   │   └── keymaps/                    # Example keymaps
│   │       ├── default/
│   │       │   ├── keymap.c            # Default layout
│   │       │   └── README.md           # Layout description
│   │       ├── programmer/
│   │       │   ├── keymap.c            # Programmer-optimized layout
│   │       │   └── README.md
│   │       ├── gaming/
│   │       │   ├── keymap.c            # Gaming layout
│   │       │   └── README.md
│   │       └── vim_user/
│   │           ├── keymap.c            # Vim-optimized layout
│   │           └── README.md
│   │
│   └── build/                          # Build output (gitignored)
│       ├── AmberClick90.hex
│       ├── AmberClick90.elf
│       └── *.o
│
├── tools/                              # Development tools
│   ├── README.md                       # Tools documentation
│   ├── flash/                          # Flashing tools
│   │   ├── amberclick-flash            # USB bootloader flasher (Linux/macOS)
│   │   ├── amberclick-flash.exe        # USB bootloader flasher (Windows)
│   │   ├── flash.c                     # Flasher source (LibUSB)
│   │   ├── flash.h
│   │   ├── Makefile                    # Flasher build
│   │   └── README.md                   # Flashing tool usage
│   ├── updi/                           # UPDI programming
│   │   ├── flash_bootloader.sh         # Script to flash bootloader
│   │   ├── flash_firmware.sh           # Script to flash firmware
│   │   └── README.md                   # UPDI programming guide
│   └── test/                           # Test utilities
│       ├── key_test.c                  # Key tester firmware
│       └── README.md                   # Testing documentation
│
├── tests/                              # Firmware tests
│   ├── test_matrix.c                   # Matrix scanning tests
│   ├── test_debounce.c                 # Debounce tests
│   ├── test_encoder.c                  # Encoder tests
│   └── README.md                       # Testing guide
│
└── examples/                           # Example configurations
    ├── README.md                       # Examples documentation
    ├── my_layout/                      # Example: Personal layout
    │   ├── keymap.c
    │   ├── macros.c
    │   └── README.md
    └── office_productivity/            # Example: Office layout
        ├── keymap.c
        ├── macros.c
        └── README.md
```

## Key Notes

### Compiled vs Not Compiled
- **Compiled**: `firmware/src/`, `firmware/bootloader/`, `firmware/mcc_generated/`
- **NOT Compiled**: `firmware/lib/` (reference library for copy-paste)
- **Build Output**: `firmware/build/` (gitignored)

### User-Modifiable Files
- `firmware/src/user_keymap.c` - User's active keymap
- `firmware/src/user_macros.c` - User's active macros (copied from lib or custom)
- `firmware/src/user_encoder.c` - User's encoder behavior (copied from lib or custom)
- `firmware/src/user_config.h` - Firmware configuration (led default brightness, debounce method and time, etc.)

### Reference Files (Copy From Here)
- `firmware/lib/macros/*` - Copy functions to `src/macros.c`
- `firmware/lib/encoders/*` - Copy code to `src/encoder.c`
- `firmware/lib/keymaps/*` - Reference complete keymaps

### gitignore Suggestions
```gitignore
# Build artifacts
firmware/build/
*.o
*.elf
*.hex
*.map
*.lst

# KiCad backups
hardware/kicad/*-backups/
hardware/kicad/*-cache.lib
hardware/kicad/fp-info-cache

# Editor files
*.swp
*~
.vscode/
.DS_Store

# User-specific configs (optional)
# firmware/src/keymap.c
# firmware/src/macros.c
```

This structure provides clear separation between:
- **Hardware** (PCB, case, datasheets)
- **Firmware** (source, bootloader, library, tools)
- **Documentation** (guides, specs, assembly)
- **Examples** (reference configurations)rmware/build/` (gitignored)

### User-Modifiable Files
- `firmware/src/user_keymap.c` - User's active keymap
- `firmware/src/user_macros.c` - User's active macros (copied from lib or custom)
- `firmware/src/user_encoder.c` - User's encoder behavior (copied from lib or custom)
- `firmware/src/user_config.h` - Firmware configuration (led default brightness, debounce method and time, etc.)

### Reference Files (Copy From Here)
- `firmware/lib/macros/*` - Copy functions to `src/macros.c`
- `firmware/lib/encoders/*` - Copy code to `src/encoder.c`
- `firmware/lib/keymaps/*` - Reference complete keymaps

### gitignore Suggestions
```gitignore
# Build artifacts
firmware/build/
*.o
*.elf
*.hex
*.map
*.lst

# KiCad backups
hardware/kicad/*-backups/
hardware/kicad/*-cache.lib
hardware/kicad/fp-info-cache

# Editor files
*.swp
*~
.vscode/
.DS_Store

# User-specific configs (optional)
# firmware/src/keymap.c
# firmware/src/macros.c
```

This structure provides clear separation between:
- **Hardware** (PCB, case, datasheets)
- **Firmware** (source, bootloader, library, tools)
- **Documentation** (guides, specs, assembly)
- **Examples** (reference configurations)urations)