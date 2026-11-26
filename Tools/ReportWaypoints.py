"""Verify saved waypoint data and fresh UI/restart evidence from TestWaypoints."""
import argparse
import hashlib
import json
from pathlib import Path
import re
from PIL import Image

def read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))

def sha(path):
    with path.open("rb") as source:
        return hashlib.file_digest(source, "sha256").hexdigest()

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    host = read(args.directory / "Host.json")
    assert sha(Path(host["executable"])).lower() == host["sha256"].lower()
    cases = read(args.directory / "Cases.json")
    assert {c["entry"] for c in cases} == {0, 1, 2}
    results = []
    for case in cases:
        folder = args.directory / case["name"]
        log = (folder / "Runtime.log").read_text(encoding="utf-8-sig")
        phases = re.findall(r"DIVE_WAYPOINT_PHASE (\d+) PASS", log)
        assert [int(p) for p in phases] == list(range(21))
        restart = (folder / "Restart.log").read_text(encoding="utf-8-sig")
        assert "DIVE_WAYPOINT_PASS restart=1" in restart
        assert not re.search(r"DIVE_\w+_FAIL|Failed to add hitbox|Failed to compile Material", log + restart)
        saved = read(folder / "Waypoints.json")
        assert saved["schema_version"] == 1 and saved["area"] == "walensee_lv95"
        assert len(saved["waypoints"]) == 1
        point = saved["waypoints"][0]
        assert point["id"] == saved["target"] and point["name"] == "Meine Position" and not point["from_map"]
        assert 2726000 <= point["east"] <= 2742000 and 1219000 <= point["north"] <= 1223000
        assert 0 <= point["depth"] <= 200
        images = []
        expected_size = tuple(int(v) for v in case["viewport"].split("x"))
        for path in sorted(folder.glob("*.png")):
            with Image.open(path) as image:
                image.load()
                assert image.size == expected_size
            images.append(dict(file=path.name, sha256=sha(path)))
        assert len(images) == case["screenshots"] and len(images) >= 9
        assert any(i["file"].endswith("_Restart.png") for i in images)
        results.append(dict(case=case, waypoint=point, images=images,
                            evidence={n: sha(folder / n) for n in ("Runtime.log", "Restart.log", "Waypoints.json")}))
    report = dict(status="PASS", host=host, cases=results, limits=[
        "UI integration uses engine hitboxes, simulated bound keys and text submission callbacks.",
        "Straight-line bearings do not establish a traversable or safe route.",
        "Map picks use surveyed bed depth; current-position marks store the player position.",
        "These tests cover Windows gameplay, not native macOS rendering."])
    if args.output:
        args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("PASS", len(results), "routes", sum(len(r["images"]) for r in results), "images")

if __name__ == "__main__":
    main()
