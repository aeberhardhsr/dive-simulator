"""Download and verify the CC0 tree sources used by the landscape asset pipeline."""
import hashlib
import json
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import urllib.request

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'Documentation/VEGETATION_ASSETS.json'
IDS = ('tree_small_02', 'fir_tree_01')


def read_url(url):
    request = urllib.request.Request(url, headers={'User-Agent': 'WalenseeDiveSimulator/0.1 (CC0 tree import)'})
    with urllib.request.urlopen(request, timeout=180) as response:
        return response.read()


def main():
    if MANIFEST.exists():
        manifest = json.loads(MANIFEST.read_text(encoding='utf-8'))
        if tuple(a['id'] for a in manifest['assets']) != IDS:
            raise ValueError('Unexpected source models')
    else:
        catalog = json.loads(read_url('https://api.polyhaven.com/assets?type=models&category=nature'))
        manifest = dict(license='CC0-1.0', license_url='https://polyhaven.com/license', resolution='2k', assets=[])
        for name in IDS:
            data = json.loads(read_url('https://api.polyhaven.com/files/' + name))
            blend = dict(data['blend']['2k']['blend'])
            files = [dict(file=f'Art/Vegetation/{name}/{name}_2k.blend', **{k:blend[k] for k in ('url','size','md5')})]
            for relative, item in sorted(blend['include'].items()):
                if '..' in Path(relative).parts or Path(relative).is_absolute():
                    raise ValueError('Invalid source path')
                files.append(dict(file=f'Art/Vegetation/{name}/{relative}', **item))
            manifest['assets'].append(dict(id=name, source='https://polyhaven.com/a/' + name,
                authors=catalog[name]['authors'], description=catalog[name]['description'],
                local_species_claim=False, files=files))

    def fetch(item):
        path = ROOT / item['file']
        path.parent.mkdir(parents=True, exist_ok=True)
        content = path.read_bytes() if path.exists() else read_url(item['url'])
        # Poly Haven uses unpadded hexadecimal MD5 in some catalogue records.
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
        print('VEGETATION_SOURCE_VERIFIED', path.name, len(content), flush=True)

    with ThreadPoolExecutor(max_workers=3) as pool:
        list(pool.map(fetch, [f for a in manifest['assets'] for f in a['files']]))
    MANIFEST.write_text(json.dumps(manifest, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
