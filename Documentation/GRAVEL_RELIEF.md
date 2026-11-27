# Dichteres Kiesrelief im Nahbereich

Kleine räumliche Steine füllen die vorhandenen Kiesflächen und unregelmäßigen Felsfußbereiche dichter aus. Die zuvor überwiegend flach wirkende Bodentextur erhält dort zusätzliche Silhouetten, Eigenverschattung und Kontaktflächen. Die größeren Formationen und Pflanzen behalten ihre Verteilung. Das ist ein weiterer Schritt zum [visuellen Ziel](VISUAL_TARGET.md), keine vollständige Referenzabnahme.

Die spielbare Grafikvorschau startet mit `Builds/WindowsGravelRelief/Start-WaterPreview.cmd`. Sie enthält auch die vorherige Korrektur der oberflächennahen Nebelkante. Der komplette Paketordner wird benötigt. Der normale Weg vom Menü zum Tauchgang wurde in diesem Paket nicht manuell durchgespielt; der automatische Haupt-EXE-Start ist geprüft.

## Verteilung und Nachladen

Die Kandidatendichte steigt von vier auf 32 pro Quadratmeter. Die vorhandenen Kies-/Sedimentmasken, Tiefengrenzen und Ausschlussbereiche bestimmen weiterhin, welche Kandidaten tatsächlich Steine werden. Es entsteht kein flächendeckender Steinbelag: Sedimentflächen bleiben offen, während felsnahe Bereiche mehr Kleingeröll erhalten. Drei vorhandene Formen, variierende Größen, Neigungen und teilweise eingegrabene Steine vermeiden eine regelmäßige Anordnung. Die sichtbare Höhe bleibt innerhalb der bisherigen sechs Zentimeter relativ zum vermessenen Grund; die kleinen Steine behalten ihre Abfragekollision.

Die Nachladezellen sind nun 16 statt 32 Meter breit. Neun Zellen halten die lokale Umgebung vor; insgesamt gibt es höchstens 73.728 Kandidaten. Beim Schwimmen kommt weiterhin höchstens eine neue Zelle pro Frame hinzu. Der konfigurierte maximale Zeichenabstand beträgt 15 Meter und liegt innerhalb des erhaltenen Nachbarfensters. Die kleinen Steine ergänzen die vorhandenen Bodenmaterialien im Nahbereich. Größere Felsen haben unverändert ihren eigenen, weiter reichenden Darstellungsbereich.

Vier-Meter-Suchfelder enthalten die Felsen, deren konservativ begrenzter Einfluss sie berührt. Dadurch prüft ein Kieskandidat nur die örtlich relevanten Felsen. Reihenfolge und genaue Distanz-/Dichteformeln bleiben erhalten. Die Grenzen der Suchfelder enthalten den gesamten größten Felsfußbereich einschließlich dessen zusätzlicher 60 Zentimeter; sie verändern keine Annahmekriterien.

Die Umstellung des Zellrasters verändert die einzelnen Kiespositionen. Weltfeste Zufallsfolgen sorgen dafür, dass sie beim Verlassen und erneuten Laden derselben Stelle identisch zurückkehren. Einzelne Steine sind gestaltet, nicht vermessen.

## Vergleich aus dem Spielpaket

Gleiche Nahkamera mit bisheriger Kandidatendichte auf dem neuen Zellraster:

![Dünnere Kiesverteilung im Diagnosevergleich](Images/GravelRelief_Before.png)

Neue Dichte mit zusätzlichem räumlichem Relief an den Felsfüßen:

![Dichteres Kleingeröll zwischen Felsen und Seegrund](Images/GravelRelief_After.png)

Im lokalen Neun-Zellen-Fenster der Ledi-Prüfung stehen 20.201 statt 2.526 Kiesel. 3.378 statt 407 liegen innerhalb der untersuchten großen Felsradien. Diese Radien umschließen die ganzen Blöcke, nicht deren exakte Kontaktflächen; die Zahl misst deshalb keine sichtbare Bodenabdeckung. Die dichtere Variante besteht 216 Kollisionsproben.

## Prüfung und erster Versuch

Der native Vergleich `artifacts/gravel-relief-20261127-021831` besteht zwei Fälle mit sechs Aufnahmen. Gleiche Kamerapositionen zeigen die neue Dichte gegenüber einer Diagnosevariante mit alter Kandidatendichte auf dem **neuen** 16-Meter-Raster. Das ist kein exakter Nachbau der früheren 32-Meter-Positionen. `-DiveSparseGravel` ist nur im Smoke-Test aktiv. Der [native Prüfbericht](GRAVEL_RELIEF_NATIVE_RESULTS.json) bindet Quellen, Materialien, Editor-Modul und Aufnahmen.

Die Laufzeitprüfung vergleicht erhaltene und erneut geladene Transformationen, prüft die schrittweise Wiederherstellung des Neun-Zellen-Fensters sowie Kollisionskontakte an Stichproben. Bildunterschiede bestätigen eine sichtbare Änderung; sie bewerten nicht automatisch deren gestalterische Qualität.

Der erste Versuch `artifacts/gravel-relief-20261127-021527` verwendete die achtfache Dichte noch auf dem alten 32-Meter-Raster. Er bestand die Funktionsprüfungen, baute aber einzelne neue Zellen im untersuchten Ledi-Vergleich in etwa 76–83 ms auf. Die kleineren Zellen reduzieren diese ausgewählten Aufbauzeiten im zweiten Vergleich auf etwa 6–20 ms. Beide Versuche bleiben archiviert; diese Einzelwerte sind keine Bildratenmessung und vergleichen unterschiedliche Zellflächen.

Der erste Paketaufruf verwendete versehentlich das veraltete Editor-Modul im Hauptprojekt. Dort wurde die Klasse `DiveCoastBakeCommandlet` nicht gefunden; die Vorprüfung brach vor dem Paketbau ab. Das bedeutet keinen nachgewiesenen Fehler der Küstenassets. Der korrigierte Aufruf verwendet das aktuelle Editor-Sandbox-Projekt. Das ursprüngliche Protokoll bleibt unter `artifacts/gravel-relief-package-build.log` erhalten.

```powershell
./Tools/Build.ps1 -EditorSandbox
./Tools/TestGravelRelief.ps1 -Preview
./Tools/Build.ps1 -EditorSandbox -Target Package -BuildSlot GravelRelief -PackageDirectory Builds/WindowsGravelRelief
./Tools/TestGravelRelief.ps1 -Packaged -PackageDirectory Builds/WindowsGravelRelief
```

Der vollständige Paketlauf `artifacts/gravel-relief-20261127-022420` besteht neun Fälle mit 36 Aufnahmen: beide Nahvergleiche, alle drei Einstiege, Rasterdarstellung ohne Nanite, Schwimm-/Nachladestrecke, Uferwechsel und Sediment. Zusätzlich besteht der automatische Haupt-EXE-Start. Der [Paketbericht](GRAVEL_RELIEF_RESULTS.json) bindet die geprüften Quellen, Assets, EXE, Inhaltscontainer, Kameras, Bilder und beiden Leistungsläufe.

## Leistung und offene Punkte

Bei 1080p/Hoch mit Wasserprofil auf RTX 3090 / Ryzen 9 9950X3D erreicht der Unterwasserlauf 68,74 FPS im Mittel, gegenüber 69,15 FPS im vorherigen Paket. Die Nachladestrecke erreicht 97,41 FPS gegenüber 97,33 FPS. Die Mittelwerte allein verschleiern die verbleibenden Ruckler: Drei Frames überschreiten 33,33 ms, der schlechteste erreicht 57,21 ms statt zuvor 42,81 ms. Der 1%-Low der Strecke sinkt von 77,58 auf 72,36 FPS.

Die zwei größten Spitzen bei ungefähr 6,86 und 17,71 Sekunden nach Messbeginn fallen zeitlich mit Kies-Zellwechseln zusammen; die dritte liegt bei einem Wechsel der größeren Population. Die maximale Spielthreadzeit beträgt 56,96 ms, die GPU bleibt in der gesamten Strecke unter 11,42 ms. Das stützt CPU-/Nachladeaufwand als nächste Untersuchungsrichtung, isoliert aber noch nicht die einzelnen Kosten von Abbau, Aufbau und Physik. Kein erneuter Lauf ersetzt diese Messung. [Vollständige Leistungsdaten](PERFORMANCE.md).

Gleichmäßige Bodenbereiche, wiedererkennbare Felsformen und vereinfachte Vegetation bleiben offene Grafikaufgaben. Die Vorschau erfüllt weder die vollständige Referenzqualität noch eine durchgehende 60-FPS-Abnahme oder einen aktuellen Langzeittest. Überwasserwelt und anschließend Menü/HUD/FPV folgen in der vereinbarten Reihenfolge.
