"""Independent analytic checks of the survey-to-texture coordinate contract."""
import sys
import unittest
from pathlib import Path

import numpy as np
from shapely.geometry import Polygon

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'Tools'))
from lake_footprint import generate, layout, sample


def rectangle_signed(x, y, bounds):
    left, bottom, right, top = bounds
    outside = np.hypot(np.maximum(np.maximum(left-x, x-right), 0),
                       np.maximum(np.maximum(bottom-y, y-top), 0))
    inside = np.minimum(np.minimum(x-left, right-x), np.minimum(y-bottom, top-y))
    return np.where(inside >= 0, inside, -outside)


class LakeFootprintTests(unittest.TestCase):
    def setUp(self):
        self.lake = Polygon([(100, 100), (120, 100), (120, 112), (100, 112)],
                            [[(108, 104), (112, 104), (112, 108), (108, 108)]])
        self.grid = layout(self.lake, spacing=1, limit=4)

    def raster(self, tile_size):
        result = np.empty((self.grid['rows'], self.grid['columns']), dtype='<f2')
        generate(self.lake, self.grid, result, tile_size=tile_size)
        return result

    def test_distances_against_rectangle_with_island(self):
        g = self.grid
        x, y = np.meshgrid(g['west']+np.arange(g['columns'])+.5,
                           g['north']-np.arange(g['rows'])-.5)
        analytic = np.minimum(rectangle_signed(x, y, (100, 100, 120, 112)),
                              -rectangle_signed(x, y, (108, 104, 112, 108)))
        expected = np.clip(analytic, -4, 4).astype('<f2')
        np.testing.assert_array_equal(self.raster(7), expected)
        # Cross tile boundaries, including tiles containing no shoreline.
        np.testing.assert_array_equal(self.raster(2), expected)

    def test_sampling_axes_half_texel_and_islands(self):
        actual = sample(self.grid, self.raster(7),
                        [99.2, 100.8, 110, 116, 120.8], [102, 102, 106, 106, 109])
        np.testing.assert_allclose(actual, [-.8, .8, -1.5, 3.5, -.8], atol=1e-6)
        # The island centre is smoothed over four centres; no claim of exact
        # distance between texels (nor at the wet ridge halfway to the shore).
        # Its dry classification must remain intact.
        np.testing.assert_array_equal(sample(self.grid, self.raster(7), [-1e6, 1e6], [106, 106]), [-4, -4])

    def test_dimension_and_numeric_rejection(self):
        for spacing in [0, -1, float('nan')]:
            with self.assertRaises(ValueError):
                layout(self.lake, spacing=spacing)
        with self.assertRaises(ValueError):
            layout(self.lake, spacing=.0001)
        with self.assertRaises(ValueError):
            sample(self.grid, self.raster(7), float('nan'), 106)


if __name__ == '__main__':
    unittest.main()
