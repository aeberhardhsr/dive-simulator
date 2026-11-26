"""Generate a separate, provenance-labelled estimate for all Walensee gaps.

Outputs are staged source data; package_coast.py audits and copies them into
Unreal. Never edits bathymetry, the existing three shore patches, or any source
raster. One global solve avoids different interpolated values on adjacent tiles.
"""
import os
os.environ.setdefault("OPENBLAS_NUM_THREADS", "1")

import hashlib
import json
import math
from pathlib import Path
import time

import numpy as np
from coast_model import solve

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "divingsim/Content/Diving/Geodata"
RAW = ROOT / "Geodata/Raw/Landscape"
DEST = ROOT / "Geodata/Processed/Coast"
TILE = 128


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def land_grid(bounds, catalog):
    """Read verified native 2 m ALTI; interpolate to 1 m centres in strips."""
    import rasterio
    from rasterio.merge import merge
    west, south, east, north = bounds
    selected = [t for t in catalog if t["east"] < east+2 and t["east"]+1000 > west-2
                and t["north"] < north+2 and t["north"]+1000 > south-2]
    for tile in selected:
        if digest(RAW/tile["file"]) != tile["sha256"]:
            raise ValueError(f"Source hash mismatch: {tile['file']}")
    sources = [rasterio.open(RAW/t["file"]) for t in selected]
    try:
        native, transform = merge(sources, bounds=(west-2, south-2, east+2, north+2),
                                  res=2, nodata=-9999)
    finally:
        for source in sources:
            source.close()
    native = native[0]
    if not np.isfinite(native).all() or (native == -9999).any():
        raise ValueError("Incomplete native land coverage")
    result = np.empty((north-south, east-west), dtype="<f4")
    x = (west+np.arange(east-west)+.5-transform.c)/2-.5
    column, tx = np.floor(x).astype(int), x % 1
    for first in range(0, len(result), 128):
        last = min(first+128, len(result))
        y = (transform.f-(north-np.arange(first, last)-.5))/2-.5
        row, ty = np.floor(y).astype(int), (y % 1)[:, None]
        a = native[row[:, None], column]
        b = native[row[:, None], column+1]
        c = native[row[:, None]+1, column]
        d = native[row[:, None]+1, column+1]
        result[first:last] = ((a*(1-tx)+b*tx)*(1-ty)+(c*(1-tx)+d*tx)*ty)-419
    return result, selected


def add_bathymetry(values, bounds, manifest):
    """Replace only valid source cells. Return their exact provenance mask."""
    west, south, east, north = bounds
    surveyed = np.zeros(values.shape, dtype=bool)
    sources = []
    for tile in manifest["tiles"]:
        tw, ts = int(tile["xllcorner"]), int(tile["yllcorner"])
        cols, rows = int(tile["ncols"]), int(tile["nrows"])
        tn = ts+rows
        w, s, e, n = max(west, tw), max(south, ts), min(east, tw+cols), min(north, tn)
        if w >= e or s >= n:
            continue
        path = DATA/tile["file"]
        raw = np.fromfile(path, dtype="<i2").reshape(rows, cols)
        part = raw[tn-n:tn-s, w-tw:e-tw]
        valid = part != -32768
        target = values[north-n:north-s, w-west:e-west]
        target[valid] = (part[valid].astype(np.float64)*.01).astype(np.float32)
        surveyed[north-n:north-s, w-west:e-west] = valid
        sources.append(dict(file=tile["file"], sha256=digest(path)))
    return surveyed, sources


def write_tiles(values, labels, selected, bounds, destination, tile_size=TILE):
    """Write selected cores with one-cell halos from the same global field."""
    west, south, east, north = bounds
    if values.shape != labels.shape or values.shape != selected.shape or values.shape != (north-south, east-west):
        raise ValueError("Mismatched tile grids")
    if values.shape[0] % tile_size or values.shape[1] % tile_size:
        raise ValueError("Unaligned tile grid")
    destination.mkdir(parents=True, exist_ok=True)
    tiles = []
    covered = np.zeros(values.shape, dtype=bool)
    for row in range(0, len(values), tile_size):
        for col in range(0, values.shape[1], tile_size):
            core = np.s_[row:row+tile_size, col:col+tile_size]
            if not selected[core].any():
                continue
            if row == 0 or col == 0 or row+tile_size == len(values) or col+tile_size == values.shape[1]:
                raise ValueError("Selected coast tile lacks halo")
            halo = np.s_[row-1:row+tile_size+1, col-1:col+tile_size+1]
            tw, ts = west+col, north-row-tile_size
            name = f"coast_{tw}_{ts}"
            terrain, provenance = destination/(name+".f32"), destination/(name+".u8")
            values[halo].astype("<f4").tofile(terrain)
            labels[halo].astype("u1").tofile(provenance)
            covered[core] = True
            tiles.append(dict(west=tw-1, south=ts-1, columns=tile_size+2, rows=tile_size+2,
                              core_west=tw, core_south=ts, core_size=tile_size,
                              file=terrain.name, sha256=digest(terrain),
                              provenance_file=provenance.name, provenance_sha256=digest(provenance),
                              core_source_counts={str(i): int((labels[core] == i).sum()) for i in (1, 2, 3)}))
    if (selected & ~covered).any():
        raise ValueError("Incomplete coast coverage")
    return tiles


def main():
    from rasterio.features import rasterize
    from rasterio.transform import from_origin
    from shapely.geometry import shape
    started = time.monotonic()
    boundary = ROOT/"Geodata/Raw/WaterBoundary/walensee_polygon.geojson"
    reference = json.loads((ROOT/"Documentation/WALENSEE_COAST_SOURCE.json").read_text(encoding="utf-8"))
    if digest(boundary) != reference["polygon_sha256"]:
        raise ValueError("Lake outline hash mismatch")
    lake = shape(json.loads(boundary.read_text(encoding="utf-8"))["geometry"])
    if not lake.is_valid or len(lake.interiors) != 3:
        raise ValueError("Invalid lake outline")
    w, s, e, n = lake.bounds
    # Three outer tile widths include the coast band, expanded cores and halos.
    # Core edges align with the existing 8 m landscape vertex lattice (4 mod 8).
    bounds = [math.floor((w-4)/TILE)*TILE+4-3*TILE, math.floor((s-4)/TILE)*TILE+4-3*TILE,
              math.ceil((e-4)/TILE)*TILE+4+3*TILE, math.ceil((n-4)/TILE)*TILE+4+3*TILE]
    west, south, east, north = bounds
    print("Reading original land", bounds, flush=True)
    values, land_sources = land_grid(bounds, json.loads((RAW/"catalog.json").read_text()))
    surveyed, bed_sources = add_bathymetry(values, bounds, json.loads((DATA/"manifest.json").read_text()))
    transform = from_origin(west, north, 1, 1)
    inside = rasterize([(lake, 1)], out_shape=values.shape, transform=transform, dtype="uint8").astype(bool)
    unknown = inside & ~surveyed
    labels = np.full(values.shape, 2, dtype="u1")
    labels[unknown] = 3
    labels[surveyed] = 1
    print("Solving", int(unknown.sum()), "unmeasured lake cells", flush=True)
    result, solver = solve(values, inside, surveyed, progress=lambda iteration: print("CG iteration", iteration, flush=True))
    if not np.array_equal(result[~unknown], values[~unknown]):
        raise ValueError("Solver changed original measured constraints")
    band = lake.buffer(64).difference(lake.buffer(-64))
    selected = rasterize([(band, 1)], out_shape=values.shape, transform=transform, dtype="uint8").astype(bool)
    selected |= unknown
    # Every modeled cell stays at least 32 m from the exterior LOD collar.
    from scipy.ndimage import maximum_filter
    selected |= maximum_filter(unknown, size=65, mode="constant", cval=0)
    tiles = write_tiles(result, labels, selected, bounds, DEST)
    manifest = dict(schema=1, status="source cells for coast render and collision generation", bounds=bounds,
                    horizontal_crs="EPSG:2056", vertical_crs="EPSG:5728", water_elevation_m=419,
                    spacing_m=1, halo_cells=1, tile_core_metres=TILE, tile_lattice_offset_m=4,
                    exterior_collar_m=32, modeled_cell_margin_m=32,
                    encoding="little-endian float32 metres relative to 419 m LN02, north-to-south cell centres",
                    source_outline=reference, land_sources=land_sources, bathymetry_sources=bed_sources,
                    provenance={"1": "Original swissBATHY3D cell; unchanged apart from float32 representation",
                                "2": "Native 2 m swissALTI3D land, bilinear at 1 m; outside lake outline",
                                "3": "Modeled lake gap: harmonic interpolation, not surveyed depth"},
                    method="Four-neighbour Laplace equation on lake cells without bathymetry. Fixed land-side solver constraints capped at scenario water level; original land values retained. Cell-centre rasterization excludes all three island holes.",
                    uncertainty="No estimated accuracy. Outline, terrain and bathymetry have different acquisition dates and resolutions; this is a game surface, not a navigation or diving survey.",
                    solver=solver, lake_cells=int(inside.sum()), measured_lake_cells=int((inside & surveyed).sum()),
                    measured_cells_outside_outline=int((surveyed & ~inside).sum()),
                    tile_count=len(tiles), tiles=tiles)
    (DEST/"coast.json").write_text(json.dumps(manifest, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    report = {key: value for key, value in manifest.items() if key not in ("tiles", "land_sources", "bathymetry_sources")}
    report.update(manifest_sha256=digest(DEST/"coast.json"), land_source_count=len(land_sources),
                  bathymetry_source_count=len(bed_sources),
                  elapsed_seconds=round(time.monotonic()-started, 2),
                  output_bytes=sum((DEST/t[key]).stat().st_size for t in tiles for key in ("file", "provenance_file")))
    (ROOT/"Documentation/COAST_MODEL_RESULTS.json").write_text(json.dumps(report, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print("PASS", len(tiles), "tiles;", solver, flush=True)


if __name__ == "__main__":
    main()
