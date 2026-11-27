# Niedriger Bewuchs zwischen den höheren Trieben

Die bisherigen Pflanzengruppen erhalten eine zweite, niedrige Schicht aus Armleuchteralgen. Sie verbindet Teile der einzelnen Gruppen zu dichteren Inseln, während freie Sedimentflächen erhalten bleiben. Die bestehenden sechs Kandidaten für höhere Pflanzen werden zuerst mit derselben Zufallsfolge erzeugt. Danach folgen höchstens zwölf zusätzliche Kandidaten im selben Radius von 1,7 Metern. Bei geringerer örtlicher Bedeckung werden mehr davon ausgelassen.

Die zusätzliche Schicht verwendet das vorhandene Armleuchteralgen-Modell, mit etwa 8–22 Zentimetern Höhe und variierender Breite. Modelle, Materialien, Bodengeometrie und Felsverteilung ändern sich nicht. Es ist eine gestaltete Verteilung, keine vermessene Vegetationskarte des Walensees. Die fein verzweigten Formen bleiben in grösserer Entfernung noch dünn und benötigen weitere gestalterische Arbeit.

Die Lebensraumbedingungen für Armleuchteralgen gelten auch für diese Schicht: zulässige Tiefe, Sichtweite und Bodenneigung. Wurzeln liegen zwei Zentimeter über dem abgetasteten Grund; Felsgrundflächen erhalten zehn Zentimeter Abstand, Einstiege 3,5 Meter. Innerhalb eines Gruppenankers halten alle akzeptierten Pflanzenwurzeln mindestens 35 Zentimeter Abstand. Zwischen verschiedenen Ankern gibt es weiterhin keine allgemeine Mindestabstandsprüfung. Der vollständige Gruppenrand wird vor der Zellzuordnung ausgewertet, damit Nachladen die Verteilung nicht verändert.

## Vergleich und Prüfung

Der native Lauf `artifacts/plant-understory-20261127-052706` besteht zwei Fälle mit sechs Bildern. An der festen Weesener Kamera wächst die Anzahl im geladenen Populationsfenster von 2.133 auf 4.810. Alle 2.677 zusätzlichen Pflanzen gehören zur niedrigen Schicht; die Zahl hoher Pflanzen bleibt 292. Diese Zahlen beschreiben geladene Instanzen, nicht die im Bild sichtbare Anzahl. Die drei Kameraposen und die Prüfsumme sämtlicher Felstransformationen sind vor und nach der Änderung gleich.

Die Laufzeitprüfungen kontrollieren Bodenkontakt, Lebensraum, Einstiegs- und Felsabstände, eindeutige Wurzeln, Zellzuordnung sowie die tatsächlich gespeicherten Instanztransformationen. Eine zweite Erzeugung mit vollständigen benachbarten Felslisten prüft die im Spiel gekürzte Felsvorauswahl. Nachladen und Rückkehr werden ebenfalls geprüft. Die bestehende automatische Prüfung enthält zudem Felsen, Kies, Fischbewegung und die gestaffelte Entfernung alter Populationskomponenten.

Der [native Bericht](PLANT_UNDERSTORY_NATIVE_RESULTS.json) hält Quellen, Assets und Bilder über Prüfsummen zusammen. Die allgemeinen Editor-Startfehler des ToolsetRegistry-Python-Moduls und die `LogAutomationTest`-Meldungen treten bereits im unveränderten Vergleichszustand auf und bleiben im Bericht erhalten; die erfolgreiche Spielprüfung wird nicht als fehlerfreier Editorstart ausgegeben.

## Spielbares Paket

Der erste Entwurf in `WindowsPlantUnderstory` besteht neun Paketfälle mit 36 Aufnahmen und den automatischen Haupt-EXE-Start. Die Weesen-Messung zeigt jedoch zu viel zusätzlichen Grafikaufwand: 64,33 statt 69,93 FPS im Mittel, 52,12 statt 60,69 FPS im 1%-Low. Die mittlere GPU-Zeit steigt von 13,67 auf 14,90 ms; die Spielthread-Zeit bleibt mit 5,15 gegenüber 5,18 ms nahezu gleich. Schatten, Basisflächen und Bewegungsdaten tragen zum zusätzlichen Aufwand bei. Diese Einzelläufe verwenden dieselbe EXE, dieselben Inhalte und 1080p/Hoch mit Wasserprofil, 100 Prozent Renderauflösung, ohne VSync/Limit und ohne weitere Unreal-/Spielinstanz.

Walenstadt erreicht 72,13 FPS, die 90-Sekunden-Murg-Strecke 96,92 FPS bei höchstens 28,89 ms. Kein Messlauf enthält Bilder über 33,33 ms; das ersetzt keine durchgängige 60-FPS-Abnahme. [Erster Paket- und Leistungsbericht](PLANT_UNDERSTORY_INITIAL_RESULTS.json). Der bestehende fehlende CelestialVault-Sternmaterialverweis ist weiterhin im Paketprotokoll enthalten.

Dieser Entwurf wird vor der Empfehlung als neue Grafikvorschau weiter optimiert. Bis dahin bleibt `WindowsTalus` die empfohlene Vorschau.

Originalaufnahmen aus dem neuen Paket, dieselbe Weesener Kamera:

![Bisherige Pflanzengruppen](Images/PlantUnderstory_Before.png)

![Zusätzlicher niedriger Bewuchs](Images/PlantUnderstory_After.png)

## Offenes Qualitätsziel

Die dichteren Inseln sind ein weiterer Schritt zur [Unterwasserreferenz](VISUAL_TARGET.md). Natürliche Pflanzenoberflächen, Bewuchs direkt auf Fels, Sedimentübergänge und die gemeinsame Lichtwirkung bleiben offen. Dieser Schritt ist keine vollständige visuelle Abnahme. Anschliessend folgen die Überwasserlandschaft und erst danach [Menü, HUD und FPV](UI_UX_TARGET.md).
