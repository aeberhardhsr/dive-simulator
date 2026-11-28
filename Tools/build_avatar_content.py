"""Import the original articulated diver and configure PBR and distance LODs."""
from pathlib import Path
import json
import unreal as u
ROOT = Path(__file__).resolve().parents[1]
BASE = '/Game/Diving/Avatar'
tools = u.AssetToolsHelpers.get_asset_tools()
lib = u.MaterialEditingLibrary


def build_avatar(materials_only=False):
    # Keep rig and colour import on the FBX factory controlled by FbxImportUI.
    # Restore the editor's original route after this isolated import.
    flag = 'Interchange.FeatureFlags.Import.FBX'
    previous = u.SystemLibrary.get_console_variable_int_value(flag)
    u.SystemLibrary.execute_console_command(None, flag+' 0')
    try:
        _build_avatar(materials_only)
    finally:
        u.SystemLibrary.execute_console_command(None, flag+' '+str(previous))


def _build_avatar(materials_only=False):
    def material(name, roughness, metallic=0, suit=False, fin=False):
        path = BASE+'/'+name
        mat = u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset(name, BASE, u.Material, u.MaterialFactoryNew())
        for node in list(lib.get_material_expressions(mat)):
            lib.delete_material_expression(mat, node)
        mat.set_editor_property('used_with_skeletal_mesh', suit)
        mat.set_editor_property('two_sided', fin)
        color = lib.create_material_expression(mat, u.MaterialExpressionVertexColor)
        base_color = color
        if suit:
            # The FBX importer retains encoded RGB8 (black = 46,54,58).
            # VertexColor does not decode these authored sRGB reflectances.
            decode = lib.create_material_expression(mat, u.MaterialExpressionCustom)
            inp = u.CustomInput(); inp.set_editor_property('input_name', 'Encoded')
            decode.set_editor_property('inputs', [inp])
            decode.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
            decode.set_editor_property('code', 'float3 c=saturate(Encoded.rgb); return lerp(c/12.92,pow((c+.055)/1.055,2.4),step(.04045,c));')
            lib.connect_material_expressions(color, '', decode, 'Encoded')
            base_color = decode
        if not lib.connect_material_property(base_color, '', u.MaterialProperty.MP_BASE_COLOR):
            raise RuntimeError('Vertex colour connection failed '+name)
        for value, prop in [(roughness, u.MaterialProperty.MP_ROUGHNESS), (metallic, u.MaterialProperty.MP_METALLIC)]:
            node = lib.create_material_expression(mat, u.MaterialExpressionConstant)
            node.set_editor_property('r', value)
            lib.connect_material_property(node, '', prop)
        if suit:
            # Fine directional weave; no image or third-party texture dependency.
            tex = lib.create_material_expression(mat, u.MaterialExpressionTextureCoordinate)
            normal = lib.create_material_expression(mat, u.MaterialExpressionCustom)
            inp = u.CustomInput(); inp.set_editor_property('input_name', 'UV')
            normal.set_editor_property('inputs', [inp])
            normal.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
            normal.set_editor_property('code', 'float2 p=UV*1800; float2 a=saturate(1-fwidth(p)/3.14); return normalize(float3(.05*sin(p)*a,1));')
            lib.connect_material_expressions(tex, '', normal, 'UV')
            lib.connect_material_property(normal, '', u.MaterialProperty.MP_NORMAL)
        if fin:
            tex = lib.create_material_expression(mat, u.MaterialExpressionTextureCoordinate)
            bend = lib.create_material_expression(mat, u.MaterialExpressionVectorParameter)
            bend.set_editor_property('parameter_name', 'BladeBend')
            normal = lib.create_material_expression(mat, u.MaterialExpressionCustom)
            inputs = []
            for name in ['UV', 'Bend']:
                inp = u.CustomInput(); inp.set_editor_property('input_name', name); inputs.append(inp)
            normal.set_editor_property('inputs', inputs)
            normal.set_editor_property('output_type', u.CustomMaterialOutputType.CMOT_FLOAT3)
            normal.set_editor_property('code', 'float t=saturate((UV.x-.34)/.66); return Bend.rgb*t*t;')
            lib.connect_material_expressions(tex, '', normal, 'UV')
            lib.connect_material_expressions(bend, '', normal, 'Bend')
            lib.connect_material_property(normal, '', u.MaterialProperty.MP_WORLD_POSITION_OFFSET)
        lib.recompile_material(mat)
        if not u.EditorAssetLibrary.save_loaded_asset(mat):
            raise RuntimeError('Material save failed '+name)
        return mat
    suit = material('M_DiverSuit', .83, suit=True)
    if materials_only:
        u.log('DIVE_AVATAR_MATERIALS_OK suit_rgb=srgb_decoded meshes_unchanged=1')
        return
    rubber = material('M_DiverRubber', .67)
    metal = material('M_DiverCylinder', .35, .6)
    fin = material('M_DiverFin', .65, fin=True)
    manifest = json.loads((ROOT/'blender/exports/Avatar/Manifest.json').read_text(encoding='utf-8'))
    static_editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)
    skeletal_editor = u.get_editor_subsystem(u.SkeletalMeshEditorSubsystem)
    report = []
    for entry in manifest['assets']:
        name, skeletal = entry['name'], entry['skeletal']
        task = u.AssetImportTask()
        task.set_editor_property('filename', str(ROOT/'blender/exports/Avatar'/(name+'.fbx')))
        task.set_editor_property('destination_path', BASE)
        task.set_editor_property('destination_name', name)
        for key in ['automated', 'save', 'replace_existing', 'replace_existing_settings']:
            task.set_editor_property(key, True)
        options = u.FbxImportUI()
        options.set_editor_property('automated_import_should_detect_type', False)
        options.set_editor_property('import_mesh', True)
        options.set_editor_property('import_as_skeletal', skeletal)
        options.set_editor_property('import_materials', False)
        options.set_editor_property('import_textures', False)
        options.set_editor_property('import_animations', False)
        options.set_editor_property('mesh_type_to_import', u.FBXImportType.FBXIT_SKELETAL_MESH if skeletal else u.FBXImportType.FBXIT_STATIC_MESH)
        if skeletal:
            options.set_editor_property('create_physics_asset', False)
            data = options.get_editor_property('skeletal_mesh_import_data')
        else:
            data = options.get_editor_property('static_mesh_import_data')
            data.set_editor_property('combine_meshes', True)
            data.set_editor_property('auto_generate_collision', False)
        data.set_editor_property('vertex_color_import_option', u.VertexColorImportOption.REPLACE)
        task.set_editor_property('options', options)
        tools.import_asset_tasks([task])
        asset = u.load_asset(BASE+'/'+name)
        if not asset:
            raise RuntimeError('Missing avatar asset '+name)
        mat = suit if skeletal else metal if 'Cylinder' in name else fin if name == 'SM_DiveFin' else rubber
        if skeletal:
            if not skeletal_editor.regenerate_lod(asset, 3):
                raise RuntimeError('Skeletal LOD generation failed '+name)
            slots = asset.get_editor_property('materials')
            if not slots:
                slots = [u.SkeletalMaterial()]
            for index in range(len(slots)):
                slot = slots[index]
                slot.set_editor_property('material_interface', mat)
                slots[index] = slot
            asset.set_editor_property('materials', slots)
            count = skeletal_editor.get_lod_count(asset)
        else:
            asset.set_material(0, mat)
            reduction = u.StaticMeshReductionOptions()
            reduction.set_editor_property('auto_compute_lod_screen_size', False)
            settings = []
            for percentage, screen in [(1., 1.), (.5, .35), (.2, .12)]:
                setting = u.StaticMeshReductionSettings()
                setting.set_editor_property('percent_triangles', percentage)
                setting.set_editor_property('screen_size', screen)
                settings.append(setting)
            reduction.set_editor_property('reduction_settings', settings)
            count = static_editor.set_lods(asset, reduction)
        if count != 3:
            raise RuntimeError('Expected three LODs for '+name)
        if not u.EditorAssetLibrary.save_loaded_asset(asset):
            raise RuntimeError('Save failed '+name)
        report.append({'name': name, 'lods': count, 'class': asset.get_class().get_name()})
    (ROOT/'artifacts/avatar-import.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    u.log('DIVE_AVATAR_CONTENT_OK assets='+str(len(report)))


if __name__ == '__main__':
    build_avatar('-DiveAvatarMaterialsOnly' in u.SystemLibrary.get_command_line().split())
