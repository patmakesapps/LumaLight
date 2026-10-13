# LumaLight — enclosure spec (v2, open back, 2026-10-12)

Modelled in `LumaLight.blend` (Blender 5.1, 1 BU = 1 mm). Assembly sits at the origin in **LUMALIGHT ASSEMBLY**
(`LUMALIGHT PRINT PARTS` + linked electronics in `LUMALIGHT PLACED PARTS`). Electronics are parked at x 300+ in
numbered collections; the L-mark and wordmark are in `LOGOS`. Body coordinates: X = width (centred), Y = depth
(front outer face y = 0, back of the panel y = 56), Z up from the counter. Cutaway views are in `views/`.

## Why v2

v1 (bottom plate + spine, 40 deep) failed on the bench: the dupont housings on the ESP32 pins pointed at the
front wall with 0.5 mm to the ring pocket, and the ToF board had to be threaded down from the top past them with
its JST and cable attached (`bench reference/bad_design _01/_02.jpg`). v2 opens the **back** instead of the bottom,
hangs the ESP32 on the back panel, gives the wires an 18 mm bay, and makes the sensor a push-in-from-behind part.
Everything can be wired on the bench first.

## Concept

An upright white monolith, 62 × 56 × 100 mm (body 53 deep + 3 mm back panel), vertical front edges chamfered 2.5,
top edges 1 mm. The front carries a Ø46 halo (transparent diffuser disc, NeoPixel ring behind it) and a 16 × 5 mm
"visor" slit for the ToF sensor below it. The LUMALIEN L-mark is recessed into the top face with purple glued
inlays; **LUMALIGHT** is recessed on the front near the base, **LUMALIEN** on the back panel. The back panel is
held by 4 screws into corner bosses; the USB-C cable leaves through a notch at the bottom of the panel.

## Layout (mm)

| Item | Position | Notes |
|---|---|---|
| Walls | 3.0 everywhere | cavity x ±28, y 3..53, z 3..97, open at the back |
| Halo | centre (0, z 65) | through hole Ø40; front recess Ø46 × 1.5 for the diffuser flange |
| Ring pocket rib | inner front wall, ID 37.4 / OD 44, y 3..9.5 | ring loads from the **front** through the Ø40 hole, LEDs forward, 0.3 radial play |
| Stop tabs | 3 tabs at 90°/210°/330°, 20° of arc, 1.5 inward, front face y 8.3 (45° underside to y 6.8 at the root) | ring back face rests at y 8.3; LED tops at y 4.7, 1.7 air gap to the diffuser; rotate the ring so its pads miss the tabs |
| Diffuser | flange Ø45.8 × 1.5 + boss Ø39.7 × 1.5, flush with the front | glued in; it captures the ring. Unchanged from v1 |
| Sensor slit | 16 × 5 through the wall, centre z 36.5, 1 mm outward chamfer | the VL53L1X package sits 21.5 above the board's bottom edge |
| Sensor board | x −12.5..10.5, PCB y 10..11.6, z 15..41.5 | component side (package **and** JST) faces the wall; JST housing y 4..10, cable exits downward at x −7..5 |
| Sensor mount | seats y 3..10 at x −15.7..−11.8 and 9.8..13.7 (0.7 under the board edges), cheeks y 10..12.7 at x −15.7..−12.7 and 10.7..13.7, bottom ledges z 13..15 at x −15.7..−9.5 and 7.5..13.7 | board is **pushed straight on from the back** and clicks behind two snap fingers (1.0 × 5, z 25.5..30.5, hooks 0.6 at y 11.7). Nothing above it, nothing to slide. 12 mm under the board for the cable to turn back |
| ESP32 | vertical on the back panel, USB-C down, bottom edge z 42, centred in X; PCB y 44.4..46, components toward the panel, pins toward the front | header plastic y 41.9..44.4, dupont housings y 27.9..41.9 |
| Wire bay | y 9.5 (rib back) .. 27.9 (housing tips) | 18.4 mm for the dupont leads to turn; 19.6 mm to the ring's back face; plus 14 mm of free channel each side of the ESP32, full depth |
| Standoffs (on the panel) | 4 × Ø6 × 7 at (±11.6, z 44.5) and (±11.6, z 91.3), Ø2.0 pilots 5.5 deep | 4 × M2.5 × 6 self-tap. Same hole pattern as v1, which fit |
| USB-C plug | hangs below the board, overmold centred y ≈ 47.6, keepout x ±6, y 44..51.5, z 14..41 | 1.5 mm to the panel; cable turns in the 11 mm above the floor |
| Back panel | 62 × 100 × 3 at y 53..56, back vertical edges chamfered 2.5, top-back edge 1.0 | 1.5 mm alignment lip (frame 1.5 wide, 55.4 × 93.4) enters the cavity; relieved at the corners and the notch |
| Panel screws | 4 at (±25, 6) and (±25, 94): Ø2.8 through, counterbore Ø5.5 × 1.5 from the outside | into 6 × 6 × 10 corner bosses in the body (y 43..53), Ø2.0 pilots 8 deep; M2.5 × 8 self-tap, driven from the back |
| Cable notch | 10 wide × 16 tall, rounded top, bottom centre of the panel, open at the bottom edge | cable exits backward along the counter |
| Top L-mark | 22 tall, centred on the top face (y 17..39), 1.2 deep | purple inlays unchanged (0.15 clearance, 1.4 thick → 0.2 proud); glue in |
| Text | LUMALIGHT front, centre z 11; LUMALIEN on the panel, centre z 23; Segoe UI Bold 7, 0.6 deep | |

## Print parts (5)

| Part | Material | Orientation |
|---|---|---|
| LUMALIGHT BODY | white | **front face on the bed**, open back up, no supports. The front gets the plate texture; every inner feature (rib, tabs, seats, fingers, bosses) grows straight up |
| LUMALIGHT BACK PANEL | white | outer face on the bed, standoffs and lip up, no supports |
| LUMALIGHT DIFFUSER | transparent | flange face down (bed texture frosts it) |
| LUMALIGHT LOGO INLAY L / TRI | purple | flat |
| TEST_SENSOR_POCKET (`STL/test/`) | any | front face down. 44 × 48 slab of the front wall with the sensor pocket, slit and floor stub, ~20 min. Print this and the panel before the body |

STLs in `STL/`, print orientation, 1:1 mm. Hardware: 4 × M2.5 × 6 self-tap (ESP32 to panel), 4 × M2.5 × 8 self-tap
(panel to body). No inserts, no magnets, every screw driven along its axis from outside. Glue: diffuser, inlays.

## Assembly order (everything pre-wired)

1. On the bench: screw the ESP32 to the panel standoffs, components toward the panel, pins toward you. Feed the
   USB-C cable through the panel notch from the outside and plug it in. Plug the sensor's leads and the ring's
   leads onto the pins. The whole harness is now live and the body is still empty.
2. Sensor: hold the board by its edges with the JST toward the front wall and the cable hanging down. Push it
   straight into the pocket from the back until both side fingers click over it. Tuck the cable under the board
   toward the back.
3. Ring: in from the front through the halo hole, wires first, LEDs facing out, until it seats on the three tabs.
   Turn it so the solder pads sit between tabs. The panel just lies behind the body while you do this; the leads
   are long enough to stay plugged in.
4. Glue the diffuser into the front recess. The ring is now captured.
5. Bring the panel up to the body, fold the dupont leads into the bay, seat the lip, 4 screws from the back.
6. Glue the purple inlays into the top recess.

To service it: 4 screws, the panel comes away with the ESP32 on it and everything stays plugged in.

## Verify on the bench before/after printing

- Snap fingers: 0.4 mm hook engagement over the board edge. If the board pops out when you tug the cable,
  a dab of hot glue on one cheek is the fallback.
- The slit is centred on the package 21.5 mm above the board's bottom edge; if the real package sits elsewhere
  the slit moves, nothing else does.
- The JST housing is modelled 6 mm tall with 1 mm to the wall. If the real one stands taller, say so and the
  seats get taller.
- Dupont housings modelled 14 mm long on 2.5 mm header plastic. Bay is 18.4 mm; the ring pads sit at y 8.3.

## Suggested print order

1. `STL/test/TEST_SENSOR_POCKET.stl` (~20 min): push the real sensor in with its cable on, check the click and that the cable turns under the board.
2. `LUMALIGHT_BACK_PANEL.stl`: screw the ESP32 on with all leads plugged, hold a ruler against it. From the panel's inner face the housing tips reach 25 mm; the ring's back is 44.7 mm away, the front wall 50 mm.
3. Only then the body.
