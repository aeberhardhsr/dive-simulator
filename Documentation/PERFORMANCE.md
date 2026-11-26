# Gerenderte Leistungsmessung

Die Messung verwendet den eigenständigen Windows-Development-Build mit der tatsächlich gerenderten Spielwelt. Der Unreal-CSV-Profiler erfasst Bildzeiten sowie Game-, Render-, RHI- und GPU-Zeiten. Sie ersetzt weder die visuelle Abnahme noch einen zweistündigen Dauertest.

## Reproduzieren

```powershell
./Tools/Build.ps1 -Target Editor
./Tools/Build.ps1 -Target Package -PackageDirectory Builds/WindowsPerformance
./Tools/Benchmark.ps1 -Scenario underwater -Resolution 1080p
./Tools/Benchmark.ps1 -Scenario underwater -Resolution 1440p
./Tools/Benchmark.ps1 -Scenario underwater -Resolution 2160p
./Tools/Benchmark.ps1 -Scenario surface -Resolution 2160p
./Tools/Benchmark.ps1 -Scenario shore -Resolution 1080p
./Tools/Benchmark.ps1 -Scenario stream -Resolution 1080p
```

Standard: 15 Sekunden Aufwärmzeit, anschließend 60 Sekunden Messung, Qualitätsstufe 2 (High), 100 % Renderauflösung. `-Quality`, `-RenderScale`, `-Seconds`, `-Entry` und `-PackageDirectory` sind einstellbar. `-Editor` verwendet den Editor-Spielmodus; dessen Ergebnisse sind getrennt zu bewerten. Die Aufwärmzeit umfasst keinen kalten Start und keine vollständige Shader-/PSO-Vorwärmung für alle möglichen Blickrichtungen.

VSync, dynamische Auflösung und Bildratenbegrenzung sind für die Messung deaktiviert. Die sekundäre Renderauflösung beträgt ausdrücklich 100 %, damit Windows-DPI-Skalierung die angeforderte Auflösung nicht unbemerkt reduziert. Das Skript prüft tatsächliche Viewportabmessungen, beide Auflösungsskalen und alle neun Qualitätsgruppen. Diese temporären Benchmark-Einstellungen werden nicht als persönliche Grafikoptionen gespeichert.

Alle Szenarien verwenden Exploration, identische Standardumwelt und Einstieg 0 (Weesen), sofern nicht anders angegeben. Unterwasser sinkt der Taucher zunächst auf etwa 5,3 m und dreht danach die Kamera um 360° pro Minute. An der Oberfläche wird der Aufstiegsimpuls gehalten; auch dort und am Ufer dreht die Kamera. `stream` schwimmt mit kräftigem Flossenschlag geradeaus und prüft die zurückgelegte Strecke. Terrain bleibt auf höchstens neun lokale Kacheln begrenzt. Diese vier Abläufe decken weder alle Einstiege noch extreme Wetter-/Tiefenbedingungen ab.

Der instrumentierte Development-Build führt zusätzlich die vorhandenen Laufzeitprüfungen für Tiere, Ausrüstung, Wasser und Audio aus. Speicherabfragen erfolgen nur einmal pro Sekunde. Profiler und Prüfungen verursachen zusätzlichen Aufwand; gemessen wird kein optimierter Shipping-Build.

## Auswertung und Belege

Jeder Lauf erzeugt einen eigenen Ordner `artifacts/perf-…` mit Runtime.log, Host.json, Metadata.json, Capture.csv, Summary.json und Frame.png. Host.json erfasst die vorhandenen Spiel-/Editorprozesse sowie verfügbare NVIDIA-Treiberdaten und die GPU-Auslastung vor dem Start. Das Bild entsteht erst nach Ende der Messung. Der Profiler wartet auf den tatsächlich abgeschlossenen CSV-Schreibvorgang.

`Tools/analyze_performance.py` verlangt den vollständigen CSV-Abschluss, plausible Messdauer, GPU-Zeiten, Kachelgrenzen und szenariospezifische Bewegungs-/Tiefenbedingungen. Genau der erste und letzte Datenframe werden als Messgrenzen entfernt. Sämtliche Ruckler innerhalb des Messfensters bleiben erhalten. Perzentile verwenden den nächsten Rang; durchschnittliche FPS sind Bildanzahl geteilt durch Gesamtzeit. „1 % Low“ ist der Kehrwert der mittleren Dauer des langsamsten Prozents der Bilder, nicht der Kehrwert von p99. GPU-Passzeiten können sich überlappen und dürfen nicht addiert werden.

Ein erfolgreich erstellter Bericht bedeutet **keine bestandene Leistungsabnahme**. Das getrennte Feld `all_frames_within_30fps_budget` ist nur wahr, wenn jedes berücksichtigte Bild höchstens 33,333 ms benötigt. Der Bericht enthält außerdem alle Überschreitungen von 16,667, 33,333 und 100 ms. Die Tests prüfen unter anderem, dass ein einzelner 500-ms-Ruckler erhalten bleibt und eine unvollständige CSV-Datei abgewiesen wird.

## Messstand vom 25. November 2026

Hardware: NVIDIA RTX 3090 (24 GB, Treiber 617.14), AMD Ryzen 9 9950X3D, 32 GB RAM; Windows, DirectX 12, Unreal 5.8.4. Die alte Testinstanz wurde vor dieser Serie mit Zustimmung geschlossen. Der vorhandene Unreal-Editor blieb geöffnet; seine Last und andere Desktopaktivitäten können Messungen beeinflussen. Es handelt sich um einzelne Durchläufe auf einem leistungsfähigen Entwicklungsrechner, keine Mindesthardware-Freigabe.

Alle folgenden Läufe verwenden native Renderauflösung und Qualitätsstufe 2. Zeiten sind Millisekunden. Die kompakten Ergebnisse einschließlich Hardware-/Zeitkontext stehen zusätzlich in `PERFORMANCE_RESULTS.json`; vollständige Rohdaten liegen lokal in den darin genannten Artefaktordnern.

| Szene | Auflösung | Ø FPS | 1 % Low FPS | p95 | p99 | Maximum | Bilder >33,33 ms |
|---|---|---:|---:|---:|---:|---:|---:|
| Unterwasser | 1920×1080 | 128,82 | 92,71 | 8,99 | 9,93 | 40,46 | 1 / 7727 |
| Unterwasser, Wiederholung | 1920×1080 | 128,70 | 96,71 | 8,93 | 9,92 | 11,33 | 0 / 7720 |
| Unterwasser | 2560×1440 | 98,44 | 75,61 | 11,40 | 12,83 | 14,53 | 0 / 5904 |
| Unterwasser | 3840×2160 | 59,91 | 48,62 | 18,52 | 20,14 | 22,01 | 0 / 3593 |
| Oberfläche | 1920×1080 | 127,32 | 94,88 | 9,04 | 10,03 | 11,61 | 0 / 7638 |
| Oberfläche | 3840×2160 | 59,60 | 47,72 | 18,69 | 20,33 | 22,11 | 0 / 3574 |
| Ufer | 1920×1080 | 139,01 | 102,95 | 8,31 | 9,24 | 11,11 | 0 / 8339 |
| Nachladen | 1920×1080 | 128,04 | 94,87 | 9,01 | 9,97 | 14,21 | 0 / 7680 |

Der Nachladelauf legt während des Messfensters 94,59 m zurück. Die Kachelzahl fällt bei den Wechseln für vier Frames auf sieben beziehungsweise acht und erreicht jeweils wieder neun; der maximale gemessene Prozessspeicher beträgt 2406,88 MiB. Das ist ein kurzer Streamingnachweis, kein Langzeitnachweis gegen Speicherlecks.

Der einzelne 40,46-ms-Frame im ersten Unterwasserlauf liegt bei Simulationssekunde 34,16. Dazu sind 42,53 ms Renderthread, 1,88 ms Gamethread und 6,82 ms GPU-Zeit aufgezeichnet. Die Kachelzahl bleibt neun. Diese Zähler lokalisieren keine eindeutige Ursache; ein externer Stall oder eine Synchronisationswartezeit ist damit weder bewiesen noch ausgeschlossen. Der Frame bleibt vollständig in der Auswertung.

Der gezielte zweite 60-Sekunden-Lauf desselben Ablaufs reproduziert diesen Ausreißer nicht: maximal 11,33 ms bei 128,70 FPS im Mittel. Er ersetzt den ersten Lauf nicht und beweist keine dauerhafte Ruckelfreiheit. Die JSON-Ergebnisdatei enthält beide Läufe sowie SHA256-Prüfsummen von Rohdaten, Metadaten, Bildern, Laufzeitlogs und dem gemessenen Executable.

Bei 4K liegen bereits die mittleren GPU-Zeiten nahe am 60-FPS-Budget. Insbesondere temporale Kantenglättung/Rekonstruktion, Reflexionen und Schatten kosten Zeit. Die aufgezeichneten GPU-Pässe rechtfertigen keine Behauptung, die Spielphysik sei der Engpass. Es wurde für diese Serie keine Grafikqualität abgesenkt oder Optimierung als bereits erreicht ausgegeben.

Die Unterwasser-, Oberflächen-, Ufer- und Nachladeaufnahmen wurden visuell geprüft. Die Szene ist gerendert, die Instrumente sind sichtbar; die bekannten flachen Orthobilddetails und vereinfachten Gelände-/Objektoberflächen bleiben sichtbar. Hohe Bildraten sind keine Fotorealismusabnahme.

Eine universelle 60-FPS-Zusage, die zweistündige Stabilitätsprüfung und die tatsächliche macOS-Leistungsmessung bleiben offen. 4K erreicht in diesen beiden Läufen ungefähr 60 FPS im Mittel, aber keine durchgehend eingehaltenen 16,667 ms. Auch das strengere Kriterium „jedes Bild unter 33,333 ms“ besteht wegen des einzelnen Ausreißers nicht in allen Läufen.

## Folgestand mit gescannten PBR-Oberflächen

Nach Erneuerung von Seegrund, Fels und untergetauchtem Ufer wurden vier weitere Messungen mit derselben Methodik durchgeführt. Der Build `Builds/WindowsBed` erreicht 123,34 FPS unter Wasser in 1080p, 56,71 FPS in 4K, 122,34 FPS beim Nachladen und 132,85 FPS am Ufer. Alle 26.107 berücksichtigten Bilder dieser Serie bleiben unter 33,333 ms. Die höheren Materialkosten reduzieren die mittlere Bildrate gegenüber den jeweiligen Vorgängermessungen um ungefähr 4–5 %. Die historische Tabelle oben bleibt als Vergleich erhalten. Vollständige neue Werte und Package-Prüfsummen: [BED_PERFORMANCE.json](BED_PERFORMANCE.json); Ablauf-/Lichtprüfung und Grenzen: [VALIDATION.md](VALIDATION.md).

## Folgestand mit Hintergrundspeicherung

Das Speichern eines langen synthetischen Protokolls verursachte im Spielthread mehrere hundert Millisekunden Arbeit. Zwischenstände werden deshalb jetzt als eigene Datenkopien im Hintergrund verarbeitet; beim Abschluss wird die Speicherung weiterhin abgewartet. Die Kostenmessung mit 7.201 Messpunkten und die Wiederherstellungsprüfungen stehen in [VALIDATION.md](VALIDATION.md). Zwei ergänzende native 1080p-Läufe erreichen 117,83 beziehungsweise 120,45 FPS; alle 14.293 Bilder bleiben unter 33,333 ms. Der erste Lauf enthält eine Phase höherer GPU-Zeiten und bleibt vollständig dokumentiert. Ergebnisse: [BACKGROUND_LOG_RESULTS.json](BACKGROUND_LOG_RESULTS.json). Diese kurzen Renderläufe ersetzen keine zweistündige Stabilitätsprüfung mit wachsendem Protokoll.


## Zweistündiger Lauf unter paralleler Nutzung

Der am 25. November 2026 geprüfte Build `b4b6fba` erfüllt in Weesen/Exploration zwei echte Stunden ohne Absturz, mit 240 erfolgreichen Zwischensicherungen und einem erneut lesbaren vollständigen Tauchprotokoll. Das ist ein Stabilitätsnachweis für den getesteten älteren Build. Mittelwert 76,13 FPS, P95 ≤24,75 ms, P99 ≤27,50 ms und Maximum 267,12 ms ergeben keine durchgehende 60-FPS-Freigabe. 147 von 240 Berichtsintervallen liegen unter 60 FPS; sämtliche Bildzeiten bleiben enthalten. Editor, Entwicklungsaufgaben und später eine zusätzliche Benutzer-Spielinstanz liefen parallel. Methodik, vollständige Zahlen, Diagramm und Grenzen stehen in [SOAK.md](SOAK.md) und [SOAK_2H_RESULTS.json](SOAK_2H_RESULTS.json). Die älteren Kurzläufe oben und dieser belastete Langzeitlauf werden nicht zu einem vermeintlich ungestörten Benchmark zusammengeführt.

## Folgestand mit Körper- und Ausrüstungskollision

Der Build `Builds/WindowsContacts` erreicht in zwei getrennten 60-Sekunden-Läufen nach je 15 Sekunden Aufwärmzeit bei 1920×1080, Qualitätsstufe 2 und 100 Prozent Renderauflösung auf RTX 3090 / Ryzen 9 9950X3D unter Wasser 86,96 FPS im Mittel / 71,19 FPS im langsamsten Prozent und stationär am Ufer 98,09 / 82,43 FPS. Alle 11.099 gemessenen Bilder bleiben unter 16,67 ms; Maxima sind 15,0835 beziehungsweise 14,3866 ms. Keine weitere Unreal-Spiel- oder Editorinstanz war beim jeweiligen Start aktiv. Die beiden Bilder wurden visuell geprüft. Der Uferlauf bleibt stationär und ersetzt keinen Geh-/Streamingtest; diese kurzen Einzelmessungen ersetzen weder Mindesthardwareprüfung noch eine Langzeitfreigabe. Vollständige Einstellungen und Prüfsummen: [Unterwasser](CONTACTS_PERFORMANCE.json), [Ufer](CONTACTS_SHORE_PERFORMANCE.json). Umsetzung und weitere Prüfungen: [CONTACTS.md](CONTACTS.md).


## WindowsSediment: aktive Wolken und Unterwasservergleich

Zwei getrennte 60-Sekunden-Fenster nach je 15 Sekunden Aufwärmzeit, RTX 3090 / Ryzen 9 9950X3D, 1920×1080, Qualität 2, 100 Prozent Renderauflösung: Sedimentszenario 87.34 FPS im Mittel / 67.03 FPS im langsamsten Prozent, gewöhnlicher Unterwasserlauf 86.45 / 69.90 FPS. Im Sedimentlauf enthalten 5238 von 5238 gemessenen Bildern aktive Wolken (Minimum 6, Maximum 24, Mittel 19.51). Die Zählung erfasst simulierte Wolken, keine garantierte Bildabdeckung oder maximale Transparenzlast. Auch der gewöhnliche Unterwasserlauf erzeugt bis zu zwei Wolken. Die Szenarien fahren unterschiedliche Wege; ihre Differenz ist kein isolierter Effektkostenvergleich. Ein vom Nutzer geöffneter Unreal-Editor lief parallel; die Messungen sind daher keine isolierten Vergleiche.

Vollständige Zeiten, langsame Bilder, Einstellungen und Artefakt-Prüfsummen: [Sediment](SEDIMENT_PERFORMANCE.json), [Unterwasser](SEDIMENT_UNDERWATER_PERFORMANCE.json). Die kurzen Messungen belegen weder Mindesthardware noch native macOS-Leistung oder eine Langzeitfreigabe.

## WindowsHabitat: gruppierte Felsen und dichterer Bewuchs

Zwei getrennte Messungen des Pakets aus Laufzeitstand `c14eaa5`, RTX 3090 / Ryzen 9 9950X3D, 1920×1080, Qualität 2, 100 Prozent Renderauflösung, ohne Bildratenbegrenzung und nach jeweils 15 Sekunden Aufwärmzeit. Beim Start war keine weitere Unreal-Spiel- oder Editorinstanz aktiv. Alle inneren Messbilder bleiben in der Auswertung; nur je ein Randbild wird verworfen.

| Szenario | Messdauer | Mittel | 1%-Low | P95 Bildzeit | Höchste Bildzeit | Bilder >33,33 ms |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Unterwasseransicht Weesen | 60 s | 68,18 FPS | 54,48 FPS | 17,25 ms | 19,70 ms | 0 / 4.089 |
| Schwimmen/Nachladen Murg | 90 s | 100,53 FPS | 77,34 FPS | 10,84 ms | 68,83 ms | 2 / 9.046 |

Der Murg-Lauf legt im Messfenster 144,86 Meter zurück und endet im offenen Wasser bei rund 24,7 Metern Tiefe. Er ist deshalb kein Vergleich derselben sichtbaren Objektdichte mit der bodennahen Weesen-Aufnahme. Der Unterwasserlauf erreicht keine durchgehend eingehaltenen 60 FPS; der Streckenlauf enthält zwei kurze Ausreißer. Diese Einschränkungen bleiben Teil des Ergebnisses.

In der Weesen-Aufnahme liegt die mittlere GPU-Zeit bei 14,05 ms, der Spielthread bei 5,87 ms. `ShadowDepths` ist mit 4,35 ms der größte aufgezeichnete GPU-Pass. Im Murg-Lauf dauern reguläre Aufbauten neuer Populationszellen laut Protokoll 34–51 ms; der Spielthread erreicht maximal 68,71 ms. Die Messungen liefern damit Ansatzpunkte für Schatten- und Nachladeoptimierung, ohne deren genaue Ursachen bereits durch einen isolierten Abschaltvergleich zu beweisen.

EXE-/Paketidentität, vollständige Einstellungen, CSV-Dateien, Bilder und Prüfsummen stehen im [Habitat-Prüfbericht](HABITAT_RESULTS.json). Die kurzen Läufe ersetzen weder einen Langzeittest dieses Pakets noch eine Mindesthardware- oder visuelle Qualitätsabnahme.

## Nachladen der Habitat-Objekte: Cache und verteilte Erstellung

Zwei weitere Pakete erhalten dieselbe Felsgeometrie und Pflanzendichte. `WindowsPopulationCache` berechnet identische Felslayouts nur einmal je benötigter Zelle; `WindowsPopulationStaged` verteilt die Erstellung entfernter Komponenten auf mehrere Bilder. Nahe Geometrie und Hindernisdaten bleiben vollständig. Alle drei Messungen verwenden dieselbe 90-Sekunden-Murg-Strecke (144,86 m), RTX 3090 / Ryzen 9 9950X3D, 1080p, Qualität 2, 100 Prozent Renderauflösung, 15 Sekunden Aufwärmzeit, keine weitere Unreal-Instanz und keine Bildratenbegrenzung.

| Paket | Mittel | 1%-Low | P95 Bildzeit | Höchste Bildzeit | Bilder >33,33 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| WindowsHabitat | 100,53 FPS | 77,34 FPS | 10,84 ms | 68,83 ms | 2 / 9.046 |
| WindowsPopulationCache | 100,77 FPS | 80,80 FPS | 10,82 ms | 41,25 ms | 2 / 9.067 |
| WindowsPopulationStaged | 100,93 FPS | 80,21 FPS | 10,79 ms | 37,12 ms | 1 / 9.082 |

Die letzten beiden Übergänge verteilen sich jetzt jeweils auf drei Bilder. Die protokollierte Populationsarbeit dauert einzeln 2,73–11,46 ms; zuvor waren es 14,79–20,80 ms je vollständiger Reihe im Cache-Paket. Der Spielthread erreicht weiterhin 36,72 ms, die vollständige Bildzeit 37,12 ms. Das langsame Einzelbild bleibt erhalten und verhindert eine Freigabe als ruckelfrei. Die mittlere Bildrate ändert sich kaum; die kleinen Unterschiede der Perzentile sind keine garantierten Verbesserungen über mehrere Läufe.

Die drei Paketprüfungen mit neun Bildern und der Root-Launcher bestehen. Das Messbild wurde geprüft und zeigt wie zuvor offenes Wasser bei 24,7 m Tiefe. Die Ergebnisse ersetzen keine Messung dichter sichtbarer Bodenvegetation, keinen Langzeittest und keine visuelle Abnahme. Belege: [Cache-Vergleich](POPULATION_CACHE_RESULTS.json), [verteilte Erstellung](POPULATION_STAGED_RESULTS.json), [Implementierung und Prüfungen](POPULATION_STREAMING.md).

## WindowsBenthic: Oberflächenbeläge und präzisere Geländegrenzen

Ein 60-Sekunden-Unterwasserlauf bei Weesen aus Laufzeitstand `4f6e4e4`, nach 15 Sekunden Aufwärmzeit, RTX 3090 / Ryzen 9 9950X3D, 1920×1080, Qualität 2, 100 Prozent Renderauflösung, ohne Bildratenbegrenzung und ohne weitere Unreal-Instanz: **68,34 FPS im Mittel / 55,21 FPS im langsamsten Prozent**. P95 beträgt 17,07 ms, die höchste Bildzeit 21,36 ms. 395 von 4.098 Bildern überschreiten 16,67 ms; keines überschreitet 33,33 ms. Alle inneren Messbilder bleiben erhalten, nur je ein Randbild wird wie bisher entfernt.

Die GPU benötigt im Mittel 14,01 ms, der Spielthread 5,80 ms. Größter erfasster GPU-Pass bleibt `ShadowDepths` mit 4,29 ms. Das Bild wurde geprüft und zeigt den bodennahen Weesen-Bereich bei rund 5,2 Metern Tiefe. Gegenüber der früheren Habitat-Messung desselben Szenarios (68,18 / 54,48 FPS) bleibt die mittlere Leistung ähnlich; der kleine Unterschied ist kein belastbarer Geschwindigkeitsgewinn. Material-, Präzisions- und zwischenzeitliche Nachladeänderungen sind in diesem Gesamtvergleich gemeinsam enthalten.

Durchgehend 60 FPS, Mindesthardware, Langzeitstabilität und sämtliche Blickrichtungen sind weiterhin nicht nachgewiesen. Der frühere Murg-Streamingwert wird nicht als neue Messung dieses Pakets übernommen. EXE, Container, Rohdaten und Prüfbilder: [Benthic-Prüfbericht](BENTHIC_RESULTS.json).

## WindowsPlantCanopy: feinere Blattstruktur

Das Paket aus Quellstand `68b9022` wurde im gleichen Weesen-Unterwasserszenario 60 Sekunden nach 15 Sekunden Aufwärmen gemessen: RTX 3090 / Ryzen 9 9950X3D, 1920×1080, Hoch, 100 Prozent Renderauflösung, ohne Bildratenlimit. Es lief keine weitere Unreal-/Spielinstanz; die GPU-Auslastung vor dem Start lag bei 16 Prozent. 4.061 vollständige innere Bildintervalle ergeben 59,975 s, **67,71 FPS im Mittel / 54,71 FPS im langsamsten Prozent**. P95 beträgt 17,30 ms, die höchste Bildzeit 19,32 ms. 514 Bilder überschreiten 16,67 ms, keines 33,33 ms.

Die GPU-Zeit liegt im Mittel bei 14,15 ms, der Spielthread bei 5,82 ms. Das geprüfte Messbild zeigt den Weesen-Boden bei rund 5,2 Metern Tiefe. Die mittlere Bildrate liegt nahe am älteren Benthic-Lauf (68,34 FPS); diese Einzelmessungen mit unterschiedlicher Hintergrundauslastung isolieren die Kosten der Pflanzenänderung nicht. Durchgehend 60 FPS, alle Blickrichtungen und ein neuer Langzeittest bleiben offen. [Paket- und Leistungsnachweis](PLANT_CANOPY_RESULTS.json).

## WindowsViewPath: Blickweg statt konstantem Farbfilter

Der eigenständige Stand `f19bd3f` wurde 60 Sekunden nach 15 Sekunden Aufwärmen im Weesen-Unterwasserszenario gemessen: RTX 3090 / Ryzen 9 9950X3D, 1920×1080, Hoch, feste 100 Prozent Renderauflösung, ohne VSync/Bildratenlimit und ohne weitere Unreal-/Spielinstanz. Die vor dem Start gemeldete GPU-Auslastung betrug 25 Prozent. Aus 4.073 vollständigen inneren Bildintervallen über 59,968 s ergeben sich **67,92 FPS im Mittel / 54,37 FPS im langsamsten Prozent**. P95 beträgt 17,34 ms, die höchste Bildzeit 19,78 ms; 503 Bilder überschreiten 16,67 ms, keines 33,33 ms.

GPU und Spielthread benötigen im Mittel 14,12 bzw. 5,81 ms. Das geprüfte Messbild zeigt den bodennahen Weesen-Bereich bei 5,2 Metern Tiefe. Die mittlere Leistung bleibt numerisch nahe am PlantCanopy-Lauf (67,71 FPS), ohne daraus einen Geschwindigkeitsgewinn oder isolierte Shaderkosten abzuleiten. Dieser Lauf belegt weder durchgehend 60 FPS noch Langzeitstabilität oder alle Szenarien. [Paket-, Bild- und Rohdatennachweis](VIEW_PATH_RESULTS.json).

## WindowsFormationsFinal: zusammenhängende Felsgruppen

Quellstand `9833ef4`, RTX 3090 / Ryzen 9 9950X3D, 1920×1080, Qualität Hoch, feste 100 Prozent Renderauflösung, ohne VSync/Bildratenlimit. Beide Läufe beginnen nach 15 Sekunden Aufwärmen und ohne weitere Unreal-/Spielinstanz. Die veränderte Felsanordnung beeinflusst sichtbare Flächen und Schatten; diese Messungen isolieren keine Optimierungskosten gegenüber älteren Paketen.

| Messung | Mittel | 1%-Low | P95 | Maximum | Bilder >33,33 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Weesen unter Wasser, 60 s | 72,91 FPS | 61,79 FPS | 15,32 ms | 17,42 ms | 0 / 4.373 |
| Murg Schwimmen/Nachladen, 90 s | 100,88 FPS | 81,55 FPS | 10,85 ms | 34,36 ms | 1 / 9.077 |

Der Weesen-Lauf umfasst 59,980 Sekunden vollständige Bildintervalle; drei Bilder überschreiten 16,67 ms. GPU und Spielthread benötigen im Mittel 13,10 bzw. 6,60 ms. Die gemeldete GPU-Auslastung vor dem Start liegt bei 20 Prozent. Die Sichtkontrolle zeigt den bodennahen Bereich bei 5,2 Metern Tiefe mit den neuen Felsgruppen. Durchgehend 60 FPS und Langzeitstabilität sind damit weiterhin nicht belegt.

Der Murg-Lauf umfasst 89,980 Sekunden und 144,86 Meter Bewegung. Vier Bilder überschreiten 16,67 ms, eines 33,33 ms, keines 100 ms. GPU und Spielthread benötigen im Mittel 9,30 bzw. 4,68 ms; die höchste Spielthreadzeit beträgt 33,94 ms. Die GPU-Auslastung vor dem Start liegt ebenfalls bei 20 Prozent. Das Nachladeprotokoll bestätigt die gestaffelten Übergänge mit sieben, acht und neun aktiven Zellen. Das abschließende Bild zeigt freies Wasser bei 24,7 Metern Tiefe; diese Strecke ist keine Messung dauerhaft dichter bodennaher Felsansichten. Der einzelne Ruckler bleibt ein offener Leistungsbefund. [Quellstand, Paket, Bilder und Rohdatennachweise](FORMATIONS_RESULTS.json).
