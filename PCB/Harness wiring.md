# Harness wiring and continuity checks

Connector numbers below are from the boards in **KiCad controls build - prototype** and the two small control boards in this package. The ends of the two RGB harnesses land directly on the ring PCB pads. Keep those J12 headers off both rings.

| Main board connection | Pin order at the main board | Other end |
| --- | --- | --- |
| J1 battery | 1 PACK_P, 2 thermistor, 3 GND | Protected LP574459 pack positive, pack NTC, pack negative |
| J3 USB | 1 VBUS, 2 GND, 3 D+, 4 D− | USB daughterboard J9 pins 1–4 respectively |
| J5 left speaker | 1 DRV_L, 2 DRV_RETURN | Left Dayton + and isolated return |
| J6 right speaker | 1 DRV_R, 2 DRV_RETURN | Right Dayton + and isolated return |
| J7 boom mic | 1 MIC_BOOM, 2 GND | Capsule hot through the B3F-1000 normally-open contact, then capsule ground directly |
| J8 ambient mic | 1 MIC_AMBIENT, 2 GND | Ambient capsule hot and ground |
| J10 left RGB | 1 +5V_RGB, 2 GND, 3 left DI, 4 left DO | Left ring J12 pads 1–4 respectively |
| J11 right RGB | 1 +5V_RGB, 2 GND, 3 right DI | Right ring J12 pads 1–3 respectively. Leave right J12 pad 4 unused. |
| SW1 main power | 1 VSYS, 2 SYS_SW | Master slide common and one maintained throw. Insulate the unused throw. |
| SW8 mode | 1 BT_MODE_N, 2 GND | Mode selector J1 pins 1–2 respectively |
| Four-button board | SW3-1, SW4-1, SW3-2, SW5-1, SW6-1 | Button-board J1 pins 1–5 respectively |

The right ring is downstream of the left ring through main-board R27. Keep the left ring data-out return connected even if bringing up the right ring separately. Battery, left driver and left ring require **nine separate conductors through the headband**. Keep driver return isolated from chassis and power ground. Keep the USB data pair together, maintain D+/D− polarity, and minimize free wire between J3 and J9.

Main J2 brings out 3V3, SWDIO, SWDCLK, GND and reset on pins 1–5. Main J4 brings out P1.10, P1.09 and GND on pins 1–3. Those two headers are service connections with no permanent cup-to-cup harness.

## Headband harness installation

Use nine separated strands of black 26 AWG silicone wire from three Adafruit 3892 packs. Each pack supplies four 1 m strands. The published outside diameter is 1.5 ± 0.1 mm. Nine strands arranged three by three fit inside a Ø6.13 mm geometric envelope at the maximum published wire diameter. Keep the finished bundle at or below Ø6.30 mm where it passes through the guides. Label both ends of every strand before routing. [Wire specification](https://www.adafruit.com/product/3892)

Each shell has a seat for a split TPU grommet and an interior restraint bridge. The grommet has a Ø6.42 mm bore. The guide on each slotted yoke adapter has a Ø6.84 mm bore. Bring the bundle through the guides with the wire slack clear of the adjuster. Fit the split grommets around the conductors and seat the lips against the outside of each shell. Secure the bundle at each interior bridge with a flat tie no wider than 2.5 mm. Keep the tie tail and thicker labels inside the cups and clear of the boards and baffles.

Onshape reported zero interferences with two temporary Ø6.30 mm × 16 mm gauges through the installed passages in the neutral assembly. The gauges were removed after the check. On the first build, pass the formed cable through both grommets and leave enough relaxed slack for full adjustment, both swivels and cup removal. Cycle the band while checking for pinch, chafe and tension at the keyed joints. Set the final cut lengths after that check.

## First-unit continuity

The left battery tray now includes a shelf alongside the pack for the Molex Micro-Lock Plus inline disconnect. Use a 505570-0301 receptacle with three 505572-1000 socket terminals on the pack side and a 213719-2031 plug with three 213720-1000 pin terminals on the harness side. Follow the molded circuit numbers. Circuit 1 is pack positive, circuit 2 is the NTC, and circuit 3 is pack negative. Confirm each conductor on the actual pack. Check both terminal specifications against the actual insulation diameter. The harness-side pins accept 0.90–1.50 mm insulation, so use a compatible pigtail with individual insulated splices if the silicone wire measures above that range. Do not trim insulation to fit the crimp. [Molex connector specification](https://www.molex.com/content/dam/molex/molex-dot-com/en_us/pdf/datasheets/987651-4888.pdf)

Retain the harness-side housing with a 2.5 mm tie through the two slots in the shelf. Point the latch toward the removable baffle and pull the pack half away from the headband to disconnect. The 11.5 × 18.9 × 9.0 mm mating envelope and 8 mm release sweep cleared the modeled hardware. Verify the real latch, lead bends and servicing clearance with the baffle removed.

- Check every conductor with both boards and battery disconnected. Check each adjacent pair and power-to-ground for unintended shorts.
- Verify the pack pinout and NTC from the actual leads before inserting a keyed mating connector. Wire colors alone do not establish these functions.
- Specify the protected LP574459 with three 50 mm 26 AWG leads and a 10 kΩ ±1% B3435 thermistor when requesting a quote. The listed pack has bare leads. Confirm the finished pack length, the connector option and pack pinout at order time. The supplier lists 59 mm and 60 mm lengths on different pages. [Leads and thermistor](https://www.lipolbattery.com/High-Endurance-Batteries-for-Seamless-Global-Communication-LP574459.html), [sample ordering and nominal dimensions](https://www.lipobattery.us/lipo-battery-lp574459-2000mah-3-7v-7-4wh-with-pcm-and-ntc-and-wires/)
- Check the nine-conductor bundle against both yoke travel limits, the access path to the removable cups and the swept volume of the boom. Record cut lengths from the first physical build after setting service slack.
- Verify the speakers' returns remain isolated from USB, battery ground and the metal band. Use isolated test equipment on the floating audio return.
- Verify RGB and USB at the assembled wire lengths. Limit initial RGB testing to the design's intended current budget and observe the measured total draw during bring-up.
