"""Parametric FreeCAD CAD generator and physical validator for the 1D Balance Cube.

All dimensions match simulink/params_1d_cube.m so that mass, CoM height,
flywheel inertia, and +/-45 deg stop geometry align across Simscape Multibody,
MATLAB Coder C99, KiCad electronics, and mechanical CAD.

Manufacturing split:
  * Square_Frame_140mm   : 6 mm cast acrylic, laser cut from the DXF.
  * Brass_Flywheel_90mm  : C36000 brass, CNC lathe/mill.
  * Pivot_Clamp_Hub,
    Base_And_Stops       : FDM 3D printed (PETG recommended), modelled print-ready
                           (hole compensation, teardrop horizontal holes, nut traps,
                           elephant-foot chamfer, flat print faces at Z min).
  * Stop_Pad_TPU         : FDM printed TPU 95A, 2 off, press into the stop pockets.
  * KP08 pillow blocks, 8 mm shaft, JGB37-520 motor, MPU-6050, fasteners: purchased
    (reference geometry only).

Global coordinate system (units: mm):
  X : Horizontal plane perpendicular to pivot axis (tilt moves +Z toward +X for +theta)
  Y : Pivot & flywheel spin axis (-Y = Front / Flywheel side, +Y = Rear / Motor side)
  Z : Vertical Up (Z = 0.0 is the pivot shaft centerline)

Run from the FreeCAD GUI (or freecadcmd):
    import sys; sys.path.insert(0, r"<repo>/cad")
    import generate_cad_models as g; g.build_all()
"""

from __future__ import annotations

import json
import math
from dataclasses import dataclass
from pathlib import Path

import FreeCAD as App
import MeshPart
import Part

CAD_DIR_DEFAULT = Path(__file__).resolve().parent


@dataclass(frozen=True)
class BalanceCubeCADParams:
    # Square frame (matches params_1d_cube.m)
    square_side_mm: float = 140.0
    plate_thickness_mm: float = 6.0          # nominal sheet thickness used for mass properties
    acrylic_measured_thickness_mm: float = 6.0  # MEASURE your sheet (cast acrylic varies +/-0.3 mm) and set this
    plate_density_kgm3: float = 1190.0
    pivot_corner_offset_mm: float = 15.0
    plate_corner_fillet_mm: float = 2.5
    pivot_nose_cutoff_z_mm: float = -10.5

    # Brass flywheel (matches params_1d_cube.m)
    flywheel_density_kgm3: float = 8500.0
    flywheel_outer_radius_mm: float = 45.0
    flywheel_inner_radius_mm: float = 33.0
    flywheel_rim_thickness_mm: float = 12.0
    flywheel_web_thickness_mm: float = 3.0
    flywheel_hub_boss_dia_mm: float = 16.0
    flywheel_bore_dia_mm: float = 6.0
    flywheel_dflat_mm: float = 5.5
    flywheel_setscrew_tap_mm: float = 2.5     # M3 tap drill
    # Set screw sits in the hub boss on the plate-facing (+Y local) side, clear of the web,
    # so it is reachable through a radial access hole in the rim (2 holes, 180 deg apart for balance).
    flywheel_setscrew_y_local_mm: float = 4.0
    flywheel_access_hole_dia_mm: float = 3.4  # passes an M3 grub screw and a 1.5 mm hex key
    flywheel_gap_from_plate_mm: float = 2.0

    # Motor: 12V JGB37-520 6.25:1 + rear 11 PPR encoder
    motor_mass_kg: float = 0.155
    motor_can_dia_mm: float = 37.0
    motor_can_len_mm: float = 48.0
    motor_encoder_dia_mm: float = 34.0
    motor_encoder_len_mm: float = 11.0
    motor_boss_dia_mm: float = 12.0
    motor_boss_len_mm: float = 2.5
    motor_shaft_len_mm: float = 15.0          # D-shaft protrusion beyond the gearbox face (MEASURE yours)
    motor_shaft_eccentricity_mm: float = 7.0  # Offset between output shaft and gearbox can
    motor_bolt_circle_dia_mm: float = 31.0
    motor_bolt_hole_dia_mm: float = 3.4
    # Only the three holes that receive screws are cut. The 90 deg hole would leave a 0.3 mm
    # acrylic web next to the 13 mm boss bore.
    motor_bolt_angles_deg: tuple = (30.0, 150.0, 270.0)
    motor_shaft_clear_bore_mm: float = 13.0

    # IMU: GY-521 / MPU-6050 breakout (symmetric top mount cleanly above flywheel)
    imu_mount_z_mm: float = 141.0
    imu_hole_spacing_mm: float = 15.5
    imu_hole_dia_mm: float = 3.2
    imu_mass_kg: float = 0.005

    # Pivot shaft, KP08 pillow blocks (purchased)
    pivot_shaft_dia_mm: float = 8.0
    pivot_shaft_len_mm: float = 96.0
    pivot_bore_plate_mm: float = 8.2
    kp08_y_offset_mm: float = 28.0
    kp08_center_height_mm: float = 15.0
    kp08_base_len_mm: float = 55.0
    kp08_bolt_spacing_mm: float = 42.0
    kp08_width_mm: float = 13.0
    steel_density_kgm3: float = 7850.0

    # FDM printing
    print_density_kgm3: float = 1270.0        # PETG, solid. Print the clamp hub at 100 % infill.
    print_hole_comp_mm: float = 0.2           # added to printed hole diameters (FDM holes print undersize)
    clamp_slot_clearance_mm: float = 0.15     # per side, acrylic slot in printed clamp hub
    clamp_cheek_top_z_mm: float = 23.5
    m3_nut_af_mm: float = 5.5
    m5_nut_af_mm: float = 8.0
    nut_trap_clearance_mm: float = 0.3
    elephant_foot_chamfer_mm: float = 0.5

    # Base (printed)
    base_len_x_mm: float = 180.0
    base_width_y_mm: float = 92.0
    base_thickness_mm: float = 8.0
    base_top_z_mm: float = -25.0

    # +/-45 deg stops with TPU pads
    stop_x_pos_mm: float = 65.0
    stop_width_x_mm: float = 16.0
    stop_depth_y_mm: float = 16.0
    stop_angle_deg: float = 45.0
    stop_pad_thickness_mm: float = 3.0
    stop_pad_pocket_depth_mm: float = 1.5
    stop_pad_size_mm: float = 12.6
    stop_pocket_size_mm: float = 13.0

    @property
    def L_center_mm(self) -> float:
        return self.square_side_mm / math.sqrt(2.0) - self.pivot_corner_offset_mm

    @property
    def stop_top_z_mm(self) -> float:
        # Exact world Z where the diamond lower edge sits horizontal at theta = +/-45 deg.
        # This is the top of the TPU pad (the contact surface).
        return -self.pivot_corner_offset_mm * math.sin(math.radians(self.stop_angle_deg))

    @property
    def stop_pillar_top_z_mm(self) -> float:
        return self.stop_top_z_mm - (self.stop_pad_thickness_mm - self.stop_pad_pocket_depth_mm)


# --------------------------------------------------------------------------- helpers

def _v(x: float, y: float, z: float) -> App.Vector:
    return App.Vector(float(x), float(y), float(z))


def _cylinder_y(radius: float, y_start: float, y_end: float, center_xz: tuple[float, float] = (0.0, 0.0)) -> Part.Shape:
    cx, cz = center_xz
    length = abs(y_end - y_start)
    ymin = min(y_start, y_end)
    return Part.makeCylinder(radius, length, _v(cx, ymin, cz), _v(0, 1, 0))


def _box_bounds(xmin: float, xmax: float, ymin: float, ymax: float, zmin: float, zmax: float) -> Part.Shape:
    return Part.makeBox(xmax - xmin, ymax - ymin, zmax - zmin, _v(xmin, ymin, zmin))


def _slot_y(cx: float, cz: float, length: float, width: float, angle_deg: float, y_start: float, y_end: float) -> Part.Shape:
    """Stadium-shaped zip-tie slot extruded along Y."""
    r = width * 0.5
    half_straight = max(0.1, 0.5 * length - r)
    th = math.radians(angle_deg)
    ux, uz = math.cos(th), math.sin(th)
    nx, nz = -math.sin(th), math.cos(th)

    c1x, c1z = cx - half_straight * ux, cz - half_straight * uz
    c2x, c2z = cx + half_straight * ux, cz + half_straight * uz

    p1 = _v(c1x + r * nx, y_start, c1z + r * nz)
    p2 = _v(c2x + r * nx, y_start, c2z + r * nz)
    p2_mid = _v(c2x + r * ux, y_start, c2z + r * uz)
    p3 = _v(c2x - r * nx, y_start, c2z - r * nz)
    p4 = _v(c1x - r * nx, y_start, c1z - r * nz)
    p1_mid = _v(c1x - r * ux, y_start, c1z - r * uz)

    edges = [
        Part.makeLine(p1, p2),
        Part.Arc(p2, p2_mid, p3).toShape(),
        Part.makeLine(p3, p4),
        Part.Arc(p4, p1_mid, p1).toShape(),
    ]
    face = Part.Face(Part.Wire(edges))
    return face.extrude(_v(0, y_end - y_start, 0))


def _extrude_xz_polygon(points_xz: list[tuple[float, float]], y_start: float, y_end: float) -> Part.Shape:
    pts = [_v(x, y_start, z) for x, z in points_xz]
    pts.append(pts[0])
    face = Part.Face(Part.makePolygon(pts))
    return face.extrude(_v(0, y_end - y_start, 0))


def _extrude_xy_polygon(points_xy: list[tuple[float, float]], z_start: float, z_end: float) -> Part.Shape:
    pts = [_v(x, y, z_start) for x, y in points_xy]
    pts.append(pts[0])
    face = Part.Face(Part.makePolygon(pts))
    return face.extrude(_v(0, 0, z_end - z_start))


def _teardrop(radius: float, axis: str, a_start: float, a_end: float, cu: float, cz: float,
              truncate_mm: float | None = None) -> Part.Shape:
    """Horizontal hole along X or Y with a 45 deg roof pointing +Z (prints without support).

    cu is the hole centre on the other horizontal axis (X for axis='y', Y for axis='x').
    truncate_mm flattens the roof at cz + truncate_mm (short bridge instead of a point).
    """
    s = radius / math.sqrt(2.0)
    kite = [(cu, cz), (cu + s, cz + s), (cu, cz + radius * math.sqrt(2.0)), (cu - s, cz + s)]
    if axis == "y":
        cyl = _cylinder_y(radius, a_start, a_end, (cu, cz))
        roof = _extrude_xz_polygon(kite, a_start, a_end)
    elif axis == "x":
        cyl = Part.makeCylinder(radius, a_end - a_start, _v(a_start, cu, cz), _v(1, 0, 0))
        pts = [_v(a_start, u, z) for u, z in kite]
        pts.append(pts[0])
        roof = Part.Face(Part.makePolygon(pts)).extrude(_v(a_end - a_start, 0, 0))
    else:
        raise ValueError(axis)
    shape = cyl.fuse(roof)
    if truncate_mm is not None:
        big = 4.0 * radius
        if axis == "y":
            cap = _box_bounds(cu - big, cu + big, a_start - 1, a_end + 1, cz + truncate_mm, cz + big)
        else:
            cap = _box_bounds(a_start - 1, a_end + 1, cu - big, cu + big, cz + truncate_mm, cz + big)
        shape = shape.cut(cap)
    return shape.removeSplitter()


def _hex_prism_y(af: float, cx: float, cz: float, y_start: float, y_end: float) -> Part.Shape:
    """Hex nut pocket along Y, flat on top: the roof is a short (~3.4 mm) bridge.

    (Vertex-up would give 30 deg roof faces, which sag worse than a short bridge.)
    """
    r = af / math.sqrt(3.0)
    pts = [(cx + r * math.cos(math.radians(60 * k)), cz + r * math.sin(math.radians(60 * k))) for k in range(6)]
    return _extrude_xz_polygon(pts, y_start, y_end)


def _hex_prism_z(af: float, cx: float, cy: float, z_start: float, z_end: float) -> Part.Shape:
    r = af / math.sqrt(3.0)
    pts = [(cx + r * math.cos(math.radians(60 * k)), cy + r * math.sin(math.radians(60 * k))) for k in range(6)]
    return _extrude_xy_polygon(pts, z_start, z_end)


def _round_box_xy_edges(box: Part.Shape, z_tol: float = 1e-4, radius: float = 3.0) -> Part.Shape:
    vert_edges = [
        e for e in box.Edges
        if abs(e.Vertexes[0].X - e.Vertexes[1].X) < z_tol
        and abs(e.Vertexes[0].Y - e.Vertexes[1].Y) < z_tol
    ]
    return box.makeFillet(radius, vert_edges) if vert_edges else box


def _round_xz_plate_edges(plate: Part.Shape, radius: float) -> Part.Shape:
    y_edges = [
        e for e in plate.Edges
        if abs(e.Vertexes[0].X - e.Vertexes[1].X) < 1e-4
        and abs(e.Vertexes[0].Z - e.Vertexes[1].Z) < 1e-4
    ]
    return plate.makeFillet(radius, y_edges) if y_edges else plate


def _chamfer_bottom_edges(shape: Part.Shape, z_bottom: float, size: float) -> Part.Shape:
    """Elephant-foot compensation on the first printed layer."""
    edges = [e for e in shape.Edges if abs(e.BoundBox.ZMin - z_bottom) < 1e-6 and abs(e.BoundBox.ZMax - z_bottom) < 1e-6]
    return shape.makeChamfer(size, edges) if edges else shape


def _motor_hole_xz(p: BalanceCubeCADParams, deg: float) -> tuple[float, float]:
    z_can = p.L_center_mm - p.motor_shaft_eccentricity_mm
    r_bc = 0.5 * p.motor_bolt_circle_dia_mm
    return r_bc * math.cos(math.radians(deg)), z_can + r_bc * math.sin(math.radians(deg))


CLAMP_BOLT_XZ = [(0.0, 17.0), (-9.0, 11.0), (9.0, 11.0)]


# --------------------------------------------------------------------------- parts

def build_square_frame(p: BalanceCubeCADParams, hole_comp: float = 0.0) -> Part.Shape:
    """6 mm diamond frame. Laser-cut acrylic by default; hole_comp > 0 grows every hole
    for an FDM-printed version (printed holes come out undersize)."""
    c = 0.5 * hole_comp
    h_diag = p.square_side_mm / math.sqrt(2.0)
    z_bot_tip = -p.pivot_corner_offset_mm
    z_mid = p.L_center_mm
    z_top_tip = z_bot_tip + 2.0 * h_diag
    half_y = 0.5 * p.plate_thickness_mm

    # Cut bottom tip flat at pivot_nose_cutoff_z_mm so the pivot bore has >6 mm wall
    # while staying completely inside the 45 deg rest edges.
    dx_bot = max(4.5, p.pivot_nose_cutoff_z_mm - z_bot_tip)
    poly_xz = [
        (-dx_bot, p.pivot_nose_cutoff_z_mm),
        (dx_bot, p.pivot_nose_cutoff_z_mm),
        (h_diag, z_mid),
        (0.0, z_top_tip),
        (-h_diag, z_mid),
    ]
    plate = _extrude_xz_polygon(poly_xz, -half_y, +half_y)

    # Fillet only the three outer tips (top, left, right); keep bottom clamp flat clean
    outer_y_edges = []
    for e in plate.Edges:
        is_y_edge = abs(e.Vertexes[0].X - e.Vertexes[1].X) < 1e-4 and abs(e.Vertexes[0].Z - e.Vertexes[1].Z) < 1e-4
        if is_y_edge and e.CenterOfMass.z > 10.0:
            outer_y_edges.append(e)
    if outer_y_edges:
        plate = plate.makeFillet(p.plate_corner_fillet_mm, outer_y_edges)

    cuts: list[Part.Shape] = []
    y0, y1 = -half_y - 1.0, half_y + 1.0

    # 1. Pivot shaft bore + 3x M3 clamp hub bolt holes
    cuts.append(_cylinder_y(0.5 * p.pivot_bore_plate_mm + c, y0, y1, (0.0, 0.0)))
    for hx, hz in CLAMP_BOLT_XZ:
        cuts.append(_cylinder_y(1.6 + c, y0, y1, (hx, hz)))

    # 2. Central motor output shaft & bearing-boss clearance bore (13 mm)
    cuts.append(_cylinder_y(0.5 * p.motor_shaft_clear_bore_mm + c, y0, y1, (0.0, z_mid)))

    # 3. JGB37-520 faceplate screws (only the 3 used positions)
    for deg in p.motor_bolt_angles_deg:
        hx, hz = _motor_hole_xz(p, deg)
        cuts.append(_cylinder_y(0.5 * p.motor_bolt_hole_dia_mm + c, y0, y1, (hx, hz)))

    # 4. Top symmetric MPU6050 IMU mount (M3 x 2, level along X axis above flywheel)
    for sign in (-1.0, 1.0):
        cuts.append(_cylinder_y(0.5 * p.imu_hole_dia_mm + c, y0, y1, (sign * 0.5 * p.imu_hole_spacing_mm, p.imu_mount_z_mm)))

    # 5. Zip-tie cable strain relief slots along lower-right edge
    for center_s in (56.0, 84.0):
        cx = center_s / math.sqrt(2.0) - 10.0 / math.sqrt(2.0)
        cz = (center_s / math.sqrt(2.0) + z_bot_tip) + 10.0 / math.sqrt(2.0)
        cuts.append(_slot_y(cx - 2.8, cz + 2.8, 5.2 + hole_comp, 2.2 + hole_comp, 45.0, y0, y1))
        cuts.append(_slot_y(cx + 2.8, cz - 2.8, 5.2 + hole_comp, 2.2 + hole_comp, 45.0, y0, y1))

    return plate.cut(Part.makeCompound(cuts)).removeSplitter()


def build_brass_flywheel(p: BalanceCubeCADParams) -> Part.Shape:
    """Local brass flywheel centred at (0, 0, 0), Y in [-rim/2, +rim/2]; +Y faces the acrylic plate."""
    half_rim = 0.5 * p.flywheel_rim_thickness_mm
    half_web = 0.5 * p.flywheel_web_thickness_mm
    r_out = p.flywheel_outer_radius_mm
    r_in = p.flywheel_inner_radius_mm
    r_boss = 0.5 * p.flywheel_hub_boss_dia_mm
    r_bore = 0.5 * p.flywheel_bore_dia_mm
    y_ss = p.flywheel_setscrew_y_local_mm

    wheel = _cylinder_y(r_out, -half_rim, +half_rim, (0.0, 0.0))
    rim_edges = [e for e in wheel.Edges if isinstance(e.Curve, Part.Circle) and abs(e.Curve.Radius - r_out) < 1e-3]
    if rim_edges:
        wheel = wheel.makeChamfer(0.5, rim_edges)

    cuts: list[Part.Shape] = []
    # Recessed front & rear annular pockets (leave central web + hub boss)
    for y_a, y_b in [(-half_rim - 0.5, -half_web), (half_web, half_rim + 0.5)]:
        cuts.append(_cylinder_y(r_in, y_a, y_b).cut(_cylinder_y(r_boss, y_a - 0.5, y_b + 0.5)))

    # 6.0 mm D-shaft bore (flat at +Z = dflat - r_bore = +2.5 mm)
    shaft_cyl = _cylinder_y(r_bore, -half_rim - 1.0, half_rim + 1.0)
    flat_cutter = _box_bounds(-r_bore - 1.0, r_bore + 1.0, -half_rim - 2.0, half_rim + 2.0,
                              p.flywheel_dflat_mm - r_bore, r_bore + 1.0)
    cuts.append(shaft_cyl.cut(flat_cutter))

    # Radial M3 set-screw tap from the bore through the boss onto the D-flat (+Z),
    # located in the boss beside the web so it exits into the plate-side pocket.
    cuts.append(Part.makeCylinder(0.5 * p.flywheel_setscrew_tap_mm, r_boss + 0.5, _v(0.0, y_ss, 0.0), _v(0, 0, 1)))

    # Radial access holes through the rim (aligned with the set screw, plus a balancing twin at -Z)
    r_acc = 0.5 * p.flywheel_access_hole_dia_mm
    for sgn in (1.0, -1.0):
        start = sgn * (r_in - 0.5)
        cuts.append(Part.makeCylinder(r_acc, (r_out - r_in) + 2.0, _v(0.0, y_ss, start), _v(0, 0, sgn)))

    return wheel.cut(Part.makeCompound(cuts)).removeSplitter()


def build_pivot_clamp_hub(p: BalanceCubeCADParams) -> Part.Shape:
    """Printed split-clamp hub joining the acrylic corner to the 8 mm shaft.

    Print orientation: as modelled (Z up, flat lug bottom on the bed, slot open upward).
    Horizontal holes are teardrops, cheek nuts sit in flat-top hex traps on the rear face.
    """
    comp = p.print_hole_comp_mm
    collar_r = 11.5
    collar_half_y = 16.0
    cheek_half_y = 9.0
    slot_half_y = 0.5 * p.acrylic_measured_thickness_mm + p.clamp_slot_clearance_mm
    r_m3 = 0.5 * (3.2 + comp)

    collar = _cylinder_y(collar_r, -collar_half_y, +collar_half_y, (0.0, 0.0))
    # Lower lug: 0.8 mm chamfer on all bed edges (elephant foot, no curled fillet overhang at the bed)
    lower_lug = _chamfer_bottom_edges(_box_bounds(-10.0, 10.0, -collar_half_y, +collar_half_y, -15.2, 0.0), -15.2, 0.8)

    top = p.clamp_cheek_top_z_mm
    cheek_poly = [(-10.0, -11.5), (10.0, -11.5), (26.0, 11.0), (10.0, top), (-10.0, top), (-26.0, 11.0)]
    cheeks = _round_xz_plate_edges(_extrude_xz_polygon(cheek_poly, -cheek_half_y, +cheek_half_y), 2.0)

    hub = collar.fuse(lower_lug).fuse(cheeks)

    cuts: list[Part.Shape] = []
    # Acrylic receiving slot (floor at Z = -11.2 forms a 4 mm bridge underneath)
    cuts.append(_box_bounds(-45.0, 45.0, -slot_half_y, +slot_half_y, -11.2, 35.0))

    # Shaft bore along Y: teardrop with a flat 0.3 mm roof so the shaft still locates
    r_bore = 0.5 * (p.pivot_shaft_dia_mm + comp)
    cuts.append(_teardrop(r_bore, "y", -collar_half_y - 1.0, collar_half_y + 1.0, 0.0, 0.0, truncate_mm=r_bore + 0.3))

    # Bottom pinch slits in the outer collar lobes
    for ya, yb in [(-collar_half_y - 1.0, -9.2), (9.2, collar_half_y + 1.0)]:
        cuts.append(_box_bounds(-0.7, 0.7, ya, yb, -16.0, -3.9))

    # 2x M3 pinch bolts across X (M3x25 + nut on the flat lug faces)
    for py in (-12.6, 12.6):
        cuts.append(_teardrop(r_m3, "x", -12.0, 12.0, py, -10.5))

    # 3x M3 acrylic clamping bolts along Y + hex nut traps on the rear (+Y) cheek face
    trap_af = p.m3_nut_af_mm + p.nut_trap_clearance_mm
    for hx, hz in CLAMP_BOLT_XZ:
        cuts.append(_teardrop(r_m3, "y", -cheek_half_y - 1.0, cheek_half_y + 1.0, hx, hz))
        cuts.append(_hex_prism_y(trap_af, hx, hz, cheek_half_y - 2.6, cheek_half_y + 1.0))

    return hub.cut(Part.makeCompound(cuts)).removeSplitter()


def build_base_and_stops(p: BalanceCubeCADParams) -> Part.Shape:
    """Printed base + 2x KP08 risers + +/-45 deg stop pillars with TPU pad pockets.

    Print orientation: as modelled (bottom face on the bed).
    """
    comp = p.print_hole_comp_mm
    hx = 0.5 * p.base_len_x_mm
    hy = 0.5 * p.base_width_y_mm
    z_top = p.base_top_z_mm
    z_bot = z_top - p.base_thickness_mm

    base = _round_box_xy_edges(_box_bounds(-hx, hx, -hy, hy, z_bot, z_top), radius=3.0)
    base = _chamfer_bottom_edges(base, z_bot, p.elephant_foot_chamfer_mm)

    z_riser_top = -p.kp08_center_height_mm
    risers = []
    for y_sign in (-1.0, 1.0):
        yc = y_sign * p.kp08_y_offset_mm
        risers.append(_round_box_xy_edges(_box_bounds(-30.0, 30.0, yc - 9.0, yc + 9.0, z_top - 0.5, z_riser_top), radius=2.0))

    z_pillar = p.stop_pillar_top_z_mm
    stops = []
    for x_sign in (-1.0, 1.0):
        xc = x_sign * p.stop_x_pos_mm
        pillar = _box_bounds(xc - 0.5 * p.stop_width_x_mm, xc + 0.5 * p.stop_width_x_mm,
                             -0.5 * p.stop_depth_y_mm, 0.5 * p.stop_depth_y_mm, z_top - 0.5, z_pillar)
        stops.append(_round_box_xy_edges(pillar, radius=1.5))

    solid = base
    for s in risers + stops:
        solid = solid.fuse(s)

    cuts: list[Part.Shape] = []
    # 4x M4 corner mounting holes with socket-head counterbores from the top
    for sx in (-1.0, 1.0):
        for sy in (-1.0, 1.0):
            cx, cy = sx * (hx - 10.0), sy * (hy - 9.0)
            cuts.append(Part.makeCylinder(0.5 * (4.5 + comp), p.base_thickness_mm + 2.0, _v(cx, cy, z_bot - 1.0), _v(0, 0, 1)))
            cuts.append(Part.makeCylinder(0.5 * (7.6 + comp), 5.0, _v(cx, cy, z_top - 4.2), _v(0, 0, 1)))

    # 4x M5 KP08 bolts with hex nut traps from underneath
    trap_af = p.m5_nut_af_mm + p.nut_trap_clearance_mm
    for y_sign in (-1.0, 1.0):
        yc = y_sign * p.kp08_y_offset_mm
        for x_sign in (-1.0, 1.0):
            bx = x_sign * 0.5 * p.kp08_bolt_spacing_mm
            cuts.append(Part.makeCylinder(0.5 * (5.3 + comp), 22.0, _v(bx, yc, z_bot - 1.0), _v(0, 0, 1)))
            cuts.append(_hex_prism_z(trap_af, bx, yc, z_bot - 1.0, z_bot + 4.5))

    # TPU pad pockets in the stop pillar tops
    half_pk = 0.5 * p.stop_pocket_size_mm
    for x_sign in (-1.0, 1.0):
        xc = x_sign * p.stop_x_pos_mm
        cuts.append(_box_bounds(xc - half_pk, xc + half_pk, -half_pk, half_pk, z_pillar - p.stop_pad_pocket_depth_mm, z_pillar + 1.0))

    return solid.cut(Part.makeCompound(cuts)).removeSplitter()


def build_stop_pad(p: BalanceCubeCADParams) -> Part.Shape:
    """TPU stop pad, local: centred on X/Y, Z in [0, thickness]. Print 2."""
    h = 0.5 * p.stop_pad_size_mm
    pad = _box_bounds(-h, h, -h, h, 0.0, p.stop_pad_thickness_mm)
    return _round_box_xy_edges(pad, radius=1.0)


# --------------------------------------------------------------------------- purchased reference parts

def build_kp08_reference(p: BalanceCubeCADParams, y_center: float) -> tuple[Part.Shape, Part.Shape]:
    """KP08 cast pillow block housing + steel inner bearing insert."""
    w = p.kp08_width_mm
    ya, yb = y_center - 0.5 * w, y_center + 0.5 * w
    z_base = -p.kp08_center_height_mm
    flange = _box_bounds(-27.5, 27.5, ya, yb, z_base, z_base + 5.5)
    neck = _box_bounds(-13.5, 13.5, ya, yb, z_base + 5.0, 0.0)
    boss = _cylinder_y(14.5, ya, yb, (0.0, 0.0))
    housing = flange.fuse(neck).fuse(boss)
    housing_cuts = [
        _cylinder_y(7.5, ya - 1.0, yb + 1.0, (0.0, 0.0)),
        Part.makeCylinder(2.7, 10.0, _v(-21.0, y_center, z_base - 1.0), _v(0, 0, 1)),
        Part.makeCylinder(2.7, 10.0, _v(+21.0, y_center, z_base - 1.0), _v(0, 0, 1)),
    ]
    housing = housing.cut(Part.makeCompound(housing_cuts)).removeSplitter()
    inner_race = _cylinder_y(7.4, ya - 2.0, yb + 2.0, (0.0, 0.0)).cut(_cylinder_y(4.0, ya - 3.0, yb + 3.0, (0.0, 0.0)))
    return housing, inner_race


def build_motor_dshaft(p: BalanceCubeCADParams) -> Part.Shape:
    """6 mm D-shaft (flat on +Z, matching the flywheel bore) protruding from the gearbox face."""
    y_face = 0.5 * p.plate_thickness_mm
    z_shaft = p.L_center_mm
    r = 0.5 * p.flywheel_bore_dia_mm - 0.02
    shaft = _cylinder_y(r, y_face - p.motor_shaft_len_mm, y_face, (0.0, z_shaft))
    flat_z = z_shaft + (p.flywheel_dflat_mm - 0.5 * p.flywheel_bore_dia_mm) - 0.05
    return shaft.cut(_box_bounds(-5, 5, y_face - p.motor_shaft_len_mm - 1, y_face + 1, flat_z, z_shaft + 5))


def build_motor_body(p: BalanceCubeCADParams) -> Part.Shape:
    y_face = 0.5 * p.plate_thickness_mm
    z_shaft = p.L_center_mm
    z_can = z_shaft - p.motor_shaft_eccentricity_mm
    can = _cylinder_y(0.5 * p.motor_can_dia_mm, y_face, y_face + p.motor_can_len_mm, (0.0, z_can))
    boss = _cylinder_y(0.5 * p.motor_boss_dia_mm, y_face - p.motor_boss_len_mm, y_face + 1.0, (0.0, z_shaft))
    return can.fuse(boss).removeSplitter()


def build_jgb37_motor_reference(p: BalanceCubeCADParams) -> tuple[Part.Shape, Part.Shape, Part.Shape]:
    """JGB37-520 gearmotor body + D-shaft, rear Hall encoder, and the frame-side fasteners."""
    y_face = 0.5 * p.plate_thickness_mm
    z_can = p.L_center_mm - p.motor_shaft_eccentricity_mm
    motor_metal = build_motor_body(p).fuse(build_motor_dshaft(p)).removeSplitter()

    y_enc = y_face + p.motor_can_len_mm
    enc = _cylinder_y(0.5 * p.motor_encoder_dia_mm, y_enc, y_enc + p.motor_encoder_len_mm, (0.0, z_can))
    pcb = _box_bounds(-12.0, 12.0, y_enc + 8.0, y_enc + 10.5, z_can - 21.0, z_can - 8.0)
    encoder_part = enc.fuse(pcb).removeSplitter()

    # M3 button heads on the flywheel side of the plate + clamp hub socket heads
    heads = [_cylinder_y(2.7, -4.6, -3.0, _motor_hole_xz(p, d)) for d in p.motor_bolt_angles_deg]
    heads += [_cylinder_y(2.7, -11.4, -9.0, xz) for xz in CLAMP_BOLT_XZ]
    return motor_metal, encoder_part, Part.makeCompound(heads)


def build_imu_and_electronics(p: BalanceCubeCADParams) -> tuple[Part.Shape, Part.Shape]:
    """MPU-6050 breakout on the acrylic + offboard ESP32/L298N carrier beside the rig."""
    imu_pcb = _box_bounds(-10.5, 10.5, 6.0, 7.6, p.imu_mount_z_mm - 8.0, p.imu_mount_z_mm + 8.0)
    imu_chip = _box_bounds(-2.0, 2.0, 7.6, 8.6, p.imu_mount_z_mm - 2.0, p.imu_mount_z_mm + 2.0)
    s1 = _cylinder_y(2.2, 3.0, 6.0, (-0.5 * p.imu_hole_spacing_mm, p.imu_mount_z_mm))
    s2 = _cylinder_y(2.2, 3.0, 6.0, (+0.5 * p.imu_hole_spacing_mm, p.imu_mount_z_mm))
    imu = imu_pcb.fuse(imu_chip).fuse(s1).fuse(s2)

    x0, y0, z0 = 154.0, -24.0, p.base_top_z_mm - p.base_thickness_mm + 4.0
    board = _box_bounds(x0, x0 + 72.0, y0, y0 + 54.0, z0, z0 + 1.6)
    esp32 = _box_bounds(x0 + 6.0, x0 + 34.0, y0 + 4.0, y0 + 48.0, z0 + 1.6, z0 + 9.0)
    l298_hs = _box_bounds(x0 + 44.0, x0 + 66.0, y0 + 14.0, y0 + 36.0, z0 + 1.6, z0 + 22.0)
    term_blk = _box_bounds(x0 + 46.0, x0 + 66.0, y0 + 38.0, y0 + 48.0, z0 + 1.6, z0 + 11.0)
    return imu, board.fuse(esp32).fuse(l298_hs).fuse(term_blk)


# --------------------------------------------------------------------------- document helpers

def _apply_style(obj, color: tuple[float, float, float], transparency: int = 0) -> None:
    if not App.GuiUp:
        return
    vo = obj.ViewObject
    vo.ShapeColor = color
    vo.Transparency = transparency
    vo.DisplayMode = "Flat Lines"


def _add_part(doc, name: str, shape: Part.Shape, color: tuple[float, float, float], transparency: int = 0):
    obj = doc.addObject("Part::Feature", name)
    obj.Shape = shape
    _apply_style(obj, color, transparency)
    return obj


def _add_link(doc, name: str, target, placement: App.Placement | None = None):
    lnk = doc.addObject("App::Link", name)
    lnk.LinkedObject = target
    if placement is not None:
        lnk.Placement = placement
    return lnk


def _export_laser_dxf(plate_shape: Part.Shape, dxf_path: Path) -> bool:
    """Project the acrylic plate onto the XY plane and export clean 1:1 DXF contours."""
    import importDXF

    temp_doc = App.newDocument("_DXF_Tmp")
    try:
        main_face = max((f for f in plate_shape.Faces if abs(f.normalAt(0, 0).y) > 0.99), key=lambda f: f.Area)
        face_flat = main_face.copy()
        face_flat.translate(_v(0.0, -main_face.CenterOfMass.y, 0.0))
        face_flat.rotate(_v(0, 0, 0), _v(1, 0, 0), -90.0)
        obj = temp_doc.addObject("Part::Feature", "Square_Frame_2D_Profile")
        obj.Shape = face_flat
        temp_doc.recompute()
        importDXF.export([obj], str(dxf_path))
        return dxf_path.exists() and dxf_path.stat().st_size > 0
    finally:
        App.closeDocument(temp_doc.Name)


def _export_step(objs, path: Path) -> None:
    """STEP export that resolves App::Link objects and keeps colours when the GUI is up."""
    try:
        if App.GuiUp:
            import ImportGui as _imp
        else:
            import Import as _imp
        _imp.export(objs, str(path))
    except Exception:
        Part.export(objs, str(path))


def _as_solid(shape: Part.Shape) -> Part.Shape:
    if isinstance(shape, Part.Solid):
        return shape
    if len(shape.Solids) == 1:
        return shape.Solids[0]
    return shape


def _mass_props(shape: Part.Shape, mass_kg: float | None = None, density_kgm3: float | None = None):
    """Return (mass_kg, CoM mm vector, I_yy about the pivot axis in kg*m^2).

    Either the density or a total mass (spread uniformly over the shape) must be given.
    """
    vol_mm3 = shape.Volume
    if mass_kg is None:
        mass_kg = vol_mm3 * 1e-9 * density_kgm3
    rho = mass_kg / (vol_mm3 * 1e-9)
    com = shape.CenterOfMass if not isinstance(shape, Part.Compound) else _compound_com(shape)
    if isinstance(shape, Part.Compound):
        i_cm = 0.0
        for s in shape.Solids:
            m_i = s.Volume * 1e-9 * rho
            i_cm += s.MatrixOfInertia.A22 * rho * 1e-15 + m_i * (((s.CenterOfMass.x - com.x) * 1e-3) ** 2 + ((s.CenterOfMass.z - com.z) * 1e-3) ** 2)
    else:
        i_cm = shape.MatrixOfInertia.A22 * rho * 1e-15
    i_pivot = i_cm + mass_kg * ((com.x * 1e-3) ** 2 + (com.z * 1e-3) ** 2)
    return mass_kg, com, i_pivot


def _compound_com(shape: Part.Shape) -> App.Vector:
    tot = 0.0
    acc = App.Vector(0, 0, 0)
    for s in shape.Solids:
        acc += s.CenterOfMass * s.Volume
        tot += s.Volume
    return acc * (1.0 / tot)



def analyze_rig(p: BalanceCubeCADParams, *, plate, plate_density, wheel_parts, wheel_axis_xz, wheel_envelope,
                wheel_bore_y, setscrew_y, clamp, shaft, motor, encoder, imu, fasteners, base, pads, others) -> dict:
    """Mass properties about the pivot, interference sweep and flywheel clearances for a rig.

    wheel_parts: [(shape in assembly position, density kg/m^3), ...] -- everything that spins with the wheel.
    wheel_envelope: (radius, y_min, y_max) of the spinning wheel.  wheel_bore_y: (y_min, y_max) of the D-bore.
    """
    # Pendulum (wheel spin inertia excluded; the wheel only counts as a point mass at its CoM)
    m_plate, c_plate, I_plate = _mass_props(plate, density_kgm3=plate_density)
    m_clamp, c_clamp, I_clamp = _mass_props(clamp, density_kgm3=p.print_density_kgm3)
    m_shaft, _, I_shaft = _mass_props(shaft, density_kgm3=p.steel_density_kgm3)

    ax, az = wheel_axis_xz
    m_wheel = 0.0
    mom = App.Vector(0, 0, 0)
    I_spin = 0.0
    for shp, rho in wheel_parts:
        for sol in shp.Solids:
            m_i = sol.Volume * 1e-9 * rho
            c = sol.CenterOfMass
            m_wheel += m_i
            mom += c * m_i
            I_spin += sol.MatrixOfInertia.A22 * rho * 1e-15 + m_i * (((c.x - ax) * 1e-3) ** 2 + ((c.z - az) * 1e-3) ** 2)
    com_wheel = mom * (1.0 / m_wheel)
    I_wheel_pend = m_wheel * ((com_wheel.x * 1e-3) ** 2 + (com_wheel.z * 1e-3) ** 2)

    motor_and_enc = Part.makeCompound([build_motor_body(p), encoder])
    m_motor, _, I_motor = _mass_props(motor_and_enc, mass_kg=p.motor_mass_kg)
    m_imu = p.imu_mass_kg
    I_imu = m_imu * (p.imu_mount_z_mm * 1e-3) ** 2
    # Fasteners as point masses: 3x M3x8 button (0.9 g), 3x M3x20 SHCS + nut (2.2 g), 2x M3x25 + nut (2.5 g)
    fast_pts = [(0.9e-3, _motor_hole_xz(p, d)) for d in p.motor_bolt_angles_deg]
    fast_pts += [(2.2e-3, xz) for xz in CLAMP_BOLT_XZ]
    fast_pts += [(2.5e-3, (0.0, -10.5))] * 2
    m_fast = sum(m for m, _ in fast_pts)
    I_fast = sum(m * ((x * 1e-3) ** 2 + (z * 1e-3) ** 2) for m, (x, z) in fast_pts)
    z_fast = sum(m * z for m, (_, z) in fast_pts) / m_fast

    def _row(m, z_mm, i_pivot):
        return {"m_kg": m, "z_com_m": z_mm * 1e-3, "I_pivot_kgm2": i_pivot}
    breakdown = {
        "plate": _row(m_plate, c_plate.z, I_plate),
        "clamp_hub": _row(m_clamp, c_clamp.z, I_clamp),
        "pivot_shaft": _row(m_shaft, 0.0, I_shaft),
        "flywheel_point_mass": _row(m_wheel, com_wheel.z, I_wheel_pend),
        "motor_encoder": _row(m_motor, _compound_com(motor_and_enc).z, I_motor),
        "imu": _row(m_imu, p.imu_mount_z_mm, I_imu),
        "fasteners": _row(m_fast, z_fast, I_fast),
    }
    m_total = sum(r["m_kg"] for r in breakdown.values())
    I_pend = sum(r["I_pivot_kgm2"] for r in breakdown.values())
    Mgl = 9.80665 * sum(r["m_kg"] * r["z_com_m"] for r in breakdown.values())
    mass = {
        "cad_plate_kg": round(m_plate, 5),
        "cad_clamp_hub_petg_kg": round(m_clamp, 5),
        "cad_pivot_shaft_kg": round(m_shaft, 5),
        "cad_flywheel_kg": round(m_wheel, 5),
        "cad_motor_encoder_kg": round(m_motor, 5),
        "cad_imu_fasteners_kg": round(m_imu + m_fast, 5),
        "cad_flywheel_I_spin_kgm2": float(f"{I_spin:.6e}"),
        "cad_flywheel_I_spin_exact_kgm2": I_spin,
        "cad_total_rotating_kg": round(m_total, 4),
        "cad_Ip_reduced_kgm2": float(f"{I_pend:.5e}"),
        "cad_Mgl_Nm": round(Mgl, 5),
        "breakdown": breakdown,
    }

    # Interference sweep: everything on the pendulum vs everything stationary, every 5 deg
    wheel_all = Part.makeCompound([s for s, _ in wheel_parts])
    rotating = {"plate": plate, "flywheel": wheel_all, "clamp": clamp, "motor": motor,
                "encoder": encoder, "imu": imu, "fasteners": fasteners}
    stationary = dict({"base": base, "pad_L": pads[0], "pad_R": pads[1]}, **others)
    sweep, min_clear = {}, {}
    lim = int(round(p.stop_angle_deg))
    for deg in range(-lim, lim + 1, 5):
        total_overlap = 0.0
        for rn, rs in rotating.items():
            rot = rs.copy()
            rot.rotate(_v(0, 0, 0), _v(0, 1, 0), float(deg))
            for sn, ss in stationary.items():
                d = rot.distToShape(ss)[0]
                if d < 1e-6:
                    total_overlap += rot.common(ss).Volume
                key = f"{rn}~{sn}"
                if key not in min_clear or d < min_clear[key][0]:
                    min_clear[key] = (round(d, 3), deg)
        sweep[f"{deg:+d}deg_overlap_mm3"] = round(float(total_overlap), 4)
    tightest = dict(sorted(min_clear.items(), key=lambda kv: kv[1][0])[:8])

    # Spinning flywheel (relative motion) vs everything fixed to the pendulum
    R, y0, y1 = wheel_envelope
    env = _cylinder_y(R, y0, y1, wheel_axis_xz)
    y_face = 0.5 * p.plate_thickness_mm
    shaft_tip = y_face - p.motor_shaft_len_mm
    flywheel_rel = {
        "envelope_vs_plate_mm": round(env.distToShape(plate)[0], 3),
        "envelope_vs_clamp_mm": round(env.distToShape(clamp)[0], 3),
        "envelope_vs_motor_body_mm": round(env.distToShape(build_motor_body(p))[0], 3),
        "envelope_vs_imu_mm": round(env.distToShape(imu)[0], 3),
        "flywheel_vs_fastener_heads_mm": round(wheel_all.distToShape(fasteners)[0], 3),
        "dshaft_in_bore_overlap_mm3": round(motor.common(wheel_all).Volume, 4),
        "dshaft_engagement_mm": round(min(y_face, wheel_bore_y[1]) - max(shaft_tip, wheel_bore_y[0]), 2),
        "setscrew_on_shaft": bool(shaft_tip < setscrew_y - 0.5 * p.flywheel_setscrew_tap_mm),
    }

    stop_contact = {}
    for deg in (-p.stop_angle_deg, p.stop_angle_deg):
        r = plate.copy()
        r.rotate(_v(0, 0, 0), _v(0, 1, 0), deg)
        pad = pads[0] if deg < 0 else pads[1]
        stop_contact[f"{deg:+.0f}deg"] = {
            "plate_to_pad_mm": round(r.distToShape(pad)[0], 4),
            "plate_to_base_mm": round(r.distToShape(base)[0], 3),
        }
    return {"mass": mass, "sweep": sweep, "tightest": tightest, "flywheel_rel": flywheel_rel, "stop_contact": stop_contact}


# --------------------------------------------------------------------------- build

PURCHASED_COLORS = {
    "Pivot_Shaft_8mm": (0.80, 0.82, 0.84),
    "PillowBlock_KP08_Front": (0.18, 0.36, 0.22),
    "PillowBlock_KP08_Rear": (0.18, 0.36, 0.22),
    "KP08_Insert_Rings": (0.82, 0.84, 0.86),
    "Motor_JGB37_520": (0.64, 0.67, 0.70),
    "Motor_Encoder_Rear": (0.14, 0.14, 0.16),
    "Fastener_Heads_M3": (0.85, 0.85, 0.88),
    "IMU_MPU6050": (0.10, 0.32, 0.68),
    "Electronics_Module_Offboard": (0.12, 0.45, 0.22),
}


def build_rig_assembly(path: Path, doc_name: str, p: BalanceCubeCADParams, *, frame, flywheel, clamp, base, pad,
                       wheel_pl: App.Placement, pad_pls: list, purchased: dict,
                       frame_name: str, flywheel_name: str, extra_members: list | None = None,
                       extra_fixed_to_frame: list | None = None, extra_fixed_to_flywheel: list | None = None):
    """Assembly-workbench document shared by the acrylic/brass and all-printed rigs.

    Pendulum chain: shaft -(revolute, +/-stop angle)- base;  clamp, frame, motor, encoder,
    IMU, fasteners fixed to the shaft/frame;  flywheel -(revolute)- motor.
    """
    from assembly_utils import ROT_Z_TO_Y, ExplodeMove, JointSpec, Member, build_assembly_document

    members = [
        Member(frame_name, link_target=frame),
        Member(flywheel_name, link_target=flywheel, placement=wheel_pl),
        Member("Pivot_Clamp_Hub", link_target=clamp),
        Member("Base_And_Stops", link_target=base),
        Member("Stop_Pad_TPU_Left", link_target=pad, placement=pad_pls[0]),
        Member("Stop_Pad_TPU_Right", link_target=pad, placement=pad_pls[1]),
    ]
    for n, s in purchased.items():
        members.append(Member(n, shape=s, color=PURCHASED_COLORS.get(n, (0.7, 0.7, 0.7))))
    members += extra_members or []

    origin = App.Placement()
    pivot = App.Placement(_v(0, 0, 0), ROT_Z_TO_Y)
    spin = App.Placement(_v(0, wheel_pl.Base.y, p.L_center_mm), ROT_Z_TO_Y)
    fixed = lambda n, a, b: JointSpec("Fixed", n, a, b, origin)
    lim = p.stop_angle_deg
    joints = [
        fixed("Fix_KP08_Front", "Base_And_Stops", "PillowBlock_KP08_Front"),
        fixed("Fix_KP08_Rear", "Base_And_Stops", "PillowBlock_KP08_Rear"),
        fixed("Fix_Pad_Left", "Base_And_Stops", "Stop_Pad_TPU_Left"),
        fixed("Fix_Pad_Right", "Base_And_Stops", "Stop_Pad_TPU_Right"),
        fixed("Fix_Electronics", "Base_And_Stops", "Electronics_Module_Offboard"),
        JointSpec("Revolute", "Pivot_Revolute", "Base_And_Stops", "Pivot_Shaft_8mm", pivot, (-lim, lim)),
        fixed("Fix_Clamp_to_Shaft", "Pivot_Shaft_8mm", "Pivot_Clamp_Hub"),
        fixed("Fix_KP08_Inner_Rings", "Pivot_Shaft_8mm", "KP08_Insert_Rings"),
        fixed("Fix_Frame_to_Clamp", "Pivot_Clamp_Hub", frame_name),
        fixed("Fix_Motor", frame_name, "Motor_JGB37_520"),
        fixed("Fix_Encoder", "Motor_JGB37_520", "Motor_Encoder_Rear"),
        fixed("Fix_IMU", frame_name, "IMU_MPU6050"),
        fixed("Fix_Fasteners", frame_name, "Fastener_Heads_M3"),
        JointSpec("Revolute", "Flywheel_Revolute", "Motor_JGB37_520", flywheel_name, spin),
    ]
    for n in extra_fixed_to_frame or []:
        joints.append(fixed(f"Fix_{n}", frame_name, n))
    for n in extra_fixed_to_flywheel or []:
        joints.append(JointSpec("Fixed", f"Fix_{n}", flywheel_name, n, spin))
    on_wheel = [flywheel_name] + list(extra_fixed_to_flywheel or [])

    explode = [
        ExplodeMove(on_wheel, _v(0, -70, 0)),
        ExplodeMove(["Fastener_Heads_M3"], _v(0, -35, 0)),
        ExplodeMove(["Motor_JGB37_520", "Motor_Encoder_Rear"], _v(0, 80, 0)),
        ExplodeMove(["Motor_Encoder_Rear"], _v(0, 40, 0)),
        ExplodeMove(["IMU_MPU6050"], _v(0, 45, 15)),
        ExplodeMove([frame_name] + list(extra_fixed_to_frame or []), _v(0, 0, 70)),
        ExplodeMove(["Pivot_Clamp_Hub"], _v(0, 0, 30)),
        ExplodeMove(["PillowBlock_KP08_Front"], _v(0, -45, 0)),
        ExplodeMove(["PillowBlock_KP08_Rear"], _v(0, 45, 0)),
        ExplodeMove(["Stop_Pad_TPU_Left", "Stop_Pad_TPU_Right"], _v(0, 0, 25)),
    ]
    if extra_fixed_to_flywheel:
        explode.insert(1, ExplodeMove(list(extra_fixed_to_flywheel), _v(0, -40, 0)))
    doc, asm, _, status = build_assembly_document(path, doc_name, "Base_And_Stops", members, joints, explode)
    if status != 0:
        App.Console.PrintWarning(f"{doc_name}: assembly solver returned {status}\n")
    return doc, asm


PART_COLORS = {
    "Square_Frame_140mm": ((0.82, 0.92, 0.98), 55),
    "Brass_Flywheel_90mm": ((0.86, 0.68, 0.21), 0),
    "Pivot_Clamp_Hub": ((0.20, 0.21, 0.24), 0),
    "Base_And_Stops": ((0.68, 0.71, 0.74), 0),
    "Stop_Pad_TPU": ((0.85, 0.25, 0.20), 0),
}

# Reference values from simulink/params_1d_cube.m (update if that file changes)
PARAMS_REF = {
    "flywheel_kg": 0.38721,
    "flywheel_I_spin_kgm2": 5.1430e-4,
    "total_kg": 0.7272,
    "Ip_reduced_kgm2": 0.00577,
}


def build_all(cad_dir: str | Path | None = None, close_parts_keep_assembly: bool = False) -> dict:
    """Regenerate every document, export and the verification report.

    The assembly links to the part documents, so they stay open by default.
    """
    p = BalanceCubeCADParams()
    cad_root = Path(cad_dir) if cad_dir else CAD_DIR_DEFAULT
    exports_dir = cad_root / "exports"
    cad_root.mkdir(parents=True, exist_ok=True)
    exports_dir.mkdir(parents=True, exist_ok=True)

    target_names = set(PART_COLORS) | {"BalanceCube_1D_Assembly"}
    for dname in list(App.listDocuments().keys()):
        if dname in target_names:
            App.closeDocument(dname)

    # Shapes
    s_plate = _as_solid(build_square_frame(p))
    s_wheel_local = _as_solid(build_brass_flywheel(p))
    y_wheel_center = -(0.5 * p.plate_thickness_mm + p.flywheel_gap_from_plate_mm + 0.5 * p.flywheel_rim_thickness_mm)
    wheel_pl = App.Placement(_v(0.0, y_wheel_center, p.L_center_mm), App.Rotation())
    s_wheel_assy = s_wheel_local.copy()
    s_wheel_assy.Placement = wheel_pl

    s_clamp = _as_solid(build_pivot_clamp_hub(p))
    s_base = _as_solid(build_base_and_stops(p))
    s_pad_local = _as_solid(build_stop_pad(p))
    pad_pls = [
        App.Placement(_v(sx * p.stop_x_pos_mm, 0.0, p.stop_top_z_mm - p.stop_pad_thickness_mm), App.Rotation())
        for sx in (-1.0, 1.0)
    ]
    s_pads = []
    for pl in pad_pls:
        sp = s_pad_local.copy()
        sp.Placement = pl
        s_pads.append(sp)

    s_shaft = _cylinder_y(0.5 * p.pivot_shaft_dia_mm, -0.5 * p.pivot_shaft_len_mm, +0.5 * p.pivot_shaft_len_mm)
    s_kp_f_housing, s_kp_f_ring = build_kp08_reference(p, -p.kp08_y_offset_mm)
    s_kp_r_housing, s_kp_r_ring = build_kp08_reference(p, +p.kp08_y_offset_mm)
    s_motor, s_encoder, s_fasteners = build_jgb37_motor_reference(p)
    s_imu, s_electronics = build_imu_and_electronics(p)

    # Part documents + exports
    part_specs = [
        ("Square_Frame_140mm", s_plate, ["step", "stl", "dxf"]),
        ("Brass_Flywheel_90mm", s_wheel_local, ["step", "stl"]),
        ("Pivot_Clamp_Hub", s_clamp, ["step", "stl"]),
        ("Base_And_Stops", s_base, ["step", "stl"]),
        ("Stop_Pad_TPU", s_pad_local, ["step", "stl"]),
    ]
    validation = {}
    part_docs = {}
    for name, shp, fmts in part_specs:
        validation[name] = {
            "valid": bool(shp.isValid() and len(shp.Solids) == 1),
            "solids": len(shp.Solids),
            "volume_mm3": round(shp.Volume, 2),
        }
        col, transp = PART_COLORS[name]
        doc = App.newDocument(name)
        obj = _add_part(doc, name, shp, col, transp)
        doc.recompute()
        doc.saveAs(str(cad_root / f"{name}.FCStd"))
        part_docs[name] = (doc, obj)

        if "step" in fmts:
            _export_step([obj], exports_dir / f"{name}.step")
        if "stl" in fmts:
            mesh = MeshPart.meshFromShape(Shape=shp, LinearDeflection=0.02, AngularDeflection=0.1, Relative=False)
            mesh.write(str(exports_dir / f"{name}.stl"))
        if "dxf" in fmts:
            _export_laser_dxf(shp, exports_dir / f"{name}_LaserCut.dxf")

    # Assembly-workbench document: fabricated parts are App::Links to their own .FCStd,
    # purchased parts are embedded. Joints: base grounded, pendulum revolute about the
    # pivot (limited to the +/-45 deg stops), flywheel revolute about the motor shaft.
    assy, asm = build_rig_assembly(
        cad_root / "BalanceCube_1D_Assembly.FCStd", "BalanceCube_1D_Assembly", p,
        frame=part_docs["Square_Frame_140mm"][1], flywheel=part_docs["Brass_Flywheel_90mm"][1],
        clamp=part_docs["Pivot_Clamp_Hub"][1], base=part_docs["Base_And_Stops"][1], pad=part_docs["Stop_Pad_TPU"][1],
        wheel_pl=wheel_pl, pad_pls=pad_pls,
        purchased={
            "Pivot_Shaft_8mm": s_shaft, "PillowBlock_KP08_Front": s_kp_f_housing, "PillowBlock_KP08_Rear": s_kp_r_housing,
            "KP08_Insert_Rings": s_kp_f_ring.fuse(s_kp_r_ring), "Motor_JGB37_520": s_motor,
            "Motor_Encoder_Rear": s_encoder, "Fastener_Heads_M3": s_fasteners, "IMU_MPU6050": s_imu,
            "Electronics_Module_Offboard": s_electronics,
        },
        frame_name="Square_Frame_140mm", flywheel_name="Brass_Flywheel_90mm",
    )
    _export_step([asm], exports_dir / "BalanceCube_1D_Assembly.step")

    # ---------------- Mass properties, interference sweep, flywheel clearances
    analysis = analyze_rig(
        p, plate=s_plate, plate_density=p.plate_density_kgm3,
        wheel_parts=[(s_wheel_assy, p.flywheel_density_kgm3)], wheel_axis_xz=(0.0, p.L_center_mm),
        wheel_envelope=(p.flywheel_outer_radius_mm, y_wheel_center - 0.5 * p.flywheel_rim_thickness_mm,
                        y_wheel_center + 0.5 * p.flywheel_rim_thickness_mm),
        wheel_bore_y=(y_wheel_center - 0.5 * p.flywheel_rim_thickness_mm, y_wheel_center + 0.5 * p.flywheel_rim_thickness_mm),
        setscrew_y=y_wheel_center + p.flywheel_setscrew_y_local_mm,
        clamp=s_clamp, shaft=s_shaft, motor=s_motor, encoder=s_encoder, imu=s_imu, fasteners=s_fasteners,
        base=s_base, pads=s_pads,
        others={"kp08_F": s_kp_f_housing, "kp08_R": s_kp_r_housing, "ring_F": s_kp_f_ring, "ring_R": s_kp_r_ring,
                "electronics": s_electronics},
    )
    mi = analysis["mass"]
    report = {
        "params_mm": {
            "square_side_mm": p.square_side_mm,
            "L_center_mm": round(p.L_center_mm, 3),
            "stop_contact_z_mm": round(p.stop_top_z_mm, 4),
            "stop_pillar_top_z_mm": round(p.stop_pillar_top_z_mm, 4),
            "flywheel_y_center_mm": round(y_wheel_center, 3),
            "clamp_slot_width_mm": round(p.acrylic_measured_thickness_mm + 2 * p.clamp_slot_clearance_mm, 3),
        },
        "part_validity": validation,
        "mass_inertia_comparison": dict(mi, **{
            "params_flywheel_kg": PARAMS_REF["flywheel_kg"],
            "params_flywheel_I_spin_kgm2": PARAMS_REF["flywheel_I_spin_kgm2"],
            "params_total_kg": PARAMS_REF["total_kg"],
            "params_Ip_reduced_kgm2": PARAMS_REF["Ip_reduced_kgm2"],
            "Ip_deviation_pct": round(100.0 * (mi["cad_Ip_reduced_kgm2"] - PARAMS_REF["Ip_reduced_kgm2"])
                                      / PARAMS_REF["Ip_reduced_kgm2"], 2),
        }),
        "interference_sweep": analysis["sweep"],
        "tightest_clearances_mm_at_deg": analysis["tightest"],
        "flywheel_relative_motion": analysis["flywheel_rel"],
        "stop_contact": analysis["stop_contact"],
    }
    (cad_root / "cad_verification_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")

    if close_parts_keep_assembly:
        for doc, _ in part_docs.values():
            App.closeDocument(doc.Name)

    if App.GuiUp:
        import FreeCADGui as Gui
        Gui.setActiveDocument(assy.Name)
        view = Gui.ActiveDocument.ActiveView
        if view is not None:
            view.viewIsometric()
            view.fitAll()

    return report


if __name__ == "__main__":
    print(json.dumps(build_all(), indent=2))
