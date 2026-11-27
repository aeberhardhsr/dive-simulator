"""Reject invalid native comparison evidence; no synthetic visual acceptance."""
import sys
from pathlib import Path
import unittest

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'Tools'))
from ReportWaveTransport import differences, validate_metadata


def example():
    host = dict(case='day', wave_transport_study=True, packaged=False, water_preview=True,
                solar_shader_study=True, solar_volume_study=True)
    log = '\n'.join(['DIVE_CAUSTICS_PROBE_PASS captures=4', 'DIVE_WATER_PREVIEW_READY',
                     'DIVE_SOLAR_SHADER_STUDY_READY', 'DIVE_SOLAR_VOLUME_STUDY_READY',
                     'DIVE_WATER_SCENE_READY waves=4'] + [
        f'DIVE_CAUSTICS_CAPTURE phase={i} wind=6 night=0 air=0 deep=0 strength={strength} '
        f'clock={clock} camera=X=100.000 Y=200.000 Z=-85.000'
        for i, (strength, clock) in enumerate([(1, 20), (0, 20), (1, 22), (1, 22)])])
    return log, host


class WaveCaptureReportTests(unittest.TestCase):
    def test_metadata_controls(self):
        log, host = example()
        self.assertEqual(len(validate_metadata(log, host, 'day', True)), 4)
        for malformed in [log.replace('phase=3', 'phase=2'),
                          log.replace('clock=22', 'clock=23'),
                          log.replace('wind=6', 'wind=0'),
                          log.replace('strength=0', 'strength=1'),
                          log.replace('DIVE_WATER_SCENE_READY waves=4', '')]:
            with self.subTest(log=malformed), self.assertRaises(ValueError):
                validate_metadata(malformed, host, 'day', True)

    def test_failure_overrides_success_marker(self):
        log, host = example()
        for failure in ['unbound parameters Scene', 'Fatal error: shader',
                        'Failed to compile Material', 'DIVE_OTHER_FAIL',
                        'DIVE_SPECTRAL_VIEW_READY']:
            with self.subTest(failure=failure), self.assertRaises(ValueError):
                validate_metadata(log+'\n'+failure, host, 'day', True)

    def test_camera_change_rejected(self):
        log, host = example()
        log = log.replace('Z=-85.000', 'Z=-86.000', 1)
        with self.assertRaisesRegex(ValueError, 'Camera moved'):
            validate_metadata(log, host, 'day', True)

    def test_requested_mode_must_match_binding(self):
        log, host = example()
        host['wave_transport_study'] = False
        with self.assertRaisesRegex(ValueError, 'water scene binding'):
            validate_metadata(log, host, 'day', False)
        log = log.replace('DIVE_WATER_SCENE_READY waves=4', '')
        self.assertEqual(len(validate_metadata(log, host, 'day', False)), 4)

    def test_signed_pixel_differences_do_not_wrap(self):
        before = np.full((900, 1600, 3), [250, 10, 100], dtype=np.uint8)
        after = np.full((900, 1600, 3), [10, 20, 100], dtype=np.uint8)
        for metric in differences(before, after).values():
            self.assertAlmostEqual(metric['mean_absolute_rgb8'], 250/3, places=4)
            np.testing.assert_allclose(metric['mean_signed_rgb8'], [-240, 10, 0])


if __name__ == '__main__':
    unittest.main()
