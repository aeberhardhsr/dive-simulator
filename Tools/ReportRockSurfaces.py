"""Bind actual scan-material captures to native or packaged build inputs."""
import argparse
import json
import re
from pathlib import Path
import numpy as np
from ReportNativeWaterLighting import ROOT, pixels, record, sha
from ReportRocks import package_check


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('run',type=Path);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--performance',type=Path,nargs='*',default=[])
    a=p.parse_args();run=a.run.resolve()
    host=json.loads((run/'Host.json').read_text(encoding='utf-8-sig'))
    for item in host['files']:assert sha(ROOT/item['file'])==item['sha256'],item['file']
    if host['packaged']:package_check(host)
    cases=json.loads((run/'Cases.json').read_text(encoding='utf-8-sig'))
    expected={'forms-legacy','forms-scan','forms-raster','habitat-legacy','habitat-scan','habitat-default','stream','torch'}
    if not host['packaged']:expected.add('unlit-scan')
    assert len(cases)==len(expected) and {c['name'] for c in cases}==expected
    evidence=[];logs={}
    for c in cases:
        folder=run/c['name'];log=(folder/'Runtime.log').read_text(encoding='utf-8-sig');logs[c['name']]=log
        assert 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:',log)
        for i in range(1,4):
            material='M_RockSurface' if c['legacy'] else f'M_RockScan{i}'
            assert re.search(rf'DIVE_ROCK_SURFACE_READY variant={i} material=/Game/Diving/Materials/{material}\.',log)
        if c['raster']:assert 'r.Nanite = "0"' in log
        if c['bounded']:assert 'DIVE_BOUNDED_WATER_PASS native=1' in log
        if c['name'].startswith('habitat') or c['name']=='stream':assert 'DIVE_POPULATION_PASS' in log
        if c['name']=='stream':assert 'DIVE_STEERING_WORLD_PASS species=5' in log
        if c['name']=='unlit-scan':assert 'ShowFlag.PostProcessing = "0"' in log
        images=sorted(folder.glob('*.png'));assert len(images)==c['screenshots']
        for image in images:pixels(image)
        evidence.append(dict(**c,runtime=record(folder/'Runtime.log'),images=[record(x) for x in images],engine_error_lines=[s for s in log.splitlines() if ': Error:' in s]))
    # Same fixture, same geometry and camera; only the surface selection differs.
    poses=lambda name:re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)',logs[name])
    assert len(poses('habitat-legacy'))==3 and poses('habitat-legacy')==poses('habitat-scan')
    differences=[]
    for i in range(1,4):
        for view in range(2):
            image=f'Rock_{i}_{view}.png'
            before=pixels(run/'forms-legacy'/image);after=pixels(run/'forms-scan'/image)
            diff=float(np.abs(before-after).mean());assert diff>.1
            differences.append(dict(variant=i,view=view,mean_absolute_rgb_difference=diff))
    materials=json.loads((ROOT/'Documentation/ROCK_SURFACES.json').read_text())
    for source in materials['assets']:
        for item in source['maps'].values():assert sha(ROOT/item['file'])==item['sha256']
    content=json.loads((ROOT/'artifacts/RockSurfaceContentBuild.json').read_text())
    assert len(content)==3 and all(x['uv_channels']>=1 for x in content)
    published=[]
    launcher=None;performance=[]
    if host['packaged']:
        for case,name in [('forms-legacy','RockSurface_Before.png'),('forms-scan','RockSurface_After.png')]:
            path=ROOT/'Documentation/Images'/name
            assert sha(path)==sha(run/case/'Rock_1_0.png')
            published.append(record(path))
        if (run/'Launcher.json').exists():
            launcher=json.loads((run/'Launcher.json').read_text(encoding='utf-8-sig'))
            assert launcher['status']=='PASS' and launcher['exit_code']==0
            assert launcher['executable_sha256'].lower()==host['sha256'].lower()
            assert launcher['log_sha256'].lower()==sha(run/'Launcher.log')
        for folder in a.performance:
            folder=folder.resolve()
            summary=json.loads((folder/'Summary.json').read_text(encoding='utf-8-sig'))
            h=summary['host'];m=summary['metadata']
            assert not h['editor'] and not h['background_processes']
            assert h['executable_sha256']==host['sha256'].lower()
            assert sorted(h['package_containers'],key=lambda x:x['file'])==sorted(host['package_containers'],key=lambda x:x['file'])
            assert m['width']==1920 and m['height']==1080 and m['quality']==2
            assert m['r.ScreenPercentage']==100 and m['r.VSync']==0 and m['t.MaxFPS']==0
            performance.append(dict(summary=summary,files=[record(folder/name) for name in ['Host.json','Metadata.json','Summary.json','Capture.csv','Frame.png','Runtime.log']]))
        if a.performance:assert {x['summary']['metadata']['scenario'] for x in performance}=={'perf-underwater','perf-stream'} and len(performance)==2
    result=dict(status='PASS',scope='Matching scanned rock surfaces, rendering and streaming; not overall visual acceptance',host=host,cases=evidence,
        screenshot_count=sum(c['screenshots'] for c in cases),comparisons=differences,published=published,content_audit=content,content_audit_file=record(ROOT/'artifacts/RockSurfaceContentBuild.json'),
        source_manifest=record(ROOT/'Documentation/ROCK_SURFACES.json'),launcher=launcher,performance=performance,limitations=[
            'Source boulders are not Walensee geological surveys; colour desaturation and wet coatings are authored',
            'Two-kilopixel source maps retain ordinary texture streaming and mip filtering',
            'No mesh, collision or terrain reshaping; spatial texture scale follows the existing instance scale',
            'Bounded water remains a separate experimental smoke-test mode',
            'Any attached benchmarks are short scenario measurements, not sustained frame-rate or endurance acceptance',
            'Full shoreline quality and requested overall reference fidelity remain incomplete'])
    a.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'PASS {len(cases)} cases / {result["screenshot_count"]} captures -> {a.output}')


if __name__=='__main__':main()
