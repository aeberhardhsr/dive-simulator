# Kosten der Wasserstreuung in Reflexionen

Die zusätzlichen Schattenabfragen entlang der Reflexionsstrahlen sind in der geprüften Uferszene der wichtigste Mehrbedarf des neuen Wasserwegs. Das Erzwingen roher Hardware-Reflexionstreffer allein ist deutlich günstiger. Eine erste Variante mit wiederverwendeten Sonnenlichtdaten ergab keinen nachgewiesenen Geschwindigkeitsgewinn und wird nicht als Standard übernommen. Es wurde in diesem Schritt keine schnellere Spielversion ausgeliefert.

## Messung

Vier aufeinanderfolgende native Läufe: jeweils 60 Sekunden nach 15 Sekunden Aufwärmzeit, 1920 × 1080, Qualitätsstufe 2, 100 % Renderauflösung, keine Bildratenbegrenzung. RTX 3090 und Ryzen 9950X3D. Der simulierte Taucher dreht sich bei 25° Blickneigung nach oben; die Wasseroberfläche liegt im Bild. Alle Varianten verwenden die Auswahl aus der [Studie zum einheitlichen Wasserweg](REFLECTION_UNIFIED_STUDY.md), den Wasser-Stencil und dieselben Mediumdaten.

| Variante | Durchschnitt | Mittlere GPU-Zeit | GPU-Pass LumenReflections |
| --- | ---: | ---: | ---: |
| `routing`: nur rohe Reflexionstreffer, keine Wasserintegration | 67,30 FPS | 14,22 ms | 2,60 ms |
| `unshadowed`: 16 Wasserabschnitte, ohne zusätzliche Volumenschatten | 66,15 FPS | 14,49 ms | 2,87 ms |
| `cached`: vollständige Integration, Sonnenlichtdaten wiederverwendet | 55,45 FPS | 17,39 ms | 5,77 ms |
| `full`: vollständige Integration als frische Referenz | 55,49 FPS | 17,38 ms | 5,78 ms |

Die ersten beiden Varianten sind Diagnoseeingriffe und keine zulässigen Ersatzbilder für die fertige Wasseroptik. `routing` lässt die Dämpfung und Streuung des reflektierten Wasserwegs aus. `unshadowed` lässt Licht durch Abschattungen wirken. Ihre bessere Bildrate wird deshalb nicht als Optimierung des Spiels gewertet.

Zwischen vollständiger Integration und der Variante ohne zusätzliche Schattenabfragen liegen rund 2,91 ms im Reflexionspass. Die Wiederverwendung der konstanten Lichtdaten ändert diesen Pass dagegen nur um etwa 0,015 ms. Diese kleine Differenz rechtfertigt keine Übernahme als Leistungsverbesserung.

Die Aufnahmen zeigen jeweils den tatsächlich nach oben gerichteten Unterwasserblick. Die Kamerapfade sind ähnlich, aber nicht pixelidentisch; zwischen `cached` und `full` unterscheiden sich der Schlusskurs um etwa 1,71° und die Simulationsdauer um 0,28 s. Es handelt sich um bewegte Spielszenen im Source-Editor-Lauf, keine isolierten GPU-Mikromessungen oder Paketabnahme. GPU-Passzeiten können überlappen und dürfen nicht addiert werden. Zwei der früheren Diagnoseläufe verwenden einen Modulstand mit weniger erlaubten Startprofilen; diese Änderung betrifft die Prüfung beim Programmstart, nicht die laufende Szenensimulation.

## Umsetzung und Schutz vor Verwechslung

`PrepareReflectionTransport.py --profile` erzeugt den gewählten Shaderzweig sowie einen Manifest-Eintrag und `ProfileMode.txt`. Das Programm prüft den angeforderten Modus gegen diese Datei und protokolliert ihn. Der Benchmark überprüft das Protokoll und bindet die Shaderdateien an die Messung. Die normale Vorgabe bleibt `full`.

`cached` hält Richtung, Brechungsfaktor und phasenabhängige Farbe von höchstens zwei gerichteten Lichtquellen pro Strahl vor. Bei mehr gerichteten Lichtern wird die ursprüngliche Berechnung verwendet. Endliche Lichtquellen werden weiterhin an jedem Abschnitt anhand des dortigen Lichtrasters ausgewählt. Alle 16 Wasserabschnitte und ihre Schattenabfragen bleiben erhalten. Der Zweig bleibt nur zur Reproduktion des verworfenen Optimierungsversuchs verfügbar; seine Abschlussaufnahme ersetzt keine vollständige visuelle Abnahme.

Nach den Messungen wurde `full` wiederhergestellt. Zwei native Fälle mit sechs Uferaufnahmen liefen erfolgreich durch und wurden bildlich geprüft. Gegenüber den entsprechenden zuvor geprüften Aufnahmen betragen die mittleren absoluten RGB-Abweichungen 0,24 bis 0,51 Anzeigestufen. Die gespiegelten Steine und das Sichtfenster zur Sonne bleiben erhalten. Die Kontrolle gilt für diese Aufnahmen und ersetzt keine vollständige Abnahme der Wasseroptik.

## Nächster Ansatz

**Nachtrag 28. November 2026:** Der hier beschriebene nächste Versuch ist inzwischen implementiert und geprüft. Die [abgeschlossene Schattenkarten-Optimierung](REFLECTION_SHADOW_STUDY.md) erreicht in ihrem frischen Vergleich 62,29 statt 55,19 FPS. Die folgenden Absätze beschreiben den damaligen Ausgangspunkt.

Die Engine nutzt für ihre normale Volumenbeleuchtung bereits `SampleVirtualShadowMapTranslatedWorld`. Die Rückgabe enthält eine Gültigkeitsangabe. Ein nächster Versuch soll vorhandene Szenenschatten für die passende Lichtquelle nutzen und bei fehlenden Daten auf die bisherigen Schattenstrahlen zurückfallen. Dafür müssen die aktuellen VSM-Ressourcen ausdrücklich an den Reflexionspass gebunden werden; die bislang vorhandene Forward-Light-Bindung allein genügt nicht. Es gibt hierzu noch keinen implementierten oder gemessenen Leistungsgewinn.

Zu prüfen sind dann insbesondere die richtige Sonnen-/Mondzuordnung, fehlende Kartenseiten, feine Uferschatten, weit entfernte Bergabschattung, bewegte Lampen und Übergänge beim Wechsel zwischen Schattenkarte und Raytracing. Die übrigen offenen Grenzen der Wasseroptik aus der vorherigen Studie bestehen weiter.

## Reproduktion

Nach dem Source-Editor-Build werden die vier Modi nacheinander vorbereitet und gemessen. Beispiel für `routing`; für die übrigen Läufe den Profilnamen in beiden Befehlen entsprechend ersetzen:

```powershell
Tools/.geodata-env/Scripts/python.exe Tools/PrepareReflectionTransport.py --unified --profile routing
./Tools/Benchmark.ps1 -EditorSandbox -SandboxName SourceEditorSandbox -EngineRoot 'O:/games/dive game/Builds/UE584' -WaterPreview -ReflectionTransportStudy -ReflectionUnified -ReflectionProfile routing -Scenario underwater -Entry 0 -LookPitch 25 -Seconds 60 -Resolution 1080p -Quality 2
```

Anschliessend den vollständigen Pfad wiederherstellen und seine Uferansichten prüfen:

```powershell
./Tools/TestReflectionTransport.ps1 -Unified -Suite consistency
Tools/.geodata-env/Scripts/python.exe Tools/ReportReflectionCost.py --full artifacts/perf-underwater-1080p-q2-20261127-233538-748 --routing artifacts/perf-underwater-1080p-q2-20261127-232542-879 --unshadowed artifacts/perf-underwater-1080p-q2-20261127-232810-849 --cached artifacts/perf-underwater-1080p-q2-20261127-233220-915 --visual artifacts/reflection-transport-20261127-233828 --previous-visual artifacts/reflection-transport-20261127-231414
```

Für neue Läufe sind beim Bericht die neu ausgegebenen Verzeichnisse einzusetzen. Der [Ergebnisbericht](REFLECTION_COST_RESULTS.json) bindet Rohmessungen, tatsächliche Einstellungen, Shaderstände und Bilder. Die wiederhergestellte Referenz bleibt ein Render-Versuch; das bisherige spielbare Paket wurde nicht verändert.
