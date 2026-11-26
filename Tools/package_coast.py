"""Copy only independently audited coast data into the Unreal content tree."""
import json
from pathlib import Path
import shutil
from validate_coast import verify

ROOT = Path(__file__).resolve().parents[1]


def main():
    verify()
    source = ROOT/"Geodata/Processed/Coast"
    target = ROOT/"divingsim/Content/Diving/Geodata/Coast"
    target.mkdir(exist_ok=True)
    manifest = json.loads((source/"coast.json").read_text(encoding="utf-8"))
    for tile in manifest["tiles"]:
        for key in ("file", "provenance_file"):
            shutil.copyfile(source/tile[key], target/tile[key])
    shutil.copyfile(source/"coast.json", target/"coast.json")
    expected = {tile[key] for tile in manifest["tiles"] for key in ("file", "provenance_file")}
    for pattern in ("coast_*.f32", "coast_*.u8"):
        for obsolete in target.glob(pattern):
            if obsolete.name not in expected and obsolete.resolve().parent == target.resolve():
                obsolete.unlink()
    print("Packaged", len(manifest["tiles"]), "audited coast tiles")


if __name__ == "__main__":
    main()
