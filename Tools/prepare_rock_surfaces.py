"""Pin the matching CC0 PBR maps for the three existing scanned boulders."""
import hashlib
import json
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
from prepare_rock_assets import read_url

ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/'Documentation/ROCK_SURFACES.json'


def main():
    geometry=json.loads((ROOT/'Documentation/ROCK_ASSETS.json').read_text())
    if DEST.exists():
        manifest=json.loads(DEST.read_text())
    else:
        manifest=dict(schema=1,license='CC0-1.0',license_url='https://polyhaven.com/license',resolution='2k',local_geology_claim=False,assets=[])
        for index,source in enumerate(geometry['assets']):
            files=json.loads(read_url('https://api.polyhaven.com/files/'+source['id']))
            maps={}
            for kind in ['Diffuse','nor_dx','arm']:
                item=files[kind]['2k']['png']
                maps[kind]=dict(file=f"Art/Rocks/{source['id']}/{source['id']}_{kind}_2k.png",**item)
            manifest['assets'].append(dict(id=source['id'],mesh=f'SM_LakeRock{index+1}',source=source['source'],authors=source['authors'],maps=maps))
    assert [x['id'] for x in manifest['assets']]==[x['id'] for x in geometry['assets']]
    def fetch(item):
        p=ROOT/item['file'];data=p.read_bytes() if p.exists() else read_url(item['url'])
        assert len(data)==item['size'] and int(hashlib.md5(data).hexdigest(),16)==int(item['md5'],16),str(p)
        digest=hashlib.sha256(data).hexdigest()
        assert item.get('sha256',digest)==digest
        if not p.exists():p.write_bytes(data)
        item['sha256']=digest
        print('ROCK_SURFACE_SOURCE_OK',p.name,flush=True)
    with ThreadPoolExecutor(max_workers=3) as pool:
        list(pool.map(fetch,[m for a in manifest['assets'] for m in a['maps'].values()]))
    DEST.write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')


if __name__=='__main__':main()
