"""Plot validated soak intervals. Requires matplotlib; does not exclude slow frames."""
import argparse
import json
from datetime import datetime, timedelta
from pathlib import Path

import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

from analyze_soak import validate


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('directory', type=Path)
    parser.add_argument('output', type=Path)
    parser.add_argument('--background-start-utc', help='Observed start of an additional user game, ISO 8601')
    args = parser.parse_args()
    read = lambda name: json.loads((args.directory / name).read_text(encoding='utf-8-sig'))
    result = read('Result.json')
    host = read('Host.json')
    intervals = [json.loads(line) for line in (args.directory / 'Intervals.jsonl').read_text(encoding='utf-8-sig').splitlines()]
    validate(result, intervals, read('DiveLog.json'))
    rows = intervals + [result]
    start = datetime.fromisoformat(intervals[0]['utc']) - timedelta(seconds=intervals[0]['total']['wall_seconds'])
    elapsed = [row['total']['wall_seconds'] / 60 for row in rows]
    duration = elapsed[-1]

    fig, axes = plt.subplots(4, 1, sharex=True, figsize=(13, 9), layout='constrained')
    fig.suptitle(f'{duration:.0f} Minuten gerendertes Spiel: Stabilität und beobachtete Last', fontsize=17)
    axes[0].set_title(f"{result['location']} / {result['mode']} · {result['width']} × {result['height']} · Qualität {result['quality']} · {result['gpu']}\n"
                      f"Paket-SHA {host['executable_sha256'][:12]} · Rahmenbedingungen siehe Aktivitätsprotokoll · kein isolierter FPS-Benchmark", fontsize=10)
    series = [([row['interval']['average_fps'] for row in rows], 'FPS je Berichtsintervall', '#126c8c'),
              ([row['interval']['maximum_ms'] for row in rows], 'Längstes Bildintervall (ms)', '#ad5925'),
              ([row['memory_mb'] for row in rows], 'Prozessspeicher (MiB)\nabgetastetes Working Set', '#634e91'),
              ([row['successful_autosaves'] for row in rows], 'Erfolgreiche\nZwischensicherungen', '#2e795b')]
    for ax, (values, label, color) in zip(axes, series):
        ax.plot(elapsed, values, color=color, linewidth=1.2)
        ax.set_ylabel(label, fontsize=9)
        ax.set_ylim(bottom=0)
        ax.grid(alpha=.2)
        ax.spines[['top', 'right']].set_visible(False)
        if args.background_start_utc:
            begin = (datetime.fromisoformat(args.background_start_utc) - start).total_seconds() / 60
            ax.axvspan(max(0, begin), duration, color='#365a84', alpha=.07)
    axes[0].axhline(60, color='#9b2b35', linestyle='--', linewidth=.8, label='60 FPS')
    axes[0].legend(loc='upper right', fontsize=8)
    axes[1].axhline(33.333, color='#9b2b35', linestyle='--', linewidth=.8)
    for minute in range(10, int(duration), 10):
        axes[1].axvline(minute, color='#888888', linestyle=':', linewidth=.65)
    axes[1].set_title('Gepunktet: planmäßige Zehn-Minuten-Aufnahmen; Verzögerungen bleiben enthalten',
                      loc='left', fontsize=8)
    activity_path = args.directory / 'DevelopmentActivity.jsonl'
    if activity_path.exists():
        pending = {}
        for line in activity_path.read_text(encoding='utf-8-sig').splitlines():
            event = json.loads(line)
            name = event.get('event', '')
            moment = (datetime.fromisoformat(event['utc']) - start).total_seconds() / 60
            for suffix in ('_started', '_begin'):
                if name.endswith(suffix):
                    pending[name[:-len(suffix)]] = moment
            for suffix in ('_finished', '_end'):
                if name.endswith(suffix) and name[:-len(suffix)] in pending:
                    begin = pending.pop(name[:-len(suffix)])
                    axes[0].axvspan(max(0, begin), min(duration, moment), ymin=.96, ymax=1, color='#c87927', alpha=.7)
    axes[-1].set_xlim(0, duration)
    axes[-1].set_xlabel('Gemessene Laufzeit (Minuten)')
    background_label = ' Blau hinterlegt: zusätzliches Benutzerspiel.' if args.background_start_utc else ''
    fig.supxlabel('Orange: Aktivitätspaare mit Start und Ende; weitere gleichzeitige Last siehe Protokoll.' + background_label + '\n'
                  'Speicherwerte sind Stichproben unter Betriebssystem-Speicherdruck; kein Nachweis von Speicherleckfreiheit.', fontsize=9)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    fig.savefig(args.output, dpi=140)
    plt.close(fig)
    print(args.output)


if __name__ == '__main__':
    main()
