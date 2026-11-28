"""Unreal: import small-stone meshes without changing the verified boulder assets."""
import hashlib
import json
from pathlib import Path
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
BASE = '/Game/Diving/Gravel'


def props(obj, **values):
    for name, value in values.items():
        obj.set_editor_property(name, value)
    return obj


def build_gravel_material():
    material_path = '/Game/Diving/Materials/M_GravelSurface'
    material = u.load_asset(material_path)
    if not material:
        material = u.EditorAssetLibrary.duplicate_asset('/Game/Diving/Materials/M_RockSurface', material_path)
    if not material:
        raise RuntimeError('Missing lake-rock source material')
    # Reuse the lakebed gravel PBR maps so exposed stones belong to the same
    # substrate. The boulder graph and its larger-scale projection stay intact.
    code = (ROOT/'Tools/Shaders/Lakebed.ush').read_text()
    needle = 'P,n,180,.65,rn,ra)'
    if code.count(needle) != 1:
        raise RuntimeError('Lakebed rock projection changed; review gravel scale')
    expressions = [e for e in u.MaterialEditingLibrary.get_material_expressions(material)
                   if isinstance(e, u.MaterialExpressionCustom)]
    if len(expressions) != 1:
        raise RuntimeError('Unexpected gravel material graph')
    gravel_code = code.replace(needle, 'P,n,95,.4,rn,ra)')
    gravel_code = gravel_code.replace(
        'RockColor,RockColorSampler,RockNormal,RockNormalSampler,RockARM,RockARMSampler',
        'GravelColor,GravelColorSampler,GravelNormal,GravelNormalSampler,GravelARM,GravelARMSampler')
    expressions[0].set_editor_property('code', gravel_code)
    u.EditorAssetLibrary.set_metadata_tag(material, 'DiveBedShaderSHA256', hashlib.sha256(gravel_code.encode()).hexdigest())
    props(material, used_with_nanite=True, used_with_instanced_static_meshes=True)
    u.MaterialEditingLibrary.recompile_material(material)
    if not u.EditorAssetLibrary.save_loaded_asset(material):
        raise RuntimeError('Gravel material save failed')
    return material


def build_gravel():
    manifest = json.loads((ROOT/'blender/exports/Gravel/Manifest.json').read_text())
    material = build_gravel_material()
    editor = u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)
    tools = u.AssetToolsHelpers.get_asset_tools()
    flag = 'Interchange.FeatureFlags.Import.FBX'
    previous = u.SystemLibrary.get_console_variable_int_value(flag)
    u.SystemLibrary.execute_console_command(None, flag+' 0')
    records = []
    try:
        for source in manifest['assets']:
            path = ROOT/source['file']
            if hashlib.sha256(path.read_bytes()).hexdigest() != source['sha256']:
                raise RuntimeError('Gravel source hash mismatch')
            options = props(u.FbxImportUI(), automated_import_should_detect_type=False, import_mesh=True,
                import_as_skeletal=False, import_materials=False, import_textures=False,
                mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH)
            props(options.get_editor_property('static_mesh_import_data'), combine_meshes=True,
                auto_generate_collision=False, normal_import_method=u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
            task = props(u.AssetImportTask(), filename=str(path), destination_path=BASE,
                destination_name=source['name'], automated=True, replace_existing=True,
                replace_existing_settings=True, save=False, options=options)
            tools.import_asset_tasks([task])
            mesh = u.load_asset(BASE+'/'+source['name'])
            if not mesh:
                raise RuntimeError('Missing imported gravel')
            mesh.set_material(0, material)
            reduction = props(u.StaticMeshReductionOptions(), auto_compute_lod_screen_size=False,
                reduction_settings=[props(u.StaticMeshReductionSettings(), percent_triangles=p, screen_size=s)
                                    for p,s in [(1.,1.),(.35,.08),(.1,.025),(.025,.008)]])
            if editor.set_lods(mesh, reduction) != 4:
                raise RuntimeError('Missing gravel LODs')
            nanite = editor.get_nanite_settings(mesh)
            props(nanite, enabled=True, fallback_target=u.NaniteFallbackTarget.PERCENT_TRIANGLES,
                fallback_percent_triangles=1., fallback_relative_error=0.)
            editor.set_nanite_settings(mesh, nanite, True)
            if not editor.set_convex_decomposition_collisions(mesh, 1, 16, 10000):
                raise RuntimeError('Gravel convex contact hull failed')
            triangles = [mesh.get_num_triangles(i) for i in range(4)]
            if triangles[0] != source['triangles'] or not all(a>b>0 for a,b in zip(triangles,triangles[1:])):
                raise RuntimeError('Unexpected built gravel detail')
            bounds = mesh.get_bounds()
            if max(abs(bounds.origin.x), abs(bounds.origin.y), abs(bounds.origin.z)) > .1 or max(bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z) > 100.1:
                raise RuntimeError('Gravel scale or centre mismatch')
            if editor.get_convex_collision_count(mesh) != 1:
                raise RuntimeError('Expected one bounded gravel contact hull')
            targets = [1., .08, .025, .008]
            if not editor.set_lod_screen_sizes(mesh, targets):
                raise RuntimeError('Gravel LOD thresholds were not applied')
            screens = list(editor.get_lod_screen_sizes(mesh))
            if any(abs(a-b)>1e-6 for a,b in zip(screens,targets)):
                raise RuntimeError('Gravel render-data thresholds differ')
            if not u.EditorAssetLibrary.save_loaded_asset(mesh):
                raise RuntimeError('Gravel mesh save failed')
            records.append(dict(name=source['name'], source_sha256=source['sha256'], triangles=triangles,
                collision_hulls=1, nanite=True, screen_sizes=screens))
    finally:
        u.SystemLibrary.execute_console_command(None, flag+' '+str(previous))
    (ROOT/'artifacts/GravelContentBuild.json').write_text(json.dumps(records, indent=2)+'\n')
    u.log('DIVE_GRAVEL_CONTENT_PASS variants=3 lods=4 collision=1')


if __name__ == '__main__':
    build_gravel()
