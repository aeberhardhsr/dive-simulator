"""Verify a packaged shore-transition run and retain its reproducible evidence."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

import numpy as np
from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
BASELINE = 'e90f18e9e3497e5440826080413c8e961ab0cc24'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8-sig'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    args = parser.parse_args()
    host = read(args.run / 'Host.json')
    assert host['shore_seams']
    exe = Path(host['executable'])
    assert sha(exe).lower() == host['executable_sha256'].lower()
    cases = read(args.run / 'Cases.json')
    assert {c['name'] for c in cases} == {'buildings0','buildings1','buildings2','fallback1','shore0','shore1','shore2','sediment0','avatar0','breathing','wrist','stream1'}
    results, seams, comparisons = [], {}, []
    for case in cases:
        folder = args.run / case['name']
        log = (folder / 'Runtime.log').read_text(encoding='utf-8-sig')
        assert case['status'] == 'PASS' and 'DIVE_SMOKE_PASS' in log
        assert 'DIVE_BUILDINGS_PASS buildings=16249 cells=715' in log
        assert not re.search(r'DIVE_\w*FAIL|Failed to compile Material|Default Material will be used', log)
        if case['name'] == 'fallback1':
            assert 'r.Nanite = "0"' in log
        if case['name'] in ('buildings0','buildings1','buildings2'):
            old = ROOT / 'artifacts/buildings-validation-20261126-015734' / case['name']
            before_log=(old/'Runtime.log').read_text(encoding='utf-8-sig')
            camera=re.search(r'DIVE_BUILDINGS_INSPECTION ([^\r\n]+)',log).group(1)
            assert camera == re.search(r'DIVE_BUILDINGS_INSPECTION ([^\r\n]+)',before_log).group(1)
            image_name=f"Shore_{case['entry']}_Buildings.png"
            comparisons.append(dict(entry=case['entry'],camera=camera,before_sha256=sha(old/image_name),after_sha256=sha(folder/image_name)))
        if case['scenario'] in ('shore-preview', 'shore'):
            values = re.findall(r'DIVE_SHORE_SEAM_PASS entry=(\d) points=(\d+) max_error_cm=([\d.]+) interior=(\d+) bed=(\d+) source_error_cm=([\d.]+)', log)
            assert {v[0] for v in values} == {'0','1','2'}
            for index, count, error, interior, bed, source_error in values:
                assert int(count) >= 100 and int(interior) >= 100 and int(bed) >= 100
                assert float(error) <= .05 and float(source_error) <= .05
                seams[index] = dict(boundary_points=int(count), maximum_error_cm=float(error),
                    preserved_interior_vertices=int(interior), preserved_bathymetry_vertices=int(bed),
                    maximum_source_rounding_cm=float(source_error))
        images = []
        for path in sorted(folder.glob('*.png')):
            with Image.open(path) as image:
                image.load()
                assert image.width >= 512 and image.height >= 512
                images.append(dict(name=path.name, size=list(image.size), sha256=sha(path)))
        assert len(images) == case['screenshots'] and images
        results.append(dict(name=case['name'], images=images, log_sha256=sha(folder / 'Runtime.log')))
    data = ROOT / 'divingsim/Content/Diving/Geodata'
    original_files = [data/'landscape.f32'] + sorted(data.glob('*_shore.f32')) + sorted(data.glob('*_shore.u8')) + sorted(data.glob('*.i16'))
    unchanged = []
    for path in original_files:
        relative = path.relative_to(ROOT).as_posix()
        pointer = subprocess.check_output(['git', 'show', BASELINE + ':' + relative], cwd=ROOT).decode()
        oid = re.search(r'oid sha256:([a-f0-9]{64})', pointer).group(1)
        assert sha(path) == oid, relative
        unchanged.append(dict(path=relative, sha256=oid))
    # Independent baseline discrepancy at the old mesh perimeter. The source
    # arrays are unchanged; this does not evaluate the new C++ blending helper.
    m = read(data/'landscape.json')
    land = np.fromfile(data/m['file'], dtype='<f4').reshape(m['rows'],m['columns'])
    def sample(g,w,n,s,east,north,triangular=False):
        x=(east-w)/s-.5;y=(n-north)/s-.5;c=np.floor(x).astype(int);r=np.floor(y).astype(int);tx=x-c;ty=y-r
        a=g[r,c];b=g[r,c+1];cc=g[r+1,c];d=g[r+1,c+1]
        if triangular:
            return np.where(tx+ty<=1,a+(b-a)*tx+(cc-a)*ty,d+(cc-d)*(1-tx)+(b-d)*(1-ty))
        return (a*(1-tx)+b*tx)*(1-ty)+(cc*(1-tx)+d*tx)*ty
    before = []
    for site in read(data/'shoreline.json')['sites']:
        g=np.fromfile(data/site['file'],dtype='<f4').reshape(384,384);w=site['west'];n=site['south']+384;t=np.arange(1,384)
        east=np.r_[w+t,w+t,np.full(383,w+1),np.full(383,w+383)]
        north=np.r_[np.full(383,n-1),np.full(383,n-383),n-t,n-t]
        a=sample(g,w,n,1,east,north);b=sample(land,m['west'],m['north'],8,east,north,True)
        errors=np.abs(a[a>1]-b[a>1])
        before.append(dict(site=site['id'],old_boundary_maximum_difference_m=float(errors.max()),old_boundary_p95_difference_m=float(np.percentile(errors,95))))
    native = read(ROOT/'artifacts/ShoreSeamAutomation/index.json')
    assert native['succeeded'] == 30 and native['failed'] == native['succeededWithWarnings'] == native['notRun'] == 0
    snapshots = []
    for directory in ('Source','Config'):
        for path in sorted((ROOT/'divingsim'/directory).rglob('*')):
            if path.is_file():
                copy=ROOT/'Builds/EditorSandbox/divingsim'/path.relative_to(ROOT/'divingsim')
                assert sha(path) == sha(copy)
                snapshots.append(dict(path=path.relative_to(ROOT).as_posix(),sha256=sha(path)))
    previous=read(ROOT/'Documentation/BUILDINGS_RESULTS.json')
    assert sha(Path(previous['executable'])) == previous['executable_sha256']
    package=(ROOT/'artifacts/shore-seam-package.log').read_text(encoding='utf-8-sig')
    assert 'BUILD SUCCESSFUL' in package and 'DIVE_LAND_VERIFY_PASS' in package
    report=dict(status='PASS',executable=str(exe),executable_sha256=sha(exe),baseline_commit=BASELINE,
        previous_build_preserved_sha256=previous['executable_sha256'],before=before,boundaries=seams,identical_camera_comparisons=comparisons,
        unchanged_source_rasters=unchanged,source_snapshot=snapshots,native_tests=30,cases=results,
        image_count=sum(len(c['images']) for c in results),
        evidence={name:sha(ROOT/'artifacts'/name) for name in ['shore-seam-core.log','shore-seam-python.log','shore-seam-source-validation.log','ShoreSeamAutomation/index.json','shore-seam-editor-final.log','shore-seam-landscape.log','shore-seam-material.log','shore-seam-package.log']},
        package_archives=[dict(path=str(p),sha256=sha(p)) for p in sorted((exe.parents[2]/'Content/Paks').iterdir()) if p.is_file()],
        limits=['The 32 m transition band is a render/collision approximation, not new survey data.',
                'Boundary checks compare procedural vertices with the source background triangles; visual Nanite/fallback checks are separate.',
                'Far shoreline/bathymetry transitions, vegetation and photorealistic material detail remain incomplete.',
                'No native macOS gameplay or minimum-hardware validation.'])
    (ROOT/'Documentation/SHORE_TRANSITION_RESULTS.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('PASS',len(cases),'cases',report['image_count'],'images',len(unchanged),'unchanged survey rasters',sha(exe))


if __name__ == '__main__':
    main()
