"""Create timestamped review animations and frame-change evidence for water crossings."""
import argparse
import json
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('run', type=Path)
    parser.add_argument('--baseline', type=Path, help='Compare matching simulation frames from another completed sweep')
    parser.add_argument('--skip-animation', action='store_true', help='Refresh evidence without encoding GIF files again')
    args = parser.parse_args()
    cases = json.loads((args.run/'Cases.json').read_text(encoding='utf-8-sig'))
    if isinstance(cases, dict):
        cases = [cases]
    baseline_cases = {}
    if args.baseline:
        loaded = json.loads((args.baseline/'Cases.json').read_text(encoding='utf-8-sig'))
        if isinstance(loaded, dict):
            loaded = [loaded]
        baseline_cases = {case['name']: case for case in loaded}
    results = []
    for case in cases:
        if not case['options'].get('WaterlineSweep') or case['status'] != 'PASS':
            raise ValueError('Expected a completed native sweep')
        folder = args.run/case['name']
        files = sorted(folder.glob('WaterSweep_*.png'))
        if len(files) != 201:
            raise ValueError('Incomplete image sequence')
        times = [float(x.split(' seconds=')[1].split()[0]) for x in case['captures']]
        if len(times) != 201 or any(abs(t-i*.1) > .00001 for i,t in enumerate(times)):
            raise ValueError('Unexpected simulation times')
        if args.baseline:
            baseline = baseline_cases[case['name']]
            if baseline['status'] != 'PASS' or baseline['captures'] != case['captures']:
                raise ValueError('Reference camera/wave trajectory differs')
        frames, changes, comparison = [], [], []
        previous = None
        for i, file in enumerate(files):
            with Image.open(file) as source:
                if source.size != (1600, 900):
                    raise ValueError('Unexpected native screenshot dimensions')
                img = source.convert('RGB').resize((800, 450), Image.Resampling.LANCZOS)
            pixels = np.asarray(img, dtype=np.float32)
            if args.baseline:
                with Image.open(args.baseline/case['name']/file.name) as reference:
                    if reference.size != (1600, 900):
                        raise ValueError('Unexpected reference dimensions')
                    reference_pixels = np.asarray(reference.convert('RGB').resize((800, 450), Image.Resampling.LANCZOS), dtype=np.float32)
                comparison.append(dict(frame=i, seconds=times[i], rgb_mae=float(np.abs(pixels-reference_pixels).mean())))
            if previous is not None:
                changes.append(dict(frame=i, seconds=times[i], rgb_mae=float(np.abs(pixels-previous).mean())))
            previous = pixels
            ImageDraw.Draw(img).text((8,8),f"{case['name']} / {times[i]:.2f} s",fill='white',stroke_width=2,stroke_fill='black')
            frames.append(img)
        durations = [max(10,round(1000*(b-a))) for a,b in zip(times,times[1:])]+[100]
        if not args.skip_animation:
            frames[0].save(folder/'Sweep.gif',save_all=True,append_images=frames[1:],duration=durations,loop=0)
        # One frame per second plus the largest adjacent changes. Originals stay
        # intact; the sheet is only a readable index into the captured sequence.
        selected = sorted(set(range(0,201,10)) | {x['frame'] for x in sorted(changes,key=lambda x:x['rgb_mae'],reverse=True)[:6]})
        sheet = Image.new('RGB',(1600,225*((len(selected)+3)//4)),(10,20,20))
        for j,i in enumerate(selected):
            sheet.paste(frames[i].resize((400,225),Image.Resampling.LANCZOS),((j%4)*400,(j//4)*225))
        sheet.save(folder/'SweepReview.jpg',quality=90)
        if comparison:
            selected_comparison = [x['frame'] for x in sorted(comparison, key=lambda x:x['rgb_mae'], reverse=True)[:6]]
            sheet = Image.new('RGB',(1600,450*len(selected_comparison)),(10,20,20))
            for row,i in enumerate(selected_comparison):
                with Image.open(args.baseline/case['name']/files[i].name) as reference:
                    img = reference.convert('RGB').resize((800,450),Image.Resampling.LANCZOS)
                ImageDraw.Draw(img).text((8,8),f'Previous material / {times[i]:.2f} s',fill='white',stroke_width=2,stroke_fill='black')
                sheet.paste(img,(0,row*450))
                sheet.paste(frames[i],(800,row*450))
            sheet.save(folder/'BaselineComparison.jpg',quality=90)
        results.append(dict(case=case['name'],frames=len(files),seconds=times[-1]-times[0],
            max_frame_gap_seconds=max(b-a for a,b in zip(times,times[1:])),changes=changes, baseline_comparison=comparison,
            scope='201 native frames at fixed 0.1 s simulation steps; 20 s simulated wave/camera motion. Wall-clock capture is slower; not a frame-rate test or every presented game frame. Review the sequence and the largest changes.'))
        print(folder/'SweepReview.jpg' if args.skip_animation else folder/'Sweep.gif')
    (args.run/'SweepReview.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')


if __name__ == '__main__':
    main()
