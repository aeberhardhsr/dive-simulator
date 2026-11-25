"""Inspect original equipment geometry in neutral light, without changing the blend."""
import bpy
from pathlib import Path
from mathutils import Vector
ROOT=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'blender/DivingAssets.blend'))
mat=bpy.data.materials.new('EquipmentPreview'); mat.use_nodes=True
nodes=mat.node_tree.nodes; vertex=nodes.new('ShaderNodeVertexColor'); vertex.layer_name='Color'
mat.node_tree.links.new(vertex.outputs['Color'],nodes.get('Principled BSDF').inputs['Base Color'])
nodes.get('Principled BSDF').inputs['Roughness'].default_value=.45
for obj in bpy.data.objects:
    if obj.name in ['SM_LeftSleeve','SM_LeftGlove','SM_WristComputer']:
        obj.location.y=-.12
    elif obj.name in ['SM_RightSleeve','SM_RightGlove','SM_DiveTorch']:
        obj.location.y=.12
    else:
        obj.hide_render=True; continue
    obj.data.materials.clear(); obj.data.materials.append(mat)
scene=bpy.context.scene; scene.world.color=(.25,.25,.25)
bpy.ops.object.camera_add(location=(.6,-.8,.85)); camera=bpy.context.object
camera.rotation_euler=(Vector((-.05,0,0))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO'; camera.data.ortho_scale=.72; scene.camera=camera
for pos,power in [((0,-1,2),180),((1,1,.5),90)]:
    bpy.ops.object.light_add(type='AREA',location=pos); lamp=bpy.context.object
    lamp.data.energy=power; lamp.data.size=2
    lamp.rotation_euler=(-lamp.location).to_track_quat('-Z','Y').to_euler()
scene.render.engine='CYCLES'; scene.cycles.samples=32
scene.render.resolution_x=1400; scene.render.resolution_y=900; scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG'; scene.render.filepath=str(ROOT/'artifacts/EquipmentMeshes.png')
bpy.ops.render.render(write_still=True)
