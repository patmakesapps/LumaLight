"""LumaLight enclosure v2.1 (2026-10-12): open-back body + back panel carrying the ESP32; sensor screwed to two blocks.
Run with CadQuery 2.8 / Python 3.12:  python build_lumalight.py  -> STL/ (print orientation) + asm_*.stl for the blend.
World frame (assembly): X width (centred), Y depth (front outer face y=0), Z up. 1 unit = 1 mm.
"""
import math, os, sys, tempfile
import cadquery as cq

OUT_STL = os.path.join(os.path.dirname(os.path.abspath(__file__)), "STL")
# assembly-oriented meshes for the blend go to a scratch folder (set LUMALIGHT_ASM_DIR to choose)
SCR = os.environ.get("LUMALIGHT_ASM_DIR") or os.path.join(tempfile.gettempdir(), "lumalight_asm")
os.makedirs(SCR, exist_ok=True)
FONT = "C:/Windows/Fonts/segoeuib.ttf"

# ---------------- layout numbers ----------------
W, D, H = 62.0, 53.0, 100.0      # body outer; depth excludes the panel (56 with it)
T = 3.0                          # wall
PT = 3.0                         # panel thickness  -> total depth 48
HALO_Z = 65.0
HALO_HOLE, HALO_RECESS, RECESS_D = 40.0, 46.0, 1.5
RIB_ID, RIB_OD, RIB_Y0, RIB_Y1 = 37.4, 44.0, 3.0, 9.5
TAB_Y = 8.3                      # ring back face rests here
# CQRobot VL53L1X board (vendor drawing B07F3TV3G4 + ROLX-4 model): 23 wide x 28.5 tall, cable edge down.
TOF_W, TOF_H = 23.0, 28.5
TOF_X0, TOF_X1 = -TOF_W / 2, TOF_W / 2   # board centred on the package
TOF_Y0, TOF_Y1 = 10.0, 11.6              # PCB (component side toward the wall)
TOF_Z0 = 12.3                            # board bottom (cable) edge; 9.3 mm under it for the plug + bend
PKG_UP = 24.65                           # package centre above the cable edge (3.85 below the hole edge)
HOLE_UP, HOLE_DX = 25.3, 8.2             # 2 x dia 3.0 holes at the top corners (drawing 16.1 / ROLX-4 16.8 apart)
PKG_Z = TOF_Z0 + PKG_UP                  # 36.95
HOLE_Z = TOF_Z0 + HOLE_UP                # 37.6
WIN_W, WIN_H, WIN_CH = 11.0, 7.0, 0.5    # stadium window through the front wall, outward chamfer
ESP_Z0 = 42.0                    # ESP32 bottom (USB) edge
ESP_PCB_Y0 = 36.4                # pin side face of the PCB
STANDOFF_H = 7.0
PANEL_Y0 = D                     # 45
PANEL_Y1 = D + PT                # 48
BOSS = 6.0
SCREW_XY = [(-25.0, 6.0), (25.0, 6.0), (-25.0, 94.0), (25.0, 94.0)]
ESP_HOLES = [(-11.6, ESP_Z0 + 2.5), (11.6, ESP_Z0 + 2.5), (-11.6, ESP_Z0 + 49.3), (11.6, ESP_Z0 + 49.3)]
NOTCH_W, NOTCH_H = 10.0, 16.0
TEXT_FRONT_Z, TEXT_BACK_Z, TEXT_SIZE, TEXT_D = 11.0, 23.0, 7.0, 0.6
LMARK_D = 1.2


def box(x0, x1, y0, y1, z0, z1):
    return cq.Workplane("XY").box(x1 - x0, y1 - y0, z1 - z0, centered=False).translate((x0, y0, z0))


def cyl_y(x, z, r, y0, y1):
    """Cylinder along +Y from y0 to y1."""
    return cq.Workplane("XY").circle(r).extrude(y1 - y0).rotate((0, 0, 0), (1, 0, 0), -90).translate((x, y0, z))


# ================= BODY =================
# outer plan profile: front corners chamfered 2.5
plan = [(-28.5, 0), (28.5, 0), (31, 2.5), (31, D), (-31, D), (-31, 2.5)]
body = cq.Workplane("XY").polyline(plan).close().extrude(H)
try:
    body = body.faces(">Z").edges("not >Y").chamfer(1.0)
except Exception as e:
    print("top chamfer fallback:", e)
    body = body.faces(">Z").edges("<Y").chamfer(1.0)

# cavity, open at the back
body = body.cut(box(-28, 28, T, D + 1, T, H - T))

# halo hole + diffuser recess
body = body.cut(cyl_y(0, HALO_Z, HALO_HOLE / 2, -1, T + 1))
body = body.cut(cyl_y(0, HALO_Z, HALO_RECESS / 2, -1, RECESS_D))

# ring pocket rib with 3 stop tabs (built with axis on Z, then turned to Y)
rib = cq.Workplane("XY").circle(RIB_OD / 2).circle(RIB_ID / 2).extrude(RIB_Y1 - RIB_Y0).translate((0, 0, RIB_Y0))
prof = [(17.2, TAB_Y), (18.7, TAB_Y - 1.5), (19.6, TAB_Y - 1.5), (19.6, RIB_Y1), (17.2, RIB_Y1)]
tab0 = cq.Workplane("XZ").polyline(prof).close().revolve(20, (0, 0, 0), (0, 1, 0))
bb = tab0.val().BoundingBox()
start = 0.0 if bb.ymax > 1.0 else -20.0        # which way did it revolve
for ang in (270.0, 150.0, 30.0):                # -> world 90 / 210 / 330
    rib = rib.union(tab0.rotate((0, 0, 0), (0, 0, 1), ang - 10.0 - start))
rib = rib.rotate((0, 0, 0), (1, 0, 0), -90).translate((0, 0, HALO_Z))
body = body.union(rib)

# sensor mount: the board is pushed straight on from the back between two loose cheeks, its component
# face rests on the side seats, and two M2.5 screws into blocks at its top-corner holes hold it.
SEAT_Z0, SEAT_Z1 = TOF_Z0 - 2.0, TOF_Z0 + TOF_H          # 10.3 .. 40.8
BLOCK_X0, BLOCK_H = 5.9, 5.0                              # screw block from 0.4 past the window edge to the seat
SEAT_IN, CHEEK_PLAY = 1.0, 0.75
sens = None
for side in (-1, 1):
    def X(a, b):
        return (min(side * a, side * b), max(side * a, side * b))
    lo, hi = X(TOF_W / 2 - SEAT_IN, 15.7)         # seat: the component face rests on a 1 mm band along each edge
    s_ = box(lo, hi, T, TOF_Y0, SEAT_Z0, SEAT_Z1)
    lo, hi = X(TOF_W / 2 + CHEEK_PLAY, 15.7)      # cheek: loose side guide (0.75 play), 1.1 proud of the back face
    s_ = s_.union(box(lo, hi, TOF_Y0, TOF_Y1 + 1.1, SEAT_Z0, SEAT_Z1))
    # no bottom ledge and nothing at the corners: the board has R4 corners and the two screws set its position
    lo, hi = X(BLOCK_X0, 15.7)                    # screw block, joined to the seat
    s_ = s_.union(box(lo, hi, T, TOF_Y0, HOLE_Z - BLOCK_H / 2, HOLE_Z + BLOCK_H / 2))
    sens = s_ if sens is None else sens.union(s_)
body = body.union(sens)
for side in (-1, 1):                              # dia 2.0 pilots, 6.5 deep, M2.5 x 6 self-tap from the back
    body = body.cut(cyl_y(side * HOLE_DX, HOLE_Z, 1.0, T + 0.5, TOF_Y0 + 0.5))

# sensor window: 11 x 7 stadium centred on the package, 0.5 mm outward chamfer
body = body.cut(cq.Workplane("XZ", origin=(0, T + 1, PKG_Z)).slot2D(WIN_W, WIN_H).extrude(T + 2))
flare = (cq.Workplane("XZ", origin=(0, WIN_CH, PKG_Z)).slot2D(WIN_W, WIN_H)
         .workplane(offset=WIN_CH).slot2D(WIN_W + 2 * WIN_CH, WIN_H + 2 * WIN_CH).loft())
body = body.cut(flare)

# corner bosses for the panel screws (pilot along Y from the back)
for (sx, sz) in SCREW_XY:
    x0 = sx - BOSS / 2 if sx > 0 else sx - BOSS / 2
    body = body.union(box(sx - BOSS / 2, sx + BOSS / 2, D - 10.0, D, sz - BOSS / 2, sz + BOSS / 2))
    body = body.cut(cyl_y(sx, sz, 1.0, D - 8.0, D + 1))

# front text
txt = cq.Workplane("XZ", origin=(0, TEXT_D, TEXT_FRONT_Z)).text("LUMALIGHT", TEXT_SIZE, TEXT_D, combine=False, fontPath=FONT)
body = body.cut(txt)

# top L-mark recess, from the parked l_LOGO outline (park x/z -> top face x/y)
s = 22.0 / 36.78
def m(px, pz):
    return ((px - 406.77) * s - 9.94, (pz + 13.16) * s + (D + PT - 22.0) / 2)
L_poly = [m(406.77, -13.16), m(433.14, -13.16), m(437.67, -4.58), m(417.04, -4.58), m(417.04, 15.77), m(406.77, 23.62)]
T_poly = [m(418.88, -2.43), m(440.0, -2.43), m(418.88, 18.7)]
for poly in (L_poly, T_poly):
    body = body.cut(cq.Workplane("XY", origin=(0, 0, H)).polyline(poly).close().extrude(-LMARK_D))

# ================= BACK PANEL =================
pplan = [(-31, PANEL_Y0), (31, PANEL_Y0), (31, PANEL_Y0 + 0.5), (28.5, PANEL_Y1), (-28.5, PANEL_Y1), (-31, PANEL_Y0 + 0.5)]
panel = cq.Workplane("XY").polyline(pplan).close().extrude(H)
try:
    panel = panel.faces(">Z").edges("not <Y").chamfer(1.0)
except Exception as e:
    print("panel top chamfer fallback:", e)
    try:
        panel = panel.faces(">Z").edges(">Y").chamfer(1.0)
    except Exception as e2:
        print("panel chamfer skipped:", e2)

# alignment lip inside the cavity opening
lip = box(-27.7, 27.7, PANEL_Y0 - 1.5, PANEL_Y0, 3.3, H - 3.3).cut(box(-26.2, 26.2, PANEL_Y0 - 2, PANEL_Y0 + 1, 4.8, H - 4.8))
for sx in (-1, 1):
    for z0, z1 in ((0, 10.5), (H - 10.5, H)):
        lip = lip.cut(box(min(sx * 21, sx * 32), max(sx * 21, sx * 32), PANEL_Y0 - 2, PANEL_Y0 + 1, z0, z1))
lip = lip.cut(box(-NOTCH_W / 2 - 1, NOTCH_W / 2 + 1, PANEL_Y0 - 2, PANEL_Y0 + 1, 0, 10.5))
panel = panel.union(lip)

# ESP32 standoffs with pilots (9 deep, into the panel). v2.1: the EN/BOOT buttons sit 1.5 mm above the two
# USB-end holes (z 46..52, 3.5 tall), so those two standoffs get an open-sided C neck for the first 3.6 mm:
# dia 5 clipped 1.0 above the hole centre, nothing on the button side. Full dia 6 above that.
NECK_H, NECK_R, NECK_CLIP = 3.6, 2.5, 1.0
for (hx, hz) in ESP_HOLES:
    top = PANEL_Y0 - STANDOFF_H                       # 46, the PCB's component face
    if hz < ESP_Z0 + 10.0:                            # USB-end pair, next to the buttons
        neck = cyl_y(hx, hz, NECK_R, top, top + NECK_H + 0.01)
        neck = neck.cut(box(hx - 4, hx + 4, top - 1, top + NECK_H + 1, hz + NECK_CLIP, hz + 6))
        panel = panel.union(neck).union(cyl_y(hx, hz, 3.0, top + NECK_H, PANEL_Y0))
    else:
        panel = panel.union(cyl_y(hx, hz, 3.0, top, PANEL_Y0))
    panel = panel.cut(cyl_y(hx, hz, 1.0, top - 0.5, PANEL_Y1 - 1.0))

# panel screws: through hole + counterbore from the outside
for (sx, sz) in SCREW_XY:
    panel = panel.cut(cyl_y(sx, sz, 1.4, PANEL_Y0 - 3, PANEL_Y1 + 1))
    panel = panel.cut(cyl_y(sx, sz, 2.75, PANEL_Y1 - 1.5, PANEL_Y1 + 1))

# USB-C cable notch, open to the bottom edge, rounded top
notch = box(-NOTCH_W / 2, NOTCH_W / 2, PANEL_Y0 - 3, PANEL_Y1 + 1, -1, NOTCH_H - NOTCH_W / 2)
notch = notch.union(cyl_y(0, NOTCH_H - NOTCH_W / 2, NOTCH_W / 2, PANEL_Y0 - 3, PANEL_Y1 + 1))
panel = panel.cut(notch)

# back text, reads correctly from behind
back_plane = cq.Plane(origin=(0, PANEL_Y1 - TEXT_D, TEXT_BACK_Z), xDir=(-1, 0, 0), normal=(0, 1, 0))
btxt = cq.Workplane(back_plane).text("LUMALIEN", TEXT_SIZE, TEXT_D, combine=False, fontPath=FONT)
panel = panel.cut(btxt)

# ================= export =================
def report(name, wp):
    sol = wp.val()
    bb = sol.BoundingBox()
    print(f"{name:22s} solids={len(wp.solids().vals())} vol={sol.Volume():9.0f} mm3  "
          f"x {bb.xmin:7.2f}..{bb.xmax:7.2f}  y {bb.ymin:7.2f}..{bb.ymax:7.2f}  z {bb.zmin:7.2f}..{bb.zmax:7.2f}")

report("BODY", body)
report("BACK PANEL", panel)

# assembly-oriented meshes for Blender
cq.exporters.export(body, os.path.join(SCR, "asm_BODY.stl"), tolerance=0.01, angularTolerance=0.1)
cq.exporters.export(panel, os.path.join(SCR, "asm_BACK_PANEL.stl"), tolerance=0.01, angularTolerance=0.1)

# print orientation: body front face on the bed; panel outer face on the bed
body_p = body.rotate((0, 0, 0), (1, 0, 0), 90)       # (x,y,z)->(x,-z,y): front face -> z=0
bb = body_p.val().BoundingBox()
body_p = body_p.translate((0, -bb.ymin, -bb.zmin))
panel_p = panel.rotate((0, 0, 0), (1, 0, 0), -90)    # (x,y,z)->(x,z,-y): outer face -> z=0
bb = panel_p.val().BoundingBox()
panel_p = panel_p.translate((0, -bb.ymin, -bb.zmin))
report("BODY print", body_p)
report("PANEL print", panel_p)
cq.exporters.export(body_p, os.path.join(OUT_STL, "LUMALIGHT_BODY.stl"), tolerance=0.01, angularTolerance=0.1)
cq.exporters.export(panel_p, os.path.join(OUT_STL, "LUMALIGHT_BACK_PANEL.stl"), tolerance=0.01, angularTolerance=0.1)
# ---- test coupon: sensor pocket only (front wall x +-22, z 0..48, floor stub to y 14) ----
os.makedirs(os.path.join(OUT_STL, "test"), exist_ok=True)
coupon = body.intersect(box(-22, 22, -1, 14, -1, 48))
coupon_p = coupon.rotate((0, 0, 0), (1, 0, 0), 90)
bb = coupon_p.val().BoundingBox(); coupon_p = coupon_p.translate((0, -bb.ymin, -bb.zmin))
report("COUPON print", coupon_p)
cq.exporters.export(coupon_p, os.path.join(OUT_STL, "test", "TEST_SENSOR_POCKET.stl"), tolerance=0.01, angularTolerance=0.1)
print("exported")
