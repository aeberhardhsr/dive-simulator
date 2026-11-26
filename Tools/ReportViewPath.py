"""Bind view-path rendering, visibility/night checks and performance to a package."""
import argparse
import json
import re
import subprocess
import numpy as np
from PIL import Image
from ReportRocks import ROOT, read, sha, package_check
from ReportBenthic import record, logcheck
from validate_lighting_frames import scene
from ReportCaustics import report as caustics_report


def pixels(path):
    with Image.open(path) as im:
        assert im.size==(1600,900)
        return np.asarray(im.convert('RGB'),dtype=float)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run',type=lambda p:(ROOT/p).resolve())
    parser.add_argument('--performance',required=True,type=lambda p:(ROOT/p).resolve())
    args=parser.parse_args();run=args.run
    host=read(run/'Host.json');package_check(host)
    launcher=read(run/'Launcher.json')
    assert launcher['status']=='PASS' and launcher['exit_code']==0
    assert launcher['executable_sha256'].lower()==host['sha256'].lower()
    assert launcher['log_sha256'].lower()==sha(run/'Launcher.log')
    caustics_path=(run/'CausticsRun.txt').read_text(encoding='utf-8-sig').strip()
    assert caustics_path.startswith('DIVE_CAUSTICS_VALIDATION=')
    caustics=caustics_report(ROOT/caustics_path.split('=',1)[1])
    assert caustics['host']['executable_sha256'].lower()==host['sha256'].lower()
    cases=read(run/'Cases.json')
    expected={'habitat0','habitat1','habitat2','murky','clear','look-day','look-night',
        'surface0','surface1','surface2','storm','night-on','night-off','dark-on','dark-off'}
    assert len(cases)==len(expected) and {c['name'] for c in cases}==expected
    evidence=[]
    for case in cases:
        folder=run/case['name'];log=logcheck(folder/'Runtime.log')
        assert case['status']=='PASS' and 'DIVE_VIEW_PATH_PASS metric=world_ray exit=local_water_level tint=neutral' in log
        if case['scenario'].startswith('surface'):
            assert 'surface=1' in log
            for phase in ['water','transition','air']: assert f'DIVE_MEDIUM_PASS phase={phase}' in log
        elif case['name'].startswith('look-'):
            assert 'DIVE_WATER_INSPECTION_PASS final_state=1' in log
            assert {p.name for p in folder.glob('*.png')}=={f'Water_1_{v}.png' for v in range(3)}
        elif case['scenario']=='rocks':
            assert 'DIVE_HABITAT_INSPECTION_PASS views=3' in log
            assert {p.name for p in folder.glob('*.png')}=={f'Habitat_0_{v}.png' for v in range(3)}
        elif case['scenario']=='torch':
            assert 'DIVE_TORCH_TARGET hit=1' in log
            assert f"DIVE_TORCH_PASS on={int(case['name'].endswith('-on'))}" in log
            if case['name'].startswith('dark-'):
                elevation=float(re.search(r'DIVE_LIGHT_PASS elevation=([-\d.]+)',log)[1])
                assert elevation < -18, 'Strict darkness requires astronomical night'
        if case['name']=='storm': assert 'DIVE_WEATHER_PASS kind=3 wind=22.0' in log
        images=[]
        for path in sorted(folder.glob('*.png')):
            pixels(path);images.append(record(path))
        assert len(images)==case['screenshots'] and images
        evidence.append(dict(**case,log=record(folder/'Runtime.log'),images=images,
            engine_error_lines=[line for line in log.splitlines() if ': Error:' in line]))
    on=scene(run/'dark-on/DiveUnderwater_0_torch.png')
    off=scene(run/'dark-off/DiveUnderwater_0_torch.png')
    night=dict(off_p999=float(np.quantile(off,.999)),lit_pixels=int(((on>20)&(off<3)).sum()),
        mean_difference=float((on-off).mean()))
    assert night['off_p999']<=2 and night['lit_pixels']>5000 and night['mean_difference']>1
    twilight_off=scene(run/'night-off/DiveUnderwater_0_torch.png')
    previous_twilight=scene(ROOT/'artifacts/viewpath-previous-midnight.png')
    twilight_elevation=float(re.search(r'DIVE_LIGHT_PASS elevation=([-\d.]+)',logcheck(run/'night-off/Runtime.log'))[1])
    assert -18 < twilight_elevation < -12
    twilight=dict(hour=0,sun_elevation=twilight_elevation,off_p999=float(np.quantile(twilight_off,.999)),
        off_mean=float(twilight_off.mean()),previous_off_p999=float(np.quantile(previous_twilight,.999)),
        previous_off_mean=float(previous_twilight.mean()),previous_image=record(ROOT/'artifacts/viewpath-previous-midnight.png'),
        previous_log=record(ROOT/'artifacts/viewpath-previous-midnight.log'),
        note='The initial <=2 darkness check fails at summer midnight (astronomical twilight). Retained as a separate observation; strict darkness is checked at 01:30 with solar elevation below -18 degrees.')
    previous=ROOT/'artifacts/plant-canopy-validation-20261126-162555'
    old_host=read(previous/'Host.json');package_check(old_host)
    before=previous/'entry2/Habitat_0_2.png';after=run/'habitat2/Habitat_0_2.png'
    old_pose=re.findall(r'DIVE_HABITAT_VIEW view=2[^\r\n]*',logcheck(previous/'entry2/Runtime.log'))[-1]
    new_pose=re.findall(r'DIVE_HABITAT_VIEW view=2[^\r\n]*',logcheck(run/'habitat2/Runtime.log'))[-1]
    assert old_pose==new_pose
    old_rgb=pixels(before)[600:850,50:550].mean(axis=(0,1))
    new_rgb=pixels(after)[600:850,50:550].mean(axis=(0,1))
    ratio_change=(new_rgb[0]/new_rgb[1])/(old_rgb[0]/old_rgb[1])
    assert ratio_change>1.05, 'Near-surface camera tint did not visibly decrease'
    assert sha(before)==sha(ROOT/'Documentation/Images/ViewPath_Before.png')
    assert sha(after)==sha(ROOT/'Documentation/Images/ViewPath_After.png')
    # Same terrain/camera in both visibility cases; retain measured contrast,
    # without treating this scene-dependent display statistic as optical calibration.
    visibility={}
    for name in ['murky','clear']:
        rgb=pixels(run/name/'Habitat_0_2.png')[180:450,650:1150]
        luminance=rgb@np.array([.2126,.7152,.0722])
        visibility[name]=dict(mean=float(luminance.mean()),std=float(luminance.std()))
    assert visibility['clear']['std']>visibility['murky']['std'], 'Clear view did not retain more distant contrast'
    paths=[ROOT/p for p in ['Tools/Shaders/UnderwaterView.ush','Tools/build_optics_content.py',
        'divingsim/Source/DiveSim/DivePawn.cpp','divingsim/Source/DiveSim/DivePawn.h',
        'divingsim/Source/DiveSim/DiveWorld.cpp','divingsim/Source/DiveSim/DiveWorld.h',
        'divingsim/Source/DiveSim/DiveRockInspection.cpp','divingsim/Content/Diving/Materials/M_Underwater.uasset']]
    for path in paths:
        committed=subprocess.run(['git','show',f"{host['source_head']}:{path.relative_to(ROOT).as_posix()}"],cwd=ROOT,check=True,capture_output=True).stdout
        if path.suffix=='.uasset': assert f'oid sha256:{sha(path)}'.encode() in committed
        else: assert path.read_bytes().replace(b'\r\n',b'\n')==committed.replace(b'\r\n',b'\n')
    perf=args.performance;summary=read(perf/'Summary.json')
    assert summary['host']['executable_sha256'].lower()==host['sha256'].lower()
    assert summary['host']['package_containers']==host['package_containers']
    assert not summary['host']['background_processes'] and not summary['host']['editor']
    assert summary['metadata']['scenario']=='perf-underwater'
    build=ROOT/'artifacts/viewpath-package-build.log'
    assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
    report=dict(status='PASS',host=host,launcher=launcher,cases=evidence,inputs=[record(p) for p in paths],build=record(build),
        comparison=dict(previous_host=old_host,pose=new_pose,before=record(before),after=record(after),
            roi_xyxy=[50,600,550,850],before_mean_rgb=old_rgb.tolist(),after_mean_rgb=new_rgb.tolist(),red_green_ratio_change=float(ratio_change)),
        night=night,summer_midnight=twilight,visibility=dict(roi_xyxy=[650,180,1150,450],cases=visibility),caustics=caustics,
        performance=dict(summary=summary,files=[record(perf/n) for n in ['Host.json','Metadata.json','Summary.json','Runtime.log','Capture.csv','Frame.png']]),
        limitations=['Authored homogeneous water medium, not a measured radiative-transfer model.',
            'Local water-level clipping does not trace a complete refracted ray through moving waves.',
            'Display-space image statistics verify changes, not photorealistic or photometric acceptance.',
            'Reference visual quality, endurance, all viewpoints and native macOS acceptance remain open.'])
    output=ROOT/'Documentation/VIEW_PATH_RESULTS.json'
    output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(dict(status='PASS',cases=len(cases)+1,screenshots=sum(c['screenshots'] for c in cases)+4,report=str(output))))


if __name__=='__main__':main()
