import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'Tools'))
from ReportFPVProbe import cast_rays


class FPVProbeTests(unittest.TestCase):
    def mesh(self, depth=20):
        return {'horizontal_fov_degrees': 90,
                'vertices': [[depth, y, z, 'forearm', 46, 54, 58]
                             for y, z in [(-10, -10), (10, -10), (0, 10)]],
                'indices': [0, 1, 2]}

    def test_known_center_intersection_and_outside(self):
        hits = cast_rays(self.mesh(), 1920, 1080, [(960, 540), (0, 0)])
        self.assertAlmostEqual(hits[0]['depth_cm'], 20)
        self.assertEqual(hits[0]['bones'], ['forearm']*3)
        self.assertFalse(hits[1]['hit'])

    def test_behind_and_near_plane_excluded(self):
        for depth in [-20, 5]:
            self.assertFalse(cast_rays(self.mesh(depth), 1920, 1080, [(960, 540)])[0]['hit'])

    def test_same_ray_at_different_resolution_and_invalid_index(self):
        a = cast_rays(self.mesh(), 1920, 1080, [(1100, 600)])[0]
        b = cast_rays(self.mesh(), 960, 540, [(550, 300)])[0]
        self.assertEqual(a['depth_cm'], b['depth_cm'])
        mesh = self.mesh(); mesh['indices'] = [0, 1, 99]
        with self.assertRaises(ValueError):
            cast_rays(mesh, 1920, 1080, [(960, 540)])
