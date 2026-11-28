"""Prepare small, provenance-labelled shore patches from the verified surveys.

Land and bathymetry are kept unchanged. Only the gap between those surveys is
harmonically interpolated. This gap is modeled terrain, never surveyed depth.
"""
import hashlib
import json
from pathlib import Path

import numpy as np
from pyproj import Transformer
import rasterio
from rasterio.merge import merge

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / "divingsim/Content/Diving/Geodata"
RAW = ROOT / "Geodata/Raw/Landscape"
SIZE = 384
SITES = [
    dict(id="weesen_strandbad", name="Weesen / Strandbad", latitude=47.13933,
         longitude=9.12251, inland_north=6, inland_east=0, yaw=180,
         source="https://www.swiss-divers.ch/informationen/tauchplaetze/walensee-tauchplaetze/strandbad-weesen/"),
    dict(id="murg_west", name="Murg / West", latitude=47.113067,
         longitude=9.208343, inland_north=-10, inland_east=0, yaw=0,
         source="https://www.swiss-divers.ch/informationen/tauchplaetze/walensee-tauchplaetze/murg-west-murg/"),
    dict(id="lediwracks", name="Mols / Lediwracks", latitude=47.115558,
         longitude=9.292695, inland_north=-10, inland_east=0, yaw=0,
         source="https://www.swiss-divers.ch/informationen/tauchplaetze/walensee-tauchplaetze/lediwracks-mols/"),
]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def fill_gap(values, known, tolerance=1e-5, max_iterations=10000):
    """Solve Laplace's equation in unmeasured cells, with fixed measured cells.

    Outer edges use zero normal gradient. Red/black SOR accelerates convergence.
    The original known cells are never assigned. Return residual and iterations.
    """
    result = np.asarray(values, dtype=np.float64).copy()
    unknown = ~np.asarray(known, dtype=bool)
    if not known.any():
        raise ValueError("No measured constraints")
    if not unknown.any():
        return result, 0.0, 0
    result[unknown] = -1.0
    rows, cols = np.indices(result.shape)
    phases = [unknown & ((rows + cols) % 2 == parity) for parity in (0, 1)]
    for iteration in range(max_iterations):
        for phase in phases:
            padded = np.pad(result, 1, mode="edge")
            mean = (padded[:-2, 1:-1] + padded[2:, 1:-1] +
                    padded[1:-1, :-2] + padded[1:-1, 2:]) * .25
            result[phase] += 1.85 * (mean[phase] - result[phase])
        if iteration % 20 == 0:
            padded = np.pad(result, 1, mode="edge")
            mean = (padded[:-2, 1:-1] + padded[2:, 1:-1] +
                    padded[1:-1, :-2] + padded[1:-1, 2:]) * .25
            residual = float(np.abs(result[unknown] - mean[unknown]).max())
            if residual <= tolerance:
                return result, residual, iteration + 1
    raise ValueError(f"Shore gap solver did not converge: {residual} m")


def read_land(west, south, catalog):
    east, north = west + SIZE, south + SIZE
    selected = [t for t in catalog if t["east"] < east + 2 and
                t["east"] + 1000 > west - 2 and t["north"] < north + 2 and
                t["north"] + 1000 > south - 2]
    for tile in selected:
        if digest(RAW / tile["file"]) != tile["sha256"]:
            raise ValueError(f"Source hash mismatch: {tile['file']}")
    datasets = [rasterio.open(RAW / t["file"]) for t in selected]
    try:
        native, transform = merge(datasets, bounds=(west-2, south-2, east+2, north+2),
                                  res=2, nodata=-9999)
    finally:
        for src in datasets:
            src.close()
    native = native[0].astype(np.float64)
    if native.shape != (SIZE//2+2, SIZE//2+2) or np.any(native == -9999):
        raise ValueError("Incomplete native land coverage")
    # Target centres at integer+0.5, source centres at odd integer metres.
    x = (west + np.arange(SIZE) + .5 - transform.c) / 2 - .5
    y = (transform.f - (north - np.arange(SIZE) - .5)) / 2 - .5
    col, row = x.astype(int), y.astype(int)
    tx, ty = x-col, y-row
    a = native[row[:, None], col[None, :]]
    b = native[row[:, None], col[None, :]+1]
    c = native[row[:, None]+1, col[None, :]]
    d = native[row[:, None]+1, col[None, :]+1]
    values = (a*(1-tx)+b*tx)*(1-ty[:, None])+(c*(1-tx)+d*tx)*ty[:, None]
    return values - 419, selected


def read_bathy(west, south, manifest):
    north, east = south + SIZE, west + SIZE
    result = np.full((SIZE, SIZE), np.nan)
    sources = []
    for tile in manifest["tiles"]:
        tw, ts = int(tile["xllcorner"]), int(tile["yllcorner"])
        cols, rows = int(tile["ncols"]), int(tile["nrows"])
        te, tn = tw + cols, ts + rows
        w, s, e, n = max(west, tw), max(south, ts), min(east, te), min(north, tn)
        if w >= e or s >= n:
            continue
        path = DEST / tile["file"]
        source = np.fromfile(path, dtype="<i2").reshape(rows, cols)
        sub = source[tn-n:tn-s, w-tw:e-tw]
        target = result[north-n:north-s, w-west:e-west]
        valid = sub != -32768
        target[valid] = sub[valid].astype(np.float64) * .01
        sources.append(dict(file=tile["file"], sha256=digest(path)))
    return result, sources


def main():
    catalog = json.loads((RAW / "catalog.json").read_text())
    bathy = json.loads((DEST / "manifest.json").read_text())
    transform = Transformer.from_crs(4326, 2056, always_xy=True)
    sites = []
    for spec in SITES:
        east, north = transform.transform(spec["longitude"], spec["latitude"])
        west = int(east // 32) * 32 - SIZE//2
        south = int(north // 32) * 32 - SIZE//2
        land, land_sources = read_land(west, south, catalog)
        bed, bed_sources = read_bathy(west, south, bathy)
        surveyed = np.isfinite(bed)
        dry = ~surveyed & (land >= 0)
        provenance = np.full(bed.shape, 3, dtype="uint8")
        provenance[dry] = 2
        provenance[surveyed] = 1
        known = surveyed | dry
        constrained = np.where(surveyed, bed, land)
        terrain, residual, iterations = fill_gap(constrained, known)
        if not np.array_equal(terrain[known], constrained[known]):
            raise ValueError("Solver modified measured constraints")
        target = DEST / (spec["id"] + "_shore.f32")
        labels = DEST / (spec["id"] + "_shore.u8")
        terrain.astype("<f4").tofile(target)
        provenance.tofile(labels)
        start_east = east + spec["inland_east"]
        start_north = north + spec["inland_north"]
        row, col = int(south+SIZE-start_north), int(start_east-west)
        if provenance[row, col] != 2 or terrain[row, col] < .3:
            raise ValueError(f"Start is not on surveyed dry land: {spec['id']}")
        site = dict(**spec, east=east, north=north, west=west, south=south,
                    columns=SIZE, rows=SIZE, spacing_m=1, start_east=start_east,
                    start_north=start_north, file=target.name, sha256=digest(target),
                    provenance_file=labels.name, provenance_sha256=digest(labels),
                    source_counts={str(i):int((provenance == i).sum()) for i in (1,2,3)},
                    harmonic_residual_m=residual, iterations=iterations,
                    land_sources=land_sources, bathymetry_sources=bed_sources)
        sites.append(site)
        print(spec["id"], site["source_counts"], "residual", residual,
              "iterations", iterations, "start_ground_m", terrain[row,col], flush=True)
    manifest = dict(schema=1, horizontal_crs="EPSG:2056", vertical_crs="EPSG:5728",
                    water_elevation_m=419, source_date="2026-11-25",
                    attribution="Terrain: © swisstopo; entry coordinates: Swiss Divers",
                    encoding="little-endian float32 metres relative to 419 m LN02, north-to-south cell centres",
                    provenance={"1":"swissBATHY3D, unchanged 1 m cell",
                                "2":"swissALTI3D native 2 m terrain, bilinear at 1 m, above scenario water level",
                                "3":"modeled gap, harmonic interpolation; not surveyed depth"},
                    access_geometry="Reference coordinates only; stairs, paths and access furniture are not surveyed by this dataset.",
                    sites=sites)
    (DEST / "shoreline.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")


if __name__ == "__main__":
    main()
