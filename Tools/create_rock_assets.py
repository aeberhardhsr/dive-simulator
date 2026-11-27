"""Blender: prepare three bounded rock forms from pinned CC0 geometry.

Source files remain unchanged. Normalize to a one-metre exclusion radius so
runtime placement can size the forms without invalidating fish/entry clearance.
"""
import bpy
import bmesh
import hashlib
import json
import math
import sys
from pathlib import Path
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parents[1]
formations='--formations' in sys.argv
OUT = ROOT/('blender/exports/FormationRocks' if formations else 'blender/exports/Rocks')
OUT.mkdir(parents=True, exist_ok=True)
manifest = json.loads((ROOT/('Documentation/ROCK_FORMATION_ASSETS.json' if formations else 'Documentation/ROCK_ASSETS.json')).read_text(encoding='utf-8'))
report = dict(schema=1, units='metres', radius_metres=1, local_geology_claim=False, assets=[])
prepared = []

for index, source in enumerate(manifest['assets']):
    path = ROOT/source['file']
    if hashlib.sha256(path.read_bytes()).hexdigest() != source['sha256']:
        raise ValueError('Source digest changed')
    with bpy.data.libraries.load(str(path), link=False) as (available, loaded):
        name = source['id']+'_LOD0' if source['id']+'_LOD0' in available.objects else source['id']
        if name not in available.objects:
            raise ValueError('Missing source mesh '+name)
        loaded.objects = [name]
    obj = loaded.objects[0]
    bpy.context.collection.objects.link(obj)
    obj.data = obj.data.copy()
    obj.data.transform(obj.matrix_world)
    obj.parent = None
    obj.matrix_world = Matrix.Identity(4)
    obj.modifiers.clear()
    obj.data.materials.clear()
    obj.hide_set(False)
    obj.hide_viewport = obj.hide_render = False
    mesh = obj.data
    lo = Vector(tuple(min(v.co[i] for v in mesh.vertices) for i in range(3)))
    hi = Vector(tuple(max(v.co[i] for v in mesh.vertices) for i in range(3)))
    centre = (lo+hi)*.5
    radius = max((v.co-centre).length for v in mesh.vertices)
    for vertex in mesh.vertices:
        vertex.co = (vertex.co-centre)/radius
    bm = bmesh.new()
    bm.from_mesh(mesh)
    boundary = [edge for edge in bm.edges if edge.is_boundary]
    if boundary:
        raise ValueError('Standalone boulder must have a closed source surface: '+source['id'])
    bmesh.ops.recalc_face_normals(bm, faces=list(bm.faces))
    bmesh.ops.triangulate(bm, faces=list(bm.faces))
    nonmanifold = sum(not edge.is_manifold for edge in bm.edges)
    bm.to_mesh(mesh)
    bm.free()
    mesh.update()
    mesh.calc_loop_triangles()
    if nonmanifold or any(not math.isfinite(c) for v in mesh.vertices for c in v.co):
        raise ValueError('Invalid closed rock '+source['id'])
    if any(p.area<1e-14 for p in mesh.polygons):
        raise ValueError('Degenerate rock faces '+source['id'])
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    obj.name = source.get('mesh',f'SM_LakeRock{index+1}')
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    target = OUT/(obj.name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(target), use_selection=True, apply_unit_scale=True,
        bake_space_transform=False, axis_forward='-Y', axis_up='Z', mesh_smooth_type='FACE',
        add_leaf_bones=False, bake_anim=False, use_mesh_modifiers=True)
    report['assets'].append(dict(name=obj.name, source=source['id'], source_sha256=source['sha256'],
        source_bounds_metres=[list(lo),list(hi)], source_radius_metres=radius,
        triangles=len(mesh.loop_triangles), vertices=len(mesh.vertices), closed_source_boundary_edges=len(boundary),
        nonmanifold_edges=nonmanifold, maximum_radius=max(v.co.length for v in mesh.vertices),
        file=target.relative_to(ROOT).as_posix(), sha256=hashlib.sha256(target.read_bytes()).hexdigest()))
    prepared.append(obj)

for obj in list(bpy.data.objects):
    if obj not in prepared:
        bpy.data.objects.remove(obj, do_unlink=True)
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/('blender/FormationRocks.blend' if formations else 'blender/RockAssets.blend')))
(OUT/'Manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('ROCK_GEOMETRY_PASS', json.dumps(report))
