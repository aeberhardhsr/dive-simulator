# Detailed underwater rock forms

The previous underwater dressing repeated one deformed low-detail sphere. Three distinct source-based forms now replace it. The graphical preview is `Builds/WindowsRockDetail/divingsim.exe`: nine packaged cases and 54 fresh captures pass. Performance and endurance acceptance for this package remain pending. The measured FishSteering baseline is preserved unchanged.

## Sources and interpretation

Geometry sources are [Boulder 01](https://polyhaven.com/a/boulder_01) by Rico Cilliers, [Namaqualand Boulder 05](https://polyhaven.com/a/namaqualand_boulder_05) by Dario Barresi/Jenelle van Heerden, and [Namaqualand Boulder 06](https://polyhaven.com/a/namaqualand_boulder_06) by Greg Zaal/Jenelle van Heerden. Poly Haven publishes these assets under [CC0](https://polyhaven.com/license). Pinned download URLs, authors, provider MD5, file sizes and our SHA-256 are in [ROCK_ASSETS.json](ROCK_ASSETS.json).

These are not Walensee scans or a reconstruction of local rock geology. The two Namaqualand source shapes originate from a different environment. Only their geometry is used; desert colors and lichen imagery are not imported. The existing shared lake-rock PBR material supplies the surfaces, with provenance in [BED_SURFACES.md](BED_SURFACES.md). Placement and rescaling remain authored. The surveyed lakebed is unchanged.

## Geometry and runtime

`prepare_rock_assets.py` verifies the original Blender files without modifying them. `create_rock_assets.py` extracts the highest source LOD, applies its object transform, centers its bounds, and uniformly normalizes its maximum vertex distance to one metre. The three closed source surfaces contain 66,122 / 89,618 / 109,114 triangles. The builder rejects non-finite coordinates, open/nonmanifold edges and degenerate faces. No missing back surfaces are invented for these standalone boulders.

The runtime retains the existing deterministic placement, size and rotation sequence. A separate cell/candidate hash selects one of the three forms. Each 96-metre cell now owns three rock components plus the existing three plant components. Nearby cells keep all instances when the streaming window changes. Conservative 1.05-metre exclusion radii scaled per instance continue to protect fish and entry clearances; these are not triangle-exact fish contacts.

The Unreal importer enables Nanite, generates four raster LODs (100 / 35 / 10 / 2.5 percent target triangles), and checks imported dimensions against the source-normalized bounds. Screen-size thresholds are 1 / 0.20 / 0.07 / 0.02. The fallback target is explicitly PercentTriangles: Unreal's Auto mode overrides the requested error and produced visibly reduced close-range geometry in the first package. A separate audit of built render data now verifies that fallback LOD 0 retains all 66,122 / 89,618 / 109,114 source triangles. Convex decomposition creates 5 / 1 / 1 hulls for diver contacts. Convex hulls approximate recesses; they do not reproduce every visual crevice. The existing player contact sweeps remain enabled during ordinary play.

## Inspection scope

On 26 November 2026 the editor-game comparison produced eight fresh views with the intended material. An initial run exposed a missing Nanite material usage flag; the importer now explicitly sets and saves it, and the corrected run contains no fallback-material warning. A separate 90-second Murg streaming run passes population retention and all five species' steering checks. These checks ran alongside the unchanged FishSteering endurance package and are not isolated performance measurements.

The first `WindowsRocks` package passed nine technical render/gameplay cases, but visual comparison exposed the automatically reduced fallback described above. Changing LOD screen thresholds alone did not fix it. Explicit PercentTriangles fallback does: corrected native and packaged raster comparisons retain the source fractures, and the built-data audit confirms the triangle counts. The overall target remains the user's [visual references](VISUAL_TARGET.md), far beyond acceptance of these three meshes alone.

The refined package passes the old/new comparison with Nanite on and off, population/steering streaming routes at all three entries, complete shore routes at all three entries, and the discoveries scenario. [ROCK_GEOMETRY_RESULTS.json](ROCK_GEOMETRY_RESULTS.json) binds the executable and cooked files to 54 captures, source/config hashes, asset hashes and the built-data audit. [CI](ROCK_GEOMETRY_CI.json) records portable checks on Windows, macOS and Linux; this is not a native macOS game acceptance.

The user opened the root launcher through Explorer during the final discovery test. The subsequent automated launcher test correctly refused to start another instance. Read-only process evidence instead verifies that the actual root launcher started the expected packaged child. This does not claim an automated launcher playthrough or exit. The user's instance was left running. Three editor-game logs additionally contain the installed engine's optional ToolsetRegistry PythonTestRunner registration error; the report records that exact engine-only traceback. Import/audit commandlets and packaged cases do not receive that exception.

Previous form, identical controlled location and camera:

![Previous rounded form in the packaged comparison](Images/Rock_Previous.png)

Detailed form, ordinary raster fallback after correction:

![Detailed rock with preserved close-range fractures](Images/Rock_Detail.png)

Another of the three forms:

![Block-shaped variation in the same comparison](Images/Rock_Block.png)

`preview_rock_assets.py` produces a neutral Blender geometry review, not a game screenshot. The `rocks` smoke scenario compares the old form and three new forms at the same location, scale and two camera angles in surveyed water. It keeps the ordinary ground/water material and light configuration, but hides other dressing and equipment and holds simulation still. This controlled fixture does not prove natural placement, collision response, performance or all LOD transitions. The ordinary entry and streaming routes must be checked separately, along with a Nanite-disabled rendering case.

```powershell
./Tools/.geodata-env/Scripts/python.exe Tools/prepare_rock_assets.py
& 'C:/Program Files/Blender Foundation/Blender 5.3/blender.exe' --background --factory-startup --disable-autoexec --python-exit-code 1 --python Tools/create_rock_assets.py
./Tools/Build.ps1 -Target Content -EditorSandbox -OnlyRocks
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/SmokeTest.ps1 -EditorSandbox -Scenario rocks -MaxFPS 30 -RuntimeLog artifacts/rock-editor-inspection.log
```
