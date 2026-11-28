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
        texture=u.load_asset(path) if u.EditorAssetLibrary.does_asset_exist(path) else None
        if source.exists():
            if hashlib.sha256(source.read_bytes()).hexdigest()!=item["sha256"]:
                raise RuntimeError("Imagery source checksum differs: "+str(source))
        current=texture and u.EditorAssetLibrary.get_metadata_tag(texture,"DiveImagerySHA256")==item["sha256"]
        if source.exists() and not current:
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
        elif not current:
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
    sys.path.insert(0,str(ROOT/"Tools"))
    from build_ground_content import build_landscape_ground
    build_landscape_ground()
    u.log("DIVE_IMAGERY_CONTENT_OK")


if __name__=="__main__": build_imagery()
