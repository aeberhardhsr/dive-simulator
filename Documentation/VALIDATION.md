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
| Auftauchen im Package | DIVE_SMOKE_PASS: 38 s, Kamera oberhalb Z=0 erreicht, 39 gespeicherte Samples; Screenshot geprüft, Wasseroptik noch offen |
| Manuelle Bedienung im Package | DE/EN-Umschaltung, Flaschenwahl, Start, Pause, Abbruch, Auswertung und Rückkehr zum Menü geprüft |
| Walensee-Geodaten | Offizielle Archiv-SHA256 stimmt; alle 46 Kacheln im Package geladen. Vergleich sämtlicher Laufzeitwerte mit Float32-Quelle: 23.334.479 Messwerte, 13.808.385 NoData-Werte erhalten, maximal 0,005 m Quantisierungsabweichung |
| Tiefenkarte | Package-Screenshot geprüft: korrekte Nordorientierung, drei Marker, aktuelle Position, Konturen und Quellenangabe |

![Tiefenkarte aus der verpackten Anwendung bei Murg](Images/WalenseeMap.png)

Reproduktionsbefehle stehen in der README. Laufzeit-Protokolle und lokale Build-Ausgaben sind unter `artifacts/` abgelegt und nicht Bestandteil der Versionsverwaltung. Das automatische Gameplay startet mit fest definierten Einstellungen, speichert ein eigenes Protokoll, prüft dessen Inhalt und verändert keine Benutzereinstellungen.

## Behobene Fehler bei dieser Prüfung

- Laufzeitlicht verwendete zunächst eine nur im Editor vorhandene Hilfsfunktion; jetzt wird die Runtime-Komponente angesprochen.
- Logspeicherung wertete den booleschen Rückgabewert von `IFileManager::Move` falsch aus; erfolgreich geschriebene Dateien wurden als Fehler gemeldet.
- Schrift benötigt in Canvas einen gültigen Font-Asset-Verweis; dieser wird jetzt explizit geladen und mitgekocht.
- `-NoLiveCoding` verursachte beim monolithischen Development-Build einen inkompatiblen `WITH_LIVE_CODING`-Wert gegenüber den vorgebauten Engine-Bibliotheken. Der resultierende Absturz bei der Klassenregistrierung wurde im Dump untersucht; das Build-Skript erhält jetzt die Engine-Vorgabe. Der erneute Package-Start und Gameplay-Test bestehen.

## Noch nicht abgenommen

Fische, Pflanzen und synthetische Geräusche sind Prototypen. Fotorealismus, echte Uferzugänge, Landschaft über Wasser, vollständige Lokalisierung, optische Qualität des Oberflächenübergangs, fünf Tierarten/drei Pflanzentypen, Ausrüstungsmodelle, Hardcore-Unterschiede, Logbuchbrowser und weitere MUST-Anforderungen bleiben offen. Der Oberflächentest bestätigt die Bewegung in die Luft; die aktuelle Ansicht über Wasser zeigt noch zu klar den Grund. Kein macOS-Spieltest, kein kontrollierter Performance-Benchmark und kein zweistündiger Dauertest wurden durchgeführt. Der erste Commit hat die Kern-/Parser-CI auf Windows, macOS und Linux bestanden (Run 45513013970); dies ist keine Prüfung des vollständigen Spiels auf diesen Plattformen.
