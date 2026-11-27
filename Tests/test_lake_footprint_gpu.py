import sys
import unittest
from pathlib import Path
import numpy as np

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Tools'))
from ReportLakeFootprintGPU import check_readback


class LakeGPUReadbackTests(unittest.TestCase):
    def setUp(self):
        self.probes=np.array([[12,30,-2,0],[13,35,4,1],[15,32,0,.5]],dtype=float)
        self.capture=self.probes[:,[0,1,2,2,3,3]].copy()

    def test_small_interpolation_error(self):
        self.capture[2,3]=.002
        self.capture[2,5]=.502
        report=check_readback(self.probes,self.capture)
        self.assertEqual(report['count'],3)
        self.assertAlmostEqual(report['max_distance_error_m'],.002)

    def test_wrong_axis_or_missing_rows_rejected(self):
        for captured in [self.capture[:2],self.capture[:,[1,0,2,3,4,5]]]:
            with self.assertRaises(ValueError):check_readback(self.probes,captured)

    def test_wrong_gpu_distance_weight_and_nonfinite_rejected(self):
        for column,value in [(3,1),(5,.2),(3,float('nan'))]:
            capture=self.capture.copy();capture[0,column]=value
            with self.assertRaises(ValueError):check_readback(self.probes,capture)
