# Headphones

I wanted to make my own over-ear headphones and learn how to fit the audio electronics, battery, and controls into something I could actually wear. I also wanted a fold-up mic and RGB around the cups.

The cups are closed-back and use 40 mm Dayton drivers, an adjustable OKCSC headband, and Brainwavz memory-foam pads. There are 16 LEDs in each cup. The headset can pair with a phone over Bluetooth or use a USB dongle for PC audio and the boom mic. The headset's USB-C port handles charging and wired SignalRGB.

![Early Onshape view with the boom mic down](CAD/headset-preview.jpg)

## Files

- [BOM](BOM.md) and [CSV](BOM.csv)
- [Onshape and STEP files](CAD/)
- [KiCad projects and fabrication files](PCB/fabrication/)
- [Wiring](PCB/Harness%20wiring.md) and [RGB power](PCB/LED%20rail%20protection.md)
- [Headset firmware](code/headset/) and [dongle firmware](code/dongle/)
- [SignalRGB plugin](code/signalrgb/Headphones.js)

## Build status

Both firmware targets compile. The boards have passed KiCad's current layout checks. The first assembled unit still needs charging, audio, RF, microphone, and comfort tests.

The one-off subtotal is $281.35 with the hot-air station, boards, paste, and stencils. Printing, shipping, tax, the cut selector seal, and any missing bench tools are extra. The [fabrication notes](PCB/fabrication/) have the board quantities and parts to leave off.

The deeper baffles are in the CAD folder for fit checks. Confirm the battery, thermistor, wires, pad fit, and headband clamp before sending the cups to print.
