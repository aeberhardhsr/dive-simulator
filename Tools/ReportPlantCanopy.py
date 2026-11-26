"""Verify packaged plant geometry, rendered views and performance provenance."""
import argparse
import json
import re
import subprocess
from PIL import Image
from ReportRocks import ROOT, read, sha, package_check
from ReportBenthic import record, logcheck


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=lambda p:(ROOT/p).resolve())
    parser.add_argument('--performance',required=True,type=lambda p:(ROOT/p).resolve())
    args=parser.parse_args()
    run=args.run
    host=read(run/'Host.json');package_check(host)
    launcher=read(run/'Launcher.json')
    assert launcher['status']=='PASS' and launcher['exit_code']==0
    assert launcher['executable_sha256'].lower()==host['sha256'].lower()
    assert launcher['log_sha256'].lower()==sha(run/'Launcher.log')
    cases=read(run/'Cases.json')
    assert len(cases)==5 and {c['name'] for c in cases}=={'forms','entry0','entry1','entry2','stream'}
    evidence=[]
    for case in cases:
        folder=run/case['name'];log=logcheck(folder/'Runtime.log')
        assert case['status']=='PASS'
        if case['forms']:
            assert {p.name for p in folder.glob('*.png')}=={f'Plant_{v}_{a}.png' for v in range(6) for a in range(2)}
            assert 'DIVE_PLANT_INSPECTION_PASS' in log
        elif not case['stream']:
            assert 'DIVE_HABITAT_INSPECTION_PASS views=3' in log
            assert {p.name for p in folder.glob('*.png')}=={f'Habitat_0_{i}.png' for i in range(3)}
        if not case['forms']:
            for marker in ['DIVE_POPULATION_PASS','DIVE_ROCK_LAYOUT_PASS','DIVE_POPULATION_STAGED_PASS','DIVE_POPULATION_NEAR_TELEPORT_PASS']:
                assert marker in log
        images=[]
        for path in sorted(folder.glob('*.png')):
            with Image.open(path) as im:
                assert im.size==(1600,900);im.verify()
            images.append(record(path))
        assert len(images)==case['screenshots'] and images
        evidence.append(dict(**case,log=record(folder/'Runtime.log'),images=images,
            engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    geometry=read(ROOT/'blender/exports/Plants/Manifest.json')
    content=read(ROOT/'artifacts/PlantContentBuild.json')
    assert len(geometry['assets'])==len(content)==3
    for mesh,asset in zip(geometry['assets'],content):
        assert mesh['name']==asset['name'] and mesh['triangles']==asset['triangles'][0]
        assert sha(ROOT/mesh['file'])==mesh['sha256']==asset['source_sha256']
        assert mesh['uv_channels']==asset['source_uv_channels']==2
        assert len(asset['triangles'])==4 and all(a>b>0 for a,b in zip(asset['triangles'],asset['triangles'][1:]))
    paths=[ROOT/p for p in ['Tools/create_aquatic_plants.py','Tools/build_plant_content.py',
        'Tools/Shaders/AquaticPlant.ush','blender/AquaticPlants.blend','blender/exports/Plants/Manifest.json',
        'divingsim/Content/Diving/Materials/M_AquaticPlant.uasset']]
    paths+=sorted((ROOT/'divingsim/Content/Diving/Plants').glob('*.uasset'))
    paths+=[ROOT/m['file'] for m in geometry['assets']]
    for path in paths:
        committed=subprocess.run(['git','show',f"{host['source_head']}:{path.relative_to(ROOT).as_posix()}"],cwd=ROOT,check=True,capture_output=True).stdout
        if path.suffix in ['.uasset','.blend','.fbx']:
            assert f'oid sha256:{sha(path)}'.encode() in committed
        else: assert path.read_bytes().replace(b'\r\n',b'\n')==committed.replace(b'\r\n',b'\n')
    perf=args.performance;summary=read(perf/'Summary.json')
    assert summary['host']['executable_sha256'].lower()==host['sha256'].lower()
    assert summary['host']['package_containers']==host['package_containers']
    assert not summary['host']['background_processes'] and not summary['host']['editor']
    assert summary['metadata']['scenario']=='perf-underwater'
    perf_log=(perf/'Runtime.log').read_text(encoding='utf-8-sig')
    assert 'DIVE_PERF_CAPTURE_READY' in perf_log and not re.search(r'DIVE_\w*FAIL|Fatal error:',perf_log)
    build=ROOT/'artifacts/plants-package-build.log'
    assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
    before=ROOT/'artifacts/plants-before/Plant_5_0.png'
    after=run/'forms/Plant_5_0.png'
    before_log=ROOT/'artifacts/plants-before.log'
    assert 'DIVE_PLANT_INSPECTION_PASS' in logcheck(before_log)
    assert sha(before)==sha(ROOT/'Documentation/Images/PlantCanopy_Before.png')
    assert sha(after)==sha(ROOT/'Documentation/Images/PlantCanopy_After.png')
    report=dict(status='PASS',host=host,launcher=launcher,cases=evidence,geometry=geometry,content=content,
        inputs=[record(p) for p in paths],build=record(build),
        comparison=dict(before=record(before),before_log=record(before_log),after=record(after),
            note='Same plant inspection camera; native previous content versus packaged new content. Ordinary plant animation remains active.'),
        performance=dict(summary=summary,files=[record(perf/n) for n in ['Host.json','Metadata.json','Summary.json','Runtime.log','Capture.csv','Frame.png']]),
        limitations=['Generic authored plant groups, not scans or species-identification models.',
            'Fixed screenshots do not establish temporal anti-aliasing quality or botanical accuracy.',
            'Reference visual quality, long endurance and native macOS acceptance remain open.'])
    output=ROOT/'Documentation/PLANT_CANOPY_RESULTS.json'
    output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(status='PASS',cases=len(cases),screenshots=sum(c['screenshots'] for c in cases),report=str(output))))


if __name__=='__main__':main()
