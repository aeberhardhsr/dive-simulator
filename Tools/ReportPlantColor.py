"""Bind the plant color audit, original-material comparisons and cooked tests."""
import argparse
import hashlib
import json
import re
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def record(path):
    return dict(file=path.relative_to(ROOT).as_posix(), sha256=sha(path), bytes=path.stat().st_size)


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def captures(folder, expected):
    log_path = folder / 'Runtime.log'
    log = log_path.read_text(encoding='utf-8-sig')
    assert 'DIVE_SMOKE_PASS' in log
    assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used|Fatal error:', log)
    images = sorted(folder.glob('*.png'))
    assert len(images) == expected, folder
    for path in images:
        with Image.open(path) as image:
            assert image.size == (1600, 900), path
    return dict(name=folder.name, status='PASS', screenshots=expected, log=record(log_path),
                images=[record(path) for path in images],
                population=sorted(set(re.findall(r'DIVE_PLANT_BEDS_PASS ([^\r\n]+)', log))))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--study', type=Path, required=True)
    parser.add_argument('--package', type=Path, required=True, help='TestPlantColor result directory')
    parser.add_argument('--performance', type=Path, action='append', default=[])
    parser.add_argument('--output', type=Path, default=ROOT/'Documentation/PLANT_COLOR_RESULTS.json')
    args = parser.parse_args()
    study, package = args.study.resolve(), args.package.resolve()
    host = read(package/'Host.json')
    assert host['packaged'] and sha(Path(host['executable'])) == host['sha256']
    for item in host['files']:
        assert sha(ROOT/item['file']) == item['sha256'], item['file']
    pak_root = Path(host['executable']).parents[2]/'Content/Paks'
    for item in host['package_containers']:
        assert sha(pak_root/item['file']) == item['sha256'], item['file']
    audit_path = ROOT/'Documentation/PLANT_COLOR_AUDIT.json'
    audit = read(audit_path)
    assert audit['status'] == 'PASS' and len(audit['assets']) == 3
    assert sha(ROOT/'blender/AquaticPlants.blend') == audit['blend_sha256']
    assert sha(ROOT/'Tools/Shaders/AquaticPlant.ush') == audit['shader_sha256']
    assert sha(ROOT/'divingsim/Content/Diving/Materials/M_AquaticPlant.uasset') == audit['material_sha256']
    assert sha(study/'M_AquaticPlant.fixed.uasset') == audit['material_sha256']
    assert sha(study/'color-buffer-final/Runtime.log') == audit['native_log_sha256']
    for item in audit['assets']:
        assert sha(ROOT/'blender/exports/Plants'/(item['mesh']+'.fbx')) == item['fbx_sha256']
        assert sha(ROOT/'divingsim/Content/Diving/Plants'/(item['mesh']+'.uasset')) == item['mesh_sha256']
    native = [captures(study/name, count) for name, count in [
        ('color-before', 3), ('color-after', 3), ('color-forms-before', 12),
        ('color-forms', 12), ('color-buffer-final', 2)]]
    assert native[0]['population'] == native[1]['population']
    cases = read(package/'Cases.json')
    assert {c['name'] for c in cases} == {'weesen','murg','forms','raster','torch','shore','stream'}
    assert len(cases) == 7 and all(c['status'] == 'PASS' for c in cases)
    cooked = [captures(package/c['name'], c['screenshots']) for c in cases]
    launcher = read(package/'Launcher.json')
    assert launcher['status'] == 'PASS' and launcher['executable_sha256'].lower() == host['sha256']
    performance = []
    for directory in args.performance:
        directory = directory.resolve()
        perf_host = read(directory/'Host.json')
        assert not perf_host['editor'] and perf_host['water_preview']
        assert perf_host['executable_sha256'] == host['sha256']
        assert perf_host['package_containers'] == host['package_containers']
        performance.append(dict(directory=directory.relative_to(ROOT).as_posix(),
                                summary=read(directory/'Summary.json'),
                                artifacts=[record(directory/name) for name in
                                           ['Host.json','Metadata.json','Capture.csv','Summary.json','Frame.png','Runtime.log']]))
    report = dict(status='PASS', visual_target_complete=False, host=host,
                  audit=record(audit_path), original_material=record(study/'M_AquaticPlant.original.uasset'),
                  fixed_material=record(study/'M_AquaticPlant.fixed.uasset'),
                  native_cases=native, native_screenshots=sum(c['screenshots'] for c in native),
                  package_cases=cooked, package_screenshots=sum(c['screenshots'] for c in cooked),
                  launcher=launcher, launcher_log=record(package/'Launcher.log'),
                  performance=performance,
                  limits=['Range audit of LOD0 does not verify vertex ordering or real-world reflectance.',
                          'Native before/after swaps only the plant material; package cases use the fixed material.',
                          'Existing engine/plugin startup diagnostics remain in full logs.',
                          'Underwater lighting, sediment detail and distant thin branches remain unfinished.'])
    args.output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print('PLANT_COLOR_REPORT_PASS', args.output)


if __name__ == '__main__':
    main()
