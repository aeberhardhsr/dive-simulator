"""Reproducible original meshes; run with Blender --background --python this_file."""
import bpy
import math
import random
from pathlib import Path
from mathutils import Vector, noise

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'blender' / 'exports'
OUT.mkdir(parents=True, exist_ok=True)
random.seed(1948)
bpy.ops.object.select_all(action='SELECT')
bpy.ops.object.delete(use_global=False)

def mesh_object(name, vertices, faces, colors):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(vertices, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    color = mesh.color_attributes.new(name='Color', type='FLOAT_COLOR', domain='POINT')
    for i, value in enumerate(colors):
        color.data[i].color = value
    mesh.color_attributes.active_color = color
    for face in mesh.polygons:
        face.use_smooth = True
    return obj

def export(obj):
    bpy.ops.object.select_all(action='DESELECT')
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    bpy.ops.export_scene.fbx(filepath=str(OUT / (obj.name+'.fbx')), use_selection=True,
                            apply_unit_scale=True, bake_space_transform=False,
                            axis_forward='-Y', axis_up='Z', mesh_smooth_type='FACE',
                            add_leaf_bones=False, bake_anim=False, use_mesh_modifiers=True)

# Layered limestone, deliberately irregular silhouette rather than scaled spheres.
bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=4, radius=1)
rock = bpy.context.object
rock.name = 'SM_Rock'
for vertex in rock.data.vertices:
    p = vertex.co.copy()
    scale = 1 + .24*noise.noise_vector(p*1.7)[0] + .09*noise.noise_vector(p*5)[1]
    vertex.co = Vector((p.x*scale, p.y*scale*.8, p.z*scale*.68))
    vertex.co.z += .06*math.sin(vertex.co.x*8+vertex.co.y*5)
    if vertex.co.z < -.45:
        vertex.co.z = -.45 + (vertex.co.z+.45)*.3
for poly in rock.data.polygons:
    poly.use_smooth = True
export(rock)

# Species-specific original meshes are kept in a reusable Blender helper.
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
from create_ecology_assets import create_fish, create_fine_plants
create_fish(mesh_object,export)
create_fine_plants(mesh_object,export)

# Freshwater pondweed with curved stems and paired leaves.
verts,faces,colors=[],[],[]
for stalk in range(9):
    height=random.uniform(.35,1.15)
    base=Vector((random.uniform(-.22,.22),random.uniform(-.22,.22),0))
    bend=Vector((random.uniform(-.22,.22),random.uniform(-.22,.22),0))
    for segment in range(11):
        t=segment/10
        center=base+Vector((0,0,height*t))+bend*t*t
        width=.006*(1-t*.6)
        offset=len(verts)
        verts.extend([center+Vector((-width,0,0)),center+Vector((width,0,0))])
        colors.extend([(.08,.18,.025,t)]*2)
        if segment>0: faces.append((offset-2,offset-1,offset+1,offset))
        if segment in (2,4,6,8):
            angle=stalk*2.1+segment*1.7
            for sign in (-1,1):
                direction=Vector((math.cos(angle)*sign,math.sin(angle)*sign,.25))
                sideways=Vector((-direction.y,direction.x,0)).normalized()
                length=random.uniform(.09,.19)*(1-t*.4)
                a=len(verts)
                for part in range(7):
                    u=part/6
                    p=center+direction*length*u+Vector((0,0,.04*math.sin(u*math.pi)))
                    w=.024*math.sin(u*math.pi)
                    for side in (-1,1):
                        verts.append(p+sideways*w*side)
                        colors.append((.11+.04*u,.24+.08*u,.035,t))
                    if part>0:
                        k=a+part*2; faces.append((k-2,k-1,k+1,k))
plant=mesh_object('SM_Pondweed',verts,faces,colors)
export(plant)
from create_equipment_assets import create_equipment
create_equipment(mesh_object,export)
from create_torch_assets import create_torches
create_torches()
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'blender'/'DivingAssets.blend'))
print('DIVE_ASSETS_OK', OUT)
