# Boom mute assembly

The editable Onshape model has a cam integrated into the rotating boom, a guided follower, a pocket for the B3F-1000, a wire channel and a removable guard. The existing 95° revolute mate is preserved. The guard uses one CNC Kitchen TC-M2x3.0 heat-set insert and one Accu SSCF-M2-5-A4-BL M2 × 5 socket screw. The right shell stays a single solid.

The live assembly reported no solid interferences with all 37 instances at the deployed pose. The live mate preview reached −95°. A separate solid sweep checked the cam and boom against the new mount, guard, switch case and terminal envelope at 14 poses from deployed to folded. The grouped follower and switch reference depict the deployed state. The preview does not solve electrical contacts or the follower’s elastic or axial travel.

## Wiring and fitting

1. Verify the normally-open contacts with a continuity meter. Use one pin from each contact group. Use short flexible leads with insulation over the solder joints and leave the other terminals electrically isolated from the enclosure and each other.
2. Insert the follower and fit the switch from the rear. Retain the switch with a nonconductive shim against the guard so the case cannot move under load. Keep solder joints unloaded and route the twisted switch leads through the side channel. Secure the wires inside the cup with enough slack to remove the guard.
3. Disconnect the capsule from the main board for the first cam check. Set follower or shim thickness against the actual switch. With the boom deployed, advance only far enough to close the normally-open contact reliably. Check return through the full fold and verify that the contact is open at the parked stop. Check that the deployed position does not bottom out the tact. The B3F-1000 pretravel spec is 0.15–0.45 mm. Its nominal minimum applicable load is specified separately as a reference.
4. Put the switch contact in series with the capsule hot lead to main J7 pin 1. Keep J7 pin 2 as the continuous capsule ground. Check deploy, fold and re-deploy with a meter first. Then verify mute depth, contact noise, bias and fold/unfold transients in both microphone modes on the powered unit.

[Aratas B3F data sheet](https://components.omron.com/system/files/2023-01/datasheet_pdf/A070-E1.pdf)
