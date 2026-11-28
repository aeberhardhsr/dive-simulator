"""Blender: bounded low-cost gravel forms derived from the pinned rock geometry."""
import bpy
import bmesh
import hashlib
import json
import math
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT/'blender/exports/Gravel'
OUT.mkdir(parents=True, exist_ok=True)
source = ROOT/'blender/RockAssets.blend'
manifest = json.loads((ROOT/'blender/exports/Rocks/Manifest.json').read_text())
bpy.ops.wm.open_mainfile(filepath=str(source), use_scripts=False)
report = dict(schema=1, units='metres', radius_metres=1,
              source_blend_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),
              intended_use='Rescaled small stones in authored underwater gravel patches; not measured sediment geometry',
              assets=[])
for index, item in enumerate(manifest['assets']):
    obj = bpy.data.objects[item['name']]
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    obj.data = obj.data.copy()
    modifier = obj.modifiers.new('Small-stone silhouette budget', 'DECIMATE')
    modifier.ratio = 1200/item['triangles']
    modifier.use_collapse_triangulate = True
    bpy.ops.object.modifier_apply(modifier=modifier.name)
    bm = bmesh.new()
    bm.from_mesh(obj.data)
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bmesh.ops.triangulate(bm, faces=list(bm.faces))
    nonmanifold = sum(not edge.is_manifold for edge in bm.edges)
    bm.to_mesh(obj.data)
    bm.free()
    obj.data.update()
    triangles = len(obj.data.polygons)
    if nonmanifold or not 900 <= triangles <= 1250:
        raise ValueError('Unexpected gravel topology or triangle count')
    if any(not math.isfinite(c) for v in obj.data.vertices for c in v.co):
        raise ValueError('Non-finite gravel vertex')
    if any(face.area < 1e-14 for face in obj.data.polygons):
        raise ValueError('Degenerate gravel triangle')
    # Quadric simplification can move vertices slightly beyond the old hull.
    # Recenter and normalize the finished geometry before assigning its radius.
    lo = Vector(tuple(min(v.co[i] for v in obj.data.vertices) for i in range(3)))
    hi = Vector(tuple(max(v.co[i] for v in obj.data.vertices) for i in range(3)))
    centre = (lo+hi)*.5
    source_radius = max((v.co-centre).length for v in obj.data.vertices)
    for vertex in obj.data.vertices:
        vertex.co = (vertex.co-centre)/source_radius
    radius = max(v.co.length for v in obj.data.vertices)
    if radius > 1.00001:
        raise ValueError('Gravel radius normalization failed')
    for face in obj.data.polygons:
        face.use_smooth = True
    obj.name = f'SM_Gravel{index+1}'
    target = OUT/(obj.name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(target), use_selection=True, apply_unit_scale=True,
        bake_space_transform=False, axis_forward='-Y', axis_up='Z', mesh_smooth_type='FACE',
        add_leaf_bones=False, bake_anim=False, use_mesh_modifiers=True)
    report['assets'].append(dict(name=obj.name, source=item['source'],
        original_source_sha256=item['source_sha256'], triangles=triangles,
        nonmanifold_edges=nonmanifold, maximum_radius=radius,
        file=target.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(target.read_bytes()).hexdigest()))
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'blender/GravelAssets.blend'))
(OUT/'Manifest.json').write_text(json.dumps(report, indent=2)+'\n')
print('GRAVEL_GEOMETRY_PASS', json.dumps(report))
