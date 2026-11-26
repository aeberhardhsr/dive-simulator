"""Bind revised plant geometry, original-model comparisons and package checks."""
import argparse
import json
from pathlib import Path
from ReportNativeWaterLighting import ROOT, record, sha, pixels


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--package-report',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True)
    a=p.parse_args()
    package=json.loads(a.package_report.read_text(encoding='utf-8'))
    assert package['status']=='PASS' and package['host']['packaged']
    assert len(package['cases'])==9 and package['screenshot_count']==43
    assert package['host']['plant_forms'] and package['launcher']['status']=='PASS'
    for item in package['host']['files']:assert sha(ROOT/item['file'])==item['sha256']
    manifest_path=ROOT/'blender/exports/Plants/Manifest.json'
    manifest=json.loads(manifest_path.read_text())
    imported=json.loads((ROOT/'artifacts/PlantContentBuild.json').read_text())
    assert manifest['revision']=='curved-shoots' and len(manifest['assets'])==3
    limits={'SM_Pondweed':1.15,'SM_Stonewort':.45,'SM_Milfoil':.9}
    for item in manifest['assets']:
        assert sha(ROOT/item['file'])==item['sha256']
        assert 0<item['height_metres']<=limits[item['name']]+1.e-6
        entry=next(x for x in imported if x['name']==item['name'])
        assert entry['source_sha256']==item['sha256'] and entry['source_uv_channels']==2
        assert entry['triangles'][0]==item['triangles']
        assert len(entry['triangles'])==4 and all(x>y>0 for x,y in zip(entry['triangles'],entry['triangles'][1:]))
    old=json.loads((ROOT/'Documentation/PLANT_BEDS_RESULTS.json').read_text())
    backup=ROOT/'artifacts/plant-structure-source-before'
    for item in old['host']['files']:
        if '/Plants/' in item['file'] or item['file'].endswith('/M_AquaticPlant.uasset'):
            assert sha(backup/Path(item['file']).name)==item['sha256']
    comparisons=[]
    for species,variant in [('Pondweed',1),('Stonewort',3),('Milfoil',5)]:
        for view in range(2):
            pair=[]
            for phase in ['before','after']:
                path=ROOT/f'artifacts/plant-structure-{phase}/Plant_{variant}_{view}.png'
                pixels(path);pair.append(record(path))
                if view==1:
                    published=ROOT/f'Documentation/Images/PlantGrowth_{species}_{phase}.png'
                    assert sha(published)==sha(path)
            comparisons.append(dict(species=species,view=view,before=pair[0],after=pair[1]))
    logs=[]
    for name in ['before','after','habitat']:
        path=ROOT/f'artifacts/plant-structure-{name}.log'
        log=path.read_text(encoding='utf-8-sig')
        assert 'DIVE_SMOKE_PASS' in log and 'DIVE_PLANT_INSPECTION_FAIL' not in log
        if name=='habitat':assert 'DIVE_PLANT_BEDS_PASS' in log and 'DIVE_POPULATION_PASS' in log
        else:assert 'DIVE_PLANT_INSPECTION_PASS groups=3 states=2 views=2 captures=12' in log
        logs.append(record(path))
    files=[ROOT/'Tools/create_aquatic_plants.py',ROOT/'blender/AquaticPlants.blend',manifest_path,
        ROOT/'artifacts/PlantContentBuild.json',ROOT/'artifacts/plant-structure-bounded-blender.log',
        ROOT/'artifacts/plant-structure-bounded-content.log',ROOT/'artifacts/plant-structure-blender.log',
        ROOT/'artifacts/plant-structure-content-build.log']
    files += [ROOT/x['file'] for x in manifest['assets']]
    files += list(backup.iterdir())
    result=dict(status='PASS',scope='Curved authored plant models; full scene reference remains open',
        package=record(a.package_report.resolve()),manifest=manifest,imported=imported,files=[record(x) for x in files],
        comparisons=comparisons,native_logs=logs,
        limitations=['General botanical groups, not species-identification models or surveyed local vegetation',
            'More detailed geometry retains automatic distance simplification and subpixel foliage limits',
            'Screenshot checks and short benchmarks do not prove full visual or endurance acceptance'])
    a.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print('PASS revised models, six native comparisons, nine package cases / 43 captures')


if __name__=='__main__':main()
