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

# Perch-shaped body with taper, dorsals, pectorals, caudal fork and painted bars.
verts, faces, colors = [], [], []
RINGS, SIDES = 25, 24
for r in range(RINGS):
    t = r/(RINGS-1)
    x = -.19+t*.36
    shape = max(.02, math.sin(math.pi*t))**.65
    for s in range(SIDES):
        angle = s/SIDES*2*math.pi
        y = math.cos(angle)*.044*shape
        z = math.sin(angle)*.078*shape
        verts.append((x,y,z))
        dorsal = (math.sin(angle)+1)/2
        stripe = .48 if math.cos((t-.1)*math.pi*13)>.65 and dorsal>.25 else 1
        base = ((.45-.29*dorsal)*stripe, (.51-.25*dorsal)*stripe, (.34-.21*dorsal)*stripe)
        colors.append((*base, max(0,(-x-.02)/.2)))
        if r<RINGS-1:
            a=r*SIDES+s; b=r*SIDES+(s+1)%SIDES
            faces.append((a,b,b+SIDES,a+SIDES))

def fin(points, color):
    start=len(verts); verts.extend(points); colors.extend([color]*len(points))
    for i in range(1,len(points)-1):
        faces.append((start,start+i,start+i+1))
        faces.append((start,start+i+1,start+i))

fin([(-.17,0,0),(-.27,0,.068),(-.25,0,0),(-.27,0,-.068)],(.42,.20,.06,1))
fin([(-.11,0,.06),(-.12,0,.10),(-.08,0,.135),(-.04,0,.12),(.02,0,.115),(.07,0,.06)],(.24,.28,.14,.25))
fin([(-.1,0,-.06),(-.10,0,-.10),(-.02,0,-.08),(.02,0,-.04)],(.62,.24,.055,.3))
for sign in [-1,1]:
    fin([(.06,sign*.03,-.01),(-.035,sign*.095,-.045),(.01,sign*.04,-.06)],(.53,.26,.055,.3))

fish=mesh_object('SM_Perch',verts,faces,colors)
# Eyes are merged into the same mesh and retain vertex colour.
parts=[fish]
for sign in [-1,1]:
    for radius, offset, color in [(.012,.0,(.62,.55,.25,0)),(.007,.007,(.008,.012,.01,0))]:
        bpy.ops.mesh.primitive_uv_sphere_add(segments=12, ring_count=8, radius=radius,
            location=(.115,sign*(.031+offset),.022))
        eye=bpy.context.object
        attr=eye.data.color_attributes.new(name='Color',type='FLOAT_COLOR',domain='POINT')
        for item in attr.data: item.color=color
        eye.data.color_attributes.active_color=attr
        parts.append(eye)
bpy.ops.object.select_all(action='DESELECT')
for obj in parts: obj.select_set(True)
bpy.context.view_layer.objects.active=fish
bpy.ops.object.join()
export(fish)

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
bpy.ops.wm.save_as_mainfile(filepath=str(ROOT/'blender'/'DivingAssets.blend'))
print('DIVE_ASSETS_OK', OUT)
