# Unterwasserwelt: Arbeit am visuellen Ziel

Maßstab ist die [vom Nutzer bereitgestellte Qualitätsreferenz](VISUAL_TARGET.md), nicht die Abnahme einzelner Modelle. Der aktuelle Stand erreicht diese Gesamtreferenz noch nicht.

Eine [zusätzliche geometrische Sedimentschicht](SEDIMENT_RELIEF_INVESTIGATION.md) wurde bei Weesen erprobt und nicht übernommen: Trotz bestandener Kontaktprüfungen bleiben die flächigen Bodenwellen gestalterisch zu wenig gezielt; zugleich verschiebt die neue Höhenabfrage die Felsen. Die weitere Arbeit soll Ablagerungen an bestehende Felsfüsse anbinden und deren Formen erhalten. Die Spielvorschau bleibt beim geprüften Felsausrichtungsstand.

Die [überarbeitete Felsausrichtung](ROCK_POSE.md) verwendet weniger abgeflachte, unterschiedlich geneigte Blöcke. Echte Konturen und Kontaktflächen ändern sich, während Positionen, Anzahl und konservative Hindernishüllen erhalten bleiben. Neun Paketfälle mit 36 Bildern, zusätzliche Einbettungsproben und der Launcher-Test bestehen. Die Unterwassermessung erreicht bei 1080p/Hoch mit Wasserprofil 69,93 FPS im Mittel. Sedimentformen, Bodenanschlüsse, Pflanzen und Lichtwirkung bleiben weiter in Arbeit.

Der [Versuch mit Scan-Höhenkarten und Parallax am Seegrund](BED_RELIEF_INVESTIGATION.md) wurde nicht in die Spielvorschau übernommen: Die Materialdetails ändern sich, die großräumig flache Bodenwirkung bleibt jedoch erhalten. Zwei native Vergleichsläufe zeigen zusätzlich rund 0,27 ms GPU-Aufwand. Quellen, Patch und Bilder bleiben dokumentiert; die nächste visuelle Arbeit muss tatsächliche Bodenformen und Übergänge in der vollständigen Szene betreffen.

Die [Spiegelungen an der Wasserunterseite](WATER_REFLECTIONS.md) sind im Grafikprototyp schärfer. Eine Hardware-Raytracing-Kontrolle mit Beleuchtung am Trefferpunkt ergänzt die bislang fehlenden Bodenoberflächen. Die falsche flache Wassergeometrie wird dabei aus sekundären Strahlen entfernt; die beobachtete rechteckige Grenze verschwindet. Paketleistung, normaler Spielbetrieb und vollständiger reflektierter Wasserweg bleiben noch zu prüfen beziehungsweise umzusetzen.

Im [Prototyp des begrenzten Wasservolumens](BOUNDED_WATER.md#kameralicht-nach-den-material--und-pflanzenkorrekturen) entfällt jetzt ein zusätzlicher unverschatteter Kamera-Lichtanteil. Die kleine Farb-/Helligkeitskorrektur besteht zwölf native Szenenfälle mit 33 Aufnahmen, einschliesslich Nacht, Sturm und Ersatzdarstellung. Sie ist noch kein neuer normaler Spielpfad; dessen Paketstand bleibt unten unverändert. Vollständige spektrale Dämpfung und eine überzeugende Wasserbrechung bleiben offen.

Die anschliessende [Überarbeitung der Pflanzenformen](PLANT_GROWTH.md) ergänzt gebogene Triebe, dichtere obere Blattbereiche und zurückhaltendere Gewebefarben. Die feste Nahkamera zeigt weniger starre Silhouetten; automatische Detailreduktion, feine Blattdarstellung und Materialqualität bleiben begrenzt.

Die aktuelle [Pflanzenverteilung](PLANT_BEDS.md) bildet kleine Gruppen mit vorherrschender Art und niedrigeren Rändern. Sie respektiert Tiefe, Neigung, Felsen und Einstiegsbereiche und bleibt beim Nachladen stabil. Acht Paketfälle mit 31 Aufnahmen sowie der normale Spielstart bestehen. Pflanzenmodelle und Materialien behalten ihre sichtbaren Qualitätsgrenzen; die Gruppen allein erfüllen die Referenz nicht.

Aktuelle Materialarbeit: [passende Scandetails für jede Felsform](ROCK_SURFACES.md) ersetzen die gemeinsame Steinprojektion. Bruchkanten, Poren und Rauheit folgen dadurch den jeweiligen UVs. Die Gesamtszene und die Qualität der Übergänge bleiben weiter in Arbeit.

Die anschliessende [Korrektur der Quellfarb-Kodierung](BENTHIC_COLOR.md) entfernt die zu helle Interpretation der 16-Bit-Scans. Sand und Felsen erscheinen weniger ausgebleicht. Bodenvariation, Pflanzen und Lichtverteilung bleiben offen.

[Räumliche Bruchstücke am Felsfuss](ROCK_RUBBLE.md) ergänzen jetzt eine mittlere Grössenstufe zwischen grossen Blöcken und Kleinkies. Sie verwenden die vorhandenen Scanoberflächen, besitzen Kollision und werden von Pflanzenverteilung sowie Fischbewegung berücksichtigt. Die Vergleichsbilder zeigen mehr räumliche Übergänge; die vermessenen Geländehöhen bleiben erhalten. Die zusätzliche CPU-Arbeit und einzelne Ruckler sind in den [Paketmessungen](PERFORMANCE.md) dokumentiert. Die gesamte Referenzqualität bleibt offen.

Die anschliessende [räumliche Hindernissuche](ROCK_SEARCH.md) begrenzt Felsabfragen für die Fischbewegung auf nahe Kandidaten. Sie erhält den Grafikstand und schafft in der gemessenen Unterwasserszene CPU-Reserve: 3,40 statt rund 11,1 ms mittlere Spielthreadzeit. Kandidatenvollständigkeit, Wegschnittpunkte, numerische Kraftabweichungen und das Leeren/Wiederherstellen der Fenster werden im fertigen Paket geprüft. Die Gestaltung von Pflanzen, Sedimentübergängen und Licht bleibt die nächste visuelle Arbeit; die Optimierung allein erreicht die Bildreferenz nicht.

Aktuelle Entwicklung: [räumlich begrenztes Wassermedium](BOUNDED_WATER.md) zusammen mit der [getrennten Wasserunterseite](WATER_INTERFACE.md). Himmel und Sonne sind wieder durch die Oberfläche sichtbar. Das integrierte Streulicht wird nicht mehr nachträglich über die gesamte Hintergrundentfernung abgedunkelt; dadurch verschwindet der beobachtete schwarze Fernstreifen. Vollständige spektrale Lichtwege, Brechung, Reflexionsdetails und die Gesamtabstimmung bleiben offen. Die Varianten sind weiterhin auf Diagnoseläufe begrenzt. Die aktuelle Spielvorschau ist im README verlinkt; die experimentellen Wasserpfade sind weiterhin kein normaler Spielstandard.

## Geröllsäume am Felsfuß

Quellstand `5b58239` verkleinert die Ausschlusszone für kleine Bodensteine. Die bisher verwendete vollständige Hindernishülle umschließt auch den freiliegenden oberen Felsen; sie ist größer als dessen teilweise vergrabener Bodenanschluss. Dadurch entstanden sichtbar leere Ränder. Kleine Steine dürfen jetzt näher an die Oberfläche heranreichen. Einzelne bleiben im undurchsichtigen Fels verborgen; die Änderung berechnet keinen exakten Schnitt zwischen Mesh und Gelände. Die Kollisions- und Ausweichhüllen für Taucher und Fische behalten ihre bisherigen Größen.

Um größere Blöcke entstehen zusätzlich unregelmäßige, weich auslaufende Geröllsäume. Eine weltfeste Verteilung unterbricht die Säume, damit keine geschlossenen kreisförmigen Ringe entstehen. Zwischen 25 und 40 Metern Tiefe fällt der Zusatz aus. Die vorhandenen Sediment-/Kiesfelder bleiben erhalten; Größe, Materialien und Detailstufen der kleinen Steine ändern sich nicht. Weiterhin gelten neun 32-Meter-Zellen mit je höchstens 4.096 Kandidaten und maximal sechs Zentimetern Halbachse entlang der Bodennormalen. Gelände wird nicht angehoben. Die Verteilung ist gestaltet und keine Messung örtlicher Erosion oder Ablagerung.

Die neue Diagnose `-HabitatView -HabitatClose` wählt stabile Nahansichten realer Felsgruppen. `-NoRockAprons` stellt darin die bisherige Kies-Ausschlussregel wieder her. Nur Diagnoseläufe berücksichtigen diesen Schalter. Felsen, Kamera und Materialien stimmen in beiden Zuständen überein; die Prüfung vergleicht zusätzlich weiterhin die weiten Ansichten an allen drei Einstiegen. Der native Nahvergleich zeigt mehr kleine Steine zwischen Felsfuß und freiem Boden; in der weiten Ansicht ist die Änderung deutlich weniger auffällig.

Das Paket **`Builds/WindowsRockAprons/divingsim.exe`** enthält diesen Stand und besteht sieben Szenenfälle mit 25 Bildern sowie den normalen Launcher-Start: drei Habitat-/Nachladeprüfungen, zwei Zustände der Nahansicht, Nanite-Ersatzdarstellung und Sediment. Die Kontaktproben sowie Entladen/Wiederherstellen bestehen an allen drei Einstiegen. Der Zähler `edge_stones` erfasst Steine innerhalb der früheren großen Ausschlussradien; er ist kein Maß für tatsächlich sichtbare Steine. Der Sedimentlauf endet mit vier erzeugten und null verbleibenden Wolken. Die [CI-Quellprüfungen](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45864287319) sind erfolgreich.

Gleiche Nahkamera mit bisheriger Ausschlusszone:

![Felsfuß mit großer Kies-Ausschlusszone](Images/RockAprons_Before.png)

Gleiche Nahkamera mit Geröllsäumen:

![Zusätzliche kleine Steine am Felsfuß](Images/RockAprons_After.png)

Die Felsformen und Materialien sind in diesem Paar identisch. Bewegte Fische und Lichtmuster können sich zwischen den Läufen geringfügig unterscheiden. Materialdetails, Bewuchs, Lichtwirkung und die gesamte visuelle Referenzabnahme bleiben offen.

Der [gebundene Prüfbericht](ROCK_APRONS_RESULTS.json) enthält Quellstand, Paketcontainer, sieben Fälle, alle Vergleichsbilder und zwei Leistungsmessungen. Die Spiel-EXE hat SHA-256 `e70538614375e50d4111cbe0f4f3950252fdb8e23c331570de56dac69557722d`. Die Ausweichprüfung erfasst die jeweils aktiven Fischarten: vier in Weesen, fünf in Murg und fünf bei Lediwracks. Der Berichtsprüfer verlangt eine gültige lokale Artenzahl und mindestens einen vollständigen Fünf-Arten-Fall, statt fünf Arten an jedem Ort vorauszusetzen. Die sieben Sedimentbilder decken Start, Wolke, Sichtbarkeit, ausgeblendete Kontrolle, Absetzen, freien Zustand und die allgemeine Unterwasseraufnahme ab.

Bei 1080p/Hoch erreicht der 60-Sekunden-Unterwasserlauf 73,08 FPS im Mittel und 61,56 FPS im langsamsten Prozent, ohne Bild über 33,33 ms. Die 90-Sekunden-Murg-Strecke erreicht 100,75 / 80,49 FPS und enthält einen einzelnen Ruckler mit 37,04 ms; dieser Leistungsbefund bleibt offen. [Messbedingungen und Grenzen](PERFORMANCE.md). Reproduktion: `Tools/TestPopulationCache.ps1 -PackageDirectory Builds/WindowsRockAprons -RequireStaged -RockAprons`; der Bericht wird mit `Tools/ReportPopulationCache.py --staged --rock-aprons` und den im JSON gebundenen Laufverzeichnissen erzeugt.

## Zusammenhängende Felsgruppen

Die Platzierung aus Quellstand `1a18dfc` verbindet größere Blöcke zu kurzen Felsrücken. Eine kontinuierliche Verteilung erzeugt dichtere Bereiche und freiere Sedimentflächen. Die Hauptrichtung ändert sich langsam über die Umgebung und folgt an steileren Stellen der örtlichen Höhenlinie. Bis zu vier größere Blöcke je Gruppe überlappen teilweise; kleinere Felsen und Geröll bleiben am Rand lockerer verteilt. Das vergrößert die räumlichen Formen, ohne die vermessene Geländetopologie zu verändern. Es handelt sich um gestaltete Formationen, nicht um vermessene einzelne Felsvorkommen.

Die drei vorhandenen Felsmodelle und ihre Materialien bleiben erhalten. Jeder Block erhält Größe, Höhe und Drehung; die höchste horizontale Skalierung steigt von 2,4 auf 3,2. Das vorhandene Raster von 96-Meter-Zellen, weltfesten Zufallswerten und eindeutiger Zellzugehörigkeit bleibt bestehen. Ein einreihiger Nachbarbereich reicht für den maximalen Versatz von 8,5 Metern je Weltachse aus. Bestehende Einstiegs- und Zugangskorridore bleiben inklusive des vergrößerten Felsradius ausgespart. Pflanzen, Kies und Fischbewegung berücksichtigen dieselben aktualisierten Hindernisdaten.

Die erweiterten Laufzeitprüfungen kontrollieren eindeutige Positionen, Größenbegrenzung, Einstiegsschutz, unveränderte Wiederherstellung beim Nachladen und zusammenliegende größere Blöcke. Der letzte Punkt prüft Abstände konservativer Hüllen, nicht die Qualität sichtbarer Felsanschlüsse. Diese wird zusätzlich anhand identischer Kameras vor und nach der Änderung beurteilt. Sichtbar bleiben derzeit zu glatte Übergänge zum Sediment, wiedererkennbare Einzelformen und eine insgesamt noch nicht ausreichende Material- und Lichtqualität.

Alle neun festen Kamerapositionen an den drei Einstiegen stimmen im Vergleich mit `WindowsViewPath` überein. Die folgenden Bilder zeigen die dritte Lediwracks-Ansicht bei 14 Metern Sichtweite und Tageslicht.

![Vorherige einzelne Felsgruppen](Images/Formations_Before.png)

![Zusammenhängende größere Felsblöcke](Images/Formations_After.png)

Der erste Paketlauf (`WindowsFormations`, `1a18dfc`) besteht zehn Fälle, scheitert aber beim Start der Sedimentprüfung: Das Prüfszenario versetzt den Taucher erst nach der Aktualisierung der Wasserparameter. Die strenge Prüfung erkennt dadurch einen veralteten Augenabstand. Quellstand `9833ef4` legt diese Testversetzung vor die Kameraaktualisierung. Die Parameterprüfung bleibt unverändert. Der korrigierte native Lauf besteht Aufwirbeln, Pause und vollständiges Absetzen; er endet mit sechs erzeugten und null verbleibenden Sedimentwolken. Das erste Paket und sein Fehlerprotokoll bleiben erhalten.

Das korrigierte Paket **`Builds/WindowsFormationsFinal/divingsim.exe`** besteht zwölf Szenenfälle mit 60 Bildern und den normalen Launcher-Start. Es enthält Quellstand `9833ef4f86c9804d8c2ed2d9ef56b45fddbd4c7d`; die Spiel-EXE hat SHA-256 `b0d8104e6329c1c8c41d241224d608aa9be962e116f7776543dde8f8da940306`. Geprüft sind alle drei Habitatansichten und Schwimm-/Uferwege, die Nanite-Ersatzdarstellung, Sediment und Navigation. Die CI-Quellprüfungen auf Windows, macOS und Linux sind ebenfalls [erfolgreich](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45859950114).

Der [Ergebnisbericht](FORMATIONS_RESULTS.json) verbindet Quelldateien, Paketcontainer, zwölf Szenenfälle, identische Vergleichskameras, veröffentlichte Bilder und zwei Leistungsmessungen. Er erhält auch den fehlgeschlagenen ersten Lauf. Bei 1080p/Hoch erreicht der bodennahe 60-Sekunden-Lauf 72,91 FPS im Mittel und 61,79 FPS im langsamsten Prozent, ohne Bild über 33,33 ms. Der 90-Sekunden-Murg-Lauf erreicht 100,88 / 81,55 FPS, enthält aber einen Ruckler mit 34,36 ms. Er führt teilweise durch freies tieferes Wasser und belegt keine Leistung in durchgehend dichten Felsgruppen. [Messbedingungen und Grenzen](PERFORMANCE.md).

Reproduktion: `Tools/TestHabitat.ps1 -PackageDirectory Builds/WindowsFormationsFinal`; anschließend `Tools/ReportHabitat.py` mit `--formations`, `--baseline-run`, `--prior-run`, `--build`, `--output` und den beiden `--performance`-Verzeichnissen aus dem Ergebnisbericht. Die Szenenprüfung endet bei einem Fehler; ein späterer vollständiger Lauf besitzt ein eigenes Verzeichnis. Menü/HUD/FPV folgen weiterhin erst nach Unterwasser- und Überwassergrafik; die [gesicherten UI-Referenzen](UI_UX_TARGET.md) bleiben dafür verbindliche Designrichtung.

## Blickabhängige Wasserfarbe

Der neue Entwicklungsstand entfernt den zusätzlichen konstanten Grünfilter und die feste kanalweise Entsättigung der Tauchkamera. Die Wasserfarbe kommt aus entfernungsabhängiger Absorption und Streuung; die bestehende Entsättigung bei körperlicher Beeinträchtigung bleibt wirksam. Nahe Materialien behalten damit mehr ihrer eigenen Farbe, statt unabhängig vom Abstand immer gleich eingefärbt zu werden.

`Tools/Shaders/UnderwaterView.ush` verwendet die räumliche Strecke zwischen Kamera und Tiefenpufferpunkt. Zuvor diente die projizierte Szenentiefe als Streckenmaß, sodass seitliche Blickstrahlen einen zu kurzen Wasserweg erhielten. Unreal rekonstruiert die Position für das Postprocess-Material aus dem Tiefenpuffer; Welt- und Kameraposition werden vor der Umwandlung zum Shader-Eingang mit den LWC-Materialknoten subtrahiert. Bei einem Blick in die Luft endet die berücksichtigte Strecke am örtlichen Wasserstand. Große Sky-Tiefen werden vor der Längenberechnung skaliert, um einen Überlauf zu vermeiden.

Das bleibt eine homogene, gestaltete Mediennäherung. Wellenbrechung kommt weiterhin vom Wasseroberflächenmaterial, die Grenze der Postprocess-Strecke verwendet den Wasserstand am Auge. Ein vollständiger gebrochener Strahlengang, örtlich vermessene optische Koeffizienten und spektral getrennte direkte/indirekte Beleuchtung sind damit nicht erreicht. Das Kameramaterial färbt die Tauchlampe entlang des Sichtwegs; es legt keinen zusätzlichen Sonnenweg auf lokales Lampenlicht. Die bestehenden Werte für Sichtweite, Absorption, Streufarbe, Sonne und Nebel wurden in diesem Schritt nicht neu abgestimmt.

Die Laufzeitprüfung kontrolliert nun die tatsächlichen Sichtweiten-, Augentiefen- und Umgebungslichtparameter sowie das Materialgewicht an der Wasserlinie. Ein separater Vergleichskamerafehler ist ebenfalls korrigiert: Nach einer Kameraversetzung bei eingefrorenem Welttick wird das Medium aktualisiert. Pflanzen-/Fels-Nahansichten hatten teilweise noch die Nebeleinstellung vom Einstieg behalten. Historische Nahansichten sind deshalb keine Beleuchtungsabnahme dieses neuen Standes. Die Habitatansichten hatten bereits eine Aktualisierung pro Bild; diese findet jetzt zusätzlich nach der Positionierung statt.

Das eigenständige Paket `Builds/WindowsViewPath/divingsim.exe` aus Quellstand `f19bd3f` besteht 15 Szenenfälle mit 41 Bildern sowie eine kontrollierte Lichtmusterprüfung mit vier Bildern. Geprüft werden alle drei Habitatansichten/Einstiege, 4/14/40 Meter Sichtweite, Tag-/Nachtblick zur Oberfläche, Wasser/Luft-Übergänge an allen drei Einstiegen, Sturm und Lampenlicht. Der normale Launcher-Start besteht ebenfalls. Die Quellprüfungen für Windows, macOS und Linux sind [erfolgreich](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45852139404); dies ist kein nativer Unreal-Mac-Build.

Die Nachtprüfung unterscheidet jetzt Dämmerung und vollständige Nacht. Am 21. Juni um 00:00 CEST liegt die simulierte Sonne bei −16,877 Grad; nach Entfernen des konstanten Filters bleiben sehr schwache Konturen sichtbar. Die ursprünglich darauf angewandte strenge Dunkelheitsprüfung schlägt mit P99,9 = 7/255 fehl (vorheriges Paket: 2/255). Dieser Befund wird erhalten, nicht als fehlerfreie Schwarzaufnahme umgedeutet. Um 01:30 CEST steht die Sonne bei −19,447 Grad: Im geprüften Umgebungsbereich beträgt der maximale Farbkanal ohne Lampe durchgehend 0; mit Lampe überschreiten 257.442 Pixel den Wert 20/255. Die strenge Schwarzprüfung gilt für diesen konkreten Zustand, nicht pauschal für jede Nacht mit Mondlicht.

Die Lichtmusterprüfung hält Kamera, Licht und Uhrzeit gezielt fest. Auf der beleuchteten Empfangsfläche beträgt der mittlere Bildunterschied zwischen ein-/ausgeschaltetem Lichtmuster 4,33/255, im Schatten nur 0,38/255. Eine veränderte Wellenzeit bewegt das Muster (6,13/255), eine eingefrorene Zeit bleibt nahezu unverändert (0,44/255). Diese Bildstatistiken prüfen die Wirkung und Kopplung, nicht photometrische Genauigkeit.

Die Bildpaare zeigen dieselbe Kamera bei 14 Metern Sichtweite. Im nahen Felsausschnitt sinkt der feste Grünanteil relativ zu den eigenen Materialfarben; der Kontrastverlust in der Entfernung bleibt bestehen. Die separate klare/trübe Szene bestätigt stärkeren Detailverlust bei 4 Metern. Ein einzelnes günstiges Standbild gilt weiterhin nicht als Gesamtabnahme.

![Bisherige zusätzliche Kamerafärbung](Images/ViewPath_Before.png)

![Wasserfärbung entlang des Blickwegs](Images/ViewPath_After.png)

`Tools/TestViewPath.ps1` erzeugt die Szenen- und Lichtmusteraufnahmen; `Tools/ReportViewPath.py` verbindet Bildprüfung, Quelldateien, Paket und Leistungsmessung. [Prüfbericht](VIEW_PATH_RESULTS.json). Die älteren folgenden Ergebnisberichte beziehen sich jeweils auf ihre ausdrücklich benannten Pakete.

Der neue 60-Sekunden-Lauf erreicht bei 1080p/Hoch 67,92 FPS im Mittel und 54,37 FPS im langsamsten Prozent. Kein Bild überschreitet 33,33 ms, durchgehend 60 FPS werden jedoch weiterhin verfehlt. [Messbedingungen](PERFORMANCE.md). Die Referenzqualität, insbesondere zusammenhängende geologische Formen, Bewuchs, überzeugende Lichtstrahlen und vollständige Unterwasserbrechung, bleibt offen. Die bekannten scharfkantigen Lampenschatten und der alte CelestialVault-Sternmaterialverweis sind in diesem Schritt nicht behoben.

## Größenstufen des Seegrunds

Die drei großen Felsformen ergänzen bereits den bisherigen Grund. Eine weitere Größenstufe ist im Entwicklungsstand integriert: `create_gravel_assets.py` leitet aus denselben dokumentierten CC0-Formen drei geschlossene Steinmeshes mit je 1.200 Dreiecken ab. Die fertigen Formen werden nach der Vereinfachung erneut zentriert und auf einen bekannten Radius normiert. Originale und große Felsen bleiben erhalten. `GravelAssets.blend` und `blender/exports/Gravel/Manifest.json` halten die Ableitung fest.

`build_gravel_content.py` importiert sie separat mit vier Detailstufen, vollständiger Nanite-Ersatzgeometrie und je einer einfachen konvexen Kontakthülle. Ein eigenes Material nutzt die vorhandenen CC0-Kieskarten des Seegrunds. Die tatsächlichen Rasterstufen enthalten 1.200 / 420 / 120 / 64 Dreiecke; Unreal erhält bei der letzten Stufe mehr als die angeforderten 30 Dreiecke.

CPU-Platzierung und Bodenmaterial verwenden dieselbe räumliche Patch-Funktion mit ganzzahligem Hash. Das erzeugt unregelmäßige Kiesfelder und freiere Sedimentflächen ohne regelmäßiges Raster. Die Steine orientieren sich an der Vermessung, liegen teilweise im Boden und meiden große Felsen. Ihre horizontalen Halbachsen reichen bis 24,5 cm; die Halbachse entlang der Bodennormalen ist auf 6 cm begrenzt. Die Verteilung bleibt gestaltet und darf nicht als lokale Sedimentmessung bezeichnet werden.

Der Nahbereich umfasst höchstens neun Zellen von jeweils 32 m mit jeweils 4.096 Platzierungskandidaten und drei Instanzkomponenten. Zeichnung endet nach 30 m. Beim Schwimmen wird eine neue Zelle pro Bild aufgebaut; Einstiege, größere Ortswechsel und die feste Vergleichsansicht laden das vollständige Fenster. Beibehaltene Zellen behalten ihre Komponenten, erneut besuchte Zellen erhalten dieselben Transformationen.

## Bisherige Sichtprüfung und Grenzen

Der erste native Vergleich (`artifacts/gravel-first-comparison/`) zeigte zu wenige, auffällig helle Steine. Im zweiten Vergleich (`artifacts/gravel-refined-comparison/`) passen die verwendeten Kieskarten und die höhere Dichte besser zum Grund. Beide Vergleiche enthalten dieselben zwei Kameras mit ausgeblendeten und eingeblendeten Steinen. Das sind Aufnahmen aus dem Unreal-Entwicklungsstand; die bereits veröffentlichte Vorschau `WindowsRockDetail` enthält diese Änderung noch nicht.

Die abschließende native Prüfung umfasst fünf Fälle und elf Bilder: jeweils einen Streaminglauf für Weesen, Murg und Lediwracks sowie die zwei Kameras mit ausgeblendeten/eingeblendeten Steinen für Nanite und Rasterdarstellung. Alle fünf Laufprüfungen bestehen. Prüfsummen, Buildprotokolle und genaue Testmarker stehen in [GRAVEL_INTEGRATION_RESULTS.json](GRAVEL_INTEGRATION_RESULTS.json).

| Einstieg | Steine im Ausgangsfenster | erfolgreiche Kontaktproben |
| --- | ---: | ---: |
| Weesen | 5.208 | 192 |
| Murg | 5.941 | 196 |
| Lediwracks | 4.743 | 144 |

Jeder Rückwegtest erhält sechs bestehende Zellen unverändert und stellt neun entladene Zellen mit identischen Transformationen wieder her. Der Aufbau über mehrere Bilder wird ausdrücklich mit sieben, acht und neun aktiven Zellen geprüft. Die Verteilungstests prüfen Grenzen, Tiefenabnahme, freie und dichte Bereiche sowie kontinuierliche Übergänge. Die Editorstarts enthalten weiterhin den bereits im vorigen Fels-Test vorhandenen ToolsetRegistry-Pythonfehler und 17 frühe `Condition failed`-Meldungen. Diese sind im Bericht erhalten; die nativen Spielprüfungen sind keine Aussage über bestandene Engine-Selbsttests.

Entwicklungsansicht ohne neue Steine:

![Gleiche Kamera, Kiesgeometrie ausgeblendet](Images/Gravel_Hidden.png)

Entwicklungsansicht mit neuen Steinen – weiterhin deutlich unter der Gesamtqualität der Nutzerreferenz:

![Gleiche Kamera, Kiesgeometrie eingeblendet](Images/Gravel_Visible.png)

Die Pflanzenformen und -oberflächen wurden inzwischen weiterentwickelt. Das neue eigenständige Paket `WindowsUnderwaterDetail` enthält Felsen, Kies und die drei neuen Pflanzengruppen. Zwölf Paketprüfungen mit 74 Aufnahmen sowie der normale Spielstart bestehen; Details und Paketidentität stehen in [UNDERWATER_DETAIL_RESULTS.json](UNDERWATER_DETAIL_RESULTS.json). Echte Vorher-/Nachher-Aufnahmen sind unter [Unterwasserbewuchs](AQUATIC_PLANTS.md) dokumentiert.

Die visuelle Gesamtabnahme ist weiter offen: fehlende mittlere Geröllstrukturen, zu vereinzelt wirkender Bewuchs und insbesondere die gemeinsame Wirkung von Licht und Wasser. Die Referenzqualität ist deutlich noch nicht erreicht. Der [zweistündige Murg-Lauf](SOAK.md) betrifft das ältere unveränderte Paket `WindowsFishSteering`; er wird nicht als Dauerlaufnachweis für die neuen Modelle ausgegeben. Rechenzeiten aus parallel laufender Entwicklung sind keine isolierte Bildratenmessung.

## Licht und Wasseroberfläche

Die weitergehende Arbeit an Pflanzenformen und ihren Oberflächen ist in [Unterwasserbewuchs](AQUATIC_PLANTS.md) dokumentiert. Die bisherigen groben Blätter und die neue Integration bleiben in kontrollierten Spielansichten vergleichbar.

Die normale Aufnahme `artifacts/rock-validation-20261126-113456/stream0/DiveUnderwater_0_stream.png` zeigt eine sehr dunkle Wasseroberfläche bei geringer Tiefe. Das ist zunächst ein visueller Befund; Schattenlage, Belichtung, Volumen und Oberfläche sind getrennt zu untersuchen.

Ein konkreter technischer Ansatzpunkt ist der installierte UE-5.8-Renderpfad: `Engine/Shaders/Private/BasePassPixelShader.usf` setzt bei Single Layer Water `CameraIsUnderWater = false` (Zeile 1698); `SingleLayerWaterComposite.usf` tut dies ebenfalls (Zeile 68). `SingleLayerWaterShading.ush` berechnet mit diesem Zustand die optische Weglänge hinter der Oberfläche statt zwischen Unterwasserkamera und Oberfläche. Unser bisheriges zweiseitiges Single-Layer-Water-Material verwendet auch für die Unteransicht diesen Pfad. Eine korrekte Unteransicht ist damit nicht allein durch die vorhandenen Materialparameter belegt.

Der kontrollierte Vergleich vom 26. November isoliert zwei getrennte Probleme. Die erste Testkamera stand genau auf den XY-Koordinaten der Einstiegsboje und damit innerhalb ihrer zweiseitig sichtbaren Leine. Die nahezu einfarbigen Aufnahmen aus den ersten `artifacts/water-diagnostic-*`-Versuchen und den anschließenden Abschaltversuchen sind deshalb **kein Nachweis für einen Shaderfehler**. Erst der Vergleich aus freiem, vermessenem Wasser ist dafür geeignet. Boje und Leine liegen nun zwei Meter neben dem unveränderten Tauchstart; der Untergrund wird dort geprüft. Das verhindert dieselbe Verdeckung beim direkten Spieleinstieg. Die Vergleichskamera hält zusätzlich Abstand zu den tatsächlichen Leinenpositionen und protokolliert Kamera, Seegrund und aktive Nachbearbeitung.

Mit freier Kamera zeigte sich: Bei unverändertem Sonnenstand, Sichtweite, Belichtung, Wellen und Kamera-Nachbearbeitung verschwindet die starke falsche Abdunklung nach oben, wenn Single Layer Water unterhalb der Oberfläche keine Wasserabsorption für den dahinterliegenden Luftraum mehr anwendet. `UpdateAtmosphere` setzt deshalb `OpticalScale = 14 / Sichtweite × (1 − Kamerawasseranteil)`. Der Kamerawasseranteil verwendet den bestehenden glatten Übergang über 0,5 Meter. Über Wasser bleibt die bisherige Volumenwirkung erhalten; die Unterwasser-Nachbearbeitung für den Weg zwischen Kamera und Oberfläche bleibt aktiv. Globale Helligkeit, Nebeldichte und Himmelshöhe wurden dafür nicht verändert. Die vorübergehenden Shader-Bypass- und Atmosphärenhöhen-Experimente wurden entfernt.

`Tools/SmokeTest.ps1 -Scenario rocks -WaterView -WaterStart 0 -WaterEnd 1` erzeugt sechs Vergleichsbilder: Zustand 0 erzwingt das frühere Oberflächenvolumen, Zustand 1 deaktiviert es bei vollständig eingetauchter Kamera. Je Zustand werden zwei aufwärts gerichtete Unterwasserblicke und ein Blick über Wasser aufgenommen. Die erweiterten Diagnosezustände 2–7 kombinieren Luftnebel und abgeschaltete Kameraabsorption; 8/9 sind Kontrollen ohne Nebel und Kameraabsorption, mit ausgeblendeter/eingeblendeter Wasserfläche. Diese Abschaltungen gelten ausschließlich im ausdrücklich gestarteten Vergleichstest. Ein bestandener Testmarker bestätigt Ablauf und frische Aufnahmen, nicht die Erfüllung des visuellen Qualitätsziels.

Die Korrektur ist ein begrenzter Fortschritt bei der Unteransicht, kein vollständiger physikalischer Unterwasser-Renderer. Insbesondere korrekte winkelabhängige Brechung, Totalreflexion bzw. Snell-Fenster und die räumliche Wirkung von Licht, Trübung und Bewuchs bleiben weiterzuentwickeln. Das geforderte Ergebnis umfasst einen nachvollziehbaren Blick durch die Oberfläche, dieselbe Wellengeometrie und einen konsistenten Übergang beim Auftauchen. Nacht, größere Tiefe und trübe Bedingungen bleiben Teil der Abnahme. Engine-Dateien wurden nur gelesen und nicht verändert.

Das Paket **`Builds/WindowsWaterOptics/divingsim.exe`**, gebaut aus Laufzeitstand `13abf1e`, besteht sieben Prüfungen mit 26 frischen Aufnahmen sowie einen zusätzlichen Start-/Spiellauf über die Root-EXE. Der [Prüfbericht](WATER_OPTICS_RESULTS.json) bindet EXE, Paketcontainer, Laufprotokolle, Quellstände und Bilder über SHA-256. Geprüft wurden der kontrollierte Tag-/Nachtvergleich, Auf-/Abtauchen an allen drei Einstiegen, Sturm und eine Nachtansicht mit Lampe bei vier Metern Sichtweite. Die bekannten CelestialVault-Startmeldungen sind im Bericht erhalten. Ein Dauerlauf oder isolierter Bildratenvergleich dieses Pakets wurde nicht durchgeführt.

Erzwungene frühere Volumenbehandlung im neuen Paket, feste freie Kamera:

![Vergleich: bisherige Absorption hinter der Oberfläche](Images/Underwater_Surface_Previous.png)

Dieselbe Kamera mit korrigierter Volumenbehandlung; Unterwassertrübung bleibt aktiv:

![Vergleich: Luftraum hinter der Oberfläche](Images/Underwater_Surface_Current.png)

Normaler Tauchlauf im Paket; die Umgebung bleibt deutlich unter dem angestrebten Detailniveau:

![Spielansicht mit korrigierter Wasseroptik](Images/Underwater_Surface_Game.png)


## Zusammenhängende Fels- und Pflanzenbereiche

Die nächste Überarbeitung ersetzt die weitgehend gleichmäßige Verteilung einzelner großer Felsen durch unregelmäßige Gruppen. Eine Gruppe enthält einen größeren Fels, mehrere mittlere Steine und kleinere Bruchstücke. Die vorhandenen drei Felsformen werden dafür unterschiedlich skaliert, gedreht, teilweise eingegraben und an der vermessenen Bodenneigung ausgerichtet. Die kleinen Kiesinstanzen bleiben eine eigene Größenstufe. Freie Sedimentflächen bleiben zwischen den Gruppen erhalten.

Die Pflanzen folgen kontinuierlichen räumlichen Feldern statt wiederkehrenden Sinusbändern. Innerhalb eines Bereichs überwiegt eine Art, mit eingestreutem Mischbewuchs. Bestehende Grenzen für Tiefe, Sichtweite und Hangneigung gelten weiter; Felsflächen werden beim Pflanzen ausgespart. Das ist eine gestaltete Spielumgebung, keine kartierte Verteilung realer Walensee-Lebensräume.

Die Verteilung hängt von Weltkoordinaten ab. Felsgruppen werden einschließlich ihres Randbereichs berechnet; jeder Stein gehört genau der Zelle seiner endgültigen Position. Damit können Gruppen Zellgrenzen überschreiten, ohne doppelte Steine oder bei einer Rückkehr andere Anordnungen zu erzeugen. Einstiege und Zugangswege behalten ihre festen Freiräume. Instanzen werden gebündelt angelegt, bevor ihre Komponenten registriert werden.

`Tools/SmokeTest.ps1 -EditorSandbox -Scenario rocks -HabitatView -Hour 12` erzeugt drei Ansichten ohne HUD. Die Kamerawahl hängt ausschließlich vom vermessenen Gelände ab, nicht von der jeweils getesteten Objektverteilung. Der native Vergleich hält Positionen und Blickwinkel zwischen dem vorherigen Stand und der überarbeiteten Verteilung gleich. Er zeigt eine dichtere Staffelung und mehr Bewuchs; Materialien, Beleuchtung und die Gesamtwirkung liegen weiterhin deutlich unter der Nutzerreferenz. Die erste Variante mit weiter auseinanderliegenden Gruppen wurde nach Sichtprüfung verdichtet.

Vorher, native Spielaufnahme:

![Vorherige Verteilung von Felsen und Pflanzen](Images/Habitat_Before.png)

Überarbeitete Verteilung aus derselben Kamera:

![Gruppierte Felsen, Geröll und Pflanzen](Images/Habitat_After.png)


Die erste Paketserie (`artifacts/habitat-validation-20261126-142507`) wurde nach drei erfolgreichen Weesen-Fällen beim Murg-Kameratest abgebrochen. Die ursprüngliche Suche fand dort keinen flachen Bereich im vorgegebenen Tiefenband. Die Vergleichsprüfung erlaubt nun in einem zweiten Suchdurchlauf einen steileren vermessenen Hang und richtet den Blick an dessen Bodenverlauf aus. Weesens Vergleichskamera bleibt unverändert. Der abgebrochene Lauf bleibt erhalten und wird nicht als vollständige Paketfreigabe gewertet.


Bei der Sichtprüfung bleiben neben den einheitlichen Felsfarben und den dünnen Pflanzenoberflächen feine Linien auf einzelnen Bodenflächen sichtbar, unter anderem im Lediwracks-Vergleich. Ihre Ursache ist noch nicht isoliert; sie werden nicht als Bestandteil einer natürlichen Sedimentstruktur gewertet. Die nächste Bodenüberarbeitung muss auch diese sichtbaren Übergänge prüfen.


Die drei Streaming-Prüfungen des überarbeiteten Pakets erfassen im jeweiligen anfänglichen 3×3-Zellfenster folgende Instanzen. Die Zahlen beschreiben geladene Bereiche und nicht die gleichzeitig im Bild sichtbaren Objekte:

| Einstieg | Felsen und Geröll | Wasserpflanzen |
| --- | ---: | ---: |
| Weesen | 3.722 | 3.430 |
| Murg | 5.886 | 96 |
| Mols/Lediwracks | 5.750 | 723 |

Die unterschiedliche Pflanzendichte folgt den bestehenden Tiefen-, Sichtweiten- und Hanggrenzen. Alle drei Läufe prüfen eindeutige Zellzugehörigkeit, freien Einstiegsraum, sechs unverändert behaltene und neun identisch wiederhergestellte Zellen sowie die Ausweichbewegung aller fünf Fischarten.

Die vollständige abschließende Serie `artifacts/habitat-validation-20261126-143554` besteht mit **zwölf Fällen und 60 frischen Aufnahmen** sowie einem zusätzlichen Start-/Spiellauf über die Root-EXE. Das Paket `Builds/WindowsHabitat/divingsim.exe` wurde aus Laufzeitstand `c14eaa5` gebaut. Der [Prüfbericht](HABITAT_RESULTS.json) bindet EXE, Paketcontainer, Quellstände, Protokolle und Bilder über SHA-256. Die bekannte CelestialVault-Startmeldung bleibt im Bericht erhalten. Ein neuer Langzeitlauf ist damit nicht belegt. Die automatischen Simulationsprüfungen für diesen Quellstand bestehen auf GitHub ebenfalls ([Lauf](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45830285311)).

Die ergänzenden 1080p-Leistungsmessungen erreichen 68,18 FPS im Unterwasserlauf und 100,53 FPS auf der Murg-Strecke. Der Unterwasserlauf unterschreitet zeitweise 60 FPS, der Streckenlauf enthält zwei Bilder über 33,33 ms (Maximum 68,83 ms). Die Ansichten und Tiefen unterscheiden sich; die Zahlen sind kein isolierter Vergleich der Detailkosten. Schattenkosten und Nachladespitzen bleiben zu optimieren. Methodik und vollständige Grenzen: [Leistung](PERFORMANCE.md).

Die nachfolgende Vorschau `Builds/WindowsPopulationStaged/divingsim.exe` übernimmt diese Gestaltung unverändert und optimiert deren Nachladen. Begrenzte Wiederverwendung der Felslayouts und der Aufbau entfernter Zellen über mehrere Bilder senken im gleichen Murg-Messablauf das höchste Bild von 68,83 auf 37,12 ms; ein Bild bleibt über 33,33 ms. Drei Paketfälle mit neun festen Ansichten, Geometrie-/Fischkontinuität, Teleportprüfung und normaler Spielstart bestehen. Diese gezielte Prüfung wiederholt nicht die gesamte ältere Zwölf-Fälle-Serie. [Prüfbericht](POPULATION_STAGED_RESULTS.json), [Details](POPULATION_STREAMING.md). Boden- und Pflanzenmaterialien, Licht und die sichtbaren feinen Bodenlinien bleiben die nächsten Grafikthemen.

## Bewuchs auf Steinoberflächen und feine Geländelücken

Der gemeinsame Bodenshader ergänzt olivbraune Beläge und feine Ablagerungen auf Felsen, Kies und Sediment. Das räumliche Muster verbindet die Oberflächen über Objektgrenzen hinweg. Nach oben gerichtete Flächen tragen mehr Belag; zwischen 14 und 38 Metern Tiefe wird er ausgeblendet. Feine Farbunterschiede behalten die Struktur der vorhandenen PBR-Karten bei. Die Nassabdunklung ist etwas schwächer. Es entstehen weder zusätzliche Texturdateien noch neue Fels- oder Pflanzeninstanzen. Die Verteilung ist eine gestalterische Annäherung an Aufwuchs, keine Kartierung realer Algenarten oder Simulation ihres Wachstums.

Die Untersuchung der geraden dunklen Bodenlinien erfolgt aus derselben Lediwracks-Kamera. Ohne dynamische Schatten bleiben sie sichtbar. Eine unbeleuchtete Ansicht ohne Nachbearbeitung zeigt kleine schwarze Lücken; bei ausgeschaltetem Nanite verschwinden sie. Eine gezielte Erhöhung der Nanite-Positionspräzision an den vier benachbarten Uferkacheln beseitigt sie auch mit aktivem Nanite. Im festgelegten Bildausschnitt `(180,170)–(560,250)` sinkt die Zahl nahezu schwarzer Pixel von neun auf null. Diese Prüfung betrifft diesen Ausschnitt, nicht sämtliche möglichen Geländenähte.

Der reguläre Aufbau verwendet deshalb für Ufer- und Landschaftskacheln dieselbe Nanite-Positionsstufe von `2^-6 cm` (0,015625 cm) statt 1 cm. So ist die Rundung an Grenzen zur prozeduralen Bathymetrie erheblich kleiner. Quellhöhen, Dreiecke und Kollisionsflächen werden nicht verschoben; die Bake-Prüfungen vergleichen die Ausgangsgeometrie weiterhin mit den Geländedaten. Die neue Aufbauversion und die Laufzeitprüfungen verhindern, dass ältere Assets mit Zentimeterpräzision als aktueller Stand geladen werden.

Unreal stellt die Nanite-Aufbaueinstellungen nur im Editor bereit. Deshalb prüfen die Bake-Commandlets jedes Asset einschließlich seiner tatsächlichen Positionsstufe und schreiben diese in die Gelände-Manifeste. Das verpackte Spiel prüft den Manifestwert. Der erste Paketbau mit einem direkten Laufzeitzugriff auf die Editor-Einstellung scheiterte beim Kompilieren; dieser Lauf bleibt als fehlgeschlagener Versuch erhalten und zählt nicht zur Paketprüfung.

Reproduktion: `Tools/Build.ps1 -EditorSandbox -Target Content -OnlyBenthic` baut die vier gemeinsam verwendeten Materialgraphen; `-Target Coast` und `-Target Landscape` erzeugen die Geländedarstellungen. `Tools/SmokeTest.ps1 -EditorSandbox -Scenario rocks -HabitatView -Entry 2 -Hour 12 -DisableLighting` erzeugt die unbeleuchtete Diagnose. `-DisableShadows` und `-DisableNanite` erlauben getrennte Kontrollaufnahmen. Diese Optionen verändern keine gespeicherten Spiel-Grafikeinstellungen.

Die spielbare Vorschau **`Builds/WindowsBenthic/divingsim.exe`** enthält den Laufzeitstand `4f6e4e457996232a12a4a0bd1500c64602098498`. Die Spiel-EXE hat SHA-256 `1502b3ea65451f0b1435c43e8d5c3f50c78c97e22f0780923d417e326490475c`. Neun Paketfälle mit 20 frischen Bildern und der zusätzliche normale Launcher-Start bestehen: drei Tagesansichten, die Nanite-Ersatzdarstellung, eine feste Nachtansicht ohne Lampe, drei Ufer-Bodenansichten und ein zusätzlicher Nachtfall mit Lampe und geprüftem beleuchtetem Ziel. Die Nachtansicht ohne Lampe ist nahezu schwarz und belegt keine Materiallesbarkeit; der zusätzliche Lampenfall liefert dafür die sichtbare Kontrolle. Die scharf abgestuften Lampenschatten bleiben ein offenes Qualitätsthema.

Der [Prüfbericht](BENTHIC_RESULTS.json) bindet Quellstände, 573 Gelände-Assets, die vier Materialgraphen, Paketcontainer, Diagnosebilder und die Leistungsmessung. Auch der vollständig neu aufgebaute Stand enthält im festgelegten unbeleuchteten Diagnoseausschnitt null schwarze Lückenpixel. Ein neuer Langzeit- oder Streaming-Leistungsvergleich ist damit nicht belegt. Die 60-Sekunden-Unterwassermessung erreicht 68,34 FPS im Mittel und 55,21 FPS im langsamsten Prozent; durchgehend 60 FPS werden weiterhin nicht eingehalten. Details: [Leistung](PERFORMANCE.md).

Gleiche Kamera vor der Überarbeitung:

![Boden und Felsen vor Belägen und Präzisionskorrektur](Images/Benthic_Before.png)

Aktuelles Paket mit Belägen und korrigierter Geländepräzision:

![Aktueller Unterwasserstand mit fleckigen Felsbelägen](Images/Benthic_After.png)

Die Referenzqualität ist weiterhin offen. Besonders die dünnen Pflanzen, die gleichförmige Grundstruktur in größeren Bereichen sowie die Licht- und Schattenwirkung brauchen weitere Überarbeitung. Die Bilder belegen diesen Zwischenstand und keine vollständige visuelle Abnahme.
