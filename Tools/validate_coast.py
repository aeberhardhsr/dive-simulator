"""Independently audit exported coast tiles against source rasters and outline."""
import hashlib
import json
from pathlib import Path

import numpy as np
import rasterio
from rasterio.features import rasterize
from rasterio.transform import from_origin
from shapely.geometry import shape

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT/"Geodata/Processed/Coast"
DATA = ROOT/"divingsim/Content/Diving/Geodata"
RAW = ROOT/"Geodata/Raw/Landscape"


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def verify():
    manifest = json.loads((DEST/"coast.json").read_text(encoding="utf-8"))
    west, south, east, north = manifest["bounds"]
    shape_grid = (north-south, east-west)
    values = np.full(shape_grid, np.nan, "f4")
    labels = np.zeros(shape_grid, "u1")
    covered = np.zeros(shape_grid, bool)
    core_covered = np.zeros(shape_grid, bool)
    shared = 0
    require(len(manifest["tiles"]) == manifest["tile_count"], "Tile count mismatch")
    for tile in manifest["tiles"]:
        terrain = DEST/tile["file"]
        provenance = DEST/tile["provenance_file"]
        require(digest(terrain) == tile["sha256"] and digest(provenance) == tile["provenance_sha256"], "Output hash mismatch")
        size = tile["core_size"]+2
        require(tile["columns"] == tile["rows"] == size, "Invalid halo dimensions")
        z = np.fromfile(terrain, "<f4").reshape(size, size)
        p = np.fromfile(provenance, "u1").reshape(size, size)
        require(np.isfinite(z).all() and ((p >= 1) & (p <= 3)).all(), "Invalid height/provenance")
        require(tile["core_west"] == tile["west"]+1 and tile["core_south"] == tile["south"]+1, "Halo origin mismatch")
        if "tile_lattice_offset_m" in manifest:
            require((tile["core_west"]-manifest["tile_lattice_offset_m"]) % tile["core_size"] == 0 and
                    (tile["core_south"]-manifest["tile_lattice_offset_m"]) % tile["core_size"] == 0,
                    "Coast core is not aligned to the background landscape lattice")
        row, col = north-tile["south"]-size, tile["west"]-west
        require(row >= 0 and col >= 0 and row+size <= len(values) and col+size <= values.shape[1], "Tile outside model")
        region = np.s_[row:row+size, col:col+size]
        overlap = covered[region]
        shared += int(overlap.sum())
        require(np.array_equal(values[region][overlap], z[overlap]) and np.array_equal(labels[region][overlap], p[overlap]), "Adjacent tile halo mismatch")
        values[region], labels[region], covered[region] = z, p, True
        core = np.s_[row+1:row+size-1, col+1:col+size-1]
        require(not core_covered[core].any(), "Duplicate tile core")
        core_covered[core] = True
        require(tile["core_source_counts"] == {str(i): int((p[1:-1, 1:-1] == i).sum()) for i in (1, 2, 3)}, "Provenance count mismatch")

    boundary = ROOT/"Geodata/Raw/WaterBoundary/walensee_polygon.geojson"
    require(digest(boundary) == manifest["source_outline"]["polygon_sha256"], "Outline changed")
    lake = shape(json.loads(boundary.read_text(encoding="utf-8"))["geometry"])
    inside = rasterize([(lake, 1)], out_shape=shape_grid, transform=from_origin(west, north, 1, 1), dtype="uint8").astype(bool)
    surveyed = np.zeros(shape_grid, bool)
    source_manifest = json.loads((DATA/"manifest.json").read_text())
    measured_checked = 0
    for tile in source_manifest["tiles"]:
        path = DATA/tile["file"]
        pinned = next(t for t in manifest["bathymetry_sources"] if t["file"] == tile["file"])
        require(digest(path) == pinned["sha256"], "Bathymetry source changed")
        raw = np.fromfile(path, "<i2").reshape(int(tile["nrows"]), int(tile["ncols"]))
        tw, ts = int(tile["xllcorner"]), int(tile["yllcorner"])
        tn, te = ts+raw.shape[0], tw+raw.shape[1]
        w, s, e, n = max(west, tw), max(south, ts), min(east, te), min(north, tn)
        if w >= e or s >= n:
            continue
        part = raw[tn-n:tn-s, w-tw:e-tw]
        region = np.s_[north-n:north-s, w-west:e-west]
        measured = part != -32768
        surveyed[region] = measured
        check = covered[region] & measured
        measured_checked += int(check.sum())
        expected = (part[check].astype("f8")*.01).astype("f4")
        require(np.array_equal(values[region][check], expected), "Original measured height changed")
        require((labels[region][check] == 1).all(), "Measured provenance lost")
    unknown = inside & ~surveyed
    require(not (unknown & ~core_covered).any(), "Unmeasured lake cell missing from output cores")
    if "modeled_cell_margin_m" in manifest:
        from scipy.ndimage import minimum_filter
        margin = manifest["modeled_cell_margin_m"]
        protected = minimum_filter(core_covered, size=2*margin+1, mode="constant", cval=0)
        require(protected[unknown].all(), "Modeled cells enter exterior LOD collar")
    require(np.array_equal(labels[covered] == 3, unknown[covered]), "Model values outside correct gap")
    require(np.array_equal(labels[covered] == 1, surveyed[covered]), "False survey labels")
    require((values[unknown] <= 0).all(), "Modeled lake cell above scenario water")
    free = np.flatnonzero(unknown)
    neighbour_sum = np.zeros(len(free), "f8")
    for offset in (-values.shape[1], values.shape[1], -1, 1):
        neighbour_sum += np.minimum(values.ravel()[free+offset].astype("f8"), 0)
    residual = float(np.max(np.abs(values.ravel()[free].astype("f8")-neighbour_sum*.25)))
    require(np.isfinite(residual) and residual <= 1e-5, "Exported interpolation equation failed")

    # Independent land spot-check: read the four original 2 m cell centres
    # directly, including stencils that cross original 1 km GeoTIFF boundaries.
    source_tiles = {(int(t["east"]), int(t["north"])): t for t in manifest["land_sources"]}
    land_indices = np.flatnonzero(labels.ravel() == 2)[::97]
    sample_rows, sample_cols = np.unravel_index(land_indices, shape_grid)
    xs, ys = west+sample_cols+.5, north-sample_rows-.5
    low_x, low_y = np.floor((xs-1)/2)*2+1, np.floor((ys-1)/2)*2+1
    expected = np.zeros(len(xs), "f8")
    tx, ty = (xs-low_x)/2, (ys-low_y)/2
    for dx, dy, weight in ((0, 0, (1-tx)*(1-ty)), (2, 0, tx*(1-ty)), (0, 2, (1-tx)*ty), (2, 2, tx*ty)):
        px, py = low_x+dx, low_y+dy
        keys = np.column_stack(((px//1000*1000).astype(int), (py//1000*1000).astype(int)))
        for key in np.unique(keys, axis=0):
            select = (keys == key).all(axis=1)
            source = source_tiles[tuple(key)]
            with rasterio.open(RAW/source["file"]) as dataset:
                samples = np.array([item[0] for item in dataset.sample(zip(px[select], py[select]))])
            require(np.isfinite(samples).all() and not (samples == -9999).any(), "Missing land stencil")
            expected[select] += samples*weight[select]
    expected = (expected-419).astype("f4")
    land_error = float(np.abs(expected-values.ravel()[land_indices]).max())
    require(land_error == 0, f"Native land interpolation mismatch: {land_error}")
    for tile in manifest["land_sources"]:
        require(digest(RAW/tile["file"]) == tile["sha256"], "Land source changed")
    require(int(unknown.sum()) == manifest["solver"]["unknown_cells"], "Unknown count mismatch")
    report = dict(status="PASS", scope="Intermediate data only; no rendered game acceptance", manifest_sha256=digest(DEST/"coast.json"),
                  tile_count=len(manifest["tiles"]), identical_shared_halo_cells=shared,
                  measured_cells_compared=measured_checked, land_cells_independently_sampled=len(xs),
                  land_sample_max_error_m=land_error, complete_modeled_lake_cells=int(unknown.sum()),
                  independently_checked_harmonic_residual_m=residual,
                  lake_cells=int(inside.sum()), island_holes=len(lake.interiors),
                  source_counts={str(i): int((labels[core_covered] == i).sum()) for i in (1, 2, 3)})
    (ROOT/"Documentation/COAST_MODEL_AUDIT.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    print(json.dumps(report, indent=2), flush=True)
    return report


if __name__ == "__main__":
    verify()
