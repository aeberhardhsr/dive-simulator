"""Validate independent packaged-mode evidence; never generate synthetic results."""
import argparse
import hashlib
import json
from pathlib import Path
import struct


def read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def report(folder):
    host = read(folder / "Host.json")
    cases = read(folder / "Cases.json")
    expected = {"exploration", "manual", "simulation", "hardcore", "warm", "hypoxia"}
    assert {case["name"] for case in cases} == expected, "Incomplete mode matrix"
    output = {"host": host, "cases": [], "status": "PASS"}
    for case in cases:
        name = case["name"]
        directory = folder / name
        data = read(directory / "Dive.json")
        samples = data["samples"]
        assert data["schema_version"] == 5 and data["status"] == "smoke_pass"
        assert data["physiology_model"] == "coupled_impairments_v2"
        assert len(samples) >= 20 and data["distance_m"] > 1
        assert all(a["seconds"] <= b["seconds"] for a, b in zip(samples, samples[1:]))
        expected_mode = 0 if name in {"exploration", "manual"} else 2 if name in {"hardcore", "warm"} else 1
        assert data["mode"] == expected_mode
        assert data["automatic_hints"] == (expected_mode != 2)
        assert data["assisted_buoyancy"] == (name == "exploration")
        fault_samples = [s for s in samples if s["regulator_freeflow"]]
        assert bool(fault_samples) == (name == "hardcore")
        if expected_mode == 0:
            assert all(s["cylinder_bar"] == 300 and s["motor_capacity"] == 1 and s["visual_impairment"] == 0
                       and s["breath_reserve"] == 1 and s["spo2_percent"] == 98 for s in samples)
        else:
            assert samples[-1]["cylinder_bar"] < 300
            assert samples[-1]["motor_capacity"] < 1
        if name == "hypoxia":
            assert samples[-1]["spo2_percent"] < 90 and samples[-1]["motor_capacity"] < .7
            assert samples[-1]["visual_impairment"] > .4 and samples[-1]["panic"] > .4
        if name == "hardcore":
            assert samples[0]["regulator_freeflow"] is False, "Fault must develop during live exposure"
            assert samples[-1]["freeflow_lost_litres"] > 100
            assert all(s["warnings"] & 512 for s in fault_samples)
            assert all(0 <= s["freeflow_local_lps"] <= 6.000001 for s in samples)
            assert all(a["freeflow_lost_litres"] <= b["freeflow_lost_litres"] for a, b in zip(samples, samples[1:]))
            # Independent mass budget: loss through the fault cannot exceed total tank draw.
            assert samples[-1]["freeflow_lost_litres"] <= (300-samples[-1]["cylinder_bar"])*data["cylinder_litres"] + 1e-7
        images = []
        for stage in ("Baseline", "Effects", "Menu"):
            path = directory / f"Mode_{name}_{stage}.png"
            binary = path.read_bytes()
            assert binary[:8] == b"\x89PNG\r\n\x1a\n" and len(binary) > 10000
            width, height = struct.unpack(">II", binary[16:24])
            assert (width, height) == (1600, 900)
            images.append({"file": str(path), "sha256": hashlib.sha256(binary).hexdigest(), "width": width, "height": height})
        output["cases"].append({"name": name, "mode": data["mode"], "status": "PASS", "samples": len(samples),
            "seconds": data["duration_seconds"], "distance_m": data["distance_m"], "max_depth_m": data["maximum_depth_m"],
            "final_gas_bar": samples[-1]["cylinder_bar"], "minimum_capacity": min(s["motor_capacity"] for s in samples),
            "maximum_visual_impairment": max(s["visual_impairment"] for s in samples),
            "minimum_spo2": min(s["spo2_percent"] for s in samples),
            "first_fault_seconds": fault_samples[0]["seconds"] if fault_samples else None,
            "lost_litres": samples[-1]["freeflow_lost_litres"], "screenshots": images})
    output["limits"] = ["Short native scenarios on one Windows machine; no two-hour or macOS acceptance.",
        "Narcosis/decompression recovery and thermal comparisons have portable-core coverage; no separate native deep-dive scenario in this matrix.",
        "Authored physiological and equipment parameters remain unvalidated for real diving."]
    return output


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = report(args.directory.resolve())
    if args.output:
        args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"PASS {len(result['cases'])} packaged mode cases, {sum(len(c['screenshots']) for c in result['cases'])} screenshots")
