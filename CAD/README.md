# CAD

[Headset assembly in Onshape](https://cad.onshape.com/documents/7dd0dda11718de6338e637f2/w/5f67542aa0c734a01c21f813/e/294b57f9c610073ea687bedc)

The Onshape document holds the shell, controls, yokes and harness routing. The preview in the root README shows an earlier revision. This folder includes the populated board STEP files and a copy of the custom features.

## Baffles and pads

[Comfort baffles STEP](Comfort%20baffles.step) has two bodies at their neutral assembly positions. The same pair is in the Onshape Part Studio named **Comfort baffles - 4 mm recess and 2 mm seat**. These are fit references. The main assembly still uses the original baffles.

The Brainwavz pads are 110 × 90 × 30 mm with a 70 × 50 mm opening. A 4 mm driver recess and 2 mm raised pad seat give 24 mm of modeled ear depth with 10 mm pad compression. The 10 mm is an estimate. Check the mounting flap, pad compression, clamp and swivel with the real parts.

The model cleared the surrounding solids in the neutral pose. The tightest modeled gaps on the left are 2.4 mm to the 7 mm battery and 0.9 mm to the thermistor gauge. Check the insulated sensor, driver terminals, solder joints and wire bends with the real cell installed, and confirm that nothing presses on the pouch. The 6 mm recess option in Onshape overlaps the sensor gauge. Use the 4 mm reference for fit work.

## Board STEP files

The dongle STEP includes the bottom-side D1 TVS diode near the plug. It extends about 1.29 mm below the PCB. Allow at least 1.6 mm at D1 and check the actual plug and case before closing it.

Use the main board layout and placement CSV for population. Fit R9 and R10. Leave SW7 off and wire the boom switch to its pads.
