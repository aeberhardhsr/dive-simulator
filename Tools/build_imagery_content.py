"""Import verified SWISSIMAGE textures and create their terrain materials."""
import hashlib
import json
from pathlib import Path
import sys
import unreal as u

ROOT=Path(__file__).resolve().parents[1]


def build_imagery():
    manifest=json.loads((ROOT/"divingsim/Content/Diving/Geodata/imagery.json").read_text(encoding="utf-8"))
    lib=u.MaterialEditingLibrary
    for item in manifest["images"]:
        path="/Game/Diving/Textures/"+item["asset"]
        source=ROOT/"Geodata/Processed/Imagery"/item["file"]
        if source.exists():
            if hashlib.sha256(source.read_bytes()).hexdigest()!=item["sha256"]:
                raise RuntimeError("Imagery source checksum differs: "+str(source))
            task=u.AssetImportTask()
            task.set_editor_property("filename",str(source))
            task.set_editor_property("destination_path","/Game/Diving/Textures")
            task.set_editor_property("destination_name",item["asset"])
            task.set_editor_property("automated",True)
            task.set_editor_property("replace_existing",True)
            task.set_editor_property("save",False)
            u.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
            texture=u.load_asset(path)
            if not texture: raise RuntimeError("Texture import failed: "+path)
            u.EditorAssetLibrary.set_metadata_tag(texture,"DiveImagerySHA256",item["sha256"])
        else:
            texture=u.load_asset(path)
            if not texture or u.EditorAssetLibrary.get_metadata_tag(texture,"DiveImagerySHA256")!=item["sha256"]:
                raise RuntimeError("Run Tools/prepare_imagery.py before rebuilding imagery")
        texture.set_editor_property("srgb",True)
        texture.set_editor_property("address_x",u.TextureAddress.TA_CLAMP)
        texture.set_editor_property("address_y",u.TextureAddress.TA_CLAMP)
        texture.set_editor_property("lod_bias",0)
        texture.set_editor_property("max_texture_size",4096)
        # Procedural meshes do not provide reliable imported UV-density data
        # to the texture streamer. Four bounded terrain maps fit in ~13.4 MiB BC1.
        texture.set_editor_property("never_stream",True)
        if not u.EditorAssetLibrary.save_loaded_asset(texture): raise RuntimeError("Cannot save "+path)
        u.log("DIVE_IMAGERY_TEXTURE "+item["asset"])
    path="/Game/Diving/Materials/M_LandscapePhoto"
    existing=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
    material=existing or u.AssetToolsHelpers.get_asset_tools().create_asset(
        "M_LandscapePhoto","/Game/Diving/Materials",u.Material,u.MaterialFactoryNew())
    for node in list(lib.get_material_expressions(material)): lib.delete_material_expression(material,node)
    material.set_editor_property("two_sided",True)
    photo=lib.create_material_expression(material,u.MaterialExpressionTextureSampleParameter2D)
    photo.set_editor_property("parameter_name","Ortho")
    photo.set_editor_property("texture",u.load_asset("/Game/Diving/Textures/T_Ortho_landscape"))
    lib.connect_material_property(photo,"RGB",u.MaterialProperty.MP_BASE_COLOR)
    rough=lib.create_material_expression(material,u.MaterialExpressionConstant)
    rough.set_editor_property("r",.92)
    lib.connect_material_property(rough,"",u.MaterialProperty.MP_ROUGHNESS)
    lib.recompile_material(material)
    if not u.EditorAssetLibrary.save_loaded_asset(material): raise RuntimeError("Cannot save landscape photo material")
    sys.path.insert(0,str(ROOT/"Tools"))
    from build_shore_content import build_shore_material
    build_shore_material("M_ShorePhoto",True)
    u.log("DIVE_IMAGERY_CONTENT_OK")


if __name__=="__main__": build_imagery()
