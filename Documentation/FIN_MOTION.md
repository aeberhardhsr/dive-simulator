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
```
