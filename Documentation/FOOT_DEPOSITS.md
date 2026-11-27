# Örtliche Ablagerungen an Felsfüssen

**Spielbare Grafikvorschau: `Builds/WindowsFootDeposits/Start-WaterPreview.cmd`.** Sie besteht die Funktionsprüfungen, enthält aber zusätzliche Leistungsspitzen; `WindowsFormations` bleibt als vorherige Vorschau erhalten. Die neue Auflage ergänzt vorhandene grosse Felsgruppen um räumliche Böschungen. Sie verbessert die bisher weitgehend ebenen Anschlüsse zwischen Fels, Geröll und Sediment. Die vollständige Unterwasserreferenz ist damit weiterhin nicht erreicht.

## Paketstand und Vergleich

Das Paket unter `Builds/WindowsFootDeposits` besteht sieben Fälle mit 30 Aufnahmen sowie den automatischen Start über die Haupt-EXE. Die drei direkten Unterwasserorte, der Uferweg bei Weesen, Nachladen und Sediment sind geprüft. Die Kontaktzahlen stimmen mit dem nativen Lauf überein. Im Sedimenttest entstehen 21 Wolken; am Ende sind null aktiv und ihre optische Summe ist null. Der [Paketbericht](FOOT_DEPOSITS_RESULTS.json) bindet diese Nachweise an EXE, Inhaltscontainer, Quellstand und sechs Messungen. Der bekannte fehlende CelestialVault-Sternmaterialeintrag bleibt im Fehlerprotokoll enthalten; daraus folgt keine Nachtfreigabe.

Vorher, gleiche Kamera und Wasserprofil:

![Vorheriger Anschluss zwischen Fels und Seegrund](Images/FootDeposits_Before.png)

Mit örtlicher Auflage:

![Böschungen an denselben Felsgruppen](Images/FootDeposits_After.png)

Die grossen Kerne bleiben stehen; lose Steine und Bewuchs folgen der angehobenen Oberfläche. Die sichtbare Verbesserung betrifft den örtlichen Bodenanschluss. Freie Sedimentflächen, die feine Pflanzendarstellung und die gemeinsame Lichtwirkung bleiben deutlich hinter der Nutzerreferenz zurück.

Bei 1080p/Hoch mit Wasserprofil erreicht Weesen 69,06 FPS gegenüber 70,00 FPS ohne Auflage. Walenstadt enthält drei lange Frames bis 80,11 ms und die Nachladestrecke zwei bis 48,83 ms; die jeweiligen Kontrollen bleiben bei maximal 22,86 bzw. 30,06 ms. Die [vollständigen Messbedingungen und Werte](PERFORMANCE.md#windowsfootdeposits-örtliche-böschungen-am-felsfuss) bleiben ohne ausgeblendete Ausreisser erhalten. Nächster Optimierungspunkt ist die Eingrenzung dieser zusätzlichen CPU-/Nachladespitzen. Die neue Vorschau erfüllt weder eine durchgängige 60-FPS-Zusage noch das gesamte visuelle Ziel.

## Gestaltung und gemeinsame Oberfläche

Die vier Kernkandidaten einer Aufschlussgruppe bestimmen Lage und Ausrichtung der Ablagerung. Unregelmässige, längliche Ausläufe vermeiden einen kreisförmigen Ring. Beiträge überlagern sich über ihr Maximum; sie summieren sich nicht zu unbegrenzt hohen Hügeln. Die zusätzliche Höhe ist auf 70 cm begrenzt. Flache Uferbereiche, tiefe Zonen, steile Hänge, Einstiege, Zugangswege und Messdatenränder erhalten keine oder eine weich ausgeblendete Auflage.

Die Felskerne behalten ihre bisherigen Transformationen. Lose Steine und Bruchstücke werden auf die neue Höhe gesetzt und an die geänderte Hangneigung angepasst. Pflanzen, kleine Kiesel, bodennahe Fische, Bewegung und Sediment verwenden dieselbe Oberfläche. Die sichtbare Geländemesh-Kollision und die Höhenabfrage interpolieren dieselben Dreiecksebenen auf dem bestehenden Ein-Meter-Raster. Es werden keine zusätzlichen Geländedreiecke erzeugt.

Das ist gestaltete Spielgeometrie, keine neue Vermessung und keine Simulation realen Sedimenttransports. `SampleMeasured` und die Tiefenkarte behalten die unveränderten Quelldaten. Das bisher gerenderte Gelände wird durch die Auflage örtlich um bis zu 70 cm angehoben; die Tiefenanzeige des Tauchers bezieht sich weiterhin auf dessen tatsächliche Lage unter der Wasseroberfläche.

Die Auflage besitzt einen eigenen, begrenzten Quell- und Höhenspeicher mit jeweils höchstens 25 Zellen. Höhenzellen enthalten maximal 96 × 96 Werte. Entfernte Zellen werden einzeln entfernt, statt sämtliche nahen Höhen neu aufzubauen. Die Quellabfrage erzeugt nur die vier Kernkandidaten; spätere lose Steine beeinflussen deren ursprüngliche Annahme nicht. Ein Laufzeittest vergleicht diese Quellen mit den tatsächlich vorhandenen Felskernen.

## Bisherige Entwicklung und Prüfgrenzen

Die erste Kontaktprüfung meldete fünf Abweichungen unter 804 Proben. Alle fünf lagen an Messdatenrändern ohne vollständiges Unterwasserdreieck. Dort blieb die Oberfläche unverändert; gemischte Übergänge gehören zum separaten Ufermodell. Der korrigierte Test erfasst beide Dreieckshälften und prüft zusätzlich unveränderte Höhen an diesen ausgeschlossenen Rändern. Er vergleicht ausserdem Höhen nach dem vollständigen Leeren und Wiederaufbauen der Zwischenspeicher.

Die erste Uferprüfung verglich auch nicht verwendete Vertices des Ufermodells mit der neuen Spieloberfläche. Vollständig vermessene Dreiecke werden dort absichtlich nicht gezeichnet, weil sie im nachgeladenen Seegrund liegen. Die Prüfung unterscheidet nun die unveränderten Originalwerte aller gespeicherten Vertices von der zusätzlichen Kontinuitätsprüfung für tatsächlich verwendete Übergangsvertices. Der anschliessende Uferlauf bei Weesen besteht. Die Rohprotokolle der früheren Fehlläufe bleiben unter `artifacts/foot-deposits-20261127-075355`, `artifacts/foot-deposit-contact-diagnostic.log` und `artifacts/foot-deposits-20261127-080442` erhalten.

Der abschliessende native Lauf unter `artifacts/foot-deposits-20261127-081111` besteht sechs Fälle mit 23 Aufnahmen, einschliesslich Ausrichtung loser Steine und räumlichem Höhenspeicher. Bei Weesen stimmen 1598 von 1598 Kontaktproben mit dem sichtbaren Boden überein, bei Murg 2278 von 2278 und bei Walenstadt 1196 von 1196. Die maximale Auflage beträgt dort 70,00 / 53,05 / 69,99 cm. Die Wiederaufbauprüfung liefert dieselben Höhen. Ufermodellprüfungen erfassen alle drei Übergänge; der tatsächliche Laufweg ist nur für Weesen geprüft. Der [native Bericht](FOOT_DEPOSITS_NATIVE_RESULTS.json) hält diesen damaligen, noch optionalen Quell- und DLL-Stand fest.

Vier native Messungen auf RTX 3090 / Ryzen 9 9950X3D, 1080p/Hoch mit Wasserprofil und 100 Prozent Renderauflösung, ohne VSync/Limit, jeweils 15 Sekunden Aufwärmen:

| Messung | Mit Auflage | Ohne Auflage |
| --- | ---: | ---: |
| Weesen, 60 s, mittlere FPS | 68,86 | 69,11 |
| Weesen, mittlerer Spielthread | 6,68 ms | 6,07 ms |
| Weesen, längster Frame | 39,99 ms | 40,18 ms |
| Murg-Nachladen, 90 s, mittlere FPS | 95,90 | 96,04 |
| Murg-Nachladen, längster Frame | 45,51 ms | 25,96 ms |

Der Nachladelauf mit Auflage enthält einen Frame über 33,33 ms, die Kontrolle keinen. Der längste Frame lässt sich aus den erfassten Timings nicht eindeutig der Höhenberechnung zuordnen; er bleibt vollständig enthalten. Die Mittelwerte belegen keine durchgehend stabile Mindestbildrate. Ein nativer Vergleich ist keine Paket-, Langzeit-, Mindesthardware- oder vollständige visuelle Freigabe.

## Werkzeuge

`Tools/TestFootDeposits.ps1 -Preview` vergleicht die bisherige und die neue Oberfläche bei Weesen. Ohne `-Preview` kommen Murg, Walenstadt, der Uferweg bei Weesen, Nachladen und Sediment hinzu. `-Packaged -PackageDirectory Builds/WindowsFootDeposits` prüft das eigenständige Paket. Die Auflage ist im neuen Quellstand standardmässig aktiv; nur Diagnoseläufe können sie über `-DiveNoFootDeposits` abschalten. Die PowerShell-Werkzeuge bieten dafür `-NoFootDeposits`. `Tools/ReportFootDeposits.py` bindet Aufnahmen, Kontaktprüfungen, Messungen und Launcher an Quellen, DLL beziehungsweise Paketbytes. Die unabhängigen C++-Formprüfungen liegen in `Tests/FootDepositTests.cpp`.
