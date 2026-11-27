# Unterschiedliche Höhen und Ausrichtungen der Felsblöcke

**Korrektur zur Ortszuordnung:** Die Unterwasser-Vergleiche mit direktem Start und Index 2 liegen bei Walenstadt/Ost. Frühere Bezeichnungen als „Ledi“ bzw. „Lediwracks“ und historische `ledi`-Fallnamen dürfen nicht mit dem separaten Uferzugang Mols/Lediwracks gleichgesetzt werden. [Zuordnung der Testorte und Prüfumfang](TEST_ENTRY_LOCATIONS.md).

Die gescannten Felsformen wurden bisher bei der Platzierung stark vertikal zusammengedrückt und nahezu einheitlich am Gelände ausgerichtet. Die neue Darstellung lässt die ursprünglichen Bruchflächen stärker hervortreten: Kernblöcke sind weniger abgeflacht, grössere und kleinere Hauptsteine besitzen unterschiedliche Neigungen. Die zusätzlich erzeugten Bruchstücke am Felsfuss behalten ihre bisherige Form.

Das verändert tatsächliche Geometrie, Konturen, Schatten und Kontaktflächen. Der zuvor untersuchte [Material-Parallax](BED_RELIEF_INVESTIGATION.md) bleibt verworfen. Die neue Ausrichtung ist eine gestalterische Annäherung, keine Rekonstruktion lokaler Walensee-Geologie oder physikalische Ablagerungssimulation.

## Aufbau und unveränderte Platzierungsdaten

Der vertikale Skalierungsfaktor relativ zur horizontalen Grösse steigt bei Kernblöcken von 0,50–0,85 auf 0,78–1,00, bei den anderen Hauptsteinen von 0,55–0,80 auf 0,72–1,00. Ein unabhängiger, weltfester Zufallsstrom ergänzt bis zu ±22 Grad Neigung und ±18 Grad seitliche Kippung. Die bereits vorhandene Ausrichtung entlang des Geländes bleibt die Grundlage. Zufallsfolge und Ablehnungsregeln der ursprünglichen Platzierung werden nicht verändert.

Position, horizontale Grösse, Modellwahl, Anzahl und konservative Hindernishülle jedes Steins bleiben gleich. Dadurch bleiben auch die Eingaben für Pflanzenverteilung, Kleinkies und Fisch-Ausweichverhalten erhalten. Die drei Quellmeshes passen in eine Einheitskugel; kein neuer Skalierungsfaktor übersteigt die bisherige horizontale Skalierung. Die gedrehten Vertices bleiben deshalb innerhalb der bestehenden 1,05-fachen Ausschlusshülle. Die tatsächlichen Kollisionskörper folgen den neuen Transformationen und können entsprechend andere Kontaktflächen ergeben.

Die festen Einstiegspunkte und Zugangswege behalten ihre vorhandenen Freiräume. Vermessene Geländehöhen, Materialquellen, Texturauflösung und Zahl der Instanzkomponenten werden nicht verändert. Der bereits verteilte Aufbau und Abbau von Kies- sowie Fels-/Pflanzenzellen bleibt aktiv.

## Native Prüfung

`artifacts/rock-pose-20261127-040627` besteht zwei Fälle mit sechs Aufnahmen. `-DiveLegacyRockPose` stellt im Smoke-Test die vorherige Skalierung und Ausrichtung wieder her. Beide Varianten verwenden dieselbe EXE, dieselben Assets und exakt dieselben drei Kamerapositionen.

Alle 10.614 Steine des geladenen Vergleichsgebiets besitzen in beiden Varianten dieselben sortierten Platzierungsdaten: Position, Radius, Modellindex und Kennzeichen für Bruchstücke ergeben denselben SHA-1-Wert `D685E3B48D6F4A168459ABA4C6F9FA4666B9E158`. Die erweiterte Laufzeitprüfung kontrolliert zusätzlich gültige Transformationen und den Erhalt der Hindernishüllen. 44 von oben und unten ausgeführte Kollisionsproben treffen in beiden Varianten den jeweiligen grossen Block; seine Unterseite liegt unter dem Grund und seine Oberseite darüber.

Diese Proben prüfen die konvexen Kontaktkörper an ausgewählten Mittellinien. Sie sind keine vollständige Prüfung sämtlicher Dreiecke, Überhänge oder möglichen Bewegungsrichtungen. Die vorhandenen Prüfungen für stabile Zelllayouts, schnelle Richtungswechsel, nahe Teleports, Kieskontakte und Fischbewegung bleiben aktiv. [Nativer Bericht](ROCK_POSE_NATIVE_RESULTS.json).

In den Aufnahmen sind höhere, unregelmässigere Konturen und mehr geneigte Bruchflächen sichtbar. Die gesamte Referenzqualität ist weiterhin offen: flache Sedimentfelder, Bodenanschlüsse, Pflanzen, Lichtwirkung und wiederholte Formen benötigen weitere Arbeit. Überwasserwelt und anschliessend Menü/HUD/FPV folgen weiterhin der vereinbarten Reihenfolge.

## Vergleich aus dem Spielpaket

Vorherige Ausrichtung, im Diagnosemodus derselben EXE:

![Bisher abgeflachte Felsgruppe](Images/RockPose_Before.png)

Neue Ausrichtung, identische Kamera:

![Höhere und unterschiedlich geneigte Felsgruppe](Images/RockPose_After.png)

Weitere Ansicht bei Weesen:

![Felsgruppe in der weiteren Unterwasseransicht](Images/RockPose_Wide.png)

Die Bilder zeigen echte Spielgeometrie. Die Zahl der Instanzen und die verwendeten Scan-Assets sind im direkten Vergleich gleich. Grössere projizierte Flächen und andere Überschneidungen können trotzdem den Renderaufwand verändern; gleiche Objektzahl bedeutet keine garantierte identische Bildrate.

## Geprüftes Paket

`Builds/WindowsRockPose/Start-WaterPreview.cmd` startet die Vorschau mit dem aktuellen Wasserprofil. Der gesamte Paketordner wird benötigt. Der normale Haupt-EXE-Start besteht den zusätzlichen automatischen Launcher-Test. Ein vollständig manuell bedienter Menü-zu-Tauchgang-Test dieses Pakets steht noch aus.

`artifacts/rock-pose-20261127-041106` besteht neun Fälle mit 36 Aufnahmen: Nahvergleich beider Varianten, alle drei Einstiege, Darstellung ohne Nanite, Nachladestrecke, Uferwechsel und Sediment. Die neuen Kontaktprüfungen bestehen mit 40 Proben in Weesen, 56 in Murg und 44 bei Ledi. Alle sieben Fälle mit Populationsprüfung bestehen ausserdem die bestehenden Prüfungen für Nachladefenster, nahe Teleports, begrenzten Komponentenabbau und Fisch-Ausweichverhalten.

Im Paket besitzen beide Vergleichsvarianten denselben Platzierungs-Hash `C8D1CFFE5912B9564886C1E5E0D1404583E36825`. Hashgleichheit wird jeweils innerhalb derselben EXE geprüft, nicht als build- oder plattformübergreifende Bitgleichheit behauptet. Die Eingabedateien des nativen Berichts bleiben nach dem Paketbau unverändert. Der [Paketbericht](ROCK_POSE_RESULTS.json) verbindet die konkreten Quellen, Assets, EXE, Inhaltscontainer, Bilder und Leistungsmessungen.

## Leistung

RTX 3090 / Ryzen 9 9950X3D, 1080p/Hoch mit Wasserprofil, 100 Prozent Renderauflösung, VSync und Bildratenlimit aus. Keine weitere Unreal-/Spielinstanz war beim Start aktiv. Jeweils 15 Sekunden Aufwärmen; alle drei Messungen verwenden dieselbe EXE und dieselben Inhaltscontainer wie die Szenenprüfung. Die Unterwasserkontrolle schaltet ausschliesslich die vorherige Felsausrichtung ein. Die jüngsten Optimierungen des Komponentenabbaus bleiben in allen Läufen aktiv.

| Messung | Mittel | 1%-Low | P95 | Maximum | Frames über 33,33 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Unterwasser-Kontrolle, vorherige Ausrichtung, 60 s | 68,35 FPS | 58,63 FPS | 16,07 ms | 19,86 ms | 0 / 4099 |
| Unter Wasser, neue Ausrichtung, 60 s | 69,93 FPS | 60,98 FPS | 15,57 ms | 17,58 ms | 0 / 4194 |
| Murg Nachladestrecke, neue Ausrichtung, 90 s | 96,90 FPS | 74,28 FPS | 11,24 ms | 27,01 ms | 0 / 8719 |

Die mittlere GPU-Zeit beträgt in der Unterwasserkontrolle 13,99 ms und mit neuer Ausrichtung 13,66 ms. In diesen Einzelläufen zeigt sich kein Leistungseinbruch. Das ist kein allgemeiner Beschleunigungsnachweis: Sichtbarkeit und Überschneidungen ändern sich, und die variabel getakteten Abläufe sind nicht frameidentisch. Die normale Unterwasseraufnahme zeigt Grund und Felsen bei 5,3 Metern; die Nachladestrecke endet bei 24,7 Metern in freiem Wasser.

Vier Unterwasserframes und zwölf Frames der normalen Nachladestrecke überschreiten 16,67 ms. Durchgehend stabile 60 FPS oder eine Mindesthardware-Freigabe sind damit nicht bewiesen. Laufende Wasser-/Meshprüfungen gehören weiterhin zum erfassten CPU-Aufwand. Ein aktueller Langzeittest und die gesamte visuelle Referenzabnahme bleiben offen. [Leistungsübersicht](PERFORMANCE.md).

Messordner: `artifacts/perf-underwater-1080p-q2-20261127-041907-436` (Kontrolle), `artifacts/perf-underwater-1080p-q2-20261127-042040-688` (neu) und `artifacts/perf-stream-1080p-q2-20261127-042216-373` (Nachladen). Alle Messzeilen bleiben erhalten.

```powershell
./Tools/Build.ps1 -EditorSandbox
./Tools/TestRockPose.ps1 -Preview
./Tools/Build.ps1 -EditorSandbox -Target Package -BuildSlot RockPose -PackageDirectory Builds/WindowsRockPose
./Tools/TestRockPose.ps1 -Packaged -PackageDirectory Builds/WindowsRockPose
```
