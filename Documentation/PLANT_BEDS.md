# Pflanzengruppen im Flachwasser

Weiterentwicklung: [niedriger Bewuchs zwischen den höheren Trieben](PLANT_UNDERSTORY.md). Die folgenden Zahlen dokumentieren den vorherigen Gruppenstand. **Ortskorrektur:** Die unten als „Lediwracks“ bezeichneten direkten Unterwasserprüfungen mit Index 2 liegen bei Walenstadt/Ost; sie prüfen nicht den separaten Uferzugang Mols/Lediwracks. [Zuordnung](TEST_ENTRY_LOCATIONS.md).

Die bisher einzeln verteilten Wasserpflanzen wachsen jetzt in kleinen, unregelmässigen Gruppen. Innerhalb einer Gruppe überwiegt eine geeignete Art. Randpflanzen bleiben niedriger; dazwischen bleiben offene Sedimentflächen. Das ist eine gestalterische Verteilung, keine vermessene Vegetationskarte des Walensees. Pflanzenmodelle, Materialien und Geländehöhen bleiben unverändert.

## Platzierung

Ein weltweites Vier-Meter-Raster erzeugt versetzte Gruppenanker. Kontinuierliche Verteilungsfelder bestimmen die Belegung, Tiefe, Bodenneigung und Sichtweite die geeigneten Arten. Bis zu sechs Kandidaten liegen innerhalb von 1,7 Metern um einen Anker. Der letzte kann eine weitere geeignete Art verwenden. Zufällige Breiten, Drehungen und nach aussen abnehmende Höhen brechen die Gleichförmigkeit auf.

Felsgrundflächen einschliesslich Bruchstücken erhalten zehn Zentimeter zusätzlichen Abstand; um Ein- und Ausstiege bleiben 3,5 Meter frei. Die skalierte Pflanzenhöhe muss mindestens 30 Zentimeter unter der Wasseroberfläche bleiben. Innerhalb einer Gruppe liegen Wurzeln mindestens 35 Zentimeter auseinander. Dieser Mindestabstand gilt nicht zwischen verschiedenen Gruppen. Jede Wurzel gehört genau einer 96-Meter-Populationszelle. Ein vollständiger Gruppenrand wird vor der Zellzuordnung ausgewertet, damit Nachladen keine Gruppen an Zellgrenzen abschneidet. Die Felsvorauswahl nutzt das räumliche Suchraster; eine genaue Abstandskontrolle entscheidet weiterhin über die Platzierung.

## Nativer Vergleich und Prüfung

Der [native Bericht](PLANT_BEDS_NATIVE_RESULTS.json) bindet zwei Fälle mit sechs Aufnahmen an Quellen und Assets. Die niedrige Vergleichskamera hängt ausschliesslich vom Gelände ab und ist vor und nach der Änderung identisch. Das geladene Weesener Fenster enthält vorher 2.422, danach 2.171 Pflanzen; darunter 840 mit Höhenfaktor unter 0,55 und 297 über 0,8. Der erste Entwurf mit grösseren Verteilungsflächen bestand die technischen Prüfungen, liess diesen Nahbereich aber zu leer. Er wurde visuell verworfen und bleibt als gesonderter Versuch im Bericht erhalten.

Die Laufzeitprüfung kontrolliert eindeutige Wurzeln, Zellzuordnung, Bodenkontakt, Lebensraumbedingungen, Felsabstand, Einstiegsfreiraum und tatsächliche Instanztransformationen. Eine Regeneration mit vollständigen benachbarten Felslisten prüft zusätzlich die gekürzte Felsvorauswahl der Produktion. Die Prüfung wird nach dem Nachladen und der Rückkehr ins ursprüngliche Fenster wiederholt. Das ersetzt keine visuelle Abnahme oder Langzeitprüfung.

Die Paketfenster enthalten bei Murg 101 Pflanzen (27 niedrig, 17 hoch) und bei Lediwracks 791 (294 niedrig, 118 hoch). Diese Zähler beschreiben geladene Instanzen, nicht die im einzelnen Kamerabild sichtbaren Pflanzen. Die steile Murg-Nahansicht zeigt weiterhin weitgehend freien Felsgrund.

Gleiche Weesener Nahkamera mit bisheriger Verteilung:

![Bisherige Verteilung](Images/PlantBeds_Before.png)

Gruppen mit nach aussen abnehmender Höhe:

![Pflanzengruppen](Images/PlantBeds_After.png)

## Geprüftes Windows-Paket

`Builds/WindowsPlantBedsReady/divingsim.exe` enthält Laufzeitstand `96f0f2560d304145962136420308d227c7b42615`. Die Spiel-EXE hat SHA-256 `3beaee712b3092082996807b822f2f9e7e60dfcf999c596cc3cf49d064ce2e13`. Der Lauf `artifacts/plant-beds-20261126-233755` besteht acht Fälle mit 31 Aufnahmen: bisherige und neue Verteilung, Murg, Lediwracks, Ersatzdarstellung ohne Nanite, Schwimmen/Nachladen, Uferablauf sowie Fotoentdeckungen mit Archiv und Logbuch. Der normale Launcher-Start besteht ebenfalls. Die Entdeckungsprüfung bestätigt Funktion und Speicherung, nicht die gestalterische Qualität des bisherigen Menüs oder der Testfotos.

Beide CI-Läufe des Laufzeitstands bestehen: [Push](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45905506106), [Pull Request](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45905512537). Der [Paketbericht](PLANT_BEDS_RESULTS.json) bindet Quellen, Assets, EXE, Inhaltscontainer, Aufnahmen und Messungen. Reproduktion: `Tools/TestPlantBeds.ps1 -Packaged`; ergänzend `Tools/TestPackageLauncher.ps1` und die im Bericht genannten Leistungsläufe.

Bei 1080p/Hoch erreicht der 60-Sekunden-Unterwasserlauf 74,76 FPS im Mittel (1%-Low 64,01), ohne Bilder über 33,33 ms. Die 90-Sekunden-Nachladestrecke erreicht 100,89 FPS (1%-Low 80,03), enthält aber zwei längere Bilder bis 43,00 ms. Die mittleren Spielthreadzeiten von 3,49 / 3,08 ms erhalten die CPU-Reserve des vorherigen Suchrasterstands. [Messbedingungen und Grenzen](PERFORMANCE.md). Ein aktueller Langzeittest und die vollständige visuelle Abnahme bleiben offen.

## Korrigierter Diagnoseablauf und offene Qualität

Der erste Windows-Prüflauf (`artifacts/plant-beds-20261126-232628`, Quellstand `7eb222a`) besteht den Vorher-/Nachhervergleich sowie Murg und Lediwracks. Der folgende Fall ohne Nanite erzeugt seine drei Bilder, führt aber die angeforderten Weltprüfungen nicht aus: Die Aufnahme friert den Welttick nach vier Sekunden Aufnahmezeit ein; bei langsamen Startbildern hat die begrenzte Simulationszeit die Zwei-Sekunden-Prüfschwelle noch nicht erreicht. Der Prüftreiber weist den Lauf deshalb zurück. Die Korrektur wartet ausdrücklich auf die angeforderten Weltprüfungen, bevor sie die Szene einfriert, und bricht bei ausbleibender Bereitschaft nach 30 Sekunden ab. Sie ändert nur den Diagnoseablauf. Der unvollständige Lauf bleibt erhalten.

Ein zusätzlicher nativer Lauf mit bewusst begrenzten zwei Bildern pro Sekunde und ohne Nanite besteht: beide Pflanzenprüfungen und `DIVE_POPULATION_PASS` stehen vor der ersten Aufnahme im Log. Der vollständige korrigierte Paketlauf besteht anschliessend auch den zuvor unvollständigen Rasterfall. Der frühere native Vergleichsbericht bleibt an seinen damaligen Quellstand gebunden; die Pflanzenverteilung änderte sich durch diese Diagnosekorrektur nicht.

Die Gruppen sind in der Nahansicht erkennbar; die bestehenden dünnen, hellen Pflanzenmodelle wirken weiterhin künstlich. Felsoberflächen, Sedimentübergänge, Licht und Wasser benötigen weitere Arbeit für das [visuelle Ziel](VISUAL_TARGET.md). Dieser Schritt ist keine Abnahme der Unterwasserwelt. Danach folgen die Überwasserlandschaft und erst anschliessend das bereits festgehaltene [Menü-/HUD-/FPV-Design](UI_UX_TARGET.md).
