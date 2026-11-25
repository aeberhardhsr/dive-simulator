"""Original deterministic 48 kHz PCM sound design; no external recordings.
Loop noise uses an overlap crossfade; environment layers are stereo.
"""
from pathlib import Path
from array import array
import math, random, sys, wave
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'blender'/'exports'
RATE=48000
SOUNDS={
    'A_Inhale':(1.45,False,False), 'A_Exhale':(1.65,False,False),
    'A_Bubbles':(2.2,False,False), 'A_Inflate':(3,True,False),
    'A_Vent':(3,True,False), 'A_Heart':(.34,False,False),
    'A_Underwater':(12,True,True), 'A_Surface':(12,True,True),
    'A_Wind':(12,True,True), 'A_Rain':(12,True,True),
    'A_Warning':(.44,False,False),
}

def channel(name,duration,loop,seed):
    rng=random.Random(seed)
    n=round(duration*RATE); overlap=round(.25*RATE) if loop else 0
    samples=array('d'); low=slow=0.
    bubbles=[]
    if name=='A_Bubbles':
        for j in range(22):
            bubbles.append((rng.uniform(0,duration-.2),rng.uniform(150,600),rng.uniform(.03,.1)))
    for i in range(n+overlap):
        t=i/RATE; noise=rng.uniform(-1,1)
        low+=.12*(noise-low); slow+=.007*(noise-slow)
        if name=='A_Inhale':
            env=math.sin(math.pi*min(1,t/duration))**.65
            signal=(.24*(noise-low)+.3*low)*env*(.8+.2*math.sin(t*13)**2)
        elif name=='A_Exhale':
            env=math.sin(math.pi*min(1,t/duration))**.8
            signal=(.7*low+.07*noise)*env*(.6+.4*math.sin(t*29)**4)
        elif name=='A_Bubbles':
            signal=0
            for start,freq,decay in bubbles:
                age=t-start
                if 0<=age<decay*6:
                    signal+=.12*math.sin(2*math.pi*(freq*age+180*age*age))*math.exp(-age/decay)*min(1,age/.003)
        elif name=='A_Inflate': signal=.24*(noise-low)+.13*low
        elif name=='A_Vent': signal=.65*low*(.65+.35*math.sin(t*43)**2)+.035*noise
        elif name=='A_Heart':
            signal=0
            for start,level,freq in [(0,.42,62),(.14,.27,77)]:
                age=t-start
                if age>=0: signal+=level*math.sin(2*math.pi*freq*age)*math.exp(-age*28)*min(1,age/.008)
        elif name=='A_Underwater':
            signal=.8*slow+.035*math.sin(2*math.pi*42*t)+.02*math.sin(2*math.pi*67*t)
        elif name=='A_Surface': signal=(.65*low+.03*noise)*(.55+.45*math.sin(t*1.3+math.sin(t*.7))**2)
        elif name=='A_Wind': signal=(1.2*slow+.12*low)*(.6+.4*math.sin(t*.6+.7)**2)
        elif name=='A_Rain': signal=(.18*noise+.25*low)*(.85+.15*math.sin(t*1.8)**2)
        else:
            pulse=t%.22
            signal=.22*math.sin(2*math.pi*920*t)*math.sin(math.pi*pulse/.12)**2 if pulse<.12 else 0
        if not loop: signal*=min(1,t/.005,max(0,(duration-t)/.02))
        samples.append(signal)
    if loop:
        # First sample follows the tail; crossfade into the original head.
        for i in range(overlap):
            a=i/overlap
            samples[i]=samples[n+i]*(1-a)+samples[i]*a
    return samples[:n]

def main():
    OUT.mkdir(parents=True,exist_ok=True)
    for index,(name,(duration,loop,stereo)) in enumerate(SOUNDS.items()):
        channels=[channel(name,duration,loop,1948+index*31+c*997) for c in range(2 if stereo else 1)]
        pcm=array('h')
        for frame in zip(*channels):
            for value in frame:
                if abs(value)>.95: raise RuntimeError(f'Clipping headroom exceeded: {name}')
                pcm.append(round(value*32767))
        if sys.byteorder!='little': pcm.byteswap()
        with wave.open(str(OUT/(name+'.wav')),'wb') as f:
            f.setnchannels(len(channels)); f.setsampwidth(2); f.setframerate(RATE); f.writeframes(pcm.tobytes())
        print(name, duration, 'loop' if loop else 'one-shot', 'stereo' if stereo else 'mono')
    print('DIVE_AUDIO_OK')

if __name__=='__main__': main()
