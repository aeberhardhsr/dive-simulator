# Unterwasserobjekte über mehrere Frames aufbauen

Die entfernten Kies-, Fels- und Pflanzenzellen werden schrittweise vorbereitet und registriert. Formen, Dichte, Materialien, Zufallsfolge und endgültige Transformationen bleiben erhalten. Diese Änderung verringert die auf einen einzelnen Frame konzentrierte Aufbauarbeit; sie ist keine neue gestalterische Qualitätsstufe.

**Geprüfte Vorschau: `Builds/WindowsStreamBuild/Start-WaterPreview.cmd`.** Sieben Paketfälle mit 30 Aufnahmen und der automatische Haupt-EXE-Start bestehen. Der [Prüfbericht](STREAMING_BUILD_RESULTS.json) bindet Quellen, EXE, Inhaltscontainer, Kontaktprüfungen, Aufnahmen und sechs vollständige Messungen zusammen. Die frühere Vorschau `WindowsFootDeposits` bleibt erhalten.

## Ergebnis und Bildprüfung

Die normalen Paketläufe bestätigen 33 Aufrufe für drei Kieszellen und 27 für drei Fels-/Pflanzenzellen, gegenüber jeweils drei vollständigen Zellaufbauten in der Kontrolle. Beibehaltene und wiederhergestellte Anordnungen, nahe Ortswechsel und abgebrochene Teilaufbauten bestehen. Die Bodenproben stimmen bei Weesen 1598/1598, Murg 2278/2278 und Walenstadt 1196/1196 überein. Im Sedimentfall entstehen 21 Wolken; am Ende sind keine mehr aktiv.

Gleiche feste Kamera, vollständiger Aufbau je Zelle:

![Kontrollansicht mit bisherigem Aufbauverfahren](Images/StreamingBuild_Before.png)

Gleiche feste Kamera, schrittweiser Aufbau:

![Gleiche Fels- und Bodenanordnung nach schrittweisem Aufbau](Images/StreamingBuild_After.png)

Die Ansichten behalten dieselben Kameradaten und Objektzahlen. Pflanzenbewegung und zeitliche Bildrekonstruktion können einzelne Pixel verändern. Die sichtbare Geometrie wird für diese Optimierung nicht ausgedünnt. Die geprüften Nahansichten aller drei Orte zeigen weiterhin die bekannten gestalterischen Grenzen: weitgehend freie Sedimentflächen, teilweise punktartig helle Zweige und wenig differenzierte Lichtwirkung.

Bei Walenstadt sinkt der maximale gemessene Kies-Nachladebereich von 41,37 auf 7,22 ms je Frame; auf der Murg-Strecke sinkt der maximale Fels-/Pflanzen-Nachladebereich von 28,17 auf 15,84 ms. Die jeweils längsten Gesamtframes liegen bei 34,38 statt 77,96 ms beziehungsweise 25,70 statt 47,72 ms. Alle Ausreisser bleiben in den [vollständigen Leistungsergebnissen](PERFORMANCE.md#windowsstreambuild-aufbauarbeit-aufteilen) enthalten.

Diese Einzelvergleiche sind keine Zusage dauerhaft ruckelfreier Darstellung: Die Rechnerlast schwankt, und die rotierenden Leistungsansichten sind nicht bildidentisch. Im Walenstadt-Abschlussbild verdeckt der eigene Avatar grosse Teile des neuen Laufs; die Kontrollansicht blickt in eine andere Richtung. Das ist ein offener FPV-Befund, keine visuelle Abnahme. Die festen Vergleichsansichten oben sind davon getrennt. Die Murg-Strecke endet nach rund 144,83 m bei 24,6 m Tiefe in freiem Wasser. Der bekannte fehlende CelestialVault-Sternmaterialeintrag bleibt im Bericht sichtbar; Nacht, Mindesthardware, Langzeitstabilität und vollständige manuelle Menübedienung sind nicht freigegeben.

## Ablauf und Grenzen

Kieszellen berechnen im normalen Nachladepfad höchstens 1024 Kandidaten je Aufruf. Nach der Berechnung werden ihre drei Instanzkomponenten einzeln registriert. Erst die vollständig aufgebaute Zelle wird sichtbar. Eine gewöhnliche Grenzüberschreitung mit drei fehlenden Zellen benötigt damit 33 Aufrufe statt drei vollständiger Zellaufbauten. Innerhalb von 15 m, beim Einstieg und bei grossen Ortswechseln wird benötigte Geometrie sofort fertiggestellt. Die Begrenzung ist ein Arbeitsbudget für den normalen Pfad, kein garantiertes Zeitlimit.

Fels-/Pflanzenzellen bereiten ihre Transformationen in einem Aufruf vor und registrieren danach je eine ihrer acht Komponenten pro Aufruf. Drei fehlende Zellen benötigen 27 Aufrufe. Die vorhandene 80-m-Nachbarschaft für Fische und Kollision bleibt sofort vollständig. Der räumliche Felsindex beschreibt weiterhin das gesamte Zielgebiet, auch wenn entfernte sichtbare Komponenten noch entstehen. Die Transformationsvorbereitung selbst bleibt ein zusammenhängender Arbeitsschritt.

Pro System existiert höchstens eine angefangene Zelle. Höchstens zwei Kies- beziehungsweise sieben Fels-/Pflanzenkomponenten sind fertig registriert, aber noch verborgen. Verlässt eine angefangene Zelle das Zielgebiet, werden ihre Daten verworfen und registrierte Komponenten der begrenzten Abbauwarteschlange übergeben. Das bestehende Fenster bleibt bei höchstens neun vollständigen Zellen. Die Abbauwarteschlangen bleiben auf 27 beziehungsweise 72 Komponenten begrenzt. Nahe Ortswechsel dürfen Arbeit sofort abschliessen und alte Körper sofort entfernen.

## Prüfverfahren

`Tools/TestStreamingBuild.ps1 -Packaged` prüft das Paket `Builds/WindowsStreamBuild`: Weesen mit sofortigem und schrittweisem Aufbau, Murg, Walenstadt, den Uferweg bei Weesen, die Nachladestrecke und Sediment. Die Laufzeitprüfungen vergleichen beibehaltene und wiederhergestellte Transformationen (Felsen/Pflanzen ohne Toleranz, Kies mit `0.00001`), prüfen die Aufrufbudgets, verborgene Teilkomponenten, wiederholte Richtungswechsel, Abbruch nach erster Registrierung, nahe Ortswechsel und die begrenzten Zwischenspeicher. Bestehende Kontakt-, Fisch- und Sedimentprüfungen bleiben Teil des Durchlaufs.

Die Kontrollschalter `-DiveImmediateGravelBuild` und `-DiveImmediatePopulationBuild` gelten nur in Diagnoseläufen. Sie stellen den vollständigen Aufbau einer entfernten Zelle je Aufruf her. Anders als frühere Kontrollen ohne Ablagerungen verwenden sie dieselbe Bodenform und Objektverteilung. Die PowerShell-Werkzeuge bieten entsprechende Schalter ohne das Präfix `Dive`.

`Tools/ReportStreamingBuild.py` bindet alle Aufnahmen und Messungen an Quelldateien, EXE und Inhaltscontainer. Die separaten CSV-Zeitbereiche für Vorbereitung, Registrierung und Gesamt-Nachladen dürfen wegen ihrer Verschachtelung nicht addiert werden. Aggregate CPU-Momentaufnahmen vor und nach einem Lauf dokumentieren zusätzliche Rechnerlast; sie messen weder die Last jedes einzelnen Frames noch deren Ursache.

## Offene Qualitätsziele

Die vollständige Unterwasserreferenz ist weiterhin nicht erreicht. Insbesondere freie Bodenflächen, Pflanzenwirkung, Licht und räumliche Komposition benötigen weitere Arbeit. Danach folgen die Überwasserlandschaft und die bereits gesicherte [Menü-/HUD-/FPV-Anforderung](UI_UX_TARGET.md). Die schrittweise Erzeugung ersetzt weder die visuelle Abnahme noch Langzeit- oder Mindesthardwaremessungen.
