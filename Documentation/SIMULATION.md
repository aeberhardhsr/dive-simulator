# Simulation model, version 0.1

All physiological and decompression values are unvalidated gameplay outputs. The model must not be used as a real dive computer, training aid or medical prediction.

## Units and integration

The independent header `DiveModel.h` uses metres, seconds, newtons, kilograms, litres and bar. Unreal converts distances by 100 cm/m. The game applies physics in 20 ms steps. Pressure uses freshwater density 1000 kg/m³ and gravitational acceleration 9.80665 m/s². The scenario surface pressure is configurable, initially 0.965 bar; it is not a live weather measurement.

`P = P_surface + rho * g * depth / 100000`.

The body has a nominal 100 kg inertial mass. Thrust, quadratic drag relative to the current, suit compression, remaining gas mass, ballast and BCD gas contribute to movement. Suit, body and lung displacements are deliberately approximate. There is no full hydrodynamic body/fin model yet.

## Gas and buoyancy

Surface respiratory minute volume starts at 16 L/min and rises deterministically with effort, stress and fatigue. Ambient pressure scales tank consumption. BCD inflation draws gas from the same tank. BCD gas is stored as equivalent litres at one bar; its current volume is this quantity divided by ambient pressure. An 18 L relief limit vents excess gas. Empty cylinders cannot supply additional BCD gas.

Breath phase drives lung displacement, exhalation sound and bubble emission. Ascending bubbles use the cube root of the pressure ratio to change radius. The player must distinguish swimming upward (Space) from inflating the BCD (Q).

## Physiology

Heart rate and breathing rate approach targets derived from exertion, stress and fatigue. Stress responds to low gas, plan exceedance and rapid ascent. Cold exposure depends on depth temperature and suit choice. Simulated SpO2 declines after gas exhaustion underwater, with a terminal gameplay state. These are qualitative couplings, not clinical relationships.

The thermocline is a smooth logistic transition, initially centred on 10 m with a 2 m transition scale, between 18 °C and 6 °C defaults. Surface/deep temperature, centre depth and transition scale are configurable in the environment menu. These values are scenario parameters, not Walensee measurements.

## Nitrogen approximation

Sixteen nitrogen compartments use exponential uptake/release under constant pressure within each integration step. Public Bühlmann nitrogen half-times and coefficients are used with a fixed gradient factor of 0.8. The computed ceiling is the maximum compartment ambient pressure requirement converted to freshwater depth. Displayed NDL is time at the current depth until the surface limit is reached; it does not simulate an ascent, gas switching, repetitive dives, GF-low/high interpolation or a complete stop schedule.

Reference for the published coefficient family and conventions: [Subsurface's decompression model](https://github.com/subsurface/subsurface/blob/master/core/deco.cpp). No Subsurface implementation code is included. Coefficients are numerical model data; integration and game logic here are independently implemented. The first compartment uses the 4-minute variant (a=1.2599, b=0.5050).

A separate gameplay safety-stop timer counts 180 seconds at 3–6 m after exceeding 10 m. It is not a substitute for the compartment ceiling. Model validation against independent reference profiles is still required before claiming faithful decompression simulation.

## Modes and persistence

Exploration uses unlimited tank gas and automatic neutral trim, with breathing-induced movement retained. Simulation uses finite gas and manual BCD. Hardcore currently exposes the same simulation rules; equipment faults and reduced assistance are pending and explicitly labelled in the menu.

Samples are recorded once per second. Versioned JSON includes position, depth, gas, vital signs, warnings and temperatures. Every 30 samples, a temporary file is written and renamed to the session log. Completion, interruption and gas-exhaustion states are distinct. Repeated dives currently reset tissues; persistent repetitive-dive physiology is not implemented.

The logbook reads schemas 1 and 2. Schema 2 adds the actual dive start in UTC and an automated-test flag; legacy dates are extracted from the local-time filename without claiming UTC. Background jobs scan metadata and read the selected profile without accessing Unreal objects. The index retains summaries only. Unsupported or corrupt files are counted and skipped; `.tmp` files are ignored. Numeric types, finite values, ranges and nondecreasing sample times/distances are checked before plotting. Per-file limits are 64 MB and 100,000 samples; the reader does not truncate valid profiles silently. Automated runs are hidden from the player list.

The browser paginates newest filenames first and filters by mode. Four chart groups expose all recorded scalar physiology, gas, depth, temperature and distance metrics. A shared mouse cursor selects the nearest recorded sample; warnings are the stored flags at that instant, and yellow markers indicate newly raised flags. Event arrows navigate to those transitions. Per-pixel extrema preserve narrow peaks when long profiles are drawn. No interpolation is used to invent missing vital signs. Empty records display an explicit empty state. The data remains read-only and does not resume physiology or alter a paused dive.

`Tools/TestLogbook.ps1` uses Unreal's [command-line automation runner](https://dev.epicgames.com/documentation/unreal-engine/run-automation-tests-in-unreal-engine?lang=en-US). Tests cover legacy/current formats, a 7,201-sample two-hour fixture, nearest-time lookup, missing and invalid values, ordering, unsupported versions, file indexing and isolated test-log filtering. This data-length test is not a two-hour live-game stability test. The `logbook` gameplay smoke scenario saves a real simulated dive, parses the output back, opens that file in the browser and captures all four chart groups in German/English.

## Water optics

The water surface uses Unreal's [Single Layer Water shading model](https://dev.epicgames.com/documentation/unreal-engine/single-layer-water-shading-model-in-unreal-engine), with base scattering coefficients RGB `(0.00015, 0.00045, 0.00032)` and absorption `(0.0016, 0.00035, 0.00055)` per centimetre, and phase G `0.35`. Both coefficient vectors scale by `14 / configured visibility in metres`. These are illustrative optical settings, not measured Walensee water properties. Underwater visibility additionally uses a distance-attenuation post-process and height fog. Camera tint, saturation and ambience fade according to the camera's depth below the moving surface.

## Waves and weather

`Simulation/WaterSurface.h` defines four linear deep-water components with wavelengths 20, 9, 4.5 and 2.1 m. Each uses `omega² = g k`, height `A sin(k·x - omega t + phase)`, analytical slopes and orbital water velocity. Dynamic pressure and orbital velocity decay as `exp(-k depth)`. Near the interface, pressure depth blends over 2 m to the local water column and is zero in air. This blend, the four-component spectrum, directional spread and amplitude `0.0025 × wind² × {1, .5, .25, .125}` are gameplay approximations. The linear dispersion and depth decay follow [MIT's linear water-wave notes](https://ocw.mit.edu/courses/2-017j-design-of-electromechanical-robotic-systems-fall-2009/51e6a1ab5a7fa9937bdb7aca645897fd_MIT2_017JF09_ch06.pdf). No shallow-water dispersion, shoaling, breaking waves, fetch model or measured Walensee spectrum is claimed.

Wind ranges from 0 to 12 m/s; the direction is where it blows **toward**, clockwise from north. At 12 m/s the conservative sum-of-amplitudes bound is 0.675 m; this is not significant wave height. At zero wind, both geometry and micro-normal ripples are flat. The same component coefficients and elapsed simulation time drive CPU physics and the material. Rendering uses a 257 × 257 half-metre grid centred on a 32 m snapped viewer position, plus outer flat-water rings. Geometric displacement fades between 32 and 48 m from the viewer to avoid coarse distant tessellation. The central grid covers the entire displaced area even immediately before recentering. Its bounds explicitly include vertical shader displacement. Distant micro normals remain animated. Buoys and their anchor lines follow this same rendered heightfield.

Player drag includes wave orbital water velocity. Emerged body volume and the upper movement limit use the local moving interface. Gas pressure, displayed depth and ascent rate use pressure depth; short waves do not produce full-height pressure oscillations far below the surface. Breathing bubbles stop at the local interface and expand with local pressure. The wave clock freezes with a paused dive; the world updates after the pawn.

Clear, overcast and rain are selectable scenario presets. A game-local copy of Unreal's default volumetric-cloud material changes cloud coverage/density, alongside sun/skylight intensity and atmospheric fog. Rain uses at most 1,600 instanced streaks in a 12 m volume near the viewer, advected with wind and removed beneath the water surface. Their unlit emission compensates exposure for readability; this is not physically validated precipitation/lighting. Weather, wind, temperature profile, visibility, current and hour persist in settings and each dive's JSON environment metadata. These values remain fixed during a dive. Live weather, rainfall audio, surface splash effects and reference-based optical acceptance remain open.

## Procedural ecology

`Simulation/Ecology.h` contains SI habitat envelopes, lengths, speeds, schooling flags and vegetation constraints. These are gameplay parameters, not population estimates or locally measured habitat maps. Fish occurrence is supported by the [Eawag Projet Lac Walensee report, 2014](https://www.gl.ch/public/upload/assets/3028/Projet_Lacs_-_Walensee_Schlussbericht_def.pdf?fp=1). The whitefish model represents the **Coregonus group**, not a taxonomically validated model of a particular Walensee species.

| Model | Length at scale 1 | Game depth envelope | Behaviour |
|---|---|---|---|
| Perch / Egli | 30 cm | 1.2–24 m | Local schooling, separation, diver avoidance |
| Roach / Rotauge | 24 cm | 1–14 m | Faster schooling, diver avoidance |
| Pike / Hecht | 75 cm | 1.5–16 m | Sparse solitary slow cruising, faster escape |
| Whitefish / Felchen group | 38 cm | 5–55 m | Open-water schooling, at least 2 m bottom clearance |
| Burbot / Trüsche | 55 cm | 8–80 m | Bottom-following, reduced daytime activity |

Each population is seeded from the current terrain cell. Up to 77 fish are placed only where actual bathymetry intersects the species envelope; the Weesen start can have no burbot. Individual scale varies from 0.75 to 1.15. Same-species neighbours contribute cohesion/alignment; all fish contribute short-range separation. The diver causes escape steering. Terrain look-ahead, missing-data rejection and conservative rock exclusion volumes constrain movement. Runtime smoke tests check mesh identity, finite state, depth envelopes and rock exclusion every frame. Fish-to-fish separation is steering, not a rigid-body contact solver. Feeding, reproduction and seasonal ecology remain open. Population rebuilding after a 55 m move needs a persistent streaming solution.

Models use original geometry and vertex colours: two dorsal fins and bars for perch, red iris for roach, elongated spotted pike, adipose fin for whitefish, rounded tail and chin barbel for burbot. Tail deformation uses vertex weights and per-instance world-side vectors, species-dependent cycles and escape rate. Accumulated animation phase avoids jumps when speed changes and pauses with the simulation. This is procedural vertex animation, not a skeletal anatomy simulation. Photorealistic textures, finer fin motion and independent anatomical review remain pending.

Three illustrative vegetation forms are available: broad-leaved pondweed, whorled stonewort and feathery milfoil. They are generic freshwater growth types, **not confirmed species records at the simulated locations**. References: [InfoFlora pondweeds](https://www.infoflora.ch/de/flora/potamogeton.html), [stonewort habitats](https://www.infoflora.ch/de/lebensraeume/typoch/1.1.1-armleuchteralgengesellschaft.html) and [milfoil morphology, Fisheries and Oceans Canada](https://www.dfo-mpo.gc.ca/species-especes/profiles-profils/eurasian-water-milfoil-myriophylle-epis-eng.html). Plant placement rejects steep slopes, rock footprints and insufficient light/depth. Visibility limits vegetation depth; maximum game depths are 10/18/7 m respectively. Vertex-weighted sway keeps stem bases fixed. Exact species, lake-specific mapping, seasonal growth and a physical plant-flow model remain open.

![Original Blender model overview, neutral inspection lighting](Images/EcologyAssets.png)

## First-person equipment and wrist display

`ADiveEquipment` attaches original glove, sleeve, wrist-computer and torch meshes to the first-person camera. C raises/lowers the left forearm with an interpolated pose; R cycles three pages while raised. The display remains available when the screen HUD is hidden with H. The selected wet/dry suit changes sleeve width and colour. The torch's existing spotlight now originates at its visible lens and uses the existing lamp switch/battery state. These are camera-relative static-mesh poses, not a full-body skeletal diver or inverse-kinematics system; arm/world clipping and anatomical refinement remain open.

The computer is a world-space surface in the housing recess, driven by a 768 × 512 canvas render target. It snapshots the existing simulation state at approximately 10 Hz; it has no separate physiology or gas model. All pages retain depth, tank pressure, elapsed time and heading. The dive page adds NDL, ascent rate, maximum depth, water temperature and stop timer; the vital page adds pulse, SpO2, respiration, core temperature, stress and fatigue; the gas page adds ppO2, ambient pressure, estimated gas time to 50 bar, ceiling and lamp charge. Exploration is explicitly shown as unlimited gas. The footer displays the highest-priority active warning. English/German labels follow the game language.

Exposure-compensated emission keeps the display readable in dark water. Numeric pixels are rendered after temporal reconstruction to avoid trails from changing digits. Because Unreal's [after-motion-blur translucency pass](https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/MaterialTranslucencyPass?application_version=5.4) disables hardware depth testing, the material explicitly compares scene and surface depth with a 1 mm tolerance. Automated wrist scenarios cycle all three pages with the HUD hidden, save main/vital/gas screenshots, export the source display texture and check that the render callback ran, all pages were drawn, the wrist reached its raised pose and the state snapshot is no more than 0.5 s old. These checks do not replace visual review of readability, distortion or rendering artifacts.

## Core test coverage

`Tools/TestCore.cmd` builds a standalone C++ test executable without Unreal. It checks hydrostatic pressure, depth scaling of gas use, exertion coupling, BCD consumption, expansion/relief, step partition invariance, exploration, tank exhaustion, tissue uptake/release, ceilings, stop timing and input sanitization. A passing result establishes internal consistency, not medical validity.

## Zustandsgekoppeltes Audio

`Simulation/AudioModel.h` erzeugt Einatmungsereignisse beim Phasenwechsel und Ausatmung bei Atemphase 0,52. Die Atemfrequenz stammt unmittelbar aus `DiveModel`; Tonhöhe und Dauer der Reglerklänge folgen dieser Frequenz. Lautstärke reagiert auf Anstrengung. Bei leerer Flasche beginnt kein neuer Atemzug; ein bereits eingeatmeter letzter Atemzug kann noch ausgeatmet werden. Sichtbare Blasen und der räumliche Blasenton nutzen dasselbe Ereignis. Über Wasser entstehen keine sichtbaren Atemblasen.

Der doppelte Herzton wird einmal je simuliertem Herzzyklus gestartet. Er wird bei Stress/Belastung deutlicher und besitzt einen eigenen Lautstärkeregler. Dies ist ein Komfort-/Feedbackeffekt, keine validierte Körperakustik. Das Jacket liefert tatsächlichen Ein-/Auslassfluss in lokalen Litern pro Sekunde einschließlich Überdruckablass. Leere Gasquelle bzw. leeres Jacket erzeugen keinen fiktiven Ventilfluss.

`UDiveAudio` verwaltet elf Kanäle. Ein Blasencluster startet 25 cm vor/15 cm unter dem Kopf und steigt mit 0,3 m/s im Weltkoordinatensystem auf; Distanzabschwächung und Stereoortung erfolgen durch Unreal. Die übrigen körpernahen Signale bleiben im Kopfhörermittelpunkt. Wasser-, Oberflächen-, Wind- und Regenkulisse sind unabhängige Stereoschleifen. Die Überblendung folgt der Kameratiefe unter der tatsächlichen Wellenoberfläche über 0–0,5 m. Oberflächengeräusche nehmen darunter exponentiell mit 3 m Skala ab; der Tiefpass läuft von 20 kHz auf 650 Hz. Dies sind abgestimmte Spielparameter, kein Schallausbreitungsmodell im See.

Neue Warnungen erzeugen einen Doppelton mit zwei Sekunden Schutz vor schnellem Wiederholen. Ein neuer Leerflaschenalarm umgeht diese Sperre. Anhaltende Warnungen bleiben visuell sichtbar, ohne in jedem Simulationsschritt zu piepen. Gesamt-, Umgebungs-, Ausrüstungs-, Herz- und Warnlautstärke werden multipliziert, auf 0–1 begrenzt und in den Einstellungen gespeichert. Pause hält sämtliche Quellen und Ereignisphasen an; Fortsetzen führt den Klang fort.

Die 48-kHz-PCM-Quellen entstehen deterministisch in `Tools/create_audio.py`. Schleifen verwenden eine 250-ms-Überblendung statt einer regelmäßigen stillen Stelle. Fremdaufnahmen werden nicht verwendet. Realistische Klangreferenzen, klangliche Abnahme auf Kopfhörern/Lautsprechern sowie eine detaillierte Unterwasserakustik bleiben offen.

Der Audiotest zeichnet den tatsächlich gerenderten Mastermix über [Unreals Submix-Aufnahme](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/AudioMixer/UAudioMixerBlueprintLibrary/StopRecordingOutput) auf. Er misst Signal, Übersteuerung, Pause, Stummschaltung und Wiederaufnahme. Diese technische Prüfung ersetzt keine Hörabnahme.

## Steuerung und Einführung

`FDiveControls` ist die gemeinsame Quelle für die 16 Aktionsbelegungen und ihre Anzeigen. `ADivePlayerController` verarbeitet Einzeldrücke, Tastenauswahl, das feste Escape und F1 über Unreals `FInputKeyEventArgs`. Bewegung/Jacket lesen denselben Schlüsselbestand als gehaltene Tasten; Mausachsen bleiben im Engine-Inputsystem. Eine Konfliktbelegung tauscht Aktionen, statt eine davon unbedienbar zu machen. Ungültige, doppelte und reservierte Einträge aus gespeicherten Einstellungen werden abgefangen. Die Einzel-Tastenbelegung unterstützt Tastatur und digitale Maustasten; Chords, Analogachsen und Gamepads sind hier noch nicht implementiert.

Die Steuerungsseite ist auch während einer Pause verfügbar. Pause und Änderungen geben gehaltene Eingaben über `FlushPressedKeys` frei. Sensitivität ist auf 0,03–0,5 begrenzt, das Sichtfeld auf 60–110 Grad. Alte Einstellungen ohne neue Felder verwenden die bisherigen Standards. Speicherung erfolgt über eine temporäre Datei und anschließendes Ersetzen; fehlgeschlagenes Speichern wird angezeigt.

`Simulation/Tutorial.h` prüft konkrete Aktionen: mindestens 25 Grad aufsummierte Blickbewegung, drei Meter Bewegung mit Schwimmeingabe (Strömungsdrift zählt nicht), mindestens 1,5 lokale Liter Jacket-Zufuhr und anschließende Abgabe, geöffneter Computer auf der Vitalwerteseite, Lampe aus/ein, Karte auf/zu sowie Rückkehr auf höchstens 0,8 m Tiefe innerhalb zwölf Metern zur Boje. Erst das erfolgreiche Beenden schließt die Einführung ab. Die Darstellung verwendet jederzeit die aktuelle Tastenbelegung. Der Übungstauchgang hat einen eigenen ruhigen Weesen-Tauchplan; Speichern während der Übung bewahrt den vorherigen normalen Plan. Abschluss und Abbruch stellen diesen Plan wieder her.

Die Gameplay-Tests erzeugen simulierte Unreal-Eingabeereignisse, keine Desktop-Tastendrücke. Reale Eingaben werden vom Testcontroller ignoriert. Der Belegungstest prüft den Weg über Controller, HUD-Tastenauswahl, gespeicherte Einstellungen und tatsächliche Schwimmbewegung. Der Einführungstest durchläuft alle acht Aufgaben im laufenden Spiel und liest Abschluss, Tauchlog sowie ursprünglichen Tauchplan wieder ein. Diese automatisierten Prüfungen ersetzen keine manuelle Prüfung mit verschiedenen Tastaturlayouts.
