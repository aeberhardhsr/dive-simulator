# Bruchstücke zwischen Felsblöcken und Kleinkies

**Korrektur zur Ortszuordnung:** Die Unterwasser-Vergleiche mit direktem Start und Index 2 liegen bei Walenstadt/Ost. Frühere Bezeichnungen als „Ledi“ bzw. „Lediwracks“ und historische `ledi`-Fallnamen dürfen nicht mit dem separaten Uferzugang Mols/Lediwracks gleichgesetzt werden. [Zuordnung der Testorte und Prüfumfang](TEST_ENTRY_LOCATIONS.md).

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

## Spielbares Paket und Nachweise

**`Builds/WindowsRockRubble/divingsim.exe`** enthält Laufzeit-Quellstand `7fb9f3465fa7b0f3c679188f97a2544aceab3178`. Die ausführbare Spieldatei hat SHA-256 `6ccb6abb2bb788c791e2b3b270881c6faed53d1681b9a522aebe6e52ce270729`. Der abschliessende Lauf `artifacts/rock-rubble-20261126-221139` besteht alle acht Fälle mit 29 Aufnahmen und den Start über den normalen Paket-Launcher. Der [Paketbericht](ROCK_RUBBLE_RESULTS.json) verbindet Quellen, Container, Bilder, Kollisionsprüfungen, Launcher und alle drei Leistungsmessungen. Der vorherige [native Vergleich](ROCK_RUBBLE_NATIVE_RESULTS.json) umfasst zwei Fälle mit sechs Bildern; seine damaligen Werkzeug-Hashes bleiben als historischer Nachweis erhalten. Beide CI-Läufe des Laufzeitstands sind erfolgreich: [Push](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45894375551), [Pull Request](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45894368498).

Der erste Paketlauf `artifacts/rock-rubble-20261126-220445` endet nach sieben erfassten Fällen: Der Uferweg selbst besteht, erzeugt aber neun Bilder, während das neue Erfassungsskript irrtümlich sechs erwartet. Seine Fallliste wurde nicht nachträglich auf bestanden gesetzt. Die ursprünglichen Protokolle und neun Uferbilder bleiben im Paketbericht gebunden. Der terminierende PowerShell-Fehler erschien in der Shell-Ausgabe; das umgeleitete Treiberprotokoll enthält nur die sieben abgeschlossenen Fälle. Das korrigierte Skript prüft nun Anzahl und genaue Namen aller neun Uferbilder. Danach wurde der gesamte Paketlauf wiederholt. Diese Korrektur betrifft ausschliesslich Erfassung und Bericht; das geprüfte Spielpaket wurde nicht verändert.

## Bildvergleich aus dem Windows-Paket

Vorher, zusätzliche Bruchstücke im Diagnosemodus abgeschaltet:

![Felsfuss vor den zusätzlichen Fragmenten](Images/RockRubble_Before.png)

Nachher, normale Spielverteilung mit Bruchstücken:

![Räumliche Bruchstücke am Felsfuss](Images/RockRubble_After.png)

Beide Bilder stammen aus demselben Paket und verwenden dieselbe Kamera. Die Kamerapositionen aller drei Vergleichsansichten werden im Ergebnisbericht exakt verglichen.

## Leistung

Die beiden 60-Sekunden-Unterwasserläufe erreichen bei 1080p/Hoch rund 73 FPS. Im ersten Lauf tritt ein einzelner 59,21-ms-Ausreisser auf, im Wiederholungslauf kein Bild über 33,33 ms. Die 90-Sekunden-Nachladestrecke erreicht 100,64 FPS mit zwei Ausreissern über 33,33 ms und maximal 43,64 ms. Alle drei Läufe bleiben erhalten; die Wiederholung ersetzt den auffälligen ersten Lauf nicht. Die Ursache des einzelnen Unterwasser-Ausreissers ist offen, und die CPU-Reserve ist gegenüber dem älteren Paket kleiner. [Vollständige Messbedingungen und Grenzen](PERFORMANCE.md).

Der Paketbericht nimmt beide ursprünglichen Messordner mit `--performance <Unterwasser> <Nachladen>` auf. `--repeat-underwater <Wiederholung>` ergänzt den dritten Lauf mit eigenem Kennzeichen. Der Bericht prüft, dass alle Messungen dieselbe EXE und dieselben Paketcontainer verwenden. Ein aktueller Langzeittest ist damit nicht erledigt.
