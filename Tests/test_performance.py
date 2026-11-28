import csv
import importlib.util
import json
import tempfile
import unittest
from pathlib import Path

spec = importlib.util.spec_from_file_location("perf", Path(__file__).parents[1] / "Tools/analyze_performance.py")
perf = importlib.util.module_from_spec(spec)
spec.loader.exec_module(perf)


class PerformanceTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name)

    def fixture(self, hitch=None, truncated=False):
        header = ["EVENTS", "FrameTime", "GameThreadTime", "RenderThreadTime", "RHIThreadTime", "GPUTime",
                  "DivePerf/DepthM", "DivePerf/TerrainChunks", "DivePerf/DistanceM", "DivePerf/ProcessMemoryMB"]
        data = [["", 10, 2, 8, 1, 8, 5, 9, i, 1000] for i in range(102)]
        if hitch is not None:
            data[50][1] = hitch
        with (self.path / "Capture.csv").open("w", newline="", encoding="utf-8") as out:
            writer = csv.writer(out)
            writer.writerow(header)
            writer.writerows(data)
            if not truncated:
                writer.writerow(header + ["GPU/LatePass", "TextureStreaming/Update", "TextureStreaming/Update"])
                writer.writerow(["[HasHeaderRowAtEnd]", "1"])
        duration = sum(float(row[1]) for row in data) / 1000 if hitch != "nan" else 1.02
        (self.path / "Metadata.json").write_text(json.dumps({"scenario": "perf-underwater", "requested_capture_seconds": duration}), encoding="utf-8")
        (self.path / "Host.json").write_text('{"background_processes": []}', encoding="utf-8")

    def test_average_and_percentiles(self):
        self.fixture()
        result = perf.analyze(self.path)
        self.assertEqual(result["frames"], 100)
        self.assertAlmostEqual(result["average_fps"], 100)
        self.assertEqual(result["frame"]["p99_ms"], 10)
        self.assertTrue(result["all_frames_within_30fps_budget"])

    def test_interior_hitch_is_not_discarded(self):
        self.fixture(500)
        result = perf.analyze(self.path)
        self.assertEqual(result["frame"]["max_ms"], 500)
        self.assertEqual(result["one_percent_low_fps"], 2)
        self.assertEqual(result["frames_over_100_ms"], 1)
        self.assertFalse(result["all_frames_within_30fps_budget"])

    def test_incomplete_capture_is_rejected(self):
        self.fixture(truncated=True)
        with self.assertRaises(ValueError):
            perf.analyze(self.path)

    def test_ambiguous_required_stat_is_rejected(self):
        self.fixture()
        path = self.path / "Capture.csv"
        with path.open(newline="", encoding="utf-8") as stream:
            rows = list(csv.reader(stream))
        rows[-2].append("FrameTime")
        with path.open("w", newline="", encoding="utf-8") as stream:
            csv.writer(stream).writerows(rows)
        with self.assertRaises(ValueError):
            perf.analyze(self.path)

    def test_nonfinite_frame_is_rejected(self):
        self.fixture("nan")
        with self.assertRaises(ValueError):
            perf.analyze(self.path)


if __name__ == "__main__":
    unittest.main()
