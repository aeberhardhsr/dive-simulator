"""Inspect the actual exported fish geometry and original PBR atlases in Blender."""
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parent))
import math
import bpy
from mathutils import Vector
from fish_profiles import FISH
ROOT=Path(__file__).resolve().parents[1]
bpy.ops.wm.open_mainfile(filepath=str(ROOT/'blender/FishAssets.blend'))
for index,(name,cfg) in enumerate(FISH.items()):
    obj=bpy.data.objects['SM_'+name];obj.location=(0,0,2.4-index*.60);obj.scale=(1/cfg['length'],)*3
    mat=bpy.data.materials.new('FishPreview_'+name);mat.use_nodes=True;nodes=mat.node_tree.nodes;links=mat.node_tree.links
    bsdf=nodes.get('Principled BSDF');bsdf.inputs['Specular IOR Level'].default_value=.5
    maps={}
    for kind in ('Color','Normal','Surface'):
        node=nodes.new('ShaderNodeTexImage');node.image=bpy.data.images.load(str(ROOT/'Art/Fish'/f'{name}_{kind}.png'))
        if kind!='Color':node.image.colorspace_settings.name='Non-Color'
        maps[kind]=node
    vertex=nodes.new('ShaderNodeVertexColor');vertex.layer_name='Color'
    mul=nodes.new('ShaderNodeMixRGB');mul.blend_type='MULTIPLY';mul.inputs[0].default_value=1
    links.new(maps['Color'].outputs['Color'],mul.inputs[1]);links.new(vertex.outputs['Color'],mul.inputs[2]);links.new(mul.outputs[0],bsdf.inputs['Base Color'])
    surface=nodes.new('ShaderNodeSeparateColor');links.new(maps['Surface'].outputs['Color'],surface.inputs[0]);links.new(surface.outputs['Green'],bsdf.inputs['Roughness'])
    sep=nodes.new('ShaderNodeSeparateColor');links.new(maps['Normal'].outputs['Color'],sep.inputs[0])
    inv=nodes.new('ShaderNodeMath');inv.operation='SUBTRACT';inv.inputs[0].default_value=1;links.new(sep.outputs['Green'],inv.inputs[1])
    combine=nodes.new('ShaderNodeCombineColor');links.new(sep.outputs['Red'],combine.inputs['Red']);links.new(inv.outputs[0],combine.inputs['Green']);links.new(sep.outputs['Blue'],combine.inputs['Blue'])
    normal=nodes.new('ShaderNodeNormalMap');links.new(combine.outputs[0],normal.inputs[1]);links.new(normal.outputs[0],bsdf.inputs['Normal'])
    obj.data.materials.clear();obj.data.materials.append(mat)
    curve=bpy.data.curves.new(name,'FONT');curve.body=name;curve.size=.045
    label=bpy.data.objects.new('Label_'+name,curve);bpy.context.collection.objects.link(label)
    label.location=(-.50,-.12,obj.location.z-.235);label.rotation_euler=(math.pi/2,0,0)
scene=bpy.context.scene;scene.world.use_nodes=True
scene.world.node_tree.nodes.get('Background').inputs[0].default_value=(.09,.09,.09,1)
scene.world.node_tree.nodes.get('Background').inputs[1].default_value=.4
bpy.ops.object.camera_add(location=(0,-7,1.2));camera=bpy.context.object;camera.rotation_euler=(Vector((0,0,1.2))-camera.location).to_track_quat('-Z','Y').to_euler()
camera.data.type='ORTHO';camera.data.ortho_scale=3.0;scene.camera=camera
for pos,power,size in [((-1,-3,5),260,3),((3,-2,1),100,2),((0,2,3),180,2)]:
    bpy.ops.object.light_add(type='AREA',location=pos);lamp=bpy.context.object;lamp.data.energy=power;lamp.data.size=size
    lamp.rotation_euler=(Vector((0,0,1.2))-lamp.location).to_track_quat('-Z','Y').to_euler()
scene.render.engine='CYCLES';scene.cycles.samples=48
scene.render.resolution_x=1400;scene.render.resolution_y=1800;scene.render.resolution_percentage=100
scene.render.image_settings.file_format='PNG';scene.render.filepath=str(ROOT/'artifacts/FishAssets.png')
bpy.ops.render.render(write_still=True)
