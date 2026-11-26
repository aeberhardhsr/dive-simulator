import copy
import hashlib
import importlib.util
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location('soak', Path(__file__).parents[1] / 'Tools/analyze_soak.py')
soak = importlib.util.module_from_spec(spec)
spec.loader.exec_module(soak)


class SoakTests(unittest.TestCase):
    def test_run_package_must_remain_identical(self):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder) / 'divingsim'
            executable = root / 'Binaries/Win64/divingsim.exe'
            executable.parent.mkdir(parents=True)
            executable.write_bytes(b'original binary')
            pak = root / 'Content/Paks/divingsim.utoc'
            pak.parent.mkdir(parents=True)
            pak.write_bytes(b'original content')
            host = dict(executable=str(executable), executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                        package_containers=[dict(file=pak.name, bytes=pak.stat().st_size, sha256=hashlib.sha256(pak.read_bytes()).hexdigest())])
            self.assertEqual(soak.verify_package(host), 'executable_and_cooked_containers')
            # Same-sized replacement still invalidates the run.
            pak.write_bytes(b'modified content')
            with self.assertRaisesRegex(ValueError, 'Cooked package'): soak.verify_package(host)
            pak.write_bytes(b'original content')
            executable.write_bytes(b'modified binary')
            with self.assertRaisesRegex(ValueError, 'Executable'): soak.verify_package(host)
            executable.write_bytes(b'original binary')
            (pak.parent / 'unexpected.pak').write_bytes(b'new asset')
            with self.assertRaisesRegex(ValueError, 'Cooked package'): soak.verify_package(host)

    def fixture(self, seconds=180):
        def metrics(seconds):
            return dict(frames=seconds*100, wall_seconds=seconds, average_fps=100 if seconds else 0,
                        maximum_ms=10 if seconds else 0, p95_upper_ms=10.25 if seconds else 0,
                        p99_upper_ms=10.25 if seconds else 0, over_16_67_ms=0, over_33_33_ms=0, over_100_ms=0)
        result = dict(final=True, outcome='smoke_soak_completed', requested_seconds=seconds,
                      measured_simulation_seconds=seconds, simulation_seconds=seconds+18, width=1920, height=1080, quality=2,
                      turns=1, visited_chunk_centers=3, distance_m=140, successful_autosaves=seconds//30,
                      terrain_chunks=9, failed_autosaves=0, pending_autosaves=0, samples=seconds+19,
                      surface_reached=True, minimum_depth_m=0, maximum_depth_m=8,
                      total=metrics(seconds), interval=metrics(0))
        for key, value in [('r.ScreenPercentage', 100), ('r.SecondaryScreenPercentage.GameViewport', 100),
                           ('r.DynamicRes.OperationMode', 0), ('r.VSync', 0), ('t.MaxFPS', 0)]: result[key] = value
        for name in ['ViewDistance', 'AntiAliasing', 'Shadow', 'GlobalIllumination', 'Reflection', 'PostProcess', 'Texture', 'Effects', 'Foliage']:
            result[f'sg.{name}Quality'] = 2
        rows = []
        for i in range(1, seconds//30+1):
            row = copy.deepcopy(result); row.update(final=False, total=metrics(i*30), interval=metrics(30)); rows.append(row)
        log = dict(is_test=True, status='smoke_soak_completed', duration_seconds=seconds+18, samples=[{'seconds': t} for t in range(seconds+19)])
        return result, rows, log

    def test_complete_and_hitch_accounting(self):
        result, rows, log = self.fixture()
        soak.validate(result, rows, log)
        for data in [rows[2]['interval']] + [r['total'] for r in rows[2:]] + [result['total']]:
            data.update(maximum_ms=150, over_16_67_ms=1, over_33_33_ms=1, over_100_ms=1)
        soak.validate(result, rows, log)
        result['total']['over_100_ms'] = 0
        with self.assertRaises(ValueError): soak.validate(result, rows, log)

    def test_both_clocks_required(self):
        for key in ['requested_seconds', 'measured_simulation_seconds']:
            result, rows, log = self.fixture()
            result[key] = 7200 if key == 'requested_seconds' else 179
            with self.assertRaises(ValueError): soak.validate(result, rows, log)

    def test_surface_cycle_required(self):
        result, rows, log = self.fixture(300)
        soak.validate(result, rows, log)
        result['surface_reached'] = False
        with self.assertRaises(ValueError): soak.validate(result, rows, log)

    def test_no_lost_intervals(self):
        result, rows, log = self.fixture()
        rows.pop(2)
        with self.assertRaises(ValueError): soak.validate(result, rows, log)

    def test_bad_state_or_incomplete_recording(self):
        for key, value in [('failed_autosaves', 1), ('pending_autosaves', 2), ('terrain_chunks', 10),
                           ('visited_chunk_centers', 1), ('r.ScreenPercentage', 70), ('samples', 198)]:
            result, rows, log = self.fixture(); result[key] = value
            with self.subTest(key=key), self.assertRaises(ValueError): soak.validate(result, rows, log)


if __name__ == '__main__': unittest.main()
