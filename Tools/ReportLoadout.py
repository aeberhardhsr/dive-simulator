"""Check real packaged loadout recordings against an independent mass budget."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import struct


def read(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def close(a, b):
    assert math.isfinite(a) and abs(a-b) < 1e-5, (a, b)


def report(folder):
    cases = read(folder / "Cases.json")
    assert {int(c["name"]) for c in cases} == {0, 1, 2, 3}
    output = {"host": read(folder / "Host.json"), "status": "PASS", "cases": []}
    # Independently calculated prepared masses for the four authored fixtures.
    expected = [(117, 0, 0, 12, True, 6), (123.97, 1, 2, 15, False, 6),
                (112.95, 0, 1, 10, True, 6), (128.47, 0, 2, 15, True, 12)]
    for case in sorted(cases, key=lambda c: int(c["name"])):
        index = int(case["name"])
        directory = folder / str(index)
        data = read(directory / "Dive.json")
        gear = data["loadout"]
        mass, material, bcd, litres, dry, ballast = expected[index]
        assert data["schema_version"] in {6,7} and data["status"] == "smoke_pass"
        assert data["loadout_model"] == "freshwater_mass_balance_v1"
        assert not data["assisted_buoyancy"]
        assert (gear["cylinder_material"], gear["bcd_model"], data["cylinder_litres"],
                gear["dry_suit"], gear["ballast_kg"]) == (material, bcd, litres, dry, ballast)
        items = gear["items"]
        assert len(items) == 10 and len({i["id"] for i in items}) == 10
        dry_mass = sum(i["dry_kg"] for i in items)
        close(80 + dry_mass + 200*litres*.0012, mass)
        close(gear["bcd_capacity_litres"], (18, 12, 24)[bcd])
        samples = data["samples"]
        assert len(samples) >= 10 and data["distance_m"] > 2
        assert max(s["depth_m"] for s in samples) > 1
        for s in samples:
            close(s["equipment_dry_kg"], dry_mass)
            close(s["cylinder_gas_kg"], s["cylinder_bar"]*litres*.0012)
            close(s["bcd_gas_kg"], s["bcd_surface_litres"]*.0012)
            close(s["total_mass_kg"], 80+dry_mass+s["cylinder_gas_kg"]+s["bcd_gas_kg"])
            close(s["bcd_litres"], s["bcd_surface_litres"]/s["ambient_bar"])
            assert 0 <= s["bcd_litres"] <= gear["bcd_capacity_litres"]+1e-5
            close(s["suit_litres"], (3 if dry else 2)+(4.5 if dry else 6.2)/(s["ambient_bar"]/.965)**.7)
            # Lung phase is intentionally not reconstructed from 1 Hz samples.
            rigid = sum(i["solid_displacement_litres"] for i in items if i["id"] not in {"dry", "wet7"})
            baseline = 82+rigid+s["suit_litres"]+s["bcd_litres"]
            assert abs(s["displaced_litres"]-baseline) <= .650001
            close(s["buoyancy_newtons"], (s["displaced_litres"]-s["total_mass_kg"])*9.80665)
        assert all(b["total_mass_kg"] <= a["total_mass_kg"]+1e-7 for a, b in zip(samples, samples[1:]))
        assert any(b["bcd_surface_litres"] > a["bcd_surface_litres"] for a, b in zip(samples, samples[1:])), "Inflation not exercised"
        runtime = (directory / "Runtime.log").read_text(encoding="utf-8-sig")
        impulse = re.search(r"DIVE_LOADOUT_IMPULSE_PASS mass=([\d.]+) expected_cm=([\d.]+) actual_cm=([\d.]+)", runtime)
        assert impulse and "DIVE_LOADOUT_PASS" in runtime
        close(float(impulse[1]), mass)
        expected_cm = 125/mass*.02**2*100*math.cos(math.radians(8))
        close(float(impulse[2]), expected_cm)
        close(float(impulse[3]), expected_cm)
        images = []
        for stage in ("DE", "EN", "Paused"):
            path = directory / f"Loadout_{index}_{stage}.png"
            binary = path.read_bytes()
            assert len(binary) > 10000 and binary[:8] == b"\x89PNG\r\n\x1a\n"
            assert struct.unpack(">II", binary[16:24]) == (1600, 900)
            images.append({"file": str(path), "sha256": hashlib.sha256(binary).hexdigest()})
        output["cases"].append({"name": index, "prepared_mass_kg": mass, "samples": len(samples),
                                "distance_m": data["distance_m"], "status": "PASS", "screenshots": images})
    output["limits"] = ["Authored freshwater equipment parameters; no commercial product validation.",
                        "Short Windows scenarios; no full-body or equipment-variant visual acceptance."]
    return output


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    result = report(args.directory.resolve())
    if args.output:
        args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2)+"\n", encoding="utf-8")
    print(f"PASS {len(result['cases'])} packaged loadout cases, 12 screenshots")
