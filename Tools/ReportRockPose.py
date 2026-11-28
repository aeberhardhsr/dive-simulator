"""Bind rock-pose comparisons, contact samples and performance to their build."""
import argparse
import json
import re
from pathlib import Path
import numpy as np
from ReportNativeWaterLighting import ROOT,pixels,record,sha
from ReportRocks import package_check
from analyze_performance import analyze


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('run',type=Path);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--benchmark',type=Path,action='append',default=[])
    p.add_argument('--control-benchmark',type=Path)
    a=p.parse_args();run=a.run.resolve();host=read(run/'Host.json')
    for item in host['files']:assert sha(ROOT/item['file'])==item['sha256'],item['file']
    if host['packaged']:package_check(host)
    expected={'close-before':3,'close-after':3}
    if not host['preview']:expected.update(weesen=3,murg=3,**{'ledi-wide':3,'raster':3,'stream':2,'shore':9,'sediment':7})
    cases=read(run/'Cases.json');assert len(cases)==len(expected) and {c['name'] for c in cases}==set(expected)
    logs={};evidence=[]
    for case in cases:
        name=case['name'];folder=run/name;log=(folder/'Runtime.log').read_text(encoding='utf-8-sig');logs[name]=log
        assert case['status']=='PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:',log)
        assert 'DIVE_WATER_PREVIEW_READY smoke=1 hardware=1 bounded=1 interface=1 lighting_mode=2' in log
        assert ('-DiveLegacyRockPose' in log)==(name=='close-before')
        checks=[tuple(map(int,x)) for x in re.findall(r'DIVE_ROCK_POSE_PASS legacy=(\d+) rocks=(\d+) envelope_preserved=1 probes=(\d+) contacts=(\d+) embedded=(\d+)',log)]
        states=re.findall(r'DIVE_ROCK_POSE_STATE sha1=([0-9A-F]{40})',log)
        if name not in {'shore','sediment'}:
            assert checks and len(states)==len(checks)
            assert all(old==int(name=='close-before') and rocks>0 and probes>0 and probes==contacts==buried for old,rocks,probes,contacts,buried in checks)
            for marker in ['DIVE_POPULATION_PASS','DIVE_ROCK_LAYOUT_PASS','DIVE_GRAVEL_PASS','DIVE_STEERING_WORLD_PASS',
                           'DIVE_POPULATION_RETIREMENT_PASS','DIVE_POPULATION_NEAR_TELEPORT_PASS']:
                assert marker in log,marker
        if name=='raster':assert 'r.Nanite = "0"' in log
        images=sorted(folder.glob('*.png'));assert len(images)==case['screenshots']==expected[name]
        for path in images:pixels(path)
        evidence.append(dict(**case,pose_checks=checks,state_hashes=states,runtime=record(folder/'Runtime.log'),
                             images=[record(x) for x in images],engine_errors=[s for s in log.splitlines() if ': Error:' in s]))
    pose=[re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)',logs[n]) for n in ['close-before','close-after']]
    assert len(pose[0])==3 and pose[0]==pose[1]
    states=[re.findall(r'DIVE_ROCK_POSE_STATE sha1=([0-9A-F]{40})',logs[n]) for n in ['close-before','close-after']]
    assert states[0] and states[0]==states[1]
    comparisons=[]
    for i in range(3):
        before=run/'close-before'/f'Habitat_0_{i}.png';after=run/'close-after'/f'Habitat_0_{i}.png'
        difference=float(np.abs(pixels(before)-pixels(after)).mean());assert difference>.1
        comparisons.append(dict(before=record(before),after=record(after),mean_absolute_rgb_difference=difference))
    launcher=None;benchmarks=[];control=None;published=[];build=None
    if host['packaged']:
        build_path=ROOT/'artifacts/rock-pose-package-build.log'
        assert 'BUILD SUCCESSFUL' in build_path.read_text(encoding='utf-8-sig')
        build=record(build_path)
        launcher=read(run/'launcher/Launcher.json');assert launcher['status']=='PASS' and launcher['exit_code']==0
        assert launcher['executable_sha256'].lower()==host['sha256'].lower()
        assert launcher['log_sha256'].lower()==sha(run/'launcher/Launcher.log')
        for case,label,view in [('close-before','Before',0),('close-after','After',0),('weesen','Wide',1)]:
            path=ROOT/f'Documentation/Images/RockPose_{label}.png'
            assert sha(path)==sha(run/case/f'Habitat_0_{view}.png');published.append(record(path))
        assert len(a.benchmark)==2 and a.control_benchmark
        def benchmark(folder,legacy):
            folder=folder.resolve();result=analyze(folder);h,m=result['host'],result['metadata']
            assert not h['editor'] and not h['background_processes'] and h['water_preview']
            assert h['executable_sha256']==host['sha256'].lower() and h['package_containers']==host['package_containers']
            assert h['legacy_rock_pose']==m['legacy_rock_pose']==legacy
            for key,value in {'width':1920,'height':1080,'quality':2,'r.ScreenPercentage':100,'r.VSync':0,'t.MaxFPS':0,
                              'r.VolumetricFog.DepthDistributionScale':8,'r.Lumen.HardwareRayTracing':1,
                              'gravel_immediate_retirement':False,'population_immediate_retirement':False}.items():
                assert m[key]==value,(key,m[key])
            assert all(m[k] for k in ['water_preview','bounded_water','native_scattering','water_interface'])
            return dict(result=result,files=[record(folder/n) for n in ['Capture.csv','Metadata.json','Host.json','Frame.png','Runtime.log']])
        benchmarks=[benchmark(folder,False) for folder in a.benchmark]
        assert {b['result']['metadata']['scenario'] for b in benchmarks}=={'perf-underwater','perf-stream'}
        control=benchmark(a.control_benchmark,True)
        normal=next(b for b in benchmarks if b['result']['metadata']['scenario']=='perf-underwater')
        assert control['result']['metadata']['scenario']=='perf-underwater'
        assert control['result']['metadata']['requested_capture_seconds']==normal['result']['metadata']['requested_capture_seconds']
    else:assert not a.benchmark and not a.control_benchmark
    result=dict(status='PASS',scope='Rock silhouettes, preserved placement/envelopes, sampled contacts and package regression; not full visual acceptance',
                host=host,cases=evidence,comparisons=comparisons,comparison_poses=pose[0],placement_hashes=states[0],
                launcher=launcher,benchmarks=benchmarks,control=control,published=published,build=build,
                limitations=['Authored rock placement and pose, not surveyed local geology',
                             'Collision probes sample convex hulls, not every triangle or direction',
                             'Different shapes can change visibility, contacts and GPU workload despite identical proxy envelopes',
                             'Short single-run measurements are not endurance or minimum-hardware acceptance',
                             'Sediment geometry, plants, lighting and the overall reference target remain unfinished'])
    a.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'ROCK_POSE_PASS cases={len(cases)} captures={sum(expected.values())}')


if __name__=='__main__':main()
