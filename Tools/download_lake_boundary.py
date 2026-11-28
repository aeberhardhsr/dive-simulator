"""Extract the Walensee outline from the pinned official swissTLM3D edition.

HTTP ranges fetch only the standing-water Shapefile members of the national ZIP.
ZipFile checks each member's CRC; cached files must match the saved SHA-256 list.
"""
import hashlib
import io
import json
from pathlib import Path
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT / "Geodata/Raw/WaterBoundary"
URL = "https://data.geo.admin.ch/ch.swisstopo.swisstlm3d/swisstlm3d_2026-02/swisstlm3d_2026-02_2056_5728.shp.zip"
BASE = "swissTLM3D_TLM_STEHENDES_GEWAESSER"


class RangeFile(io.RawIOBase):
    def __init__(self, url):
        self.url, self.position = url, 0
        with urllib.request.urlopen(urllib.request.Request(url, method="HEAD"), timeout=60) as response:
            self.size = int(response.headers["Content-Length"])
            self.etag = response.headers.get("ETag")

    def seek(self, offset, whence=0):
        self.position = offset + (self.position if whence == 1 else self.size if whence == 2 else 0)
        if not 0 <= self.position <= self.size:
            raise ValueError("Invalid archive seek")
        return self.position

    def tell(self):
        return self.position

    def readable(self):
        return True

    def seekable(self):
        return True

    def read(self, size=-1):
        size = min(self.size-self.position, size if size >= 0 else self.size-self.position)
        if size == 0:
            return b""
        if size < 0 or size > 64_000_000:
            raise ValueError("Unbounded archive read")
        end = self.position+size-1
        headers = {"Range": f"bytes={self.position}-{end}"}
        if self.etag:
            headers["If-Match"] = self.etag
        request = urllib.request.Request(self.url, headers=headers)
        with urllib.request.urlopen(request, timeout=60) as response:
            if response.status != 206 or response.headers.get("Content-Range") != f"bytes {self.position}-{end}/{self.size}":
                raise ValueError("Server did not return the requested byte range")
            result = response.read(size+1)
        if len(result) != size:
            raise ValueError("Truncated or oversized archive range")
        self.position += size
        return result


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    import shapefile
    from shapely.geometry import shape, mapping
    from shapely.ops import linemerge, polygonize

    DEST.mkdir(parents=True, exist_ok=True)
    previous = json.loads((DEST / "source.json").read_text()) if (DEST / "source.json").exists() else None
    pinned_path = ROOT/"Documentation/WALENSEE_COAST_SOURCE.json"
    pinned = json.loads(pinned_path.read_text(encoding="utf-8")) if pinned_path.exists() else None
    remote = RangeFile(URL)
    if pinned and (pinned["url"] != URL or pinned["archive_size"] != remote.size):
        raise ValueError("Pinned source archive changed")
    members = []
    with zipfile.ZipFile(remote) as archive:
        for extension in ("cpg", "prj", "shx", "dbf", "shp"):
            name = BASE+"."+extension
            info = archive.getinfo("TLM_GEWAESSER/"+name)
            if info.file_size > 60_000_000:
                raise ValueError("Unexpected standing-water member size")
            target = DEST/name
            if not target.exists():
                temporary = target.with_suffix(target.suffix+".tmp")
                temporary.write_bytes(archive.read(info))
                temporary.replace(target)
            item = dict(file=name, size=target.stat().st_size, sha256=digest(target))
            if item["size"] != info.file_size:
                raise ValueError("Cached member size mismatch")
            if previous and item not in previous["files"]:
                raise ValueError("Cached source hash changed")
            if pinned and item not in pinned["files"]:
                raise ValueError("Source differs from versioned checksums")
            members.append(item)
    reader = shapefile.Reader(str(DEST/BASE), encoding="utf-8")
    features, elevations = [], []
    for item in reader.iterShapeRecords():
        if item.record["GEWISS_NR"] != 9267:
            continue
        features.append(dict(type="Feature", properties=item.record.as_dict(), geometry=item.shape.__geo_interface__))
        elevations.extend(item.shape.z)
    if len(features) != 75:
        raise ValueError("Unexpected Walensee feature selection")
    merged = linemerge([shape(f["geometry"]) for f in features])
    rings = list(merged.geoms) if hasattr(merged, "geoms") else [merged]
    if not all(ring.is_ring for ring in rings):
        raise ValueError("Lake outline contains open ends")
    polygons = sorted(polygonize(rings), key=lambda p: p.area, reverse=True)
    lake = polygons[0]
    if not lake.is_valid or not 23e6 < lake.area < 25e6 or len(lake.interiors) != 3:
        raise ValueError("Invalid Walensee polygon or islands")
    source = dict(url=URL, archive_size=remote.size, etag=remote.etag, files=members)
    (DEST/"source.json").write_text(json.dumps(source, indent=2)+"\n", encoding="utf-8")
    (DEST/"walensee_lines.geojson").write_text(json.dumps(dict(type="FeatureCollection", features=features), default=str, ensure_ascii=False)+"\n", encoding="utf-8")
    polygon = dict(type="Feature", properties=dict(name="Walensee", GEWISS_NR=9267, edition="2026-02",
                   source="swissTLM3D TLM_STEHENDES_GEWAESSER", area_m2=lake.area), geometry=mapping(lake))
    (DEST/"walensee_polygon.geojson").write_text(json.dumps(polygon, ensure_ascii=False)+"\n", encoding="utf-8")
    source.update(feature_count=len(features), ring_count=len(rings), area_m2=lake.area, bounds=list(lake.bounds),
                  outline_elevation_range_m=[min(elevations), max(elevations)],
                  polygon_sha256=digest(DEST/"walensee_polygon.geojson"),
                  feature_revision_years=sorted({f["properties"]["REVISION_J"] for f in features}),
                  attribution="© swisstopo", horizontal_crs="EPSG:2056", vertical_crs="EPSG:5728",
                  licence="https://www.swisstopo.admin.ch/de/nutzungsbedingungen-kostenlose-geodaten-und-geodienste")
    (ROOT/"Documentation/WALENSEE_COAST_SOURCE.json").write_text(json.dumps(source, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print(f"PASS: {len(features)} lines, {len(rings)} rings, {lake.area:.3f} square metres")


if __name__ == "__main__":
    main()
