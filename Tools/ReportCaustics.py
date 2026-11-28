"""Check controlled rendered light-function comparisons, not shader text."""
import argparse
import hashlib
import json
from pathlib import Path
import re

import numpy as np
from PIL import Image


def sha(path):
    with path.open('rb') as source:
        return hashlib.file_digest(source,'sha256').hexdigest()


def report(folder):
    host=json.loads((folder/'Host.json').read_text(encoding='utf-8-sig'))
    assert sha(Path(host['executable'])).lower()==host['executable_sha256'].lower()
    if host['packaged']:
        assert host['package_archives']
        for archive in host['package_archives']:
            assert sha(Path(archive['path'])).lower()==archive['sha256'].lower()
    log=(folder/'Runtime.log').read_text(encoding='utf-8-sig')
    assert 'DIVE_CAUSTICS_PROBE_PASS captures=4' in log
    assert not re.search(r'DIVE_\w+_FAIL|Failed to compile Material|Default Material will be used',log)
    captures=re.findall(r'DIVE_CAUSTICS_CAPTURE phase=(\d+) wind=(\d+) night=(\d+) air=(\d+) deep=(\d+) strength=(\d+) clock=([\d.]+) camera=([^\r\n]+)',log)
    assert len(captures)==4 and [int(c[0]) for c in captures]==list(range(4))
    expected={'day':('6','0','0','0'),'calm':('0','0','0','0'),'night':('6','1','0','0'),
              'air':('6','0','1','0'),'deep':('6','0','0','1')}[host['case']]
    assert all(tuple(c[1:5])==expected for c in captures)
    assert [int(c[5]) for c in captures]==[1,0,1,1]
    assert [float(c[6]) for c in captures]==[20,20,22,22]
    assert len({c[7] for c in captures})==1
    images=[];evidence=[]
    for i in range(4):
        path=folder/f'Caustics_{i:02d}.png'
        with Image.open(path) as image:
            assert image.size==(1600,900)
            images.append(np.asarray(image.convert('RGB'),dtype=np.float64))
        evidence.append(dict(file=path.name,sha256=sha(path)))
    # Fixed interior regions selected from the receiver geometry, away from
    # silhouette edges, the buoy line, and background wildlife/particles.
    regions={'lit':(250,580,1350,820),'shadow':(900,410,1200,470),'sphere':(350,15,520,75)}
    metrics={}
    for label,i,j in [('enabled_vs_disabled',0,1),('moving_pattern',0,2),('frozen_clock',2,3)]:
        difference=np.abs(images[i]-images[j])
        metrics[label]={name:float(difference[y0:y1,x0:x1].mean()) for name,(x0,y0,x1,y1) in regions.items()}
    case=host['case'];delta=metrics['enabled_vs_disabled']
    if case=='day':
        assert delta['lit']>2 and delta['sphere']>1,delta
        assert delta['shadow']<max(.75,delta['lit']*.2),delta
        assert metrics['moving_pattern']['lit']>2,metrics
    if case in ('calm','night','air'):
        assert delta['lit']<.75 and delta['sphere']<.75,delta
    assert metrics['frozen_clock']['lit']<.75,metrics
    return dict(status='PASS',directory=str(folder.resolve()),host=host,mean_absolute_rgb8_difference=metrics,
                regions_xyxy=regions,images=evidence,log_sha256=sha(folder/'Runtime.log'))


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directories',type=Path,nargs='+')
    parser.add_argument('--output',type=Path)
    args=parser.parse_args()
    runs=[report(p) for p in args.directories]
    assert len({r['host']['executable_sha256'] for r in runs})==1
    kinds={r['host']['case']:r for r in runs}
    if 'day' in kinds and 'deep' in kinds:
        assert kinds['deep']['mean_absolute_rgb8_difference']['enabled_vs_disabled']['lit']<kinds['day']['mean_absolute_rgb8_difference']['enabled_vs_disabled']['lit']
    if args.output:
        args.output.write_text(json.dumps(dict(status='PASS',runs=runs,limits=[
            'Controlled synthetic receivers test light modulation and shadow exclusion; they do not establish photorealism.',
            'RGB8 differences include temporal rendering noise and are not radiometric measurements.',
            'The single-ray approximation is bounded and does not solve multiray focusing or refracted shadows.']),indent=2)+'\n',encoding='utf-8')
    for run in runs:print('PASS',run['host']['case'],run['mean_absolute_rgb8_difference'])


if __name__=='__main__':main()
