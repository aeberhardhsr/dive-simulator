"""Package verified candidate bytes, retaining explicit source/model distinctions."""
import hashlib
import json
from pathlib import Path
import struct
import numpy as np

ROOT=Path(__file__).resolve().parents[1]


def main():
    stage=ROOT/'Geodata/Processed/Vegetation'
    report=json.loads((stage/'candidates.json').read_text())
    raw=(stage/'candidates.trees').read_bytes()
    if hashlib.sha256(raw).hexdigest()!=report['sha256'] or raw[:4]!=b'DVT1':raise ValueError('Candidate checksum failed')
    count=struct.unpack_from('<I',raw,4)[0]
    if len(raw)!=8+24*count or count!=report['count']:raise ValueError('Candidate size failed')
    records=np.frombuffer(raw,dtype=np.dtype([('e','<f4'),('n','<f4'),('h','<f4'),('z','<f4'),('seed','<u4'),('kind','u1'),('source','u1'),('pad','<u2')]),offset=8)
    for field in ('e','n','h','z'):
        if not np.isfinite(records[field]).all():raise ValueError('Nonfinite candidate')
    if not np.isin(records['kind'],[0,6,12,13,14]).all() or not np.isin(records['source'],[0,1]).all() or records['pad'].any():raise ValueError('Invalid provenance')
    audit=json.loads((ROOT/'Documentation/VEGETATION_SOURCE_AUDIT.json').read_text())
    if audit['status']!='PASS' or audit['manifest_sha256']!=report['source_manifest_sha256']:raise ValueError('Source audit stale')
    dest=ROOT/'divingsim/Content/Diving/Geodata'
    (dest/'vegetation.trees').write_bytes(raw)
    report.update(file='vegetation.trees',md5=hashlib.md5(raw).hexdigest())
    (dest/'vegetation.json').write_text(json.dumps(report,indent=2)+'\n')
    (ROOT/'Documentation/VEGETATION_PLACEMENT.json').write_text(json.dumps(report,indent=2)+'\n')
    print('VEGETATION_PACKAGE_PASS',count)


if __name__=='__main__':main()
