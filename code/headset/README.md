# Headset firmware

This runs on the E73-2G4M08S1C on the main PCB. The selector switches the headphone and mic paths. In Bluetooth mode, the four buttons handle pairing, play/pause and volume. The boom switch cuts the mic in hardware when folded up.

Set the BM83 to Host MCU mode at 115200 baud with A2DP sink, AVRCP and HFP/HSP enabled. The charger stays in USB 100 mA mode and reports its status over SEGGER RTT. In dongle mode the TLV320AIC3204 uses the 12.288 MHz oscillator for 48 kHz stereo playback and boom mic capture. The first bench build limits codec playback to −24 dB.

## Build

From a Zephyr 4.2.0 workspace with the Arm toolchain and the `cmsis`, `cmsis_6`, `hal_nordic`, `mbedtls` and `segger` modules:

```sh
west build -b headphones_headset /path/to/headphones/code/headset
```

The build writes `build/zephyr/zephyr.hex`. J2 is the SWD header on the main board. Check the 3.3 V reference at J2 before connecting the probe.

## Dongle mode

Plug the [USB dongle](../dongle/) into the PC, move the headset selector to dongle mode and press Pair on the first connection. The pairing window lasts 30 seconds. The two boards save the bond and reconnect when selected. Playback and mic audio use 4 ms IMA ADPCM packets over a dedicated Bluetooth LE connection. The headset sends mic audio only while the PC capture stream is open.

## SignalRGB

For a cable connected to the headset, set `CONFIG_HEADSET_USB_VID` and `CONFIG_HEADSET_USB_PID`. For wireless lighting, use the dongle's USB IDs and select dongle mode. Enter the IDs of the device connected to the PC in [Headphones.js](../signalrgb/Headphones.js). Zero IDs leave direct USB RGB off.

Each frame uses two 52-byte HID feature reports. Each report starts with ID 1, command 0x52, a sequence number and cup index, then 16 RGB triples. Send the left cup first and use the same sequence number for both. The dongle forwards up to 30 frames per second. The headset gives a live USB lighting feed priority and scales both rings to an estimated 245 mA total. A black frame, or the loss of both lighting feeds, shuts off the rail.

## First bench test

Test pairing, reconnect, media and calls on a phone. Check USB enumeration and run playback and mic capture together through the dongle. Check for clicks when switching sources, confirm boom mute in both modes and measure latency. Check the codec clocks (48 kHz WCLK and 1.536 MHz BCLK), LED data timing and LED current on the assembled headset.

- [BM83 UART guide](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/UserGuides/BM83_Host_MCU_Firmware_Development_Guide_DS50002896A.pdf)
- [TLV320AIC3204 register guide](https://www.ti.com/lit/ml/slaa557/slaa557.pdf)
- [SignalRGB plugin guide](https://docs.signalrgb.com/developer/plugins/)
