# Nanite-Kachelgrenze direkt nach der Platzierung

Stand: 28. November 2026. Eine stationäre Gegenprobe grenzt den Startspalt auf die anfänglich verfügbare Nanite-Darstellung ein. Bei vollständig residenten Nanite-Daten der beiden Testkacheln ist die Kante bereits im ersten geprüften Bild geschlossen. **Das ist eine lokale Editorstudie, keine allgemeine Freigabe der Berglandschaft.** Die akzeptierte Unterwasserwelt und das eingefrorene R3-Spielpaket bleiben unverändert.

## Was die neue Probe prüft

Die Figur wird nach demselben zwölfsekündigen Szenenvorlauf einmal an E 2733236 / N 1223622 gesetzt. Ab dieser Platzierung gibt es keinen zusätzlichen Vorlauf und keine Bewegung. Bodenstützung und unveränderte Position werden während der Probe geprüft. Bilder entstehen bei Tick 0, 1, 2, 3, 4, 5, 10, 20, 30 und 60; das Protokoll enthält Tick-Zeit und Engine-Frame. Die Bilderfassung selbst verursacht Wartezeiten. Diese Zeiten sind deshalb **keine Messung der Spaltdauer bei gewöhnlichem Spielen**.

Die erste Nanite-Folge zeigt die deutliche Lücke bei Tick 0–2. Bei Tick 3 bleibt im geprüften Ausschnitt nur ein kleiner Farbresteffekt; bei Tick 4 ist der sichtbare Spalt verschwunden, obwohl die Figur stillsteht. Die konventionelle, zuvor korrigierte Ersatzgeometrie ist bereits in ihrem ersten Bild geschlossen. Das trennt diesen Befund vom bereits behobenen Fallback-Randfehler.

## Gegenprobe mit vollständig verfügbaren Nanite-Daten

Die Diagnose klont ausschliesslich den korrigierten Westnachbarn `Boundary_SM_Land_04_02` und die feine Ostkachel `Boundary_SM_LandDetail_05_02`. `TargetMinimumResidencyInKB = MAX_uint32` macht deren sämtliche Nanite-Seiten beim Laden verfügbar. Es werden weder Quellpositionen noch Kamerastart, Material, Bodenstützung oder Wasserpfad angepasst. Die native Prüfung vergleicht die Positionen und Vertex-/Dreieckszahlen von LOD0 und HiRes mit den Ausgangsassets. Zusätzlich müssen Quellgrenzen übereinstimmen und sämtliche Seiten als Root-Seiten vorliegen. Der gespeicherte Stand wird in einem neuen Prozess erneut geprüft.

| Kachel | Root-Seiten vorher | Root-Seiten in Gegenprobe | RootData vorher | RootData in Gegenprobe |
| --- | ---: | ---: | ---: | ---: |
| West, 04_02 | 1 | 84, alle | 25’780 Bytes | 2’617’304 Bytes |
| Fein, 05_02 | 1 | 1’246, alle | 25’784 Bytes | 36’103’264 Bytes |

`RootData` bezeichnet die CPU-Datenstruktur des Nanite-Assets. Die Summe von rund 36,9 MiB ist **kein gemessener GPU-Speicherbedarf**. Der Editor kann gestreamte Daten über den Derived Data Cache beziehen; `StreamablePages.GetBulkDataSize() == 0` allein würde deshalb keine vollständige Residency belegen. Die Prüfung verlangt zusätzlich die Gleichheit von Root-Seitenzahl und Gesamtseitenzahl.

Erstes Bild mit den gewöhnlichen, nur teilweise residenten Nanite-Daten:

![Startspalt in der gestreamten Nanite-Darstellung](Images/Terrain_Nanite_StreamingStart.png)

Erstes Bild mit vollständig residenten Daten der beiden Kacheln:

![Geschlossene Kante im ersten Bild der Residency-Gegenprobe](Images/Terrain_Nanite_ResidentStart.png)

Die in voller Auflösung gesichteten Schlüsselbilder bei Tick 0, 2 und 60 der Residency-Folge zeigen keinen vergleichbaren Spalt. Die grossen dunklen Flächen, die in der stationären Streaming-Folge nach dem Schliessen der Lücke erscheinen, fehlen ebenfalls in diesen Residency-Bildern. Zusätzlich zählt eine Farbauswertung in allen zehn Aufnahmen je Variante bläuliche Pixel im vorher festgelegten Spaltbereich. Sie ersetzt keine Geometrieprüfung oder vollständige visuelle Sichtung: Einzelne solche Pixel kommen auch im geschlossenen Fallback-Bild vor. **Null Farbpixel sind daher weder allgemeiner Lückenbeweis noch Freigabekriterium.**

Eine erneute Streaming-Kontrolle mit exakt derselben nativen Programmversion wie die Residency-Probe reproduziert den Startspalt. Deren Schlüsselbilder bei Tick 0, 2 und 60 sind ebenfalls in voller Auflösung gesichtet. Die beiden älteren Vorproben bleiben als vorläufige Befunde erhalten; ihr damaliges Programmbinary wurde nicht separat archiviert.

Der Vergleich stützt die Zuordnung zur anfänglich verfügbaren groben Nanite-Darstellung. Er identifiziert noch nicht die exakten verschobenen GPU-Dreiecke und beweist keine fehlerfreien Übergänge an allen 77 Kacheln oder bei allen Blickrichtungen.

## Korrigierter Fehler beim Aufbau der Diagnose

Der erste duplizierte Stand wurde von der bestehenden Prüfung der Kachelgrenzen abgewiesen. Die Quellpositionen waren identisch; `DuplicateObject` hatte jedoch den Cache der Quellmesh-Grenzen nicht übernommen. Dadurch verwendete die Kopie die grösseren Rendergrenzen. Diese grösseren Rendergrenzen waren auch im Ausgangsasset bereits vorhanden. Die Diagnose übernimmt jetzt durch erneutes Committen des unveränderten LOD0 die Quellgrenzen wie beim Ausgangsasset und prüft deren Gleichheit vor dem Speichern und nach erneutem Laden. Der bestehende Grenzwert der Laufzeitprüfung wurde nicht gelockert. Fehlversuch und damalige Assets sind separat lokal archiviert.

## Weiteres Vorgehen und Reproduktion

Zwei neue 60-s-Messungen mit 15 s Vorlauf am Einstieg Murg, umlaufender Blickrichtung, 1920 × 1080, Qualitätsstufe 2, 100 % Renderauflösung und normalem Wasserpfad verwenden die gleiche finale Programmversion. Beide Abschlussbilder sind gesichtet:

| Nanite-Daten der beiden Kacheln | FPS Mittel | 1-%-Low | Langsamster Frame | Max. Prozessspeicher im Messfenster |
| --- | ---: | ---: | ---: | ---: |
| Streaming | 76,81 | 60,69 | 33,34 ms | 10’576,8 MiB |
| Vollständig resident | 76,50 | 51,09 | 99,52 ms | 10’207,8 MiB |

Der langsamste Frame des Residency-Laufs liegt bei ungefähr Simulationssekunde 60,6; im benachbarten CPU-Sample entfallen 89,63 ms auf Garbage Collection. Auch die Kontrolle hat dort einen kleineren Aussetzer. Das bestätigt, dass die bestehende CPU-/GC-Frage offen bleibt. Alle inneren Frames bleiben in der Auswertung. Je ein Durchlauf mit unterschiedlicher Hintergrundlast belegt keinen kausalen Leistungs- oder Speichervorteil. Insbesondere ist der niedrigere Prozessspeicher im zweiten Messfenster kein Nachweis einer Einsparung durch Residency und keine Messung des GPU-Speichers.

Vor einer Übernahme braucht es eine passende Residency-Strategie für das tatsächlich begehbare Gebiet, Messungen des GPU-Speichers und längere bewegte Sichtprüfungen. Die bisherigen grossen Editorassets und deren Speichermehrbedarf sind weiterhin offen. Vollständige Residency sämtlicher Landschaftskacheln wird aus dieser Zweikachelprobe nicht abgeleitet. Materialübergänge, Waldvolumen und Wiesen bleiben Aufgaben der grafischen Weiterentwicklung.

Die Diagnoseassets sind lokal erzeugte, von Git ausgeschlossene Ableitungen. Sie werden mit dem folgenden Befehl aus den versionierten Boundary-Assets reproduziert; der Ordner `LandscapeStudy` bleibt vom normalen Cook ausgeschlossen:

```powershell
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Editor -MaxParallelActions 4
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target LandscapeFallback -ResidentStudy
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target LandscapeFallbackCheck -ResidentStudy
./Tools/SmokeTest.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -ProductionWater -Scenario land-detail -Entry 1 -Hour 12 -Date 2026-06-21 -MaxFPS 30 -FarCliffStudy -LandscapeDetailStudy -LandscapeFallbackStudy -LandscapeBoundaryProbe -LandscapeResidentStudy
```

Für die Streaming-Kontrolle `-LandscapeResidentStudy` weglassen. Für den konventionellen Vergleich zusätzlich `-DisableNanite` setzen. Die Bilder jeder Variante unmittelbar getrennt archivieren, da das Screenshot-Verzeichnis wiederverwendet wird. Die normale Gehprobe bleibt ohne `-LandscapeBoundaryProbe` verfügbar.

[Messdaten, lokale Eingabesnapshots und Protokolle](LANDSCAPE_NANITE_BOUNDARY_RESULTS.json).
