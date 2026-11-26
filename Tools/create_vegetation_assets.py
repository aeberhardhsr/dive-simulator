"""Blender: export real-scale CC0 vegetation, preserved source LODs and wind weights."""
import bpy
import hashlib
import json
import sys
from pathlib import Path
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'blender/exports/Vegetation'
OUT.mkdir(parents=True, exist_ok=True)
report = dict(schema=1, units='metres in Blender; FBX converts to Unreal centimetres', assets=[], materials={})
far_only = '--far-only' in sys.argv
if far_only:
    report = json.loads((OUT/'Manifest.json').read_text())
    report['assets'] = [a for a in report['assets'] if a['name']=='SM_Beech' or a['lod']<2]


def texture_file(image):
    source = Path(bpy.path.abspath(image.filepath)).resolve()
    if not source.is_file():
        raise ValueError('Missing texture ' + str(source))
    return source.relative_to(ROOT).as_posix()


def remember_material(mat, beech=False):
    if mat.name in report['materials']:
        return
    item = dict(name=mat.name, foliage=('leaves' in mat.name or 'twig' in mat.name and not beech),
                color=[1,1,1,1], roughness=.75, maps={})
    images = [n.image for n in mat.node_tree.nodes if n.type=='TEX_IMAGE' and n.image]
    if beech:
        for image in images:
            kind='normal' if 'normal' in image.name else 'color'
            item['maps'][kind]=texture_file(image)
        if 'twig' in mat.name:
            item['color']=[.62,.55,.5,1]
        item['alpha_from_color']=item['foliage']
    else:
        # The fir trunk material contains detail-bark and scanned-trunk inputs.
        # Use the scanned trunk maps on the original UVs; retain detail-bark on branches.
        prefix=mat.name.replace('_branches','_branch').replace('_dead_branch','_bark')
        if mat.name=='tree_small_02_trunk':
            prefix='tree_small_02'
        if mat.name=='fir_tree_01_dead_branches':
            prefix='fir_tree_01_bark'
        for kind, fragment in [('color','_diff_'),('normal','_nor_gl_'),('roughness','_rough_'),('alpha','_alpha_')]:
            candidates=[im for im in images if im.filepath.replace('\\','/').split('/')[-1].startswith(prefix+fragment)]
            if candidates:
                item['maps'][kind]=texture_file(candidates[0])
    if not item['maps'].get('color') or not item['maps'].get('normal'):
        raise ValueError('Incomplete source material '+mat.name+' '+repr([(im.filepath) for im in images])+' '+repr(item))
    report['materials'][mat.name]=item


def export(name, source_name, objects, lod, beech=False, base=None, target_triangles=None):
    bpy.ops.object.select_all(action='DESELECT')
    clones=[]
    for obj in objects:
        clone=obj.copy();clone.data=obj.data.copy();clone.modifiers.clear()
        bpy.context.collection.objects.link(clone)
        clone.hide_set(False);clone.hide_viewport=False;clone.hide_render=False
        # Bake complete parent/node transforms, including glTF quantization.
        clone.data.transform(obj.matrix_world)
        clone.parent=None;clone.matrix_world=Matrix.Identity(4)
        clone.select_set(True);clones.append(clone)
    bpy.context.view_layer.objects.active=clones[0]
    if len(clones)>1: bpy.ops.object.join()
    obj=bpy.context.object;obj.name=f'{name}_LOD{lod}'
    mesh=obj.data
    lowest=min(v.co.z for v in mesh.vertices)
    if base is None:
        trunk_indices=set()
        for p in mesh.polygons:
            mat=mesh.materials[p.material_index]
            if mat and ('trunk' in mat.name or 'bark' in mat.name): trunk_indices.update(p.vertices)
        bottom=[mesh.vertices[i].co for i in trunk_indices if mesh.vertices[i].co.z<lowest+.2]
        if not bottom: bottom=[v.co for v in mesh.vertices if v.co.z<lowest+.2]
        base=Vector((sum(v.x for v in bottom)/len(bottom),sum(v.y for v in bottom)/len(bottom),lowest))
    for v in mesh.vertices: v.co-=base
    if target_triangles:
        mesh.calc_loop_triangles()
        reduction=obj.modifiers.new('FarGeometry','DECIMATE')
        reduction.decimate_type='COLLAPSE'
        reduction.ratio=min(1.,target_triangles/len(mesh.loop_triangles))
        reduction.use_collapse_triangulate=True
        bpy.ops.object.modifier_apply(modifier=reduction.name)
        mesh=obj.data
    height=max(v.co.z for v in mesh.vertices)
    if not 3<height<40: raise ValueError('Unexpected physical tree height')
    for mat in mesh.materials:
        if not mat: raise ValueError('Empty material slot')
        remember_material(mat,beech)
    if mesh.color_attributes.get('Wind'): mesh.color_attributes.remove(mesh.color_attributes['Wind'])
    color=mesh.color_attributes.new(name='Wind',type='FLOAT_COLOR',domain='CORNER')
    for polygon in mesh.polygons:
        leaf=report['materials'][mesh.materials[polygon.material_index].name]['foliage']
        for loop in polygon.loop_indices:
            v=mesh.vertices[mesh.loops[loop].vertex_index].co
            t=max(0,min(1,v.z/height))
            color.data[loop].color=(t*t,float(leaf),t,1)
    mesh.color_attributes.active_color=color
    target=OUT/(obj.name+'.fbx')
    bpy.ops.export_scene.fbx(filepath=str(target),use_selection=True,apply_unit_scale=True,
        bake_space_transform=False,axis_forward='-Y',axis_up='Z',mesh_smooth_type='FACE',
        add_leaf_bones=False,bake_anim=False,use_mesh_modifiers=True,colors_type='LINEAR',path_mode='ABSOLUTE')
    mesh.calc_loop_triangles()
    bounds=[[min(v.co[i] for v in mesh.vertices) for i in range(3)], [max(v.co[i] for v in mesh.vertices) for i in range(3)]]
    item=dict(name=name,source=source_name,lod=lod,file=target.relative_to(ROOT).as_posix(),
              triangles=len(mesh.loop_triangles),vertices=len(mesh.vertices),height_m=height,bounds_m=bounds,
              source_base_m=list(base),materials=[m.name for m in mesh.materials],sha256=hashlib.sha256(target.read_bytes()).hexdigest())
    if target_triangles: item['authored_reduction_target_triangles']=target_triangles
    report['assets'].append(item)
    bpy.data.objects.remove(obj,do_unlink=True)
    print('TREE_EXPORT_PASS',name,lod,item['triangles'],height,flush=True)
    return base


for source, choices in [('tree_small_02',[('SM_LeafTree','tree_small_02',[0,1])]),
                        ('fir_tree_01',[(f'SM_Fir{v.upper()}',f'fir_tree_01_{v}',[1,2]) for v in 'abc'])]:
    bpy.ops.wm.open_mainfile(filepath=str(ROOT/'Art/Vegetation'/source/(source+'_2k.blend')),use_scripts=False)
    for name, prefix, levels in choices:
        if far_only:
            base=Vector(next(a['source_base_m'] for a in report['assets'] if a['name']==name and a['lod']==0))
        else:
            base=None
            for lod, original_lod in enumerate(levels):
                base=export(name,source,[bpy.data.objects[f'{prefix}_LOD{original_lod}']],lod,base=base)
        for lod, target in enumerate((20000,4000) if name=='SM_LeafTree' else (12000,3000),start=2):
            export(name,source,[bpy.data.objects[f'{prefix}_LOD{levels[-1]}']],lod,base=base,target_triangles=target)

base=None
for lod in range(0 if far_only else 3):
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.ops.import_scene.gltf(filepath=str(ROOT/f'Art/Vegetation/european_beech/lod{lod}.gltf'))
    base=export('SM_Beech','european_beech',[o for o in bpy.context.scene.objects if o.type=='MESH'],lod,beech=True,base=base)

(OUT/'Manifest.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print('VEGETATION_EXPORT_COMPLETE',len(report['assets']),flush=True)
