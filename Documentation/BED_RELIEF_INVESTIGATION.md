# Bodenrelief: Materialversuch nicht in die Spielvorschau übernommen

Der Versuch untersucht die flachen Bodenflächen zwischen räumlichen Steinen. **Entscheidung: nicht als neuer Spielstandard übernehmen.** Die Nahdetails ändern sich, die räumliche Wirkung der gesamten Szene bleibt aber zu ähnlich. Der zusätzliche GPU-Aufwand rechtfertigt diesen begrenzten Gewinn nicht. Das ist eine gestalterische Bewertung der Vergleichsbilder, kein automatisches Ergebnis eines Bilddifferenz-Schwellwerts.

Die spielbare Vorschau bleibt `Builds/WindowsPopulationRetire/Start-WaterPreview.cmd`. Laufzeitquellen, Shader und alle veränderten Spielassets wurden anschließend auf ihren vorher geprüften Stand zurückgesetzt. Das Experiment bleibt mit Quellen, Patch, Bildern und Messdaten nachvollziehbar; es ist weder ein neues Spielpaket noch eine Referenzabnahme.

Die [Wiederherstellungsprüfung](BED_RELIEF_RESTORATION.json) bestätigt alle 207 Eingabedateien der letzten Paketprüfung bytegenau, außerdem die unveränderte Spiel-EXE und ihre Inhaltscontainer. Der neu gebaute Editor besteht anschließend einen weiteren nativen Tauchlauf mit drei Aufnahmen und ohne Material-Fallback. Der archivierte Patch wurde auf dem wiederhergestellten Basisstand mit `git apply --check` geprüft.

## Untersuchte Varianten

Passende Original-Höhenkarten ergänzen die vorhandenen Poly-Haven-Scans [Gravelly Sand](https://polyhaven.com/a/gravelly_sand) und [River Small Rocks](https://polyhaven.com/a/river_small_rocks). Die bereits dokumentierte CC0-Herkunft gilt auch für diese Anbieterdateien. URLs, Größen, Anbieter-MD5 und eigene SHA256 stehen in [BED_RELIEF_SOURCES.json](BED_RELIEF_SOURCES.json). Die PNGs bleiben unverändert unter `Art/BedMaterials`; `Tools/prepare_bed_relief.py` lädt bzw. prüft genau diese Quellen. Sie sind keine Messungen einzelner Sedimenthöhen im Walensee.

Die erste Variante verschiebt die Materialabtastung abhängig von Blickrichtung und Scan-Höhe. Sie besteht zwei native Fälle mit sechs Aufnahmen unter `artifacts/bed-relief-20261127-034458`, wirkt in der vollständigen Unterwasseransicht aber zu schwach.

Die zweite Variante normalisiert den nutzbaren Höhenbereich anhand der ersten und 99. Perzentile der Originalkarten. Zusätzlich bestimmt die Kieshöhe, welche Teile eines Übergangs als Kies oder Sand erscheinen. Farbe, Normale, Rauheit und AO verwenden dieselbe Mischungsmaske. Damit bleiben einzelne erhöhte Kiesel erkennbar, statt zwei Oberflächen überall gleichmäßig zu mitteln.

Der Parallax-Marsch verwendet 12 bis 28 Schritte, explizite Texturgradienten und anschließend eine lineare Trefferinterpolation. Die gestalterisch gewählten Tiefenbereiche betragen 1,5 cm für Sand und 6 cm für Kies. Der Effekt blendet zwischen 8 und 16 Metern, an steileren Flächen und bei sehr flachem Blick aus. Die höhenabhängige Materialmischung geht zwischen 15 und 30 Metern in die bisherige Mischung über. Diese Werte sind keine aus dem See abgeleitete physikalische Simulation.

Die Methode verschiebt nur Materialkoordinaten. Sie verändert weder Silhouetten noch Geländegeometrie, Schattenwurf, Tiefenpuffer oder Kollision. Es gibt keinen zusätzlichen Selbstschatten-Marsch. Genau diese Grenze ist für die gewünschte räumliche Bodenwirkung entscheidend.

## Bildvergleich der zweiten Variante

Native Sandbox, identische Kamera, Weesen, drei Blickrichtungen. `-DiveBedReliefOff` schaltet den Effekt in derselben EXE und mit denselben Assets ab. Die sechs Bilder und alle Laufzeitprüfungen bestehen unter `artifacts/bed-relief-20261127-034803`. Der [technische Bericht](BED_RELIEF_EXPERIMENT_RESULTS.json) bindet die damaligen Quellen, Assets, Kameraangaben, Bilder und Messdateien; sein `PASS` belegt die Prüfungen, nicht die gestalterische Annahme.

Ohne Effekt:

![Bisherige Bodenoberfläche](Images/BedReliefExperiment_Before.png)

Mit normalisiertem Parallax und höhenabhängiger Mischung:

![Untersuchte Materialvariante](Images/BedReliefExperiment_After.png)

Die mittlere absolute RGB-Differenz der vollständigen Bilder beträgt je nach Blickrichtung 2,67, 1,88 und 0,71 von 255. In der unteren Bildhälfte beträgt sie 4,25, 3,18 und 1,07. Diese Zahlen zeigen eine Änderung; sie messen keine Realitätsnähe. Die großen ebenen Flächen, die Verteilung der Felsgruppen und ihre Silhouetten bleiben erhalten. Ein überzeugender Gewinn gegenüber der Nutzerreferenz ist nicht belegt.

## Leistung und Grenzen der Messung

RTX 3090 / Ryzen 9 9950X3D, native Editor-Sandbox im Spielmodus, Wasserprofil, 1080p/Hoch, 100 Prozent Renderauflösung, ohne VSync oder Bildratenlimit. Keine weitere Unreal-/Spielinstanz war beim Start aktiv. Jeweils 15 Sekunden Aufwärmen und 30 Sekunden Erfassung; zwei aufeinanderfolgende Einzelläufe, keine statistische Messreihe.

| Messwert | Ohne Effekt | Mit Effekt |
| --- | ---: | ---: |
| Mittlere Bildrate | 69,25 FPS | 67,95 FPS |
| Mittlere GPU-Zeit | 13,78 ms | 14,05 ms |
| Mittlere Spielthreadzeit | 5,27 ms | 5,26 ms |
| P95-Framezeit | 15,78 ms | 16,00 ms |
| Größte Framezeit | 40,42 ms | 39,73 ms |
| Frames über 16,67 ms | 6 / 2076 | 16 / 2036 |
| Frames über 33,33 ms | 2 | 2 |

Die Messordner sind `artifacts/perf-underwater-1080p-q2-20261127-035030-449` (ohne) und `artifacts/perf-underwater-1080p-q2-20261127-035141-983` (mit). Der GPU-Mittelwert liegt mit Effekt um rund 0,27 ms höher. Die zwei langsamen Frames pro Lauf wurden nicht entfernt. Native Ergebnisse dürfen nicht als neue Paketleistung oder als Langzeitnachweis ausgegeben werden. Temporalstabilität, alle Einstiege, Nacht und Ufer sind für diese verworfene Variante nicht vollständig geprüft.

## Reproduzierbarer Entwicklungsstand

[Der archivierte Quellpatch](Experiments/bed-relief.patch) basiert auf `ed1e2c18bf6b8abd7d94673aba6ee9695d204333`. Er enthält die Materialerzeugung, den Shader, den Diagnoseschalter sowie Vergleichs- und Berichtswerkzeuge. Neue Höhenkarten und ihr Manifest liegen separat im Repository. Der Patch ist nicht im normalen Spielcode aktiviert. Auf einem separaten Checkout dieses Basisstands lassen sich nach Übernahme des Patches, der Höhenkarten, des Manifests und von `Tools/prepare_bed_relief.py` folgende Schritte ausführen:

```powershell
git apply Documentation/Experiments/bed-relief.patch
python Tools/prepare_bed_relief.py
./Tools/Build.ps1 -EditorSandbox -Target Content -OnlyBenthic
./Tools/Build.ps1 -EditorSandbox
./Tools/TestBedRelief.ps1 -Preview
./Tools/Benchmark.ps1 -Editor -EditorSandbox -WaterPreview -Scenario underwater -Seconds 30 -BedReliefOff
./Tools/Benchmark.ps1 -Editor -EditorSandbox -WaterPreview -Scenario underwater -Seconds 30
```

Der erste Anlauf `artifacts/bed-relief-20261127-034323` wurde wegen fehlender Shader-Eingänge im Kieselmaterial abgebrochen. Der archivierte Stand entfernt dort den nicht benötigten Bodenpfad; die späteren Vergleiche enthalten keinen Material-Fallback. Ein lokaler C++-Namenskonflikt wurde ebenfalls vor den erfolgreichen Läufen korrigiert.

Die nächste visuelle Arbeit muss räumliche Bodenformen und Übergänge in der vollständigen Szene verbessern. Eine weitere Verstärkung desselben Materialeffekts ersetzt diese Arbeit nicht. Überwasserwelt und Menü/HUD/FPV bleiben in der vereinbarten Reihenfolge nachgeordnet.
