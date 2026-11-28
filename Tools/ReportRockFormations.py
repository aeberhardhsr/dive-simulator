"""Validate closed outcrop assets, paired native/package views and performance."""
import argparse,json,re
from pathlib import Path
from ReportNativeWaterLighting import ROOT,record,sha,pixels
from ReportRocks import package_check
from analyze_performance import analyze

def read(p):return json.loads(p.read_text(encoding='utf-8-sig'))

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('run',type=Path);p.add_argument('--output',required=True,type=Path)
    p.add_argument('--benchmark',action='append',type=Path,default=[])
    p.add_argument('--control-benchmark',type=Path)
    a=p.parse_args();run=a.run.resolve();host=read(run/'Host.json')
    for item in host['files']:assert sha(ROOT/item['file'])==item['sha256'],item['file']
    if host['packaged']:package_check(host)
    expected={'before':3,'after':3,'models':4}
    if not host['preview']:expected.update(weesen=3,murg=3,walenstadt=3,raster=3,stream=2,shore=9,sediment=7)
    cases=read(run/'Cases.json');assert len(cases)==len(expected) and {c['name'] for c in cases}==set(expected)
    logs={};evidence=[]
    for case in cases:
        name=case['name'];folder=run/name;log=(folder/'Runtime.log').read_text(encoding='utf-8-sig');logs[name]=log
        assert case['status']=='PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:',log)
        assert 'DIVE_WATER_PREVIEW_READY smoke=1 hardware=1 bounded=1 interface=1 lighting_mode=2' in log
        assert ('-DiveLegacyFormations' in log)==(name=='before')
        if name!='models':
            entry=1 if name in {'murg','stream'} else 2 if name=='walenstadt' else 0
            assert case['options']['Entry']==entry and re.search(r'-DiveEntry='+str(entry)+r'\b',log)
        formations=re.findall(r'DIVE_FORMATIONS_PASS legacy=(\d+) cores=(\d+) variants=([0-9,]+)',log)
        contacts=re.findall(r'DIVE_ROCK_POSE_PASS legacy=0 rocks=(\d+) envelope_preserved=1 probes=(\d+) contacts=(\d+) embedded=(\d+)',log)
        if name not in {'shore','sediment'}:
            assert formations and contacts
            for old,cores,counts in formations:
                counts=list(map(int,counts.split(',')));assert len(counts)==5
                assert int(old)==int(name=='before') and int(cores)==sum(counts[3:])
                assert (int(cores)==0 if name=='before' else min(counts[3:])>0)
            assert all(int(rocks)>0 and int(probes)>0 and probes==hits==embedded for rocks,probes,hits,embedded in contacts)
            for marker in ['DIVE_POPULATION_PASS','DIVE_ROCK_LAYOUT_PASS','DIVE_GRAVEL_PASS','DIVE_STEERING_WORLD_PASS','DIVE_POPULATION_RETIREMENT_PASS','DIVE_POPULATION_NEAR_TELEPORT_PASS']:assert marker in log,marker
        if name=='models':assert 'DIVE_FORMATION_INSPECTION_PASS variants=2 views=2 captures=4' in log
        if name=='raster':assert 'r.Nanite = "0"' in log
        images=sorted(folder.glob('*.png'));assert len(images)==expected[name]
        assert {x.name for x in images}==set(case['images'])
        for image in images:pixels(image)
        evidence.append(dict(name=name,runtime=record(folder/'Runtime.log'),formations=formations,contacts=contacts,images=[record(x) for x in images],engine_errors=[s for s in log.splitlines() if ': Error:' in s]))
    cameras=[re.findall(r'DIVE_HABITAT_VIEW ([^\r\n]+)',logs[n]) for n in ['before','after']]
    assert len(cameras[0])==3 and cameras[0]==cameras[1]
    populations=[re.findall(r'DIVE_HABITAT_PASS ([^\r\n]+)',logs[n]) for n in ['before','after']]
    assert populations[0] and populations[0]==populations[1]
    sources=read(ROOT/'Documentation/ROCK_FORMATION_ASSETS.json')
    geometry=read(ROOT/'blender/exports/FormationRocks/Manifest.json')
    assert [x['name'] for x in geometry['assets']]==['SM_LakeRock4','SM_LakeRock5']
    for item in sources['assets']:
        for f in [item,*item['maps'].values()]:assert sha(ROOT/f['file'])==f['sha256'],f['file']
    for item in geometry['assets']:
        assert item['closed_source_boundary_edges']==item['nonmanifold_edges']==0 and item['maximum_radius']<1.000001
        assert sha(ROOT/item['file'])==item['sha256']
    audit=read(ROOT/'artifacts/FormationReloadedAudit.json')
    for item,geo in zip(audit,geometry['assets']):
        assert item['name']==geo['name'] and item['triangles'][0]==geo['triangles']
        assert all(x>y>0 for x,y in zip(item['triangles'],item['triangles'][1:]))
        assert all(abs(x-y)<1e-6 for x,y in zip(item['screen_sizes'],[1,.2,.07,.02]))
    published=[];benchmarks=[];control=None;launcher=None;build=None
    if host['packaged']:
        build=ROOT/'artifacts/formation-package-build.log';assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
        build=record(build)
        launcher=read(run/'launcher/Launcher.json')
        assert launcher['status']=='PASS' and launcher['exit_code']==0 and launcher['executable_sha256'].lower()==host['sha256']
        assert launcher['log_sha256'].lower()==sha(run/'launcher/Launcher.log')
        for case,label,view in [('before','Before',1),('after','After',1),('weesen','Wide',1)]:
            path=ROOT/f'Documentation/Images/RockFormations_{label}.png'
            path.write_bytes((run/case/f'Habitat_0_{view}.png').read_bytes());published.append(record(path))
        assert len(a.benchmark)==3 and a.control_benchmark
        def benchmark(folder,legacy):
            folder=folder.resolve();result=analyze(folder);h,m=result['host'],result['metadata']
            assert not h['editor'] and not h['background_processes'] and h['water_preview']
            assert h['executable_sha256']==host['sha256'] and h['package_containers']==host['package_containers']
            assert h['legacy_formations']==m['legacy_formations']==legacy
            for k,v in {'width':1920,'height':1080,'quality':2,'r.ScreenPercentage':100,'r.VSync':0,'t.MaxFPS':0,'r.Lumen.HardwareRayTracing':1}.items():assert m[k]==v,(k,m[k])
            assert all(m[k] for k in ['water_preview','bounded_water','native_scattering','water_interface'])
            log=(folder/'Runtime.log').read_text(encoding='utf-8-sig')
            entry=int(re.search(r'-DiveEntry=(\d+)\b',log).group(1))
            assert m['warmup_seconds']==15 and m['requested_capture_seconds']==(90 if m['scenario']=='perf-stream' else 60)
            return dict(entry=entry,result=result,files=[record(folder/n) for n in ['Capture.csv','Metadata.json','Host.json','Frame.png','Runtime.log']])
        benchmarks=[benchmark(folder,False) for folder in a.benchmark];control=benchmark(a.control_benchmark,True)
        assert {(x['result']['metadata']['scenario'],x['entry']) for x in benchmarks}=={('perf-underwater',0),('perf-underwater',2),('perf-stream',1)}
        assert control['entry']==0 and control['result']['metadata']['scenario']=='perf-underwater'
    report=dict(status='PASS',scope='Two closed scanned outcrop cores, actual scene comparisons and bounded regression; not complete visual acceptance',host=host,cases=evidence,equal_comparison_cameras=cameras[0],equal_population_counts=populations[0],sources=sources,geometry=geometry,reloaded_render_audit=audit,import_reports=[record(ROOT/'artifacts'/n) for n in ['FormationContentBuild.json','FormationSurfaceBuild.json','FormationReloadedAudit.json']],build=build,launcher=launcher,benchmarks=benchmarks,control_benchmark=control,published=published,limitations=['Authored geometry/placement from non-local scans; no surveyed Walensee rock claim','Convex contacts are sampled, not triangle-exact collision','Different shapes change actual collision and reflections despite equal placement envelopes','Short single-run performance captures; no endurance or minimum-hardware claim','Only shore entry 0 is included; direct underwater entries 0/1/2 are separate locations','Bed transitions, vegetation, water optics and complete reference quality remain open'])
    a.output.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n',encoding='utf-8')
    print(f'ROCK_FORMATIONS_PASS cases={len(cases)} captures={sum(expected.values())}')

if __name__=='__main__':main()
