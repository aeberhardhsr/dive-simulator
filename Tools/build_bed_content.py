"""Import pinned CC0 maps and build world-aligned lakebed/rock PBR materials."""
import hashlib
import json
from pathlib import Path
import sys
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'Tools'))
BASE = '/Game/Diving'


def build_bed(material_names=None):
    manifest = json.loads((ROOT / 'Documentation/BED_MATERIALS.json').read_text(encoding='utf-8'))
    tools = u.AssetToolsHelpers.get_asset_tools()
    lib = u.MaterialEditingLibrary
    textures = {}
    prefixes = {'gravelly_sand': 'Sand', 'river_small_rocks': 'Gravel', 'rock_boulder_dry': 'Rock'}
    if {asset['id'] for asset in manifest['assets']} != set(prefixes):
        raise RuntimeError('Unexpected bed material source set')
    for asset in manifest['assets']:
        prefix = prefixes[asset['id']]
        for kind, suffix in [('Diffuse', 'Color'), ('nor_dx', 'Normal'), ('arm', 'ARM')]:
            item = asset['maps'][kind]
            name = 'T_Bed_' + prefix + suffix
            path = BASE + '/Textures/' + name
            source = ROOT / item['file']
            if not source.exists() or hashlib.sha256(source.read_bytes()).hexdigest() != item['sha256']:
                raise RuntimeError('Missing/changed source; run prepare_bed_materials.py: ' + str(source))
            texture = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
            if not texture or u.EditorAssetLibrary.get_metadata_tag(texture, 'DiveBedSHA256') != item['sha256']:
                task = u.AssetImportTask()
                for key, value in [('filename', str(source)), ('destination_path', BASE + '/Textures'),
                                   ('destination_name', name), ('automated', True), ('replace_existing', True), ('save', False)]:
                    task.set_editor_property(key, value)
                tools.import_asset_tasks([task])
                texture = u.load_asset(path)
                if not texture: raise RuntimeError('Failed to import ' + name)
                u.EditorAssetLibrary.set_metadata_tag(texture, 'DiveBedSHA256', item['sha256'])
            texture.set_editor_property('srgb', kind == 'Diffuse')
            texture.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_NORMALMAP if kind == 'nor_dx'
                                        else u.TextureCompressionSettings.TC_MASKS if kind == 'arm' else u.TextureCompressionSettings.TC_DEFAULT)
            texture.set_editor_property('flip_green_channel', False)
            texture.set_editor_property('address_x', u.TextureAddress.TA_WRAP)
            texture.set_editor_property('address_y', u.TextureAddress.TA_WRAP)
            texture.set_editor_property('lod_bias', 0)
            texture.set_editor_property('max_texture_size', 2048)
            # Procedural bathymetry lacks imported UV density. Nine bounded 2K maps
            # retain their mip chains but avoid an erroneous permanently blurry mip.
            texture.set_editor_property('never_stream', True)
            if not u.EditorAssetLibrary.save_loaded_asset(texture): raise RuntimeError('Cannot save ' + name)
            textures[prefix + suffix] = texture

    code = (ROOT / 'Tools/Shaders/Lakebed.ush').read_text(encoding='utf-8')
    from build_ground_content import import_ground_textures, wire_ground, ground_code
    ground_textures = import_ground_textures() if material_names is None or 'M_ShorePhoto' in material_names else {}
    for name, is_rock in [('M_Lakebed', 0), ('M_RockSurface', 1), ('M_ShorePhoto', 0)]:
        if material_names is not None and name not in material_names:
            continue
        path = BASE + '/Materials/' + name
        material = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset(
            name, BASE + '/Materials', u.Material, u.MaterialFactoryNew())
        for expression in list(lib.get_material_expressions(material)):
            lib.delete_material_expression(material, expression)
        if lib.get_num_material_expressions(material): raise RuntimeError('Cannot clear ' + name)
        material.set_editor_property('two_sided', True)
        material.set_editor_property('used_with_instanced_static_meshes', True)
        material.set_editor_property('used_with_nanite', True)
        material.set_editor_property('tangent_space_normal', False)
        material.set_editor_property('shading_model', u.MaterialShadingModel.MSM_DEFAULT_LIT)
        material.set_editor_property('blend_mode', u.BlendMode.BLEND_OPAQUE)
        custom = lib.create_material_expression(material, u.MaterialExpressionCustom, 300, 0)
        custom.set_editor_property('description', 'World-aligned scanned sediment, gravel and rock')
        shader = code
        if name == 'M_ShorePhoto':
            shader = shader.replace('// BED_FINAL_OUTPUT', '''
float2 farUV=float2((P.y+300000)/2200000,(900000-P.x)/1300000);
float3 GroundPhoto=lerp(Texture2DSample(FarPhoto,FarPhotoSampler,farUV).rgb,Photo,saturate(PatchBlend));
''' + ground_code())
        custom.set_editor_property('code', shader)
        custom.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
        names = ['P', 'N', 'T', 'IsRock'] + list(textures)
        if name == 'M_ShorePhoto': names.extend(['Photo', 'FarPhoto', 'PatchBlend', 'Camera', 'DetailStrength'] + list(ground_textures))
        inputs = []
        for input_name in names:
            item = u.CustomInput(); item.set_editor_property('input_name', input_name); inputs.append(item)
        custom.set_editor_property('inputs', inputs)
        outputs = []
        for output_name in ['SurfaceNormal', 'Roughness', 'Occlusion']:
            item = u.CustomOutput(); item.set_editor_property('output_name', output_name)
            item.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3 if output_name == 'SurfaceNormal'
                                     else u.CustomMaterialOutputType.CMOT_FLOAT1)
            outputs.append(item)
        custom.set_editor_property('additional_outputs', outputs)
        for input_name, cls in [('P', u.MaterialExpressionWorldPosition), ('N', u.MaterialExpressionVertexNormalWS),
                                ('T', u.MaterialExpressionTime), ('IsRock', u.MaterialExpressionConstant)]:
            node = lib.create_material_expression(material, cls)
            if input_name == 'IsRock': node.set_editor_property('r', is_rock)
            if not lib.connect_material_expressions(node, '', custom, input_name): raise RuntimeError(input_name)
        for input_name, texture in textures.items():
            node = lib.create_material_expression(material, u.MaterialExpressionTextureObject)
            node.set_editor_property('texture', texture)
            node.set_editor_property('sampler_type', u.MaterialSamplerType.SAMPLERTYPE_NORMAL if input_name.endswith('Normal')
                                     else u.MaterialSamplerType.SAMPLERTYPE_MASKS if input_name.endswith('ARM')
                                     else u.MaterialSamplerType.SAMPLERTYPE_COLOR)
            if not lib.connect_material_expressions(node, '', custom, input_name): raise RuntimeError(input_name)
        if name == 'M_ShorePhoto':
            wire_ground(material, custom, ground_textures)
            photo = lib.create_material_expression(material, u.MaterialExpressionTextureSampleParameter2D)
            photo.set_editor_property('parameter_name', 'Ortho')
            photo.set_editor_property('texture', u.load_asset(BASE + '/Textures/T_Ortho_weesen_strandbad'))
            if not lib.connect_material_expressions(photo, 'RGB', custom, 'Photo'): raise RuntimeError('Photo')
            far = lib.create_material_expression(material, u.MaterialExpressionTextureObject)
            far.set_editor_property('texture', u.load_asset(BASE + '/Textures/T_Ortho_landscape'))
            if not lib.connect_material_expressions(far, '', custom, 'FarPhoto'): raise RuntimeError('FarPhoto')
            blend = lib.create_material_expression(material, u.MaterialExpressionVertexColor)
            if not lib.connect_material_expressions(blend, 'A', custom, 'PatchBlend'): raise RuntimeError('PatchBlend')
        for pin, prop in [('', u.MaterialProperty.MP_BASE_COLOR), ('SurfaceNormal', u.MaterialProperty.MP_NORMAL),
                          ('Roughness', u.MaterialProperty.MP_ROUGHNESS), ('Occlusion', u.MaterialProperty.MP_AMBIENT_OCCLUSION)]:
            if not lib.connect_material_property(custom, pin, prop): raise RuntimeError('Cannot connect ' + pin)
        lib.recompile_material(material)
        u.EditorAssetLibrary.set_metadata_tag(material, 'DiveBedShaderSHA256', hashlib.sha256(shader.encode()).hexdigest())
        if not u.EditorAssetLibrary.save_loaded_asset(material): raise RuntimeError('Cannot save ' + name)
        u.log('DIVE_BED_MATERIAL ' + name)
    u.log('DIVE_BED_CONTENT_OK')


if __name__ == '__main__':
    build_bed()
