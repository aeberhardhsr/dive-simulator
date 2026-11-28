"""Fetch the pinned CC0 source maps used by the lakebed material builder."""
import hashlib
import json
import urllib.request
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / 'Art/BedMaterials'
MANIFEST = ROOT / 'Documentation/BED_MATERIALS.json'
AGENT = 'WalenseeDiveSimulator/0.1 (CC0 material import)'
ASSETS = [('gravelly_sand', 2.5), ('river_small_rocks', 2.9), ('rock_boulder_dry', 1.8)]


def read_url(url):
    request = urllib.request.Request(url, headers={'User-Agent': AGENT})
    with urllib.request.urlopen(request, timeout=120) as response:
        return response.read()


def main():
    DEST.mkdir(parents=True, exist_ok=True)
    if MANIFEST.exists():
        manifest = json.loads(MANIFEST.read_text(encoding='utf-8'))
    else:
        manifest = {'license': 'CC0-1.0', 'license_url': 'https://polyhaven.com/license',
                    'resolution': '2k', 'assets': []}
        for name, scale in ASSETS:
            data = json.loads(read_url('https://api.polyhaven.com/files/' + name))
            maps = {}
            for kind in ['Diffuse', 'nor_dx', 'arm']:
                item = dict(data[kind]['2k']['png'])
                item['file'] = f'Art/BedMaterials/{name}_{kind}.png'
                maps[kind] = item
            manifest['assets'].append({'id': name, 'source': 'https://polyhaven.com/a/' + name,
                                       'tile_metres': scale, 'maps': maps})

    def fetch(item):
        path = ROOT / item['file']
        data = path.read_bytes() if path.exists() else read_url(item['url'])
        if len(data) != item['size'] or hashlib.md5(data).hexdigest() != item['md5']:
            raise ValueError('Source size/MD5 differs: ' + item['file'])
        sha = hashlib.sha256(data).hexdigest()
        if item.get('sha256', sha) != sha:
            raise ValueError('Source SHA256 differs: ' + item['file'])
        if not path.exists():
            path.write_bytes(data)
        item['sha256'] = sha
        print('BED_SOURCE_VERIFIED', path.name, len(data))

    items = [item for asset in manifest['assets'] for item in asset['maps'].values()]
    with ThreadPoolExecutor(max_workers=3) as pool:
        list(pool.map(fetch, items))
    MANIFEST.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
