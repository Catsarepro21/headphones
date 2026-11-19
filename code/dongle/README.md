# USB dongle

The PC sees a 48 kHz, 16-bit USB headset with stereo playback, a mono mic and media buttons. The USB connection also accepts SignalRGB frames for the two LED rings. The dongle and headset use one encrypted Bluetooth LE bond. The dongle LED turns on when the link is ready.

## Build

Use the same Zephyr 4.2.0 workspace as the [headset firmware](../headset/). Set the USB vendor and product IDs to your assigned values before building.

```sh
west build -b headphones_dongle -d build-dongle /path/to/headphones/code/dongle -- \
  -DCONFIG_DONGLE_USB_VID="$USB_VID" -DCONFIG_DONGLE_USB_PID="$USB_PID"
```

The output is `build-dongle/zephyr/zephyr.hex`. The dongle uses the nRF52840 high-voltage supply. J1 provides VTref, SWDIO, SWDCLK and ground. Measure VTref before connecting the probe. A factory-default REGOUT0 may give 1.8 V, so use a probe that works at the measured target voltage. The green link LED may appear off at 1.8 V. During first-article programming, provision REGOUT0 for 3.0 V and reset the device, then verify VTref and check the LED on link. The application leaves UICR and DCDCEN0 alone. [Nordic REGOUT0 settings](https://docs.nordicsemi.com/r/bundle/ps_nrf52840/page/uicr.html)

Fit C1 at 4.7 µF on VBUS/VDDH and C2 at 10 µF on VDD. D1 is the bottom-side USB data protection diode near the plug. Use the updated placement file and allow clearance under D1 before closing the case.

On the first connection, plug in the dongle, select dongle mode on the headset and press Pair. Pair the two boards near each other. To switch to a different pair of boards, clear the bond storage on both boards. The settings partition starts at `0xf8000` and is 32 KiB. Keep the dongle's UICR settings when clearing it.
