# Headset firmware

This runs on the E73-2G4M08S1C on the main PCB. The selector switches the headphone and mic paths. In Bluetooth mode, the four buttons handle pairing, play/pause and volume. The boom switch cuts the mic in hardware when folded up.

Set the BM83 to Host MCU mode at 115200 baud with A2DP sink, AVRCP and HFP/HSP enabled. The charger stays in USB 100 mA mode and reports its status over SEGGER RTT. The codec clock runs from the 12.288 MHz oscillator. Its analog path starts powered down.

## Build

From a Zephyr 4.2.0 workspace with the Arm toolchain and the `cmsis`, `cmsis_6`, `hal_nordic` and `segger` modules:

```sh
west build -b headphones_headset /path/to/headphones/code/headset
```

The build writes `build/zephyr/zephyr.hex`. J2 is the SWD header on the main board. Check the 3.3 V reference at J2 before connecting the probe.

## SignalRGB over the headset USB-C port

Use your assigned USB IDs for `CONFIG_HEADSET_USB_VID` and `CONFIG_HEADSET_USB_PID` and enter the same values in [Headphones.js](../signalrgb/Headphones.js). Zero IDs leave USB RGB off.

Each frame uses two 52-byte HID feature reports. Each report starts with ID 1, command 0x52, a sequence number and cup index, then 16 RGB triples. Send the left cup first and use the same sequence number for both. The firmware scales both rings to an estimated 245 mA total. A black frame, unplugging USB or one second without frames shuts off the LED rail.

## Still to do

Write USB audio playback and mic capture, the 2.4 GHz dongle link and the codec PCM path. On the first build, test pairing, reconnect, media and calls on a phone. Check for clicks when switching sources and confirm the boom mute. Measure the codec clocks (48 kHz WCLK and 1.536 MHz BCLK), LED data timing and LED current.

- [BM83 UART guide](https://ww1.microchip.com/downloads/aemDocuments/documents/OTH/ProductDocuments/UserGuides/BM83_Host_MCU_Firmware_Development_Guide_DS50002896A.pdf)
- [TLV320AIC3204 register guide](https://www.ti.com/lit/ml/slaa557/slaa557.pdf)
- [SignalRGB plugin guide](https://docs.signalrgb.com/developer/plugins/)
