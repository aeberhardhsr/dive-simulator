"""Collect three fixed-scale vegetation measurements without inferring hardware certification."""
import argparse
import json
from pathlib import Path
from ReportVegetation import ROOT, read, sha


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('runs', type=Path, nargs=3, help='Underwater, shore, shore without Nanite')
    args = parser.parse_args()
    executable = ROOT/'Builds/WindowsVegetation/divingsim/Binaries/Win64/divingsim-Vegetation.exe'
    digest = sha(executable)
    runs = []
    for folder, scenario, fallback in zip(args.runs, ['perf-underwater','perf-shore','perf-shore'], [False,False,True]):
        folder = folder.resolve()
        summary = read(folder/'Summary.json')
        host, metadata = summary['host'], summary['metadata']
        assert host['executable_sha256'] == digest and not host['editor']
        assert host['disable_nanite'] == fallback
        assert host['package_containers'] == read(ROOT/'Documentation/VEGETATION_RESULTS.json')['executable']['package_containers']
        assert metadata['scenario'] == scenario
        assert metadata['width'] == 1920 and metadata['height'] == 1080
        assert metadata['r.ScreenPercentage'] == 100 and metadata['quality'] == 2
        assert 59 <= summary['measured_seconds'] <= 61
        log = (folder/'Runtime.log').read_text(encoding='utf-8-sig')
        assert 'DIVE_VEGETATION_PASS' in log and 'DIVE_VEGETATION_FAIL' not in log
        runs.append(dict(directory=str(folder), summary=summary,
                         files={name:sha(folder/name) for name in ('Capture.csv','Metadata.json','Host.json','Runtime.log','Frame.png','Summary.json')}))
    report = dict(executable=str(executable), executable_sha256=digest, runs=runs,
                  limitations='Three 60-second development-host captures, user Unreal Editor left open. Shore rotates through 360 degrees. Not minimum-hardware, long-distance streaming, endurance or native macOS certification.')
    (ROOT/'Documentation/VEGETATION_PERFORMANCE.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    for run in runs:
        summary = run['summary']
        print(summary['metadata']['scenario'], 'fallback='+str(summary['host']['disable_nanite']),
              round(summary['average_fps'],2), round(summary['one_percent_low_fps'],2), summary['frame']['max_ms'])


if __name__ == '__main__':
    main()
