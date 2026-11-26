"""Bind the packaged coastal traversal and regression evidence to its executable."""
import argparse
import hashlib
import json
from pathlib import Path
import re

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path)
    args = parser.parse_args()
    args.run = args.run.resolve()
    host = json.loads((args.run/"Host.json").read_text(encoding="utf-8-sig"))
    cases = json.loads((args.run/"Cases.json").read_text(encoding="utf-8-sig"))
    assert host["packaged"] and digest(Path(host["executable"])) == host["sha256"].lower()
    expected = {"weesen-crossing", "murg-crossing", "walenstadt-4k", "walenstadt-fallback", "weesen-shore", "murg-shore", "mols-shore", "discoveries", "controls"}
    assert len(cases) == len(expected) and {case["name"] for case in cases} == expected
    assert sorted(case["entry"] for case in cases if case["scenario"] == "coast" and not case["fallback"]) == [0, 1, 2]
    assert sorted(case["entry"] for case in cases if case["scenario"] == "shore") == [0, 1, 2]
    assert sum(bool(case["fallback"]) for case in cases) == 1
    evidence = []
    for case in cases:
        folder = args.run/case["name"]
        log = (folder/"Runtime.log").read_text(encoding="utf-8-sig")
        assert case["status"] == "PASS" and "DIVE_SMOKE_PASS" in log
        assert "DIVE_COAST_COLLISION_PASS probes=496" in log and not re.search(r"DIVE_\w+_FAIL", log)
        assert "DIVE_COAST_READY tiles=644 assets=496 triangles=11413310" in log
        if case["scenario"] == "coast":
            assert f"DIVE_COAST_ROUTE_PASS entry={case['entry']} crossed_unmeasured=1 returned_measured=1" in log
            assert "DIVE_COAST_ROUTE_CROSSED" in log
            assert f"DIVE_COAST_SEDIMENT_PASS entry={case['entry']}" in log
            for stage in ("Start", "Crossed", "Returned"):
                assert (folder/f"Coast_{case['entry']}_{stage}.png").is_file()
        elif case["scenario"] == "shore":
            assert "DIVE_SHORE_PASS" in log and log.count("DIVE_SHORE_SEAM_PASS") >= 3
        elif case["scenario"] == "discoveries":
            assert "DIVE_DISCOVERY_PASS phases=14" in log
        if case["fallback"]:
            assert "r.Nanite = \"0\"" in log or "r.Nanite 0" in log or "r.Nanite = 0" in log
        images = []
        size = tuple(map(int, case["viewport"].split("x")))
        for path in sorted(folder.glob("*.png")):
            with Image.open(path) as image:
                assert image.size == size
                image.verify()
            images.append(dict(file=str(path.relative_to(ROOT)).replace("\\", "/"), sha256=digest(path)))
        assert len(images) == case["screenshots"]
        evidence.append(dict(**case, images=images, runtime_sha256=digest(folder/"Runtime.log")))
    report = dict(status="PASS", executable=host, cases=evidence,
                  scope="Packaged crossings and returns at three original survey boundaries, cooked collision probes for every coast mesh, three existing shore routes, photo and control regressions.",
                  limits="Scripted input with one initial placement per route. No physical keyboard acceptance, whole-shore navigation, long-session, minimum-hardware or macOS game certification.")
    (ROOT/"Documentation/COAST_RESULTS.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    print("PASS", len(cases), "cases", sum(len(case["images"]) for case in evidence), "images")


if __name__ == "__main__":
    main()
