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
    a=p.parse_args();run=a.run.resolve()
    host=json.loads((run/'Host.json').read_text(encoding='utf-8-sig'))
    for item in host['files']:assert sha(ROOT/item['file'])==item['sha256'],item['file']
    if host['packaged']:package_check(host)
    assert not (host['packaged'] and host['preview'])
    expected={'close-before':3,'close-after':3}
    if not host['preview']:expected.update(weesen=3,murg=3,**{'ledi-wide':3,'raster':3,'stream':2,'shore':6})
    cases=json.loads((run/'Cases.json').read_text(encoding='utf-8-sig'))
    assert len(cases)==len(expected) and {c['name'] for c in cases}==set(expected)
    evidence=[];logs={}
    for case in cases:
        folder=run/case['name'];log=(folder/'Runtime.log').read_text(encoding='utf-8-sig');logs[case['name']]=log
        assert 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:',log)
        checks=[]
        if case['name']!='shore':
            checks=[tuple(map(int,x)) for x in re.findall(r'DIVE_RUBBLE_PASS fragments=(\d+) disabled=(\d+) contacts=(\d+)',log)]
            assert checks
            assert all((n==0 and disabled==1 and contacts==0) if case['disabled'] else (n>0 and disabled==0 and contacts>0)
                       for n,disabled,contacts in checks)
            for marker in ['DIVE_POPULATION_PASS','DIVE_ROCK_LAYOUT_PASS','DIVE_STEERING_WORLD_PASS','DIVE_GRAVEL_PASS']:
                assert marker in log,marker
        if case['name']=='raster':assert 'r.Nanite = "0"' in log
        images=sorted(folder.glob('*.png'));assert len(images)==case['screenshots']==expected[case['name']]
        for image in images:pixels(image)
        evidence.append(dict(**case,rubble_checks=checks,runtime=record(folder/'Runtime.log'),images=[record(x) for x in images],
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
        for case,name in [('close-before','RockRubble_Before.png'),('close-after','RockRubble_After.png')]:
            image=ROOT/'Documentation/Images'/name
            assert sha(image)==sha(run/case/'Habitat_0_0.png');published.append(record(image))
        for folder in a.performance:
            folder=folder.resolve();summary=json.loads((folder/'Summary.json').read_text(encoding='utf-8-sig'))
            h=summary['host'];m=summary['metadata']
            assert not h['editor'] and not h['background_processes']
            assert h['executable_sha256']==host['sha256'].lower()
            assert sorted(h['package_containers'],key=lambda x:x['file'])==sorted(host['package_containers'],key=lambda x:x['file'])
            assert m['width']==1920 and m['height']==1080 and m['quality']==2
            assert m['r.ScreenPercentage']==100 and m['r.VSync']==0 and m['t.MaxFPS']==0
            performance.append(dict(summary=summary,files=[record(folder/n) for n in ['Host.json','Metadata.json','Summary.json','Capture.csv','Frame.png','Runtime.log']]))
        if a.performance:assert len(performance)==2 and {x['summary']['metadata']['scenario'] for x in performance}=={'perf-underwater','perf-stream'}
    else:assert not a.performance
    result=dict(status='PASS',scope='Authored rock-foot fragments; not full environment acceptance',host=host,
        cases=evidence,screenshot_count=sum(expected.values()),comparisons=comparisons,launcher=launcher,published=published,performance=performance,
        limitations=['Fragments are authored dressing, not surveyed Walensee geology',
                    'Collision probes sample individual fragment bodies; not every contact direction',
                    'Additional rock footprints also affect local plants, gravel and fish paths',
                    'Short rendering and performance scenarios are not endurance acceptance',
                    'Plant quality, sediment transitions and lighting still require work'])
    a.output.write_text(json.dumps(result,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'PASS {len(cases)} cases / {sum(expected.values())} captures -> {a.output}')


if __name__=='__main__':main()
