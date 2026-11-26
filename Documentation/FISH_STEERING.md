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
```
