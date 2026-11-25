# Body and equipment contacts

The diver now moves through the world with 24 contact shapes derived from the same pose as the visible body and equipment. Torso, BCD sides, head, mask, mouthpiece, cylinder, arms, thighs, shins, boots, fins, gloves, computer and torch participate. A standing support capsule handles walking; swimming uses a small eye anchor together with the articulated body. The shapes are conservative capsules and mesh bounding boxes, not triangle-exact anatomy.

The pose advances at the simulation's 50 Hz step. Rendering consumes the accepted pose, including arm IK, leg joints and sampled foot slope. Cosmetic camera bob affects the view only. A blocked fin stroke or wrist movement can pause independently of a clear turn. Turns that lack clearance retain the requested direction and are retried as space becomes available; the controller follows the accepted physical gaze. Large turns rebuild the rig at angular intervals of at most two degrees, checking the cylinder's orbit and limb motion between those poses. Checking only endpoint boxes would miss some obstacles on this orbit.

Each translation sweeps every member against Unreal's blocking world geometry, chooses the earliest contact and slides along the contact planes, with at most four iterations. A 0.25 cm query shell detects grazing contact before the physical shape touches the surface. The solver retains one centimetre of normal separation when a hit is found and can restore consumed separation through another compound sweep. A separating contact with that shell is rechecked using the actual shape, so it can leave the surface while still detecting an obstacle farther along the same path. This avoids repeated near-zero-time hits freezing fins on a sloped triangle at the lake's large world coordinates. Pose queries use a 0.5 cm shell and sample each shape's interpolated movement at intervals of at most one centimetre. These are numerical game tolerances.

The standing-to-prone transition retracts the heels before extending the fin blades. Returning to a landing uses swept upward movement before the standing support capsule is enabled. The shore test first surfaces away from the staircase, then approaches its ramp: a direct diagonal ascent from below a deck is now correctly blocked by hands and equipment. Test success still requires a dive deeper than three metres, more than twenty metres travelled, return to the original landing height, a standing physical pose, air breathing and a saved completed log.

At spawn, the selected loadout is checked for overlap and can be raised in two-centimetre increments up to two metres. If no clear placement is found, starting the dive returns to the menu with an explanation. During gameplay regression tests, every movement is additionally checked for actual body overlap; benchmark runs omit this test-only integrity query.

When a fin, boot or shin prevents a posture change, the controller can first retract the knees in small, collision-checked increments. This gives the heels clearance on a ramp before the torso rises or turns. The tuck relaxes when the pose path permits it, and the visible joints use the same value. This is a limited clearance adjustment, not general foot-placement IK.

## Reproduce

```powershell
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/Build.ps1 -Target Package -EditorSandbox -BuildSlot Contacts -PackageDirectory Builds/WindowsContacts
./Tools/TestAvatar.ps1 -PackageDirectory Builds/WindowsContacts
./Tools/TestContacts.ps1 -PackageDirectory Builds/WindowsContacts
```

Run the native suite with the editor executable and the sandbox project:

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'O:/games/dive game/Builds/EditorSandbox/divingsim/divingsim.uproject' '-ExecCmds=Automation RunTests DiveSim;Quit' '-ReportExportPath=O:/games/dive game/artifacts/ContactsAutomation' -nullrhi -nosound -unattended -nosplash
```

The native `DiveSim.Contacts` suite covers translation and sliding, thin walls and corners, cylinder-first contact, horizontal clearance, an obstructed intermediate rotation, a grazing slope at lake coordinates, separation followed by a thin ceiling, imported-rig joint and equipment alignment, actual cylinder/fin/hand contacts and the imported cylinder's orbit during a large turn. The imported-rig alignment matrix includes six loadouts and four poses.

The avatar fixture now checks the achieved physical gaze at −65° and +85°. When an obstacle prevents the turn, it uses ordinary upward swimming thrust to gain clearance, with a fifteen-second phase limit. It captures the requested gaze only after the physical pose reaches it; a requested controller angle alone cannot pass the test.

The breathing regression checks air breathing when the mouth is actually above the local wave, rather than assuming that a fixed timestamp is dry. If a wave wets the mouth after regulator removal, it checks the no-supply state, warning and unchanged cylinder gas as well.

## Verification

The packaged WindowsContacts avatar matrix passed all six equipment cases and all 25 phases per case, with 66 fresh screenshots. Each case reached the explicitly checked +85° upward gaze. Final-package spot checks include the swimming preview, computer, extreme upward gaze, underwater movement and return to shore. Screenshots are not automatic visual acceptance: the coarse shoreline imagery and simplified character surfaces remain visible. [Avatar evidence](CONTACTS_AVATAR.json) records the actual angles, blocking parts and image/log hashes.

Thirteen further packaged gameplay cases passed: three shore routes, daytime/night wrist-computer use, all three torches, breathing-source switching, the tutorial, loadout switching, Exploration and Hardcore. They produced 70 fresh screenshots. Together with the avatar matrix, these are 19 cases and 136 screenshots from the same executable. No runtime body-overlap integrity check failed. [Gameplay evidence](CONTACTS_REGRESSION.json) records the cases, actual blocking parts and artifact hashes.

Twenty-five Unreal automation tests, fifteen Python tests and the portable simulation checks passed. The 101 source/configuration files match the build sandbox; all 25 avatar assets retain their preceding hashes. The WindowsBody executable is preserved unchanged. [Build and source evidence](CONTACTS_BUILD.json) records these checks and the executable SHA-256, `18a01ee6dad21e4e6d7b669c90ee7b8139c112aeb872adbeda06eebcbf5229e3`.

A 60-second underwater capture after 15 seconds of warmup measured **86.96 mean FPS / 71.19 one-percent-low FPS**, at 1920×1080, quality 2 and 100% render scale on the RTX 3090 / Ryzen 9 9950X3D. All 5,216 measured frames stayed below 16.67 ms; the maximum was 15.0835 ms. No other Unreal editor or game instance was present at capture start. [Underwater performance evidence](CONTACTS_PERFORMANCE.json) records the settings, host and artifact hashes. This short capture does not establish performance on minimum hardware or other platforms.

A separate stationary shore capture with the same settings measured **98.09 mean FPS / 82.43 one-percent-low FPS** across 5,883 frames, with a maximum of 14.3866 ms. No other Unreal instance was present at its start either. [Shore performance evidence](CONTACTS_SHORE_PERFORMANCE.json) includes the unchanged standing location; this is a rendering/idle-contact measurement, not a walking or streaming benchmark. Both benchmark frames were visually inspected.

The ordinary `Builds/WindowsContacts/divingsim.exe` launcher started the expected named game executable and completed its gameplay check with exit code 0: [launcher evidence](CONTACTS_LAUNCHER.json).

## Limits

This is a kinematic compound-body controller. It does not supply a dynamic ragdoll, independent finger/hose/cloth/strap collision, body self-collision or physically simulated impact impulses. Limb capsules and equipment boxes approximate the visible surfaces. The regulator's inserted mouthpiece is conservatively represented even while it is being removed. Unexpected geometry spawning inside the body does not yet have a general penetration-recovery solution. General contact-aware IK, anatomical and visual acceptance, minimum-hardware testing and native macOS verification remain open. This increment addresses DIVR-010 and parts of DIVR-001/011/012; it does not complete the full 236-requirement acceptance.
