# Einheitlicher Wasserweg für Unterwasserspiegelungen

Der getrennte Source-Engine-Versuch führt markierte Wasserreflexionen unter Wasser durch denselben Hardware-Raytracing-Pfad. Dadurch enthält die Reflexion keine bereits für den Kamerablick berechnete Wassertrübung aus einem Bildschirmtreffer. Nahe Steine bleiben in den geprüften Spiegelungen sichtbar; das Sichtfenster durch die Oberfläche bleibt erhalten. Der Versuch ist noch nicht im spielbaren Paket aktiviert.

## Auswahl und Berechnung

`ReflectionWaterMask.ush` bündelt die Auswahl anhand des Front-Layer-Passes, Wasser-Stencils 42, passender Custom-Depth, der Kameralage unter der gemeinsamen Wellenoberfläche und der Seegrenze. Diese Auswahl gilt sowohl für den Bildschirm- als auch für den Hardware-Reflexionspass.

Für ausgewählte Strahlen überspringt der Bildschirmtest die bereits beleuchtete Bildschirmfarbe. Der vorherige Clear-Pass hinterlässt einen ungelösten Strahl mit Startdistanz null; der Hardware-Pass verfolgt ihn anschliessend vollständig. Auch spätere Bildschirmfarb- und Radiance-Cache-Abkürzungen werden für diese Strahlen umgangen. Geometrietreffer erhalten direkte Hit-Lighting-Berechnung und danach die bestehende Wasserintegration. Die Berechnung der Einfachstreuung selbst bleibt gegenüber der [vorherigen Studie](REFLECTION_TRANSPORT_STUDY.md) unverändert.

Die separate Quellcode-Engine benötigt dafür eine zusätzliche Bindung der Scene-Textures im Bildschirm-Reflexionspass. `PrepareReflectionRendererPatch.py` erzeugt einen prüfbaren Diff für genau den erwarteten UE-5.8.4-Quellstand und kann ihn ausschliesslich im getrennten `Builds/UE584` anwenden. Die installierte Engine wird nicht verändert. Der Lauf prüft die kompilierte Fähigkeit über `r.DiveWater.ReflectionBindingVersion=1` und kontrolliert, dass vorbereitete Shader und angeforderter Routingmodus zusammenpassen.

## Native Bildprüfung

![Native Uferaufnahme mit gespiegelten Steinen nach vereinheitlichter Wasserberechnung](Images/ReflectionUnified_Near.png)

Unveränderte Aufnahme aus 1,5 m Tiefe; sie zeigt den geprüften Reflexionspfad, nicht die angestrebte endgültige Umgebungsqualität.

Die Serie `artifacts/reflection-transport-20261127-231045` enthält Kontrolle, aktive Integration mit global deaktivierten Bildschirmreflexionen und aktive Integration mit regulär eingeschalteten Bildschirmreflexionen. Jeweils aufgenommen wurden −20 cm, −2 cm und +20 cm relativ zur lokalen Welle. Die Serie `artifacts/reflection-transport-20261127-231414` ergänzt zwei Unterwasserblicke aus 1,5 m Tiefe und einen Luftblick im flachen Weesener Uferbereich, ebenfalls mit und ohne Bildschirmreflexionen. Alle fünf Fälle mit 15 nativen Bildern liefen erfolgreich durch und wurden visuell geprüft.

Bei den beiden untergetauchten Wasserlinienkameras unterscheiden sich die aktiven Varianten mit und ohne Bildschirmreflexionen im Mittel nur um 0,24 beziehungsweise 0,16 RGB-Anzeigestufen. Bei den beiden Uferblicken sind es 0,26 beziehungsweise 0,25 Stufen. Die gespiegelten Steine und das Sichtfenster zur Sonne bleiben erkennbar. Die geringe Differenz allein ist kein Nachweis physikalischer Korrektheit; die Prüfung kombiniert die Bilder mit der kontrollierten Auswahl der Strahlen.

Im ursprünglichen blauen Prüfbereich bleibt der Anteil blau dominierter Pixel bei 0 %. Seine mittlere Anzeigehelligkeit beträgt 32,19 beziehungsweise 28,72 von 255. Die Luftaufnahme mit regulären Bildschirmreflexionen unterscheidet sich zwischen Kontrolle und aktivem Verfahren im Mittel nur um 0,33 RGB-Stufen. Das globale Abschalten der Bildschirmreflexionen verändert die Luftaufnahme dagegen deutlich; der neue Auswahlpfad schaltet sie dort gerade nicht ab.

## Leistung und verbleibende Grenzen

Die Leistung wurde mit zwei aufeinanderfolgenden 60-Sekunden-Läufen bei 1920 × 1080, Qualitätsstufe 2, 100 % Renderauflösung und ohne Bildratenbegrenzung gemessen. Der Unterwasserblick ist um 25° nach oben geneigt, damit die Wasseroberfläche im gemessenen Bild liegt. Beide Läufe verwenden denselben Source-Editor-Build, Wasser-Stencil, Mediumdaten und deaktiviertes Inline-Lumen; nur die Shaderersetzung unterscheidet sich. Der [Ergebnisbericht](REFLECTION_UNIFIED_RESULTS.json) bindet Kameralage, tatsächliche Rendereinstellungen, Roh-CSV und Abschlussbild.

| Messwert | Kontrolle | Einheitlicher Wasserweg |
| --- | ---: | ---: |
| Durchschnitt | 68,31 FPS | 56,11 FPS |
| Langsamstes 1 % | 52,17 FPS | 44,83 FPS |
| 95. Perzentil Bildzeit | 15,84 ms | 19,15 ms |
| Mittlere GPU-Zeit | 14,01 ms | 17,19 ms |
| GPU-Pass LumenReflections | 2,46 ms | 5,68 ms |
| Bilder über 33,33 ms | 2 / 4097 | 2 / 3365 |

Gemessen auf RTX 3090 und Ryzen 9950X3D. Die Bildrate sinkt in diesem Vergleich um 17,86 %. Die zusätzliche GPU-Zeit liegt überwiegend im Reflexionspass. Das Verfahren hält hier noch keine durchgehenden 60 FPS; die Kosten der zusätzlichen Strahlen und Lichtabtastungen sind vor der Übernahme zu optimieren.

Die anschliessende [Kostenanalyse](REFLECTION_COST_STUDY.md) trennt rohe Reflexionstreffer, Wasserintegration und Schattenabfragen. Sie weist die zusätzlichen Volumenschatten als den wesentlichen Mehrbedarf in dieser Szene aus.

Die Abschlussbilder beider Läufe zeigen die Unterseite der Wasseroberfläche. Tatsächliche Tiefe jeweils rund 4,35 m, Blickneigung 25°, Simulationsdauer 74,00 beziehungsweise 73,94 s; der Schlusskurs unterscheidet sich um etwa 0,34°. Beide Läufe melden 41 durch Kollision verworfene Haltungsschritte. Das sind vergleichbare bewegte Spielszenen, jedoch keine pixelidentischen GPU-Mikromessungen und keine Leistungsabnahme eines gepackten Spiels. Die zwei Läufe liegen unter `artifacts/perf-underwater-1080p-q2-20261127-231608-056` und `artifacts/perf-underwater-1080p-q2-20261127-231809-214`.

Die bekannte dünne helle Linie nahe der Wasserlinie bleibt sichtbar. Indirektes Himmelslicht im Volumen, Mehrfachstreuung, vollständige sekundäre Grenzflächenwechsel und die gemeinsame photometrische Abstimmung sind weiterhin offen. Die Gelände- und Unterwassergestaltung erreicht damit noch nicht das gewünschte Referenzniveau. Diese Änderung behebt einen konkreten Widerspruch zwischen Reflexionsverfahren.

## Reproduktion

Auf dem noch ungepatchten separaten Quellstand zuerst den Renderer-Diff vorbereiten und prüfen, dann einmalig anwenden:

```powershell
Tools/.geodata-env/Scripts/python.exe Tools/PrepareReflectionRendererPatch.py
Tools/.geodata-env/Scripts/python.exe Tools/PrepareReflectionRendererPatch.py --apply
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -Target Editor -EditorSandbox -SandboxName SourceEditorSandbox -MaxParallelActions 4
./Tools/TestReflectionTransport.ps1 -Unified
./Tools/TestReflectionTransport.ps1 -Unified -Suite consistency
./Tools/Benchmark.ps1 -EditorSandbox -SandboxName SourceEditorSandbox -EngineRoot 'O:/games/dive game/Builds/UE584' -WaterPreview -ReflectionTransportStudy -ReflectionUnified -ReflectionTransportControl -Scenario underwater -Entry 0 -LookPitch 25 -Seconds 60 -Resolution 1080p -Quality 2
./Tools/Benchmark.ps1 -EditorSandbox -SandboxName SourceEditorSandbox -EngineRoot 'O:/games/dive game/Builds/UE584' -WaterPreview -ReflectionTransportStudy -ReflectionUnified -Scenario underwater -Entry 0 -LookPitch 25 -Seconds 60 -Resolution 1080p -Quality 2
Tools/.geodata-env/Scripts/python.exe Tools/ReportUnifiedReflections.py --waterline artifacts/reflection-transport-20261127-231045 --near artifacts/reflection-transport-20261127-231414 --control-perf artifacts/perf-underwater-1080p-q2-20261127-231608-056 --active-perf artifacts/perf-underwater-1080p-q2-20261127-231809-214
```

Alle nativen Läufe erfolgen nacheinander. Für neue Serien sind beim Berichtsaufruf die neu ausgegebenen Verzeichnisse einzusetzen. Bereits gepatchte Engine-Dateien werden vom Vorbereitungsskript absichtlich nicht erneut überschrieben. Die benötigte gemeinsame Wasser-Scene-Bindung aus der vorherigen Studie muss ebenfalls vorhanden sein. Das bisherige spielbare Paket bleibt `Builds/WindowsWaterline/Start-WaterPreview.cmd`.
