"""Signal-level tests of the original generator; no Unreal or binary assets needed."""
import importlib.util
import math
from pathlib import Path
import unittest

spec=importlib.util.spec_from_file_location('create_audio',Path(__file__).parents[1]/'Tools/create_audio.py')
audio=importlib.util.module_from_spec(spec)
spec.loader.exec_module(audio)

class AudioTests(unittest.TestCase):
    def test_sources_have_headroom_and_no_dc_offset(self):
        for name,(duration,loop,stereo) in audio.SOUNDS.items():
            with self.subTest(name=name):
                signal=audio.channel(name,duration,loop,1948)
                rms=math.sqrt(sum(v*v for v in signal)/len(signal))
                self.assertGreater(rms,.005)
                self.assertLess(max(map(abs,signal)),.95)
                self.assertLess(abs(sum(signal)/len(signal)),.003)
                if not loop:
                    self.assertEqual(signal[0],0)
                    self.assertLess(abs(signal[-1]),.001)
                else:
                    # The wrap step must fit within this source's normal changes.
                    max_step=max(abs(a-b) for a,b in zip(signal,signal[1:]))
                    self.assertLessEqual(abs(signal[-1]-signal[0]),max_step+.0001)

    def test_stereo_ambience_is_decorrelated_and_reproducible(self):
        left=audio.channel('A_Surface',1,True,1948)
        right=audio.channel('A_Surface',1,True,2945)
        self.assertEqual(left,audio.channel('A_Surface',1,True,1948))
        correlation=sum(a*b for a,b in zip(left,right))/math.sqrt(sum(v*v for v in left)*sum(v*v for v in right))
        self.assertLess(abs(correlation),.15)

if __name__=='__main__': unittest.main()
