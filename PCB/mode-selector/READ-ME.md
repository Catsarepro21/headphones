# Bluetooth/dongle selector board and retainer

The right earcup in Onshape includes the dedicated C&K JS102011SAQN selector, a rounded removable retainer, a compliant seal and the thumb slide. The KiCad board is 11 × 7.45 × 1.0 mm and uses the C&K footprint with two 0.9 mm locating holes. The STEP file shows the bare PCB. Gerber, drill and component position files are in [the fabrication folder](../fabrication/mode-selector/). Order this board in 1.0 mm thickness.

KiCad reported zero DRC violations and zero unconnected items. Onshape reported zero solid interferences with all 34 top-level instances and the boom deployed. A separate solid-geometry check covered the nominal 2 mm travel and six points along the carrier extraction path. The flexible diaphragm and cap socket still require a physical check for drag, grip and leakage.

Use the two plated J1 holes for a flexible pigtail soldered from the underside. J1-1 is common (SW1-2) and connects to headset main SW8-1, BT_MODE_N. J1-2 is the selected throw (SW1-3) and connects to main SW8-2, ground. Main R33 pulls the open position high through 10 kΩ. Route the pigtail through the relief under the carrier. Confirm both contact states before marking mode labels.

The retainer uses two Olander .2C5SHCS M2 × 5 socket screws and two CNC Kitchen TC-M2x3.0 heat-set inserts. The projected insert seats are Ø3.2 mm and 4.05 mm deep. Set the inserts flush with the housing face. Choose 0.40 mm silicone sheet and cut the diaphragm to the modeled outline. Compression stops set the assembled seal thickness to 0.30 mm. Remove the retainer and unplug or free the pigtail before drawing the carrier outward.

- [C&K selector data sheet](https://www.littelfuse.com/assetdocs/littelfuse-c-k-slide-js-series-datasheet?assetguid=aba42b08-0d2c-423b-813d-a2faa5a3bb14)
- [CNC Kitchen insert dimensions](https://cnckitchen.com/s/CNCKitchen_Heat-Set-Insert-Dimensions-and-Design-Guidelines.pdf)
- [Olander M2 × 5 screw](https://www.olander.com/items/.2C5SHCS)
