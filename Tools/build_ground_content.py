"""Shared georegistered above-water PBR detail for shore and landscape."""
import hashlib
import json
from pathlib import Path
import sys
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'Tools'))
BASE = '/Game/Diving'


def import_ground_textures():
    manifest = json.loads((ROOT / 'Documentation/GROUND_MATERIALS.json').read_text(encoding='utf-8'))
    mask = json.loads((ROOT / 'Documentation/GROUND_MASK.json').read_text(encoding='utf-8'))
    if mask['bounds'] != [2723000, 1215000, 2745000, 1228000] or mask['status'] != 'PASS':
        raise ValueError('Ground mask registration/audit mismatch')
    items = [('Woodland', mask)]
    prefixes = {'grass_ground': 'Grass', 'forest_ground_04': 'Forest'}
    for asset in manifest['assets']:
        for kind, suffix in [('Diffuse', 'Color'), ('nor_dx', 'Normal'), ('arm', 'ARM')]:
            items.append((prefixes[asset['id']] + suffix, asset['maps'][kind]))
    result = {}
    for name, item in items:
        source = ROOT / item['file']
        if hashlib.sha256(source.read_bytes()).hexdigest() != item['sha256']:
            raise ValueError('Ground source checksum mismatch: ' + str(source))
        asset_name = 'T_Ground_' + name
        path = BASE + '/Textures/' + asset_name
        texture = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
        if not texture or u.EditorAssetLibrary.get_metadata_tag(texture, 'DiveGroundSHA256') != item['sha256']:
            task = u.AssetImportTask()
            for key, value in [('filename', str(source)), ('destination_path', BASE + '/Textures'),
                               ('destination_name', asset_name), ('automated', True),
                               ('replace_existing', True), ('save', False)]:
                task.set_editor_property(key, value)
            u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            texture = u.load_asset(path)
            if not texture: raise RuntimeError('Ground import failed: ' + name)
            u.EditorAssetLibrary.set_metadata_tag(texture, 'DiveGroundSHA256', item['sha256'])
        texture.set_editor_property('srgb', name.endswith('Color'))
        texture.set_editor_property('compression_settings', u.TextureCompressionSettings.TC_NORMALMAP if name.endswith('Normal')
                                   else u.TextureCompressionSettings.TC_DEFAULT if name.endswith('Color')
                                   else u.TextureCompressionSettings.TC_MASKS)
        texture.set_editor_property('flip_green_channel', False)
        address = u.TextureAddress.TA_CLAMP if name == 'Woodland' else u.TextureAddress.TA_WRAP
        texture.set_editor_property('address_x', address)
        texture.set_editor_property('address_y', address)
        texture.set_editor_property('max_texture_size', 4096 if name == 'Woodland' else 2048)
        texture.set_editor_property('lod_bias', 0)
        texture.set_editor_property('never_stream', True)
        if not u.EditorAssetLibrary.save_loaded_asset(texture): raise RuntimeError('Ground save failed: ' + name)
        result[name] = texture
    return result


def wire_ground(material, custom, textures):
    lib = u.MaterialEditingLibrary
    strength = lib.create_material_expression(material, u.MaterialExpressionScalarParameter)
    strength.set_editor_property('parameter_name', 'GroundDetailStrength')
    strength.set_editor_property('default_value', 1.)
    if not lib.connect_material_expressions(strength, '', custom, 'DetailStrength'): raise RuntimeError('Ground strength')
    camera = lib.create_material_expression(material, u.MaterialExpressionCameraPositionWS)
    if not lib.connect_material_expressions(camera, '', custom, 'Camera'): raise RuntimeError('Ground camera')
    for name, texture in textures.items():
        node = lib.create_material_expression(material, u.MaterialExpressionTextureObjectParameter)
        node.set_editor_property('parameter_name', 'Ground' + name)
        node.set_editor_property('texture', texture)
        node.set_editor_property('sampler_type', u.MaterialSamplerType.SAMPLERTYPE_NORMAL if name.endswith('Normal')
                                 else u.MaterialSamplerType.SAMPLERTYPE_COLOR if name.endswith('Color')
                                 else u.MaterialSamplerType.SAMPLERTYPE_MASKS)
        if not lib.connect_material_expressions(node, '', custom, name): raise RuntimeError('Ground input ' + name)


def ground_code():
    return (ROOT / 'Tools/Shaders/GroundDetail.ush').read_text(encoding='utf-8')


def build_landscape_ground():
    # build_bed imports and verifies the existing rock maps used on steep faces.
    from build_bed_content import build_bed
    build_bed({'M_ShorePhoto'})
    textures = import_ground_textures()
    lib = u.MaterialEditingLibrary
    name = 'M_LandscapePhoto'
    path = BASE + '/Materials/' + name
    material = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else u.AssetToolsHelpers.get_asset_tools().create_asset(
        name, BASE + '/Materials', u.Material, u.MaterialFactoryNew())
    for node in list(lib.get_material_expressions(material)): lib.delete_material_expression(material, node)
    material.set_editor_property('two_sided', True)
    material.set_editor_property('used_with_nanite', True)
    material.set_editor_property('tangent_space_normal', False)
    libcode = (ROOT / 'Tools/Shaders/Lakebed.ush').read_text(encoding='utf-8').split('BedSurface bed;')[0]
    code = libcode + '''BedSurface bed;
float3 n=normalize(N);float3 result=Photo;float3 GroundPhoto=Photo;
SurfaceNormal=n;Roughness=.92;Occlusion=1;
''' + ground_code() + '\nreturn result;'
    custom = lib.create_material_expression(material, u.MaterialExpressionCustom)
    custom.set_editor_property('code', code)
    custom.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
    for suffix in ['Color', 'Normal', 'ARM']:
        textures['Rock' + suffix] = u.load_asset(BASE + '/Textures/T_Bed_Rock' + suffix)
    inputs = []
    for name in ['P', 'N', 'Photo', 'Camera', 'DetailStrength'] + list(textures):
        item = u.CustomInput(); item.set_editor_property('input_name', name); inputs.append(item)
    custom.set_editor_property('inputs', inputs)
    outputs = []
    for name in ['SurfaceNormal', 'Roughness', 'Occlusion']:
        item = u.CustomOutput(); item.set_editor_property('output_name', name)
        item.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3 if name == 'SurfaceNormal' else u.CustomMaterialOutputType.CMOT_FLOAT1)
        outputs.append(item)
    custom.set_editor_property('additional_outputs', outputs)
    for name, cls in [('P', u.MaterialExpressionWorldPosition), ('N', u.MaterialExpressionVertexNormalWS)]:
        node = lib.create_material_expression(material, cls)
        if not lib.connect_material_expressions(node, '', custom, name): raise RuntimeError(name)
    photo = lib.create_material_expression(material, u.MaterialExpressionTextureSampleParameter2D)
    photo.set_editor_property('parameter_name', 'Ortho')
    photo.set_editor_property('texture', u.load_asset(BASE + '/Textures/T_Ortho_landscape'))
    if not lib.connect_material_expressions(photo, 'RGB', custom, 'Photo'): raise RuntimeError('Photo')
    wire_ground(material, custom, textures)
    for pin, prop in [('', u.MaterialProperty.MP_BASE_COLOR), ('SurfaceNormal', u.MaterialProperty.MP_NORMAL),
                      ('Roughness', u.MaterialProperty.MP_ROUGHNESS), ('Occlusion', u.MaterialProperty.MP_AMBIENT_OCCLUSION)]:
        if not lib.connect_material_property(custom, pin, prop): raise RuntimeError(pin)
    lib.recompile_material(material)
    u.EditorAssetLibrary.set_metadata_tag(material, 'DiveGroundShaderSHA256', hashlib.sha256(code.encode()).hexdigest())
    if not u.EditorAssetLibrary.save_loaded_asset(material): raise RuntimeError('Landscape ground save')
    u.log('DIVE_GROUND_CONTENT_OK')


if __name__ == '__main__':
    build_landscape_ground()
