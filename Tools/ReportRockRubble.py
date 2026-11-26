"""Verify rubble rendering, stable comparison cameras, collision and package evidence."""
import argparse
import json
import re
from pathlib import Path
import numpy as np
from ReportNativeWaterLighting import ROOT, pixels, record, sha
from ReportRocks import package_check


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('run',type=Path);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--performance',type=Path,nargs='*',default=[])
    p.add_argument('--repeat-underwater',type=Path)
    p.add_argument('--prior-run',type=Path);p.add_argument('--prior-driver',type=Path)
    p.add_argument('--rock-search',action='store_true')
    p.add_argument('--baseline-run',type=Path)
    a=p.parse_args();run=a.run.resolve()
    host=json.loads((run/'Host.json').read_text(encoding='utf-8-sig'))
    for item in host['files']:assert sha(ROOT/item['file'])==item['sha256'],item['file']
    if host['packaged']:package_check(host)
    assert not (host['packaged'] and host['preview'])
    expected={'close-before':3,'close-after':3}
    if not host['preview']:expected.update(weesen=3,murg=3,**{'ledi-wide':3,'raster':3,'stream':2,'shore':9})
    cases=json.loads((run/'Cases.json').read_text(encoding='utf-8-sig'))
    assert len(cases)==len(expected) and {c['name'] for c in cases}==set(expected)
    evidence=[];logs={}
    for case in cases:
        folder=run/case['name'];log=(folder/'Runtime.log').read_text(encoding='utf-8-sig');logs[case['name']]=log
        assert 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:',log)
        checks=[]
        search_checks=[]
        if case['name']!='shore':
            checks=[tuple(map(int,x)) for x in re.findall(r'DIVE_RUBBLE_PASS fragments=(\d+) disabled=(\d+) contacts=(\d+)',log)]
            assert checks
            assert all((n==0 and disabled==1 and contacts==0) if case['disabled'] else (n>0 and disabled==0 and contacts>0)
                       for n,disabled,contacts in checks)
            for marker in ['DIVE_POPULATION_PASS','DIVE_ROCK_LAYOUT_PASS','DIVE_STEERING_WORLD_PASS','DIVE_GRAVEL_PASS']:
                assert marker in log,marker
            if a.rock_search:
                search_checks=[tuple(map(int,x)) for x in re.findall(r'DIVE_ROCK_SEARCH_PASS queries=(\d+) candidates=(\d+) exhaustive=(\d+) rocks=(\d+) exact_force=1 exact_trace=1',log)]
                assert len(search_checks)>=6
                assert all(q>0 and 0<c<e/3 and e==q*n for q,c,e,n in search_checks)
        if case['name']=='raster':assert 'r.Nanite = "0"' in log
        images=sorted(folder.glob('*.png'));assert len(images)==case['screenshots']==expected[case['name']]
        if case['name']=='shore':
            assert {x.name for x in images}=={'DiveSmoke_0_shore.png','DiveUnderwater_0_shore.png',
                'ShoreComplete_0.png','ShoreDive_0.png','ShoreMenu_0.png','ShoreReturn_0.png','ShoreStart_0.png','ShoreSurface_0.png','ShoreWater_0.png'}
        for image in images:pixels(image)
        evidence.append(dict(**case,rubble_checks=checks,search_checks=search_checks,runtime=record(folder/'Runtime.log'),images=[record(x) for x in images],
            engine_error_lines=[s for s in log.splitlines() if ': Error:' in s]))
    poses=[re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)',logs[name]) for name in ['close-before','close-after']]
    assert len(poses[0])==3 and poses[0]==poses[1]
    comparisons=[]
    for i in range(3):
        name=f'Habitat_0_{i}.png';b=run/'close-before'/name;c=run/'close-after'/name
        delta=float(np.abs(pixels(b)-pixels(c)).mean());assert delta>.1
        comparisons.append(dict(before=record(b),after=record(c),mean_absolute_rgb_difference=delta))
    launcher=None;published=[];performance=[]
    if host['packaged']:
        launcher=json.loads((run/'Launcher.json').read_text(encoding='utf-8-sig'))
        assert launcher['status']=='PASS' and launcher['exit_code']==0
        assert launcher['executable_sha256'].lower()==host['sha256'].lower()
        assert launcher['log_sha256'].lower()==sha(run/'Launcher.log')
        published_cases=[('close-after','RockSearch_Close.png')] if a.rock_search else [('close-before','RockRubble_Before.png'),('close-after','RockRubble_After.png')]
        for case,name in published_cases:
            image=ROOT/'Documentation/Images'/name
            assert sha(image)==sha(run/case/'Habitat_0_0.png');published.append(record(image))
        perf_folders=[(folder,'primary') for folder in a.performance]
        if a.repeat_underwater:
            assert len(a.performance)==2
            perf_folders.append((a.repeat_underwater,'repeat-underwater'))
        for folder,role in perf_folders:
            folder=folder.resolve();summary=json.loads((folder/'Summary.json').read_text(encoding='utf-8-sig'))
            h=summary['host'];m=summary['metadata']
            assert not h['editor'] and not h['background_processes']
            assert h['executable_sha256']==host['sha256'].lower()
            assert sorted(h['package_containers'],key=lambda x:x['file'])==sorted(host['package_containers'],key=lambda x:x['file'])
            assert m['width']==1920 and m['height']==1080 and m['quality']==2
            assert m['r.ScreenPercentage']==100 and m['r.VSync']==0 and m['t.MaxFPS']==0
            if role=='repeat-underwater':assert m['scenario']=='perf-underwater'
            performance.append(dict(role=role,summary=summary,files=[record(folder/n) for n in ['Host.json','Metadata.json','Summary.json','Capture.csv','Frame.png','Runtime.log']]))
        if a.performance:
            primary=[x for x in performance if x['role']=='primary']
            assert len(primary)==2 and {x['summary']['metadata']['scenario'] for x in primary}=={'perf-underwater','perf-stream'}
    else:assert not a.performance and not a.repeat_underwater
    prior=None
    if a.prior_run or a.prior_driver:
        assert host['packaged'] and a.prior_run and a.prior_driver
        previous=a.prior_run.resolve();driver=a.prior_driver.resolve()
        previous_host=json.loads((previous/'Host.json').read_text(encoding='utf-8-sig'))
        assert previous_host['sha256']==host['sha256']
        assert previous_host['package_containers']==host['package_containers']
        previous_cases=json.loads((previous/'Cases.json').read_text(encoding='utf-8-sig'))
        assert len(previous_cases)==7 and {c['name'] for c in previous_cases}==set(expected)-{'shore'}
        driver_text=driver.read_text(encoding='utf-8-sig')
        assert set(re.findall(r'^PASS (\S+) ',driver_text,re.M))==set(expected)-{'shore'}
        assert 'ROCK_RUBBLE=' not in driver_text
        assert 'DIVE_SMOKE_PASS shore=1' in (previous/'shore/Runtime.log').read_text(encoding='utf-8-sig')
        prior=dict(status='COLLECTOR_FAILED',reason='The original collector expected six shore images; the actual scenario correctly produced nine.',
            failure_message_capture='The terminating PowerShell error was returned to the shell session, not written to the redirected driver log; that log retains only seven completed cases.',
            host=record(previous/'Host.json'),cases=record(previous/'Cases.json'),driver=record(driver),
            shore_runtime=record(previous/'shore/Runtime.log'),shore_images=[record(x) for x in sorted((previous/'shore').glob('*.png'))])
        assert len(prior['shore_images'])==9
    baseline=None
    if a.rock_search:
        assert a.baseline_run
        old=a.baseline_run.resolve();old_host=json.loads((old/'Host.json').read_text(encoding='utf-8-sig'))
        assert old_host['packaged']==host['packaged']
        before_log=(old/'close-after/Runtime.log').read_text(encoding='utf-8-sig')
        assert re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)',before_log)==poses[1]
        old_assets={x['file']:x['sha256'] for x in old_host['files'] if '/Content/' in x['file']}
        assets={x['file']:x['sha256'] for x in host['files'] if '/Content/' in x['file']}
        assert old_assets and old_assets==assets
        baseline=dict(host=record(old/'Host.json'),runtime=record(old/'close-after/Runtime.log'),same_cameras=True,same_rock_assets=True,
            images=[record(old/'close-after'/f'Habitat_0_{i}.png') for i in range(3)])
    else:assert not a.baseline_run
    result=dict(status='PASS',scope='Conservative rock queries; not full environment acceptance' if a.rock_search else 'Authored rock-foot fragments; not full environment acceptance',host=host,prior_attempt=prior,baseline=baseline,
        cases=evidence,screenshot_count=sum(expected.values()),comparisons=comparisons,launcher=launcher,published=published,performance=performance,
        limitations=['Fragments are authored dressing, not surveyed Walensee geology',
                    'Collision probes sample individual fragment bodies; not every contact direction',
                    'Additional rock footprints also affect local plants, gravel and fish paths',
                    'Short rendering and performance scenarios are not endurance acceptance',
                    'Plant quality, sediment transitions and lighting still require work'])
    a.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'PASS {len(cases)} cases / {sum(expected.values())} captures -> {a.output}')


if __name__=='__main__':main()
