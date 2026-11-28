from pathlib import Path
import argparse,json,re,hashlib,datetime as dt
from zoneinfo import ZoneInfo
import importlib.metadata as metadata
parser=argparse.ArgumentParser(description='Verify packaged celestial evidence against IANA time-zone rules.')
parser.add_argument('artifacts',type=Path)
parser.add_argument('--output',type=Path,default=Path('Documentation/CELESTIAL_RESULTS.json'))
parser.add_argument('--bootstrap',type=Path)
parser.add_argument('--benchmark',type=Path)
args=parser.parse_args()
root=args.artifacts
summary=json.loads((root/'Summary.json').read_text(encoding='utf-8-sig'))
assert summary['completed'] and len(summary['cases'])==13
assert hashlib.sha256(Path(summary['executable']).read_bytes()).hexdigest().upper()==summary['executable_sha256']
zone=ZoneInfo('Europe/Zurich')
report={'package':summary,'clock_oracle':{'implementation':'Python zoneinfo','zone':'Europe/Zurich','tzdata':metadata.version('tzdata')},'cases':[]}
for case in summary['cases']:
 folder=root/case['name'];log=(folder/'Runtime.log').read_text(encoding='utf-8-sig')
 assert case['passed'] and 'DIVE_SMOKE_PASS' in log
 assert not re.search(r'DIVE_\w+_FAIL|Fatal error:|Assertion failed:|Failed to compile Material|Default Material will be used',log)
 engine_errors=[line for line in log.splitlines() if re.search(r'Log\w+: Error:',line)]
 assert all('CDO Constructor (CelestialVaultDaySequenceActor): Failed to find /CelestialVault/Materials/MI_StarsStar_EnergyConservative.MI_StarsStar_EnergyConservative' in line for line in engine_errors),engine_errors
 assert 'material=/CelestialVault/Materials/MI_Stars_EnergyConservative.MI_Stars_EnergyConservative visible=1' in log
 row={'name':case['name'],'options':case['options'],'runtime_log':str(folder/'Runtime.log'),'known_upstream_startup_diagnostic': 'CDO Constructor (CelestialVaultDaySequenceActor): Failed to find' in log,'markers':[line.split('LogTemp:')[-1].strip() for line in log.splitlines() if re.search(r'DIVE_(?:SMOKE_PASS|CELESTIAL_REFERENCE|CELESTIAL_PASS|CONFIG_PASS|PROFILES_PASS|DAYCYCLE_PASS|DAYCYCLE_PAUSE_PASS|WEATHER_PASS|TORCH_PASS)',line)],'dive_logs':[]}
 for path in folder.glob('*.json'):
  dive=json.loads(path.read_text(encoding='utf-8-sig'));assert dive['is_test']
  env=dive['environment'];date=dt.datetime.fromisoformat(env['scenario_date'])
  local=(date+dt.timedelta(hours=env['hour'])).replace(tzinfo=zone,fold=0)
  assert abs(local.timestamp()-env['scenario_start_utc_seconds'])<1.e-5
  assert env['scenario_timezone']=='Europe/Zurich'
  assert local.utcoffset().total_seconds()/60==env['scenario_utc_offset_minutes']
  end=dt.datetime.fromtimestamp(env['scenario_start_utc_seconds']+dive['duration_seconds'],zone)
  assert dive['samples'] and abs(dive['samples'][-1]['seconds']-dive['duration_seconds'])<.001
  row['dive_logs'].append({'path':str(path),'scenario_start':local.isoformat(),'scenario_end':end.isoformat(),'duration_seconds':dive['duration_seconds'],'samples':len(dive['samples'])})
 if case['options']['Scenario']=='daycycle':
  match=re.search(r'DIVE_DAYCYCLE_PASS elapsed=([\d.]+) start=([\d.]+) current=([\d.]+) paused_wall=([\d.]+) solar_source=[\d.]+ date=(\S+) offset_minutes=(\d+)',log)
  assert match,case['name']
  elapsed,start,hour,pause,date,offset=match.groups();assert float(pause)>=4
  options=case['options'];local=(dt.datetime.fromisoformat(options['Date'])+dt.timedelta(hours=options['Hour'])).replace(tzinfo=zone,fold=0)
  expected=dt.datetime.fromtimestamp(local.timestamp()+float(elapsed),zone)
  expected_hour=expected.hour+expected.minute/60+(expected.second+expected.microsecond/1e6)/3600
  assert expected.date().isoformat()==date and expected.utcoffset().total_seconds()/60==int(offset)
  assert abs(expected_hour-float(hour))<.001/3600,(case['name'],expected_hour,hour)
  row['clock_boundary']={'native_local_date':date,'native_local_hour':float(hour),'native_offset_minutes':int(offset),'independent_iana_local':expected.isoformat(),'pause_wall_seconds':float(pause)}
 row['sha256']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(folder.iterdir()) if p.is_file()}
 row['png_count']=sum(p.suffix.lower()=='.png' for p in folder.iterdir())
 report['cases'].append(row)
refs=next(case['markers'] for case in report['cases'] if case['name']=='summer-noon')
refs=[r for r in refs if 'DIVE_CELESTIAL_REFERENCE PASS' in r];assert len(refs)==6
report['reference_maxima']={key:max(float(re.search(key+r'=([\d.]+)',r)[1]) for r in refs) for key in ('sun_error_deg','moon_error_deg','phase_error_percentage_points')}
files=list(Path('divingsim/Source/DiveSim').rglob('*.h'))+list(Path('divingsim/Source/DiveSim').rglob('*.cpp'))+list(Path('divingsim/Source/DiveSim').glob('*.cs'))+[Path(p) for p in ['divingsim/divingsim.uproject','divingsim/Config/DefaultGame.ini','divingsim/Config/DefaultEngine.ini','Tools/SmokeTest.ps1','Tools/TestCelestial.ps1','Tools/ReportCelestial.py','Tests/DiveModelTests.cpp']]
report['tested_source_sha256']={str(p).replace('\\','/'):hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(files)}
report['scope']='Rendered packaged gameplay and sampled astronomical/calendar validation. Frame rate capped at 30; background processes are recorded in package.background_processes. Not an isolated performance benchmark, winter biome or full visual acceptance.'
if args.bootstrap:
 bootstrap=json.loads(args.bootstrap.read_text(encoding='utf-8-sig'))
 assert bootstrap['passed'] and bootstrap['child']=='divingsim-Celestial.exe'
 for path_key,hash_key in [('launcher','launcher_sha256'),('log','log_sha256'),('dive_log','dive_sha256')]:
  assert hashlib.sha256(Path(bootstrap[path_key]).read_bytes()).hexdigest().upper()==bootstrap[hash_key]
 report['bootstrap']=bootstrap
if args.benchmark:
 benchmark=json.loads((args.benchmark/'Summary.json').read_text(encoding='utf-8-sig'))
 assert benchmark['measured_seconds']>=59 and benchmark['frames']>0
 report['performance']={'path':str(args.benchmark),'result':benchmark,'sha256':{p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(args.benchmark.iterdir()) if p.is_file()}}
args.output.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps({'cases':len(report['cases']),'pngs':sum(c['png_count'] for c in report['cases']),'maxima':report['reference_maxima']},indent=2))
