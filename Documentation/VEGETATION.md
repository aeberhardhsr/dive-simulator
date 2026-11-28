# Räumliche Vegetation am Walensee

Aktuelle Ergänzung: [Korrigierter Bodenkontakt der Baum-Fernstufen](VEGETATION_ROOTS.md), geprüft am 28. November 2026. Die nachstehend beschriebenen historischen Pakete bleiben unverändert.

Der Windows-Entwicklungsbuild `Builds/WindowsVegetation/divingsim.exe` ergänzt die Landschaft um räumliche Bäume mit PBR-Materialien, Schatten, Windbewegung und vereinfachter Stammkollision. Er führt die Uferergänzung und den Tauchablauf aus `WindowsCoast` fort. Der bisherige Build bleibt unverändert erhalten. Diese Ergänzung bearbeitet WRLD-008; sie ist keine vollständige fotorealistische Landschaftsabnahme.

Gleicher Blick vom Weesener Einstieg zum Land, jeweils 1600 × 900 im verpackten Spiel:

Vorher, `WindowsCoast`:

![Weesen ohne räumliche Bäume](Images/Vegetation_Before.png)

Nachher, `WindowsVegetation`:

![Weesen mit räumlichen Bäumen](Images/Vegetation_After.png)

Die Bäume ergänzen Tiefe und Schatten. Das grobe Bodenluftbild und seine bereits fotografierten Kronen/Schatten bleiben im Vergleich sichtbar.

## Geografie und gestaltete Anteile

Die Ausgangsdaten stammen aus [swissTLM3D 2.4, Ausgabe Februar 2026](https://www.swisstopo.admin.ch/de/landschaftsmodell-swisstlm3d). Im bestehenden Landschaftsausschnitt von 22 × 13 km liegen 4.013 Wald-/Gehölzpolygone mit 655 Innenringen und 96.144 Einzelbaumspitzen. Einzelbaumspitzen sind vermessene Kronenpunkte, keine vermessenen Stammfüße. Der unabhängige Quellenvergleich liest die ursprünglichen SHP/DBF-Daten erneut und vergleicht Auswahl, Topologie und XYZ-Koordinaten: [Quellenprüfung](VEGETATION_SOURCE_AUDIT.json), [Provenienz](WALENSEE_VEGETATION_SOURCE.json).

Aus den Polygonen entstehen 716.761 gestaltete Waldkandidaten auf einem versetzten 12-m-Raster; offener Wald erhält geringere Dichte. Zusätzlich bleiben 93.837 geeignete vermessene Einzelbaumpositionen erhalten. Insgesamt enthält die Laufzeitdatei 810.598 **Kandidaten**, nicht gleichzeitig sichtbare oder vollständig vermessene Bäume. Höhen-/Wasserspiegelprüfung und Gebäudeabstand entfernen ungeeignete Kandidaten. Waldpunkte halten mindestens 5 m Abstand zu den Einzelbaumspitzen. Innenringe bleiben erhalten. [Platzierungsparameter](VEGETATION_PLACEMENT.json) und [unabhängige Prüfung der gespeicherten Positionen](VEGETATION_PLACEMENT_AUDIT.json) unterscheiden vermessene Punkte und gestaltete Verteilung.

Einzelbaumhöhen entstehen aus Kronenhöhe minus nativem swissALTI3D-Gelände. Baumarten, Waldhöhen, Dichte und Windverformung sind gestaltet. Laubbäume und Nadelbäume bilden eine höhenabhängige Mischung; dies ist keine örtliche Artenkartierung. Zur Laufzeit wird jeder Stamm am tatsächlich dargestellten Gelände verankert. Zusätzliche Freiräume halten die drei modellierten Zugangsrouten begehbar. Damit kann die endgültige Kronenhöhe von der vermessenen Höhe abweichen.

## Modelle und Laufzeit

Fünf Baumvarianten verwenden lizenzierte externe Quellen:

- [Poly Haven – Tree Small 02](https://polyhaven.com/a/tree_small_02): kleiner Laubbaum, zwei Quell-Detailstufen und zwei zusätzlich reduzierte Fernstufen.
- [Poly Haven – Fir Tree 01](https://polyhaven.com/a/fir_tree_01): drei Nadelbaumvarianten, jeweils zwei Quell-Detailstufen und zwei zusätzlich reduzierte Fernstufen.
- [tevy – European Beech](https://tevy.dev/models/european-beech), Version 1: Buche mit drei Detailstufen.

Die Quellen stehen unter CC0; Metadaten, Download-Prüfsummen und Konvertierung sind in [VEGETATION_ASSETS.json](VEGETATION_ASSETS.json) und [VEGETATION_BEECH_SOURCE.json](VEGETATION_BEECH_SOURCE.json) festgehalten. Ursprüngliche Quelldateien bleiben erhalten. Blender normalisiert Wurzelpunkt und Einheiten und exportiert die Detailstufen. Unreal verwendet Nanite mit Flächenerhaltung und native Mesh-LODs für den Fallback. Blattmaterialien sind beidseitig und maskiert; Normalen, Rauheit und Blattdurchleuchtung werden importiert. Der Wind erhält Richtung und Stärke aus dem gewählten Wetter und seine Zeit aus der Simulation; Pause hält diese Zeit an.

Dreiecke der exportierten Meshes; bei der Buche stammen alle drei Stufen aus der Quelle, bei den anderen Modellen sind LOD 2 und 3 zusätzlich reduziert:

| Modell | LOD 0 | LOD 1 | LOD 2 | LOD 3 |
|---|---:|---:|---:|---:|
| Buche | 38.648 | 17.674 | 5.708 | – |
| Nadelbaum A | 177.444 | 70.661 | 12.000 | 10.357 |
| Nadelbaum B | 128.464 | 54.927 | 12.000 | 4.847 |
| Nadelbaum C | 68.879 | 31.512 | 11.999 | 4.301 |
| Kleiner Laubbaum | 2.062.487 | 495.533 | 19.981 | 13.618 |

Feste 256-m-Zellen werden um die Spielfigur geladen. Der Radius beträgt fünf Zellen je Achse, höchstens 121 Zellen; normaler Spielbetrieb ergänzt höchstens eine neue Zelle pro Frame. Bestehende Zellen behalten ihre Instanzen. Bei Rückkehr entstehen dieselben Transformationen. Nahe Stämme besitzen unsichtbare einfache Kollisionszylinder; Blätter und Äste besitzen keine eigene Kollision. Der Render-Abstand liegt bei ungefähr 1,15 km; weiter entfernte Wälder bleiben Bestandteil des Luftbilds.

## Reproduktion

Die vollständige Quellenbeschaffung benötigt mehrere GB temporären Speicher. Rohdaten und Build-Ausgaben werden nicht ins Repository eingecheckt; Modelle, FBX-Dateien, Geodaten und Unreal-Inhalte verwenden Git LFS.

```powershell
./Tools/.geodata-env/Scripts/python.exe -m pip install -r Tools/requirements-coast.txt
./Tools/.geodata-env/Scripts/python.exe Tools/download_vegetation.py
./Tools/.geodata-env/Scripts/python.exe Tools/validate_vegetation_source.py
./Tools/.geodata-env/Scripts/python.exe Tools/prepare_vegetation_assets.py
./Tools/.geodata-env/Scripts/python.exe Tools/prepare_beech_asset.py
node Tools/decode_beech.mjs
& 'C:/Program Files/Blender Foundation/Blender 5.3/blender.exe' -b --disable-autoexec --python-exit-code 1 --python Tools/create_vegetation_assets.py
./Tools/.geodata-env/Scripts/python.exe Tools/prepare_vegetation.py
./Tools/.geodata-env/Scripts/python.exe Tools/package_vegetation.py
./Tools/.geodata-env/Scripts/python.exe Tools/validate_vegetation_placement.py
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/Build.ps1 -Target Content -OnlyVegetation -EditorSandbox
./Tools/Build.ps1 -Target Package -EditorSandbox -BuildSlot Vegetation -PackageDirectory Builds/WindowsVegetation
./Tools/TestVegetation.ps1 -PackageDirectory Builds/WindowsVegetation
python Tools/ReportVegetation.py artifacts/vegetation-validation-YYYYMMDD-HHMMSS
```

Die allgemeine Geodatenvorbereitung für Landschaft, Gebäude und Ufer muss vorher abgeschlossen sein. `--python-exit-code 1` macht Fehler im Blender-Skript als Prozessfehler sichtbar.

## Prüfstand vom 26. November 2026

Die Implementierung ist in [Commit ff57b5f](https://github.com/aeberhardhsr/dive-simulator/commit/ff57b5f7e2f578aa7c81d24b880c3c0ad2adbc29) gespeichert. Die [portable CI-Prüfung](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45769472775) besteht auf Windows, Linux und macOS; sie umfasst zwei C++-Prüfprogramme und 28 Python-Tests, keinen nativen Unreal-Spieltest auf macOS. [CI-Nachweis](VEGETATION_CI.json).

Das verpackte Spiel besteht 14 Fälle mit 57 frischen Prüfbildern: drei Landansichten, zwei Nanite-Fallbacks, eine 4K-Ansicht, drei vollständige Uferabläufe, drei Küstenüberquerungen mit Rückkehr, Fotosammlung und Steuerung. Der Start über die normale `divingsim.exe` besteht ebenfalls. 35 native Unreal-Tests, beide portablen C++-Prüfprogramme und 28 Python-Tests bestehen. [Paket-, Quellstand- und Bildnachweis](VEGETATION_RESULTS.json).

Im Weesener Landtest sind 84 Zellen mit 16.688 Bauminstanzen geladen; sechs nahe Stammproben treffen ihre Kollision. Der gezielte Streamingtest verschiebt den Ladefokus um 300 m, anschließend um 3,5 km und zurück. Eine bestehende Zelle bleibt beim ersten Schritt erhalten, wird beim Fernschritt entfernt und kehrt mit denselben 38 Transformationen zurück. Dies ist ein kontrollierter Ladefokustest, kein real durchlaufener 3,5-km-Weg. Die Materialparameter für Wind und Zeit werden gegen die Simulation geprüft; damit ist keine botanisch korrekte Windverformung nachgewiesen.

Die unabhängige Platzierungsprüfung findet maximal 1,07 mm horizontalen Rundungsfehler bei übernommenen Einzelbaumspitzen und 0,063 mm Fehler bei deren rekonstruierter Kronenhöhe vor Laufzeit-Verankerung. Alle gestalteten Waldpunkte liegen innerhalb ihrer zugeordneten Waldklasse, einschließlich der Aussparungen in den Quellen. Der kleinste gemessene Abstand eines Waldkandidaten zu einer Einzelbaumspitze beträgt 5,079 m.

Das getestete Spielprogramm hat SHA256 `264144503e29fc4cca00a4b582f0cbca7830bb0c7712e0d9bdba9928af9a360e`. Paket-Prüfsummen unterscheiden zusätzlich die Asset-Stände; eine identische EXE-Prüfsumme allein genügt dafür nicht. Messungen und Grenzen stehen in [VEGETATION_PERFORMANCE.json](VEGETATION_PERFORMANCE.json).

Endgültiger Paketstand, jeweils 60 Sekunden nach 15 Sekunden Aufwärmzeit bei 1080p / Qualitätsstufe 2 / 100 % Renderauflösung:

| Szene | Mittlere FPS | 1-%-Low FPS | Langsamster Frame |
|---|---:|---:|---:|
| Unter Wasser, Weesen | 75,57 | 61,84 | 17,08 ms |
| Ufer, Murg, vollständiger Kameraschwenk | 84,24 | 66,99 | 15,92 ms |
| Derselbe Uferlauf ohne Nanite | 75,01 | 39,46 | 26,56 ms |

In diesen drei Messungen überschreitet kein Frame 33,3 ms. Die Messungen betreffen diesen Entwicklungsrechner mit geöffnetem Benutzer-Editor; sie sind kein Nachweis für Mindesthardware oder lange Wanderungen durch nachladende Waldzellen.

Die erste Messung ohne Nanite zeigte 51,21 FPS im Mittel, 15,47 FPS im 1-%-Low und einen langsamsten Frame von 67,42 ms. Die zusätzlichen Fernstufen verbessern denselben 60-Sekunden-Uferlauf auf 75,46 FPS, 39,53 FPS und 26,82 ms; kein Frame überschreitet dort 33,3 ms. Die [ursprüngliche Messung](VEGETATION_PERFORMANCE_BEFORE_LODS.json) bleibt mit ihren Paket-Prüfsummen erhalten. Diese Werte gelten für RTX 3090 / Ryzen 9 9950X3D bei 1920 × 1080, Qualitätsstufe 2 und 100 % Renderauflösung; der Benutzer-Editor blieb geöffnet. Sie belegen keine Mindesthardware-Freigabe.

Ein Zwischenlauf der Walenstadt-Route blieb nach wenigen Metern stehen und endete mit Zeitüberschreitung. Das Protokoll zeigte auslaufenden Vortrieb ohne gemeldeten Kollisionskontakt; ein konkreter Desktop-Fokuswechsel wurde dort nicht aufgezeichnet. Der Smoke-Testtreiber prüft nun den tatsächlichen Unreal-Tastenstatus und stellt eine noch beabsichtigte synthetische Halte-Eingabe bei Verlust wieder her. Die Küstenprüfungen setzen diesen Status absichtlich zurück und verlangen anschließend weiterhin die vollständige Überquerung und Rückkehr. Die normale Spielersteuerung wird dadurch nicht verändert; fehlgeschlagener Lauf und Wiederherstellungsnachweis sind im Paketbericht festgehalten.

## Grenzen

Der Boden bleibt überwiegend das vorhandene Luftbild auf dem Gelände: fotografierte Baumkronen und Schatten sind dort weiterhin sichtbar. Straßen sind noch nicht als Vegetations-Ausschlussflächen erfasst. Saisonale Blattänderungen, originalgetreue Park-/Gartenbäume, Gräser, Waldmodelle jenseits des Ladebereichs und eine visuelle Prüfung der gesamten Landschaft fehlen. Die Quellen stammen aus unterschiedlichen Aufnahmejahren. Ein Kronenpunkt kann neben dem tatsächlichen Stamm liegen; einfache Kollisionszylinder bilden Äste und Wurzelwerk nicht ab. Die reduzierten Fernstufen verlieren Blatt-/Astdetails; ihre Wirkung muss auch auf schwächerer Hardware bewertet werden. Dieser Stand ersetzt weder Mindesthardware-/macOS-Tests noch den zweistündigen Test des endgültig abgenommenen Builds.
