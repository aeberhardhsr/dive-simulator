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
