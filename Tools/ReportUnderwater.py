"""Verify the packaged gravel/plant increment against actual captures and inputs."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from PIL import Image
from ReportRocks import package_check
from analyze_performance import analyze

ROOT=Path(__file__).resolve().parents[1]
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))
def record(path):
    data=path.read_bytes()
    return dict(file=path.relative_to(ROOT).as_posix(),bytes=len(data),sha256=hashlib.sha256(data).hexdigest())

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('run',type=Path)
    parser.add_argument('--performance',type=Path,required=True)
    args=parser.parse_args();run=args.run.resolve()
    host=read(run/'Host.json');package_check(host)
    cases=read(run/'Cases.json')
    names={'forms','forms-raster','stream0','stream1','stream2','shore0','shore1','shore2','discoveries','plants','gravel','gravel-raster'}
    assert len(cases)==len(names) and {c['name'] for c in cases}==names
    evidence=[]
    for case in cases:
        folder=run/case['name'];log_path=folder/'Runtime.log';log=log_path.read_text(encoding='utf-8-sig')
        assert case['status']=='PASS' and 'DIVE_SMOKE_PASS' in log,case['name']
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:',log),case['name']
        if case['scenario']=='rocks':
            prefix,variants,marker=('Plant',6,'DIVE_PLANT_INSPECTION_PASS') if case.get('plants') else (
                ('Gravel',2,'DIVE_GRAVEL_INSPECTION_PASS') if case.get('gravel') else ('Rock',4,'DIVE_ROCK_INSPECTION_PASS'))
            assert marker in log
            assert {p.name for p in folder.glob('*.png')}=={f'{prefix}_{v}_{a}.png' for v in range(variants) for a in range(2)}
        if case['raster']:assert 'r.Nanite = "0"' in log
        if case['scenario']=='stream':
            for marker in ['DIVE_POPULATION_PASS','DIVE_STEERING_WORLD_PASS species=5 steps_per_species=60','DIVE_GRAVEL_PASS']:assert marker in log
            assert 'staged=7,8,9' in log or 'completed=7,8,9' in log
        if case['scenario']=='discoveries':assert 'DIVE_DISCOVERY_VISIBILITY_PASS' in log
        images=[]
        for path in sorted(folder.glob('*.png')):
            with Image.open(path) as im:
                assert im.size in [(1600,900),(768,512)];im.verify()
            images.append(record(path))
        assert images and len(images)==case['screenshots']
        evidence.append(dict(**case,log=record(log_path),images=images,
            engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    inputs=[]
    for kind in ['Plants','Gravel']:
        geometry=read(ROOT/f'blender/exports/{kind}/Manifest.json')
        imported=read(ROOT/('artifacts/PlantContentBuild.json' if kind=='Plants' else 'artifacts/GravelContentBuild.json'))
        assert len(geometry['assets'])==len(imported)==3
        for source,built in zip(geometry['assets'],imported):
            assert source['name']==built['name'] and source['sha256']==built['source_sha256']
            assert record(ROOT/source['file'])['sha256']==source['sha256']
            assert built['triangles'][0]==source['triangles'] and len(built['triangles'])==4
        inputs.append(dict(kind=kind,geometry=geometry,imported=imported))
    source_files=[]
    for directory in ['Source','Config']:
        for path in sorted((ROOT/'divingsim'/directory).rglob('*')):
            if not path.is_file():continue
            relative=path.relative_to(ROOT/'divingsim')
            sandbox=ROOT/'Builds/EditorSandbox/divingsim'/relative
            assert sandbox.read_bytes()==path.read_bytes(),relative
            source_files.append(record(path))
    assets=sorted((ROOT/'divingsim/Content/Diving/Plants').glob('*.uasset'))+sorted((ROOT/'divingsim/Content/Diving/Gravel').glob('*.uasset'))
    assets += [ROOT/'divingsim/Content/Diving/Materials'/name for name in ['M_AquaticPlant.uasset','M_GravelSurface.uasset','M_Lakebed.uasset']]
    bootstrap=read(ROOT/'artifacts/underwater-bootstrap.json')
    assert bootstrap['status']=='PASS' and bootstrap['executable_sha256'].lower()==host['sha256'].lower()
    ci=read(ROOT/'artifacts/underwater-ci.json')
    assert ci['source_commit']==host['source_head'] and len(ci['jobs'])==3
    assert all(job['conclusion']=='success' for job in ci['jobs'])
    build=ROOT/'artifacts/underwater-package.log'
    assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
    performance_run=args.performance.resolve()
    performance=analyze(performance_run)
    assert performance['host']['executable_sha256'].lower()==host['sha256'].lower()
    assert sorted(performance['host']['package_containers'],key=lambda x:x['file'])==sorted(host['package_containers'],key=lambda x:x['file'])
    performance['evidence']=[record(performance_run/name) for name in ['Capture.csv','Metadata.json','Host.json','Runtime.log','Frame.png']]
    performance['scope']='One 60-second underwater scenario; no whole-game, long-duration or cross-hardware performance claim'
    (ROOT/'Documentation/UNDERWATER_DETAIL_PERFORMANCE.json').write_text(json.dumps(performance,indent=2)+'\n')
    report=dict(schema=1,status='PACKAGED_INTEGRATION_PASS',host=host,cases=evidence,inputs=inputs,
        source_files=source_files,assets=[record(p) for p in assets],build=record(build),bootstrap=bootstrap,ci=ci,
        performance=record(ROOT/'Documentation/UNDERWATER_DETAIL_PERFORMANCE.json'),
        limitations=['Visual reference not yet achieved','No endurance test of this new package; performance scope recorded separately',
            'Botanical groups and authored distribution, not species-identification or local survey models'])
    (ROOT/'Documentation/UNDERWATER_DETAIL_RESULTS.json').write_text(json.dumps(report,indent=2)+'\n')
    print(f"UNDERWATER_PACKAGE_REPORT_PASS cases={len(cases)} images={sum(len(c['images']) for c in evidence)}")

if __name__=='__main__':main()
