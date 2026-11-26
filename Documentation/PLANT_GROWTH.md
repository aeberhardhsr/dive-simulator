# Gebogene Triebe und dichtere Blattbereiche

Die bisherigen Pflanzen wirkten in den geprüften Nahansichten wie dünne Drahtbäume: weitgehend gerade Stängel, regelmässige Astetagen und lange kahle Spitzen. Die drei allgemeinen Pflanzengruppen erhalten jetzt gebogene Triebe, ungleiche Abstände zwischen Blattansätzen und mehr Blattfläche im oberen Bereich. Stängel und grünes Gewebe bekommen getrennte, zurückhaltendere Farbtöne. Die bestehende Gruppenplatzierung bleibt erhalten.

Laichkraut trägt zwölf statt acht Blattansätze je Trieb; kleinere obere Blätter reichen bis an die Spitze. Armleuchteralgen verwenden unterschiedlich viele Seitenstrahlen und kurze Verzweigungen an mehreren Stellen statt gleicher gabelförmiger Enden. Tausendblatt behält vier fein gefiederte Blätter je Quirl, mit grösseren Fiederflächen und unregelmässiger Neigung. Die Blattquirle und gegenständigen Abschnitte sind als grobe Formreferenz beim [Naturhistorischen Museum Wien](https://flora.nhm-wien.ac.at/Seiten-Arten/Myriophyllum-spicatum.htm) beschrieben. Daraus wird keine genaue Artenrekonstruktion oder nachgewiesene lokale Verteilung abgeleitet; sämtliche Geometrie ist eigenständig erzeugt.

Die Farbwerte sind gestaltete lineare Reflexionswerte. Materialgraph, Lichtdurchlässigkeit und Bewegungsmodell behalten ihr bisheriges Verfahren. Die Höhenbegrenzung hält auch seitliche Triebe innerhalb der jeweiligen Habitat-Höhe. Bodenpositionen, Artengewichte und Felsabstände ändern sich nicht.

## Geometrie und Prüfung

| Gruppe | Bisher LOD0 | Neu LOD0 | Weitere Detailstufen | Modellhöhe |
| --- | ---: | ---: | --- | ---: |
| Laichkraut | 7.224 | 10.136 | 4.562 / 1.621 / 557 | 1,031 m |
| Armleuchteralgen | 27.744 | 48.366 | 21.764 / 7.738 / 2.660 | 0,439 m |
| Tausendblatt | 50.456 | 67.032 | 30.165 / 10.726 / 3.686 | 0,867 m |

Der erste Armleuchteralgen-Entwurf benötigte 100.320 Dreiecke und überschritt mit 0,471 m die vorgesehene Höhe von 0,45 m. Er wurde vor dem Bildvergleich überarbeitet: weniger Haupttriebe und Blattetagen sowie vereinfachte kleine Verzweigungen. Die endgültigen Formen erfüllen die Höhenbegrenzung. Blender prüft endliche Koordinaten und entartete Flächen; der Unreal-Import prüft Dreieckzahlen, beide UV-Kanäle, Vertexfarben und vier abnehmende Detailstufen.

Je zwölf native Modellaufnahmen vor und nach der Änderung zeigen die alten primitiven Vergleichsmodelle und die jeweils aktuellen Formen. Die veröffentlichten Paare vergleichen ausdrücklich die zuvor ausgelieferten detaillierten Modelle mit den neuen Modellen, nicht mit den primitiven Platzhaltern. Eine zusätzliche feste Weesener Umgebungskamera besteht die Platzierungs- und Nachladeprüfung mit weiterhin 2.171 Pflanzen, 840 niedrigen und 297 hohen Exemplaren.

## Grenzen

Die Formen wirken dichter und weniger starr, behalten aber eine erkennbare prozedurale Struktur. Feine Blattteile sind in grösserer Entfernung weiterhin sehr klein auf dem Bildschirm; die vier Detailstufen sind automatisch reduziert. Pflanzenoberflächen, Übergänge zum Sediment und die Lichtwirkung bleiben weitere Arbeit für die [visuelle Referenz](VISUAL_TARGET.md). Die Unterwasserwelt ist damit noch nicht vollständig abgenommen; danach folgen Überwasserlandschaft und [Menü/HUD/FPV](UI_UX_TARGET.md).
