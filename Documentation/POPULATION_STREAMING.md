# Continuous underwater dressing and fish

The former population update cleared every rock, plant and fish after the diver moved 55 metres. Because placement was relative to the new viewer position, even nearby rocks and their collision could change. Fish also lost their individual movement and animation state. This contradicted continuous exploration.

## Fixed world cells

Rocks and plants now belong to fixed 96 × 96 metre cells in world coordinates. A 3 × 3 window stays loaded. On an ordinary edge crossing, six cells retain their existing Unreal components and instances; only the outgoing row is destroyed and the incoming row created. A diagonal crossing retains four cells. The nearest outgoing/incoming edge is 96 metres away from the viewer, beyond the configurable 40-metre water visibility. Terrain streaming remains a separate, unchanged 64-metre grid.

Each cell uses its own seeded random stream. Rock placement depends on the cell, surveyed terrain and fixed entry/access clearances, not the camera or traversal order. Plant placement also checks neighbouring cells' rock footprints so an edge does not introduce a plant/rock overlap. Returning to a cell restores identical transforms for the same dive configuration. These are authored procedural details on the surveyed terrain, not measured individual rocks or plant locations.

The original population revision used 128 rock candidates and 648 plant candidates per cell. The later [clustered habitat revision](UNDERWATER_SCENE.md) uses rock groups and 4,096 plant candidates, filtered by depth, slope, light and patch constraints. Actual counts depend on the bathymetry. The target window contains nine cells; unloaded cell components are destroyed rather than accumulated as visited locations. Collision is kept on the rock instances.

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

## Bounded reuse of clustered rock layouts

The later clustered habitat revision increased the number of rock placement queries. Each newly created population cell requested its own layout and all eight neighbouring layouts to keep plants outside rock footprints. Rebuilding the canonical fish collision array requested those layouts again. The same deterministic geometry was consequently generated several times during a cell crossing.

Rock transforms and collision states are now cached by world cell. The cache contains at most the 5×5 neighbourhood needed by the visible 3×3 population window plus its planting halo. Layouts outside that neighbourhood are removed before loading a new window, and `Populate` clears the cache when rebuilding a dive. It owns no Unreal objects and does not extend component or fish lifetimes. Rock placement, plant placement, visible density, collision radii and canonical ordering remain unchanged.

The extended population fixture compares cached transforms, positions, radii and mesh variants against a fresh calculation exactly. It also checks the 25-cell bound after boundary crossings, remote unloading and return. The existing retained-component, restored-transform, fish-state, steering and gravel checks still run. `Tools/TestPopulationCache.ps1` combines these checks with three fixed rendered views at each entry and a separate root-launcher dive.

The original `WindowsHabitat` package and its measured Murg route are retained as the performance baseline. Native fixture timings are not substituted for normal gameplay measurements; the fixture intentionally unloads and reloads distant regions in one frame.

## Staged creation of distant cells

An ordinary boundary crossing now creates one incoming population cell per frame, producing seven, eight, then nine populated cells. Missing cells are sorted by horizontal distance to their bounds. Existing nearby components keep their identity and transforms. Fish avoidance immediately receives the complete target window in canonical order, including geometry awaiting component creation.

Initial placement and any move leaving missing geometry within the 80-metre fish radius load the complete window immediately. This includes an adjacent-cell teleport landing near its far edge; cell-index distance alone would not detect that case. The runtime validator permits incomplete windows only outside that radius. The fixture checks the 7/8/9 sequence, immediate adjacent and remote teleport loading, identical restored geometry, retained nearby fish and the 25-layout cache bound. `Tools/TestPopulationCache.ps1 -RequireStaged` requires these additional markers from the packaged executable.

The verified package is `Builds/WindowsPopulationStaged/divingsim.exe`, runtime revision `fae73b210e876cc280ef5bbc3e3f5ddd022e2fa4`. Its game executable SHA-256 is `3b63e3c73cb08c18ba14778af3c3da819e0bb9de18e1593af4efddcbc323e60c`. All three entry fixtures (nine fresh images) and the ordinary root launcher pass. The fixed Weesen and Lediwracks views were inspected; the known ground lines, thin plants and overall visual shortcomings remain. Source CI also passed on [GitHub](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45837865656).

On the same isolated 90-second Murg route at 1080p High / 100% scale, the original habitat package recorded a 68.83-ms maximum frame; layout caching reduced it to 41.25 ms, and staged creation to 37.12 ms. The final run averaged 100.93 FPS with 80.21 FPS one-percent-low and one frame above 33.33 ms. In-capture population creation steps took 2.73–11.46 ms, compared with 14.79–20.80 ms for complete rows in the cache-only package. Remaining total-frame work still produces a hitch; this is not a hitch-free acceptance or a visual-quality improvement. Initial full-window creation remains synchronous.

The [cache-only report](POPULATION_CACHE_RESULTS.json) preserves the first comparison. The [staged report](POPULATION_STAGED_RESULTS.json) binds the final source revision, package, launcher, fixture images and raw frame data. Both comparisons preserve all interior measured frames; the route ends in deep open water and does not represent the worst visible vegetation density or long-term stability.
