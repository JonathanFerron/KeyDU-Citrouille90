# HID Usage to Virtual Key Translation Table

## Standard Keyboard Keys (HID Page 0x07)

| VK Constant     | VK Value | HID Page | HID Usage | Key Name      |
| --------------- | -------- | -------- | --------- | ------------- |
| `VK_ESCAPE`     | 0x1B     | 0x07     | 0x29      | Escape        |
| `VK_1`          | 0x31     | 0x07     | 0x1E      | 1 !           |
| `VK_2`          | 0x32     | 0x07     | 0x1F      | 2 @           |
| `VK_3`          | 0x33     | 0x07     | 0x20      | 3 #           |
| `VK_4`          | 0x34     | 0x07     | 0x21      | 4 $           |
| `VK_5`          | 0x35     | 0x07     | 0x22      | 5 %           |
| `VK_6`          | 0x36     | 0x07     | 0x23      | 6 ^           |
| `VK_7`          | 0x37     | 0x07     | 0x24      | 7 &           |
| `VK_8`          | 0x38     | 0x07     | 0x25      | 8 *           |
| `VK_9`          | 0x39     | 0x07     | 0x26      | 9 (           |
| `VK_0`          | 0x30     | 0x07     | 0x27      | 0 )           |
| `VK_OEM_MINUS`  | 0xBD     | 0x07     | 0x2D      | - _           |
| `VK_OEM_PLUS`   | 0xBB     | 0x07     | 0x2E      | = +           |
| `VK_BACK`       | 0x08     | 0x07     | 0x2A      | Backspace     |
| `VK_TAB`        | 0x09     | 0x07     | 0x2B      | Tab           |
| `VK_Q`          | 0x51     | 0x07     | 0x14      | Q             |
| `VK_W`          | 0x57     | 0x07     | 0x1A      | W             |
| `VK_E`          | 0x45     | 0x07     | 0x08      | E             |
| `VK_R`          | 0x52     | 0x07     | 0x15      | R             |
| `VK_T`          | 0x54     | 0x07     | 0x17      | T             |
| `VK_Y`          | 0x59     | 0x07     | 0x1C      | Y             |
| `VK_U`          | 0x55     | 0x07     | 0x18      | U             |
| `VK_I`          | 0x49     | 0x07     | 0x0C      | I             |
| `VK_O`          | 0x4F     | 0x07     | 0x12      | O             |
| `VK_P`          | 0x50     | 0x07     | 0x13      | P             |
| `VK_OEM_4`      | 0xDB     | 0x07     | 0x2F      | [ {           |
| `VK_OEM_6`      | 0xDD     | 0x07     | 0x30      | ] }           |
| `VK_RETURN`     | 0x0D     | 0x07     | 0x28      | Enter (main)  |
| `VK_LCONTROL`   | 0xA2     | 0x07     | 0xE0      | Left Control  |
| `VK_A`          | 0x41     | 0x07     | 0x04      | A             |
| `VK_S`          | 0x53     | 0x07     | 0x16      | S             |
| `VK_D`          | 0x44     | 0x07     | 0x07      | D             |
| `VK_F`          | 0x46     | 0x07     | 0x09      | F             |
| `VK_G`          | 0x47     | 0x07     | 0x0A      | G             |
| `VK_H`          | 0x48     | 0x07     | 0x0B      | H             |
| `VK_J`          | 0x4A     | 0x07     | 0x0D      | J             |
| `VK_K`          | 0x4B     | 0x07     | 0x0E      | K             |
| `VK_L`          | 0x4C     | 0x07     | 0x0F      | L             |
| `VK_OEM_1`      | 0xBA     | 0x07     | 0x33      | ; :           |
| `VK_OEM_7`      | 0xDE     | 0x07     | 0x34      | ' "           |
| `VK_OEM_3`      | 0xC0     | 0x07     | 0x35      | ` ~           |
| `VK_LSHIFT`     | 0xA0     | 0x07     | 0xE1      | Left Shift    |
| `VK_OEM_5`      | 0xDC     | 0x07     | 0x31      | \ \|          |
| `VK_Z`          | 0x5A     | 0x07     | 0x1D      | Z             |
| `VK_X`          | 0x58     | 0x07     | 0x1B      | X             |
| `VK_C`          | 0x43     | 0x07     | 0x06      | C             |
| `VK_V`          | 0x56     | 0x07     | 0x19      | V             |
| `VK_B`          | 0x42     | 0x07     | 0x05      | B             |
| `VK_N`          | 0x4E     | 0x07     | 0x11      | N             |
| `VK_M`          | 0x4D     | 0x07     | 0x10      | M             |
| `VK_OEM_COMMA`  | 0xBC     | 0x07     | 0x36      | , <           |
| `VK_OEM_PERIOD` | 0xBE     | 0x07     | 0x37      | . >           |
| `VK_OEM_2`      | 0xBF     | 0x07     | 0x38      | / ?           |
| `VK_RSHIFT`     | 0xA1     | 0x07     | 0xE5      | Right Shift   |
| `VK_MULTIPLY`   | 0x6A     | 0x07     | 0x55      | Keypad *      |
| `VK_LMENU`      | 0xA4     | 0x07     | 0xE2      | Left Alt      |
| `VK_SPACE`      | 0x20     | 0x07     | 0x2C      | Space         |
| `VK_CAPITAL`    | 0x14     | 0x07     | 0x39      | Caps Lock     |
| `VK_F1`         | 0x70     | 0x07     | 0x3A      | F1            |
| `VK_F2`         | 0x71     | 0x07     | 0x3B      | F2            |
| `VK_F3`         | 0x72     | 0x07     | 0x3C      | F3            |
| `VK_F4`         | 0x73     | 0x07     | 0x3D      | F4            |
| `VK_F5`         | 0x74     | 0x07     | 0x3E      | F5            |
| `VK_F6`         | 0x75     | 0x07     | 0x3F      | F6            |
| `VK_F7`         | 0x76     | 0x07     | 0x40      | F7            |
| `VK_F8`         | 0x77     | 0x07     | 0x41      | F8            |
| `VK_F9`         | 0x78     | 0x07     | 0x42      | F9            |
| `VK_F10`        | 0x79     | 0x07     | 0x43      | F10           |
| `VK_NUMLOCK`    | 0x90     | 0x07     | 0x53      | Num Lock      |
| `VK_SCROLL`     | 0x91     | 0x07     | 0x47      | Scroll Lock   |
| `VK_SUBTRACT`   | 0x6D     | 0x07     | 0x56      | Keypad -      |
| `VK_ADD`        | 0x6B     | 0x07     | 0x57      | Keypad +      |
| `VK_OEM_102`    | 0xE2     | 0x07     | 0x64      | Europe 2      |
| `VK_F11`        | 0x7A     | 0x07     | 0x44      | F11           |
| `VK_F12`        | 0x7B     | 0x07     | 0x45      | F12           |

## Extended Keys (HID Page 0x07)

| VK Constant   | VK Value | HID Page | HID Usage | Key Name        |
| ------------- | -------- | -------- | --------- | --------------- |
| `VK_RETURN`   | 0x0D     | 0x07     | 0x58      | Keypad Enter    |
| `VK_RCONTROL` | 0xA3     | 0x07     | 0xE4      | Right Control   |
| `VK_DIVIDE`   | 0x6F     | 0x07     | 0x54      | Keypad /        |
| `VK_SNAPSHOT` | 0x2C     | 0x07     | 0x46      | Print Screen    |
| `VK_RMENU`    | 0xA5     | 0x07     | 0xE6      | Right Alt       |
| `VK_HOME`     | 0x24     | 0x07     | 0x4A      | Home            |
| `VK_UP`       | 0x26     | 0x07     | 0x52      | Up Arrow        |
| `VK_PRIOR`    | 0x21     | 0x07     | 0x4B      | Page Up         |
| `VK_LEFT`     | 0x25     | 0x07     | 0x50      | Left Arrow      |
| `VK_RIGHT`    | 0x27     | 0x07     | 0x4F      | Right Arrow     |
| `VK_END`      | 0x23     | 0x07     | 0x4D      | End             |
| `VK_DOWN`     | 0x28     | 0x07     | 0x51      | Down Arrow      |
| `VK_NEXT`     | 0x22     | 0x07     | 0x4E      | Page Down       |
| `VK_INSERT`   | 0x2D     | 0x07     | 0x49      | Insert          |
| `VK_DELETE`   | 0x2E     | 0x07     | 0x4C      | Delete          |
| `VK_LWIN`     | 0x5B     | 0x07     | 0xE3      | Left Windows    |
| `VK_RWIN`     | 0x5C     | 0x07     | 0xE7      | Right Windows   |
| `VK_APPS`     | 0x5D     | 0x07     | 0x65      | Application     |
| `VK_POWER`    | 0x5E     | 0x07     | 0x66      | Keyboard Power  |

## Special Keys (E1 prefix)

| VK Constant | VK Value | HID Page | HID Usage | Key Name |
| ----------- | -------- | -------- | --------- | -------- |
| `VK_PAUSE`  | 0x13     | 0x07     | 0x48      | Pause    |

## System Power Keys (HID Page 0x01)

| VK Constant | VK Value | HID Page | HID Usage | Key Name     |
| ----------- | -------- | -------- | --------- | ------------ |
| `VK_POWER`  | 0x5E     | 0x01     | 0x81      | System Power |
| `VK_SLEEP`  | 0x5F     | 0x01     | 0x82      | System Sleep |

## Consumer Controls (HID Page 0x0C)

| VK Constant              | VK Value | HID Page | HID Usage | Key Name            |
| ------------------------ | -------- | -------- | --------- | ------------------- |
| `VK_MEDIA_NEXT_TRACK`    | 0xB0     | 0x0C     | 0x00B5    | Scan Next Track     |
| `VK_MEDIA_PREV_TRACK`    | 0xB1     | 0x0C     | 0x00B6    | Scan Previous Track |
| `VK_MEDIA_STOP`          | 0xB2     | 0x0C     | 0x00B7    | Stop                |
| `VK_MEDIA_PLAY_PAUSE`    | 0xB3     | 0x0C     | 0x00CD    | Play/Pause          |
| `VK_VOLUME_MUTE`         | 0xAD     | 0x0C     | 0x00E2    | Mute                |
| `VK_VOLUME_DOWN`         | 0xAE     | 0x0C     | 0x00EA    | Volume Down         |
| `VK_VOLUME_UP`           | 0xAF     | 0x0C     | 0x00E9    | Volume Up           |
| `VK_LAUNCH_MEDIA_SELECT` | 0xB5     | 0x0C     | 0x0183    | Media Select        |
| `VK_LAUNCH_MAIL`         | 0xB4     | 0x0C     | 0x018A    | Mail                |
| `VK_LAUNCH_APP2`         | 0xB7     | 0x0C     | 0x0192    | Calculator          |
| `VK_LAUNCH_APP1`         | 0xB6     | 0x0C     | 0x0194    | My Computer         |
| `VK_BROWSER_SEARCH`      | 0xAA     | 0x0C     | 0x0221    | WWW Search          |
| `VK_BROWSER_HOME`        | 0xAC     | 0x0C     | 0x0223    | WWW Home            |
| `VK_BROWSER_BACK`        | 0xA6     | 0x0C     | 0x0224    | WWW Back            |
| `VK_BROWSER_FORWARD`     | 0xA7     | 0x0C     | 0x0225    | WWW Forward         |
| `VK_BROWSER_STOP`        | 0xA9     | 0x0C     | 0x0226    | WWW Stop            |
| `VK_BROWSER_REFRESH`     | 0xA8     | 0x0C     | 0x0227    | WWW Refresh         |
| `VK_BROWSER_FAVORITES`   | 0xAB     | 0x0C     | 0x022A    | WWW Favorites       |

## Additional Function Keys (HID Page 0x07)

| VK Constant | VK Value | HID Page | HID Usage | Key Name |
| ----------- | -------- | -------- | --------- | -------- |
| `VK_F13`    | 0x7C     | 0x07     | 0x68      | F13      |
| `VK_F14`    | 0x7D     | 0x07     | 0x69      | F14      |
| `VK_F15`    | 0x7E     | 0x07     | 0x6A      | F15      |
| `VK_F16`    | 0x7F     | 0x07     | 0x6B      | F16      |
| `VK_F17`    | 0x80     | 0x07     | 0x6C      | F17      |
| `VK_F18`    | 0x81     | 0x07     | 0x6D      | F18      |
| `VK_F19`    | 0x82     | 0x07     | 0x6E      | F19      |
| `VK_F20`    | 0x83     | 0x07     | 0x6F      | F20      |
| `VK_F21`    | 0x84     | 0x07     | 0x70      | F21      |
| `VK_F22`    | 0x85     | 0x07     | 0x71      | F22      |
| `VK_F23`    | 0x86     | 0x07     | 0x72      | F23      |
| `VK_F24`    | 0x87     | 0x07     | 0x73      | F24      |

## International Keys (HID Page 0x07)

| VK Constant     | VK Value | HID Page | HID Usage | Key Name          |
| --------------- | -------- | -------- | --------- | ----------------- |
| `VK_KANA`       | 0x15     | 0x07     | 0x88      | Katakana/Hiragana |
| `VK_CONVERT`    | 0x1C     | 0x07     | 0x8A      | Henkan            |
| `VK_NONCONVERT` | 0x1D     | 0x07     | 0x8B      | Muhenkan          |
| `VK_HANGUL`     | 0x15     | 0x07     | 0x90      | Hangul/English    |
| `VK_HANJA`      | 0x19     | 0x07     | 0x91      | Hanja             |

## Key Observations

1. **Use Consumer page (0x0C) for media and volume keys** — the Page 0x07 equivalents (0x7F–0x81) are unassigned and generate no VK codes.
2. **`VK_POWER`** has two distinct HID paths: Page 0x07 usage 0x66 (Keyboard Power key) and Page 0x01 usage 0x81 (System Power Control). Both map to the same VK but are different HID usages.
3. **`VK_RETURN`** has two distinct HID paths: Page 0x07 usage 0x28 (main Enter) and Page 0x07 usage 0x58 (Keypad Enter). Both map to the same VK.
4. **`VK_PAUSE`** uses an unusual E1-prefixed PS/2 scancode sequence — it has no E0-extended equivalent.
