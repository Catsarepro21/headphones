# Harness wiring and continuity checks

These pin numbers are for the KiCad boards in this repo. The ends of the two RGB harnesses land directly on the ring PCB pads. Keep those J12 headers off both rings.

| Main board connection | Pin order at the main board | Other end |
| --- | --- | --- |
| J1 battery | 1 PACK_P, 2 thermistor, 3 GND | Protected 1S pack positive, pack NTC, pack negative |
| J3 USB | 1 VBUS, 2 GND, 3 D+, 4 D− | USB daughterboard J9 pins 1–4 respectively |
| J5 left speaker | 1 DRV_L, 2 DRV_RETURN | Left Dayton + and isolated return |
| J6 right speaker | 1 DRV_R, 2 DRV_RETURN | Right Dayton + and isolated return |
| J7 boom mic | 1 MIC_BOOM, 2 GND | Capsule hot and ground direct to J7. Keep the switch on SW7. |
| J8 ambient mic | 1 MIC_AMBIENT, 2 GND | Ambient capsule hot and ground |
| J10 left RGB | 1 +5V_RGB, 2 GND, 3 left DI, 4 left DO | Left ring J12 pads 1–4 respectively |
| J11 right RGB | 1 +5V_RGB, 2 GND, 3 right DI | Right ring J12 pads 1–3 respectively. Leave right J12 pad 4 unused. |
| SW1 main power | 1 VSYS, 2 SYS_SW | Master slide common and one maintained throw. Insulate the unused throw. |
| SW8 mode | 1 BT_MODE_N, 2 GND | Mode selector J1 pins 1–2 respectively |
| SW7 boom switch pads | 1 P1.11, 2 GND | Leave the tact footprint empty. Wire the B3F-1000 normally-open contacts to the pads. Closed means deployed. |
| Four-button board | SW3-1, SW4-1, SW3-2, SW5-1, SW6-1 | Button-board J1 pins 1–5 respectively |

The right ring is downstream of the left ring through main-board R27. Keep the left ring data-out return connected even if bringing up the right ring separately. Battery, left driver and left ring require **nine separate conductors through the headband**. Route the left speaker signal and floating return next to each other. Keep that pair away from LED data where the wires fan out inside the cups. Keep driver return isolated from chassis and power ground. Keep the USB data pair together, maintain D+/D− polarity, and minimize free wire between J3 and J9.

Main J2 brings out 3V3, SWDIO, SWDCLK, GND and reset on pins 1–5. Main J4 brings out P1.10, P1.09 and GND on pins 1–3. Those two headers are service connections with no permanent cup-to-cup harness. Cut 5 and 3 pins for main J2/J4 from one strip. Dongle J1 uses four solder pads for temporary SWD leads.

Leave the boom capsule biased in both boom positions. The folded or disconnected switch requests mute in firmware. With a live phone call and with PC capture open, verify both audio paths mute and record the fold transition before closing the cup.

## Headband harness installation

Use nine separated strands of black 26 AWG silicone wire from three Adafruit 3892 packs. Each pack supplies four 1 m strands. The published outside diameter is 1.5 ± 0.1 mm. Nine strands arranged three by three fit inside a Ø6.13 mm geometric envelope at the maximum published wire diameter. Keep the finished bundle at or below Ø6.30 mm where it passes through the guides. Label both ends of every strand before routing. [Wire specification](https://www.adafruit.com/product/3892)

Each shell has a seat for a split TPU grommet and an interior restraint bridge. The grommet has a Ø6.42 mm bore. The guide on each slotted yoke adapter has a Ø6.84 mm bore. Bring the bundle through the guides with the wire slack clear of the adjuster. Fit the split grommets around the conductors and seat the lips against the outside of each shell. Secure the bundle at each interior bridge with a flat tie no wider than 2.5 mm. Keep the tie tail and thicker labels inside the cups and clear of the boards and baffles.

Onshape reported zero interferences with two temporary Ø6.30 mm × 16 mm gauges through the installed passages in the neutral assembly. The gauges were removed after the check. On the first build, pass the formed cable through both grommets and leave enough relaxed slack for full adjustment, both swivels and cup removal. Cycle the band while checking for pinch, chafe and tension at the keyed joints. Set the final cut lengths after that check.

## First-unit continuity

Fit both halves of the [Adafruit 5087 three-pin JST-PH-compatible pair](https://www.digikey.com/en/products/detail/adafruit-industries-llc/5087/14652853) in the left battery tray. Use circuit 1 for PACK_P, 2 for the NTC and 3 for GND. Confirm continuity, pack polarity, tray fit, connector retention and strain relief before plugging in. The matched pair replaces the Micro-Lock parts on both sides. Stagger and insulate splices away from the moving section. Use the flexible silicone wire for direct-soldered signal and power harnesses. Keep the SWD and service headers accessible.

Retain the harness-side housing with a 2.5 mm tie through the two slots in the shelf. Point the latch toward the removable baffle and pull the pack half away from the headband to disconnect. The 11.5 × 18.9 × 9.0 mm mating envelope and 8 mm release sweep cleared the modeled hardware. Verify the real latch, lead bends and servicing clearance with the baffle removed.

- Check every conductor with both boards and battery disconnected. Check each adjacent pair and power-to-ground for unintended shorts.
- Verify the pack pinout and NTC from the actual leads before inserting a keyed mating connector. Wire colors alone do not establish these functions.
- Specify the protected LP574459 with three 50 mm 26 AWG leads and a 10 kΩ ±1% B3435 thermistor when requesting a quote. The listed pack has bare leads. Confirm the finished pack length, the connector option and pack pinout at order time. The supplier lists 59 mm and 60 mm lengths on different pages. [Leads and thermistor](https://www.lipolbattery.com/High-Endurance-Batteries-for-Seamless-Global-Communication-LP574459.html), [sample ordering and nominal dimensions](https://www.lipobattery.us/lipo-battery-lp574459-2000mah-3-7v-7-4wh-with-pcm-and-ntc-and-wires/)
- Check the nine-conductor bundle against both yoke travel limits, the access path to the removable cups and the swept volume of the boom. Record cut lengths from the first physical build after setting service slack.
- Verify the speakers' returns remain isolated from USB, battery ground and the metal band. Use isolated test equipment on the floating audio return.
- Verify RGB and USB at the assembled wire lengths. Limit initial RGB testing to the design's intended current budget and observe the measured total draw during bring-up.

For the 2011 trial, use the 3814 to retain the original pack plug and a separate 103AT-2 at the cell surface. Verify sensing, charge cutoff and tray clearance before use. The LP574459 with factory NTC remains the CAD fallback.
