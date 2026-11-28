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
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('run',type=Path);parser.add_argument('--performance',type=Path,required=True);parser.add_argument('--steering',action='store_true');a=parser.parse_args()
    prefix='steering' if a.steering else 'fin'
    run=a.run.resolve();host=read(run/'Host.json');exe=Path(host['executable']);assert sha(exe)==host['sha256'].lower()
    containers=[dict(file=p.name,bytes=p.stat().st_size,sha256=sha(p)) for p in sorted((exe.parents[3]/'divingsim/Content/Paks').iterdir()) if p.is_file()]
    assert containers==sorted(host['package_containers'],key=lambda p:p['file'])
    expected={'weesen','murg','mols','stream','discoveries'} if a.steering else {'fish','fins','weesen','murg','mols','stream','discoveries','decompression'}
    cases=read(run/'Cases.json');assert len(cases)==len(expected)
    assert {x['name'] for x in cases}==expected
    evidence=[];fin_changes=[]
    for case in cases:
        folder=run/case['name'];log=(folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert case['status']=='PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used',log)
        for marker in ['DIVE_IMAGERY_PASS','DIVE_GROUND_PASS','DIVE_MEDIUM_PASS','DIVE_LIGHT_PASS']:assert marker in log
        if case['name'] in ('fish','fins'):
            assert 'DIVE_FISH_INSPECTION_PASS species=5 views=20 captures=40 lods=4' in log
            image_prefix='Fin' if case['name']=='fins' else 'Fish'
            for species in ['Perch','Roach','Pike','Whitefish','Burbot']:
                for view in range(4):
                    images=[Image.open(folder/f'{image_prefix}_{species}_{view}_{frame}.png').convert('RGB') for frame in [1,2]]
                    # A fixed camera/background must produce different frames
                    # as the ordinary per-instance deformation cycle advances.
                    difference=ImageStat.Stat(ImageChops.difference(*images)).mean
                    assert sum(difference)>.02,(species,view,'no rendered movement')
                    if case['name']=='fins':
                        delta=ImageChops.difference(*images).convert('L')
                        count=lambda box:delta.crop(box).point(lambda x:255 if x>8 else 0).histogram()[255]
                        head=count((250,300,450,600));paired=count((450,350,850,650))
                        assert paired>max(200,head*3),(species,view,'fin change is not distinct from stable-head noise',paired,head)
                        fin_changes.append(dict(species=species,lod=view,paired_fin_changed_pixels=paired,head_changed_pixels=head,threshold_luma=8))
        if case['name']=='fins':assert 'DIVE_FIN_INSPECTION_PASS species=5 views=20 captures=40 rotated=10 fixed_tail=1' in log
        if case['name']=='stream':assert 'DIVE_POPULATION_PASS' in log and 'DIVE_FIN_PAUSE_PASS gpu=1 phases=1 position=1' in log
        if a.steering and case['name']=='stream':assert 'DIVE_STEERING_WORLD_PASS species=5 steps_per_species=60' in log
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
    content=read(ROOT/'artifacts/FishContentBuild.json');assert len(content)==5 and all(x['lods']==4 and x['uv_channels']==3 for x in content)
    importlog=ROOT/'artifacts/fin-content-build.log';text=importlog.read_text(encoding='utf-8-sig')
    assert not re.search(r'FBXImport: Warning|LogPython: Error:|Failed to compile Material',text)
    editorlog=ROOT/'artifacts/fin-editor-inspection.log';text=editorlog.read_text(encoding='utf-8-sig')
    for index in range(5):assert re.search(rf'DIVE_FISH_MESH species={index} .*colors=[1-9]\d+ tinted=[1-9]\d+ alpha=0\.\.255',text)
    for index in range(5):assert re.search(rf'DIVE_FIN_RIG species={index} uv=[34] roots=[1-9]\d+ tips=[1-9]\d+ paired=[1-9]\d+/[1-9]\d+',text)
    core=ROOT/f'artifacts/{prefix}-core.log';core_text=core.read_text(encoding='utf-8-sig');assert 'PASS: fish animation response (0 failures)' in core_text
    if a.steering:assert 'PASS: swept fish steering (0 failures)' in core_text
    perf=a.performance.resolve();summary=read(perf/'Summary.json')
    assert summary['host']['executable_sha256']==sha(exe) and not summary['host']['editor']
    assert summary['host']['package_containers']==host['package_containers']
    assert summary['metadata']['width']==1920 and summary['metadata']['height']==1080 and summary['metadata']['quality']==2
    assert 59<=summary['measured_seconds']<=61
    launcher=read(ROOT/f'artifacts/{prefix}-bootstrap.json');assert launcher['status']=='PASS' and launcher['executable_sha256'].lower()==sha(exe)
    assert sha(ROOT/f'artifacts/{prefix}-bootstrap.log')==launcher['log_sha256'].lower()
    package=ROOT/f'artifacts/{prefix}-package.log';assert 'BUILD SUCCESSFUL' in package.read_text(encoding='utf-8-sig')
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
    previous_report=read(ROOT/'Documentation'/('FIN_MOTION_RESULTS.json' if a.steering else 'FISH_RESULTS.json'))
    baseline=Path(previous_report['executable']['executable']);assert sha(baseline)==previous_report['executable']['sha256'].lower()
    previous=previous_report['executable']['package_containers']
    for item in previous:assert sha(baseline.parents[2]/'Content/Paks'/item['file'])==item['sha256']
    report=dict(status='PASS',executable=host,cases=evidence,isolated_fin_image_changes=fin_changes,sources=sources,assets=assets,mesh_audit=mesh,content_build=content,
        root_launcher=launcher,performance=summary,performance_files={n:sha(perf/n) for n in ['Capture.csv','Metadata.json','Host.json','Runtime.log','Frame.png','Summary.json']},
        import_log_sha256=sha(importlog),editor_color_audit_sha256=sha(editorlog),package_log_sha256=sha(package),core_log_sha256=sha(core),shader_sha256=sha(ROOT/'Tools/fish_motion_shader.hlsl'),
        preserved_previous_executable=str(baseline),preserved_fish_executable_sha256=sha(baseline),preserved_fish_containers=previous,
        limits='Authored fin hinge/wave response, not measured species kinematics or hydrodynamic validation. Isolated image comparison covers paired fins; body/median fins move together in the normal fixture. Meshes/textures are not scans or expert anatomical/photorealistic acceptance. Whitefish remains a group. Close-up fixtures force four LODs at fixed locations; natural AI is checked separately. One development-host performance capture, not minimum-hardware/macOS/endurance certification.')
    if a.steering:report['steering_limits']='Bounded kinematic turns, analytic conservative rock-sphere sweeps and quarter-metre habitat/bathymetry samples; not a full fish-body contact or hydrodynamic solver.'
    if a.steering:
        assert assets==previous_report['assets'],'Steering-only run must preserve the already-verified fin artwork'
        report['unchanged_fin_asset_evidence']='Documentation/FIN_MOTION_RESULTS.json; asset hashes identical. Isolated fin/body fixtures were not rerun for the steering-only change.'
        native=ROOT/'artifacts/steering-editor-stream.log';native_text=native.read_text(encoding='utf-8-sig')
        assert 'DIVE_STEERING_WORLD_PASS species=5 steps_per_species=60' in native_text and 'DIVE_SMOKE_PASS' in native_text
        assert not re.search(r'DIVE_\w+_FAIL',native_text)
        report['native_world_log_sha256']=sha(native)
    (ROOT/'Documentation'/('FISH_STEERING_RESULTS.json' if a.steering else 'FIN_MOTION_RESULTS.json')).write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(prefix.upper()+'_REPORT_PASS cases='+str(len(cases))+' images='+str(sum(x['screenshots'] for x in cases)))
if __name__=='__main__':main()
