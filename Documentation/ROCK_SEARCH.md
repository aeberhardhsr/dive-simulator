# Räumliche Felsabfragen für die Fischbewegung

Die zusätzlichen Bruchstücke erhöhen die Zahl der geladenen Felsen. Im Paket `WindowsRockRubble` stieg die mittlere Spielthreadzeit der Unterwasserläufe auf rund 11,1 ms. Die Fischbewegung durchlief für Ausweichen, Wegprüfung und neue Fischpositionen jeweils die gesamte Felsliste. Das neue Suchraster begrenzt diese Kandidatenlisten räumlich. Es entfernt keine sichtbaren Steine und ändert keine Kollisionshüllen, Fischparameter oder vermessenen Geländehöhen.

## Verfahren

`Simulation/RockIndex.h` ordnet jeden Felsmittelpunkt genau einer acht Meter breiten XY-Zelle zu. Punkt- und Streckenabfragen erweitern ihr Rechteck um den grössten geladenen Felsradius und den jeweiligen Fisch-/Ausweichabstand. Ein zusätzlicher Abstand von 0,1 mm berücksichtigt Rundungen der vorhandenen Float-Radien. Erst die bisherigen dreidimensionalen Abstands- und Kugel-Schnittprüfungen entscheiden über Kontakt und Ausweichwirkung. Die zweidimensionale Vorauswahl kann dadurch zusätzliche Kandidaten enthalten, darf aber keine relevanten Felsen auslassen.

Die Ergebnisse werden nach ihrem ursprünglichen Index sortiert. Damit bleibt die Reihenfolge der berücksichtigten Felsen erhalten; sie hängt nicht von der internen Hash-Reihenfolge ab. Sehr grosse oder ungültige Abfragen fallen auf die vollständige Liste zurück. Bei einem Wechsel des Populationsfensters wird das Raster aus der vollständigen Ziel-Felsliste neu aufgebaut, bevor neue Fischpositionen gesucht werden. Das gilt auch für die noch nicht dargestellten entfernten Zellen beim gestaffelten Nachladen. Ein neuer Einstieg leert Liste und Raster gemeinsam.

Die Anpassung betrifft die Felsabfragen für Fischbewegung und Fischplatzierung. Geländeproben, Abstände zwischen Fischen, Pflanzenplatzierung und die tatsächlichen Unreal-Kollisionskörper behalten ihr bisheriges Verfahren.

## Nachweise und Grenzen

Der portable C++-Test vergleicht 1.600 Streckenabfragen exakt mit einer vollständigen Kugelprüfung. Er enthält negative Koordinaten, Zellgrenzen, Tangenten, grosse Felsradien, grosse Abfragen, ungültige Abfragen und das Leeren/Neuaufbauen. In der synthetischen Verteilung werden 18.935 Kandidaten statt 6.563.200 vollständiger Felseinträge besucht. Das ist eine Algorithmusprüfung, keine Bildratenmessung.

Zusätzlich prüft die echte Welt an über das geladene Fenster verteilten Positionen, ob alle nahen Felsen enthalten sind. Wegschnittpunkte werden exakt gegen vollständige Durchläufe verglichen; Ausweichsummen dürfen höchstens 10⁻¹² voneinander abweichen. Diese enge Toleranz berücksichtigt Rundung der optimierten Double-Rechnung, keine ausgelassenen Hindernisse. Dieselben Prüfungen laufen nach Zellwechseln, Teleports und Rückkehr ins ursprüngliche Fenster. Die bestehenden Arten-, Kontakt-, Einstiegs-, Kies-, Pause- und Nachladeprüfungen bleiben aktiv. Diese Stichproben ersetzen keine vollständige Langzeitabnahme.

Der [native Nachweis](ROCK_SEARCH_NATIVE_RESULTS.json) besteht zwei Fälle mit sechs Aufnahmen und zwölf Rasterprüfungen über die verschiedenen Fensterzustände. Im normalen Lediwracks-Fenster bleiben 10.614 Felsen und 606 Pflanzen erhalten. Die erste Rasterprüfung besucht 24.424 Kandidaten statt 2.069.730 Felseinträgen; Ausweichsummen und Wegprüfungen sind exakt gleich. Die drei Vergleichskameras und sämtliche erfassten Felsassets stimmen mit dem vorherigen nativen Geröllstand überein. Die Bilder zeigen weiterhin denselben Gestaltungsstand.

## Leistung des Windows-Pakets

Das abschliessend geprüfte Paket `WindowsRockSearchReady` erreicht bei 1080p/Hoch 72,78 FPS im 60-Sekunden-Unterwasserlauf und 100,81 FPS auf der 90-Sekunden-Nachladestrecke. Die mittlere Spielthreadzeit beträgt 3,40 / 3,04 ms, gegenüber rund 11,1 / 6,01 ms im vorherigen Geröllpaket. Die GPU benötigt weiterhin 13,13 / 9,31 ms. Die Optimierung schafft damit Rechenreserve für die Spielberechnung; die unveränderte Grafik wird nicht entsprechend schneller gerendert. Unter Wasser liegt kein Bild über 33,33 ms, auf der Nachladestrecke bleibt ein Ausreisser mit 45,33 ms. Die früher beobachteten Ruckler sind damit nicht dauerhaft behoben. [Vollständige Messbedingungen](PERFORMANCE.md).

Im ersten Paket `WindowsRockSearch` erreichte der Unterwasserlauf 72,88 FPS bei 3,43 ms Spielthreadzeit; die Nachladestrecke 100,81 FPS bei 3,09 ms. Ein Unterwasserbild mit 52,74 ms und zwei Nachladebilder über 33,33 ms bleiben getrennt als historische Leistungsbefunde erhalten. Die anschliessenden Änderungen betreffen nur die Prüffunktion, nicht die normale Suchberechnung.

Der erste Paketlauf bricht bei Weesen ab: Sein weiter Test-Teleport führt korrekt in ein Fenster ohne Felsen, die neue Testfunktion verlangt aber irrtümlich mindestens eine Felsabfrage. Die beiden Lediwracks-Fälle davor bestehen. Die Korrektur prüft leere Fenster ausdrücklich mit einer lokalen Abfrage und dem vollständigen Rasterbestand auf veraltete Einträge. Ein gezielter nativer Weesen-Lauf besteht anschliessend einschliesslich dieses leeren Fensters und der Rückkehr zu 7.482 Felsen. Der fehlgeschlagene Paketlauf und seine beiden oben genannten Messungen bleiben erhalten. Die Korrektur betrifft nur die Testfunktion und ihren Berichtsprüfer, nicht die normale Spielberechnung.

Ein zweiter Paketversuch (`WindowsRockSearchFinal`, Quellstand `49881dd`) scheitert an der bitgenauen Gleichheitsforderung für Kraftsummen. Die anschliessende Diagnose im Spiel-Build findet keine fehlenden Kandidaten und keine unterschiedlichen Wegschnittpunkte. Die Kraftsummen unterscheiden sich um höchstens 1,3323 × 10⁻¹⁵. Im nativen Editor-Build ergeben dieselben Proben exakt gleiche Werte. Die Vergleichsprüfung erfasst deshalb jetzt die maximale numerische Abweichung und erlaubt 10⁻¹²; Kandidatenvollständigkeit und Wegschnittpunkte bleiben streng geprüft. Beide fehlgeschlagenen Versuche und der separate Diagnose-Build bleiben erhalten. Der Diagnose-Build kombiniert ausschliesslich eine neu kompilierte Spiel-EXE mit unveränderten Paket-Inhalten; er ist keine veröffentlichte Spielvorschau.

## Geprüfte Spielversion

**`Builds/WindowsRockSearchReady/divingsim.exe`** enthält Quellstand `f47b30cf871cb1640a6d49cb5b30accc9782c231`. Die Spiel-EXE hat SHA-256 `c9501050b3c1c3aad80fa13ad2fcb65725c83290adc39fb3f2a7ea4287433257`. Der vollständige Paketlauf `artifacts/rock-rubble-20261126-225251` besteht acht Fälle mit 29 Aufnahmen und den normalen Launcher-Start. Seine 42 Rasterprüfungen enthalten keine fehlenden Kandidaten und keine unterschiedlichen Wegschnittpunkte. Die grösste gemessene Kraftsummen-Abweichung beträgt 2,665 × 10⁻¹⁵ und liegt deutlich unter 10⁻¹². Beide CI-Läufe des Laufzeitstands sind erfolgreich: [Push](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45900816741), [Pull Request](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45900825612).

Der [abschliessende Ergebnisbericht](ROCK_SEARCH_RESULTS.json) bindet Quellen, Inhalte, EXE, Launcher, alle Szenen und deren Diagnosedaten. Er erhält die beiden früheren Fehlversuche, den gesonderten Rundungsnachweis und die Messungen des ersten Pakets getrennt vom freigegebenen Stand. Die Vergleichskameras und die erfassten Felsassets stimmen mit `WindowsRockRubble` überein. Reproduktion: `Tools/TestRockRubble.ps1 -Packaged -PackageDirectory Builds/WindowsRockSearchReady`; der Berichtsprüfer ergänzt mit `--rock-search`, `--baseline-run` und den im JSON genannten Laufverzeichnissen die Such- und Verlaufskontrollen.

Nahaufnahme aus dem geprüften Paket, gleicher Grafikstand mit räumlicher Suche:

![Felsen und Bruchstücke mit räumlicher Hindernissuche](Images/RockSearch_Close.png)

Die [visuelle Zielqualität](VISUAL_TARGET.md) bleibt offen. Die Optimierung schafft CPU-Reserve; sie verbessert die sichtbare Umgebung nicht selbst. Unterwasserwelt hat weiterhin Vorrang vor Überwasserlandschaft und anschliessend [Menü/HUD/FPV](UI_UX_TARGET.md).
