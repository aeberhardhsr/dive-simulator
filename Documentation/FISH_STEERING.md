# Bounded fish steering and obstacle paths

The previous collision fallback negated the complete fish velocity when a proposed position was invalid. That caused an instantaneous reversal, and orienting a stopped fish from its zero velocity could also reset its heading. The new steering state keeps heading separately from speed and approaches a new direction at a bounded angular rate. This develops BIOL-004/005/007/008; it is not a complete biological swimming or rigid-body contact model.

## Movement

The existing schooling, separation, home-area, habitat and diver-escape rules supply a preferred direction. An engine-independent SI model (`Simulation/FishSteering.h`) checks that direction and the current heading ahead of the fish. Where the path is obstructed, it evaluates alternative yaw/pitch directions, favoring clear travel while retaining the preferred route and current orientation.

Turning is bounded in yaw/pitch together, and pitch stays within ±0.55 radians. The authored angular limit depends on body length and escape state, bounded to 0.8–4 radians/s. Speed changes are limited to 0.8 m/s² normally and 2.5 m/s² during escape; turning away from the current heading and limited clearance reduce the target speed. These are game-response parameters, not measured species-specific locomotion values.

Current remains a separate displacement term. Every actual displacement is checked before it is applied. A blocked step stops at a safe fraction and drops swimming speed to zero while retaining the gradually changing heading. There is no velocity negation or teleport to a supposedly safe point. Fin cadence/effort follows swimming speed; asymmetric fin motion follows the bounded change in heading. Nearby streamed fish preserve heading and speed along with both animation phases.

## Geometry checks

`ADiveWorld::TraceFishPath` combines analytic segment/sphere intersection against conservative rock exclusion volumes with bathymetry/habitat samples at intervals of at most 25 cm. Rock radius includes half the fish's scaled length. Terrain probes reject missing survey data and species-depth/bottom-clearance violations. Sampling is a limited approximation to the continuously varying terrain envelope; the fish does not have an articulated collision hull. Fish-to-fish separation remains steering, not a rigid-body collision solver.

The predictive rays use the requested/current swimming speed and current advection. A small clearance reserve starts braking before contact. If current pushes a fish into a closed region despite its chosen movement, the final path check holds it outside the obstacle. Resolving such cases with a measured active station-holding or hydrodynamic model remains open.

## Verification scope

The portable test covers crossing a rock even when the endpoint is clear, moving away from a rock, pause, gradual reversal, obstacle traversal at 30/60/120 Hz, a sloped bank, current advection and a completely blocked drift. The obstacle route reaches the far side without penetration or emergency stops at each tested rate.

The native population fixture uses copies of each available species' state against the actual loaded lake and rock data for 60 steps per species. It verifies each entire step and the angular limit. It restores gameplay state and does not replace the ordinary game motion. The normal smoke routes additionally reject angular-limit violations or unsafe path steps on every update. Pause and streaming retention include heading, pitch and speed. The packaged matrix covers the three entries, streaming and photo detection. The separate fin/body pose fixtures are not rerun for this steering-only change; all 26 fish asset hashes must match the previously verified fin-motion package.

This does not certify natural maneuvering across every species, fish-body contact, predator/prey behavior or the complete BIOL requirements. The existing fin/body close-ups remain controlled pose fixtures, not evidence of these natural routes.

## Packaged result

On 26 November 2026, `Builds/WindowsFishSteering/divingsim.exe` passed all five packaged cases with 13 fresh images: Weesen, Murg, Mols, population streaming and photo discovery. The native and packaged world challenges each covered five species × 60 steps; the measured maximum turn was 0.075000 rad against a 0.075001-rad tolerance. The actual pause gate and nearby-fish retention checks passed. Captures were visually inspected for ordinary rendering and photo collection; still images do not prove continuous turning behavior.

The root launcher started the expected child (`77160de878c1c3a74ff55664e955000133d60a9c12833a695a8722ba2bf4370b`) and completed a dive. Raw case evidence is in `artifacts/steering-validation-20261126-104636`. [FISH_STEERING_CI.json](FISH_STEERING_CI.json) records five portable C++ test programs and Python checks passing on Windows, macOS and Linux for source commit `1f5db8e`; this is not native Unreal macOS verification.

[FISH_STEERING_RESULTS.json](FISH_STEERING_RESULTS.json) binds the executable, cooked containers, source/config files, native test log, captures and performance evidence. All 26 fish asset hashes match the previous fin-motion report. The WindowsFinMotion executable and all its cooked containers remain unchanged.

The 60-second 1920×1080 quality-2 underwater capture on the RTX 3090 measured 72.25 FPS average, 59.21 FPS low-1%, 15.86 ms p95 and 18.16 ms maximum, with zero frames above 33.3 ms. Evidence: `artifacts/perf-underwater-1080p-q2-20261126-105241-873`. The user's separate game instance had ended. The Unreal start window remained as a background process: normal close did not end it, and Windows denied process termination. This development-host sample is not minimum-hardware, whole-lake or endurance acceptance, nor a controlled before/after performance experiment.

## Reproduction

No asset regeneration is required; the previous fin meshes, atlases and shader are reused.

```powershell
./Tools/TestCore.cmd
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/SmokeTest.ps1 -EditorSandbox -Scenario stream -Entry 1 -PopulationChecks -MaxFPS 30 -RuntimeLog artifacts/steering-editor-stream.log
./Tools/Build.ps1 -Target Package -EditorSandbox -BuildSlot FishSteering -PackageDirectory Builds/WindowsFishSteering
./Tools/TestFinMotion.ps1 -PackageDirectory Builds/WindowsFishSteering -SteeringChecks
./Tools/TestFinLauncher.ps1 -PackageDirectory Builds/WindowsFishSteering -ArtifactPrefix steering
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsFishSteering -Scenario underwater -Resolution 1080p -Quality 2 -Seconds 60
./Tools/.geodata-env/Scripts/python.exe Tools/ReportFinMotion.py artifacts/steering-validation-20261126-104636 --performance artifacts/perf-underwater-1080p-q2-20261126-105241-873 --steering
```
