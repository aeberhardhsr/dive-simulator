"""Check terrain build inputs and complete tile coverage without Unreal.

The Unreal LandscapeCheck commandlet additionally verifies every mesh source
vertex/UV and the actual Nanite data. This script does not replace that check.
"""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "divingsim/Content/Diving/Geodata"


def input_digest(json_line_ending=None):
    digest = hashlib.md5()  # Build freshness only; source authenticity uses SHA256.
    names = ["landscape.f32", "landscape.json", "manifest.json", "shoreline.json"]
    names += sorted(path.name for path in DATA.glob("*.i16"))
    for name in names:
        path = DATA / name
        if path.suffix == ".json":
            value = path.read_text(encoding="utf-8-sig")
            if json_line_ending is not None:
                value = value.replace("\r\n", "\n").replace("\n", json_line_ending)
            digest.update(value.replace("\r\n", "\n").encode("utf-8"))
        else:
            with path.open("rb") as source:
                while block := source.read(1024 * 1024):
                    digest.update(block)
    return digest.hexdigest().upper()


def main():
    grid = json.loads((DATA / "landscape.json").read_text(encoding="utf-8"))
    origin = json.loads((DATA / "manifest.json").read_text(encoding="utf-8"))
    meshes = json.loads((DATA / "landscape_meshes.json").read_text(encoding="utf-8"))
    assert meshes["schema"] == 1 and meshes["baker_version"] == 1
    assert meshes["spacing_m"] == grid["spacing_m"] == 8
    assert meshes["tile_cells"] == 256
    hashes = {input_digest(), input_digest("\n"), input_digest("\r\n")}
    assert hashes == {meshes["input_md5"]}, "Stale build inputs or platform-dependent digest"
    expected = {(x, y) for y in range(0, grid["rows"] - 1, 256)
                for x in range(0, grid["columns"] - 1, 256)}
    observed = set()
    vertices = triangles = 0
    for tile in meshes["tiles"]:
        x, y = tile["column"], tile["row"]
        assert (x, y) in expected and (x, y) not in observed
        observed.add((x, y))
        right, bottom = min(x + 256, grid["columns"] - 1), min(y + 256, grid["rows"] - 1)
        assert (tile["last_column"], tile["last_row"]) == (right, bottom)
        assert tile["origin_x_cm"] == (grid["north"] - (y + .5) * 8 - origin["origin_north"]) * 100
        assert tile["origin_y_cm"] == (grid["west"] + (x + .5) * 8 - origin["origin_east"]) * 100
        assert tile["vertices"] == (right - x + 1) * (bottom - y + 1)
        assert 0 < tile["triangles"] <= 2 * (right - x) * (bottom - y)
        name = f"SM_Land_{x // 256:02}_{y // 256:02}"
        assert tile["asset"] == f"/Game/Diving/Landscape/{name}.{name}"
        assert (DATA.parent / "Landscape" / (name + ".uasset")).stat().st_size > 1000
        vertices += tile["vertices"]
        triangles += tile["triangles"]
    assert observed == expected, "Uncovered landscape tile"
    assert (vertices, triangles) == (meshes["vertices"], meshes["triangles"])
    print(f"PASS landscape manifest: {len(observed)} tiles, {vertices} source vertices, "
          f"{triangles} triangles; full edge coverage; identical LF/CRLF input digest")


if __name__ == "__main__":
    main()
