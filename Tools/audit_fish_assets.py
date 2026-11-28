"""Independent structural checks of the Blender output (not an anatomy approval)."""
from pathlib import Path
import json
import math
import hashlib
import bpy
ROOT=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'blender/FishAssets.blend'))
expected={'Perch':.30,'Roach':.24,'Pike':.75,'Whitefish':.38,'Burbot':.55};records=[]
for name,length in expected.items():
    obj=bpy.data.objects['SM_'+name];mesh=obj.data
    xs=[v.co.x for v in mesh.vertices];span=max(xs)-min(xs)
    assert abs(span-length)<length*.015,(name,span,length)
    assert 20000<sum(len(p.vertices)-2 for p in mesh.polygons)<30000
    assert mesh.uv_layers.active and mesh.color_attributes.active_color
    assert len(mesh.uv_layers)==3
    offsets=mesh.uv_layers['FinOffset'].data;kinds=mesh.uv_layers['FinKind'].data
    counts={k:0 for k in range(7)};roots=0;tips=0
    for i in range(len(mesh.loops)):
        kind=round(kinds[i].uv.x*8);assert 0<=kind<=6
        counts[kind]+=1
        if kind:
            distance=math.hypot(offsets[i].uv.x-.5,offsets[i].uv.y-.5)
            roots+=distance<1e-7;tips+=distance>.015
    assert all(counts[k]>0 for k in [0,1,3,4,5,6]) and roots>100 and tips>100
    for v in mesh.vertices:assert all(math.isfinite(c) for c in v.co)
    for p in mesh.polygons:assert p.area>1e-14,(name,p.index,'degenerate polygon')
    values=[tuple(x.uv) for x in mesh.uv_layers.active.data]
    assert all(0<=u<=1 and 0<=v<=1 for u,v in values)
    assert any(.76<v<.94 for u,v in values) and any(.95<v<1 for u,v in values)
    alpha=[x.color[3] for x in mesh.color_attributes.active_color.data]
    assert min(alpha)==0 and max(alpha)>.7
    source=ROOT/'blender/exports'/('SM_'+name+'.fbx')
    records.append(dict(name=name,length_m=span,vertices=len(mesh.vertices),triangles=sum(len(p.vertices)-2 for p in mesh.polygons),uv_channels=3,fin_types=counts,anchored_fin_loops=roots,moving_fin_loops=tips,fbx_sha256=hashlib.sha256(source.read_bytes()).hexdigest()))
report=dict(status='PASS',scope='Length, finite geometry, positive polygon areas, atlas UV islands and animation weights; not expert morphology validation.',meshes=records)
(ROOT/'Documentation/FISH_MESH_AUDIT.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('FISH_MESH_AUDIT_PASS species=5')
