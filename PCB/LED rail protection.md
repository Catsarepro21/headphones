# LED rail protection and first-unit tests

The main board supplies both rings through U13, a TI TPS2553DBVR-1 latch-off switch. U5, its feedback network, C33 and C34 stay on `+5V_RGB_RAW`. U13 feeds `+5V_RGB` to the U6 data buffer and J10 and J11. Both devices use the existing active-high `RGB_ENABLE` signal. A sustained overload trips U13 and disconnects both rings. Cycling the master switch or taking `RGB_ENABLE` low resets the latch. Both rings still use the existing 16-pixel WS2812B-B/W boards.

| Designator | Orderable part | Function |
| --- | --- | --- |
| Main U13 | TI TPS2553DBVR-1 | Latching high-side switch, SOT-23-6 |
| Main R210 | Yageo RC0603FR-0768K1L | 68.1 kΩ ±1% programmed current limit |
| Main C213 | Yageo CC0603KRX7R7BB105 | 1 µF X7R input bypass, 16 V |
| Ring C52, each cup | Nichicon UWT1V470MCL1GS | 47 µF ±20% bulk capacitor, Ø6.3 × 5.8 mm |

The TI current-limit equations give approximately 343 mA minimum, 388 mA typical and 439 mA maximum with the resistor tolerance included. The 300 mA continuous design budget is below the calculated trip range. Full white on 32 pixels is an estimated 1.17 A at 5 V using 12 mA per channel plus 0.6 mA idle per pixel. A full-white command can trip the protection. Use brightness settings within the measured continuous budget for normal patterns. [TI switch datasheet](https://www.ti.com/lit/ds/symlink/tps2553-1.pdf), [Worldsemi V5 datasheet for C114586](https://datasheet.lcsc.com/lcsc/2006151006_Worldsemi-WS2812B-B-W_C114586.pdf)

The input and load switch retain separate capacitance. The two ring bulk capacitors and all 32 LED bypass capacitors total about 117 µF at +20%. The simple constant-current startup estimate is 1.86 ms at the minimum calculated trip current with nominal pixel idle current. TI specifies a 3 ms minimum overcurrent deglitch interval. Measure the actual startup waveforms and rail overshoot. [Nichicon capacitor dimensions](https://www.nichicon.com/en-us/part/uwt1v470mcl1gs/4187/)

The BQ24074 input mode pins default low through resistors, selecting USB 100 mA. After a USB host selects the headset configuration, firmware sets EN1 low and EN2 high. The 3.92 kΩ ILIM resistor gives approximately 390 mA typical and 444 mA at the specified maximum programming factor and minimum resistor tolerance. Host suspend drives both pins high. The 1.78 kΩ ISET resistor sets an approximately 500 mA charge target when the input budget permits. The charger reduces charge current and uses the pack to supplement the system load as needed. Log VBUS input and pack current in each mode, and confirm charger termination with the actual pack. [TI charging and power-path datasheet](https://www.ti.com/lit/ds/symlink/bq24074.pdf)

On battery, the firmware budgets about 245 mA for both rings at 5 V. With a valid USB supply, it turns the LED rail off until a host selects the USB configuration and the charger accepts the higher current setting. The budget then becomes about 65 mA. USB reset, suspend and deconfiguration return the LED rail to off while USB power remains valid. A change in budget reapplies the limit to the latest active frame. The 65 mA estimate includes 19.2 mA of LED idle load and up to 45.2 mA of lighting current. It is the 5 V rail estimate. Measure charger input and pack current separately to account for boost losses and the audio load.

## Bench checks

1. Use a current-limited pack simulator or bench supply for first power-up. With both rings installed, test RGB startup with USB unplugged from the lowest intended pack voltage to 4.2 V. Scope raw boost output, protected LED output and the 3.3 V rail together.
2. Use a WS2812 pattern generator and exercise both rings at up to 300 mA combined steady draw. Record battery current and the voltage at both rings with the complete harness. Check for LED flicker, radio or audio interference and unacceptable surface temperature. At the assumed low 2.9 V system input and 80% efficiency, the LED boost alone draws about 0.67 A at 300 mA output. Confirm pack and power-path headroom with the radio and audio active.
3. With the rings disconnected, use an electronic load on the protected LED rail to verify the cutoff threshold, latch-off and reset. Verify an overload leaves the 3.3 V radio and codec supply operating. Reconnect the two rings and verify recovery after a bounded test at the highest commanded load.
4. Repeat the normal load test with USB connected and the actual protected pack and thermistor installed. Verify input draw, charging behavior and cutoff. Confirm the assembled voltage and temperature margins before wearing the headset.
