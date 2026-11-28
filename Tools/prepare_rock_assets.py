"""Pin CC0 rock geometry sources. Existing lake-rock materials supply the surface."""
import hashlib
import json
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'Documentation/ROCK_ASSETS.json'
IDS = ('boulder_01', 'namaqualand_boulder_05', 'namaqualand_boulder_06')


def read_url(url):
    request = urllib.request.Request(url, headers={'User-Agent': 'WalenseeDiveSimulator/0.1 (CC0 rock geometry import)'})
    with urllib.request.urlopen(request, timeout=180) as response:
        return response.read()


def main():
    if MANIFEST.exists():
        manifest = json.loads(MANIFEST.read_text(encoding='utf-8'))
        if tuple(a['id'] for a in manifest['assets']) != IDS:
            raise ValueError('Unexpected rock sources')
    else:
        catalog = json.loads(read_url('https://api.polyhaven.com/assets?type=models'))
        manifest = dict(schema=1, license='CC0-1.0', license_url='https://polyhaven.com/license',
                        geometry_only=True, local_geology_claim=False, assets=[])
        for name in IDS:
            item = json.loads(read_url('https://api.polyhaven.com/files/' + name))['blend']['2k']['blend']
            manifest['assets'].append(dict(id=name, source='https://polyhaven.com/a/' + name,
                authors=catalog[name]['authors'], source_dimensions_mm=catalog[name]['dimensions'],
                file=f'Art/Rocks/{name}/{name}_2k.blend', **{k:item[k] for k in ('url','size','md5')}))

    def fetch(item):
        path = ROOT / item['file']
        path.parent.mkdir(parents=True, exist_ok=True)
        content = path.read_bytes() if path.exists() else read_url(item['url'])
        if len(content) != item['size'] or int(hashlib.md5(content).hexdigest(), 16) != int(item['md5'], 16):
            raise ValueError('Source size/MD5 mismatch: ' + item['file'])
        digest = hashlib.sha256(content).hexdigest()
        if item.get('sha256', digest) != digest:
            raise ValueError('Pinned SHA256 mismatch: ' + item['file'])
        if not path.exists():
            temporary = path.with_suffix(path.suffix + '.tmp')
            temporary.write_bytes(content)
            temporary.replace(path)
        item['sha256'] = digest
        print('ROCK_SOURCE_VERIFIED', path.name, len(content), flush=True)

    with ThreadPoolExecutor(max_workers=3) as pool:
        list(pool.map(fetch, manifest['assets']))
    MANIFEST.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
