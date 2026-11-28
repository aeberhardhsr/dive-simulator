"""Unreal editor: PBR tree materials, native source LODs and Nanite foliage."""
from pathlib import Path
import hashlib
import json
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
BASE='/Game/Diving/Vegetation'
tools=u.AssetToolsHelpers.get_asset_tools()
lib=u.MaterialEditingLibrary
editor=u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)


def build_vegetation():
    flag='Interchange.FeatureFlags.Import.FBX'
    previous=u.SystemLibrary.get_console_variable_int_value(flag)
    u.SystemLibrary.execute_console_command(None,flag+' 0')
    try:
        build()
    finally:
        u.SystemLibrary.execute_console_command(None,flag+' '+str(previous))


def build():
    manifest=json.loads((ROOT/'blender/exports/Vegetation/Manifest.json').read_text())
    materials={};textures={}
    for source in manifest['materials'].values():
        loaded={}
        for kind, filename in source['maps'].items():
            key=(filename,kind)
            if key not in textures:
                path=ROOT/filename
                digest=hashlib.sha256(path.read_bytes()).hexdigest()
                name='T_'+path.stem.replace('-','_')+'_'+kind
                asset_path=BASE+'/Textures/'+name
                texture=u.load_asset(asset_path) if u.EditorAssetLibrary.does_asset_exist(asset_path) else None
                if texture is None or u.EditorAssetLibrary.get_metadata_tag(texture,'TreeSHA256')!=digest:
                    task=u.AssetImportTask()
                    for k,v in [('filename',str(path)),('destination_path',BASE+'/Textures'),('destination_name',name),
                                ('automated',True),('replace_existing',True),('save',False)]: task.set_editor_property(k,v)
                    tools.import_asset_tasks([task]);texture=u.load_asset(asset_path)
                    if not texture: raise RuntimeError('Texture import failed '+name)
                    u.EditorAssetLibrary.set_metadata_tag(texture,'TreeSHA256',digest)
                texture.set_editor_property('srgb',kind=='color')
                texture.set_editor_property('compression_settings',u.TextureCompressionSettings.TC_NORMALMAP if kind=='normal' else u.TextureCompressionSettings.TC_DEFAULT if kind=='color' else u.TextureCompressionSettings.TC_MASKS)
                texture.set_editor_property('flip_green_channel',kind=='normal')
                texture.set_editor_property('max_texture_size',2048)
                texture.set_editor_property('never_stream',False)
                if not u.EditorAssetLibrary.save_loaded_asset(texture): raise RuntimeError('Texture save failed')
                textures[key]=texture
            loaded[kind]=textures[key]
        name='M_'+source['name'];path=BASE+'/Materials/'+name
        mat=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset(name,BASE+'/Materials',u.Material,u.MaterialFactoryNew())
        for expr in list(lib.get_material_expressions(mat)): lib.delete_material_expression(mat,expr)
        leaf=source['foliage']
        mat.set_editor_property('two_sided',leaf)
        mat.set_editor_property('blend_mode',u.BlendMode.BLEND_MASKED if leaf else u.BlendMode.BLEND_OPAQUE)
        mat.set_editor_property('shading_model',u.MaterialShadingModel.MSM_TWO_SIDED_FOLIAGE if leaf else u.MaterialShadingModel.MSM_DEFAULT_LIT)
        mat.set_editor_property('used_with_instanced_static_meshes',True)
        mat.set_editor_property('used_with_nanite',True)
        mat.set_editor_property('opacity_mask_clip_value',.45)
        mat.set_editor_property('dithered_lod_transition',True)
        def expression(cls): return lib.create_material_expression(mat,cls)
        nodes={}
        for kind,texture in loaded.items():
            node=expression(u.MaterialExpressionTextureSample);node.set_editor_property('texture',texture)
            node.set_editor_property('sampler_type',u.MaterialSamplerType.SAMPLERTYPE_NORMAL if kind=='normal' else u.MaterialSamplerType.SAMPLERTYPE_COLOR if kind=='color' else u.MaterialSamplerType.SAMPLERTYPE_MASKS)
            nodes[kind]=node
        tint=expression(u.MaterialExpressionConstant3Vector);tint.set_editor_property('constant',u.LinearColor(*source['color']))
        color=expression(u.MaterialExpressionMultiply);lib.connect_material_expressions(nodes['color'],'RGB',color,'A');lib.connect_material_expressions(tint,'',color,'B')
        lib.connect_material_property(color,'',u.MaterialProperty.MP_BASE_COLOR)
        lib.connect_material_property(nodes['normal'],'RGB',u.MaterialProperty.MP_NORMAL)
        if 'roughness' in nodes: lib.connect_material_property(nodes['roughness'],'R',u.MaterialProperty.MP_ROUGHNESS)
        else:
            rough=expression(u.MaterialExpressionConstant);rough.set_editor_property('r',source['roughness']);lib.connect_material_property(rough,'',u.MaterialProperty.MP_ROUGHNESS)
        if leaf:
            alpha=nodes['alpha'] if 'alpha' in nodes else nodes['color']
            lib.connect_material_property(alpha,'R' if 'alpha' in nodes else 'A',u.MaterialProperty.MP_OPACITY_MASK)
            sub=expression(u.MaterialExpressionMultiply);sub.set_editor_property('const_b',.45)
            lib.connect_material_expressions(color,'',sub,'A');lib.connect_material_property(sub,'',u.MaterialProperty.MP_SUBSURFACE_COLOR)
        wind=expression(u.MaterialExpressionCustom)
        names=['P','Weights','Clock','Wind']
        inputs=[]
        for label in names:
            item=u.CustomInput();item.set_editor_property('input_name',label);inputs.append(item)
        wind.set_editor_property('inputs',inputs);wind.set_editor_property('output_type',u.CustomMaterialOutputType.CMOT_FLOAT3)
        wind.set_editor_property('code','float p=dot(P.xy,float2(.00013,.00019)); float sway=(sin(Clock*1.1+p)+.35*sin(Clock*2.17+p*1.7))*Weights.r; float flutter=sin(Clock*7.1+P.x*.13+P.y*.17)*Weights.g*Weights.b; return float3(Wind.xy*(sway*14+flutter*1.8),abs(sway)*Wind.z);')
        position=expression(u.MaterialExpressionWorldPosition)
        weights=expression(u.MaterialExpressionVertexColor)
        time=expression(u.MaterialExpressionScalarParameter);time.set_editor_property('parameter_name','TreeTime');time.set_editor_property('default_value',0)
        direction=expression(u.MaterialExpressionVectorParameter);direction.set_editor_property('parameter_name','TreeWind');direction.set_editor_property('default_value',u.LinearColor(0,0,0,0))
        for node,output,label in [(position,'','P'),(weights,'','Weights'),(time,'','Clock'),(direction,'','Wind')]: lib.connect_material_expressions(node,output,wind,label)
        lib.connect_material_property(wind,'',u.MaterialProperty.MP_WORLD_POSITION_OFFSET)
        lib.recompile_material(mat)
        if not u.EditorAssetLibrary.save_loaded_asset(mat): raise RuntimeError('Material save failed '+name)
        materials[source['name']]=mat

    results=[]
    for name in sorted({a['name'] for a in manifest['assets']}):
        levels=sorted([a for a in manifest['assets'] if a['name']==name],key=lambda a:a['lod'])
        for level in levels:
            if hashlib.sha256((ROOT/level['file']).read_bytes()).hexdigest()!=level['sha256']: raise RuntimeError('FBX source changed')
        task=u.AssetImportTask()
        for k,v in [('filename',str(ROOT/levels[0]['file'])),('destination_path',BASE+'/Meshes'),('destination_name',name),('automated',True),('replace_existing',True),('save',False)]: task.set_editor_property(k,v)
        options=u.FbxImportUI()
        for k,v in [('automated_import_should_detect_type',False),('import_mesh',True),('import_as_skeletal',False),('import_materials',False),('import_textures',False),('import_animations',False),('mesh_type_to_import',u.FBXImportType.FBXIT_STATIC_MESH)]: options.set_editor_property(k,v)
        data=options.get_editor_property('static_mesh_import_data')
        data.set_editor_property('combine_meshes',True);data.set_editor_property('auto_generate_collision',False)
        data.set_editor_property('vertex_color_import_option',u.VertexColorImportOption.REPLACE)
        data.set_editor_property('normal_import_method',u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS_AND_TANGENTS)
        task.set_editor_property('options',options);tools.import_asset_tasks([task])
        mesh=u.load_asset(BASE+'/Meshes/'+name)
        if not mesh: raise RuntimeError('Mesh import failed '+name)
        for level in levels[1:]:
            if editor.import_lod(mesh,level['lod'],str(ROOT/level['file']))!=level['lod']: raise RuntimeError('LOD import failed '+name)
        # Bind by imported slot name, including slots introduced by a source LOD.
        slots=mesh.get_editor_property('static_materials')
        for i,slot in enumerate(slots):
            material_name=str(slot.get_editor_property('imported_material_slot_name'))
            if material_name not in materials: raise RuntimeError('Unknown tree material '+material_name)
            mesh.set_material(i,materials[material_name])
        settings=editor.get_nanite_settings(mesh)
        settings.set_editor_property('enabled',True)
        settings.set_editor_property('shape_preservation',u.NaniteShapePreservation.PRESERVE_AREA)
        settings.set_editor_property('fallback_percent_triangles',1.0);settings.set_editor_property('fallback_relative_error',0.0)
        editor.set_nanite_settings(mesh,settings,True)
        screen_sizes=[1,.16,.05] if name=='SM_Beech' else [1,.5,.15,.05]
        if not editor.set_lod_screen_sizes(mesh,screen_sizes): raise RuntimeError('LOD screen sizes failed')
        if not u.EditorAssetLibrary.save_loaded_asset(mesh): raise RuntimeError('Tree mesh save failed')
        bounds=mesh.get_bounds()
        height=2*bounds.box_extent.z/100
        if abs(height-levels[0]['height_m'])>.03: raise RuntimeError('FBX metre/centimetre conversion mismatch')
        results.append(dict(name=name,path=mesh.get_path_name(),height_m=height,lods=editor.get_lod_count(mesh),materials=[str(s.get_editor_property('imported_material_slot_name')) for s in slots]))
        u.log('VEGETATION_ASSET_PASS '+name)
    destination=ROOT/'divingsim/Content/Diving/Geodata/vegetation_assets.json'
    destination.write_text(json.dumps(dict(schema=1,assets=results),indent=2)+'\n',encoding='utf-8')
    u.log('VEGETATION_CONTENT_PASS '+str(len(results)))


if __name__=='__main__':
    build_vegetation()
