# Prüfung des spielbaren Walensee-Inkrements

Stand: 25. November 2026. Windows 11, Unreal Engine 5.8.4, MSVC 14.44, Windows SDK 10.0.22621. Lokaler Entwicklungsrechner; keine allgemeine Hardwarefreigabe.

| Prüfung | Ergebnis |
|---|---|
| Engineunabhängiger C++-Kern | Bestanden: 0 Fehler bei Druck, Gas, BCD, Kopplungen, Geweben, Stopptimer, Sanitierung und Rasterabfragen über Kachelgrenzen |
| Python-Bathymetrieparser | 3 Tests bestanden: Reihenfolge/NoData, unvollständiges Raster, falsche Zeilenbreite |
| Unreal-Editor-Modul | Development-Build erfolgreich |
| Asset-/Map-Erzeugung | Commandlet erfolgreich, letzte Content-Erzeugung ohne Warnungen oder Fehler |
| Eigenständiger Windows-Build | BuildCookRun erfolgreich; Spiel startet ohne Editor |
| Automatisches Gameplay im Package, drei Startorte | Jeweils DIVE_SMOKE_PASS: 22 s und 23 gespeicherte Samples; Weesen 5,67 m maximale Tiefe / 18,77 m Strecke, Murg 11,45 m / 23,10 m, Walenstadt 9,64 m / 21,85 m |
| Geländestreaming im Package | DIVE_SMOKE_PASS: 90 s Exploration bei Murg, 110,92 m horizontale Entfernung vom Start, maximal neun Kacheln über den gesamten Lauf, 91 gespeicherte Samples |
| Auftauchen im aktuellen Landschafts-Package | Alle drei Orte DIVE_SMOKE_PASS: jeweils 38 s, Kamera oberhalb Z=0 erreicht, 39 gespeicherte Samples. Unterwasser-/Oberflächenaufnahmen geprüft; keine Material-Kompilierungsfehler oder Ersatzmaterialien. Lokaler Ausgabeordner: `Builds/WindowsLandscape` |
| Manuelle Bedienung im Package | DE/EN-Umschaltung, Flaschenwahl, Start, Pause, Abbruch, Auswertung und Rückkehr zum Menü geprüft |
| Walensee-Geodaten | Offizielle Archiv-SHA256 stimmt; alle 46 Kacheln im Package geladen. Vergleich sämtlicher Laufzeitwerte mit Float32-Quelle: 23.334.479 Messwerte, 13.808.385 NoData-Werte erhalten, maximal 0,005 m Quantisierungsabweichung |
| Tiefenkarte | Package-Screenshot geprüft: korrekte Nordorientierung, drei Marker, aktuelle Position, Konturen und Quellenangabe |
| Landschaftsdaten | 286 swissALTI3D-Quelldateien der Ausgabe 2026 mit offiziellen SHA256 geprüft; 858 native 4 × 4-Pixelblöcke stimmen mit dem 8-m-Raster bis auf weniger als 0,000062 m numerische Abweichung überein |
| Wassermaterial | Single Layer Water mit Streuung/Absorption/Reflexion erzeugt; Content-Commandlet erfolgreich, 0 Fehler und 0 Warnungen |
| Wiederholte Asset-Erzeugung | Zwei aufeinanderfolgende Content-Builds und anschließender Editor-Spieltest bestanden; keine Material-Kompilierungsfehler oder Ersatzmaterialien |

![Tiefenkarte aus der verpackten Anwendung bei Murg](Images/WalenseeMap.png)

![Oberfläche und swissALTI3D-Landschaft im Package bei Weesen](Images/WalenseeSurface.png)

Reproduktionsbefehle stehen in der README. Laufzeit-Protokolle und lokale Build-Ausgaben sind unter `artifacts/` abgelegt und nicht Bestandteil der Versionsverwaltung. Das automatische Gameplay startet mit fest definierten Einstellungen, speichert ein eigenes Protokoll, prüft dessen Inhalt und verändert keine Benutzereinstellungen.

## Behobene Fehler bei dieser Prüfung

- Laufzeitlicht verwendete zunächst eine nur im Editor vorhandene Hilfsfunktion; jetzt wird die Runtime-Komponente angesprochen.
- Logspeicherung wertete den booleschen Rückgabewert von `IFileManager::Move` falsch aus; erfolgreich geschriebene Dateien wurden als Fehler gemeldet.
- Schrift benötigt in Canvas einen gültigen Font-Asset-Verweis; dieser wird jetzt explizit geladen und mitgekocht.
- `-NoLiveCoding` verursachte beim monolithischen Development-Build einen inkompatiblen `WITH_LIVE_CODING`-Wert gegenüber den vorgebauten Engine-Bibliotheken. Der resultierende Absturz bei der Klassenregistrierung wurde im Dump untersucht; das Build-Skript erhält jetzt die Engine-Vorgabe. Der erneute Package-Start und Gameplay-Test bestehen.
- Die GeoTIFF-Vorschaustufen erzeugten beim Herunterskalieren abweichende Höhen statt der beabsichtigten Flächenmittel. Die Pipeline mittelt jetzt direkt die nativen 2-m-Pixel; `Tools/validate_landscape.py` prüft Quell- und Laufzeitprüfsummen sowie verteilte Pixelblöcke unabhängig nach.
- Unreals `DeleteAllMaterialExpressions` verändert in dieser Engineversion die Sammlung während ihrer Iteration und lässt dadurch Knoten zurück. Die Asset-Pipeline löscht jetzt über eine separate Liste und prüft, dass der Graph leer ist. Das verhindert doppelte Wasser-Ausgabeknoten und daraus folgende Ersatzmaterialien. Der Smoke-Test schlägt bei Material-Kompilierungsfehlern nun ausdrücklich fehl.

## Noch nicht abgenommen

Fische, Pflanzen und synthetische Geräusche sind Prototypen. Fotorealismus, echte Uferzugänge, detaillierte Landschaft mit Landbedeckung, vollständige Lokalisierung, optische Qualitätsabnahme des Oberflächenübergangs, geometrische Wellen, fünf Tierarten/drei Pflanzentypen, Ausrüstungsmodelle, Hardcore-Unterschiede, Logbuchbrowser und weitere MUST-Anforderungen bleiben offen. Der Oberflächentest bestätigt die Bewegung in die Luft; das Wassermaterial dämpft den Grund jetzt, benötigt aber weitere Abstimmung. Das Land ist ein Hintergrundmesh ohne Spielerkollision. Kein macOS-Spieltest, kein kontrollierter Performance-Benchmark und kein zweistündiger Dauertest wurden durchgeführt. Der Bathymetrie-Commit hat die Kern-/Parser-CI auf Windows, macOS und Linux bestanden (Run 45515640763); dies ist keine Prüfung des vollständigen Spiels auf diesen Plattformen.
