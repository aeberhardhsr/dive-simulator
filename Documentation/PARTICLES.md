# Schwebeteilchen und Atemblasen

Der Partikelstand ergänzt die drei Tauchlampen um ein auf der GPU bewegtes Schwebefeld und transparente, druckabhängig expandierende Atemblasen. Der vorige Lampen-Build bleibt unter `Builds/WindowsTorch` erhalten. Die vollständige visuelle und fachliche Abnahme des Simulators bleibt offen.

## Geprüfter Windows-Stand

Start: `Builds/WindowsParticles/divingsim.exe`; der gesamte Paketordner wird benötigt. Der eigentliche Spielprozess heißt `divingsim-Particles.exe`, SHA-256 `64B544C97CB34729ED1FDB06C88EE2D2C6B096A74C76502C01F58DDA9C9B9402`. Engine 5.8.4, Development/Win64, 25. November 2026.

Auch der Root-Starter ist geprüft: Er startet den richtigen Particles-Prozess, dessen 22-s-Spieltest mit 23 Messpunkten und Exitcode 0 endet. Der Test nutzt ein eigenes Saved-Verzeichnis. Belege: `artifacts/particles-bootstrap.json` und `artifacts/particles-bootstrap.log`.

13 Paketfälle bestehen mit 33 frischen PNGs: Wide/Spot nachts, Wide tagsüber, ausgeschaltete Lampe, geringe Sicht, vergrößerte GPU-Diagnose und Nullgrößen-Kontrolle, ruhige Oberfläche, Sturm, Streaming, Audio, Atemregler und Pause. Das diagnostische Bild enthält 1.715 markierte Pixel außerhalb des HUD-Bereichs, das Kontrollbild null. Der 90-s-Streaminglauf legt 110,96 m zurück und meldet 20 Komponenten-Zellwechsel einschließlich initialer Platzierung; alle Seed-/Instanzdaten bleiben unverändert. Er erzeugt 784 Blasen und entfernt 720 an der Oberfläche. Die viersekündige Pause hält die Simulations-/Szenariouhr an.

Die C++-Kernprüfungen einschließlich Druck-Volumen-Invariante, 15 Python-Prüfungen und der gezielte Sandbox-Zeitstempeltest bestehen. Die [GitHub-Kernprüfung](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45640761516) besteht auf Windows, macOS und Linux. Implementierung: Commit `42176f4`. Rohbelege: `artifacts/particles-validation-20261125-155851`, maschinenlesbare Auswertung: [PARTICLE_RESULTS.json](PARTICLE_RESULTS.json). Die Laufzeitdaten wurden vor dem Commit auf dem im Bericht ausdrücklich als geändert markierten Quellstand erzeugt; der dortige `source_head` bezeichnet den vorherigen Commit.

Die abschließende Leistungsmessung auf RTX 3090/Ryzen 9 9950X3D umfasst 15 s Aufwärmen und 60 s Aufnahme bei 1920 × 1080, Qualität 2, 100 % Renderauflösung, ohne dynamische Auflösung, VSync oder FPS-Limit. 5.443 ausgewertete Bilder ergeben 90,74 FPS im Mittel und 73,46 FPS im langsamsten Prozent. Das langsamste Bild dauert 14,51 ms; kein Bild überschreitet 16,67 ms. Neun Geländekacheln bleiben durchgehend geladen. Es lief kein zweiter Unreal-/Spielprozess; sonstige Desktoplast ist nicht vollständig ausgeschlossen. Dies ist eine einzelne lokale Messung, keine Mindesthardware- oder Dauerlauffreigabe. Beleg: `artifacts/perf-underwater-1080p-q2-20261125-155726-995`, [PARTICLE_PERFORMANCE.json](PARTICLE_PERFORMANCE.json).

## Schwebefeld

640 deterministisch verteilte Körner bilden einen Würfel von acht Metern Kantenlänge um den Taucher. Ihre unveränderlichen Instanzdaten werden einmal übertragen. Der Vertexshader berechnet Strömungsdrift, räumliches Wiederholen, Rand-Ausblendung, Abstand zur Kamera und Ausschluss oberhalb der aktuellen Wellenfläche. Die CPU aktualisiert gemeinsame Parameter und verschiebt die Komponente nur beim Wechsel einer Acht-Meter-Zelle. Es gibt keine Schleife, die pro Bild 640 Teilchenpositionen auf der CPU berechnet oder hochlädt.

Das Material ist beleuchtet und im normalen Spiel nicht selbstleuchtend. Radien liegen zwischen etwa 0,5 und 1,5 mm. Die Dichte ist gestalterisch gewählt; Sichtweite verändert die optische Dämpfung, derzeit nicht die Kornzahl. Strömung folgt derselben konfigurierten Y-Richtung wie bisher, mit einer zusätzlichen kleinen X-Drift. Dieses analytische GPU-Verfahren verwendet World Position Offset, keine Niagara-/Compute-Fluidsimulation. Kollision dieser Schwebeteilchen mit Boden und Fischen sowie Mehrfachstreuung fehlen. Flossenbedingte Sedimentaufwirbelung ist inzwischen als eigener Effekt ergänzt: [SEDIMENT.md](SEDIMENT.md).

Der eigene Grain-Mesh besitzt konservative Bounds für die maximale Shader-Verschiebung. Die normale Unreal-Instanzoptimierung bleibt aktiv. Ein Material-WPO-Limit und größere Komponenten-Bounds allein reichten bei dieser Engine nicht aus: Frühere Versuche meldeten korrekte CPU-Daten, zeigten aber keine Körner. Erst ein gerenderter Diagnosetest mit vorübergehend ausgeschalteter Instanz-Ausblendung isolierte diese Ursache. Die endgültige Lösung erweitert die Mesh-Bounds ausschließlich dieses Systems.

Die Schwebekörner empfangen Beleuchtung, tragen aber nicht selbst zur indirekten Szenenbeleuchtung, Distance Fields oder Raytracing-Geometrie bei. Diese Darstellungen dürfen den unverformten Kugelmesh und seine großen Verschiebungs-Bounds nicht als reale Hindernisse oder große Beleuchtungsflächen behandeln. Ein erster Paketkandidat erreichte 80,81 FPS und zeigte gegenüber dem Lampenstand zusätzlichen Lumen-/Distance-Field-Aufwand; deshalb wurde diese Teilnahme ausdrücklich deaktiviert.

Aktuelle und vorherige gemeinsame Parameter werden getrennt an `PreviousFrameSwitch` übergeben. Die Zeit stammt aus der angehaltenen Welt-/Simulationsuhr, nicht aus einem unabhängig weiterlaufenden Material-Time-Knoten.

## Atemblasen

Eine Ausatmung mit Regler unter Wasser erzeugt bis zu 28 zeitlich versetzte Blasen an zwei seitlichen Austritten nahe dem Mund. Der Audio-Ursprung liegt zwischen diesen Austritten. Anfangsradien von 3–12 mm, eine begrenzte Aufstiegsgeschwindigkeit und seitliches Wackeln sind gestalterische Parameter. Die Bewegung berücksichtigt Strömung und die vorhandene Wellen-Orbitalgeschwindigkeit.

Die CPU berechnet weiterhin die Blasenbewegung. Der Radius folgt `r = r0 * cbrt(p0 / p)` mit dem Druck am tatsächlichen Austritt und am aktuellen Ort. Die leicht abgeflachte, wobbelnde Ellipsoidform erhält dabei ihr Volumen. Die Blase wird entfernt, sobald ihr oberer Rand die lokale Wellenfläche erreicht. Zusätzlich begrenzen 90 Sekunden Alter, 30 m Abstand und höchstens 512 Zustände den visuellen Pool. Bei vollem Pool werden neue visuelle Blasen ausgelassen; bestehende Zustände und deren Draw-Indizes bleiben bis zur nächsten Aktualisierung erhalten.

Das Thin-Translucent-Material hat keine deckende diffuse Schicht. Spiegelung und Bildschirmraum-Brechung ersetzen die früheren gefüllten Scheiben. Das relative Luft/Wasser-Brechungsverhältnis beträgt ungefähr 0,75. Dies ist eine Näherung: gekrümmte Ein-/Austrittsflächen, Totalreflexion, Mehrfachreflexion und Lichttransport durch mehrere überlappende Blasen werden nicht vollständig physikalisch gelöst. Bildschirmränder, Überlagerungen und fehlende Informationen hinter Vordergrundobjekten können sichtbare Artefakte verursachen. Das Verfahren ist keine vollständige volumetrische Blasensimulation.

## Reproduzieren und prüfen

```powershell
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/Build.ps1 -Target Content -OnlyParticles -EditorSandbox
./Tools/TestCore.cmd
./Tools/TestEditorSandbox.ps1
./Tools/Build.ps1 -Target Package -EditorSandbox -BuildSlot Particles -PackageDirectory Builds/WindowsParticles
./Tools/TestParticles.ps1 -PackageDirectory Builds/WindowsParticles
Tools/.geodata-env/Scripts/python.exe Tools/ReportParticles.py artifacts/particles-validation-YYYYMMDD-HHmmss
```

Der Testmodus `-DiveParticleScale=20 -DiveNoBubbles` vergrößert ausschließlich diagnostische Körner und färbt sie emissiv violett. Ein Vergleich mit Skalierung null muss tatsächlich verschieden gerenderte Pixel ergeben. Diese Bilder sind ausdrücklich keine normale Spielansicht. Außerhalb von `-DiveSmokeTest` bleiben solche Optionen wirkungslos.

Die Laufzeitprüfung kontrolliert sämtliche unveränderten Instanztransformationen und Seed-Daten, die konservativen Bounds und die Druck-Volumen-Invariante der tatsächlich gezeichneten Blasenellipsoide. Der Streamingfall muss mehrere Zellen durchqueren. Native Marker allein gelten nicht als Nachweis der sichtbaren GPU-Ausgabe; dafür gibt es zusätzlich den Pixelvergleich.

Der zeitgesteuerte Atemreglertest verwendet seinen ursprünglichen Einstieg Weesen. Ein erster Matrixlauf hatte ihn versehentlich nach Murg verlegt und scheiterte: Die Aufzeichnung zeigt bei 24,02 s Umgebungsluft und einen trockenen Mund, bei 25,02 s nach erneutem Eintauchen dagegen korrekt keine Atemversorgung. Die feste Prüferwartung „Umgebungsluft nach 25 s“ passte dort nicht. Dieser fehlgeschlagene Lauf bleibt unter `artifacts/particles-breathing-entry1-failure` erhalten. Die physiologische Simulation und die Prüferwartungen wurden nicht abgeschwächt; nur der bisherige Testort wurde wiederhergestellt. Eine variable Atemtest-Choreografie für alle Einstiege bleibt eine mögliche Erweiterung.

Beim isolierten Editor-Build wurde außerdem ein reproduzierbarer Fehler in der Quellkopie behoben: Ein während eines früheren Builds bearbeiteter Header konnte beim Kopieren einen älteren Zeitstempel als dessen Objektdateien behalten. Das führte zu gemischten Klassenlayouts. Geänderte Dateien erhalten jetzt beim Synchronisieren einen aktuellen Zeitstempel, unveränderte Dateien bleiben unangetastet. `TestEditorSandbox.ps1` prüft ausdrücklich eine Inhaltsänderung mit absichtlich altem Quellzeitstempel.

Bezug zu den Anforderungen: WATR-008, WATR-012, WATR-013, WATR-014 und GFX-008. Die Windows-Implementierung ersetzt keine native macOS-Grafikprüfung oder fachliche Gesamtfreigabe.

## Echte Paketaufnahmen

Unveränderte 1600 × 900-Aufnahmen aus automatisierten Spielszenarien des Partikelpakets, mit normaler Teilchengröße und ohne Diagnosefarbe. Die feinen Körner können bei verkleinerter Darstellung kaum erkennbar sein; die großen Konturen nahe den Bildrändern sind Atemblasen.

![Nacht mit Wide-Lampe, normalen Körnern und transparenten Atemblasen](Images/Particles_Night.png)

![Tagesansicht mit transparenten Blasenkonturen](Images/Particles_Day.png)
