# Headphones

I wanted to make my own over-ear headphones and learn how to fit the audio electronics, battery, and controls into something I could actually wear. I also wanted a fold-up mic and RGB around the cups.

The design uses printed closed-back cups, an adjustable OKCSC headband, KUTOU oval pads, and Dayton drivers. There are 16 addressable LEDs behind the diffuser on each cup. The headset has Bluetooth, a separate USB dongle, USB-C charging and data, and a boom mic that mutes when it folds up.

## Pictures

![Early Onshape view of the headphones with the boom mic deployed](CAD/headset-preview.jpg)

## Project files

- [BOM and purchasing links](BOM.md) · [CSV](BOM.csv)
- [Editable Onshape assembly](https://cad.onshape.com/documents/7dd0dda11718de6338e637f2/w/5f67542aa0c734a01c21f813/e/294b57f9c610073ea687bedc)
- [CAD files and board STEP exports](CAD/)
- [Headset KiCad project](PCB/headset/headphones.kicad_pro) · [Dongle](PCB/headset/dongle/dongle.kicad_pro)
- [Button board](PCB/controls/headset-controls.kicad_pro) · [Mode selector](PCB/mode-selector/headset-mode-selector.kicad_pro)
- [Board fabrication and placement files](PCB/fabrication/)
- [Wiring and connector pinout](PCB/Harness%20wiring.md) · [LED power and bench checks](PCB/LED%20rail%20protection.md)
- [Code](code/) is reserved for firmware.

## Build status

The hardware is at the prototype stage. The current boards passed KiCad checks and the modeled headset assembly passed an interference check. The first build will check the fit of the purchased headband and pads, the cable slack through the yokes, and charging, audio, mic mute, and LED protection on the bench.
