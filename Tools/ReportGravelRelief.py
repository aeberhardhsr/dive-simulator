"""Bind dense near-field gravel comparisons to their runtime and package evidence."""
import argparse
import csv
import json
import re
from pathlib import Path

import numpy as np
from ReportNativeWaterLighting import ROOT, pixels, record, sha
from ReportRocks import package_check
from analyze_performance import analyze, timing


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def phases(folder):
    with (folder/'Capture.csv').open(encoding='utf-8-sig',newline='') as stream:
        rows=list(csv.reader(stream))
    header=rows[-2]
    result={}
    for name in ['DiveGravel/GameThread/Stream','DiveGravel/GameThread/BuildCell','DiveGravel/GameThread/Register','DiveGravel/GameThread/AddInstances','DiveGravel/GameThread/Retire',
                 'Exclusive/GameThread/EventWait/EndPhysics']:
        index=header.index(name)
        values=[float(row[index]) if index<len(row) else 0. for row in rows[2:-3]]
        result[name]=dict(**timing(values),nonzero_frames=sum(v>0 for v in values))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--benchmark', type=Path, action='append', default=[])
    parser.add_argument('--retirement', action='store_true')
    parser.add_argument('--control-benchmark', type=Path)
    args = parser.parse_args()
    run = args.run.resolve()
    host = read(run/'Host.json')
    assert not args.control_benchmark or (args.retirement and host['packaged'])
    for item in host['files']:
        assert sha(ROOT/item['file']) == item['sha256'], item['file']
    if host['packaged']:
        package_check(host)
        assert not host['preview']
    expected = {'close-before':3, 'close-after':3}
    if not host['preview']:
        expected.update(weesen=3, murg=3, **{'ledi-wide':3, 'raster':3, 'stream':2, 'shore':9, 'sediment':7})
    cases = read(run/'Cases.json')
    assert len(cases) == len(expected) and {c['name'] for c in cases} == set(expected)
    logs, evidence, counts = {}, [], {}
    for case in cases:
        name = case['name']
        folder = run/name
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        logs[name] = log
        assert case['status'] == 'PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:', log)
        assert 'DIVE_WATER_PREVIEW_READY smoke=1 hardware=1 bounded=1 interface=1 lighting_mode=2' in log
        assert ('-DiveSparseGravel' in log) == (name == 'close-before')
        checks = [dict(retained=int(a), reloaded=int(b), cells=int(c), stones=int(d), contacts=int(e), edge_stones=int(f))
            for a,b,c,d,e,f in re.findall(r'DIVE_GRAVEL_PASS retained=(\d+) reloaded=(\d+) cells=(\d+) stones=(\d+) contacts=(\d+) staged=7,8,9 edge_stones=(\d+)', log)]
        retirement_checks=[]
        if name not in {'shore', 'sediment'}:
            assert checks
            assert all(x['retained']==6 and x['reloaded']==9 and x['cells']==9 and
                0<x['stones']<=9*8192 and x['contacts']>0 for x in checks)
            for marker in ['DIVE_POPULATION_PASS', 'DIVE_ROCK_LAYOUT_PASS', 'DIVE_STEERING_WORLD_PASS']:
                assert marker in log
            counts[name] = checks[0]['stones']
            if args.retirement:
                retirement_checks=re.findall(r'DIVE_GRAVEL_RETIREMENT_PASS maximum_pending=(\d+) drained=1 immediate=0',log)
                assert retirement_checks and all(0<int(n)<=27 for n in retirement_checks)
        if name == 'raster':
            assert 'r.Nanite = "0"' in log
        loads = [dict(x=int(x),y=int(y),loaded=int(a),unloaded=int(b),active=int(c),stones=int(d),milliseconds=float(t))
            for x,y,a,b,c,d,t in re.findall(r'DIVE_GRAVEL_CELL x=(-?\d+) y=(-?\d+) loaded=(\d+) unloaded=(\d+) active=(\d+) stones=(\d+) milliseconds=([\d.]+)', log)]
        assert loads and all(0<x['active']<=9 and x['stones']<=9*8192 for x in loads)
        images = sorted(folder.glob('*.png'))
        assert len(images) == case['screenshots'] == expected[name]
        for image in images:
            pixels(image)
        evidence.append(dict(**case, checks=checks, retirement_maximum_pending=[int(n) for n in retirement_checks], loads=loads, runtime=record(folder/'Runtime.log'),
            images=[record(x) for x in images], engine_errors=[s for s in log.splitlines() if ': Error:' in s]))
    poses = [re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)',logs[n]) for n in ['close-before','close-after']]
    assert len(poses[0])==3 and poses[0]==poses[1]
    assert 6<counts['close-after']/counts['close-before']<10
    comparisons = []
    for i in range(3):
        before,after = [run/n/f'Habitat_0_{i}.png' for n in ['close-before','close-after']]
        difference = float(np.abs(pixels(before)-pixels(after)).mean())
        assert difference>.1
        comparisons.append(dict(before=record(before),after=record(after),mean_absolute_rgb_difference=difference))
    benchmarks, published = [], []
    launcher = None
    if host['packaged']:
        launcher = read(run/'launcher/Launcher.json')
        assert launcher['status']=='PASS' and launcher['exit_code']==0
        assert launcher['executable_sha256'].lower()==host['sha256'].lower()
        assert launcher['log_sha256'].lower()==sha(run/'launcher/Launcher.log')
        for name,label in [('close-before','Before'),('close-after','After')]:
            prefix='GravelRetirement' if args.retirement else 'GravelRelief'
            path=ROOT/f'Documentation/Images/{prefix}_{label}.png'
            assert sha(path)==sha(run/name/'Habitat_0_0.png')
            published.append(record(path))
        for folder in args.benchmark:
            folder=folder.resolve()
            result=analyze(folder)
            h,m=result['host'],result['metadata']
            assert h['executable_sha256']==host['sha256'].lower() and h['package_containers']==host['package_containers']
            assert not h['background_processes'] and not h['editor'] and h['water_preview']
            for key,value in {'width':1920,'height':1080,'quality':2,'r.ScreenPercentage':100,'r.VSync':0,'t.MaxFPS':0,
                'r.VolumetricFog.DepthDistributionScale':8,'r.VolumetricFog':1,'r.Lumen.HardwareRayTracing':1}.items():
                assert m[key]==value,(key,m[key])
            assert all(m[k] for k in ['water_preview','water_interface','bounded_water','native_scattering'])
            if args.retirement:
                assert not m['gravel_immediate_retirement'] and not h['gravel_immediate_retirement']
            benchmarks.append(dict(result=result,files=[record(folder/n) for n in ['Capture.csv','Metadata.json','Host.json','Frame.png','Runtime.log']]))
        assert len(benchmarks)==2 and {b['result']['metadata']['scenario'] for b in benchmarks}=={'perf-underwater','perf-stream'}
    else:
        assert not args.benchmark
    build = None
    retirement = None
    if host['packaged']:
        build_path = ROOT/('artifacts/gravel-retirement-package-build.log' if args.retirement else 'artifacts/gravel-relief-package-sandbox-build.log')
        assert 'BUILD SUCCESSFUL' in build_path.read_text(encoding='utf-8-sig')
        build = record(build_path)
        if args.retirement:
            assert args.control_benchmark
            folder=args.control_benchmark.resolve()
            control=analyze(folder)
            normal=next(b['result'] for b in benchmarks if b['result']['metadata']['scenario']=='perf-stream')
            h,m=control['host'],control['metadata']
            assert h['executable_sha256']==host['sha256'].lower() and h['package_containers']==host['package_containers']
            assert not h['editor'] and not h['background_processes']
            assert h['gravel_immediate_retirement'] and m['gravel_immediate_retirement']
            for key in ['scenario','width','height','quality','requested_capture_seconds','r.ScreenPercentage','r.VSync','t.MaxFPS',
                        'water_preview','water_interface','bounded_water','native_scattering','r.Lumen.HardwareRayTracing',
                        'r.Lumen.HardwareRayTracing.LightingMode','r.VolumetricFog.DepthDistributionScale']:
                assert m[key]==normal['metadata'][key],key
            normal_folder=next(p.resolve() for p in args.benchmark if read(p/'Metadata.json')['scenario']=='perf-stream')
            retirement=dict(control=control,control_phases=phases(folder),staged_phases=phases(normal_folder),
                files=[record(folder/n) for n in ['Capture.csv','Metadata.json','Host.json','Frame.png','Runtime.log']],
                interpretation='Single-run same-executable control; compare CPU waits and complete frame-time distributions, not just mean FPS')
    result=dict(status='PASS', scope=('Staged gravel-body retirement, bounded lifetime, preserved rendering and same-executable performance control; not full performance acceptance' if args.retirement else 'Dense near-field gravel rendering, deterministic streaming and collision samples; not reference-quality acceptance'),
        retirement_enabled=args.retirement,
        host=host,cases=evidence,screenshot_count=sum(expected.values()),comparison_poses=poses[0],comparisons=comparisons,
        launcher=launcher,benchmarks=benchmarks,published=published,build=build,retirement=retirement,
        limitations=['Sparse comparison uses the new 16 m cell grid at the old candidate density, not the exact old 32 m layout',
            'Gravel is authored placement, not surveyed individual stones',
            'Collision probes are samples, not exhaustive contact coverage',
            'Short measurements do not prove sustained performance or endurance',
            'Overall environment fidelity and later menu/HUD redesign remain unfinished'])
    args.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'PASS gravel relief: {len(cases)} cases, {sum(expected.values())} captures')


if __name__=='__main__':
    main()
