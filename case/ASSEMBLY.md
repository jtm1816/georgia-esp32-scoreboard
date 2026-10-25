# Georgia scoreboard case — design revision 6

![Actual CAD geometry](case-preview.png)

## Parts and colors

| File | Color | Quantity |
| --- | --- | --- |
| front-bezel-red.stl | Red | 1 |
| lettering-white.stl | White | 1 set of 15 letters |
| rear-shell-black.stl | Black | 1 |
| stand-black.stl | Black | 1 |

The bezel has no stylus holder. The black cradle holds the screen 30 degrees back from vertical (60 degrees above the desk). The case body is 100 � 80 � 24 mm. A solid black lower plinth fills the gap from the bottom of the bezel to the flat base. Its front edge aligns with the lower bezel edge, with no projecting front tray.

## Hardware assumptions — check against your actual parts

- PCB outline: 86 × 50 mm; PCB thickness: 1.6 mm.
- Four mounting holes: 78 × 42 mm center spacing, nominal 3.2 mm board-hole diameter.
- Screen assembly: approximately 70 × 50 mm and 4 mm above the PCB. Visible front opening: 60 × 45.5 mm. The screen pocket is centered; actual board revisions may have different offsets or display dimensions.
- Four **M3 heat-set inserts, 4.6 mm outside diameter × 4.0 mm long**. The model uses 4.6 mm pilot pockets, 4.3 mm deep, in 7.2 mm bosses. Insert suppliers specify their own pilot sizes; adjust `insert_od` to the supplier's recommended pilot diameter. Do not use longer inserts in these blind pockets without redesigning the boss stack.
- Four **M3 × 20 mm button-head screws**. Rear head pockets are 6.2 mm diameter × 1.8 mm deep. Verify actual screw-head dimensions and insertion depth; screws must not bottom out in the inserts.
- Rear USB-C opening: **14 mm wide x 5.5 mm tall**, horizontal, with no extra clearance. A vertical mounting pad rests on the 3 mm flat base, so the cable enters straight backward parallel to the desk. An internal recess leaves a 2.4 mm mounting face. Adapter retention and internal wiring clearance need checking against the actual adapter.

The user elected to skip the mounting-hole fit-check print. These files use published dimensions; no physical fit or load testing has been performed. Digital solid checks do not prove hardware fit.

## PETG printing recommendations

Use red PETG for the bezel, white PETG for the lettering, and black PETG for the shell and stand. These are starting settings, not a tested printer profile. Use your filament maker's temperature profile and dry filament as directed.

For a 0.4 mm nozzle, start with 0.20 mm layers, 4 walls, 5 top/bottom layers, and 20-30% gyroid infill. Print the small white letters at 0.12-0.16 mm layers if desired. Use a 3-5 mm brim where adhesion is limited, especially for the lettering. Use a PETG-compatible build surface; follow its maker's release-layer instructions.

| Part | Orientation | Supports |
| --- | --- | --- |
| Red bezel | Front face down, bosses up | Off initially. Inspect the shallow lettering-pocket bridges and screen pocket in the slicer. Keep support out of insert pockets. |
| White letters | Flat, supplied orientation | Off. Print slowly and keep the matching layout for assembly. |
| Black rear shell | Supplied orientation, projecting USB pad lowest | Normal/snug supports under the elevated rear face; allow supports from model surfaces where needed inside the port pad. Use manual painting to cover unsupported pad ceilings. Avoid filling screw bores with support. |
| Black stand | Flat base down | Off initially. Inspect brace slopes and the top of the solid lower support for unsupported toolpaths. |

For PETG supporting PETG, start with a **0.30 mm top Z gap**, **0.30 mm bottom Z gap**, **0.6 mm XY separation**, **3 interface layers**, **80% interface density**, and **12-15% support density**. Use a 45-degree overhang threshold measured from the horizontal; slicers use different angle conventions. These values trade underside finish for easier removal. Slicers may round Z gaps to layer increments, so inspect the effective gap. Do not use a zero Z gap for same-material supports. Add local supports only where the preview shows they are needed, and ensure they remain removable through the open shell.

The rear shell is the most support-dependent part. Preview every layer around the vertical USB-C mounting pad; the opening must stay 14 x 5.5 mm after cleanup. A side orientation may reduce the broad supported rear surface but increases shell height and may need a brim. No orientation has been physically tested.

Import all STLs in millimeters at 100%. Each file's lowest point is at the bed. Letter pockets have approximately 0.12 mm side clearance and seat 0.6 mm deep, leaving the letters 0.6 mm raised. Use suitable adhesive if needed.

Sources: [Prusa PETG material guide](https://help.prusa3d.com/article/petg_2059) and [Sovol support settings guide](https://www.sovol3d.com/blogs/news/3d-printing-support-settings-explained-how-to-get-cleaner-easier-to-remove-fdm-prints). Part orientations and support placements above are design-specific recommendations based on this CAD geometry.

## Assembly

1. Install the white GEORGIA and BULLDOGS letters into their matching recesses on the front bezel.
2. Install the four M3 inserts from the **inside/rear** of the front bezel. Keep them coaxial and flush with the boss tops; protect the thin front skin from excess heat or insertion force.
3. Install the USB-C power adapter in the rear opening, and provide internal strain relief appropriate to its actual body. The model supplies the opening, not custom latches for an unspecified adapter.
4. With USB disconnected, connect the adapter's regulated 5 V and ground to the board's 5 V/VIN and GND. This port is power only. Identify the actual board connections before wiring.
5. Seat the board screen-first into the bezel pocket. The four front bosses support the PCB around its mounting holes; the display must not bear screw-clamping force.
6. Place the black rear shell over the board. Its hollow support columns contact the rear of the PCB at the same four mounting holes.
7. Insert each rear screw along the shared axis: **rear screw → rear support column → PCB hole → front heat-set insert**. Tighten only until seated. The screws must neither bend the PCB nor press the screen against the bezel.
8. Seat the assembled case against the black cradle's rear side braces, which set the 30-degree angle. The solid lower plinth supports the case underside. Secure the shell to the two rear braces with suitable adhesive or thin mounting tape so it cannot slide forward when tapped. Keep the rear screws and USB opening clear.
9. Remove the rear shell when access to BOOT, RESET, or the original USB programming socket is needed.

## Editable source

`parameters.json` contains the dimensions. `design_case.py` regenerates the STLs and the colored `case-assembly.glb` preview. Install Python dependencies with:

```text
pip install manifold3d trimesh numpy fonttools
python design_case.py
```

The lettering uses Arial Black when available. On other systems, set `GEORGIA_CASE_FONT` to a bold TrueType font path before regenerating. A different font changes the letter pockets, so always regenerate the bezel and lettering together.

`case-assembly.glb` is for viewing the assembled design; it includes non-printable screen reference. Print the individual STLs. `validation.json` records digital mesh checks, not a physical-fit certificate.

## References

- [Manufacturer specifications, mounting drawing on page 6](https://mischianti.org/wp-content/uploads/2025/04/ESP32-2432S028-Specifications-EN.pdf)
- [Element14 board dimensions and technical drawing](https://community.element14.com/technologies/embedded/b/blog/posts/esp32-cheap-yellow-display-cyd-guide-with-a-jellyfish-example)

The Element14 drawing is approximate and describes a different USB-port revision; board geometry must be checked against the actual hardware.