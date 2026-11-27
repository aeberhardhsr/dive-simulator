"""Bind native deposited-bed comparisons and contact checks to their inputs."""
import argparse,json,re
from pathlib import Path
from ReportNativeWaterLighting import ROOT,record,sha,pixels
from ReportRocks import package_check
from analyze_performance import analyze

def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('run',type=Path);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--benchmark',action='append',type=Path,default=[])
    p.add_argument('--control-benchmark',action='append',type=Path,default=[])
    a=p.parse_args();run=a.run.resolve();host=read(run/'Host.json')
    packaged=host.get('packaged',False)
    for item in host['files']:assert sha(ROOT/item['file'])==item['sha256'],item['file']
    if packaged:package_check(host)
    baseline=read(ROOT/'Documentation/ROCK_FORMATIONS_RESULTS.json')
    assets=[f for f in baseline['host']['files'] if f['file'].startswith('divingsim/Content/')]
    for item in assets:assert sha(ROOT/item['file'])==item['sha256'],item['file']
    expected={'before':3,'after':3}
    if not host['preview']:expected.update(murg=3,walenstadt=3,shore=9,stream=2,sediment=7)
    cases=read(run/'Cases.json');assert {c['name'] for c in cases}==set(expected) and len(cases)==len(expected)
    logs={};evidence=[]
    for case in cases:
        name=case['name'];folder=run/name;log=(folder/'Runtime.log').read_text(encoding='utf-8-sig');logs[name]=log
        assert case['status']=='PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Fatal error:|Failed to compile Material|Default Material will be used',log)
        assert ('-DiveNoFootDeposits' in log)==(name=='before')
        assert f'DIVE_FOOT_DEPOSITS_READY enabled={int(name!="before")} maximum_cm=70' in log
        assert 'DIVE_WATER_PREVIEW_READY smoke=1 hardware=1 bounded=1 interface=1 lighting_mode=2' in log
        entry=1 if name in {'murg','stream'} else 2 if name=='walenstadt' else 0
        assert case['options']['Entry']==entry and re.search(r'-DiveEntry='+str(entry)+r'\b',log)
        checks=re.findall(r'DIVE_FOOT_DEPOSIT_PASS ([^\r\n]+)',log)
        if name not in {'before','shore','sediment'}:
            assert checks
            for check in checks:
                fields=dict(re.findall(r'(\w+)=([\d.]+)',check))
                assert int(fields['probes'])>100 and fields['probes']==fields['contacts']
                assert int(fields['raised'])>0 and 0<float(fields['maximum_cm'])<=70.001
                assert fields['rebuild_equal']=='1' and int(fields['source_cores'])>0
            for marker in ['DIVE_POPULATION_PASS','DIVE_ROCK_LAYOUT_PASS','DIVE_GRAVEL_PASS','DIVE_STEERING_WORLD_PASS','DIVE_POPULATION_RETIREMENT_PASS','DIVE_POPULATION_NEAR_TELEPORT_PASS']:assert marker in log,marker
        images=sorted(folder.glob('*.png'));assert len(images)==expected[name]
        assert {x.name for x in images}==set(case['images'])
        for image in images:pixels(image)
        evidence.append(dict(name=name,checks=checks,runtime=record(folder/'Runtime.log'),images=[record(x) for x in images],engine_errors=[line for line in log.splitlines() if ': Error:' in line]))
    cameras=[re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)',logs[name]) for name in ['before','after']]
    assert len(cameras[0])==3 and cameras[0]==cameras[1]
    published=[]
    for case,label in [('before','Before'),('after','After')]:
        target=ROOT/f'Documentation/Images/FootDeposits_{label}.png'
        target.write_bytes((run/case/'Habitat_0_1.png').read_bytes());published.append(record(target))
    benchmarks=[];control=[];launcher=None;build=None
    if packaged:
        build=ROOT/'artifacts/foot-deposit-package-build.log'
        assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
        build=record(build)
        launcher=read(run/'launcher/Launcher.json')
        assert launcher['status']=='PASS' and launcher['exit_code']==0 and launcher['executable_sha256'].lower()==host['sha256']
        assert launcher['log_sha256'].lower()==sha(run/'launcher/Launcher.log')
        assert 'DIVE_FOOT_DEPOSITS_READY enabled=1 maximum_cm=70' in (run/'launcher/Launcher.log').read_text(encoding='utf-8-sig')
        assert a.benchmark and a.control_benchmark
    if a.benchmark:
        assert len(a.benchmark)==len(a.control_benchmark)==(3 if packaged else 2)
        def benchmark(folder,enabled):
            folder=folder.resolve();result=analyze(folder);h,m=result['host'],result['metadata']
            assert h['editor']==(not packaged) and not h['background_processes']
            if packaged:
                assert h['executable_sha256']==host['sha256'] and h['package_containers']==host['package_containers']
            else:assert h['editor_sandbox']
            assert h['foot_deposits']==m['foot_deposits']==enabled and m['water_preview']
            for key,value in {'width':1920,'height':1080,'quality':2,'r.ScreenPercentage':100,'r.VSync':0,'t.MaxFPS':0,'r.Lumen.HardwareRayTracing':1}.items():assert m[key]==value,(key,m[key])
            log=(folder/'Runtime.log').read_text(encoding='utf-8-sig')
            entry=int(re.search(r'-DiveEntry=(\d+)\b',log).group(1))
            assert m['warmup_seconds']==15 and m['requested_capture_seconds']==(90 if m['scenario']=='perf-stream' else 60)
            return dict(entry=entry,result=result,files=[record(folder/name) for name in ['Host.json','Metadata.json','Capture.csv','Frame.png','Runtime.log']])
        benchmarks=[benchmark(folder,True) for folder in a.benchmark]
        control=[benchmark(folder,False) for folder in a.control_benchmark]
        assert {x['result']['metadata']['scenario'] for x in benchmarks}=={'perf-underwater','perf-stream'}
        assert {x['result']['metadata']['scenario'] for x in control}=={'perf-underwater','perf-stream'}
        sites={('perf-underwater',0),('perf-stream',1)}|({('perf-underwater',2)} if packaged else set())
        assert {(x['result']['metadata']['scenario'],x['entry']) for x in control}==sites
        assert {(x['result']['metadata']['scenario'],x['entry']) for x in benchmarks}==sites
    report=dict(status='PASS',scope='Local deposited slopes and shared floor contacts; functional checks passed, not full visual or frame-time acceptance',host=host,asset_baseline=record(ROOT/'Documentation/ROCK_FORMATIONS_RESULTS.json'),checked_assets=assets,cases=evidence,equal_comparison_cameras=cameras[0],published=published,build=build,launcher=launcher,benchmarks=benchmarks,control_benchmark=control,limitations=['Deposits are authored up to 70 cm, not surveyed or physically simulated transport','Only shore entry 0 checked; other shore sites and night remain unverified','Underlying metre-grid resolution remains; full reference quality is not achieved','Short single-run performance captures; no minimum-hardware or endurance claim','Walenstadt and streaming have additional long frames with deposits; their exact cause is not isolated'])
    a.output.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'FOOT_DEPOSITS_PASS cases={len(cases)} captures={sum(expected.values())}')

if __name__=='__main__':main()
