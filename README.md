# Commodore 64 and VIC-20 keyboard matrix decoder

Learn more on YouTube:<br>
TODO url here

This is a project for the Raspberry Pi Pico or any other RP2040 board.
Standard C/C++ SDK install. DuPont-style breadboard jumpers can be used.
CBM keyboards have a single inline 0.1" spaced 20 pin connector.

```
pin 1 to ground
pin 2 keyed
pin 3 to GP18
pin 4 unused
pin 5 to GP15
pin 6 to GP14
... (7-18) to (GP13-2)
pin 19 to GP1
pin 20 to GP0
```

The code in `src` is a series of iterations showing how to read a
keyboard martix without diodes. The final iteration is proper and good.

The code in `tinyusb_kb` is a quick hack of an example from the TinyUSB
library. The 10ms rate limiting logic is from there. That's fine for
typing, but some systems can update every 1ms if we were to let them.

## Mapping

There is not separate "OS" and "Emulation" modes. There is one mapping.
This is not a compromise. Everything works in emulation.
All of ASCII is typeable.

Use `src/vice.vkm` as a keymap for the VIC-20 and C64 emulators in VICE.
Go to Settings > Input devices > Keyboard and assign it to the
"Symbolic (user)" position.

Because this is not an ASCII keyboard, some things need a new home.
If a key has an ASCII symbol, it's typeable as labelled.
The exceptions are as follows:

```
C= ............. Tab
RUN/STOP ....... ESC
L.Arrow ........ Backspace
U.Arrow ........ Circumflex "^"
SHIFT + ........ Page Up
SHIFT - ........ Page Down
CLR ............ End
Sterling ....... Grave "`"
SHIFT Sterling . Underbar "_"
SHIFT @ ........ L.Brace "{"
SHIFT * ........ R.Brace "}"
Restore ........ Backslash "\"
SHIFT Restore .. Vertical bar "|"
SHIFT 0 ........ F12
```
