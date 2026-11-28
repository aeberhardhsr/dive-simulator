"""Update source colour interpretation without rebuilding material graphs/meshes."""
import hashlib
import json
from pathlib import Path
import sys
import unreal as u

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT/'Tools'))
from texture_color import set_scan_source_encoding

records = []
for manifest, scanned in [('BED_MATERIALS.json', False), ('ROCK_SURFACES.json', True)]:
    assets = json.loads((ROOT/'Documentation'/manifest).read_text())['assets']
    for index, asset in enumerate(assets):
        prefix = f'T_RockScan{index+1}_' if scanned else 'T_Bed_' + {
            'gravelly_sand': 'Sand', 'river_small_rocks': 'Gravel', 'rock_boulder_dry': 'Rock'}[asset['id']]
        for kind, suffix in [('Diffuse', 'Color'), ('nor_dx', 'ScanNormal' if scanned else 'Normal'), ('arm', 'ARM')]:
            source = asset['maps'][kind]
            assert hashlib.sha256((ROOT/source['file']).read_bytes()).hexdigest() == source['sha256']
            name = prefix + suffix
            texture = u.load_asset('/Game/Diving/Textures/' + name)
            tag = 'DiveScanSHA256' if scanned else 'DiveBedSHA256'
            if not texture or u.EditorAssetLibrary.get_metadata_tag(texture, tag) != source['sha256']:
                raise RuntimeError('Missing or stale imported source: ' + name)
            set_scan_source_encoding(texture, kind == 'Diffuse')
            if not u.EditorAssetLibrary.save_loaded_asset(texture):
                raise RuntimeError('Cannot save ' + name)
            records.append(dict(asset=texture.get_path_name(), source_sha256=source['sha256'],
                source_encoding=str(texture.get_editor_property('source_color_settings').get_editor_property('encoding_override'))))
(ROOT/'artifacts/BenthicColorBuild.json').write_text(json.dumps(records, indent=2)+'\n')
u.log('DIVE_BENTHIC_COLOR_PASS textures=18')
