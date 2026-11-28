# Physikarbeit beim Kieswechsel verteilen

Die dichtere [Kiesverteilung](GRAVEL_RELIEF.md) bleibt erhalten. Diese Änderung verteilt das Entfernen alter Kieskomponenten und ihrer Kollisionskörper auf mehrere Frames. Sie soll die CPU-Spitzen an Zellgrenzen verringern, ohne die Geometrie, lokale Steindichte oder Kollisionsform der aktiven Bereiche zu reduzieren.

Die geprüfte Vorschau startet über `Builds/WindowsGravelRetire/Start-WaterPreview.cmd`. Der gesamte Paketordner wird benötigt. Der Starter öffnet das normale Menü mit dem aktuellen Wasserprofil. Der automatische Haupt-EXE-Start ist geprüft; ein vollständig manuell bedienter Menü-zu-Tauchgang-Test dieses Pakets steht noch aus.

## Befund und Lebensdauer

Im vorherigen Nachladelauf `artifacts/perf-stream-1080p-q2-20261127-023324-968` erreicht ein Frame 57,21 ms. Der dazugehörige Spielthread arbeitet 56,96 ms; etwa 35,85 ms davon entfallen auf das Warten auf die Physik. Die zweitgrößte Spitze enthält etwa 29,88 ms Physikwartezeit. Die CSV-FrameTime bildet den Abstand zum vorherigen Frame ab; die ausführenden Threadwerte liegen daher in der jeweils vorangehenden Messzeile. Die Ereignisse fallen zeitlich mit Kies-Zellwechseln zusammen. Das begründet die Untersuchung des massenhaften Entfernens von Kollisionskörpern, isoliert dessen Anteil aber noch nicht.

Verlassene Zellen liegen bei normaler Bewegung mindestens 16 Meter entfernt, außerhalb des auf 15 Meter begrenzten Kies-Zeichenabstands. Ihre Komponenten werden sofort unsichtbar. Normalerweise werden höchstens zwei davon pro Frame vollständig zerstört. Die noch registrierten Kollisionskörper dürfen kurz weiter existieren; sie werden nicht mit abgeschalteter Kollision in aktive Zellen übernommen. So verteilt sich auch die nachgelagerte Physikarbeit.

Beim Start und bei größeren Teleports werden alte Komponenten sofort abgebaut. Schnelle Richtungswechsel können sonst wiederholt alte Komponenten erzeugen; deshalb begrenzt eine zusätzliche Aufholregel die Warteschlange nach jedem Aufruf auf höchstens 27 Komponenten. In diesem Sonderfall darf der Abbau mehr als zwei Komponenten pro Frame umfassen. Es wird kein allgemeines Zeitbudget behauptet.

## Prüfung

Der native Vergleich `artifacts/gravel-relief-20261127-024610` besteht beide Fälle mit sechs Aufnahmen. Die erweiterte Laufzeitprüfung erzeugt 20 schnelle Richtungswechsel, prüft die Unsichtbarkeit und Begrenzung der zurückgestellten Komponenten, lässt den Abbau im Stillstand auslaufen und vergleicht anschließend alle neun Zelllayouts mit den ursprünglichen Transformationen. Im dichten Fall werden höchstens 26 Komponenten zurückgestellt; am Ende ist die Warteschlange leer. Die vorhandenen Kollisionsproben und Prüfungen der erhaltenen bzw. neu geladenen Zellen bleiben aktiv. [Nativer Bericht](GRAVEL_RETIREMENT_NATIVE_RESULTS.json).

## Kontrollmessung aus derselben EXE

RTX 3090 / Ryzen 9 9950X3D, 1080p/Hoch mit Wasserprofil, 100 Prozent Renderauflösung, VSync und Bildratenlimit aus, 15 Sekunden Aufwärmen und anschließend jeweils 90 Sekunden Schwimm-/Nachladestrecke. Beide Starts melden keine weitere Unreal-/Spielinstanz. Es sind zwei einzelne, aufeinanderfolgende Läufe mit variabler Framerate, keine statistische Messreihe oder framegenaue Wiederholung.

| Messwert | Sofortiger Abbau | Verteilter Abbau |
| --- | ---: | ---: |
| Mittlere Bildrate | 97,30 FPS | 97,11 FPS |
| 1%-Low | 72,35 FPS | 72,97 FPS |
| Größte Framezeit | 56,94 ms | 45,96 ms |
| Frames über 33,33 ms | 3 / 8756 | 1 / 8738 |
| Frames über 16,67 ms | 9 | 12 |
| Kiesabbau: höchste direkte Zeit | 12,72 ms | 3,72 ms |
| Kiesaufbau: höchste direkte Zeit | 4,17 ms | 4,87 ms |
| Physikwartezeit: Maximum | 35,18 ms | 21,85 ms |

Die Spitzen werden kleiner, aber auf mehr moderat langsame Frames verteilt. Der Gesamtaufwand verschwindet nicht. Die letzte Spitze bei etwa 61,26 Sekunden im verteilten Lauf fällt mit `DIVE_POPULATION_CELL` für die größeren Fels-/Pflanzenzellen zusammen. Der Kiesabbau misst dort lediglich 0,021 ms direkt; die genaue Aufteilung der übrigen Population-/Physikarbeit bleibt weiter zu untersuchen. Diese Beobachtung stützt eine weitere Untersuchung der größeren Zellen, statt die Steindichte erneut zu reduzieren.

Die beiden Messordner sind `artifacts/perf-stream-1080p-q2-20261127-025221-458` (sofort) und `artifacts/perf-stream-1080p-q2-20261127-025428-832` (verteilt). CSV, Metadaten und Runtime-Protokolle bleiben vollständig erhalten; kein Ruckler wird aus der Auswertung entfernt.

## Geprüftes Spielpaket

`artifacts/gravel-relief-20261127-025642` besteht alle neun Fälle mit 36 Aufnahmen: beide Nahvergleiche, die drei Einstiegsorte, Rasterdarstellung ohne Nanite, Schwimm-/Nachladestrecke, Uferwechsel und Sediment. In allen sieben Fällen mit Populationsprüfung bleibt der Rückstand auf höchstens 26 Komponenten begrenzt und wird vollständig abgebaut; die ursprünglichen Layouts werden exakt wiederhergestellt. Der Haupt-EXE-Launcher besteht seinen zusätzlichen automatischen Tauchlauf.

Der [Paketbericht](GRAVEL_RETIREMENT_RESULTS.json) verknüpft Quellen, Assets, EXE, Inhaltscontainer und Aufnahmen mit den drei Leistungsläufen und den neuen Zeitmessungen. Das dichte Nahrelief ist im Paket weiterhin sichtbar:

![Erhaltenes Kiesrelief im Paket mit verteiltem Physikabbau](Images/GravelRetirement_After.png)

Die im Bericht gebundene Datei `GravelRetirement_Before.png` zeigt die sparsame Diagnoseverteilung, nicht den sofortigen Physikabbau. Die Grafik wird durch die Abbaupolitik nicht absichtlich verändert.

Der zusätzliche 60-Sekunden-Unterwasserlauf erreicht 68,56 FPS im Mittel, 58,95 FPS im 1%-Low und maximal 18,72 ms pro Frame. 36 Frames überschreiten 16,67 ms; keines überschreitet 33,33 ms. Die [Leistungsübersicht](PERFORMANCE.md) hält Messbedingungen und Grenzen fest.

`-DiveImmediateGravelRetirement` stellt ausschließlich im Smoke-Test den sofortigen Abbau her. Der Leistungsvergleich kann deshalb dieselbe EXE, dieselben Inhaltscontainer, dieselbe Geometrie und dieselben Grafikparameter verwenden. Neue CSV-Zeiten erfassen Kies-Aufbau, Registrierung, Instanzanlage und Abbau. Zusätzlich protokolliert jeder Zellaufbau die Zeit für die Positionsberechnung, Komponenten-Vorbereitung, Instanzanlage und Registrierung. Die nachgelagerte Physikwartezeit bleibt separat in der Engine-Messung sichtbar.

```powershell
./Tools/Build.ps1 -EditorSandbox
./Tools/TestGravelRelief.ps1 -Preview
./Tools/Build.ps1 -EditorSandbox -Target Package -BuildSlot GravelRetire -PackageDirectory Builds/WindowsGravelRetire
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsGravelRetire -WaterPreview -Scenario stream -Entry 1 -Seconds 90 -ImmediateGravelRetirement
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsGravelRetire -WaterPreview -Scenario stream -Entry 1 -Seconds 90
```

Die gesamte grafische Referenzqualität, dauerhaft stabile Bildzeiten und ein aktueller Langzeittest bleiben offen. Die Arbeit an Überwasserwelt und anschließend Menü/HUD/FPV folgt weiterhin der vereinbarten Reihenfolge.
