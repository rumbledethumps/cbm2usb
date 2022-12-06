# Commodore 64 and VIC-20 keyboard matrix decoder

The code in `src` is a series of iterations showing how to read a
keyboard martix without diodes. The final iteration is proper and good.
TODO final iteration

The code in `tinyusb_kb` is a quick hack of an example from the TinyUSB
library. The 10ms rate limiting logic is from there. That's fine for
typing, but some systems can update every 1ms if we were to let them.

Learn more on YouTube:<br>
TODO url here
