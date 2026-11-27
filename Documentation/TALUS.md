# Dichtere Bruchstücke an bestehenden Felsfüssen

Die zusätzliche Geometrie liegt gezielt nahe den vorhandenen Felsblöcken. Sie ergänzt deren Übergang zum Kies und Seegrund, ohne die vermessene Bodenoberfläche oder die Hauptfelsen anzuheben. Der vorherige [Versuch mit einer flächigen Sedimentschicht](SEDIMENT_RELIEF_INVESTIGATION.md) bleibt verworfen.

## Verteilung und Kontakte

Die ersten 24 Bruchstück-Kandidaten jeder Felsgruppe bleiben erhalten. Bis zu 48 zusätzliche Kandidaten liegen näher am jeweiligen grossen Block: Der radiale Abstand wird aus dem 0,45- bis 1,05-fachen Hindernisradius plus einem grössenabhängigen Zusatz berechnet, statt aus dem bisherigen Bereich 0,60–1,25. Zu kleinen Nachbarsteinen ist eine engere Überlappung erlaubt. Die vorhandenen Tiefen-, Hang-, Einstiegs- und Zugangsregeln bleiben aktiv. Offene Flächen fern der grossen Blöcke erhalten dadurch keine zusätzliche flächige Bestreuung.

Grössen, Quellmeshes, Materialien und teilweise eingegrabene Platzierung entsprechen den bestehenden [Felsbruchstücken](ROCK_RUBBLE.md). Die zusätzlichen Instanzen werden in denselben Komponenten gezeichnet und besitzen dieselbe Kontakt- und Hindernisbehandlung. Pflanzen, Kleinkies und Fischwege berücksichtigen die erweiterten Hindernisse; ihre örtliche Verteilung kann sich deshalb ändern. Die Kandidatenzahl je Anker ist auf 22 Hauptsteine plus 72 Bruchstücke begrenzt. Weltfeste Zufallsfolgen und der bestehende Überlappungsrand erhalten eindeutige Zellbesitzer beim Nachladen.

Die Dichte ist gestaltet, nicht aus einer Vermessung einzelner Walensee-Steine abgeleitet. Sie ist auch kein Erosionsmodell. Derselbe Satz von drei Scanformen wiederholt sich weiterhin. Das Gesamtziel benötigt zusätzlich überzeugenderen Bewuchs, abwechslungsreiche Sedimentflächen und eine abgestimmte Lichtwirkung.

## Zuordnung der Testorte

Die direkten Unterwassereinstiege sind Weesen/West, Murg/Süd und Walenstadt/Ost. Der historische Testfallname `ledi-wide` meint hier Index 2 im direkten Wasserstart, also **Walenstadt/Ost**. Er bezeichnet keine Prüfung am separaten Uferzugang Mols/Lediwracks. Diese Verwechslung wurde bei der Prüfung der Benchmark-Aufnahme entdeckt und korrigiert. [Zuordnung und Quellen](TEST_ENTRY_LOCATIONS.md).

## Nativer Vergleich

`artifacts/talus-20261127-045026` besteht zwei Fälle mit sechs Bildern. `-DiveLegacyTalus` schaltet ausschliesslich die zusätzlichen Kandidaten ab. Beide Varianten verwenden exakt dieselben drei Kamerapositionen, dieselbe EXE und dieselben Assets.

Im untersuchten Gebiet Walenstadt/Ost steigt die Zahl der Bruchstücke von 5.279 auf 13.851. Die 5.335 Hauptsteine bleiben identisch: Ein sortierter Hash umfasst Position, Hindernisradius, Modellwahl, Quaternion und Skalierung. Beide Varianten liefern `64ECD2D5496D112DE66CC0B0A0FCC18D6B9027DE`. Die 292 Kontaktproben der Kontrolle und die 656 Proben der neuen Variante treffen die jeweiligen konvexen Körper. Die vorhandenen Prüfungen für stabile Layouts, Nachladefenster, nahe Teleports, begrenzten Komponentenabbau und Fischbewegung bestehen ebenfalls. [Nativer Bericht](TALUS_NATIVE_RESULTS.json).

Die Kontaktproben sind Stichproben an einzelnen Linien, keine Prüfung sämtlicher Dreiecke oder Bewegungsrichtungen. Mehr Instanzen und Hindernisse benötigen eine eigene Leistungsprüfung; die unveränderte Komponentenanzahl garantiert keine unveränderte Bildrate. Die Bilder zeigen zusätzliche räumliche Bruchstücke an den Felsfüssen, erfüllen aber noch nicht die vollständige [Qualitätsreferenz](VISUAL_TARGET.md).

## Vergleich im Windows-Paket

`Builds/WindowsTalus/Start-WaterPreview.cmd` startet die neue Vorschau; der komplette Paketordner wird benötigt. `artifacts/talus-20261127-050336` besteht neun Fälle mit 36 Aufnahmen: direkter Nahvergleich, alle drei direkten Unterwassereinstiege, Darstellung ohne Nanite, Nachladestrecke, Uferwechsel und Sediment. Der zusätzliche automatische Start über den Haupt-Launcher besteht ebenfalls. Ein vollständig manuell bedienter Menü-zu-Tauchgang-Test dieses Pakets steht noch aus.

Alle sieben Fälle mit Populationsprüfung bestehen die erweiterten Kontaktproben sowie die bisherigen Layout-, Nachlade-, Teleport-, Komponentenabbau- und Fischbewegungsprüfungen. Bei Weesen treffen 501 von 501 Bruchstück-Proben, bei Murg 475 von 475 und bei Walenstadt/Ost 656 von 656. Die beiden Walenstadt-Vergleichsvarianten besitzen auch im Paket identische Hauptfels-Transformationen (Hash `814B6F4ADD26A99A7CF51E1E692A2B9B909A2EF8`). Dieser Vergleich gilt innerhalb derselben EXE; buildübergreifende Bitgleichheit wird nicht behauptet. Der [Paketbericht](TALUS_RESULTS.json) verbindet Quellen, Inhalte, Bilder, Launcher und Messungen.

Die folgenden Originalbilder stammen aus dem Windows-Paket. Kontrolle mit bisheriger Verteilung:

![Felsfuss mit bisheriger Verteilung](Images/Talus_Before.png)

Zusätzliche Bruchstücke, identische Kamera:

![Dichteres Geröll am unveränderten Felsfuss](Images/Talus_After.png)

Weitere Ansicht bei Weesen:

![Felsen und Geröllfelder bei Weesen](Images/Talus_Wide.png)

## Leistung

RTX 3090 / Ryzen 9 9950X3D, 1920×1080 bei 100 Prozent Renderauflösung, Hoch mit Wasserprofil, VSync und Bildratenlimit aus. Jeweils 15 Sekunden Aufwärmen, keine weitere Unreal-/Spielinstanz beim Start. Die Unterwasserkontrolle verwendet dieselbe EXE und dieselben Inhalte und schaltet ausschliesslich die zusätzlichen Bruchstücke ab.

| Messung | Mittel | 1%-Low | P95 | Maximum | Frames über 33,33 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Weesen, bisherige Verteilung, 60 s | 70,00 FPS | 57,93 FPS | 15,61 ms | 53,67 ms | 1 / 4198 |
| Weesen, zusätzliche Bruchstücke, 60 s | 69,91 FPS | 60,39 FPS | 15,69 ms | 18,08 ms | 0 / 4193 |
| Walenstadt/Ost, zusätzliche Bruchstücke, 60 s | 76,66 FPS | 58,55 FPS | 14,79 ms | 18,26 ms | 0 / 4597 |
| Murg Nachladestrecke, 90 s | 97,33 FPS | 73,27 FPS | 11,17 ms | 29,74 ms | 0 / 8758 |

Die mittlere GPU-Zeit bei Weesen liegt bei 13,64 ms in der Kontrolle und 13,67 ms mit zusätzlichem Geröll. Die mittlere Spielthread-Zeit steigt von 4,41 auf 5,08 ms. Der zusätzliche CPU-Aufwand bleibt also sichtbar, obwohl der durchschnittliche Bilddurchsatz in diesen Läufen nahezu gleich ist. Der einzelne lange Frame in der Kontrolle bleibt vollständig im Ergebnis; seine Ursache ist nicht nachgewiesen. Aus dem unterschiedlichen Maximum folgt kein Beschleunigungsnachweis.

Zehn Frames des neuen Weesen-Laufs, 38 Frames bei Walenstadt/Ost und 14 Frames der Nachladestrecke überschreiten 16,67 ms. Die Messungen beweisen keine durchgehend stabilen 60 FPS. Der Weesen-Blick zeigt Grund und Felsen bei 5,3 Metern; Walenstadt/Ost zeigt Felsen bei 4,3 Metern. Die Nachladestrecke endet bei 24,4 Metern in freiem Wasser. Walenstadt/Ost erreicht 6,09 ms mittlere Spielthread- und 12,41 ms mittlere GPU-Zeit. Laufende Wasser-/Meshprüfungen gehören zum erfassten Aufwand; die Aufnahmen und Abläufe sind nicht frameidentisch. Ein aktueller Langzeittest und die vollständige Referenzabnahme bleiben offen.

Messordner: `artifacts/perf-underwater-1080p-q2-20261127-050036-155` (Kontrolle), `artifacts/perf-underwater-1080p-q2-20261127-050203-662` (Weesen neu) und `artifacts/perf-stream-1080p-q2-20261127-045740-989` (Murg Nachladen). Der zusätzliche Walenstadt-Lauf liegt unter `artifacts/perf-underwater-1080p-q2-20261127-051145-902`.
