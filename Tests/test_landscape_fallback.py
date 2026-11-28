import importlib.util
from pathlib import Path
import unittest

import numpy as np

spec = importlib.util.spec_from_file_location('fallback_audit', Path(__file__).resolve().parents[1]/'Tools/audit_landscape_fallback.py')
audit = importlib.util.module_from_spec(spec)
spec.loader.exec_module(audit)


class FallbackBoundaryTests(unittest.TestCase):
    def test_attribute_splits_do_not_create_false_boundaries(self):
        # The two triangles have separate render vertices along their diagonal.
        v = np.array([[0, 0, 0], [1, 0, 0], [0, 1, 0],
                      [1, 0, 0], [1, 1, 0], [0, 1, 0]], dtype=float)
        edges = audit.open_edges(v, np.array([[0, 1, 2], [3, 4, 5]]))
        self.assertEqual(len(edges), 4)

    def test_collinear_subdivision_has_no_seam(self):
        coarse = np.array([[[-10, 0, 1], [0, 0, 11]]], dtype=float)
        fine = np.array([[[-10, 0, 1], [-4, 0, 7]], [[-4, 0, 7], [0, 0, 11]]], dtype=float)
        x = np.array([-9.5, -4, -1.5])
        np.testing.assert_allclose(audit.section(coarse, 0, x), audit.section(fine, 0, x))

    def test_shifted_boundary_remains_detectable(self):
        edges = np.array([[[-10, 7, 30], [0, 7, 40]]], dtype=float)
        np.testing.assert_allclose(audit.section(edges, 0, np.array([-5.])), [[7, 35]])

    def test_missing_boundary_is_not_a_zero_gap(self):
        edges = np.array([[[-10, 0, 1], [-6, 0, 5]]], dtype=float)
        with self.assertRaises(ValueError):
            audit.section(edges, 0, np.array([-5.]))


if __name__ == '__main__':
    unittest.main()
