# LumaLight — enclosure spec (v1 concept, 2026-10-12)

Modelled in `LumaLight.blend` (Blender 5.1, 1 BU = 1 mm). Assembly sits at the origin in **LUMALIGHT ASSEMBLY**
(`LUMALIGHT PRINT PARTS` + linked electronics in `LUMALIGHT PLACED PARTS`). Electronics are parked at x 300+ in
numbered collections; the L-mark and wordmark are in `LOGOS`. Body coordinates: X = width (centred), Y = depth
(front outer face y = 0, back y = 40), Z up from the counter.

## Concept

An upright white monolith, 62 × 40 × 100 mm, vertical edges chamfered 2.5 mm, top edges 1 mm. The front carries a
Ø46 halo (transparent diffuser disc, NeoPixel ring behind it) and a 16 × 5 mm "visor" slit for the ToF sensor
below it. The LUMALIEN L-mark is recessed into the top face with a purple glued inlay; **LUMALIGHT** is recessed
on the front near the base, **LUMALIEN** on the back. The USB-C cable leaves through a notch at the back-bottom.

## Layout (mm)

| Item | Position | Notes |
|---|---|---|
| Walls | 3.0 everywhere | cavity x ±28, y 3..37, z 3..97, open bottom |
| Halo | centre (0, z 60) | through hole Ø40; front recess Ø46 × 1.5 for the diffuser flange |
| Ring pocket rib | inner front wall, ID 37.4 / OD 44, 6.5 tall (y 3..9.5) | ring loads from the **front** through the Ø40 hole, LEDs forward, 0.3 radial play |
| Stop tabs | 3 tabs at the back of the rib, 90°/210°/330°, 20° of arc, 1.5 inward, y 8.3..9.5 | ring back face rests on them at y 8.3; LED tops at y 4.7, 1.7 air gap to the diffuser; rotate the ring so its wires miss the tabs |
| Diffuser | flange Ø45.8 × 1.5 + boss Ø39.7 × 1.5, flush with the front | glued in; it is what captures the ring. No retainer, no screws inside the cavity |
| Sensor slit | 16 × 5 through the wall, centre z 30.5, 1 mm outward chamfer | sized for the VL53L1X 27° FOV at 10 mm behind the face |
| Sensor rails | C-channels on the inner front wall at x −15.7..−11.2 and 9.2..13.7, slot y 10..12, z 9..35, bottom stops at z 8..9 | board drops in from above (before the ring goes in), JST points down, cable bends in the 6 mm above the base plate |
| Sensor board | x −12.5..10.5, PCB y 10..11.6, z 9..35.5 | component side faces the wall; package assumed at (12.5, 21.5) on the board |
| ESP32 | vertical, USB-C down, bottom edge z 34, centred in X; PCB y 26.7..28.3, pins toward the front | components face the spine; dupont housings occupy y 10..24 |
| Spine | on the base plate, 36 wide × 3 thick, y 33.8..36.8 (flush with the plate's back edge), z 3..92; cable notch x ±6 up to z 16 | 4 standoffs Ø6 × 5.5 at (±11.6, 36.5) and (±11.6, 83.3), Ø2.0 pilots, M2.5 self-tap |
| USB-C plug | hangs below the board, y ≈ 26.6..33.2, down to z ≈ 11 | cable bends to the back-wall notch 9 wide × 10 tall |
| Base plate | 55.6 × 33.6 × 3, flush with the bottom | 2 × M2.5 self-tap at (±25, 20) into 6 × 6 bosses on the inner side walls, counterbored from below |
| Top L-mark | 22 tall, centred on the top face, 1.2 deep | purple inlays (L + triangle) 0.15 clearance, 1.4 thick → 0.2 proud; glue in |
| Text | LUMALIGHT front, centre z 11; LUMALIEN back, centre z 23; Segoe UI Bold 7, 0.6 deep | |

## Print parts (5)

| Part | Material | Orientation |
|---|---|---|
| LUMALIGHT BODY | white | upside down, top face on the bed, no supports |
| LUMALIGHT BASE PLATE + spine | white | on its back: spine face on the bed, standoffs pointing up, the plate stands as a 33.6 mm fin. No supports |
| LUMALIGHT DIFFUSER | transparent | flange face down (bed texture frosts it) |
| LUMALIGHT LOGO INLAY L / TRI | purple | flat |

STLs in `STL/`, print orientation, 1:1 mm. Hardware: 4 × M2.5 × 6 self-tap (ESP32 to spine), 2 × M2.5 × 8 self-tap
(base plate). No inserts, no magnets, no screws that need a driver inside the cavity. Glue: diffuser, inlays.

## Assembly order

1. Sensor board into its rails (JST down). With the halo hole still open you can guide it in with a finger from the front.
2. Ring in from the front: feed its three wires through the Ø40 hole first, then the ring, LEDs facing out, until it seats on the three tabs. Rotate it so the wires sit between tabs.
3. Diffuser into the front recess and glue it. The ring is now captured.
4. Screw the ESP32 to the spine (components toward the spine). Plug the USB-C cable in and the ring and sensor dupont leads onto the pins while the plate assembly is still outside the body.
5. Slide the base plate assembly up into the body, pass the cable through the back notch, 2 screws from below.
6. Glue the purple inlays into the top recess.

## Verify on the bench before printing

- ESP32 hole pattern (modelled as Ø3.0 at 2.5 in from each edge, 46.8 × 23.2) and board length 51.8 — from a forum measurement, not this exact USB-C board.
- CQRobot board width 23.0 (rails are cut to 23.4) and the position of the sensor package relative to the board's bottom edge (slit is centred 21.5 above it).
- The ring passes the Ø40 hole with its wires attached; if the solder blobs on the back pads stand more than ~1.5 mm proud they may touch the tabs, so pick a rotation where the pads sit between tabs.
