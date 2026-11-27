# Visuelles Qualitätsziel

Die am 26. November 2026 vom Nutzer bereitgestellten Bilder definieren die gewünschte Detail- und Realitätsstufe für das fertige Spielerlebnis. Sie sind gestalterische Referenzen, keine bereits erreichten Spielaufnahmen, geografischen Messungen oder Vorgaben für eine identische Szene. Originale liegen unverändert unter `References/`. Daraus werden keine Texturen oder Hintergrundkulissen für das Spiel übernommen.

## Unter Wasser – zuerst

![Qualitätsreferenz Unterwasserwelt](References/Underwater_QualityTarget.png)

Das Ziel ist eine zusammenhängende, räumlich glaubwürdige Umgebung: große Felsformationen und Geländeformen, mittlere Blöcke, kleinere Steine und feines Sediment greifen ineinander. Natürliche Größenverteilung, unregelmäßige Gruppen, teilweise eingebettete Steine und unterschiedliche freie Bodenflächen vermeiden einen gleichmäßig dekorierten Untergrund. Bewuchs und Oberflächenbeläge verbinden Felsen und Boden, mit fein differenzierten Pflanzenformen und nachvollziehbaren Übergängen.

Licht von der Wasseroberfläche, Abschattung, lokale Lichtmuster und allmählicher Kontrastverlust mit der Entfernung müssen gemeinsam Tiefe erzeugen. Farbe, Trübung und Lichtstrahlen bleiben abhängig von Sichtweite, Tiefe, Wetter und Sonnenstand. Der helle türkisgrüne Referenzzustand ist kein konstanter Filter für alle Tauchgänge. Materialdetails müssen bei normalem Spielabstand sichtbar sein; Nahansichten dürfen keine vorzeitig groben Detailstufen zeigen.

Der aktuelle Stand liegt deutlich unter dieser Gesamtreferenz. Drei detailliertere lose Felsformen und zwei zusätzliche Gruppenkerne sind Bausteine. Besonders offen bleiben die Verzahnung der unterschiedlichen Größenstufen, organischer Bewuchs, natürliche Materialübergänge sowie die gemeinsame visuelle Wirkung von Gelände, Wasser und Beleuchtung. Der [Asset- und Lückenabgleich](VISUAL_ASSET_REVIEW.md) konkretisiert die nächsten Arbeiten anhand vorhandener Spielaufnahmen und Quelldaten.

## Über Wasser – anschließend auf vergleichbarem Niveau

![Qualitätsreferenz Landschaft über Wasser](References/AboveWater_QualityTarget.png)

Bergsilhouetten und gegliederte Steilflächen, Waldvolumen, Wiesen, Ufer und begehbare Nahbereiche müssen über mehrere Entfernungen glaubwürdig zusammenpassen. Die reale Walensee-Geografie und Maßstäblichkeit bleiben maßgeblich. Grobe Geländefacetten, flächig wirkende Wälder oder aufgezogene Luftbilder reichen für das Ziel nicht aus. Vegetation, Felsbänder und Bodenmaterialien brauchen räumliche Struktur und zusammenhängende Übergänge.

Wasserreflexionen, kleine Wellen und der sichtbare Grund in flachem Wasser sollen zur tatsächlichen Umgebung passen. Einstiegselemente erhalten überzeugende Geometrie, Materialmaßstäbe und Gebrauchsspuren. Der dargestellte Holzsteg wird dadurch nicht an jedem Einstieg vorgeschrieben. Beleuchtung, Schatten und atmosphärische Entfernung müssen Landschaft und Wasser verbinden.

## Sichtprüfung im spielbaren Paket

Verglichen werden echte Spielaufnahmen und Bewegung im ausgelieferten Unreal-Paket: Unterwasser-Nahansicht, durchschwommene Fels-/Bewuchszone, tieferer und trüberer Bereich, Uferwechsel, begehbarer Nahbereich und Bergpanorama. Vergleichbare Kamera, Auflösung, Sichtweite und Beleuchtung werden dokumentiert. Bewegungen prüfen zusätzlich Detailwechsel, Nachladen, Flimmern, Materialstreckung und Übergänge an der Wasserlinie.

Technische Testmarker, Polygonzahlen oder einzelne attraktive Standbilder ersetzen diese visuelle Gesamtabnahme nicht. Die bestehenden Anforderungen an spielbare Bildraten und Stabilität gelten weiterhin. Ein bestandener Renderingtest bedeutet daher noch nicht, dass das Qualitätsziel dieser Referenzen erreicht ist.

## Menü-/HUD-Überarbeitung während des Engine-Builds

Ursprünglich war die Neugestaltung von Menüführung, HUD und FPV nach der Unterwasser- und Überwasser-Grafik vorgesehen. Am 27. November hat der Nutzer ausdrücklich erlaubt, diese Arbeit während des laufenden Engine-Builds vorzuziehen. Die fünf zusätzlichen Entwürfe, die bemängelten Layout-/Text-/Umlautprobleme und die Abnahmekriterien stehen im [UI-/UX-Designziel](UI_UX_TARGET.md). Die parallele UI-Arbeit ersetzt keine der oben beschriebenen Grafikarbeiten.
