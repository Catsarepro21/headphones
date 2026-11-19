# Headset firmware

This runs on the E73-2G4M08S1C on the main PCB. The selector switches the headphone and mic paths. In Bluetooth mode, the four buttons handle pairing, play/pause and volume. The boom switch uses main SW7 pads as a GPIO input. The capsule stays biased at J7. An open contact mutes dongle capture and requests mute on an active BM83 call. Verify the BM83 mute and unmute commands with the installed module profile.

Set the BM83 to Host MCU mode at 115200 baud with A2DP sink, AVRCP and HFP/HSP enabled. In dongle mode the TLV320AIC3204 uses the 12.288 MHz oscillator for 48 kHz stereo playback and boom mic capture. The first bench build limits codec playback to −24 dB.

The charger defaults to USB 100 mA. A powered headset with configured USB IDs requests a 500 mA USB configuration. After the host selects it, the charger uses the 3.92 kΩ R2 input limit, about 390 mA typical. During host suspend it goes into standby. The off state and charge-only sources use the 100 mA fallback. The input budget is shared between system load and battery charging. Charge status is logged over SEGGER RTT.

## Build

From a Zephyr 4.2.0 workspace with the Arm toolchain and the `cmsis`, `cmsis_6`, `hal_nordic`, `mbedtls` and `segger` modules:

```sh
west build -b headphones_headset -d build-headset /path/to/headphones/code/headset -- \
  -DCONFIG_HEADSET_USB_VID="$USB_VID" -DCONFIG_HEADSET_USB_PID="$USB_PID"
```

Set USB_VID and USB_PID to assigned IDs. The build writes `build-headset/zephyr/zephyr.hex`. J2 is the SWD header on the main board. Check the 3.3 V reference at J2 before connecting the probe.

## Dongle mode

Plug the [USB dongle](../dongle/) into the PC, move the headset selector to dongle mode and press Pair on the first connection. The pairing window lasts 30 seconds. The two boards save the bond and reconnect when selected. Playback and mic audio use 4 ms IMA ADPCM packets over a dedicated Bluetooth LE connection. The headset sends mic audio only while the PC capture stream is open.

## SignalRGB

For a cable connected to the headset, set `CONFIG_HEADSET_USB_VID` and `CONFIG_HEADSET_USB_PID`. For wireless lighting, use the dongle's USB IDs and select dongle mode. Enter the IDs of the device connected to the PC in [Headphones.js](../signalrgb/Headphones.js). A zero VID or PID keeps direct USB RGB disabled and leaves charging in the 100 mA fallback.

Each frame uses two 52-byte HID feature reports. Each report starts with ID 1, command 0x52, a sequence number and cup index, then 16 RGB triples. Send the left cup first and use the same sequence number for both. The dongle forwards up to 30 frames per second. The headset gives a live USB lighting feed priority. It budgets about 245 mA for both rings on battery and 65 mA on a configured USB host. The rings stay off during USB100 and suspend. A black frame or expired feeds also shut off the rail.

## First bench test

Test pairing, reconnect, media and calls on a phone. Check USB enumeration and run playback and mic capture together through the dongle. Check for clicks when switching sources, confirm boom mute in both modes and measure latency. Check the codec clocks (48 kHz WCLK and 1.536 MHz BCLK), LED data timing and LED current on the assembled headset.

Check charging with SW1 off, with a configured data host, and after host suspend and resume. Verify battery temperature sensing, charge termination, and the switched 3.3 V rail when USB is present and SW1 is open. Populate R2 with 3.92 kΩ, R9 with 1 MΩ and R10 with 56 kΩ. The ADC reports actual pack millivolts over SEGGER RTT every 30 seconds. Check the reading against a meter and confirm the sense node stays below 0.23 V with a 4.2 V pack and SW1 open. Leave the SW7 footprint unpopulated for the wired boom contact.

### RTT with a CMSIS-DAP probe

The NXP MCU-Link Base can use pyOCD 0.45 with the nRF52840 target. Power the probe before the target and connect by signal from the 1.27 mm probe cable to J2 on the headset. Wire VTref, SWDIO, SWDCLK and ground. With the headset already flashed, save this as pyocd.yaml in the working folder and run `pyocd run -t nrf52840`.

```yaml
rtt:
  - control-block:
      auto-detect: true
    channel:
      - number: 0
        mode: stdio
```

Confirm the battery and charger readings appear before starting a timed test. [RTT options](https://pyocd.io/docs/options.html)

## BM83 Service

On the underside of the main board, TP1 is BM83 RX, TP2 is BM83 TX, TP4 is SYS_CFG, TP5 is BM83 reset, TP6 is ground and TP10 is adapter input. The nRF reset is separately available on J2 pin 5. For external UART service, temporarily leave R28 open so the MCU cannot drive adapter TX. Cross TX and RX and connect common ground. Verify voltage compatibility. Keep TP10 open on this PCB. If an approved programming sequence needs 5 V there, isolate U8.23 from the 3.3 V rail and qualify the setup first. With power removed, restore R28 before normal operation. Follow the [Microchip UART programming requirements](https://support.microchip.com/s/article/BM83-Firmware-Update-Methods-and-Audio-Interface-Options--USB-UART-Programming--Metadata-Output--and-I2S-DAC-Support).

- [BM83 UART guide](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/UserGuides/BM83_Host_MCU_Firmware_Development_Guide_DS50002896A.pdf)
- [TLV320AIC3204 register guide](https://www.ti.com/lit/ml/slaa557/slaa557.pdf)
- [SignalRGB plugin guide](https://docs.signalrgb.com/developer/plugins/)
