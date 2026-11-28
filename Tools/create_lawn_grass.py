"""Blender: deterministic curved geometry for a short, mixed lawn tuft."""
import bpy
import hashlib
import json
import math
import random
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'blender/exports/Lawn'
OUT.mkdir(parents=True,exist_ok=True)
manifest=[]
for lod,count,segments in [(0,192,3),(1,64,2),(2,20,1)]:
    bpy.ops.wm.read_factory_settings(use_empty=True)
    rng=random.Random(281129)
    vertices=[];faces=[];colours=[]
    for blade in range(count):
        x,y=rng.uniform(-.24,.24),rng.uniform(-.24,.24)
        height=rng.uniform(.04,.10);width=rng.uniform(.003,.005)
        yaw=rng.uniform(0,math.tau);bend=rng.uniform(.30,.70)*height
        shade=rng.uniform(.1,.9)
        dx,dy=math.cos(yaw),math.sin(yaw)
        first=len(vertices)
        for k in range(segments):
            t=k/segments
            for side in (-1,1):
                vertices.append((x+dx*bend*t*t-dy*width*(1-t)*side,y+dy*bend*t*t+dx*width*(1-t)*side,height*t))
                colours.append((shade,t,height*t/.2,1))
        vertices.append((x+dx*bend,y+dy*bend,height));colours.append((shade,1,height/.2,1))
        for k in range(segments-1):
            a=first+2*k;faces.extend([(a,a+1,a+2),(a+1,a+3,a+2)])
        a=first+2*(segments-1);faces.append((a,a+1,a+2))
    mesh=bpy.data.meshes.new('Lawn');mesh.from_pydata(vertices,[],faces);mesh.update()
    attr=mesh.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
    for item,c in zip(attr.data,colours):item.color=c
    mesh.color_attributes.active_color=attr
    obj=bpy.data.objects.new('SM_LawnGrass',mesh);bpy.context.collection.objects.link(obj)
    obj.select_set(True);bpy.context.view_layer.objects.active=obj
    material=bpy.data.materials.new('LawnGrass');mesh.materials.append(material)
    path=OUT/f'SM_LawnGrass_LOD{lod}.fbx'
    bpy.ops.export_scene.fbx(filepath=str(path),use_selection=True,object_types={'MESH'},apply_unit_scale=True,
        axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',add_leaf_bones=False,bake_anim=False,colors_type='LINEAR')
    manifest.append(dict(file=path.relative_to(ROOT).as_posix(),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),lod=lod,blades=count,triangles=len(faces),max_height_m=max(v[2] for v in vertices)))
(OUT/'Manifest.json').write_text(json.dumps(dict(schema=1,units='metres',assets=manifest),indent=2)+'\n',encoding='utf-8')
print('LAWN_GRASS_EXPORT_PASS',flush=True)
