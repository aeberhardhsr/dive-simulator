# Feinere Geländeform gegenüber Murg

Stand: 28. November 2026. Eine 2048 × 2048 Meter grosse Bergkachel gegenüber Murg ist als separate **2-Meter-Geometrie** aufgebaut und mit der bisherigen 8-Meter-Geometrie verglichen. In den Bildern werden Felskanten und Rinnen feiner gegliedert. Die gestreckte Luftbildfarbe und harte Materialübergänge bleiben sichtbar. **Dies ist eine lokale Darstellungsstudie, noch keine Übernahme in das normale Spiel.**

## Belegter Detailverlust in der bisherigen Aufbereitung

Der neue Prüflauf verifiziert alle 286 ursprünglichen swissALTI3D-Dateien und vergleicht ihre 2-m-Pixelzentren mit der tatsächlichen Dreiecksinterpolation des 8-m-Ausgangsmeshs. Er verwendet dieselbe Diagonale wie der Meshbau, keine bilineare Ersatzfläche. Drei analytische Tests prüfen Ebene, Diagonale und Randbereich. Die ursprüngliche 8-m-Fläche besteht aus 4 × 4 Flächenmitteln der Quelldaten.

| Bereich | Geprüfte Punkte | Median der absoluten Höhendifferenz | P95 | P99 |
| --- | ---: | ---: | ---: | ---: |
| Land über 421 m LN02 | 65’272’585 | 0,20 m | 1,52 m | 4,49 m |
| Kartierter Fels / lockerer Fels | 3’413’992 | 1,03 m | 7,83 m | 15,80 m |
| Nördliches Seeufer, kartierter Fels | 711’508 | 1,34 m | 10,30 m | 21,56 m |

Das sind **vertikale Unterschiede zur Datenquelle**. An sehr steilen Flächen bedeuten sie weder denselben senkrechten Abstand zur Oberfläche noch einen entsprechend grossen sichtbaren Bildfehler. Die 2-m-Quelle ist ebenfalls ein Höhenfeld und enthält keine vermessenen Überhänge. Der nördliche Teilbereich ist ein ausdrücklich definiertes geografisches Fenster, keine Sichtbarkeitsprüfung. [Vollständiger Datenvergleich mit Grenzen und Quellen](LANDSCAPE_RESOLUTION_AUDIT.json).

## Vergleich im Spielrenderer

Murg/West, 21. Juni um 12:00 Uhr, 1600 × 900, jeweils zwölf Sekunden Vorlauf, normaler Hardware-/VSM-Wasserpfad. Die Kamera und das Material bleiben innerhalb eines Paares gleich.

Kartiertes Felsmaterial, bisherige 8-m-Geometrie:

![Kartierter Fels mit 8-m-Geometrie](Images/Terrain_Murg_Mapped8m.png)

Dasselbe Material mit der einzelnen feineren Bergkachel:

![Kartierter Fels mit lokaler 2-m-Geometrie](Images/Terrain_Murg_Mapped2m.png)

Die feineren Kanten und Rinnen innerhalb der Kachel zeigen einen sichtbaren Beitrag der Geländeauflösung. Andere Teile der Berglandschaft bleiben beim bisherigen Stand; ein durchgehend neues Panorama wird nicht behauptet.

Die zweite Gegenprobe hält das bisherige Luftbildmaterial fest:

![Luftbild mit 8-m-Geometrie](Images/Terrain_Murg_Photo8m.png)

![Luftbild mit lokaler 2-m-Geometrie](Images/Terrain_Murg_Photo2m.png)

Hier bleibt die senkrechte Farbstreckung trotz feinerer Geometrie deutlich bestehen. Materialprojektion und Geländeform müssen daher gemeinsam weiterbearbeitet werden. Die [kartierte Materialstudie](LANDSCAPE_CLIFF_COVERAGE.md) hat zusätzlich offene Helligkeitsgrenzen; die [visuelle Referenz](VISUAL_TARGET.md) ist nicht erreicht.

## Umsetzung und Prüfungen

Die separate Kachel `SM_LandDetail_05_02` liegt vollständig im Berggebiet, mindestens rund 157 Meter über dem Szenariowasserspiegel. Sie verwendet 1’050’625 Vertices und 2’097’152 Dreiecke als Nanite-Quelle. Die vorhandenen 2-m-Höhen sind bilinear auf das an die bisherige Kachel ausgerichtete 2-m-Vertexraster abgebildet. Innerhalb eines 32-m-Randes gehen Höhen und Normalen zur bisherigen Kante über. Es wurde kein erfundenes Felsrauschen auf die Geländeform addiert.

Der native Meshbau und eine zweite Prüfung nach erneutem Laden des gespeicherten Assets bestehen. Sämtliche Quellvertices, UVs und Normalen sind geprüft. Die grösste Randabweichung beträgt **0,013916 cm**, die grösste Abweichung zur vorbereiteten Höhe **0,007812 cm**. Diese Werte betreffen Quellgeometrie und gespeicherte Attribute, nicht eine lückenlose Prüfung aller während Bewegung gewählten Nanite-Stufen.

Vier native Renderläufe bestehen und alle vier Bilder sind gesichtet. Bei aktiver Studie wird genau eine von weiterhin 77 Landschaftskacheln ersetzt. Die Materialauswahl, der Wasserpfad sowie die bestehenden Uferanschlüsse und Uferkollisionen sind bestätigt. Alle **77 bisherigen Landschaftsassets, 44 Materialien und 53 Laufzeitdateien des eingefrorenen R3-Pakets** sind bytegleich erhalten. [Prüfbericht mit archivierten Eingaben, Bildern und Protokollen](LANDSCAPE_DETAIL_STUDY_RESULTS.json).

Der erste Bau erforderte eine erneute Erfassung der Quelldateien durch UnrealBuildTool; dafür unterstützt `Build.ps1` jetzt `-RefreshSources`. Ein Namenskonflikt und ein anschliessender Include-Fehler wurden korrigiert. Beim ersten Meshlauf war die generische SHA-256-Plattformfunktion dieser Windows-Engine nicht implementiert. SHA-256 bleibt deshalb die Offline-Identitätsprüfung; die native Laufzeit verwendet zusätzlich Core-SHA-1-Prüfungen gegen veraltete oder beschädigte Eingaben. Die fehlgeschlagenen Versuche bleiben im Prüfbericht erhalten; sie zählen nicht zu den bestandenen Läufen.

## Grenzen vor einer spielbaren Übernahme

Die physische Geländeabfrage bleibt bisher bei 8 Metern. **Die Darstellungsstudie darf deshalb nicht allein als neue begehbare Geländegrundlage übernommen werden.** Zuerst muss die physische Höhenabfrage mit derselben feineren Oberfläche verbunden und geprüft werden. Die bestehende Uferkollision ist davon getrennt und unverändert.

Eine neue Leistungsmessung der feineren Kachel, bewegte Sichtprüfung, weitere Sonnenstände und Paketvalidierung fehlen noch. Die zuvor beobachteten CPU-/Speicherbereinigungs-Aussetzer sind weiterhin offen; die alten FPS-Werte gelten nicht als Messung dieser neuen Geometrie. Waldvolumen, Wiesen, Nahbereich und Materialübergänge bleiben eigenständige Aufgaben.

Nächste Schritte: Fels-/Vegetationsübergänge anhand der klareren Geometrie verbessern, die physische Geländeabfrage passend ergänzen und die CPU-Aussetzer eingrenzen. Erst nach Bewegungs- und Leistungstests soll die feinere Darstellung auf weitere passende Bergflächen erweitert und ins normale Spiel übernommen werden. Die fünf Unterwasser-Abschlussrunden bleiben abgeschlossen.

## Reproduktion

```powershell
./Tools/.geodata-env/Scripts/python.exe Tools/prepare_landscape_detail.py
./Tools/.geodata-env/Scripts/python.exe -m unittest discover -s Tests -p test_landscape_resolution.py
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Editor -MaxParallelActions 4 -RefreshSources
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target LandscapeDetail
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target LandscapeDetailCheck
./Tools/SmokeTest.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -ProductionWater -Scenario shore-preview -Entry 1 -Hour 12 -Date 2026-06-21 -MaxFPS 30 -ShorePreviewSeconds 12 -FarCliffStudy -LandscapeDetailStudy
```

Für die Geometriekontrolle `-LandscapeDetailStudy` weglassen; für das Luftbildpaar zusätzlich in beiden Läufen `-FarCliffStudy` weglassen. Die Quelldateien stammen aus dem vorhandenen verifizierten Geodatensatz. Die Aufbereitung verändert weder `landscape.f32` noch die alten Landschaftsmeshes.
