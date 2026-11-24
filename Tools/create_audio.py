"""Original, deterministic procedural sound placeholders. No external recordings."""
from pathlib import Path
import math, random, struct, wave
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'blender'/'exports'
OUT.mkdir(parents=True,exist_ok=True)
RATE=24000

def write(name,duration,kind):
    rng=random.Random(1948)
    data=bytearray(); low=0
    for i in range(int(RATE*duration)):
        t=i/RATE; phase=t/duration
        low=.87*low+.13*rng.uniform(-1,1)
        if kind=='breath':
            envelope=math.sin(math.pi*phase)**.6
            bubbles=.1*math.sin(2*math.pi*(190*t+7*math.sin(t*8)))*math.sin(t*31)**6
            signal=(low*.7+bubbles)*envelope
        elif kind=='ambient':
            signal=.07*low+.012*math.sin(2*math.pi*45*t)+.008*math.sin(2*math.pi*71*t)
            # Exact periodic fade at loop boundary, avoids clicks.
            signal*=min(1,t/.1,(duration-t)/.1)
        else:
            envelope=math.sin(math.pi*min(1,(t% .22)/.12))**2 if t%.22<.12 else 0
            signal=.12*math.sin(2*math.pi*920*t)*envelope
        data.extend(struct.pack('<h',int(max(-1,min(1,signal))*32767)))
    with wave.open(str(OUT/(name+'.wav')),'wb') as f:
        f.setnchannels(1);f.setsampwidth(2);f.setframerate(RATE);f.writeframes(data)

write('A_Breath',1.6,'breath')
write('A_Ambience',8,'ambient')
write('A_Warning',.44,'warning')
print('DIVE_AUDIO_OK')
