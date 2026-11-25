"""Independent checks of packaged shoreline heights, provenance and solver."""
import hashlib
import json
from pathlib import Path
import numpy as np
import rasterio
from prepare_shoreline import fill_gap

ROOT=Path(__file__).resolve().parents[1]
DATA=ROOT/"divingsim/Content/Diving/Geodata"
RAW=ROOT/"Geodata/Raw/Landscape"


def main():
    # A planar analytic solution must be recovered inside a missing rectangle.
    y,x=np.indices((31,39)); plane=.3*x-.2*y+5
    known=np.ones(plane.shape,dtype=bool); known[3:-3,4:-4]=False
    reconstructed,residual,_=fill_gap(plane,known,tolerance=1e-9)
    assert np.max(np.abs(reconstructed-plane))<1e-7
    assert np.array_equal(reconstructed[known],plane[known])
    manifest=json.loads((DATA/"shoreline.json").read_text())
    bathy=json.loads((DATA/"manifest.json").read_text())
    total_modeled=0
    for site in manifest["sites"]:
        for name,sha in ((site["file"],site["sha256"]),
                         (site["provenance_file"],site["provenance_sha256"])):
            assert hashlib.sha256((DATA/name).read_bytes()).hexdigest()==sha
        z=np.fromfile(DATA/site["file"],dtype="<f4").reshape(site["rows"],site["columns"])
        labels=np.fromfile(DATA/site["provenance_file"],dtype="u1").reshape(z.shape)
        assert np.isfinite(z).all()
        assert set(np.unique(labels))=={1,2,3}
        assert {str(i):int((labels==i).sum()) for i in (1,2,3)}==site["source_counts"]
        north=site["south"]+site["rows"]
        row,col=np.indices(z.shape)
        east=site["west"]+col+.5; n=north-row-.5
        checked=np.zeros(z.shape,dtype=bool)
        max_bathy_error=0
        for tile in bathy["tiles"]:
            w,s=tile["xllcorner"],tile["yllcorner"]
            mask=(labels==1)&(east>=w)&(east<w+tile["ncols"])&(n>=s)&(n<s+tile["nrows"])
            if not mask.any(): continue
            source=np.fromfile(DATA/tile["file"],dtype="<i2").reshape(int(tile["nrows"]),int(tile["ncols"]))
            r=(s+tile["nrows"]-n[mask]).astype(int); c=(east[mask]-w).astype(int)
            original=source[r,c]
            assert (original!=-32768).all()
            error=np.abs(z[mask]-original.astype(float)*.01)
            max_bathy_error=max(max_bathy_error,float(error.max()))
            assert error.max()<2e-5
            checked[mask]=True
        assert checked[labels==1].all()
        # Read native source pixels around dispersed independent land positions.
        land=np.argwhere(labels==2)[::211]
        datasets=[]
        for tile in site["land_sources"]:
            assert hashlib.sha256((RAW/tile["file"]).read_bytes()).hexdigest()==tile["sha256"]
            datasets.append(rasterio.open(RAW/tile["file"]))
        def height(e,n):
            for ds in datasets:
                if ds.bounds.left<=e<ds.bounds.right and ds.bounds.bottom<=n<ds.bounds.top:
                    return float(next(ds.sample([(e,n)]))[0])-419
            raise AssertionError("Missing native neighbour")
        max_land_error=0
        try:
            for r,c in land:
                e,northing=east[r,c],n[r,c]
                e0=np.floor((e-1)/2)*2+1; n0=np.floor((northing-1)/2)*2+1
                tx=(e-e0)/2; ty=(northing-n0)/2
                expected=(height(e0,n0)*(1-tx)+height(e0+2,n0)*tx)*(1-ty)+(height(e0,n0+2)*(1-tx)+height(e0+2,n0+2)*tx)*ty
                max_land_error=max(max_land_error,abs(expected-float(z[r,c])))
            assert max_land_error<1e-4
        finally:
            for ds in datasets: ds.close()
        padded=np.pad(z.astype(float),1,mode="edge")
        mean=(padded[:-2,1:-1]+padded[2:,1:-1]+padded[1:-1,:-2]+padded[1:-1,2:])*.25
        residual=float(np.max(np.abs(z[labels==3]-mean[labels==3])))
        assert residual<2e-5
        assert z[labels==3].min()>=z[labels!=3].min()-1e-4
        assert z[labels==3].max()<=z[labels!=3].max()+1e-4
        total_modeled+=int((labels==3).sum())
        print(site["id"],"bathy_max_error_m",max_bathy_error,
              "land_max_error_m",max_land_error,"gap_residual_m",residual,flush=True)
    print("SHORELINE_DATA_PASS sites=3 modeled_cells="+str(total_modeled))


if __name__=="__main__": main()
