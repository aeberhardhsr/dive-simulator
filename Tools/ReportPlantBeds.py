"""Bind plant-bed comparisons, placement checks and measured package evidence."""
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
    p.add_argument('--performance',type=Path,nargs=2)
    p.add_argument('--publish-view',type=int,choices=range(3),default=0)
    p.add_argument('--prior-run',type=Path)
    p.add_argument('--incomplete-run',type=Path)
    p.add_argument('--slow-start',type=Path)
    a=p.parse_args();run=a.run.resolve()
    host=json.loads((run/'Host.json').read_text(encoding='utf-8-sig'))
    for item in host['files']:assert sha(ROOT/item['file'])==item['sha256'],item['file']
    if host['packaged']:package_check(host)
    assert not (host['packaged'] and host['preview'])
    expected={'before':3,'after':3}
    if not host['preview']:expected.update(murg=3,ledi=3,raster=3,stream=2,shore=9,discoveries=None)
    cases=json.loads((run/'Cases.json').read_text(encoding='utf-8-sig'))
    assert len(cases)==len(expected) and {c['name'] for c in cases}==set(expected)
    evidence=[];logs={}
    for case in cases:
        folder=run/case['name'];log=(folder/'Runtime.log').read_text(encoding='utf-8-sig');logs[case['name']]=log
        assert 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:',log)
        checks=[]
        if case['name'] not in {'shore','discoveries'}:
            checks=[tuple(map(int,x)) for x in re.findall(r'DIVE_PLANT_BEDS_PASS legacy=(\d+) plants=(\d+) short=(\d+) tall=(\d+) unique_roots=(\d+)',log)]
            assert len(checks)==2
            assert all(bool(legacy)==case['legacy'] and n==unique and 0<=short<=n and 0<=tall<=n for legacy,n,short,tall,unique in checks)
            for marker in ['DIVE_POPULATION_PASS','DIVE_ROCK_LAYOUT_PASS','DIVE_ROCK_SEARCH_PASS','DIVE_STEERING_WORLD_PASS','DIVE_GRAVEL_PASS']:
                assert marker in log,marker
        if case['name']=='after':assert all(n>0 and short>0 and tall>0 for legacy,n,short,tall,unique in checks)
        if case['name']=='raster':assert 'r.Nanite = "0"' in log
        images=sorted(folder.glob('*.png'));assert len(images)==case['screenshots']
        if expected[case['name']] is not None:assert len(images)==expected[case['name']]
        if case['name']=='discoveries':
            required={f'Discoveries_0_{x}.png' for x in ['Gallery_DE','Species_DE','Species_EN','Logbook','Plant_EN']}
            names={x.name for x in images}
            assert required<=names<=required|{'DiveSmoke_0_discoveries.png','DiveUnderwater_0_discoveries.png'}
            assert 'DIVE_DISCOVERY_LOG' in log
        for path in images:pixels(path)
        evidence.append(dict(**case,plant_checks=checks,runtime=record(folder/'Runtime.log'),images=[record(x) for x in images],
            engine_error_lines=[s for s in log.splitlines() if ': Error:' in s]))
    poses=[re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)',logs[name]) for name in ['before','after']]
    assert len(poses[0])==3 and poses[0]==poses[1]
    comparisons=[]
    for i in range(3):
        b=run/'before'/f'Habitat_0_{i}.png';c=run/'after'/f'Habitat_0_{i}.png'
        delta=float(np.abs(pixels(b)-pixels(c)).mean());assert delta>.1
        comparisons.append(dict(before=record(b),after=record(c),mean_absolute_rgb_difference=delta))
    launcher=None;published=[];performance=[]
    if host['packaged']:
        launcher=json.loads((run/'Launcher.json').read_text(encoding='utf-8-sig'))
        assert launcher['status']=='PASS' and launcher['exit_code']==0
        assert launcher['executable_sha256'].lower()==host['sha256'].lower()
        assert launcher['log_sha256'].lower()==sha(run/'Launcher.log')
        for case,name in [('before','PlantBeds_Before.png'),('after','PlantBeds_After.png')]:
            path=ROOT/'Documentation/Images'/name
            assert sha(path)==sha(run/case/f'Habitat_0_{a.publish_view}.png');published.append(record(path))
        assert a.performance
        for folder in a.performance:
            folder=folder.resolve();summary=json.loads((folder/'Summary.json').read_text(encoding='utf-8-sig'))
            h=summary['host'];m=summary['metadata']
            assert not h['editor'] and not h['background_processes']
            assert h['executable_sha256']==host['sha256'].lower()
            assert sorted(h['package_containers'],key=lambda x:x['file'])==sorted(host['package_containers'],key=lambda x:x['file'])
            assert m['width']==1920 and m['height']==1080 and m['quality']==2
            assert m['r.ScreenPercentage']==100 and m['r.VSync']==0 and m['t.MaxFPS']==0
            performance.append(dict(summary=summary,files=[record(folder/n) for n in ['Host.json','Metadata.json','Summary.json','Capture.csv','Frame.png','Runtime.log']]))
        assert {x['summary']['metadata']['scenario'] for x in performance}=={'perf-underwater','perf-stream'}
    else:assert not a.performance
    prior=None
    if a.prior_run:
        old=a.prior_run.resolve()
        old_cases=json.loads((old/'Cases.json').read_text(encoding='utf-8-sig'))
        assert {c['name'] for c in old_cases}=={'before','after'}
        prior=dict(status='VISUALLY_REJECTED',reason='The first broad placement patches left the fixed Weesen near view too bare despite passing placement checks.',
            host=record(old/'Host.json'),cases=record(old/'Cases.json'),files=[])
        for name in ['before','after']:
            log=(old/name/'Runtime.log').read_text(encoding='utf-8-sig')
            assert 'DIVE_SMOKE_PASS' in log and 'DIVE_PLANT_BEDS_PASS' in log
            assert re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)',log)==poses[0]
            prior['files']+=[record(old/name/'Runtime.log')]+[record(old/name/f'Habitat_0_{i}.png') for i in range(3)]
    incomplete=None
    if a.incomplete_run:
        old=a.incomplete_run.resolve()
        completed=json.loads((old/'Cases.json').read_text(encoding='utf-8-sig'))
        assert {c['name'] for c in completed}=={'before','after','murg','ledi'}
        log=(old/'raster/Runtime.log').read_text(encoding='utf-8-sig')
        assert 'DIVE_SMOKE_PASS' in log and 'DIVE_POPULATION_PASS' not in log and 'DIVE_PLANT_BEDS_PASS' not in log
        incomplete=dict(status='INCOMPLETE_CHECKS',reason='Raster scene froze world ticking before clamped world time reached the scheduled population checks; captures succeeded but the required checks never ran.',
            host=record(old/'Host.json'),cases=record(old/'Cases.json'),files=[record(x) for x in sorted(old.glob('*/*')) if x.is_file()])
    slow=None
    if a.slow_start:
        path=a.slow_start.resolve();h=path.with_name(path.stem+'-host.json')
        context=json.loads(h.read_text(encoding='utf-8-sig'))
        assert context['status']=='PASS' and context['max_fps']==2 and context['runtime_sha256']==sha(path)
        for item in context['files']:assert sha(ROOT/item['file'])==item['sha256']
        log=path.read_text(encoding='utf-8-sig')
        assert 'DIVE_SMOKE_PASS' in log and 'DIVE_PLANT_BEDS_PASS' in log and 'r.Nanite = "0"' in log
        assert log.index('DIVE_POPULATION_PASS')<log.index('DIVE_ROCK_CAPTURE')
        assert not re.search(r'DIVE_\w*FAIL|Fatal error:',log)
        slow=dict(host=record(h),runtime=record(path),max_fps=2)
    result=dict(status='PASS',scope='Authored plant groups; not full visual reference acceptance',host=host,cases=evidence,prior_attempt=prior,incomplete_attempt=incomplete,slow_start=slow,
        screenshot_count=sum(c['screenshots'] for c in cases),comparisons=comparisons,launcher=launcher,published=published,performance=performance,
        limitations=['Groups and species weights are authored, not surveyed Walensee distributions',
            'Plant models and surfaces retain their existing quality limitations',
            'Placement and streaming checks do not replace visual or endurance acceptance'])
    a.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f"PASS {len(cases)} cases / {result['screenshot_count']} captures -> {a.output}")


if __name__=='__main__':main()
