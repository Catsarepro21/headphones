# Board files

The build uses seven boards across six designs. Each folder has the Gerbers, drill files, and a placement CSV. Verify the JLCPCB preview, layer count, thickness and assembly side before ordering.

| Board | Qty for one headset | Layers | Thickness | KiCad source |
| --- | ---: | ---: | ---: | --- |
| [Main](headphones/) | 1 | 4 | 1.6 mm | [Project](../headset/headphones.kicad_pro) |
| [USB-C](usb-c/) | 1 | 2 | 1.6 mm | [Layout](../headset/usb-c.kicad_pcb) |
| [RGB ring](rgb-ring/) | 2 | 2 | 1.6 mm | [Layout](../headset/rgb-ring.kicad_pcb) |
| [Dongle](dongle/) | 1 | 2 | 0.8 mm | [Project](../headset/dongle/dongle.kicad_pro) |
| [Buttons](controls/) | 1 | 2 | 1.6 mm | [Project](../controls/headset-controls.kicad_pro) |
| [Mode selector](mode-selector/) | 1 | 2 | 1.0 mm | [Project](../mode-selector/headset-mode-selector.kicad_pro) |

## Before assembly

- Fit R9 (1 MΩ) and R10 (56 kΩ) on the main board. Leave SW3 through SW7 empty for the wired controls. If the BM83 needs external UART setup, leave R28 open until that is done.
- Build both RGB rings from the same layout. Leave both J12 headers off and solder the harnesses to their pads.
- Fit dongle D1 on the bottom face. Check its orientation and the clearance under the plug.
- Check the assembly shop's component rotations against the footprint and silkscreen. The placement CSVs use KiCad coordinates.

U8 has two hidden ground pads. Check the profile on a spare board and confirm both pads wet properly before using that process on the main board. Install U8 on its final heating cycle. A continuity check alone cannot confirm both joints. Keep direct hot air off the RGB lenses and check one ring before finishing the second.

## Cost

The bare-board estimate was $38.70 for five copies of each design. The $281.35 home-assembly subtotal includes the $220.70 parts and station, $12.95 of paste, and $9.00 for up to three small stencil patterns. Confirm the stencil and board shipping together. A new SWD probe brings the subtotal to $298.00, and a meter and separate UART bridge bring it to $323.34. Printing, the selector seal, shipping, tax, and any other missing tools are still open. [BOM](../../BOM.md) · [Pricing sheet](https://docs.google.com/spreadsheets/d/1EZlvAER-2UgOb8yQGnecRwhJc7PUS93z2gQPCFvvkVY/edit)
