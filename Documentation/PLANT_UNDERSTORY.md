# Niedriger Bewuchs zwischen den höheren Trieben

Die aktuelle Grafikvorschau startet über `Builds/WindowsPlantCover/Start-WaterPreview.cmd`. Sie enthält den zusätzlichen niedrigen Bewuchs, sparsamere Algengeometrie, höhenabhängige Pflanzenbewegung und eine sichtweitenabhängige Darstellungsgrenze. Zwölf Paketfälle mit 45 Aufnahmen und der automatische Haupt-EXE-Start bestehen. Der 60-Sekunden-Lauf bei Weesen erreicht 68,48 FPS im Mittel; die vollständige Referenzqualität und durchgehend 60 FPS sind weiterhin nicht erreicht. [Paket, Quellen, Modelle, Bilder und Messungen](PLANT_COVER_RESULTS.json).

Die bisherigen Pflanzengruppen erhalten eine zweite, niedrige Schicht aus Armleuchteralgen. Sie verbindet Teile der einzelnen Gruppen zu dichteren Inseln, während freie Sedimentflächen erhalten bleiben. Die bestehenden sechs Kandidaten für höhere Pflanzen werden zuerst mit derselben Zufallsfolge erzeugt. Danach folgen höchstens zwölf zusätzliche Kandidaten im selben Radius von 1,7 Metern. Bei geringerer örtlicher Bedeckung werden mehr davon ausgelassen.

Die zusätzliche Schicht verwendet Armleuchteralgen mit etwa 8–22 Zentimetern Höhe und variierender Breite. Bodengeometrie und Felsverteilung bleiben erhalten. Nach dem ersten Verteilungsvergleich wurden die feinen Algenzweige und die höhenabhängige Bewegung zusätzlich überarbeitet, wie unten beschrieben. Es ist eine gestaltete Verteilung, keine vermessene Vegetationskarte des Walensees. Die fein verzweigten Formen bleiben in grösserer Entfernung noch dünn und benötigen weitere gestalterische Arbeit.

Die Lebensraumbedingungen für Armleuchteralgen gelten auch für diese Schicht: zulässige Tiefe, Sichtweite und Bodenneigung. Wurzeln liegen zwei Zentimeter über dem abgetasteten Grund; Felsgrundflächen erhalten zehn Zentimeter Abstand, Einstiege 3,5 Meter. Innerhalb eines Gruppenankers halten alle akzeptierten Pflanzenwurzeln mindestens 35 Zentimeter Abstand. Zwischen verschiedenen Ankern gibt es weiterhin keine allgemeine Mindestabstandsprüfung. Der vollständige Gruppenrand wird vor der Zellzuordnung ausgewertet, damit Nachladen die Verteilung nicht verändert.

## Vergleich und Prüfung

Der native Lauf `artifacts/plant-understory-20261127-052706` besteht zwei Fälle mit sechs Bildern. An der festen Weesener Kamera wächst die Anzahl im geladenen Populationsfenster von 2.133 auf 4.810. Alle 2.677 zusätzlichen Pflanzen gehören zur niedrigen Schicht; die Zahl hoher Pflanzen bleibt 292. Diese Zahlen beschreiben geladene Instanzen, nicht die im Bild sichtbare Anzahl. Die drei Kameraposen und die Prüfsumme sämtlicher Felstransformationen sind vor und nach der Änderung gleich.

Die Laufzeitprüfungen kontrollieren Bodenkontakt, Lebensraum, Einstiegs- und Felsabstände, eindeutige Wurzeln, Zellzuordnung sowie die tatsächlich gespeicherten Instanztransformationen. Eine zweite Erzeugung mit vollständigen benachbarten Felslisten prüft die im Spiel gekürzte Felsvorauswahl. Nachladen und Rückkehr werden ebenfalls geprüft. Die bestehende automatische Prüfung enthält zudem Felsen, Kies, Fischbewegung und die gestaffelte Entfernung alter Populationskomponenten.

Der [native Bericht](PLANT_UNDERSTORY_NATIVE_RESULTS.json) hält Quellen, Assets und Bilder über Prüfsummen zusammen. Die allgemeinen Editor-Startfehler des ToolsetRegistry-Python-Moduls und die `LogAutomationTest`-Meldungen treten bereits im unveränderten Vergleichszustand auf und bleiben im Bericht erhalten; die erfolgreiche Spielprüfung wird nicht als fehlerfreier Editorstart ausgegeben.

## Erster Paketentwurf und Leistung

Der erste Entwurf in `WindowsPlantUnderstory` besteht neun Paketfälle mit 36 Aufnahmen und den automatischen Haupt-EXE-Start. Die Weesen-Messung zeigt jedoch zu viel zusätzlichen Grafikaufwand: 64,33 statt 69,93 FPS im Mittel, 52,12 statt 60,69 FPS im 1%-Low. Die mittlere GPU-Zeit steigt von 13,67 auf 14,90 ms; die Spielthread-Zeit bleibt mit 5,15 gegenüber 5,18 ms nahezu gleich. Schatten, Basisflächen und Bewegungsdaten tragen zum zusätzlichen Aufwand bei. Diese Einzelläufe verwenden dieselbe EXE, dieselben Inhalte und 1080p/Hoch mit Wasserprofil, 100 Prozent Renderauflösung, ohne VSync/Limit und ohne weitere Unreal-/Spielinstanz.

Walenstadt erreicht 72,13 FPS, die 90-Sekunden-Murg-Strecke 96,92 FPS bei höchstens 28,89 ms. Kein Messlauf enthält Bilder über 33,33 ms; das ersetzt keine durchgängige 60-FPS-Abnahme. [Erster Paket- und Leistungsbericht](PLANT_UNDERSTORY_INITIAL_RESULTS.json). Der bestehende fehlende CelestialVault-Sternmaterialverweis ist weiterhin im Paketprotokoll enthalten.

Dieser erste Entwurf wurde wegen des zusätzlichen Grafikaufwands weiter optimiert und durch `WindowsPlantCover` als aktuelle Vorschau abgelöst. Das ursprüngliche Paket und seine Messergebnisse bleiben als Vergleich erhalten.

Originalaufnahmen aus dem neuen Paket, dieselbe Weesener Kamera:

![Bisherige Pflanzengruppen](Images/PlantUnderstory_Before.png)

![Zusätzlicher niedriger Bewuchs](Images/PlantUnderstory_After.png)

## Sichtabhängige Darstellungsgrenze

Die Optimierung begrenzt die Darstellung aller Unterwasserpflanzen auf das Fünffache der eingestellten Sichtweite, mindestens jedoch 40 Meter. Bei 14 Metern Sichtweite ergibt das 70 Meter, bei 40 Metern Sichtweite 200 Meter. Es handelt sich um eine entfernte Abschaltgrenze, nicht um eine neu hinzugefügte Material-Ausblendung. Die Pflanzen bleiben im geladenen ökologischen Zustand erhalten; die Grenze verändert weder Platzierung noch Artenwahl. Bereits vorhandene Komponenten erhalten bei einer geänderten Umgebungskonfiguration die neue Grenze, nachgeladene Komponenten verwenden sie beim Erzeugen.

`-DiveUnlimitedPlants` stellt nur für Diagnose-/Messläufe die unbegrenzte Darstellung wieder her. Fünf native Fälle mit 15 Aufnahmen bestehen; zusätzliche feste Bildpaare bei 14 und 40 Metern Sichtweite vergleichen beide Zustände mit gleicher Population und Kamera. In den geprüften Bildern fehlt kein erkennbarer naher Bewuchs; die mittleren RGB-Abweichungen von 0,29–0,51 auf der 0–255-Skala enthalten auch laufende Pflanzenbewegung und zeitliche Bildrekonstruktion. [Nativer Bericht](PLANT_UNDERSTORY_CULLING_NATIVE_RESULTS.json).

Zwei kurze native 15-Sekunden-Vorvergleiche nach jeweils 15 Sekunden Aufwärmen erreichen 66,98 FPS ohne Grenze und 67,32 FPS mit Grenze (`artifacts/perf-underwater-1080p-q2-20261127-055435-814` und `artifacts/perf-underwater-1080p-q2-20261127-055527-405`). Dieser geringe Unterschied ist kein belastbarer Leistungsgewinn und keine Paketabnahme. Der nahe Bewuchs muss weiter optimiert werden; die Grenze allein löst seinen zusätzlichen Aufwand nicht. `Tools/Benchmark.ps1 -EditorSandbox` bindet solche nativen Vorprüfungen jetzt an die tatsächlich verwendete Spiel-DLL und hält sie von Paketmessungen getrennt.

## Feinere Abstimmung von Geometrie und Bewegung

Die millimeterdünnen Hauptstrahlen der Armleuchteralgen verwenden nun zwei statt vier Längssegmente und drei statt fünf weich schattierte Seiten. Ansatzpunkte, Spitzen, Bogenparameter und kleine Seitenverzweigungen bleiben erhalten. Die Modellhöhe bleibt 0,4395 Meter. Die höchste Detailstufe sinkt von 48.366 auf 29.262 Dreiecke; die weiteren Stufen besitzen 13.168, 4.682 und 1.610 Dreiecke. Laichkraut und Tausendblatt behalten ihre Geometrie und Dreieckzahlen. Beide UV-Kanäle und die Farb-/Bewegungsgewichte bestehen den Import.

Der seitliche Bewegungsausschlag wird jetzt mit dem tatsächlichen Höhenfaktor der Instanz skaliert. Dadurch bewegt sich eine auf 20 Prozent Höhe verkleinerte Pflanze höchstens etwa 1,8 statt neun Zentimeter in der stärkeren Bewegungsrichtung; die Wurzeln bleiben verankert. Das behebt den zuvor gleich grossen Ausschlag sehr kleiner und grosser Pflanzen. Es bleibt eine gestaltete Animation, keine hydrodynamische Berechnung.

Je zwölf native Modellansichten vor und nach der Änderung bestehen. Die Nahansicht erhält die verzweigte Silhouette. Ein kurzer nativer 15-Sekunden-Lauf erreicht 68,47 FPS (`artifacts/perf-underwater-1080p-q2-20261127-060904-250`), gegenüber 67,32 FPS vor dieser Modell-/Bewegungsänderung mit derselben Entfernungsgrenze. Diese kurze Einzelmessung ist nur ein Vorvergleich; massgeblich sind die folgenden längeren Paketmessungen.

Originale der Algen-Nahansicht: [vorher](Images/PlantBranches_Before.png) und [nachher](Images/PlantBranches_After.png). Beide Bilder zeigen den detaillierten Modellstand ihrer jeweiligen Revision; die alten primitiven Vergleichsmodelle werden hierfür nicht verwendet.

## Abschliessendes Paket und Leistung

`artifacts/plant-understory-20261127-061541` besteht zwölf Fälle mit 45 Aufnahmen: ohne/mit niedriger Schicht, dieselbe Szene ohne Darstellungsgrenze, klares Wasser mit/ohne Grenze, Weesen, Murg, Walenstadt/Ost, ohne Nanite, Nachladen, Uferablauf und Sediment. Der normale Haupt-EXE-Start mit Wasserprofil besteht ebenfalls. Drei direkte Wasserstarts und der Weesener Uferablauf werden geprüft; das ist keine Prüfung aller drei Uferzugänge oder der vollständigen manuellen Menüführung. [Ortszuordnung](TEST_ENTRY_LOCATIONS.md).

Die finalen Bildpaare vergleichen die zusätzliche niedrige Schicht bei gleichem optimiertem Modell- und Materialstand:

![Ohne zusätzliche niedrige Schicht](Images/PlantCover_Before.png)

![Mit zusätzlicher niedriger Schicht](Images/PlantCover_After.png)

RTX 3090 / Ryzen 9 9950X3D, 1920×1080 bei 100 Prozent Renderauflösung, Hoch mit Wasserprofil, ohne VSync/Limit, jeweils 15 Sekunden Aufwärmen und keine weitere Unreal-/Spielinstanz. Alle vier folgenden Messungen verwenden dieselbe EXE und dieselben Inhaltscontainer wie die zwölf Paketfälle. Die Kontrolle verwendet ebenfalls die optimierten Modelle und den niedrigen Bewuchs; sie deaktiviert nur die Darstellungsgrenze.

| Lauf | Mittel | 1%-Low | P95 | Maximum | Bilder >33,33 ms | Spielthread im Mittel |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Weesen/West, normal, 60 s | 68,48 FPS | 59,85 FPS | 15,86 ms | 22,56 ms | 0 / 4107 | 5,33 ms |
| Weesen/West, unbegrenzte Darstellung, 60 s | 66,90 FPS | 56,45 FPS | 16,61 ms | 18,35 ms | 0 / 4012 | 5,24 ms |
| Walenstadt/Ost, normal, 60 s | 73,85 FPS | 58,18 FPS | 15,50 ms | 18,28 ms | 0 / 4429 | 6,39 ms |
| Murg/Süd, Nachladestrecke, 90 s | 96,45 FPS | 71,33 FPS | 11,30 ms | 30,18 ms | 0 / 8679 | 4,59 ms |

Bei Weesen sinkt die mittlere GPU-Zeit in diesen Kontrollläufen von 14,30 auf 13,96 ms. Walenstadt erreicht 12,90 ms, die Murg-Strecke 9,72 ms. Zwölf normale Weesen-Bilder, 54 in Walenstadt und 14 auf der Strecke überschreiten 16,67 ms. Diese Einzelläufe belegen weder durchgehend 60 FPS noch eine allgemeine Beschleunigung. Der normale Weesen-Lauf enthält trotz besserem Mittel einen höheren Einzel-Ausreisser als die Kontrolle; er bleibt in der Auswertung enthalten. Gegenüber dem ersten dichten Entwurf mit 64,33 FPS ist der Rückgang weitgehend aufgeholt, gegenüber der ursprünglichen Kontrolle ohne niedrigen Bewuchs mit 69,93 FPS verbleibt Aufwand. Diese beiden älteren Vergleiche verwenden einen anderen Paketstand.

Die Messaufnahmen zeigen Grund und Felsen bei 5,3 m (Weesen) und 4,4 m (Walenstadt); die rund 145 Meter lange Strecke endet bei 24,6 m in freiem Wasser. Der bekannte fehlende CelestialVault-Sternmaterialverweis bleibt im Paketprotokoll erhalten. Ein aktueller Langzeittest, Mindesthardware und die visuelle Gesamtqualität sind weiterhin offen.

Reproduktion: `Tools/TestPlantUnderstory.ps1 -Packaged -CullingCompare -PackageDirectory Builds/WindowsPlantCover`, anschliessend `Tools/TestPackageLauncher.ps1` und die vier im [Prüfbericht](PLANT_COVER_RESULTS.json) gebundenen Messungen. `Tools/ReportPlantUnderstory.py` prüft auch die Modellquellen, Importsummen, Vorher-/Nachherbilder und unveränderten Felszustände innerhalb der Vergleichspaare.

## Offenes Qualitätsziel

Die dichteren Inseln sind ein weiterer Schritt zur [Unterwasserreferenz](VISUAL_TARGET.md). Feine Zweige wirken in manchen Spieleransichten noch punktartig und hell. Natürliche Pflanzenoberflächen, Bewuchs direkt auf Fels, Sedimentübergänge und die gemeinsame Lichtwirkung bleiben offen. Dieser Schritt ist keine vollständige visuelle Abnahme. Anschliessend folgen die Überwasserlandschaft und erst danach [Menü, HUD und FPV](UI_UX_TARGET.md).
