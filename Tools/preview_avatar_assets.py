"""Render authored scuba assemblies without saving changes to their source scene."""
import math
from pathlib import Path
import bpy
from mathutils import Vector
ROOT = Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'blender/AvatarAssets.blend'))
material = bpy.data.materials.new('Preview PBR')
material.use_nodes = True
nodes = material.node_tree.nodes
vertex = nodes.new('ShaderNodeVertexColor')
vertex.layer_name = 'Color'
material.node_tree.links.new(vertex.outputs['Color'], nodes.get('Principled BSDF').inputs['Base Color'])
nodes.get('Principled BSDF').inputs['Roughness'].default_value = .55
names = ['SK_DiverSuit', 'SM_DiverHood', 'SM_DiveMask', 'SM_SecondStage', 'SM_BCDWing',
         'SM_WeightPockets', 'SM_RegulatorHose', 'SM_DiveCompass', 'SM_CylinderSteel12']
for obj in list(bpy.data.objects):
    obj.hide_render = obj.type == 'MESH' and obj.name not in names
    if obj.type == 'MESH':
        obj.data.materials.clear()
        obj.data.materials.append(material)
bpy.data.objects['SM_CylinderSteel12'].location = (-.24, 0, 1.24)
for name in ['SM_DiveBoot', 'SM_DiveFin']:
    source = bpy.data.objects[name]
    for sign in [-1, 1]:
        obj = source.copy()
        bpy.context.collection.objects.link(obj)
        obj.location = (0, sign*.11, .10)
        obj.hide_render = False
with bpy.data.libraries.load(str(ROOT/'blender/DivingAssets.blend'), link=False) as (source, target):
    target.objects = [n for n in source.objects if n in ['SM_LeftGlove', 'SM_RightGlove']]
for obj, sign in zip(target.objects, [-1, 1]):
    bpy.context.collection.objects.link(obj)
    obj.location = (0, sign*.30, .804)
    obj.rotation_euler.y = math.pi/2
    obj.data.materials.clear()
    obj.data.materials.append(material)
scene = bpy.context.scene
scene.world.color = (.25, .25, .25)
bpy.ops.object.camera_add(location=(3, -4, 2.5))
camera = bpy.context.object
scene.camera = camera
camera.data.type = 'ORTHO'
camera.data.ortho_scale = 2.3
for pos, energy, size in [((3, -3, 4), 600, 3), ((0, 3, 3), 450, 2), ((-3, -1, 2.5), 500, 2)]:
    bpy.ops.object.light_add(type='AREA', location=pos)
    lamp = bpy.context.object
    lamp.data.energy, lamp.data.size = energy, size
    lamp.rotation_euler = (Vector((0, 0, 1))-lamp.location).to_track_quat('-Z', 'Y').to_euler()
scene.render.engine = 'CYCLES'
scene.cycles.samples = 32
scene.render.resolution_x, scene.render.resolution_y = 1100, 1400
scene.render.resolution_percentage = 100
scene.render.image_settings.file_format = 'PNG'
for label, pos in [('Front', (3, -4, 2.5)), ('Back', (-3, 4, 2.3)), ('Side', (0, -4, 1.8))]:
    camera.location = pos
    camera.rotation_euler = (Vector((0, 0, .85))-camera.location).to_track_quat('-Z', 'Y').to_euler()
    scene.render.filepath = str(ROOT/'artifacts'/('Avatar_'+label+'.png'))
    bpy.ops.render.render(write_still=True)
