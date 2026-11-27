# Kleine Sedimentrücken: nicht als Spielstandard übernommen

Ein nativer Vergleich auf Basis von `b0722003b1f29407b8a2ed6cdf63e87cc40bab3a` untersucht echte Höhenänderungen der gerenderten Meterraster-Dreiecke zwischen den vorhandenen Felsen. **Der Entwurf wird nicht übernommen:** Die Lage kleiner Steine und Pflanzen ändert sich, die grosse freie Bodenfläche wirkt in den vollständigen Bildern aber weiterhin zu flach. Das visuelle Ergebnis genügt nicht für die gewünschte [Referenzqualität](VISUAL_TARGET.md).

Die deterministischen, gestalterisch definierten Rücken erreichen vor den Habitatmasken höchstens 45 cm. Sie werden in grobem Kies abgeschwächt, an steilen Hängen sowie nahe Einstiegen ausgeblendet und mit den bestehenden Felsfuss-Ablagerungen vereinigt. Die vermessenen Quelldaten bleiben erhalten. Sichtbare Geometrie, Platzierung und Bodenkontakt verwenden die gemeinsame resultierende Oberfläche. Dies ist keine vermessene Sedimentverteilung oder Strömungssimulation.

Vorher:

![Bisheriger Boden](Images/SedimentBedform_Before.png)

Untersuchtes Relief:

![Begrenzte zusätzliche Sedimentrücken](Images/SedimentBedform_After.png)

Zwei native Fälle mit sechs Aufnahmen bestehen. Kamerapositionen, Uhrzeit, Datum und Wasserprofil stimmen überein. Im geänderten Fall treffen alle 1.598 geprüften Bodenkontakte die dargestellte Oberfläche; 14 Datenrandproben bleiben unverändert. Nach Leeren der Höhen-Caches werden identische Werte rekonstruiert. Der portable Modelltest besteht für endliche/beschränkte Höhen, Variation, wiederholbare Weltkoordinaten und stetige Zellübergänge. Er misst im geprüften Bereich 0 bis 44,649 cm vor den Habitatmasken. Diese technischen Ergebnisse sind keine gestalterische Abnahme.

Der [Bericht](SEDIMENT_BEDFORM_RESULTS.json) enthält Eingabeprüfsummen, Kameraangaben, Kontaktmarker, Modelltest und Bilddateien. Es gibt für diesen Entwurf keinen Paket-, Performance- oder Langzeitnachweis. Die zuletzt geprüfte spielbare Vorschau bleibt `Builds/WindowsPlantColor/Start-WaterPreview.cmd`.

Der vollständige [Quellpatch](Experiments/sediment-bedforms.patch) ist archiviert; die Versuchsänderungen wurden aus den Laufzeitquellen zurückgenommen. Er lässt sich auf dem genannten Basisstand mit `git apply` reproduzieren. Nach dem Editor-Build aktiviert `Tools/SmokeTest.ps1 -EditorSandbox -Scenario rocks -HabitatView -HabitatLowView -WaterPreview -PopulationChecks -SedimentBedforms -Hour 12 -Date 2026-06-21 -MaxFPS 30` das Experiment; derselbe Aufruf ohne `-SedimentBedforms` erzeugt die Kontrolle. Der Patch verändert keine Spielassets.

Nach der Rücknahme wurde das Editor-Modul neu kompiliert. Der anschliessende native Kontrolllauf mit drei Habitatansichten und Bodenkontaktprüfungen besteht; er ist als Fall `control` im [Lichtversuch-Bericht](SOLAR_SHADER_RESULTS.json) gebunden. Die Laufzeitquellen von `DiveSim` entsprechen wieder dem genannten Basisstand.

Das Ergebnis lenkt die nächste Arbeit auf das Zusammenspiel von Unterwasserbeleuchtung, grosser Geländeform und Materialwirkung. Eine stärkere Variante derselben kleinen Rücken gilt nicht automatisch als Verbesserung.
