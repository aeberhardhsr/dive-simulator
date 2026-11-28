"""Validate the actual master mix exported by the packaged audio smoke scenario."""
import argparse, math, re, struct, wave
from pathlib import Path

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('wav',type=Path)
    parser.add_argument('log',type=Path)
    args=parser.parse_args()
    with wave.open(str(args.wav),'rb') as wav:
        rate=wav.getframerate(); channels=wav.getnchannels(); width=wav.getsampwidth()
        assert width==2, f'Expected PCM16, got {width}'
        samples=struct.unpack('<'+'h'*wav.getnframes()*channels,wav.readframes(wav.getnframes()))
    duration=len(samples)/rate/channels
    assert 38<duration<90, f'Unexpected recording duration: {duration}'
    def rms(start,end):
        chunk=samples[round(start*rate)*channels:round(end*rate)*channels]
        return math.sqrt(sum(float(v)*v for v in chunk)/len(chunk))/32768
    active=rms(2,6)
    assert active>.0005, f'No audible active mix: {active}'
    peak=max(map(abs,samples))/32768
    assert peak<.98, f'Clipped master mix: {peak}'
    # Allow device buffering and transitions: measure the middle second only.
    for label,pattern in [('pause',r'DIVE_AUDIO_PAUSE t=([\d.]+) value=1'),('mute',r'DIVE_AUDIO_MASTER t=([\d.]+) value=0.00')]:
        match=re.search(pattern,args.log.read_text(encoding='utf-8-sig',errors='replace'))
        assert match, f'Missing {label} event'
        start=float(match[1]); silent=rms(start+1,start+2)
        assert silent<max(.00002,active*.005), f'{label} was not silent: {silent}'
        print(f'{label}: {silent:.8f} RMS')
    assert rms(22,26)>.0005, 'Sound did not resume after mute'
    print(f'DIVE_MIX_PASS duration={duration:.2f}s channels={channels} rate={rate} active_rms={active:.5f} peak={peak:.4f}')

if __name__=='__main__': main()
