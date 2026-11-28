"""Bind cooked first-frame exposure comparisons to camera state and package inputs."""
import argparse
import json
import re
from pathlib import Path
import numpy as np
from PIL import Image
from ReportPopulationPreparation import ROOT, record, read


def pixels(path):
    with Image.open(path) as image:
        assert image.size == (1600, 900)
        rgb = np.asarray(image.convert("RGB"), dtype=float)[200:650, 400:1500]
    luminance = rgb @ np.array([.2126, .7152, .0722])
    return dict(roi_xyxy=[400, 200, 1500, 650], mean_display_luminance=float(luminance.mean()),
                p99_display_luminance=float(np.percentile(luminance, 99)),
                fraction_luminance_at_least_200=float((luminance >= 200).mean()),
                fraction_any_channel_at_least_250=float((rgb.max(axis=2) >= 250).mean()))


def pose(log):
    number = r"(-?[\d.]+)"
    pattern = (r"DIVE_EXPOSURE_START legacy=(\d) active=1 previous=" + number +
               " target=" + number + " actual=" + number + " depth=" + number +
               " eye=X=" + number + " Y=" + number + " Z=" + number +
               " gaze=P=" + number + " Y=" + number + " R=" + number + r" lamp=(\d)")
    matches = re.findall(pattern, log)
    assert matches
    values = list(map(float, matches[0]))
    return dict(legacy=bool(values[0]), previous=values[1], target=values[2], actual=values[3],
                depth=values[4], eye=values[5:8], gaze=values[8:11], lamp=bool(values[11]))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("run", type=Path)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--launcher", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--rejected-editor-review", type=Path)
    parser.add_argument("--previous-package-report", type=Path, required=True)
    args = parser.parse_args()
    host = read(args.run / "Host.json")
    assert host["packaged"] and not host["preview"]
    for item in host["files"]:
        assert record(ROOT / item["file"])["sha256"] == item["sha256"]
    assert record(Path(host["executable"]))["sha256"] == host["sha256"]
    for item in host["package_containers"]:
        path = Path(host["executable"]).parents[2] / "Content/Paks" / item["file"]
        assert record(path)["sha256"] == item["sha256"]
    previous = read(args.previous_package_report)["host"]
    assert {x["file"]: x["sha256"] for x in previous["package_containers"]} == {
        x["file"]: x["sha256"] for x in host["package_containers"]}
    old_sources = {x["file"].replace("\\", "/"): x["sha256"] for x in previous["files"]}
    new_sources = {x["file"].replace("\\", "/"): x["sha256"] for x in host["files"]}
    changed_sources = sorted(name for name in old_sources.keys() & new_sources.keys()
                             if name.startswith("divingsim/Source/") and old_sources[name] != new_sources[name])
    assert changed_sources == ["divingsim/Source/DiveSim/DivePawn.cpp", "divingsim/Source/DiveSim/DivePawn.h"]
    expected = {"day-before": 3, "day-after": 3, "night-before": 3, "night-after": 3,
                "night-dark": 3, "shore": 9, "sediment-before": 7, "sediment-after": 7}
    cases = read(args.run / "Cases.json")
    assert len(cases) == len(expected) and {c["name"] for c in cases} == set(expected)
    evidence = {}
    for case in cases:
        folder = args.run / case["name"]
        log = (folder / "Runtime.log").read_text(encoding="utf-8-sig")
        assert case["status"] == "PASS" and "DIVE_SMOKE_PASS" in log
        assert not re.search(r"DIVE_\w*FAIL|Fatal error:|Default Material will be used|Failed to compile Material", log)
        assert "DIVE_WATER_PREVIEW_READY smoke=1 hardware=1 bounded=1 interface=1 lighting_mode=2" in log
        state = pose(log)
        assert state["legacy"] == case["name"].endswith("-before")
        error = abs(state["actual"] - state["target"])
        assert error > .1 if state["legacy"] else error <= .00001
        images = sorted(folder.glob("*.png"))
        assert len(images) == expected[case["name"]]
        evidence[case["name"]] = dict(state=state, exposure_error=error, runtime=record(folder / "Runtime.log"),
            engine_errors=[line for line in log.splitlines() if ": Error:" in line],
            images=[dict(**record(path), pixels=pixels(path)) for path in images])
    comparisons = []
    dark = pixels(args.run / "night-dark/ExposureStart.png")
    lit = pixels(args.run / "night-after/ExposureStart.png")
    assert dark["p99_display_luminance"] <= 1 and lit["p99_display_luminance"] > 25
    assert dark["mean_display_luminance"] < lit["mean_display_luminance"] * .05
    for name, filename in [("day", "ExposureStart.png"), ("night", "ExposureStart.png"),
                           ("sediment", "Sediment_0_Start.png")]:
        before, after = evidence[name + "-before"], evidence[name + "-after"]
        distance = float(np.linalg.norm(np.array(before["state"]["eye"]) - after["state"]["eye"]))
        rotation = float(np.max(np.abs(np.array(before["state"]["gaze"]) - after["state"]["gaze"])))
        assert distance <= 5 and rotation <= .01
        old = pixels(args.run / (name + "-before") / filename)
        new = pixels(args.run / (name + "-after") / filename)
        if name in {"day", "sediment"}:
            assert new["mean_display_luminance"] < old["mean_display_luminance"] * .5
            assert new["fraction_luminance_at_least_200"] < .01
        comparisons.append(dict(name=name, camera_difference_cm=distance, rotation_difference_degrees=rotation,
                                before=old, after=new))
    assert "BUILD SUCCESSFUL" in args.build.read_text(encoding="utf-8-sig")
    launcher = read(args.launcher / "Launcher.json")
    assert launcher["status"] == "PASS" and launcher["exit_code"] == 0
    assert launcher["executable_sha256"].lower() == host["sha256"]
    assert launcher["log_sha256"].lower() == record(args.launcher / "Launcher.log")["sha256"]
    published = []
    for case, label in [("day-before", "Before"), ("day-after", "After")]:
        target = ROOT / f"Documentation/Images/ExposureStart_{label}.png"
        target.write_bytes((args.run / case / "ExposureStart.png").read_bytes())
        published.append(record(target))
    report = dict(scope="Initialize camera exposure at scene start; continuous in-dive adaptation retained",
        host=host, cases=evidence, comparisons=comparisons, published=published, build=record(args.build),
        launcher=dict(result=launcher, log=record(args.launcher / "Launcher.log")), report_tool=record(Path(__file__)),
        previous_package_report=record(args.previous_package_report), same_content_containers=True,
        changed_game_sources=changed_sources,
        rejected_editor_review=record(args.rejected_editor_review) if args.rejected_editor_review else None,
        limitations=["Display-image statistics are not a physical irradiance calibration",
                     "Small recorded first-frame pose differences are bounded, not assumed pixel-identical",
                     "Waterline reflections, temporal startup noise and full reference graphics remain open",
                     "No new performance or minimum-hardware claim"])
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("EXPOSURE_START_REPORT_PASS")


if __name__ == "__main__":
    main()
