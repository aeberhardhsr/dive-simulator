import io
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / 'Tools'))
from prepare_bathymetry import parse_grid

HEADER = 'ncols 2\nnrows 2\nxllcorner 2726000\nyllcorner 1221000\ncellsize 1\nnodata_value -9999\n'


class GridTests(unittest.TestCase):
    def test_north_to_south_and_nodata_preserved(self):
        header, values = parse_grid(io.StringIO(HEADER + '410.5 -9999\n409 408\n'))
        self.assertEqual(list(values), [410.5, -9999, 409, 408])
        self.assertEqual(header['xllcorner'], 2726000)

    def test_reject_truncated_grid(self):
        with self.assertRaises(ValueError):
            parse_grid(io.StringIO(HEADER + '410 409\n'))

    def test_reject_wrong_row_width(self):
        with self.assertRaises(ValueError):
            parse_grid(io.StringIO(HEADER + '410 409 408\n407\n'))


if __name__ == '__main__':
    unittest.main()
