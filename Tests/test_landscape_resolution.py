"""Analytic checks for the height-error audit's mesh sampling, not bilinear sampling."""
from pathlib import Path
import sys
import unittest
import numpy as np

sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'Tools'))
from audit_landscape_resolution import triangle_height


class LandscapeTriangleAudit(unittest.TestCase):
    def test_plane_and_world_registration(self):
        manifest={'west':2723000,'north':1228000}
        rows,cols=np.indices((4,5))
        grid=100+2*cols+3*rows
        x=np.array([.1,1.4,2.7]);y=np.array([.3,1.8,2.2])
        height,valid=triangle_height(grid,2723000+(x+.5)*8,1228000-(y+.5)*8,manifest)
        self.assertTrue(valid.all())
        np.testing.assert_allclose(height,100+2*x+3*y,atol=1e-8,rtol=0)

    def test_actual_diagonal_differs_from_bilinear_saddle(self):
        # On the southwest/northeast diagonal, both end heights are zero.
        # Bilinear interpolation would incorrectly report height 5 at the centre.
        grid=np.array([[10.,0.],[0.,10.]])
        x=np.array([.25,.5,.75]);y=x.copy()
        height,valid=triangle_height(grid,(x+.5)*8,32-(y+.5)*8,{'west':0,'north':32})
        self.assertTrue(valid.all())
        np.testing.assert_array_equal(height,[5.,0.,5.])

    def test_outer_incomplete_cells_are_excluded(self):
        height,valid=triangle_height(np.zeros((3,3)),np.array([3.,4.,19.,20.]),np.full(4,28.),{'west':0,'north':32})
        np.testing.assert_array_equal(valid,[False,True,True,False])


if __name__=='__main__': unittest.main()
