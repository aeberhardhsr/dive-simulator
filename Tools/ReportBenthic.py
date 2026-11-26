"""Bind benthic material, terrain precision, package and performance evidence."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import numpy as np
from PIL import Image
from ReportRocks import ROOT, read, sha, package_check


def record(path):
    return dict(file=path.relative_to(ROOT).as_posix(), sha256=sha(path), bytes=path.stat().st_size)


def logcheck(path):
    text = path.read_text(encoding='utf-8-sig')
    assert 'DIVE_SMOKE_PASS' in text
    assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:', text)
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--performance', type=Path, required=True)
    args = parser.parse_args()
    run = args.run.resolve()
    host = read(run/'Host.json')
    package_check(host)
    launcher = read(run/'Launcher.json')
    assert launcher['status'] == 'PASS' and launcher['exit_code'] == 0
    assert launcher['executable_sha256'].lower() == host['sha256'].lower()
    assert launcher['log_sha256'].lower() == sha(run/'Launcher.log')
    cases = read(run/'Cases.json')
    assert {c['name'] for c in cases} == {'entry0','entry1','entry2','raster','night','shore0','shore1','shore2','torch'} and len(cases)==9
    evidence = []
    for case in cases:
        folder = run/case['name']
        log = logcheck(folder/'Runtime.log')
        assert case['status'] == 'PASS'
        if case.get('torch'):
            assert 'DIVE_TORCH_PASS on=1' in log and '-DiveHour=0' in log
        elif not case['shore']:
            for marker in ['DIVE_HABITAT_INSPECTION_PASS views=3', 'DIVE_POPULATION_PASS',
                           'DIVE_ROCK_LAYOUT_PASS', 'DIVE_POPULATION_STAGED_PASS', 'DIVE_POPULATION_NEAR_TELEPORT_PASS']:
                assert marker in log
            assert {p.name for p in folder.glob('*.png')} == {f'Habitat_0_{i}.png' for i in range(3)}
        if case['raster']: assert 'r.Nanite = "0"' in log
        if case['night']: assert '-DiveHour=0' in log
        assert 'DIVE_COAST_COLLISION_PASS' in log and 'DIVE_LANDSCAPE_PASS' in log
        images = []
        for path in sorted(folder.glob('*.png')):
            with Image.open(path) as im:
                assert im.size == (1600,900)
                im.verify()
            images.append(record(path))
        assert len(images)==case['screenshots'] and images
        evidence.append(dict(**case, log=record(folder/'Runtime.log'), images=images,
            engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    # Current asset/source bytes must still match the revision pinned by the run.
    paths = [ROOT/'Tools/Shaders/Lakebed.ush', ROOT/'Tools/build_benthic_content.py',
             ROOT/'Tools/build_bed_content.py', ROOT/'Tools/build_gravel_content.py']
    paths += sorted((ROOT/'divingsim/Source/DiveSim').glob('Dive*Terrain*'))
    paths += [ROOT/'divingsim/Source/DiveSim'/n for n in ['DiveCoast.cpp','DiveCoastBakeCommandlet.cpp','DiveLandscape.cpp','DiveLandscapeBakeCommandlet.cpp']]
    paths += [ROOT/'divingsim/Content/Diving/Materials'/f'{n}.uasset' for n in ['M_Lakebed','M_RockSurface','M_GravelSurface','M_ShorePhoto']]
    paths += sorted((ROOT/'divingsim/Content/Diving/Coast').glob('*.uasset'))
    paths += sorted((ROOT/'divingsim/Content/Diving/Landscape').glob('*.uasset'))
    paths += [ROOT/'divingsim/Content/Diving/Geodata'/n for n in ['Coast/meshes.json','landscape_meshes.json']]
    for path in paths:
        committed = subprocess.run(['git','show',f"{host['source_head']}:{path.relative_to(ROOT).as_posix()}"],cwd=ROOT,check=True,capture_output=True).stdout
        if path.suffix=='.uasset':
            assert f'oid sha256:{sha(path)}'.encode() in committed
        else:
            assert path.read_bytes().replace(b'\r\n',b'\n')==committed.replace(b'\r\n',b'\n')
    diagnostics = []
    for name in ['entry2-unlit','entry2-unlit-raster','entry2-precise','entry2-final-unlit']:
        folder=ROOT/'artifacts/benthic-native'/name
        log=logcheck(folder/'Runtime.log')
        assert 'Set new viewmode: Unlit' in log and 'ShowFlag.PostProcessing = "0"' in log
        pose=re.findall(r'DIVE_HABITAT_VIEW view=2[^\r\n]*',log)
        assert pose and all(p==pose[0] for p in pose)
        path=folder/'Habitat_0_2.png'
        pixels=np.asarray(Image.open(path).convert('RGB'))
        count=int((pixels[170:250,180:560].max(axis=2)<8).sum())
        diagnostics.append(dict(name=name,pose=pose[0],dark_pixels=count,log=record(folder/'Runtime.log'),image=record(path)))
    assert len({d['pose'] for d in diagnostics})==1
    assert diagnostics[0]['dark_pixels']>0 and all(d['dark_pixels']==0 for d in diagnostics[1:])
    perf=args.performance.resolve()
    summary=read(perf/'Summary.json')
    assert summary['host']['executable_sha256'].lower()==host['sha256'].lower()
    assert summary['host']['package_containers']==host['package_containers']
    assert not summary['host']['background_processes'] and not summary['host']['editor']
    assert summary['metadata']['scenario']=='perf-underwater'
    perf_log=(perf/'Runtime.log').read_text(encoding='utf-8-sig')
    assert 'DIVE_PERF_CAPTURE_READY' in perf_log
    assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:',perf_log)
    build=ROOT/'artifacts/benthic-package-build.log'
    build_log=build.read_text(encoding='utf-8-sig')
    for marker in ['BUILD SUCCESSFUL','DIVE_COAST_VERIFY_PASS tiles=644 assets=496','DIVE_LAND_VERIFY_PASS tiles=77']:
        assert marker in build_log
    content=read(ROOT/'artifacts/BenthicContentBuild.json')
    assert len(content)==4
    previous=ROOT/'artifacts/population-staged-validation-20261126-152000'
    previous_host=read(previous/'Host.json')
    package_check(previous_host)
    before=previous/'entry2/Habitat_0_2.png'
    after=run/'entry2/Habitat_0_2.png'
    old_pose=re.findall(r'DIVE_HABITAT_VIEW view=2[^\r\n]*',logcheck(previous/'entry2/Runtime.log'))[-1]
    new_pose=re.findall(r'DIVE_HABITAT_VIEW view=2[^\r\n]*',logcheck(run/'entry2/Runtime.log'))[-1]
    assert old_pose==new_pose
    assert sha(ROOT/'Documentation/Images/Benthic_Before.png')==sha(before)
    assert sha(ROOT/'Documentation/Images/Benthic_After.png')==sha(after)
    report=dict(status='PASS',host=host,launcher=launcher,cases=evidence,inputs=[record(p) for p in paths],
        build=record(build),content=content,diagnostics=dict(roi_xyxy=[180,170,560,250],max_channel_threshold=8,cases=diagnostics),
        comparison=dict(previous_host=previous_host,pose=new_pose,before=record(before),after=record(after),
            previous_log=record(previous/'entry2/Runtime.log')),
        performance=dict(summary=summary,files=[record(perf/n) for n in ['Host.json','Metadata.json','Summary.json','Runtime.log','Capture.csv','Frame.png']]),
        limitations=['Authored coating, not surveyed species coverage or physical growth simulation.',
            'The fixed seam crop does not establish absence of all seams everywhere.',
            'Reference visual quality, endurance, minimum hardware and native macOS acceptance remain open.'])
    output=ROOT/'Documentation/BENTHIC_RESULTS.json'
    output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(status='PASS',cases=len(cases),report=str(output))))


if __name__=='__main__':main()
