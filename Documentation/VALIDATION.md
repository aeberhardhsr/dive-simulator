# Prüfung des ersten spielbaren Inkrements

Stand: 25. November 2026. Windows 11, Unreal Engine 5.8.4, MSVC 14.44, Windows SDK 10.0.22621. Lokaler Entwicklungsrechner; keine allgemeine Hardwarefreigabe.

| Prüfung | Ergebnis |
|---|---|
| Engineunabhängiger C++-Simulationskern | Bestanden: 0 Fehler bei Druck, Gas, BCD, Kopplungen, Geweben, Stopptimer und Sanitierung |
| Python-Bathymetrieparser | 3 Tests bestanden: Reihenfolge/NoData, unvollständiges Raster, falsche Zeilenbreite |
| Unreal-Editor-Modul | Development-Build erfolgreich |
| Asset-/Map-Erzeugung | Commandlet erfolgreich, letzte Content-Erzeugung ohne Warnungen oder Fehler |
| Eigenständiger Windows-Build | BuildCookRun erfolgreich; Spiel startet ohne Editor |
| Automatisches Gameplay im Package | DIVE_SMOKE_PASS: 22 s Simulation, maximale Tiefe 6,67 m, Strecke 18,55 m, Restdruck 197,90 bar, 23 gespeicherte Samples |
| Manuelle Bedienung im Package | DE/EN-Umschaltung, Flaschenwahl, Start, Pause, Abbruch, Auswertung und Rückkehr zum Menü geprüft |
| Walensee-Geodaten | Offizielle Archiv-SHA256 stimmt; 46 Rasterkacheln aufbereitet, NoData bleibt erhalten |

![Aufnahme aus der verpackten Anwendung](Images/FirstDive.png)

Reproduktionsbefehle stehen in der README. Laufzeit-Protokolle und lokale Build-Ausgaben sind unter `artifacts/` abgelegt und nicht Bestandteil der Versionsverwaltung. Das automatische Gameplay startet mit fest definierten Einstellungen, speichert ein eigenes Protokoll, prüft dessen Inhalt und verändert keine Benutzereinstellungen.

## Behobene Fehler bei dieser Prüfung

- Laufzeitlicht verwendete zunächst eine nur im Editor vorhandene Hilfsfunktion; jetzt wird die Runtime-Komponente angesprochen.
- Logspeicherung wertete den booleschen Rückgabewert von `IFileManager::Move` falsch aus; erfolgreich geschriebene Dateien wurden als Fehler gemeldet.
- Schrift benötigt in Canvas einen gültigen Font-Asset-Verweis; dieser wird jetzt explizit geladen und mitgekocht.
- `-NoLiveCoding` verursachte beim monolithischen Development-Build einen inkompatiblen `WITH_LIVE_CODING`-Wert gegenüber den vorgebauten Engine-Bibliotheken. Der resultierende Absturz bei der Klassenregistrierung wurde im Dump untersucht; das Build-Skript erhält jetzt die Engine-Vorgabe. Der erneute Package-Start und Gameplay-Test bestehen.

## Noch nicht abgenommen

Die echte Bathymetrie ist noch nicht in die Spielwelt integriert. Die Trainingsszene, Fische, Pflanzen und synthetischen Geräusche sind Prototypen. Fotorealismus, vollständige Lokalisierung, Oberflächenübergang, fünf Tierarten/drei Pflanzentypen, Ausrüstungsmodelle, Hardcore-Unterschiede, Logbuchbrowser und weitere MUST-Anforderungen bleiben offen. Kein macOS-Spieltest, kein kontrollierter Performance-Benchmark und kein zweistündiger Dauertest wurden durchgeführt. Die CI-Konfiguration allein ist kein Nachweis für einen erfolgreichen CI-Lauf.
