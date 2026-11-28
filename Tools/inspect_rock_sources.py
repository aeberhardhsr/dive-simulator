"""Blender-only inventory of source objects before choosing production geometry."""
import bpy
import json
from pathlib import Path
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
manifest = json.loads((ROOT/'Documentation/ROCK_ASSETS.json').read_text())
report = []
for source in manifest['assets']:
    bpy.ops.wm.open_mainfile(filepath=str(ROOT/source['file']), use_scripts=False)
    meshes = []
    for obj in bpy.data.objects:
        if obj.type != 'MESH':
            continue
        obj.data.calc_loop_triangles()
        points = [obj.matrix_world @ v.co for v in obj.data.vertices]
        meshes.append(dict(name=obj.name, vertices=len(points), triangles=len(obj.data.loop_triangles),
                           minimum=[min(p[i] for p in points) for i in range(3)],
                           maximum=[max(p[i] for p in points) for i in range(3)],
                           modifiers=[dict(name=m.name, type=m.type) for m in obj.modifiers],
                           hidden=obj.hide_render))
    report.append(dict(id=source['id'], meshes=meshes))
(ROOT/'artifacts/RockSourceInventory.json').write_text(json.dumps(report, indent=2)+'\n')
print('ROCK_SOURCE_INVENTORY', json.dumps(report))
