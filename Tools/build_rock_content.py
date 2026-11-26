"""Unreal: three Nanite rock forms, four raster LODs and convex contact hulls."""
import hashlib
import json
from pathlib import Path
import unreal as u

ROOT=Path(__file__).resolve().parents[1]
BASE='/Game/Diving/Rocks'


def props(obj, **values):
    for name,value in values.items(): obj.set_editor_property(name,value)
    return obj


def build_rocks():
    manifest=json.loads((ROOT/'blender/exports/Rocks/Manifest.json').read_text())
    if len(manifest['assets'])!=3: raise RuntimeError('Expected three bounded rock forms')
    material=u.load_asset('/Game/Diving/Materials/M_RockSurface')
    if not material: raise RuntimeError('Build the lakebed materials first')
    props(material,used_with_nanite=True,used_with_instanced_static_meshes=True)
    u.MaterialEditingLibrary.recompile_material(material)
    if not u.EditorAssetLibrary.save_loaded_asset(material): raise RuntimeError('Rock Nanite material save failed')
    editor=u.get_editor_subsystem(u.StaticMeshEditorSubsystem) or u.new_object(u.StaticMeshEditorSubsystem)
    tools=u.AssetToolsHelpers.get_asset_tools()
    flag='Interchange.FeatureFlags.Import.FBX'
    previous=u.SystemLibrary.get_console_variable_int_value(flag)
    u.SystemLibrary.execute_console_command(None,flag+' 0')
    records=[]
    try:
        for source in manifest['assets']:
            path=ROOT/source['file']
            if hashlib.sha256(path.read_bytes()).hexdigest()!=source['sha256']: raise RuntimeError('Rock FBX changed')
            name=source['name']
            options=props(u.FbxImportUI(),automated_import_should_detect_type=False,import_mesh=True,
                import_as_skeletal=False,import_materials=False,import_textures=False,
                mesh_type_to_import=u.FBXImportType.FBXIT_STATIC_MESH)
            props(options.get_editor_property('static_mesh_import_data'),combine_meshes=True,auto_generate_collision=False,
                normal_import_method=u.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
            task=props(u.AssetImportTask(),filename=str(path),destination_path=BASE,destination_name=name,
                automated=True,replace_existing=True,replace_existing_settings=True,save=False,options=options)
            tools.import_asset_tasks([task])
            mesh=u.load_asset(BASE+'/'+name)
            if not mesh: raise RuntimeError('Rock import failed '+name)
            mesh.set_material(0,material)
            settings=[props(u.StaticMeshReductionSettings(),percent_triangles=p,screen_size=s)
                      for p,s in [(1.,1.),(.35,.4),(.1,.15),(.025,.04)]]
            reduction=props(u.StaticMeshReductionOptions(),auto_compute_lod_screen_size=False,reduction_settings=settings)
            if editor.set_lods(mesh,reduction)!=4: raise RuntimeError('Rock fallback LODs missing')
            nanite=editor.get_nanite_settings(mesh)
            props(nanite,enabled=True,fallback_percent_triangles=1.,fallback_relative_error=0.)
            editor.set_nanite_settings(mesh,nanite,True)
            if not editor.set_convex_decomposition_collisions(mesh,8,32,100000): raise RuntimeError('Rock hull generation failed')
            bounds=mesh.get_bounds()
            if max(abs(bounds.origin.x),abs(bounds.origin.y),abs(bounds.origin.z))>.1 or max(bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z)>100.1:
                raise RuntimeError('Rock physical units/bounds mismatch')
            expected=sorted((hi-lo)*50/source['source_radius_metres'] for lo,hi in zip(*source['source_bounds_metres']))
            actual=sorted([bounds.box_extent.x,bounds.box_extent.y,bounds.box_extent.z])
            if any(abs(a-b)>.1 for a,b in zip(actual,expected)): raise RuntimeError('Rock FBX scale mismatch')
            hulls=editor.get_convex_collision_count(mesh)
            if hulls<1: raise RuntimeError('Missing rock collision')
            if not u.EditorAssetLibrary.save_loaded_asset(mesh): raise RuntimeError('Rock save failed')
            records.append(dict(name=name,path=mesh.get_path_name(),lods=editor.get_lod_count(mesh),
                collision_hulls=hulls,nanite=editor.get_nanite_settings(mesh).get_editor_property('enabled'),
                material=material.get_path_name(),source_sha256=source['sha256']))
    finally:
        u.SystemLibrary.execute_console_command(None,flag+' '+str(previous))
    (ROOT/'artifacts/RockContentBuild.json').write_text(json.dumps(records,indent=2)+'\n')
    u.log('DIVE_ROCK_CONTENT_PASS variants=3 lods=4 nanite=1 collision=1')


if __name__=='__main__': build_rocks()
