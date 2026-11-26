"""Rebuild shared wet substrate materials without reimporting any meshes."""
from pathlib import Path
import sys
import hashlib
import json
import unreal as u
sys.path.insert(0, str(Path(__file__).resolve().parent))
from build_bed_content import build_bed
from build_gravel_content import build_gravel_material

build_bed()
build_gravel_material()
records = []
for name in ['M_Lakebed', 'M_RockSurface', 'M_GravelSurface', 'M_ShorePhoto']:
    material = u.load_asset('/Game/Diving/Materials/' + name)
    expressions = [e for e in u.MaterialEditingLibrary.get_material_expressions(material)
                   if isinstance(e, u.MaterialExpressionCustom)]
    if len(expressions) != 1:
        raise RuntimeError('Unexpected substrate graph: ' + name)
    code = expressions[0].get_editor_property('code')
    digest = hashlib.sha256(code.encode()).hexdigest()
    if 'float film=' not in code or u.EditorAssetLibrary.get_metadata_tag(material, 'DiveBedShaderSHA256') != digest:
        raise RuntimeError('Stale substrate shader: ' + name)
    records.append(dict(material=name, shader_sha256=digest))
root = Path(__file__).resolve().parents[1]
(root/'artifacts/BenthicContentBuild.json').write_text(json.dumps(records, indent=2)+'\n')
u.log('DIVE_BENTHIC_CONTENT_PASS')
