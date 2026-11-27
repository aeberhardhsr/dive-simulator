"""Check native GPU readback against the independently prepared raster samples."""
import argparse
import hashlib
import json
from pathlib import Path

import numpy as np


def check_readback(probes, captured, distance_tolerance=.025, wet_tolerance=.025):
    if probes.ndim != 2 or probes.shape[1] != 4 or captured.shape != (len(probes),6) or not len(probes):
        raise ValueError('Missing or incomplete GPU sample rows')
    if not np.isfinite(probes).all() or not np.isfinite(captured).all():
        raise ValueError('Nonfinite GPU data')
    if not np.allclose(captured[:,[0,1,2,4]],probes,rtol=0,atol=1.e-6):
        raise ValueError('GPU output belongs to different probe positions or reference data')
    errors=np.abs(captured[:,3]-probes[:,2]); wet_errors=np.abs(captured[:,5]-probes[:,3])
    if errors.max()>distance_tolerance or wet_errors.max()>wet_tolerance:
        raise ValueError('GPU sampling differs from the independent CPU reference')
    outside=np.abs(probes[:,2])>.75
    wrong=int(np.count_nonzero((captured[:,3]>0)[outside] != (probes[:,2]>0)[outside]))
    if wrong:
        raise ValueError('GPU water/land classification changed outside the uncertainty band')
    return {'count':len(probes),'max_distance_error_m':float(errors.max()),
            'p99_distance_error_m':float(np.percentile(errors,99)),
            'max_wet_weight_error':float(wet_errors.max()),'wrong_sign_outside_uncertainty':wrong}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probes',type=Path,required=True)
    parser.add_argument('--capture',type=Path,required=True)
    args=parser.parse_args()
    manifest=json.loads((args.probes/'Probes.json').read_text(encoding='utf-8'))
    binary=args.probes/'Probes.f32'
    if hashlib.sha256(binary.read_bytes()).hexdigest()!=manifest['probe_sha256']:
        raise ValueError('Probe inputs changed')
    probes=np.fromfile(binary,dtype='<f4').reshape(-1,4)
    csv=args.capture/'LakeFootprintGPU.csv'
    captured=np.loadtxt(csv,delimiter=',',skiprows=1,encoding='utf-8-sig')
    log=(args.capture/'Runtime.log').read_text(encoding='utf-8-sig')
    if 'DIVE_LAKE_GPU_PASS' not in log or 'DIVE_CAUSTICS_PROBE_PASS captures=4' not in log:
        raise ValueError('Native scene/readback did not complete')
    if 'DIVE_LAKE_GPU_FAIL' in log or 'DIVE_LAKE_FOOTPRINT_FAIL' in log:
        raise ValueError('Native footprint failure')
    report={'status':'GPU_SAMPLING_VALIDATED_NOT_FULL_SHORELINE_ACCEPTANCE','summary':check_readback(probes,captured),
        'groups':{},'probe_manifest':manifest,'capture_directory':str(args.capture.resolve()),
        'readback_sha256':hashlib.sha256(csv.read_bytes()).hexdigest(),
        'runtime_log_sha256':hashlib.sha256((args.capture/'Runtime.log').read_bytes()).hexdigest(),
        'reporter_sha256':hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        'limits':['Compute readback checks the actual Scene texture/sampler/mapping and shared lake shader.',
                  'It does not prove all rendered surface/volume samples, shoreline light paths, performance or package readiness.']}
    for name, group in manifest['groups'].items():
        start=group['start']; stop=start+group['count']
        report['groups'][name]=check_readback(probes[start:stop],captured[start:stop])
    (args.capture/'FootprintReport.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report['summary']))


if __name__=='__main__':
    main()
