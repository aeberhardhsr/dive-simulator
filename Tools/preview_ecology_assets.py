"""Render the reproducible Blender assets for morphology inspection (no source changes)."""
import bpy
from pathlib import Path
from mathutils import Vector
import math
ROOT=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'blender/DivingAssets.blend'))
material=bpy.data.materials.new('PreviewVertexColour'); material.use_nodes=True
nodes=material.node_tree.nodes; bsdf=nodes.get('Principled BSDF')
vertex=nodes.new('ShaderNodeVertexColor'); vertex.layer_name='Color'
material.node_tree.links.new(vertex.outputs['Color'],bsdf.inputs['Base Color'])
bsdf.inputs['Roughness'].default_value=.46
def label(text,x,z):
    curve=bpy.data.curves.new(text,'FONT'); curve.body=text; curve.size=.075
    obj=bpy.data.objects.new(text,curve); bpy.context.collection.objects.link(obj)
    obj.location=(x,-.16,z); obj.rotation_euler=(math.pi/2,0,0)
fish=['Perch','Roach','Pike','Whitefish','Burbot']
plants=['Pondweed','Stonewort','Milfoil']
for obj in list(bpy.data.objects):
    if not obj.name.startswith('SM_'): continue
    name=obj.name[3:]
    obj.data.materials.clear(); obj.data.materials.append(material)
    if name in fish:
        obj.location=(-.72,0,2.15-fish.index(name)*.43)
        label(name,-1.15,obj.location.z-.23)
    elif name in plants:
        obj.location=(.64,0,1.72-plants.index(name)*.8); obj.scale=(.55,)*3
        label(name,.34,obj.location.z-.1)
    else: obj.hide_render=True
scene=bpy.context.scene
scene.world.color=(.20,.20,.20)
bpy.ops.object.camera_add(location=(0,-8,1.9)); camera=bpy.context.object
camera.rotation_euler=(Vector((0,0,1.15))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO'; camera.data.ortho_scale=2.8; scene.camera=camera
for pos,power,size in [((-2,-3,5),450,4),((3,-1,2),250,3),((1,2,4),350,3)]:
    bpy.ops.object.light_add(type='AREA',location=pos)
    lamp=bpy.context.object; lamp.data.energy=power; lamp.data.shape='DISK'; lamp.data.size=size
    lamp.rotation_euler=(Vector((0,0,1))-lamp.location).to_track_quat('-Z','Y').to_euler()
scene.render.engine='CYCLES'; scene.cycles.samples=32
scene.render.resolution_x=1600; scene.render.resolution_y=1600; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'
scene.render.filepath=str(ROOT/'artifacts/EcologyAssets.png')
bpy.ops.render.render(write_still=True)
