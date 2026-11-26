import importlib.util
from pathlib import Path
import tempfile
import unittest
import zipfile

import numpy as np

spec = importlib.util.spec_from_file_location('building_preparation', Path(__file__).parents[1] / 'Tools/prepare_buildings.py')
buildings = importlib.util.module_from_spec(spec)
spec.loader.exec_module(buildings)


class BuildingPolygons(unittest.TestCase):
    def area(self, triangles):
        return np.linalg.norm(np.cross(triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0]), axis=1).sum() / 2

    def test_concave_roof(self):
        ring = np.array([[0, 0, 0], [3, 0, 0], [3, 1, 0], [1, 1, 0], [1, 3, 0], [0, 3, 0], [0, 0, 0]])
        triangles = buildings.triangulate([ring + [2727000, 1222000, 430]])
        self.assertAlmostEqual(self.area(triangles), 5)
        self.assertTrue((np.cross(triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0])[:, 2] > 0).all())

    def test_courtyard_hole(self):
        outer = np.array([[0, 0, 0], [4, 0, 0], [4, 4, 0], [0, 4, 0]])
        hole = np.array([[1, 1, 0], [1, 3, 0], [3, 3, 0], [3, 1, 0]])
        triangles = buildings.triangulate([outer, hole])
        self.assertAlmostEqual(self.area(triangles), 12)
        centers = triangles.mean(axis=1)
        self.assertFalse(((centers[:, 0] > 1) & (centers[:, 0] < 3) & (centers[:, 1] > 1) & (centers[:, 1] < 3)).any())

    def test_vertical_wall_and_coordinate_handedness(self):
        u, v = np.array([1, 1, 0]) / np.sqrt(2), np.array([0, 0, 1])
        origin = np.array([2727000., 1222000., 430.])
        ring = np.array([origin, origin + 3 * u, origin + 3 * u + 2 * v, origin + 2 * v])
        triangles = buildings.triangulate([ring])
        self.assertAlmostEqual(self.area(triangles), 6, places=7)
        source_normals = np.cross(triangles[:, 1] - triangles[:, 0], triangles[:, 2] - triangles[:, 0])
        ue = ((triangles - origin) * 100)[:, :, [1, 0, 2]]
        ue_normals = np.cross(ue[:, 2] - ue[:, 0], ue[:, 1] - ue[:, 0])
        np.testing.assert_allclose(ue_normals, source_normals[:, [1, 0, 2]] * 10000, atol=.0001)

    def test_reversed_ring_and_invalid_coordinates(self):
        ring = np.array([[0, 0, 0], [0, 1, 0], [1, 0, 0]])
        triangle = buildings.triangulate([ring])[0]
        self.assertLess(np.cross(triangle[1] - triangle[0], triangle[2] - triangle[0])[2], 0)
        with self.assertRaises(ValueError):
            buildings.triangulate([[[0, 0, 0], [1, 0, 0], [0, float('nan'), 0]]])
        with self.assertRaises(ValueError):
            buildings.triangulate([ring[:2]])

    def fixture(self, surfaces):
        polygon = '<g:Polygon><g:exterior><g:LinearRing><g:posList>2727000 1222000 430 2727002 1222000 430 2727000 1222002 430 2727000 1222000 430</g:posList></g:LinearRing></g:exterior></g:Polygon>'
        geometry = '<b:Building g:id="test"><b:boundedBy><b:RoofSurface>' + polygon + '</b:RoofSurface></b:boundedBy></b:Building>'
        solid = '<b:Building g:id="test"><b:lod2Solid><g:Solid>' + polygon + '</g:Solid></b:lod2Solid></b:Building>'
        return '<root xmlns:b="http://www.opengis.net/citygml/building/2.0" xmlns:g="http://www.opengis.net/gml">' + (geometry if surfaces else '') + solid + '</root>'

    def test_source_variants_do_not_duplicate_building(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'source.zip'
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr('source.gml', self.fixture(True))
            records = list(buildings.read_buildings(path))
            self.assertEqual(len(records), 1)
            self.assertEqual(records[0][0], 'test')
            self.assertEqual(len(records[0][1]), 1)

    def test_unmatched_solid_is_not_silently_lost(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'source.zip'
            with zipfile.ZipFile(path, 'w') as archive:
                archive.writestr('source.gml', self.fixture(False))
            with self.assertRaisesRegex(ValueError, 'Solid without'):
                list(buildings.read_buildings(path))


if __name__ == '__main__':
    unittest.main()
