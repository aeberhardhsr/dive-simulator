"""Create the building material from existing, registered SWISSIMAGE assets."""
import hashlib
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
manifest = json.loads((ROOT / 'divingsim/Content/Diving/Geodata/imagery.json').read_text(encoding='utf-8'))
images = manifest['images']
key = hashlib.sha256(Path(__file__).read_bytes() + json.dumps(manifest, sort_keys=True).encode()).hexdigest()
path = '/Game/Diving/Materials/M_Buildings'
material = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
if not material or u.EditorAssetLibrary.get_metadata_tag(material, 'DiveBuildingsMaterialSource') != key:
    if not material:
        material = u.AssetToolsHelpers.get_asset_tools().create_asset('M_Buildings', '/Game/Diving/Materials', u.Material, u.MaterialFactoryNew())
    lib = u.MaterialEditingLibrary
    lib.delete_all_material_expressions(material)
    material.set_editor_property('two_sided', False)
    material.set_editor_property('used_with_nanite', True)
    vertex = lib.create_material_expression(material, u.MaterialExpressionVertexColor)
    uv = lib.create_material_expression(material, u.MaterialExpressionTextureCoordinate)
    custom = lib.create_material_expression(material, u.MaterialExpressionCustom)
    custom.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
    inputs = []
    for name in ['UV', 'Wall', 'Roof'] + ['Photo' + str(i) for i in range(len(images))]:
        item = u.CustomInput()
        item.set_editor_property('input_name', name)
        inputs.append(item)
    custom.set_editor_property('inputs', inputs)
    assert lib.connect_material_expressions(uv, '', custom, 'UV')
    assert lib.connect_material_expressions(vertex, '', custom, 'Wall')
    assert lib.connect_material_expressions(vertex, 'A', custom, 'Roof')
    code = 'if(Roof<0.5) return Wall;\nfloat3 color=Texture2DSample(Photo0,Photo0Sampler,UV).rgb;\n'
    if images[0]['id'] != 'landscape':
        raise RuntimeError('Expected the registered landscape image first')
    for i, item in enumerate(images):
        texture = u.load_asset('/Game/Diving/Textures/' + item['asset'])
        if not texture or u.EditorAssetLibrary.get_metadata_tag(texture, 'DiveImagerySHA256') != item['sha256']:
            raise RuntimeError('Missing/stale registered imagery: ' + item['id'])
        node = lib.create_material_expression(material, u.MaterialExpressionTextureObjectParameter)
        node.set_editor_property('parameter_name', 'Photo' + str(i))
        node.set_editor_property('texture', texture)
        assert lib.connect_material_expressions(node, '', custom, 'Photo' + str(i))
        if i:
            w, h = item['east'] - item['west'], item['north'] - item['south']
            dx, dy = (item['west'] - 2723000) / 22000, (1228000 - item['north']) / 13000
            code += f'float2 p{i}=(UV-float2({dx:.12f},{dy:.12f}))*float2({22000/w:.12f},{13000/h:.12f});\n'
            code += f'if(all(p{i}>=0)&&all(p{i}<=1)) color=Texture2DSample(Photo{i},Photo{i}Sampler,p{i}).rgb;\n'
    custom.set_editor_property('code', code + 'return color;')
    lib.connect_material_property(custom, '', u.MaterialProperty.MP_BASE_COLOR)
    roughness = lib.create_material_expression(material, u.MaterialExpressionConstant)
    roughness.set_editor_property('r', .82)
    lib.connect_material_property(roughness, '', u.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(material)
    u.EditorAssetLibrary.set_metadata_tag(material, 'DiveBuildingsMaterialSource', key)
    u.EditorAssetLibrary.save_loaded_asset(material)
u.log('DIVE_BUILDINGS_MATERIAL_READY ' + key)
