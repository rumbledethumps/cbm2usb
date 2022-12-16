# Commodore 64 and VIC-20 keyboard matrix decoder

Learn more on YouTube:<br>
TODO url here

This is a C project for the Raspberry Pi Pico or any other RP2040 board.
DuPont-style breadboard jumpers mate prefectly with the CBM keyboard's
single inline 0.1" spaced 20 pin connector.

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

The default mapping is good for both ASCII and VICE. Use `src/vice.vkm`
as a keymap for the VIC-20 and C64 emulators in VICE and BMC64.
Go to Settings > Input devices > Keyboard and assign it to the
"Symbolic (user)" position.

MiSTer doesn't have a custom keymap option for C64 and VIC-20.
The keymap is reasonable for use on a modern IBM/ASCII keyboard,
but isn't very good outside the MiSTer C64 and VIC-20 cores.
Use CTRL plus the Commodore key plus the British Pound Sterling key
to swap betweeen MiSTer mode and an ASCII mode for everything else.

SHIFT-0 is impossibe to send to MiSTer C64 and VIC-20 cores. Nobody has
reported needing it so it was picked for F12, which is the menu in BMC64
and MiSTer. If you suspect a program needs it, you can easily confirm
in VICE by adding "F12 4 3 1" to the .vkm file.

Because this is not an ASCII keyboard, some things need a new home.
If a key has a printable ASCII character, it's typeable as labelled.
The exceptions are as follows:

## ASCII mode
```
C= ............... Tab
RUN/STOP ......... ESC
DEL .............. Backspace
L.Arrow .......... Delete
U.Arrow .......... Circumflex "^"
SHIFT + .......... Page Up
SHIFT - .......... Page Down
CLR .............. End
Sterling ......... Grave "`"
SHIFT Sterling ... Underbar "_"
SHIFT @ .......... L.Brace "{"
SHIFT * .......... R.Brace "}"
Restore .......... Backslash "\"
SHIFT Restore .... Vertical bar "|"
SHIFT 0 .......... F12
CTRL C= DEL ...... CTRL ALT DEL
CTRL C= STERLING . Switch to MiSTer mode
```

## MiSTer mode
Commodore 64 and VIC-20 cores only
```
SHIFT 0 .......... F12 - MiSTer OSD button - Menu
CTRL C= DEL ...... CTRL L-ALT R-ALT - MiSTer User button - Core Reset
CTRL C= STERLING . Switch to ASCII mode
```
