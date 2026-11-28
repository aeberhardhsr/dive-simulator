"""Record the tested coast executable, compiled source snapshot and terrain assets."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def sha(path):
    with path.open("rb") as stream:
        return hashlib.file_digest(stream, "sha256").hexdigest()


def main():
    gameplay = json.loads((ROOT/"Documentation/COAST_RESULTS.json").read_text())
    assert gameplay["status"] == "PASS"
    host = gameplay["executable"]
    executable = Path(host["executable"])
    assert sha(executable) == host["sha256"].lower()
    project = ROOT/"divingsim"
    sandbox = ROOT/"Builds/EditorSandbox/divingsim"
    sources = []
    for name in ("Source", "Config"):
        for path in sorted((project/name).rglob("*")):
            if not path.is_file():
                continue
            relative = path.relative_to(project)
            compiled = sandbox/relative
            a, b = path.read_bytes(), compiled.read_bytes()
            assert a.replace(b"\r\n", b"\n") == b.replace(b"\r\n", b"\n"), str(relative)
            sources.append(dict(file=relative.as_posix(), working_sha256=hashlib.sha256(a).hexdigest(),
                                compiled_sha256=hashlib.sha256(b).hexdigest(),
                                canonical_lf_sha256=hashlib.sha256(a.replace(b"\r\n", b"\n")).hexdigest()))
    native = json.loads((ROOT/"artifacts/CoastAutomation/index.json").read_text(encoding="utf-8-sig"))
    assert native["succeeded"] == 35 and native["failed"] == 0
    build_log = ROOT/"artifacts/coast-package-release.log"
    text = build_log.read_text(encoding="utf-8-sig")
    for marker in ("BUILD SUCCESSFUL", "DIVE_COAST_VERIFY_PASS tiles=644 assets=496 triangles=11413310", "DIVE_LAND_VERIFY_PASS tiles=77"):
        assert marker in text, marker
    old = ROOT/"Builds/WindowsDiscoveries/divingsim/Binaries/Win64/divingsim-Discoveries.exe"
    assert sha(old) == "b28e49ee5cb0d8b5149069570dede9938f01ad3d73ddef65a07e6ffa60d14dec"
    assets = []
    for directory in ("Coast", "Landscape"):
        for path in sorted((project/"Content/Diving"/directory).glob("*.uasset")):
            assets.append(dict(file=path.relative_to(project).as_posix(), sha256=sha(path)))
    material = project/"Content/Diving/Materials/M_ShorePhoto.uasset"
    assets.append(dict(file=material.relative_to(project).as_posix(), sha256=sha(material)))
    assert len(assets) == 496+77+1
    containers = [dict(file=path.name, bytes=path.stat().st_size, sha256=sha(path))
                  for path in sorted((ROOT/"Builds/WindowsCoast/divingsim/Content/Paks").iterdir()) if path.is_file()]
    report = dict(status="PASS", recorded_git_head=subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
                  source_note="The canonical LF hashes identify the actual working source built in the editor sandbox independently of the recorded Git HEAD.",
                  executable=host, package_containers=containers, native_tests=dict(succeeded=35, failed=0),
                  package_log_sha256=sha(build_log),
                  coast_manifest_sha256=sha(project/"Content/Diving/Geodata/Coast/coast.json"),
                  coast_mesh_manifest_sha256=sha(project/"Content/Diving/Geodata/Coast/meshes.json"),
                  previous_discoveries_executable_sha256=sha(old), sources=sources, generated_terrain_assets=assets)
    (ROOT/"Documentation/COAST_BUILD.json").write_text(json.dumps(report, indent=2)+"\n", encoding="utf-8")
    print("PASS", len(sources), "source/config files;", len(assets), "terrain assets")


if __name__ == "__main__":
    main()
