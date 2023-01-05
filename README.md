# Commodore 64 and VIC-20 keyboard matrix decoder

Learn more on YouTube:<br>
https://youtu.be/kzlCkzl-ei0

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

The code in `tinyusb_kb` is boilerplate from the TinyUSB library.
It's configured for 125 reports/second. MiSTer fast polling won't
improve this. Yet. Someone just needs to do the work and testing.

Drawings for 3D printing are in the `sch` folder.

## Mapping

The default mapping is good for both ASCII and VICE. Use `src/vice.vkm`
as a keymap for the VIC-20 and C64 emulators in VICE and BMC64.
Go to Settings > Input devices > Keyboard and assign it to the
"Symbolic (user)" position.

MiSTer doesn't have a custom keymap option for C64 and VIC-20. The only keymap
available is reasonable for using a modern IBM/ASCII keyboard, but isn't very
good for using a real CBM keyboard outside the MiSTer C64 and VIC-20 cores.
Use CTRL plus both SHIFT keys plus the British Pound Sterling key
to swap betweeen MiSTer mode and ASCII mode.

SHIFT-0 is impossibe to send to MiSTer C64 and VIC-20 cores. Nobody has
reported needing it so it is used for F12, which is the menu in BMC64
and MiSTer. If you suspect a program needs it, you can easily confirm
in VICE by adding "F12 4 3 1" to the .vkm file.

Because this is not an ASCII keyboard, some things need a new home.
If a key has a printable ASCII character, it's typeable as labelled.

## ASCII mode
```
C= .............................. Tab
RUN/STOP ........................ ESC
INST/DEL ........................ Backspace
SHIFT INST/DEL .................. Insert
L.Arrow ......................... Delete
SHIFT + ......................... Page Up
SHIFT - ......................... Page Down
CLR/HOME ........................ Home
SHIFT CLR/HOME .................. End
U.Arrow/Pi ...................... Circumflex "^"
SHIFT U.Arrow/Pi ................ Tilde "~"
Sterling ........................ Grave "`"
SHIFT Sterling .................. Underbar "_"
SHIFT @ ......................... L.Brace "{"
SHIFT * ......................... R.Brace "}"
RESTORE ......................... Backslash "\"
SHIFT RESTORE ................... Vertical bar "|"
SHIFT 0 ......................... F12
CTRL L.SHIFT R.SHIFT DEL ........ CTRL ALT DEL
CTRL L.SHIFT R.SHIFT STERLING ... Switch to MiSTer mode
CTRL L.SHIFT R.SHIFT F1 ......... F9
CTRL L.SHIFT R.SHIFT F3 ......... F10
CTRL L.SHIFT R.SHIFT F7 ......... F11
CTRL L.SHIFT R.SHIFT F9 ......... F12

```

## MiSTer mode
Commodore 64 and VIC-20 cores only
```
SHIFT 0 ......................... F12 - MiSTer OSD button - Menu
CTRL L.SHIFT R.SHIFT DEL ........ CTRL L-ALT R-ALT - MiSTer User button - Core Reset
CTRL L.SHIFT R.SHIFT STERLING ... Switch to ASCII mode
```
