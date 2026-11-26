# Fish fin motion

This increment develops BIOL-004 (realistic swimming animation) beyond the previous body/tail deformation. It does not close the requirement's full biological or visual acceptance.

## Behavior

Pectoral and pelvic fins move around their attachment to the body. Soft dorsal/anal fins receive a smaller traveling deflection; the spiny perch dorsal fin moves less. The whitefish adipose fin stays passive. The caudal fin continues to follow the body wave.

Body cadence and stroke amplitude respond smoothly to actual swimming velocity relative to the species cruise speed. Paired fins have their own accumulated phase; their stroke is reduced at higher effort and changes asymmetrically while turning. This is an authored response to the existing movement simulation, not a hydrodynamic force or muscle solver. Species base tail frequencies remain the existing design values in `Simulation/Ecology.h`.

Research on median-fin movement in trout, sunfish and yellow perch motivates treating these fins separately from the body: [Tytell, Standen and Lauder, 2008, *Escaping Flatland*](https://pubmed.ncbi.nlm.nih.gov/18165246/). Its species and experimental scope do not validate our five Walensee models, frequencies or angular amplitudes. Those parameters still require species-specific reference footage and expert review.

## Representation

Blender exports two extra UV channels alongside the existing color atlas. `FinOffset` stores the root-relative Y/Z coordinates divided by model length, biased by 0.5; `FinKind` stores the fin category divided by eight and the root's longitudinal coordinate, also biased by 0.5. The shader reverses FBX's V-coordinate flip when decoding them. The zero vector at every root remains zero under rotation, so the fin cannot pull its root away from the body. Eyes and body have a neutral rig category.

The shader rotates the encoded ray about the local longitudinal axis, then applies the instance's right/up basis and scaled length. This keeps motion aligned with a fish's actual heading, pitch and size. Data is carried in 13 per-instance floats: tail phase, right X/Y, individual phase, body stroke amplitude, right Z, up X/Y/Z, fin phase, effort, turn and body length. `Tools/fish_motion_shader.hlsl` is the source used to build the Unreal material.

The editor's mesh description contains three authored channels. Its built render buffer additionally contains generated lightmap UVs at index 3. The rig check accepts that fourth channel while verifying fin categories, roots and both paired fins in the authored channels. It does not assume that a channel-count match alone proves a working rig.

The animation uses simulation state, not engine wall-clock time. The existing world pause gate stops the state updates and leaves GPU instance values unchanged. Nearby streamed fish retain both phases, effort and steering state; newly populated slots reset their state.

## Boundaries and verification

The portable motion test covers paused and invalid inputs, cruise cadence, independent phases, speed/turn response, 30-vs-120-Hz consistency and a two-hour numerical phase run. This last check is not a two-hour rendered game soak. Blender checks fin categories, stationary roots and moving tips; Unreal checks imported channels and retained vertex colors. An isolated render fixture holds the body wave fixed and changes only the paired-fin pose, including pitched/yawed instances at reduced LODs. The normal fish fixture and gameplay cases remain separate. The population test invokes the actual world tick with the game's menu pause active and compares positions, phases and GPU values before/after; it also checks retained animation state across cell boundaries.

The deformation is a limited hinge/wave approximation. It does not reconstruct measured ray-by-ray bending or solve fluid forces. Gill ventilation, mouth opening, detailed fin-surface normal deformation, natural acceleration maneuvers, species-specific stroke validation and full photorealistic acceptance remain open.

These two unedited packaged frames hold the body wave and camera fixed while changing the paired-fin phase. They demonstrate deformation, not a natural swimming trajectory:

![Perch paired fins, first controlled pose](Images/Fin_Perch_A.png)

![Perch paired fins, second controlled pose](Images/Fin_Perch_B.png)

## Packaged result

Verified on 26 November 2026 in `Builds/WindowsFinMotion/divingsim.exe`: all eight cases passed with 106 fresh images. The isolated paired-fin comparisons passed for five species × four LODs: at least 4,319 pixels changed above a luminance threshold of eight in the fin region, while at most two did in the fixed head region. These regions are specific to the controlled fixture; the test is not an anatomical classifier. Visual inspection confirmed the independent fin poses and correctly oriented movement on pitched/yawed fish. The normal population, actual pause gate, cell-boundary retention, photo visibility and decompression regression cases passed.

[FIN_MOTION_RESULTS.json](FIN_MOTION_RESULTS.json) binds the child executable (`1b65bb30903a6c5635f1c269c0a6630b0e383b17d07d8844edae3d769e0c713e`), cooked containers, source/config snapshot, material/mesh assets, shader, core-test log and image evidence. The root launcher started this child and completed a dive. The previous WindowsFish executable **and all its cooked containers** match their recorded hashes. Raw case evidence is in `artifacts/fin-validation-20261126-101133`.

The 60-second 1920×1080 quality-2 underwater capture measured 75.08 FPS average, 60.94 FPS low-1%, 15.28 ms p95 and 17.52 ms maximum, with zero frames above 33.3 ms. This RTX 3090 development-host capture kept the existing editor open; it is not minimum-hardware or whole-lake performance acceptance. Raw performance evidence is in `artifacts/perf-underwater-1080p-q2-20261126-102027-356`. [FIN_MOTION_CI.json](FIN_MOTION_CI.json) records four portable C++ test programs and Python checks passing on Windows, macOS and Linux for source commit `e006f8d`; this is not a native Unreal macOS package test.

## Reproduction

Generate and audit the fish with the Blender tools documented in [SIMULATION.md](SIMULATION.md#fish-geometry-and-surface-detail). The texture atlases are unchanged.

```powershell
./Tools/TestCore.cmd
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/Build.ps1 -Target Content -EditorSandbox -OnlyFish
./Tools/SmokeTest.ps1 -EditorSandbox -Scenario fish -FinInspection -MaxFPS 30 -RuntimeLog artifacts/fin-editor-inspection.log
./Tools/Build.ps1 -Target Package -EditorSandbox -BuildSlot FinMotion -PackageDirectory Builds/WindowsFinMotion
./Tools/TestFinMotion.ps1
./Tools/TestFinLauncher.ps1
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsFinMotion -Scenario underwater -Resolution 1080p -Quality 2 -Seconds 60
./Tools/.geodata-env/Scripts/python.exe Tools/ReportFinMotion.py artifacts/fin-validation-20261126-101133 --performance artifacts/perf-underwater-1080p-q2-20261126-102027-356
```
