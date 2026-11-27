"""Pin and verify original height maps matching the existing bed scans."""
import hashlib
import json
from pathlib import Path
from prepare_bed_materials import read_url

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT/'Documentation/BED_RELIEF_SOURCES.json'


def main():
    if MANIFEST.exists():
        manifest = json.loads(MANIFEST.read_text())
    else:
        manifest = dict(license='CC0-1.0', license_url='https://polyhaven.com/license', assets=[])
        for name in ['gravelly_sand', 'river_small_rocks']:
            item = dict(json.loads(read_url('https://api.polyhaven.com/files/'+name))['Displacement']['2k']['png'])
            item.update(id=name, source='https://polyhaven.com/a/'+name, file=f'Art/BedMaterials/{name}_Height.png')
            manifest['assets'].append(item)
    for item in manifest['assets']:
        path = ROOT/item['file']
        data = path.read_bytes() if path.exists() else read_url(item['url'])
        digest = hashlib.sha256(data).hexdigest()
        assert len(data)==item['size'] and hashlib.md5(data).hexdigest()==item['md5']
        assert item.get('sha256',digest)==digest
        if not path.exists():
            path.write_bytes(data)
        item['sha256']=digest
        print('BED_HEIGHT_VERIFIED',path.name,len(data))
    MANIFEST.write_text(json.dumps(manifest,indent=2)+'\n')


if __name__=='__main__':
    main()
