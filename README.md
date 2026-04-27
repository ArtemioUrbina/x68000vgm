X68000 Quick Hack VGM player

This is just a small VGM player made for fun. For a feature rich VGM player for X68000 you can use VGMDRV (https://sites.google.com/site/waveknife7/vgmdrv).

- Timing via OPM interrupts 
- Supports YM2151 and OKI
- Can play VGMs from other systems, ignoring DAC commands (playing YM2151 only)
- Support for VGZ files, if compiled against tinf (tiny inflate library https://github.com/jibsen/tinf)
- Based on examples by FedericoTech: https://github.com/FedericoTech/X68KTutorials/
- Coded while live streaming: https://www.youtube.com/watch?v=I83gDuRurOU
- VGM specification: https://vgmrips.net/wiki/VGM_Specification
- Originally based on MDFourier for X68000: https://github.com/ArtemioUrbina/mdf-x68000

Usage:
- vgm <file> [r]
- Optional r parameter for repeat
- ESC or CTRL-C to exit
- 1 to turn FM on and off
- 2 to turn ADPCM on/off

Compile:
- SDK and compiler: https://www.target-earth.net/wiki/doku.php?id=blog%3Ax68_devtools
- if you want VGZ support (slow) decompress tinf in the root folder and make clean/make
