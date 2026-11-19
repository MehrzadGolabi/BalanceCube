"""All-3D-printed variant of the 1D Balance Cube.

Replaces the laser-cut acrylic frame and the machined brass flywheel with FDM parts:

  * Printed_Frame_140mm       : the same diamond frame as the acrylic part, printed in PETG,
                                with every hole grown by print_hole_comp_mm.
  * Printed_Flywheel_Ballast  : PETG flywheel whose rim carries N steel hex bolts + nuts as
                                ballast (ISO 4017 bolts, ISO 4032 nuts) in hex pockets, so
                                most of the spin inertia comes from cheap hardware at a large
                                radius. Printed D-bore hub with a captive M3 nut + grub screw on
                                the motor D-flat, reached through a radial access hole in the rim.

The clamp hub, base, TPU stop pads and all purchased parts are shared with the acrylic rig
(generate_cad_models.py). Every dimension is a field of PrintedRigParams.

Run from the FreeCAD GUI (or freecadcmd):
    import sys; sys.path.insert(0, r"<repo>/cad")
    import generate_printed_models as gp; gp.build_all_printed()
"""

from __future__ import annotations

import json
import math
from dataclasses import dataclass
from pathlib import Path

import FreeCAD as App
import MeshPart
import Part

import generate_cad_models as g

_v = g._v

# ISO 4017 hex bolt / ISO 4032 nut, nominal: d, across-flats s, head height k, nut height m
ISO_HEX = {
    "M5": (5.0, 8.0, 3.5, 4.7),
    "M6": (6.0, 10.0, 4.0, 5.2),
    "M8": (8.0, 13.0, 5.3, 6.8),
    "M10": (10.0, 16.0, 6.4, 8.4),
}


@dataclass(frozen=True)
class PrintedRigParams(g.BalanceCubeCADParams):
    # Printed frame
    frame_fill_fraction: float = 1.0          # effective density / solid PETG density (100 % infill recommended)

    # Printed flywheel body (PETG). Local frame: Y = spin axis, y = 0 at the hub's plate-side face,
    # wheel extends toward -Y (away from the plate).
    pfw_outer_radius_mm: float = 55.0         # limited by the clamp hub: 84 - 55 = 29 mm vs cheek top 23.5 mm
    pfw_hub_radius_mm: float = 9.0            # must stay inside the motor-screw heads (r >= 10.7 mm)
    pfw_web_thickness_mm: float = 4.0
    pfw_hub_gap_mm: float = 1.5               # plate face -> hub face (more shaft engagement)
    pfw_rim_setback_mm: float = 0.5           # hub face -> rim plate-side face (rim-to-plate gap = 2.0 mm)
    pfw_rim_wall_mm: float = 2.0              # PETG wall around the ballast hex pockets
    pfw_bore_comp_mm: float = 0.1             # printed D-bore grows by this
    pfw_setscrew_y_mm: float = 3.5            # grub screw position from the hub face
    pfw_edge_chamfer_mm: float = 0.8
    pfw_body_fill_fraction: float = 1.0       # 100 % infill keeps the wheel balanced
    pocket_clearance_mm: float = 0.3          # hex pocket oversize (AF and depth)

    # Ballast hardware (steel)
    ballast_size: str = "M8"
    ballast_bolt_len_mm: float = 20.0
    ballast_count: int = 12
    ballast_nuts_per_bolt: int = 1
    ballast_shank_mass_factor: float = 0.92   # threaded shank ~ 0.92 d effective diameter for mass

    @property
    def ballast_dims(self):
        return ISO_HEX[self.ballast_size]

    @property
    def pocket_af(self) -> float:
        return self.ballast_dims[1] + self.pocket_clearance_mm

    @property
    def pocket_corner_r(self) -> float:
        return 0.5 * self.pocket_af / math.cos(math.radians(30))

    @property
    def ballast_circle_r(self) -> float:
        return self.pfw_outer_radius_mm - self.pocket_corner_r - self.pfw_rim_wall_mm

    @property
    def rim_inner_r(self) -> float:
        return self.ballast_circle_r - self.pocket_corner_r - self.pfw_rim_wall_mm

    @property
    def head_pocket_depth(self) -> float:
        return self.ballast_dims[2] + self.pocket_clearance_mm

    @property
    def nut_pocket_depth(self) -> float:
        return self.ballast_nuts_per_bolt * self.ballast_dims[3] + self.pocket_clearance_mm

    @property
    def rim_thickness(self) -> float:
        # head recessed in the far face, bolt tip flush with the plate-side face
        return self.head_pocket_depth + self.ballast_bolt_len_mm

    @property
    def y_rim_near(self) -> float:
        return -self.pfw_rim_setback_mm

    @property
    def y_far(self) -> float:
        return self.y_rim_near - self.rim_thickness

    @property
    def ballast_angles_deg(self) -> list:
        pitch = 360.0 / self.ballast_count
        offset = (90.0 + 0.5 * pitch) % pitch  # keeps +Z / -Z (set-screw access) between two bolts
        return [offset + i * pitch for i in range(self.ballast_count)]

    def validate(self) -> list:
        errs = []
        if self.rim_thickness - self.head_pocket_depth - self.nut_pocket_depth < 3.0:
            errs.append("less than 3 mm of PETG between head and nut pockets: use a longer bolt or fewer nuts")
        if self.rim_inner_r <= self.pfw_hub_radius_mm + 5.0:
            errs.append("rim and hub overlap: reduce bolt size or increase outer radius")
        arc = 2 * math.pi * self.ballast_circle_r / self.ballast_count
        if arc < 2 * self.pocket_corner_r + 1.5:
            errs.append(f"{self.ballast_count} bolts do not fit on the {self.ballast_circle_r:.1f} mm circle")
        if self.L_center_mm - self.pfw_outer_radius_mm < self.clamp_cheek_top_z_mm + 3.0:
            errs.append("flywheel rim comes within 3 mm of the clamp hub")
        return errs


def _radial_teardrop_z(r: float, y_c: float, z0: float, z1: float) -> Part.Shape:
    """Hole along Z (radial in the wheel) with its 45 deg roof toward +Y (up when printed far-face-down)."""
    s = r / math.sqrt(2.0)
    zmin, zmax = min(z0, z1), max(z0, z1)
    cyl = Part.makeCylinder(r, zmax - zmin, _v(0, y_c, zmin), _v(0, 0, 1))
    pts = [_v(0, y_c, zmin), _v(s, y_c + s, zmin), _v(0, y_c + r * math.sqrt(2.0), zmin), _v(-s, y_c + s, zmin)]
    pts.append(pts[0])
    roof = Part.Face(Part.makePolygon(pts)).extrude(_v(0, 0, zmax - zmin))
    return cyl.fuse(roof).removeSplitter()


def _hex_prism_axis_y(af: float, cx: float, cz: float, y0: float, y1: float, rot_deg: float = 0.0) -> Part.Shape:
    r = af / math.sqrt(3.0)
    pts = [(cx + r * math.cos(math.radians(rot_deg + 60 * k)), cz + r * math.sin(math.radians(rot_deg + 60 * k))) for k in range(6)]
    return g._extrude_xz_polygon(pts, y0, y1)


def build_printed_frame(p: PrintedRigParams) -> Part.Shape:
    return g.build_square_frame(p, hole_comp=p.print_hole_comp_mm)


def build_printed_flywheel(p: PrintedRigParams) -> Part.Shape:
    """PETG flywheel body (local frame, see PrintedRigParams). Print far face down, no supports."""
    R, r_hub, r_in = p.pfw_outer_radius_mm, p.pfw_hub_radius_mm, p.rim_inner_r
    y_near, y_far = p.y_rim_near, p.y_far

    rim = g._cylinder_y(R, y_far, y_near).cut(g._cylinder_y(r_in, y_far - 1, y_near + 1))
    edges = [e for e in rim.Edges if isinstance(e.Curve, Part.Circle) and abs(e.Curve.Radius - R) < 1e-6]
    rim = rim.makeChamfer(p.pfw_edge_chamfer_mm, edges)
    web = g._cylinder_y(r_in + 0.5, y_far, y_far + p.pfw_web_thickness_mm)
    hub = g._cylinder_y(r_hub, y_far, 0.0)
    body = rim.fuse(web).fuse(hub).removeSplitter()

    cuts = []
    # Printed 6 mm D-bore (flat on +Z, matches the motor shaft)
    r_b = 0.5 * (p.flywheel_bore_dia_mm + p.pfw_bore_comp_mm)
    flat_z = p.flywheel_dflat_mm - 0.5 * p.flywheel_bore_dia_mm + 0.5 * p.pfw_bore_comp_mm
    bore = g._cylinder_y(r_b, y_far - 1, 1.0).cut(g._box_bounds(-5, 5, y_far - 2, 2, flat_z, 5))
    cuts.append(bore)

    # M3 grub screw onto the flat: radial clearance hole (+Z) through hub and rim, captive nut slot
    y_ss = -p.pfw_setscrew_y_mm
    r_acc = 0.5 * p.flywheel_access_hole_dia_mm
    cuts.append(_radial_teardrop_z(r_acc, y_ss, 0.0, R + 1.0))
    cuts.append(_radial_teardrop_z(r_acc, y_ss, -(r_in - 0.5), -(R + 1.0)))  # balancing twin through the rim only
    nut_af = p.m3_nut_af_mm + p.nut_trap_clearance_mm
    nut_e = nut_af / math.cos(math.radians(30))
    z_nut0 = flat_z + 1.2
    cuts.append(g._box_bounds(-0.5 * nut_af, 0.5 * nut_af, y_ss - 0.5 * nut_e, 1.0, z_nut0, z_nut0 + 2.4 + 0.3))

    # Ballast: through holes, head pockets in the far face, nut pockets in the plate-side face
    d, s, k, m = p.ballast_dims
    r_hole = 0.5 * (d + p.print_hole_comp_mm + 0.1)
    for a in p.ballast_angles_deg:
        x, z = p.ballast_circle_r * math.cos(math.radians(a)), p.ballast_circle_r * math.sin(math.radians(a))
        cuts.append(g._cylinder_y(r_hole, y_far - 1, y_near + 1, (x, z)))
        cuts.append(_hex_prism_axis_y(p.pocket_af, x, z, y_far - 1, y_far + p.head_pocket_depth, a))
        cuts.append(_hex_prism_axis_y(p.pocket_af, x, z, y_near - p.nut_pocket_depth, y_near + 1, a))

    return body.cut(Part.makeCompound(cuts)).removeSplitter()


def build_ballast(p: PrintedRigParams) -> tuple[Part.Shape, Part.Shape]:
    """Ballast bolts + nuts in the flywheel's local frame.

    Returns (display_shape, mass_shape): mass_shape uses the reduced shank diameter of a
    threaded bolt so the steel mass is realistic.
    """
    d, s, k, m = p.ballast_dims
    y_head_floor = p.y_far + p.head_pocket_depth
    y_nut_floor = p.y_rim_near - p.nut_pocket_depth
    disp, mass = [], []
    for a in p.ballast_angles_deg:
        x, z = p.ballast_circle_r * math.cos(math.radians(a)), p.ballast_circle_r * math.sin(math.radians(a))
        head = _hex_prism_axis_y(s, x, z, y_head_floor - k, y_head_floor, a)
        for frac, bucket in ((1.0, disp), (p.ballast_shank_mass_factor, mass)):
            shank = g._cylinder_y(0.5 * d * frac, y_head_floor, y_head_floor + p.ballast_bolt_len_mm, (x, z))
            bucket.append(head.fuse(shank).removeSplitter())
        for i in range(p.ballast_nuts_per_bolt):
            y0 = y_nut_floor + i * m
            nut = _hex_prism_axis_y(s, x, z, y0, y0 + m, a).cut(g._cylinder_y(0.5 * d, y0 - 1, y0 + m + 1, (x, z)))
            disp.append(nut)
            mass.append(nut)
    return Part.makeCompound(disp), Part.makeCompound(mass)


PRINTED_COLORS = {
    "Printed_Frame_140mm": ((0.93, 0.93, 0.95), 0),
    "Printed_Flywheel_Ballast": ((0.95, 0.55, 0.10), 0),
}


def build_all_printed(cad_dir: str | Path | None = None) -> dict:
    p = PrintedRigParams()
    errs = p.validate()
    if errs:
        raise ValueError("PrintedRigParams: " + "; ".join(errs))

    cad_root = Path(cad_dir) if cad_dir else g.CAD_DIR_DEFAULT
    exports_dir = cad_root / "exports"
    exports_dir.mkdir(parents=True, exist_ok=True)

    for dname in list(App.listDocuments().keys()):
        if dname in set(PRINTED_COLORS) | {"BalanceCube_1D_Printed_Assembly"}:
            App.closeDocument(dname)

    # Shapes
    s_frame = g._as_solid(build_printed_frame(p))
    s_fw_local = g._as_solid(build_printed_flywheel(p))
    s_ballast_local, s_ballast_mass_local = build_ballast(p)
    y_hub = -(0.5 * p.plate_thickness_mm + p.pfw_hub_gap_mm)
    wheel_pl = App.Placement(_v(0.0, y_hub, p.L_center_mm), App.Rotation())
    s_fw = s_fw_local.copy(); s_fw.Placement = wheel_pl
    s_bal = s_ballast_local.copy(); s_bal.Placement = wheel_pl
    s_bal_mass = s_ballast_mass_local.copy(); s_bal_mass.Placement = wheel_pl

    s_clamp = g._as_solid(g.build_pivot_clamp_hub(p))
    s_base = g._as_solid(g.build_base_and_stops(p))
    pad_pls = [App.Placement(_v(sx * p.stop_x_pos_mm, 0.0, p.stop_top_z_mm - p.stop_pad_thickness_mm), App.Rotation())
               for sx in (-1.0, 1.0)]
    s_pad_local = g._as_solid(g.build_stop_pad(p))
    s_pads = []
    for pl in pad_pls:
        sp = s_pad_local.copy(); sp.Placement = pl; s_pads.append(sp)
    s_shaft = g._cylinder_y(0.5 * p.pivot_shaft_dia_mm, -0.5 * p.pivot_shaft_len_mm, 0.5 * p.pivot_shaft_len_mm)
    s_kp_f, s_ring_f = g.build_kp08_reference(p, -p.kp08_y_offset_mm)
    s_kp_r, s_ring_r = g.build_kp08_reference(p, +p.kp08_y_offset_mm)
    s_motor, s_encoder, s_fasteners = g.build_jgb37_motor_reference(p)
    s_imu, s_electronics = g.build_imu_and_electronics(p)

    # New part documents + exports
    validation, part_docs = {}, {}
    for name, shp in (("Printed_Frame_140mm", s_frame), ("Printed_Flywheel_Ballast", s_fw_local)):
        validation[name] = {"valid": bool(shp.isValid() and len(shp.Solids) == 1), "solids": len(shp.Solids),
                            "volume_mm3": round(shp.Volume, 2)}
        col, transp = PRINTED_COLORS[name]
        doc = App.newDocument(name)
        obj = g._add_part(doc, name, shp, col, transp)
        doc.recompute()
        doc.saveAs(str(cad_root / f"{name}.FCStd"))
        part_docs[name] = obj
        g._export_step([obj], exports_dir / f"{name}.step")
        MeshPart.meshFromShape(Shape=shp, LinearDeflection=0.02, AngularDeflection=0.1, Relative=False).write(
            str(exports_dir / f"{name}.stl"))

    # Shared printed parts (clamp hub, base, pads) come from the acrylic rig's documents
    shared = {}
    for name in ("Pivot_Clamp_Hub", "Base_And_Stops", "Stop_Pad_TPU"):
        path = cad_root / f"{name}.FCStd"
        if name not in App.listDocuments():
            if not path.exists():
                raise FileNotFoundError(f"{path} missing: run generate_cad_models.build_all() first")
            App.openDocument(str(path))
        shared[name] = App.getDocument(name).getObject(name)

    from assembly_utils import Member
    d, s, k, m = p.ballast_dims
    ballast_name = f"Ballast_{p.ballast_count}x_{p.ballast_size}x{p.ballast_bolt_len_mm:g}"
    doc, asm = g.build_rig_assembly(
        cad_root / "BalanceCube_1D_Printed_Assembly.FCStd", "BalanceCube_1D_Printed_Assembly", p,
        frame=part_docs["Printed_Frame_140mm"], flywheel=part_docs["Printed_Flywheel_Ballast"],
        clamp=shared["Pivot_Clamp_Hub"], base=shared["Base_And_Stops"], pad=shared["Stop_Pad_TPU"],
        wheel_pl=wheel_pl, pad_pls=pad_pls,
        purchased={
            "Pivot_Shaft_8mm": s_shaft, "PillowBlock_KP08_Front": s_kp_f, "PillowBlock_KP08_Rear": s_kp_r,
            "KP08_Insert_Rings": s_ring_f.fuse(s_ring_r), "Motor_JGB37_520": s_motor,
            "Motor_Encoder_Rear": s_encoder, "Fastener_Heads_M3": s_fasteners, "IMU_MPU6050": s_imu,
            "Electronics_Module_Offboard": s_electronics,
        },
        frame_name="Printed_Frame_140mm", flywheel_name="Printed_Flywheel_Ballast",
        extra_members=[Member(ballast_name, shape=s_ballast_local, placement=wheel_pl, color=(0.55, 0.57, 0.6))],
        extra_fixed_to_flywheel=[ballast_name],
    )
    g._export_step([asm], exports_dir / "BalanceCube_1D_Printed_Assembly.step")

    analysis = g.analyze_rig(
        p, plate=s_frame, plate_density=p.print_density_kgm3 * p.frame_fill_fraction,
        wheel_parts=[(s_fw, p.print_density_kgm3 * p.pfw_body_fill_fraction), (s_bal_mass, p.steel_density_kgm3)],
        wheel_axis_xz=(0.0, p.L_center_mm),
        wheel_envelope=(p.pfw_outer_radius_mm, y_hub + p.y_far, y_hub),
        wheel_bore_y=(y_hub + p.y_far, y_hub),
        setscrew_y=y_hub - p.pfw_setscrew_y_mm,
        clamp=s_clamp, shaft=s_shaft, motor=s_motor, encoder=s_encoder, imu=s_imu, fasteners=s_fasteners,
        base=s_base, pads=s_pads,
        others={"kp08_F": s_kp_f, "kp08_R": s_kp_r, "ring_F": s_ring_f, "ring_R": s_ring_r, "electronics": s_electronics},
    )
    # Wheel mass split
    m_body = s_fw_local.Volume * 1e-9 * p.print_density_kgm3 * p.pfw_body_fill_fraction
    m_steel = s_ballast_mass_local.Volume * 1e-9 * p.steel_density_kgm3
    bal_disp = s_ballast_local.copy(); bal_disp.Placement = wheel_pl
    report = {
        "variant": "all_printed",
        "params_mm": {
            "L_center_mm": round(p.L_center_mm, 3),
            "flywheel_outer_radius_mm": p.pfw_outer_radius_mm,
            "ballast": f"{p.ballast_count} x {p.ballast_size}x{p.ballast_bolt_len_mm:g} + {p.ballast_nuts_per_bolt} nut each",
            "ballast_circle_radius_mm": round(p.ballast_circle_r, 2),
            "flywheel_rim_thickness_mm": round(p.rim_thickness, 2),
            "flywheel_hub_y_mm": round(y_hub, 3),
            "flywheel_far_face_y_mm": round(y_hub + p.y_far, 3),
        },
        "part_validity": validation,
        "flywheel_mass_split_kg": {"petg_body": round(m_body, 5), "steel_ballast": round(m_steel, 5),
                                   "per_bolt_set_g": round(1000 * m_steel / p.ballast_count, 2)},
        "ballast_vs_body_interference_mm3": round(bal_disp.common(s_fw).Volume, 4),
        "mass_inertia_comparison": analysis["mass"],
        "interference_sweep": analysis["sweep"],
        "tightest_clearances_mm_at_deg": analysis["tightest"],
        "flywheel_relative_motion": analysis["flywheel_rel"],
        "stop_contact": analysis["stop_contact"],
    }
    (cad_root / "printed_verification_report.json").write_text(json.dumps(report, indent=2), encoding="utf-8")

    if App.GuiUp:
        import FreeCADGui as Gui
        Gui.setActiveDocument(doc.Name)
        v = Gui.ActiveDocument.ActiveView
        if v is not None:
            v.viewIsometric()
            v.fitAll()
    return report


if __name__ == "__main__":
    print(json.dumps(build_all_printed(), indent=2))
