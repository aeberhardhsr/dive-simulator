"""Analytic and round-trip checks for the separate whole-lake coast model."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/"Tools"))
from coast_model import solve
from prepare_coast import write_tiles


class CoastInterpolation(unittest.TestCase):
    def test_recovers_plane_and_preserves_every_constraint(self):
        row, col = np.indices((31, 37))
        plane = (-10-row*.25-col*.125).astype(np.float32)
        lake = np.zeros(plane.shape, bool)
        lake[2:-2, 2:-2] = True
        surveyed = ~lake
        values = plane.copy()
        values[lake] = -.3
        result, report = solve(values, lake, surveyed)
        np.testing.assert_allclose(result, plane, atol=2e-6, rtol=0)
        np.testing.assert_array_equal(result[~lake], values[~lake])
        self.assertEqual(report["unknown_cells"], int(lake.sum()))
        self.assertLessEqual(report["stored_residual_m"], 1e-5)

    def test_island_and_low_lying_land_are_not_filled(self):
        values = np.full((19, 21), -12., np.float32)
        lake = np.zeros(values.shape, bool)
        lake[2:-2, 2:-2] = True
        lake[8:11, 9:12] = False  # Island hole with its own fixed shoreline.
        values[8:11, 9:12] = 3.5
        surveyed = np.zeros(values.shape, bool)
        surveyed[4, 4] = True
        values[4, 4] = -27.25
        result, _ = solve(values, lake, surveyed)
        fixed = ~lake | surveyed
        np.testing.assert_array_equal(result[fixed], values[fixed])
        self.assertTrue((result[lake] <= 0).all())
        self.assertGreater(float(result[7, 9]), -12.)

    def test_positive_shore_constraint_does_not_create_underwater_peak(self):
        values = np.full((5, 5), 100., np.float32)
        values[1, 2] = -20
        lake = np.zeros(values.shape, bool)
        lake[2, 2] = True
        surveyed = np.zeros(values.shape, bool)
        surveyed[1, 2] = True
        result, report = solve(values, lake, surveyed)
        self.assertEqual(result[2, 2], -5.)  # Three water-level constraints + measured -20.
        self.assertEqual(report["shore_constraints_clipped_to_water"], 3)
        np.testing.assert_array_equal(result[~lake], values[~lake])

    def test_fully_surveyed_lake_needs_no_solver(self):
        values = -np.arange(25, dtype=np.float32).reshape(5, 5)
        result, report = solve(values, np.ones((5, 5), bool), np.ones((5, 5), bool))
        np.testing.assert_array_equal(result, values)
        self.assertEqual(report["iterations"], 0)
        result[0, 0] = -99
        self.assertEqual(values[0, 0], 0)

    def test_rejects_open_edges_missing_survey_and_bad_values(self):
        values = np.full((5, 5), -3., np.float32)
        surveyed = np.zeros((5, 5), bool)
        surveyed[2, 2] = True
        with self.assertRaisesRegex(ValueError, "boundary"):
            solve(values, np.ones((5, 5), bool), surveyed)
        lake = np.zeros((5, 5), bool)
        lake[1:-1, 1:-1] = True
        with self.assertRaisesRegex(ValueError, "no surveyed"):
            solve(values, lake, np.zeros((5, 5), bool))
        for invalid in (np.full((5, 5), np.nan), np.zeros((4, 5)), np.empty((0, 0))):
            with self.assertRaises(ValueError):
                solve(invalid, lake, surveyed)
        with self.assertRaisesRegex(ValueError, "tolerance"):
            solve(values, lake, surveyed, tolerance=0)

    def test_adjacent_tile_halos_are_bit_identical_and_georeferenced(self):
        values = (np.arange(24*32, dtype=np.float32).reshape(24, 32)-1000)/8
        labels = (np.arange(24*32).reshape(24, 32) % 3+1).astype("u1")
        selected = np.zeros(values.shape, bool)
        selected[9, 9] = selected[9, 17] = True
        with tempfile.TemporaryDirectory() as directory:
            destination = Path(directory)
            tiles = write_tiles(values, labels, selected, (1000, 2000, 1032, 2024), destination, tile_size=8)
            self.assertEqual(len(tiles), 2)
            arrays = [np.fromfile(destination/t["file"], dtype="<f4").reshape(10, 10) for t in tiles]
            provenance = [np.fromfile(destination/t["provenance_file"], dtype="u1").reshape(10, 10) for t in tiles]
            np.testing.assert_array_equal(arrays[0][:, -2:], arrays[1][:, :2])
            np.testing.assert_array_equal(provenance[0][:, -2:], provenance[1][:, :2])
            np.testing.assert_array_equal(arrays[0], values[7:17, 7:17])
            self.assertEqual((tiles[0]["west"], tiles[0]["south"]), (1007, 2007))
            self.assertEqual(sum(tiles[0]["core_source_counts"].values()), 64)
            # Metadata must remain JSON serializable for independent consumers.
            self.assertEqual(len(json.loads(json.dumps(tiles))), 2)

    def test_selected_edge_tile_is_rejected_instead_of_losing_halo(self):
        with tempfile.TemporaryDirectory() as directory:
            with self.assertRaisesRegex(ValueError, "halo"):
                write_tiles(np.zeros((24, 24), "f4"), np.ones((24, 24), "u1"),
                            np.ones((24, 24), bool), (0, 0, 24, 24), Path(directory), tile_size=8)


if __name__ == "__main__":
    unittest.main()
