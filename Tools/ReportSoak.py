"""Archive a validated soak without confusing orchestration HEAD with package identity."""
import argparse
import hashlib
import json
from pathlib import Path
from analyze_soak import validate, verify_package
from soak_memory import read_memory

def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    report = read(args.run / 'Summary.json')
    intervals = [json.loads(line) for line in (args.run / 'Intervals.jsonl').read_text(encoding='utf-8-sig').splitlines()]
    validate(report['result'], intervals, read(args.run / 'DiveLog.json'))
    assert verify_package(report['host']) == report['package_integrity']
    for name, digest in report['sha256'].items():
        assert hashlib.sha256((args.run / name).read_bytes()).hexdigest() == digest, name
    rows = intervals + [report['result']]
    report['source_commit_scope'] = 'Host source_commit identifies verifier checkout at launch, not necessarily compiled package source; identify package by executable and cooked-container hashes.'
    report['interval_columns'] = ['wall_seconds', 'average_fps', 'maximum_ms', 'memory_mb', 'successful_autosaves']
    report['intervals'] = [[row['total']['wall_seconds'], row['interval']['average_fps'], row['interval']['maximum_ms'], row['memory_mb'], row['successful_autosaves']] for row in rows]
    report['windows_below_60_fps'] = sum(row['interval']['average_fps'] < 60 for row in rows)
    report['minimum_window_fps'] = min(row['interval']['average_fps'] for row in rows)
    activity = args.run / 'DevelopmentActivity.jsonl'
    report['development_activity'] = [json.loads(line) for line in activity.read_text(encoding='utf-8-sig').splitlines()] if activity.exists() else None
    host_memory = args.run / 'HostMemory.jsonl'
    samples = read_memory(host_memory, report['host'].get('production_water', False), report['result']['requested_seconds'])
    if samples is not None:
        report['host_memory_sha256'] = hashlib.sha256(host_memory.read_bytes()).hexdigest()
        report['host_memory_samples'] = samples
    report['memory_review_required'] = True
    report['captures'] = [{'file': p.name, 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()} for p in sorted(args.run.glob('*.png'))]
    report['limitations'] = ['Background activity is documented by the launcher snapshot and any separately supplied activity log; no blanket claim of concurrent builds',
        'Review private-memory and working-set histories at comparable route phases; structural validation alone does not prove memory stability',
        'Only this immutable package and the recorded route/mode are covered']
    args.output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f"SOAK_REPORT_VALIDATED windows={len(rows)} below60={report['windows_below_60_fps']} minimum_fps={report['minimum_window_fps']:.2f} memory_review=required")

if __name__ == '__main__':
    main()
