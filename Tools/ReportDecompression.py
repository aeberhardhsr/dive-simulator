"""Bind mathematical, native and packaged decompression evidence to inputs."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]
def sha(path):
    with path.open('rb') as stream:return hashlib.file_digest(stream,'sha256').hexdigest()
def read(path):return json.loads(path.read_text(encoding='utf-8-sig'))

def main():
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('run',type=Path)
    parser.add_argument('--performance',type=Path,required=True);args=parser.parse_args()
    run=args.run.resolve();host=read(run/'Host.json');exe=Path(host['executable'])
    assert host['packaged'] and sha(exe)==host['sha256'].lower()
    containers=[dict(file=p.name,bytes=p.stat().st_size,sha256=sha(p)) for p in sorted((exe.parents[3]/'divingsim/Content/Paks').iterdir()) if p.is_file()]
    assert containers==sorted(host['package_containers'],key=lambda p:p['file'])
    cases=read(run/'Cases.json')
    assert len(cases)==7 and {x['name'] for x in cases}=={'decompression','wrist-day','wrist-night','controls','shore-murg','exploration','hardcore'}
    evidence=[]
    for case in cases:
        folder=run/case['name'];log=(folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert case['status']=='PASS' and 'DIVE_SMOKE_PASS' in log
        assert not re.search(r'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used',log)
        for marker in ['DIVE_IMAGERY_PASS','DIVE_GROUND_PASS','DIVE_MEDIUM_PASS','DIVE_LIGHT_PASS']:assert marker in log
        if case['scenario'].startswith('wrist'):assert 'page=3 rendered_pages=15' in log
        if case['name']=='controls':assert 'DIVE_INPUT_PASS' in log
        if case['name']=='shore-murg':assert 'DIVE_SHORE_PASS' in log
        if case['scenario']=='modes':assert 'DIVE_MODE_PASS' in log
        if case['name']=='decompression':
            assert 'DIVE_DECOMPRESSION_PASS stages=6 pause=1 hold=1 input_cycle=4' in log
            assert 'DIVE_DECOMPRESSION_PAUSE_PASS' in log and 'DIVE_DECOMPRESSION_HOLD_PASS' in log
            for stage in ['Deep_DE','Computer_EN','Stop_DE','Progress_EN','Clear_EN','Violation_DE']:
                assert f'DIVE_DECOMPRESSION_STAGE {stage}' in log
                assert (folder/f'Deco_{stage}.png').exists() and (folder/f'DecoDisplay_{stage}.png').exists()
            assert re.search(r'DIVE_DECOMPRESSION_STAGE Clear_EN valid=1 .* stop=0\.000 wait=0\.0 tts=20\.000',log)
            assert 'DIVE_DECOMPRESSION_STAGE Violation_DE valid=0' in log
        images=[]
        for path in sorted(folder.glob('*.png')):
            with Image.open(path) as im:
                assert im.size in [(1600,900),(768,512)];im.verify()
            images.append(dict(file=path.relative_to(ROOT).as_posix(),sha256=sha(path)))
        assert len(images)==case['screenshots'] and images
        evidence.append(dict(**case,log_sha256=sha(folder/'Runtime.log'),images=images))
    core=ROOT/'artifacts/decompression-core-final.log';text=core.read_text(encoding='utf-8-sig')
    assert text.count('PROFILE_PASS')==20 and 'FAIL' not in text
    assert 'PASS: independent numerical decompression checks (0 failures)' in text
    native=ROOT/'artifacts/DecompressionAutomation/index.json';automation=read(native)
    assert automation['succeeded']==35 and automation['failed']==0
    build=ROOT/'artifacts/decompression-package.log';assert 'BUILD SUCCESSFUL' in build.read_text(encoding='utf-8-sig')
    launcher=read(ROOT/'artifacts/decompression-bootstrap.json')
    assert launcher['status']=='PASS' and launcher['exit_code']==0 and launcher['executable_sha256'].lower()==sha(exe)
    assert launcher['log_sha256'].lower()==sha(ROOT/'artifacts/decompression-bootstrap.log')
    perf=args.performance.resolve();summary=read(perf/'Summary.json')
    assert summary['host']['executable_sha256']==sha(exe) and not summary['host']['editor']
    assert summary['host']['package_containers']==host['package_containers']
    metadata=summary['metadata']
    assert metadata['scenario']=='perf-underwater' and metadata['width']==1920 and metadata['height']==1080
    assert metadata['quality']==2 and metadata['r.ScreenPercentage']==100 and 59<=summary['measured_seconds']<=61
    assert 'DIVE_GROUND_PASS' in (perf/'Runtime.log').read_text(encoding='utf-8-sig')
    performance=dict(summary=summary,files={name:sha(perf/name) for name in ('Capture.csv','Metadata.json','Host.json','Runtime.log','Frame.png','Summary.json')})
    sources=[];project=ROOT/'divingsim';sandbox=ROOT/'Builds/EditorSandbox/divingsim'
    for directory in ['Source','Config']:
        for path in sorted((project/directory).rglob('*')):
            if not path.is_file():continue
            relative=path.relative_to(project);data=path.read_bytes().replace(b'\r\n',b'\n')
            assert data==(sandbox/relative).read_bytes().replace(b'\r\n',b'\n'),str(relative)
            sources.append(dict(file=relative.as_posix(),canonical_lf_sha256=hashlib.sha256(data).hexdigest()))
    baseline=ROOT/'Builds/WindowsGround/divingsim/Binaries/Win64/divingsim-GroundDetails.exe'
    assert sha(baseline)=='87f6c3e039201ae5af19cdcefffa6ca4b20a3a2ca89d30bb2a116d13b631ea54'
    report=dict(status='PASS',executable=host,cases=evidence,sources=sources,
        numerical_profiles=20,core_log_sha256=sha(core),native_tests=dict(succeeded=35,failed=0,report_sha256=sha(native)),
        package_log_sha256=sha(build),root_launcher=launcher,performance=performance,preserved_ground_executable_sha256=sha(baseline),
        limits='Independent numerical integration validates these equations, not physiology. Accelerated model/render fixture is not a manually controlled decompression dive. No repetitive-dive, gas-sufficiency, native macOS or clinical certification.')
    (ROOT/'Documentation/DECOMPRESSION_RESULTS.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('DECOMPRESSION_PASS',len(cases),'cases',sum(len(x['images']) for x in evidence),'images; 20 numerical profiles; 35 native tests')

if __name__=='__main__':main()
