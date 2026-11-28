"""Validate saved screenshot/log associations and inventory rendered evidence."""
import argparse
import hashlib
import json
import re
from pathlib import Path
from PIL import Image, ImageStat


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def report(directory):
    host = json.loads((directory / "Host.json").read_text(encoding="utf-8-sig"))
    assert sha(Path(host["executable"])) == host["sha256"].lower(), "Executable changed"
    cases = json.loads((directory / "Cases.json").read_text(encoding="utf-8-sig"))
    assert len(cases) == 3 and {c["entry"] for c in cases} == {0, 1, 2}
    results = []
    for case in cases:
        folder = directory / case["name"]
        log = (folder / "Runtime.log").read_text(encoding="utf-8-sig")
        assert "DIVE_DISCOVERY_PASS phases=14" in log
        assert not re.search(r"DIVE_\w+_FAIL|Failed to add hitbox", log)
        for phase in range(14):
            assert f"DIVE_DISCOVERY_PHASE {phase} PASS" in log
        assert len(re.findall(r"dark=1 torch=1 fog=1 occluded=1 silt=1 natural_population=1", log)) == 2
        dive = json.loads((folder / "Dive.json").read_text(encoding="utf-8-sig"))
        assert dive["schema_version"] == 7 and dive["is_test"]
        photos = dive["photographs"]
        assert len(photos) == 2 and len({p["id"] for p in photos}) == 2
        assert set(photos[0]["species"]) & {"perch", "roach", "pike", "whitefish", "burbot"}
        assert set(photos[1]["species"]) & {"pondweed", "stonewort", "milfoil"}
        for photo in photos:
            assert re.fullmatch(r"[A-Fa-f0-9]{32}", photo["id"])
            assert (folder / (photo["id"] + ".png")).is_file()
            assert 0 <= photo["seconds"] <= dive["duration_seconds"] + .05
        images = []
        for path in sorted(folder.glob("*.png")):
            with Image.open(path) as image:
                image.load()
                assert image.size == tuple(map(int, case["viewport"].split("x")))
                assert max(ImageStat.Stat(image.convert("RGB")).stddev) > 2
                images.append(dict(file=path.name, size=image.size, sha256=sha(path)))
        assert len(images) >= 7
        if case["entry"] == 0:
            assert "DIVE_DISCOVERY_RESTART_PASS photos=2" in (folder / "Restart.log").read_text(encoding="utf-8-sig")
            assert any("Restart" in image["file"] for image in images)
        results.append(dict(case=case, directory=str(folder), photos=photos, images=images,
                            log_sha256=sha(folder / "Runtime.log"), dive_sha256=sha(folder / "Dive.json")))
    return dict(status="PASS", host=host, image_count=sum(len(r["images"]) for r in results), cases=results,
                limits="Controlled cameras at ordinary population objects; no claim of natural search, pixel recognition or photorealism. Walenstadt includes a visible open bathymetry boundary.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = report(args.directory)
    args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"PASS: {len(result['cases'])} cases, {result['image_count']} images")
