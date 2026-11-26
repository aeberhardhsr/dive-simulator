"""Blender: neutral geometry review; not a game or underwater render."""
import bpy
import sys
from pathlib import Path
from mathutils import Vector

ROOT=Path(__file__).resolve().parents[1]
gravel='--gravel' in sys.argv
bpy.ops.wm.open_mainfile(filepath=str(ROOT/('blender/GravelAssets.blend' if gravel else 'blender/RockAssets.blend')),use_scripts=False)
material=bpy.data.materials.new('Neutral geometry review')
material.use_nodes=True
material.node_tree.nodes.get('Principled BSDF').inputs['Base Color'].default_value=(.22,.24,.25,1)
material.node_tree.nodes.get('Principled BSDF').inputs['Roughness'].default_value=.8
rocks=sorted([o for o in bpy.data.objects if o.type=='MESH'],key=lambda o:o.name)
for index,obj in enumerate(rocks):
    obj.location.x=(index-1)*2.5
    obj.location.z=-min(v.co.z for v in obj.data.vertices)
    obj.data.materials.clear();obj.data.materials.append(material)
bpy.ops.mesh.primitive_plane_add(size=200,location=(0,0,-.01))
floor=bpy.context.object
floor.data.materials.append(material)
scene=bpy.context.scene
scene.world.color=(.2,.2,.2)
for location,power,size in [((-2,-4,6),1800,4),((3,2,5),1200,3)]:
    bpy.ops.object.light_add(type='AREA',location=location)
    light=bpy.context.object;light.data.energy=power;light.data.shape='DISK';light.data.size=size
    light.rotation_euler=(-light.location).to_track_quat('-Z','Y').to_euler()
bpy.ops.object.camera_add(location=(1,-10,5))
scene.camera=bpy.context.object
scene.camera.rotation_euler=(Vector((0,0,.5))-scene.camera.location).to_track_quat('-Z','Y').to_euler()
scene.camera.data.type='ORTHO';scene.camera.data.ortho_scale=8
scene.render.engine='CYCLES';scene.cycles.device='CPU';scene.cycles.samples=32
scene.render.resolution_x=1500;scene.render.resolution_y=800;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(ROOT/('artifacts/GravelGeometry.png' if gravel else 'artifacts/RockGeometry.png'))
bpy.ops.render.render(write_still=True)
