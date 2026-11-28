"""Independently verify output pixels against native official SWISSIMAGE COGs."""
from contextlib import ExitStack
import hashlib
import json
import math
from pathlib import Path
import warnings

import numpy as np
import rasterio
from rasterio.errors import NotGeoreferencedWarning
from rasterio.windows import Window

ROOT=Path(__file__).resolve().parents[1]
RAW=ROOT/"Geodata/Raw/Imagery"
PNG=ROOT/"Geodata/Processed/Imagery"
DATA=ROOT/"divingsim/Content/Diving/Geodata"


def checksum(path):
    result=hashlib.sha256()
    with path.open("rb") as source:
        while block:=source.read(1024*1024): result.update(block)
    return result.hexdigest()


def expected_pixel(sources,west,south,east,north):
    """Area-weight native source cells without GDAL warp/resampling code."""
    weighted=np.zeros(3,dtype=np.float64)
    area=0.
    for source in sources:
        b=source.bounds; spacing=source.res[0]
        w,s,e,n=max(west,b.left),max(south,b.bottom),min(east,b.right),min(north,b.top)
        if w>=e or s>=n: continue
        col0=max(0,math.floor((w-b.left)/spacing))
        col1=min(source.width,math.ceil((e-b.left)/spacing))
        row0=max(0,math.floor((b.top-n)/spacing))
        row1=min(source.height,math.ceil((b.top-s)/spacing))
        values=source.read((1,2,3),window=Window(col0,row0,col1-col0,row1-row0))
        for row in range(row0,row1):
            top=b.top-row*spacing
            dy=max(0.,min(n,top)-max(s,top-spacing))
            for col in range(col0,col1):
                left=b.left+col*spacing
                weight=dy*max(0.,min(e,left+spacing)-max(w,left))
                weighted+=values[:,row-row0,col-col0]*weight; area+=weight
    if abs(area-(east-west)*(north-south))>1e-6:
        raise AssertionError("Output pixel is not completely covered by source cells")
    return weighted/area


def main():
    manifest=json.loads((DATA/"imagery.json").read_text(encoding="utf-8"))
    assert manifest["schema"]==1 and manifest["horizontal_crs"]=="EPSG:2056"
    sources={s["file"]:s for image in manifest["images"] for s in image["sources"]}
    for name,item in sources.items():
        assert checksum(RAW/name)==item["sha256"], name
    checked=0; largest=0.
    for image in manifest["images"]:
        assert checksum(PNG/image["file"])==image["sha256"]
        with warnings.catch_warnings():
            warnings.simplefilter("ignore",NotGeoreferencedWarning) # PNG georeference is in imagery.json.
            with rasterio.open(PNG/image["file"]) as texture:
                assert texture.width==image["width"] and texture.height==image["height"] and texture.count==3
                pixels=texture.read()
        with ExitStack() as stack:
            native=[stack.enter_context(rasterio.open(RAW/s["file"])) for s in image["sources"]]
            width,height=image["width"],image["height"]
            dx=(image["east"]-image["west"])/width; dy=(image["north"]-image["south"])/height
            points={(int(x),int(y)) for x in np.linspace(0,width-1,13) for y in np.linspace(0,height-1,11)}
            # Include output pixels crossing source tile boundaries, not just centres.
            for source in native:
                for e in (source.bounds.left,source.bounds.right):
                    x=math.floor((e-image["west"])/dx)
                    for y in (0,height//2,height-1):
                        if 0<=x<width: points.add((x,y))
                for n in (source.bounds.bottom,source.bounds.top):
                    y=math.floor((image["north"]-n)/dy)
                    for x in (0,width//2,width-1):
                        if 0<=y<height: points.add((x,y))
            maximum=0.
            for x,y in points:
                west=image["west"]+x*dx; north=image["north"]-y*dy
                expected=expected_pixel(native,west,north-dy,west+dx,north)
                difference=float(np.max(np.abs(pixels[:,y,x].astype(float)-expected)))
                maximum=max(maximum,difference)
                if difference>1.01:
                    raise AssertionError(f"{image['id']} pixel {x},{y}: RGB error {difference}")
            checked+=len(points); largest=max(largest,maximum)
            print(f"PASS {image['id']}: {len(points)} independent area averages, maximum RGB error {maximum:.6f}/255",flush=True)
    print(f"PASS imagery: {len(sources)} official SHA256 hashes, {checked} pixels, maximum RGB error {largest:.6f}/255",flush=True)


if __name__=="__main__": main()
