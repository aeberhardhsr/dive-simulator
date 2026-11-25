"""Prepare georeferenced SWISSIMAGE RGB textures from verified official COGs.

Source pixels are read at native resolution, never from COG overviews. The
versioned manifest pins the sources; --refresh explicitly selects newer tiles.
"""
from concurrent.futures import ThreadPoolExecutor
import argparse
import hashlib
import json
from pathlib import Path
import time
import urllib.request
import warnings

import numpy as np
from pyproj import Transformer
import rasterio
from rasterio.transform import from_bounds, from_origin
from rasterio.warp import reproject, Resampling
from rasterio.windows import from_bounds as window_from_bounds
from rasterio.errors import NotGeoreferencedWarning

ROOT = Path(__file__).resolve().parents[1]
RAW = ROOT / "Geodata/Raw/Imagery"
DEST = ROOT / "Geodata/Processed/Imagery"
RUNTIME = ROOT / "divingsim/Content/Diving/Geodata"
BOUNDS = (2723000, 1215000, 2745000, 1228000)
COLLECTION = "https://data.geo.admin.ch/api/stac/v1/collections/ch.swisstopo.swissimage-dop10"


def sha256(path):
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(4 * 1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def read_json(url):
    for attempt in range(4):
        try:
            with urllib.request.urlopen(url, timeout=90) as response:
                return json.load(response)
        except (OSError, ValueError):
            if attempt == 3:
                raise
            time.sleep(2 ** attempt)


def discover():
    transform = Transformer.from_crs(2056, 4326, always_xy=True)
    west, south, east, north = BOUNDS
    corners = [transform.transform(x, y) for x in (west, east) for y in (south, north)]
    bbox = ",".join(map(str, (min(x for x, y in corners), min(y for x, y in corners),
                             max(x for x, y in corners), max(y for x, y in corners))))
    url = COLLECTION + "/items?limit=100&bbox=" + bbox
    selected = {}
    while url:
        page = read_json(url)
        for feature in page["features"]:
            parts = feature["id"].split("_")
            if len(parts) != 3:
                continue
            x, y = (int(n) * 1000 for n in parts[2].split("-"))
            if not (west <= x < east and south <= y < north):
                continue
            key = (x, y)
            if key in selected and selected[key]["year"] >= parts[1]:
                continue
            assets = {}
            for name, asset in feature["assets"].items():
                if not name.endswith(".tif") or asset.get("proj:epsg") != 2056:
                    continue
                spacing = float(asset["gsd"])
                checksum = asset["file:checksum"].lower()
                if not checksum.startswith("1220") or len(checksum) != 68:
                    raise ValueError("Expected official SHA256 multihash")
                assets[str(spacing)] = dict(file=name, url=asset["href"], sha256=checksum[4:],
                                           spacing_m=spacing)
            if "2.0" not in assets:
                raise ValueError("Missing 2 m source: " + feature["id"])
            selected[key] = dict(id=feature["id"], year=parts[1], east=x, north=y, assets=assets)
        url = next((link["href"] for link in page["links"] if link["rel"] == "next"), None)
        print(f"Discovered {len(selected)} latest imagery tiles", flush=True)
    expected = (east - west) // 1000 * (north - south) // 1000
    if len(selected) != expected:
        raise ValueError(f"Incomplete imagery coverage: {len(selected)}/{expected}")
    return sorted(selected.values(), key=lambda t: (t["north"], t["east"]))


def source_asset(tile, detailed):
    spacing = min(map(float, tile["assets"])) if detailed else 2.
    return dict(tile["assets"][str(spacing)], east=tile["east"], north=tile["north"],
                year=tile["year"], item=tile["id"])


def intersects(tile, bounds):
    w, s, e, n = bounds
    return tile["east"] < e and tile["east"] + 1000 > w and tile["north"] < n and tile["north"] + 1000 > s


def download(asset):
    path = RAW / asset["file"]
    if path.exists() and sha256(path) == asset["sha256"]:
        return path
    temporary = path.with_suffix(".download")
    for attempt in range(4):
        try:
            with urllib.request.urlopen(asset["url"], timeout=120) as response, temporary.open("wb") as output:
                while chunk := response.read(4 * 1024 * 1024):
                    output.write(chunk)
            if sha256(temporary) != asset["sha256"]:
                raise ValueError("Checksum mismatch: " + asset["file"])
            temporary.replace(path)
            return path
        except (OSError, ValueError):
            if attempt == 3:
                raise
            time.sleep(2 ** attempt)


def native_mosaic(assets, bounds):
    """Copy native crop windows into an aligned RGB grid, with no resampling."""
    west, south, east, north = bounds
    spacing = assets[0]["spacing_m"]
    if any(a["spacing_m"] != spacing for a in assets):
        raise ValueError("Mixed source resolutions require a deliberate reprojection")
    width, height = round((east - west) / spacing), round((north - south) / spacing)
    pixels = np.zeros((3, height, width), dtype=np.uint8)
    covered = np.zeros((height, width), dtype=bool)
    for asset in assets:
        with rasterio.open(RAW / asset["file"]) as source:
            expected = (asset["east"], asset["north"], asset["east"] + 1000, asset["north"] + 1000)
            if source.crs.to_epsg() != 2056 or source.res != (spacing, spacing) or tuple(source.bounds) != expected or source.count < 3:
                raise ValueError("Unexpected source geometry: " + asset["file"])
            w, s = max(west, expected[0]), max(south, expected[1])
            e, n = min(east, expected[2]), min(north, expected[3])
            window = window_from_bounds(w, s, e, n, source.transform).round_offsets().round_lengths()
            data = source.read((1, 2, 3), window=window)  # Native pixels, not overview read.
            if not np.all(source.read_masks(1, window=window)):
                raise ValueError("NoData in requested imagery")
            row, col = round((north - n) / spacing), round((w - west) / spacing)
            pixels[:, row:row + data.shape[1], col:col + data.shape[2]] = data
            covered[row:row + data.shape[1], col:col + data.shape[2]] = True
    if not covered.all():
        raise ValueError("Unfilled imagery pixels")
    return pixels, from_origin(west, north, spacing, spacing)


def resize(pixels, source_transform, bounds, width, height):
    output = np.empty((3, height, width), dtype=np.uint8)
    transform = from_bounds(*bounds, width, height)
    reproject(pixels, output, src_transform=source_transform, src_crs="EPSG:2056",
              dst_transform=transform, dst_crs="EPSG:2056", resampling=Resampling.average)
    return output


def pinned_catalog(manifest):
    selected={}
    for image in manifest["images"]:
        for source in image["sources"]:
            key=source["item"]
            if key not in selected:
                selected[key]=dict(id=key,year=source["year"],east=source["east"],north=source["north"],assets={})
            asset={name:source[name] for name in ("file","url","sha256","spacing_m")}
            spacing=str(source["spacing_m"])
            previous=selected[key]["assets"].get(spacing)
            if previous is not None and previous!=asset:
                raise ValueError("Conflicting pinned imagery source")
            selected[key]["assets"][spacing]=asset
    return sorted(selected.values(),key=lambda t:(t["north"],t["east"]))


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--refresh",action="store_true",help="Select the latest official acquisition-year tiles instead of the versioned manifest")
    options=parser.parse_args()
    RAW.mkdir(parents=True, exist_ok=True)
    DEST.mkdir(parents=True, exist_ok=True)
    catalog = RAW / "catalog.json"
    previous=RUNTIME/"imagery.json"
    if previous.exists() and not options.refresh:
        tiles=pinned_catalog(json.loads(previous.read_text(encoding="utf-8")))
    else:
        tiles=json.loads(catalog.read_text()) if catalog.exists() and not options.refresh else discover()
    if not catalog.exists() or options.refresh:
        catalog.write_text(json.dumps(tiles, indent=2))
    shores = json.loads((RUNTIME / "shoreline.json").read_text())["sites"]
    jobs = [dict(id="landscape", bounds=BOUNDS, width=4096, height=2048, detailed=False)]
    for site in shores:
        jobs.append(dict(id=site["id"], bounds=(site["west"], site["south"], site["west"] + 384, site["south"] + 384),
                         width=2048, height=2048, detailed=True))
    required = {}
    for job in jobs:
        job["sources"] = [source_asset(t, job["detailed"]) for t in tiles if intersects(t, job["bounds"])]
        required.update({a["file"]: a for a in job["sources"]})
    with ThreadPoolExecutor(max_workers=4) as executor:
        for i, path in enumerate(executor.map(download, required.values())):
            if (i + 1) % 25 == 0 or i + 1 == len(required):
                print(f"Verified downloads {i + 1}/{len(required)}", flush=True)
    images = []
    for job in jobs:
        pixels, transform = native_mosaic(job["sources"], job["bounds"])
        output = resize(pixels, transform, job["bounds"], job["width"], job["height"])
        path = DEST / ("T_Ortho_" + job["id"] + ".png")
        with warnings.catch_warnings():
            warnings.simplefilter("ignore",NotGeoreferencedWarning) # Georeference is stored in imagery.json.
            with rasterio.open(path, "w", driver="PNG", width=job["width"], height=job["height"],
                               count=3, dtype="uint8") as destination:
                destination.write(output)
        w, s, e, n = job["bounds"]
        images.append(dict(id=job["id"], asset=path.stem, file=path.name, west=w, south=s, east=e, north=n,
                           width=job["width"], height=job["height"], pixel_spacing_m=[(e-w)/job["width"], (n-s)/job["height"]],
                           sha256=sha256(path), sources=job["sources"]))
        print(f"Prepared {job['id']}: {job['width']} x {job['height']}", flush=True)
    manifest = dict(schema=1, horizontal_crs="EPSG:2056", attribution="© swisstopo, SWISSIMAGE",
                    additional_attribution="Includes material © (2016-2018) Planet. All rights reserved.",
                    source=COLLECTION, source_catalog_sha256=hashlib.sha256(json.dumps(
                        sorted(required.values(),key=lambda a:a["file"]),sort_keys=True).encode()).hexdigest(),
                    licence="https://www.swisstopo.admin.ch/de/nutzungsbedingungen-kostenlose-geodaten-und-geodienste",
                    resampling="Native RGB pixels; area-average in stored sRGB bytes; north-up; no COG overviews",
                    images=images)
    (RUNTIME / "imagery.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    (RUNTIME / "IMAGERY_NOTICE.txt").write_text(
        "SWISSIMAGE imagery: © swisstopo.\nIncludes material © (2016-2018) Planet. All rights reserved.\n"
        "Sources, acquisition-year labels, SHA256 checksums and processing: imagery.json.\n"
        "Aerial photographs contain captured shadows and flat images of trees/buildings; they are not 3D geometry.\n",
        encoding="utf-8")


if __name__ == "__main__":
    main()
