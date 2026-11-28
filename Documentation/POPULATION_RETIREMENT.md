# Physikarbeit beim Wechsel der Fels- und Pflanzenbereiche verteilen

Nach der [Kiesoptimierung](GRAVEL_RETIREMENT.md) blieb auf der Nachladestrecke eine Spitze von 45,96 ms. Sie fiel mit dem gleichzeitigen Entfernen dreier größerer Fels-/Pflanzenzellen zusammen; der direkte Kiesabbau benötigte dabei nur 0,021 ms. Diese Änderung verteilt nun auch das Entfernen der größeren Komponenten, ohne Anzahl, Form oder Position der aktiven Felsen und Pflanzen zu reduzieren.

## Lebensdauer und Grenzen

Die neun aktiven Zellen sind weiterhin jeweils 96 Meter breit. Verlassene Komponenten werden an derselben Grenze wie zuvor sofort unsichtbar. Ihre registrierten Kollisionskörper bleiben kurz bestehen; normalerweise werden zwei Komponenten pro Aufruf vollständig zerstört. Beim normalen Überqueren einer Zellgrenze liegen die ausgeschiedenen Zellen mindestens 96 Meter entfernt. Es wird kein zusätzlicher Zeichenabstand für Felsen oder Pflanzen eingeführt.

Schnelle Richtungswechsel können vorübergehend alte und neu erzeugte Komponenten desselben Bereichs enthalten. Eine Aufholregel begrenzt die Warteschlange nach jedem Aufruf auf neun Zellbestände, aktuell 54 Komponenten. Dabei darf der Abbau mehr als zwei Komponenten umfassen. Ein festes Millisekundenbudget ist damit nicht garantiert. Die Warteschlange wird auch im Stillstand weiter geleert.

Beim Start, bei vollständigem Wechsel des Zellfensters und sobald fehlende Geometrie innerhalb von 80 Metern liegt, werden alte Komponenten sofort entfernt und die benötigten Zellen sofort geladen. Die bestehende Ausnahme für Teleports innerhalb benachbarter Zellen bleibt erhalten. Die Felsdaten für Kollision und Fisch-Ausweichverhalten beschreiben weiterhin das vollständige Zielgebiet; nur entfernter Komponentenaufbau und -abbau werden verteilt.

## Laufzeitprüfung

Der native Test `artifacts/gravel-relief-20261127-031605` besteht beide Fälle mit sechs Aufnahmen. 20 schnelle Richtungswechsel erreichen höchstens 54 zurückgestellte Komponenten; diese bleiben unsichtbar und werden anschließend vollständig abgebaut. Alle neun ursprünglichen Zelllayouts stimmen danach exakt wieder überein. Die vorhandenen Prüfungen für nahe Teleports, erhaltene Fischzustände, Felslayouts, Pflanzenbereiche und Kieskontakte bleiben aktiv. [Nativer Bericht](POPULATION_RETIREMENT_NATIVE_RESULTS.json).

Neue CSV-Zeiten erfassen Aufbau, Instanzanlage, Registrierung und Abbau der großen Zellen getrennt. Die Engine erfasst die nachgelagerte Physikwartezeit weiterhin separat. `-DiveImmediatePopulationRetirement` stellt ausschließlich im Smoke-Test den sofortigen Abbau wieder her. Der Vergleich verwendet dieselbe EXE und dieselben Inhaltscontainer; die bereits optimierte Kiesverwaltung bleibt in beiden Varianten aktiv.

## Kontrollmessung aus derselben EXE

RTX 3090 / Ryzen 9 9950X3D, 1080p/Hoch mit Wasserprofil, 100 Prozent Renderauflösung, VSync und Bildratenlimit aus. Beide Läufe verwenden 15 Sekunden Aufwärmen und 90 Sekunden Schwimm-/Nachladestrecke ab Murg. Beim Start lief keine weitere Unreal-/Spielinstanz. Es handelt sich um zwei aufeinanderfolgende Einzelläufe mit variabler Framerate, nicht um eine statistische Messreihe oder frameidentische Wiederholung.

| Messwert | Sofortiger Abbau | Verteilter Abbau |
| --- | ---: | ---: |
| Mittlere Bildrate | 97,10 FPS | 97,17 FPS |
| 1%-Low | 70,48 FPS | 75,58 FPS |
| P95-Framezeit | 11,18 ms | 11,22 ms |
| Größte Framezeit | 45,47 ms | 25,74 ms |
| Frames über 33,33 ms | 3 / 8737 | 0 / 8743 |
| Frames über 16,67 ms | 12 | 11 |
| Fels-/Pflanzenabbau: höchste direkte Zeit | 8,70 ms | 2,63 ms |
| Fels-/Pflanzenaufbau: höchste direkte Zeit | 11,48 ms | 11,22 ms |
| Komponentenregistrierung: Maximum | 8,86 ms | 8,76 ms |
| Physikwartezeit: Maximum | 22,20 ms | 12,12 ms |

Die größte Kontrollspitze bei etwa 62,39 Sekunden enthält 8,70 ms direkten Populationsabbau und 22,20 ms Physikwartezeit. Die CSV-FrameTime misst den Abstand zum vorherigen Frame; die zugehörige Arbeit steht in der vorangehenden Messzeile. Die zwei weiteren Kontrollspitzen bei etwa acht Sekunden fallen mit Kiesarbeit zusammen. Deren Unterschied wird nicht allein der neuen Änderung zugeschrieben: Trotz gleicher EXE und Einstellungen sind die Läufe nicht zeitlich identisch.

Der Befund stützt das Verteilen des großen Komponentenabbaus. Dessen höchste direkte Zeit von 2,63 ms gehört im verteilten Lauf zu einem Frame von 15,15 ms mit 8,86 ms Physikwartezeit. Die größte verbleibende Framezeit von 25,74 ms fällt dagegen mit Kiesarbeit zusammen. Aufbau und Registrierung bleiben teuer; der Gesamtaufwand verschwindet nicht. Auch der verbesserte Lauf enthält elf Frames über 16,67 ms und belegt daher keine durchgehend 60 FPS. Die Strecke endet bei 24,6 Metern in freiem Wasser, nicht in einer dauerhaft dichten Nahansicht.

Die vollständigen Messungen liegen unter `artifacts/perf-stream-1080p-q2-20261127-032220-949` (sofort) und `artifacts/perf-stream-1080p-q2-20261127-032435-127` (verteilt). Kein Ruckler wurde aus der Auswertung entfernt.

## Geprüftes Spielpaket

Die Vorschau startet über `Builds/WindowsPopulationRetire/Start-WaterPreview.cmd`; der gesamte Paketordner wird benötigt. Der Starter öffnet das normale Menü mit dem aktuellen Wasserprofil. Der zusätzliche automatische Haupt-EXE-Test besteht. Ein vollständig manuell bedienter Menü-zu-Tauchgang-Test dieses Pakets steht noch aus.

`artifacts/gravel-relief-20261127-032641` besteht alle neun Fälle mit 36 Aufnahmen: beide Nahvergleiche, die drei Einstiegsorte, Darstellung ohne Nanite, Schwimm-/Nachladestrecke, Uferwechsel und Sediment. In allen sieben Fällen mit Populationsprüfung erreicht die Warteschlange höchstens 54 Komponenten und wird vollständig geleert; die neun ursprünglichen Layouts werden exakt wiederhergestellt. Die Kieswarteschlange bleibt auf höchstens 26 Komponenten begrenzt. Die unveränderten nativen Eingabedateien wurden nach dem Paketbau erneut über ihre Prüfsummen bestätigt.

![Erhaltenes Bodenrelief im geprüften Paket](Images/PopulationRetirement_After.png)

`PopulationRetirement_Before.png` zeigt die sparsame Diagnoseverteilung derselben Szene, nicht den sofortigen Komponentenabbau. Die Abbaupolitik ändert die Grafik nicht absichtlich. Die Sichtprüfung bestätigt erhaltene Geometrie; flache Bodenbereiche, Pflanzen, Lichtwirkung und weitere Umgebungsdetails erreichen die Nutzerreferenz noch nicht.

Der [Paketbericht](POPULATION_RETIREMENT_RESULTS.json) verknüpft Quellen, Assets, EXE, Inhaltscontainer und Aufnahmen mit dem Launcher-Test und den Leistungsmessungen.

Der zusätzliche 60-Sekunden-Unterwasserlauf `artifacts/perf-underwater-1080p-q2-20261127-033423-863` erreicht 68,64 FPS im Mittel, 59,42 FPS im 1%-Low und maximal 19,95 ms pro Frame. 19 von 4117 Frames überschreiten 16,67 ms; keines überschreitet 33,33 ms. Die Aufnahme zeigt sichtbaren Grund und Felsen bei 5,3 Metern. Messbedingungen und Grenzen stehen in der [Leistungsübersicht](PERFORMANCE.md).

```powershell
./Tools/Build.ps1 -EditorSandbox
./Tools/TestGravelRelief.ps1 -Preview
./Tools/Build.ps1 -EditorSandbox -Target Package -BuildSlot PopulationRetire -PackageDirectory Builds/WindowsPopulationRetire
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsPopulationRetire -WaterPreview -Scenario stream -Entry 1 -Seconds 90 -ImmediatePopulationRetirement
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsPopulationRetire -WaterPreview -Scenario stream -Entry 1 -Seconds 90
./Tools/TestGravelRelief.ps1 -Packaged -PackageDirectory Builds/WindowsPopulationRetire
```

Die vollständige visuelle Referenzqualität, ein aktueller Langzeittest und dauerhaft stabile Bildzeiten sind damit noch nicht nachgewiesen. Überwasserwelt und anschließend Menü/HUD/FPV folgen weiterhin der vereinbarten Reihenfolge.
