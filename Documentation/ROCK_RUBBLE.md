# Bruchstücke zwischen Felsblöcken und Kleinkies

Die neue Verteilung ergänzt teilweise eingesunkene, räumliche Felsbruchstücke an den vorhandenen Felsfüssen. Damit gibt es zwischen den grossen Blöcken und dem auf sechs Zentimeter begrenzten Kleinkies eine zusätzliche Grössenstufe. Die bestehenden grossen Formationen behalten Position, Form und Material. Die Bruchstücke verwenden dieselben drei gescannten Felsmeshes samt passenden UV-Materialien und der [korrigierten Quellfarb-Kodierung](BENTHIC_COLOR.md).

## Gestaltung und Grenzen

Ein eigener, weltfester Zufallsstrom erzeugt bis zu 24 Kandidaten je vorhandenem Formationsanker. Die horizontale Skalierung liegt zwischen 0,20 und 0,65 bei einem auf einen Meter Radius normierten Ausgangsmesh. Das ist kein Durchmesser: Die tatsächliche Ausdehnung hängt von Scanform, Seitenverhältnis und Neigung ab. Die vertikale Skalierung beträgt 55–90 Prozent der horizontalen Skalierung. Die Mitte liegt nur 1–3,25 Zentimeter über dem vermessenen Grund; ein Teil jedes Steins bleibt eingegraben.

Die Fragmente verteilen sich in unregelmässigen Abständen um die grösseren Blöcke. Bereiche tief im Inneren der bestehenden Blöcke, zu enge Überlappungen, steile Flächen und die geschützten Einstiegs-/Zugangswege werden ausgespart. Es gibt keine geschlossene dekorative Ringreihe. Die Tiefengrenzen sind 2,5–40 Meter. Das ist gestaltete Umgebung, keine Vermessung einzelner Walensee-Steine und kein physikalisches Erosionsmodell.

Die neue Geometrie wird in denselben Instanzkomponenten wie die bisherigen Felsen gerendert. Sie besitzt deren Kollision und wird in den Hindernissen für Fischbewegung, Pflanzen und Kleinkies berücksichtigt. Dadurch können sich auch nahe Pflanzen, Kiesel und Fischwege ändern. Die vermessenen Geländehöhen werden nicht verändert. Die Nachladefenster bleiben bei neun sichtbaren Zellen und höchstens 25 gespeicherten Felslayouts. Der begrenzte Kandidatenabstand bewahrt den bestehenden Nachbarbereich und die eindeutige Zellzuordnung.

Für abgewiesene Platzierungskandidaten werden die zusätzlichen Neigungsabfragen erst nach den zweidimensionalen Abstandsprüfungen ausgeführt. Das spart Geländezugriffe, ohne Zufallsfolge, Annahmekriterien oder resultierende Positionen zu ändern. Zusätzliche Geometrie kostet dennoch Rechenzeit; die Bildrate muss am Paket gemessen werden.

## Prüfung

`-DiveNoRockRubble` schaltet die Ergänzung ausschliesslich in Smoke-Tests aus. Der Vergleich nutzt dasselbe Programm, dieselben Materialien und identische Kamerapositionen. Im ersten Lediwracks-Vergleich steigt die Zahl der geladenen Felsinstanzen von 5.335 auf 10.614; davon sind 5.279 neue Bruchstücke. Diese Zahlen beziehen sich auf das gesamte geladene Fenster, nicht nur auf die sichtbaren Steine.

Die Laufzeitprüfung kontrolliert eindeutige Zellzuordnung, erhaltene und neu geladene Instanzen, identische frisch berechnete Layouts, Grössen-/Einbettungsgrenzen, geschützte Einstiege und die Übereinstimmung zwischen gerenderten Instanzen und Hindernisdaten. Zusätzlich werden einzelne Kollisionskörper der neuen Bruchstücke direkt mit Linienabfragen geprüft; im nativen Lediwracks-Fall treffen 72 solcher Proben. Das ist eine Stichprobe, keine Prüfung jeder möglichen Kontaktrichtung. Die vorhandenen Fischbewegungsprüfungen laufen gegen die ergänzte Welt.

`Tools/TestRockRubble.ps1 -Preview` erzeugt zwei native Vergleichsfälle mit sechs Bildern. Ohne `-Preview` folgen alle drei Einstiege, eine Rasterdarstellung ohne Nanite, die Nachladestrecke und der Uferweg: acht Fälle mit 29 Aufnahmen. Für das gebaute Spiel werden `-Packaged -PackageDirectory <Paketordner>` verwendet. `Tools/ReportRockRubble.py` bindet die Aufnahmen und Laufzeitprotokolle an die tatsächlichen Quellen, Assets und gegebenenfalls Paketcontainer; optionale Leistungsmessungen müssen dieselbe ausführbare Datei und dieselben Container verwenden.

Die ersten Nahaufnahmen zeigen mehr räumliche Übergänge am Felsfuss. Grössere Sedimentflächen, Pflanzen und Lichtverteilung bleiben zu gleichförmig. Die [gewünschte Gesamtqualität](VISUAL_TARGET.md) ist weiterhin nicht erreicht. Überwasserwelt und anschliessend [Menü/HUD/FPV](UI_UX_TARGET.md) bleiben die folgenden Arbeitsphasen.
