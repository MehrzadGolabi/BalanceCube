"""Build a real FreeCAD 1.x Assembly-workbench document (joints, solver, exploded view).

Shared by generate_cad_models.py (acrylic/brass rig) and generate_printed_models.py
(all-3D-printed rig).

Every joint uses detached joint coordinate systems (JCS) computed from a global frame,
so the joints do not depend on topological face/edge names that change when a
parameter changes.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path

import FreeCAD as App

# Rotation that maps a JCS's local Z axis onto global +Y (the pivot / spin axis)
ROT_Z_TO_Y = App.Rotation(App.Vector(1, 0, 0), -90)


@dataclass
class Member:
    name: str
    shape: object = None                 # Part.Shape for an embedded (purchased) part
    link_target: object = None           # document object for an App::Link (fabricated part)
    placement: App.Placement = field(default_factory=App.Placement)
    color: tuple = (0.7, 0.7, 0.7)
    transparency: int = 0


@dataclass
class JointSpec:
    kind: str                            # "Fixed" or "Revolute"
    name: str
    a: str                               # member name (reference 1)
    b: str                               # member name (reference 2)
    frame: App.Placement                 # global frame of the joint (Z = revolute axis)
    angle_limits_deg: tuple | None = None


@dataclass
class ExplodeMove:
    members: list
    offset: App.Vector


def _style(obj, color, transparency):
    if not App.GuiUp:
        return
    vo = obj.ViewObject
    vo.ShapeColor = color
    vo.Transparency = transparency
    vo.DisplayMode = "Flat Lines"


def build_assembly_document(path: Path, doc_name: str, ground: str, members: list[Member],
                            joints: list[JointSpec], explode: list[ExplodeMove]):
    import JointObject
    import UtilsAssembly

    doc = App.newDocument(doc_name)
    doc.saveAs(str(path))  # links to other files need a saved owner
    asm = doc.addObject("Assembly::AssemblyObject", "Assembly")
    jg = asm.newObject("Assembly::JointGroup", "Joints")

    objs = {}
    for m in members:
        if m.link_target is not None:
            o = asm.newObject("App::Link", m.name)
            o.LinkedObject = m.link_target
        else:
            o = asm.newObject("Part::Feature", m.name)
            o.Shape = m.shape
            _style(o, m.color, m.transparency)
        o.Placement = m.placement
        objs[m.name] = o

    gj = jg.newObject("App::FeaturePython", "GroundedJoint")
    JointObject.GroundedJoint(gj, objs[ground])
    if App.GuiUp:
        JointObject.ViewProviderGroundedJoint(gj.ViewObject)

    type_index = {t: i for i, t in enumerate(JointObject.JointTypes)}
    for js in joints:
        j = jg.newObject("App::FeaturePython", js.name)
        JointObject.Joint(j, type_index[js.kind])
        if App.GuiUp:
            JointObject.ViewProviderJoint(j.ViewObject)
        oa, ob = objs[js.a], objs[js.b]
        j.Detach1 = True
        j.Detach2 = True
        # FreeCAD 1.1: the reference root must be the part itself (an empty sub-element
        # means "the part's own placement"); rooting at the assembly is silently ignored
        # by the solver's connectivity graph.
        j.Reference1 = [oa, ["", ""]]
        j.Reference2 = [ob, ["", ""]]
        j.Placement1 = oa.Placement.inverse().multiply(js.frame)
        j.Placement2 = ob.Placement.inverse().multiply(js.frame)
        if js.angle_limits_deg and hasattr(j, "EnableAngleMin"):
            j.EnableAngleMin = True
            j.EnableAngleMax = True
            j.AngleMin = js.angle_limits_deg[0]
            j.AngleMax = js.angle_limits_deg[1]

    doc.recompute()
    solve_status = asm.solve()

    # Stored exploded view (toggle it from the "Exploded Views" group)
    if explode:
        import CommandCreateView as CCV
        vg = UtilsAssembly.getViewGroup(asm)
        view = vg.newObject("App::FeaturePython", "Exploded_View")
        CCV.ExplodedView(view)
        if App.GuiUp:
            CCV.ViewProviderExplodedView(view.ViewObject)
        steps = []
        for i, mv in enumerate(explode):
            st = asm.newObject("App::FeaturePython", f"Move{i + 1:02d}")
            CCV.ExplodedViewStep(st, 0)
            if App.GuiUp:
                CCV.ViewProviderExplodedViewStep(st.ViewObject)
            st.References = [asm, [objs[n].Name + "." for n in mv.members]]
            st.MovementTransform = App.Placement(mv.offset, App.Rotation())
            steps.append(st)
        view.Group = steps

    doc.recompute()
    doc.save()
    return doc, asm, objs, solve_status
