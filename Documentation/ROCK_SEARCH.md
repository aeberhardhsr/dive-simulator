# Räumliche Felsabfragen für die Fischbewegung

Die zusätzlichen Bruchstücke erhöhen die Zahl der geladenen Felsen. Im Paket `WindowsRockRubble` stieg die mittlere Spielthreadzeit der Unterwasserläufe auf rund 11,1 ms. Die Fischbewegung durchlief für Ausweichen, Wegprüfung und neue Fischpositionen jeweils die gesamte Felsliste. Das neue Suchraster begrenzt diese Kandidatenlisten räumlich. Es entfernt keine sichtbaren Steine und ändert keine Kollisionshüllen, Fischparameter oder vermessenen Geländehöhen.

## Verfahren

`Simulation/RockIndex.h` ordnet jeden Felsmittelpunkt genau einer acht Meter breiten XY-Zelle zu. Punkt- und Streckenabfragen erweitern ihr Rechteck um den grössten geladenen Felsradius und den jeweiligen Fisch-/Ausweichabstand. Ein zusätzlicher Abstand von 0,1 mm berücksichtigt Rundungen der vorhandenen Float-Radien. Erst die bisherigen dreidimensionalen Abstands- und Kugel-Schnittprüfungen entscheiden über Kontakt und Ausweichwirkung. Die zweidimensionale Vorauswahl kann dadurch zusätzliche Kandidaten enthalten, darf aber keine relevanten Felsen auslassen.

Die Ergebnisse werden nach ihrem ursprünglichen Index sortiert. Damit bleiben die Reihenfolge der Kraftaddition und die bisherigen Entscheidungen bei gleichen Werten erhalten. Sehr grosse oder ungültige Abfragen fallen auf die vollständige Liste zurück. Bei einem Wechsel des Populationsfensters wird das Raster aus der vollständigen Ziel-Felsliste neu aufgebaut, bevor neue Fischpositionen gesucht werden. Das gilt auch für die noch nicht dargestellten entfernten Zellen beim gestaffelten Nachladen. Ein neuer Einstieg leert Liste und Raster gemeinsam.

Die Anpassung betrifft die Felsabfragen für Fischbewegung und Fischplatzierung. Geländeproben, Abstände zwischen Fischen, Pflanzenplatzierung und die tatsächlichen Unreal-Kollisionskörper behalten ihr bisheriges Verfahren.

## Nachweise und Grenzen

Der portable C++-Test vergleicht 1.600 Streckenabfragen exakt mit einer vollständigen Kugelprüfung. Er enthält negative Koordinaten, Zellgrenzen, Tangenten, grosse Felsradien, grosse Abfragen, ungültige Abfragen und das Leeren/Neuaufbauen. In der synthetischen Verteilung werden 18.935 Kandidaten statt 6.563.200 vollständiger Felseinträge besucht. Das ist eine Algorithmusprüfung, keine Bildratenmessung.

Zusätzlich prüft die echte Welt an über das geladene Fenster verteilten Positionen, ob alle nahen Felsen enthalten sind. Ausweichsummen und Wegprüfungen werden exakt gegen vollständige Durchläufe verglichen. Dieselben Prüfungen laufen nach Zellwechseln, Teleports und Rückkehr ins ursprüngliche Fenster. Die bestehenden Arten-, Kontakt-, Einstiegs-, Kies-, Pause- und Nachladeprüfungen bleiben aktiv. Diese Stichproben ersetzen keine vollständige Langzeitabnahme.

Der [native Nachweis](ROCK_SEARCH_NATIVE_RESULTS.json) besteht zwei Fälle mit sechs Aufnahmen und zwölf Rasterprüfungen über die verschiedenen Fensterzustände. Im normalen Lediwracks-Fenster bleiben 10.614 Felsen und 606 Pflanzen erhalten. Die erste Rasterprüfung besucht 24.424 Kandidaten statt 2.069.730 Felseinträgen; Ausweichsummen und Wegprüfungen sind exakt gleich. Die drei Vergleichskameras und sämtliche erfassten Felsassets stimmen mit dem vorherigen nativen Geröllstand überein. Die Bilder zeigen weiterhin denselben Gestaltungsstand.

## Leistung des Windows-Pakets

Bei 1080p/Hoch erreicht der 60-Sekunden-Unterwasserlauf 72,88 FPS. Die mittlere Spielthreadzeit beträgt 3,43 ms gegenüber rund 11,1 ms in den vorherigen Geröllläufen. Die 90-Sekunden-Nachladestrecke erreicht 100,81 FPS bei 3,09 ms Spielthreadzeit, zuvor 6,01 ms. Die GPU-Zeiten bleiben ähnlich. Diese getrennten Messungen zeigen mehr CPU-Reserve in den geprüften Szenarien; die unveränderte Grafik wird dadurch nicht automatisch schneller gerendert. Ein Unterwasserbild mit 52,74 ms und zwei Nachladebilder über 33,33 ms bleiben als offene Leistungsbefunde erhalten. [Vollständige Messbedingungen](PERFORMANCE.md).

Der erste Paketlauf bricht bei Weesen ab: Sein weiter Test-Teleport führt korrekt in ein Fenster ohne Felsen, die neue Testfunktion verlangt aber irrtümlich mindestens eine Felsabfrage. Die beiden Lediwracks-Fälle davor bestehen. Die Korrektur prüft leere Fenster ausdrücklich mit einer lokalen Abfrage und dem vollständigen Rasterbestand auf veraltete Einträge. Ein gezielter nativer Weesen-Lauf besteht anschliessend einschliesslich dieses leeren Fensters und der Rückkehr zu 7.482 Felsen. Der fehlgeschlagene Paketlauf und seine beiden oben genannten Messungen bleiben erhalten. Die Korrektur betrifft nur die Testfunktion und ihren Berichtsprüfer, nicht die normale Spielberechnung.

Der abschliessende Paketnachweis wird nach dem vollständigen Wiederholungslauf ergänzt. Die [visuelle Zielqualität](VISUAL_TARGET.md) bleibt offen. Unterwasserwelt hat weiterhin Vorrang vor Überwasserlandschaft und anschliessend [Menü/HUD/FPV](UI_UX_TARGET.md).
