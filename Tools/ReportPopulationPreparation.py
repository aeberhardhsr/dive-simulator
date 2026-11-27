"""Bind population preparation correctness and complete packaged timing captures."""
import argparse
import csv
import hashlib
import json
import re
from pathlib import Path
from analyze_performance import analyze

ROOT = Path(__file__).resolve().parents[1]


def read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def record(path):
    path = path.resolve()
    return dict(path=path.relative_to(ROOT).as_posix(),
                sha256=hashlib.sha256(path.read_bytes()).hexdigest(), bytes=path.stat().st_size)


def benchmark(folder):
    result = analyze(folder)
    m, h = result["metadata"], result["host"]
    assert not h["editor"] and not h["background_processes"] and h["water_preview"]
    assert record(Path(h["executable"]))["sha256"] == h["executable_sha256"]
    for key, expected in {"scenario": "perf-stream", "width": 1920, "height": 1080,
                          "quality": 2, "requested_capture_seconds": 60,
                          "warmup_seconds": 15, "r.ScreenPercentage": 100,
                          "t.MaxFPS": 0, "r.VSync": 0}.items():
        assert m[key] == expected, (key, m[key])
    log = (folder / "Runtime.log").read_text(encoding="utf-8-sig")
    assert re.search(r"-DiveEntry=2\b", log)
    assert not re.search(r"DIVE_\w*FAIL|Fatal error:", log)
    with (folder / "Capture.csv").open(encoding="utf-8-sig", newline="") as stream:
        rows = list(csv.reader(stream))
    header, data = rows[-2], rows[2:-3]
    work = {}
    for name in header:
        if name.startswith("DivePopulation/GameThread/") or name == "DivePopulation/PrepareRows":
            index = header.index(name)
            values = [float(row[index]) if index < len(row) else 0 for row in data]
            work[name] = dict(maximum=max(values), active_frames=sum(v > 0 for v in values))
    return dict(result=result, work=work, files=[record(folder / name) for name in
                ["Capture.csv", "Metadata.json", "Host.json", "Runtime.log", "Frame.png"]])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("validation", type=Path)
    parser.add_argument("--baseline", type=Path, action="append", required=True)
    parser.add_argument("--benchmark", type=Path, action="append", required=True)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--launcher", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    assert len(args.baseline) == len(args.benchmark) == 2
    host = read(args.validation / "Host.json")
    assert host["packaged"] and not host["preview"]
    for item in host["files"]:
        assert record(ROOT / item["file"])["sha256"] == item["sha256"]
    assert record(Path(host["executable"]))["sha256"] == host["sha256"]
    for item in host["package_containers"]:
        path = Path(host["executable"]).parents[2] / "Content/Paks" / item["file"]
        assert record(path)["sha256"] == item["sha256"]
    cases = read(args.validation / "Cases.json")
    expected = {"before": 3, "after": 3, "shore": 9, "murg": 3,
                "walenstadt": 3, "stream": 2, "sediment": 7}
    assert {c["name"] for c in cases} == set(expected) and len(cases) == len(expected)
    evidence, logs = [], {}
    for case in cases:
        folder = args.validation / case["name"]
        log = (folder / "Runtime.log").read_text(encoding="utf-8-sig")
        logs[case["name"]] = log
        assert case["status"] == "PASS" and "DIVE_SMOKE_PASS" in log
        assert not re.search(r"DIVE_\w*FAIL|Fatal error:", log)
        if case["name"] not in {"shore", "sediment"}:
            for marker in ["DIVE_POPULATION_PREPARE_EQUAL_PASS", "DIVE_POPULATION_STAGED_PASS",
                           "DIVE_FOOT_DEPOSIT_PASS", "DIVE_POPULATION_PASS"]:
                assert marker in log
            if case["name"] != "before":
                assert "DIVE_POPULATION_BUILD_CANCEL_PASS" in log and "DIVE_POPULATION_REGISTER_CANCEL_PASS" in log
        images = sorted(folder.glob("*.png"))
        assert len(images) == expected[case["name"]]
        evidence.append(dict(name=case["name"], runtime=record(folder / "Runtime.log"),
                             images=[record(p) for p in images],
                             engine_errors=[line for line in log.splitlines() if ": Error:" in line],
                             checks=re.findall(r"DIVE_POPULATION_(?:PREPARE_EQUAL|STAGED|BUILD_CANCEL|REGISTER_CANCEL)_PASS[^\r\n]*", log)))
    for marker in ["DIVE_HABITAT_VIEW", "DIVE_HABITAT_PASS"]:
        before = re.findall(marker + r" ([^\r\n]+)", logs["before"])
        after = re.findall(marker + r" ([^\r\n]+)", logs["after"])
        assert before and before == after
    assert "BUILD SUCCESSFUL" in args.build.read_text(encoding="utf-8-sig")
    launcher = read(args.launcher / "Launcher.json")
    assert launcher["status"] == "PASS" and launcher["exit_code"] == 0
    assert launcher["executable_sha256"].lower() == host["sha256"]
    assert launcher["log_sha256"].lower() == record(args.launcher / "Launcher.log")["sha256"]
    launcher_log = (args.launcher / "Launcher.log").read_text(encoding="utf-8-sig")
    assert "DIVE_SMOKE_PASS" in launcher_log and "DIVE_WATER_PREVIEW_READY" in launcher_log
    baseline = [benchmark(p) for p in args.baseline]
    changed = [benchmark(p) for p in args.benchmark]
    assert baseline[0]["result"]["host"]["executable_sha256"] == baseline[1]["result"]["host"]["executable_sha256"]
    for item in changed:
        h = item["result"]["host"]
        assert h["executable_sha256"] == host["sha256"] and h["package_containers"] == host["package_containers"]
    report = dict(scope="Population preparation split into game-thread rows; not final graphics acceptance",
                  host=host, cases=evidence, build=record(args.build), baseline=baseline, changed=changed,
                  launcher=dict(result=launcher, log=record(args.launcher / "Launcher.log")),
                  report_tool=record(Path(__file__)),
                  same_content_containers=baseline[0]["result"]["host"]["package_containers"] == host["package_containers"],
                  limitations=["Two short runs per version; no minimum-hardware or endurance claim",
                               "Two millisecond target is checked between rows, not a hard frame limit",
                               "Nearby missing cells and teleports still finish immediately",
                               "Nested timing scopes are not additive; frame time can reflect previous-row CPU work"])
    args.output.write_text(json.dumps(report, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("POPULATION_PREPARATION_REPORT_PASS")


if __name__ == "__main__":
    main()
