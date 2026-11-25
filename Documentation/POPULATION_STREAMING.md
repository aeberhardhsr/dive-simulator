# Continuous underwater dressing and fish

The former population update cleared every rock, plant and fish after the diver moved 55 metres. Because placement was relative to the new viewer position, even nearby rocks and their collision could change. Fish also lost their individual movement and animation state. This contradicted continuous exploration.

## Fixed world cells

Rocks and plants now belong to fixed 96 × 96 metre cells in world coordinates. A 3 × 3 window stays loaded. On an ordinary edge crossing, six cells retain their existing Unreal components and instances; only the outgoing row is destroyed and the incoming row created. A diagonal crossing retains four cells. The nearest outgoing/incoming edge is 96 metres away from the viewer, beyond the configurable 40-metre water visibility. Terrain streaming remains a separate, unchanged 64-metre grid.

Each cell uses its own seeded random stream. Rock placement depends on the cell, surveyed terrain and fixed entry/access clearances, not the camera or traversal order. Plant placement also checks neighbouring cells' rock footprints so an edge does not introduce a plant/rock overlap. Returning to a cell restores identical transforms for the same dive configuration. These are authored procedural details on the surveyed terrain, not measured individual rocks or plant locations.

There are 128 rock candidates and 648 plant candidates per cell, filtered by the existing depth, slope, light and patch constraints. Actual counts depend on the bathymetry. Nine cells and their components are retained; unloaded cell components are destroyed rather than accumulated as visited locations. Collision is kept on the rock instances.

## Bounded fish population

The five existing species use 77 reusable instance slots in total. Initially they are placed near the selected entry where their habitat envelope permits. A normal cell crossing preserves active fish positions, velocities, home locations, scale, phase and animation cycle. Existing schooling, separation, escape, depth and rock avoidance remain active.

A fish beyond 80 metres horizontally is retired and its instance hidden. At most once per second, vacant slots attempt habitat-valid placement in a 65–75 metre annulus around the diver. Replacements therefore enter outside the configured visibility, and the total cannot grow with travel distance. Missing habitat can leave slots vacant. This is a bounded local population, not a persistent lake-wide animal simulation. Far fish are recycled; their individual identity is not saved. The five models, three vegetation models and their materials are unchanged, so this does not establish photorealistic or anatomical acceptance.

## Checks

```powershell
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/Build.ps1 -Target Package -EditorSandbox -BuildSlot Population -PackageDirectory Builds/WindowsPopulation
./Tools/TestPopulation.ps1 -PackageDirectory Builds/WindowsPopulation
python Tools/ReportPopulation.py artifacts/population-validation-YYYYMMDD-HHMMSS --output artifacts/population-report.json
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsPopulation -Scenario stream -Entry 1 -Seconds 120 -Resolution 1080p -Quality 2
```

The continuity fixture queries positions two centimetres apart across a real cell boundary and checks the identity and transforms of all six retained cells plus every nearby active fish state. It queries a distant location to unload all original cells, then returns and compares all nine cells' transforms exactly. A separate refill check confirms every newly occupied slot is 65–75 metres away and that the pool remains 77 slots. The fixture restores the real player's population and random state within the same frame; it is a structural check, not a rendered traversal.

The changed rock distribution exposed an existing animation defect in the sediment regression. Contact rejection can retain the previous fin-motion amplitude while its blend back to rest is obstructed. The kick phase formerly continued advancing without movement input, so a released constraint could produce a fresh unintended stroke later. The phase now advances only while movement is requested (or while actually walking); the relaxation toward rest and reading the computer remain available. `DiveSim.Contacts.StoppedKick` checks the constrained-amplitude case and resuming input. The sediment lifecycle expectation is retained rather than relaxed.

Ordinary 90-second stream scenarios then move the actual pawn. They preserve runtime habitat/contact checks and separately log real cell transitions and time spent updating population cells. The regression matrix also covers all three shore returns, sediment and entry-map selection. Scenario images and frame captures provide the separate rendered evidence. Performance captures must exclude the structural fixture; its intentional remote unload/reload is not representative gameplay.

The sediment regression's automatic reverse look can be constrained by the physical body near the bed. Its lifecycle result proves stirring, pause and expiry; an off-screen plume is not evidence of visible optical contrast. The unchanged material's dedicated optical comparison remains documented in [SEDIMENT.md](SEDIMENT.md). Independent head yaw and broader visual/anatomical acceptance remain separate work.

## Verified Windows package, 26 November 2026

The final package is `Builds/WindowsPopulation/divingsim.exe`. Its named game executable has SHA-256 `d09daa75a08b66493fd03882c42ccb8cac2bc657f286809d7e85d4c81871df6c`. All 112 source/configuration files match the build sandbox. The nine fish/plant/rock mesh assets, 25 avatar assets, underwater/sediment materials and sediment shader are unchanged; the previous WindowsNavigation executable is preserved. [Build evidence](POPULATION_BUILD.json) records source, asset, executable and package archive hashes.

All 27 native Unreal tests, 15 Python tests and portable model checks passed. The final serial package run `artifacts/population-validation-20261126-003356` passed eight cases and produced 48 fresh 1600×900 images. The three continuity fixtures retained six component sets and restored all nine cells exactly; they checked 59/77/77 unchanged nearby fish and 75/77/77 distant replacement placements at Weesen/Murg/Walenstadt respectively. Ordinary stream routes reached net displacements of 109.46/110.85/91.30 metres. All three shore-return routes, the sediment lifecycle and the shore-entry map regression passed. The earlier failed sediment run is not included in these success counts. [Runtime evidence](POPULATION_RESULTS.json) includes each log and image hash and distinguishes structural queries from ordinary gameplay crossings.

These functional scenarios use a 30 FPS cap. The user's editor remains open; they are not performance benchmarks. The final Weesen underwater view and Mols map were visually inspected. The original procedural ecology models and coarse shore imagery still limit visual quality.

The separate uncapped 1080p, quality-2, 100%-render-scale benchmark captured 119.979 seconds after 15 seconds of warmup on RTX 3090 / Ryzen 9 9950X3D. It travelled 193.13 metres during the capture and recorded 12,631 frames: **105.28 mean FPS / 89.17 one-percent-low FPS**, maximum **15.2582 ms**, zero frames above 16.67 ms. Two population cell crossings inside the capture loaded/unloaded three cells each, taking 3.580 and 2.631 ms for the population update. Process-memory samples ranged from a 2,429 MiB start to a 2,362 MiB end, with a 2,439 MiB maximum. This short sampling does not prove long-term memory stability.

The user editor (PID 33256) remained open throughout this measurement, so it is not an isolated hardware comparison. This Murg route ends in deep open water and does not represent worst-case visible vegetation/rock density or minimum hardware. No structural remote-query fixture ran in the benchmark. [Performance evidence](POPULATION_PERFORMANCE.json) records the complete frame statistics, host context, actual cell events and raw-file hashes; two CSV boundary frames are excluded by the existing analyzer, all interior frames retained.

The ordinary root launcher started the expected Population executable and passed its separate 22-second dive scenario with exit code zero. [Launcher evidence](POPULATION_LAUNCHER.json) records the child path, executable hash and log hash.

This work addresses continuity in WRLD-004/009 and individual state retention in BIOL-005/006. Full visual reference acceptance, lake-wide persistent ecology, native macOS gameplay, reference hardware and the complete specification remain open.
