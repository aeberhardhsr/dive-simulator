"""Bind generated fish artwork and packaged render/gameplay checks to their inputs."""
from pathlib import Path
import argparse
import hashlib
import json
import re
from PIL import Image, ImageChops, ImageStat
ROOT=Path(__file__).resolve().parents[1]
def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))
def sha(p):
    with p.open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()
def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('run',type=Path);parser.add_argument('--performance',type=Path,required=True);a=parser.parse_args()
    run=a.run.resolve();host=read(run/'Host.json');exe=Path(host['executable']);assert sha(exe)==host['sha256'].lower()
    containers=[dict(file=p.name,bytes=p.stat().st_size,sha256=sha(p)) for p in sorted((exe.parents[3]/'divingsim/Content/Paks').iterdir()) if p.is_file()]
    assert containers==sorted(host['package_containers'],key=lambda p:p['file'])
    cases=read(run/'Cases.json');assert len(cases)==7
    assert {x['name'] for x in cases}=={'fish','weesen','murg','mols','stream','discoveries','decompression'}
    evidence=[]
    for case in cases:
        folder=run/case['name'];log=(folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert case['status']=='PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used',log)
        for marker in ['DIVE_IMAGERY_PASS','DIVE_GROUND_PASS','DIVE_MEDIUM_PASS','DIVE_LIGHT_PASS']:assert marker in log
        if case['name']=='fish':
            assert 'DIVE_FISH_INSPECTION_PASS species=5 views=20 captures=40 lods=4' in log
            for species in ['Perch','Roach','Pike','Whitefish','Burbot']:
                for view in range(4):
                    images=[Image.open(folder/f'Fish_{species}_{view}_{frame}.png').convert('RGB') for frame in [1,2]]
                    # A fixed camera/background must produce different frames
                    # as the ordinary per-instance deformation cycle advances.
                    difference=ImageStat.Stat(ImageChops.difference(*images)).mean
                    assert sum(difference)>.02,(species,view,'no rendered movement')
        if case['name']=='stream':assert 'DIVE_POPULATION_PASS' in log
        if case['name']=='discoveries':assert 'DIVE_DISCOVERY_VISIBILITY_PASS' in log
        if case['name']=='decompression':assert 'DIVE_DECOMPRESSION_PASS' in log
        images=[]
        for p in sorted(folder.glob('*.png')):
            with Image.open(p) as im:assert im.size in [(1600,900),(768,512)];im.verify()
            images.append(dict(file=p.relative_to(ROOT).as_posix(),sha256=sha(p)))
        assert len(images)==case['screenshots'] and images
        evidence.append(dict(**case,log_sha256=sha(folder/'Runtime.log'),images=images))
    atlas=read(ROOT/'Documentation/FISH_TEXTURES.json');mesh=read(ROOT/'Documentation/FISH_MESH_AUDIT.json')
    assert len(atlas['assets'])==5 and mesh['status']=='PASS'
    for item in atlas['assets']:
        for info in item['maps'].values():assert sha(ROOT/info['file'])==info['sha256']
    for item in mesh['meshes']:assert sha(ROOT/'blender/exports'/('SM_'+item['name']+'.fbx'))==item['fbx_sha256']
    content=read(ROOT/'artifacts/FishContentBuild.json');assert len(content)==5 and all(x['lods']==4 and x['uv_channels']>=1 for x in content)
    importlog=ROOT/'artifacts/fish-content-colors.log';text=importlog.read_text(encoding='utf-8-sig')
    assert not re.search(r'FBXImport: Warning|LogPython: Error:|Failed to compile Material',text)
    editorlog=ROOT/'artifacts/fish-inspection-colors.log';text=editorlog.read_text(encoding='utf-8-sig')
    for index in range(5):assert re.search(rf'DIVE_FISH_MESH species={index} .*colors=[1-9]\d+ tinted=[1-9]\d+ alpha=0\.\.255',text)
    perf=a.performance.resolve();summary=read(perf/'Summary.json')
    assert summary['host']['executable_sha256']==sha(exe) and not summary['host']['editor']
    assert summary['host']['package_containers']==host['package_containers']
    assert summary['metadata']['width']==1920 and summary['metadata']['height']==1080 and summary['metadata']['quality']==2
    assert 59<=summary['measured_seconds']<=61
    launcher=read(ROOT/'artifacts/fish-bootstrap.json');assert launcher['status']=='PASS' and launcher['executable_sha256'].lower()==sha(exe)
    assert sha(ROOT/'artifacts/fish-bootstrap.log')==launcher['log_sha256'].lower()
    package=ROOT/'artifacts/fish-package.log';assert 'BUILD SUCCESSFUL' in package.read_text(encoding='utf-8-sig')
    sources=[]
    for directory in ['Source','Config']:
        for p in sorted((ROOT/'divingsim'/directory).rglob('*')):
            if not p.is_file():continue
            rel=p.relative_to(ROOT/'divingsim');data=p.read_bytes().replace(b'\r\n',b'\n')
            assert data==(ROOT/'Builds/EditorSandbox/divingsim'/rel).read_bytes().replace(b'\r\n',b'\n')
            sources.append(dict(file=rel.as_posix(),canonical_lf_sha256=hashlib.sha256(data).hexdigest()))
    assets=[]
    for p in sorted((ROOT/'divingsim/Content/Diving').rglob('*.uasset')):
        if '/Textures/Fish/' in p.as_posix() or p.stem in ['M_Fish']+['SM_'+n for n in ['Perch','Roach','Pike','Whitefish','Burbot']] or p.stem.startswith('MI_Fish_'):
            assets.append(dict(file=p.relative_to(ROOT).as_posix(),sha256=sha(p)))
    assert len(assets)==26
    baseline=ROOT/'Builds/WindowsDecompression/divingsim/Binaries/Win64/divingsim-Decompression.exe'
    assert sha(baseline)=='1b22cce0d5f40877556c0d2a820250516119e9243d724e0eb6f709a31e67b5f1'
    report=dict(status='PASS',executable=host,cases=evidence,sources=sources,assets=assets,mesh_audit=mesh,content_build=content,
        root_launcher=launcher,performance=summary,performance_files={n:sha(perf/n) for n in ['Capture.csv','Metadata.json','Host.json','Runtime.log','Frame.png','Summary.json']},
        import_log_sha256=sha(importlog),editor_color_audit_sha256=sha(editorlog),package_log_sha256=sha(package),
        preserved_decompression_executable_sha256=sha(baseline),
        limits='Authored meshes/textures, not scans or expert anatomical/photorealistic acceptance. Whitefish remains a group. Close-up fixture forces four LODs at a fixed location; natural AI is checked separately. One development-host performance capture, not minimum-hardware/macOS/endurance certification.')
    (ROOT/'Documentation/FISH_RESULTS.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('FISH_REPORT_PASS cases=7 images='+str(sum(x['screenshots'] for x in cases)))
if __name__=='__main__':main()
