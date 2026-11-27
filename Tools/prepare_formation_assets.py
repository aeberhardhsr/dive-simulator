"""Pin two closed CC0 outcrop cores and their matching 4K scan maps."""
import hashlib,json
from pathlib import Path
from concurrent.futures import ThreadPoolExecutor
from prepare_rock_assets import read_url

ROOT=Path(__file__).resolve().parents[1]
DEST=ROOT/'Documentation/ROCK_FORMATION_ASSETS.json'
IDS=('namaqualand_boulder_03','namaqualand_boulder_04')

def main():
    if DEST.exists():
        manifest=json.loads(DEST.read_text(encoding='utf-8'))
    else:
        catalog=json.loads(read_url('https://api.polyhaven.com/assets?type=models'))
        manifest=dict(schema=1,license='CC0-1.0',license_url='https://polyhaven.com/license',
                      local_geology_claim=False,resolution='4k',assets=[])
        for index,name in enumerate(IDS,4):
            files=json.loads(read_url('https://api.polyhaven.com/files/'+name))
            model=files['blend']['2k']['blend']
            maps={kind:dict(file=f'Art/Rocks/{name}/{name}_{kind}_4k.png',**files[kind]['4k']['png'])
                  for kind in ('Diffuse','nor_dx','arm')}
            manifest['assets'].append(dict(id=name,mesh=f'SM_LakeRock{index}',source='https://polyhaven.com/a/'+name,
                authors=catalog[name]['authors'],source_dimensions_mm=catalog[name]['dimensions'],
                file=f'Art/Rocks/{name}/{name}_2k.blend',maps=maps,**{k:model[k] for k in ('url','size','md5')}))
    assert tuple(a['id'] for a in manifest['assets'])==IDS
    def fetch(item):
        p=ROOT/item['file'];p.parent.mkdir(parents=True,exist_ok=True)
        data=p.read_bytes() if p.exists() else read_url(item['url'])
        assert len(data)==item['size'] and int(hashlib.md5(data).hexdigest(),16)==int(item['md5'],16),str(p)
        digest=hashlib.sha256(data).hexdigest();assert item.get('sha256',digest)==digest
        if not p.exists():p.write_bytes(data)
        item['sha256']=digest
        print('FORMATION_SOURCE_OK',p.name,len(data),flush=True)
    items=[item for asset in manifest['assets'] for item in [asset,*asset['maps'].values()]]
    with ThreadPoolExecutor(max_workers=3) as pool:list(pool.map(fetch,items))
    DEST.write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')

if __name__=='__main__':main()
