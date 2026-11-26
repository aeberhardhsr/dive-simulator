"""Download and pin the CC0 grass/forest surface maps; safe to rerun offline."""
import json
import prepare_bed_materials as source

if __name__ == '__main__':
    source.DEST = source.ROOT / 'Art/GroundMaterials'
    source.MANIFEST = source.ROOT / 'Documentation/GROUND_MATERIALS.json'
    source.ASSETS = [('grass_ground', 2.51), ('forest_ground_04', 3.15)]
    # The shared downloader uses its destination only for creation, so pin the
    # new paths before invoking its checksum verification/download stage.
    if not source.MANIFEST.exists():
        assets = []
        for name, scale in source.ASSETS:
            data = json.loads(source.read_url('https://api.polyhaven.com/files/' + name))
            maps = {}
            for kind in ['Diffuse', 'nor_dx', 'arm']:
                item = dict(data[kind]['2k']['png'])
                item['md5'] = item['md5'].zfill(32)
                item['file'] = f'Art/GroundMaterials/{name}_{kind}.png'
                maps[kind] = item
            assets.append({'id': name, 'source': 'https://polyhaven.com/a/' + name,
                           'tile_metres': scale, 'maps': maps})
        source.MANIFEST.write_text(json.dumps({'license': 'CC0-1.0',
            'license_url': 'https://polyhaven.com/license', 'resolution': '2k',
            'assets': assets}, indent=2) + '\n', encoding='utf-8')
    source.main()
