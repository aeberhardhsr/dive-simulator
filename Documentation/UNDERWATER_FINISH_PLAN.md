# Unterwasserwelt: Abnahme und genau fünf Abschlussiterationen

Stand: 28. November 2026. Status: **5 von 5 begrenzten Arbeitsrunden beendet; vollständige technische Abnahme nicht erreicht.** Runden 1, 3 und 5 enden mit ausdrücklich dokumentierten Abweichungen. Runde 2 besteht; Runde 4 besteht die festgelegten Zahlenkriterien und die ergänzende örtliche Sichtprüfung. [Abschlussbericht, verbleibende Pflichtkriterien und Übergabe an Überwasser](UNDERWATER_FINISH_HANDOVER.md).

Der Nutzer hat die Unterwasserwelt gestalterisch abgenommen. Felsen, Boden, Pflanzen, Verteilung und grundlegende Atmosphäre werden deshalb nicht erneut zur offenen Gestaltungsaufgabe. Die Referenzbilder bleiben Orientierung; eine identische Umsetzung oder eine erneute allgemeine Fotorealismus-Abnahme ist nicht erforderlich. Nach genau fünf zusätzlichen, begrenzten Iterationen wechselt der Schwerpunkt zur Überwasserwelt.

## Zuerst erledigt: die bereits laufende Optimierung

Die [Schattenkarten-Optimierung](REFLECTION_SHADOW_STUDY.md) wurde **vor Erstellung dieses Plans** abgeschlossen und zählt nicht zu den fünf Iterationen. Commit `6c9b969217c05c87296a04636a3f27ea72b673e5`: erfolgreicher Engine-Build, neun native Fälle mit 27 neuen Bildern, Vergleich mit sechs Referenzbildern, erzwungener Strahl-Rückfall und zwei Leistungsläufe. Ergebnis in der geprüften Szene: 62,29 statt 55,19 FPS. Die Projektprüfungen auf Windows, macOS und Linux sind sowohl im [Push-Durchlauf](https://github.com/aeberhardhsr/dive-simulator/actions/runs/46081101110) als auch im [PR-Durchlauf](https://github.com/aeberhardhsr/dive-simulator/actions/runs/46081106474) erfolgreich.

Diese Optimierung ist im separaten Source-Editor geprüft. Das spielbare Paket bleibt vorerst `Builds/WindowsWaterline/Start-WaterPreview.cmd`; es enthält den neuen Reflexionsweg noch nicht. Paketintegration ist ausdrücklich Bestandteil der folgenden fünf Iterationen.

## Verbindliche Reihenfolge und Abschlusskriterien

### 1. Wasserlinie und Auftauchen fertigstellen

**Umfang:** Den bekannten schmalen hellen Saum an der Wasserlinie untersuchen und gezielt korrigieren. Dabei die bereits behobene schwarze Fläche und die korrigierte blaue Unterwasserreflexion erhalten. Keine neue Wasserfarbe, Wellenästhetik oder Unterwassergeometrie.

**Prüfung:** Die vorhandene feste Kamera bei −20 cm, −2 cm und +20 cm relativ zur örtlichen Welle; zusätzlich ein langsamer Durchgang durch die Oberfläche an jedem der drei Uferzugänge Weesen/Strandbad, Murg/West und Mols/Lediwracks. Startmodus und tatsächlicher Ort werden protokolliert; Wasserstart Index 2 ist Walenstadt/Ost und darf nicht als Mols bezeichnet werden.

**Fertig, wenn:** Der durch Rendering verursachte Saum im reproduzierten Fehlerblick behoben ist, keine schwarze oder blaue Ersatzfläche zurückkehrt und alle drei bewegten Übergänge ohne neu eingeführtes Aufblitzen oder sichtbare Lücke geprüft sind. Vorher-/Nachher-Bilder und kurze Bewegungsaufnahmen belegen die Änderung. Falls die Untersuchung einen natürlichen Glanzreflex statt eines Fehlers nachweist, wird dieser Befund mit einer isolierenden Kontrolle dokumentiert; er wird nicht durch einen pauschalen dunklen Filter verdeckt.

**Ergebnis:** Ein abgeschlossener Übergangsbericht mit Codeänderung oder nachgewiesenem Nichtfehler und eindeutigen Prüfergebnissen.

**Abschlussentscheidung mit Abweichung:** Die begrenzte Prüfrunde ist durchgeführt; die kamerarelative Normalkorrektur vermindert den Saum. Der verbleibende Saum sowie kurze blaue Streifen beim exakten Grenzflächendurchgang bleiben Fehler beziehungsweise Einschränkungen, keine nachgewiesenen natürlichen Reflexe. Beide sind auch mit dem bisherigen Material reproduziert. Gemäss der Nutzervorgabe, offene Punkte für später festzuhalten und die fünf Runden zu begrenzen, werden sie ausdrücklich als UW-LATER-07 vertagt. Das oben formulierte vollständige Fehlerfrei-Kriterium bleibt **nicht erfüllt**. Diese Abweichung ist keine vollständige Implementierung und keine erneute gestalterische Abnahme; sie verhindert aber eine endlose Verlängerung dieser Runde. Belege und Grenzen stehen im [Übergangsbericht](WATERLINE_SEAM_STUDY.md).

### 2. Licht, Sichtweite und Schatten-Rückfall absichern

**Umfang:** Den neuen Reflexionsweg in den noch nicht belegten Randfällen prüfen und nur dabei nachgewiesene Fehler korrigieren. Sonne/Mond müssen zur richtigen Schattenquelle gehören; Lampen müssen unabhängig davon funktionieren. Die abgenommene Grundstimmung bleibt erhalten.

**Prüfung:** Die vorhandene achtteilige `response`-Serie mit dem VSM-Profil ausführen (klare/trübe Sicht, Nahkontrollen, Nacht und Lampe). Ergänzend genau vier gezielte Kontrollen: tatsächliche Sonnenabschattung, Mondabschattung, fehlende Kartendaten und mehrdeutige Lichtzuordnung. Diagnosebilder und normales Spielbild werden getrennt ausgewertet. Die reine grüne Nutzungsanzeige aus dem abgeschlossenen Versuch ersetzt keinen Nachweis tatsächlicher Abschattung.

**Fertig, wenn:** Alle zwölf Fälle bestehen; Abschattungen im normalen Bild plausibel reagieren; fehlende/mehrdeutige Daten den ursprünglichen Strahlweg verwenden; Lampen- und Nachtkontrollen erhalten bleiben; keine ungültigen Farbwerte, schwarzen Ersatzbilder oder Shaderfehler auftreten. Die vollständige Strahlenvariante bleibt als Vergleich reproduzierbar.

**Ergebnis:** Eine eindeutige Entscheidung über den freigegebenen Reflexionspfad für das Paket, mit Bildern und reproduzierbaren Einstellungen. Ein fehlerhafter Optimierungszweig wird korrigiert oder aus dem Paketkandidaten entfernt; er wird nicht stillschweigend als fertig freigegeben.

### 3. Den geprüften Stand in ein normal startbares Spielpaket übernehmen

**Umfang:** Den freigegebenen Wasser-/Reflexionsstand aus den bisherigen Einzelversuchen zu einer eindeutigen Laufzeitkonfiguration zusammenführen. Source-Game-Build, Shader-Cook und Paketierung abschliessen. Benötigte Shader und Bindungen müssen im Paket enthalten sein. Diagnoseprofile dürfen keinen normalen Start beeinflussen.

**Prüfung:** Neues, separat benanntes Windows-Paket mit eigenem Launcher. Start ohne Editor und ohne Smoke-/Study-Schalter, danach aus dem Menü alle drei Uferzugänge betreten, tauchen und zurückkehren. Die drei direkten Wasserstarts separat prüfen. Wasserlinien- und Lichtkontrollen aus Iteration 1/2 gezielt am Paket wiederholen. Den bestehenden Grafik-Ersatzpfad mit deaktiviertem Hardware-Raytracing starten; Einschränkungen dokumentieren, keine Gleichwertigkeit der Darstellung behaupten. Menü, HUD, Handgelenkcomputer und Lampe dürfen durch die Integration nicht ausfallen.

**Fertig, wenn:** Ein vollständiger Paketordner per Doppelklick spielbar ist; aktiver Renderpfad und Konfiguration im Protokoll eindeutig nachgewiesen sind; keine fehlenden Shader oder Ersatzmaterialien auftreten; die genannten Paketfälle bestehen. Der bisherige abgenommene Paketordner bleibt als Rückfallstand erhalten. Experimentelle, nicht ausgewählte Lichtmodelle bleiben ausserhalb der normalen Konfiguration.

**Ergebnis:** Ein tatsächlich spielbarer Paketkandidat mit Launcher, Prüfsummen, Build-/Cook-Protokollen und kurzer Startanleitung. Ein erfolgreicher Editorlauf allein schliesst diese Iteration nicht ab.

### 4. Bildrate und Nachladen im Paket abschliessen

**Umfang:** Den Paketkandidaten messen und ausschliesslich nachgewiesene Engpässe der Unterwasserdarstellung beziehungsweise ihres Nachladens beseitigen. Keine zusätzliche Detailstufe, Pflanzendichte oder Asset-Serie. Eine Optimierung darf die abgenommene Gestaltung nicht unbemerkt reduzieren.

**Prüfung:** Drei feste Strecken auf dem vorhandenen RTX-3090-/9950X3D-Rechner bei 1080p, Hoch/Qualität 2 und 100 % Renderauflösung: Weesen mit Blick zur Wasserunterseite (60 s), Walenstadt mit sichtbarem Boden/Bewuchs (60 s), Murg beim Zellwechsel (90 s), jeweils nach 15 s Aufwärmen. Vorher/Nachher bei einer Änderung mit identischen Einstellungen; danach ein bestätigender Lauf je Strecke. Keine parallelen Engine-Builds oder Spielinstanzen. Abschlussbilder prüfen, damit eine Messung im leeren Wasser keine dichte Szene ersetzt.

**Fertig, wenn:** Alle drei abschliessenden Strecken mindestens 60 FPS im Mittel erreichen, das 95. Perzentil der Bildzeit höchstens 20 ms beträgt und keine einzelne Bildzeit über 100 ms liegt. Sichtbare Löcher oder Unterbrechungen beim Nachladen sind ausgeschlossen. 1%-Low, maximale Bildzeit, Anzahl der Bilder über 16,67/33,33 ms sowie CPU-/GPU-Anteile werden vollständig ausgewiesen.

Diese Grenzen sind konkrete Prüfkriterien dieser Abschlussiteration. Sie behaupten weder durchgehend 60 FPS noch die vollständige Erfüllung von PERF-001 auf jeder Hardware, in 4K oder in sämtlichen Spielsituationen. Die weitergehenden Anforderungen bleiben nachvollziehbar offen. Ein verfehltes Kriterium wird nicht durch eine unbemerkte Auflösungs- oder Qualitätsänderung grün gerechnet.

**Ergebnis:** Ein gemessener, eingefrorener Paketkandidat und ein kurzer Leistungsbericht. Weitere allgemeine Engine-Optimierung wird nicht an diese Iteration angehängt.

**Abschluss:** Beide Serien mit dem unveränderten R3-Paket erfüllen die Zahlenkriterien. Die ergänzende bewegte Murg-Prüfung zeigt an zwei Zellübertritten mit sichtbarem Boden keine beobachtete Nachladelücke oder Aufblitzer. Alle 215 nummerierten Bilder wurden gesichtet, die Übergänge zusätzlich in voller Auflösung. [Sichtnachweis und Grenzen](UNDERWATER_STREAMING_VISUAL_R3.json). Vollständige reale Schwimm-/Rückkehrabläufe bleiben als Abweichung der Runden 3/5 offen; sie werden nicht aus der nativen Teststeuerung abgeleitet.

Der [Messnachweis](UNDERWATER_FINISH_PERFORMANCE_R3.json) bindet alle sechs CSV-Aufzeichnungen, Protokolle, Metadaten und Abschlussbilder über die Originalberichte sowie identische EXE-/Container-Prüfsummen. Alle sechs Abschlussbilder wurden gesichtet. Es gab keine Render-, Qualitäts- oder Auflösungsänderung zwischen den Serien.

| Strecke | Serie | Mittel FPS | 1%-Low FPS | P95 ms | Maximum ms | Bilder >16,67 / >33,33 ms |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| Weesen, Wasserunterseite | Baseline | 64,39 | 54,12 | 17,51 | 19,39 | 855 / 0 |
| Weesen, Wasserunterseite | Bestätigung | 65,25 | 55,66 | 17,16 | 18,59 | 533 / 0 |
| Walenstadt, Boden | Baseline | 71,72 | 53,71 | 15,48 | 38,18 | 22 / 1 |
| Walenstadt, Boden | Bestätigung | 73,96 | 49,82 | 15,21 | 31,89 | 43 / 0 |
| Murg, Nachladen | Baseline | 94,74 | 71,59 | 11,32 | 26,30 | 13 / 0 |
| Murg, Nachladen | Bestätigung | 94,92 | 70,67 | 11,30 | 27,65 | 14 / 0 |

CPU-/Render-/RHI-/GPU-Zeiten mit Perzentilen stehen vollständig im Messnachweis. Kein inneres Messbild überschreitet 100 ms. Die Messungen beweisen keine durchgehenden 60 FPS; insbesondere ist der 1%-Low in Walenstadt sichtbar niedriger. Es wird keine Beschleunigung zwischen Baseline und Bestätigung behauptet.

**Grenzen der Sichtprüfung:** Weesen zeigt die Wasserunterseite, Walenstadt sichtbaren Grund und Felsen. In Walenstadt begrenzt die physische Kollisionsprüfung die Drehung unterschiedlich: Endkurs 206° beziehungsweise 51°, im Bestätigungslauf 1’582 verworfene Posenschritte. Gleiche Eingaben sind hier keine identische Kamerafahrt. Beide Murg-Abschlussbilder zeigen überwiegend offenes Wasser bei 24,6 m Tiefe. In beiden Murg-Läufen sind vier Bilder mit sieben beziehungsweise acht statt neun Geländezellen protokolliert; alle übrigen haben neun. Der Code baut fehlende Randzellen von 64 m Breite schrittweise auf. Das erklärt die Zählung, belegt aber weder ein sichtbares Loch noch dessen Abwesenheit. Diese Lücke im damaligen Nachweis wurde durch die im Abschlussbericht beschriebene lokale Murg-Bildfolge ergänzt. Sie belegt zwei sichtbare Zellübertritte; eine seeweite Fehlerfreiheit folgt daraus nicht.

`Tools/TestUnderwaterFinishPerformance.ps1` führt die drei festgelegten Fälle mit 15 s Aufwärmen, 1080p, Qualität 2 und 100 % Renderauflösung aus. Die ersten beiden Fälle sind stationäre Rundblicke: Weesen mit +35° angeforderter Blickneigung zur Wasserunterseite, Walenstadt mit −12° zum Grund. Der dritte Fall durchschwimmt den bestehenden Murg-Nachladepfad. Tatsächliche Kamerapose und Abschlussbild bleiben Bestandteil der Prüfung. Der Aufruf verwendet `-Pass baseline` beziehungsweise `-Pass confirmation`; eine vorhandene Spiel-, Editor- oder UnrealBuildTool-Instanz verhindert den Start. `Benchmark.ps1 -ProductionWater` verlangt den normalen Renderpfad samt aktiver Reflexion, Seegrenzen und unveränderten Qualitätswerten, ohne einen Grafik-Testschalter an das Spiel zu übergeben. Das Werkzeug erfasst alle drei Ergebnisse auch bei verfehlten Leistungsgrenzen und schreibt höchstens `NUMERICAL_PASS_REQUIRES_IMAGE_REVIEW`, keine automatische visuelle Abnahme.

```powershell
./Tools/TestUnderwaterFinishPerformance.ps1 -PackageDirectory Builds/WindowsUnderwaterFinalCandidateR3 -Pass baseline
# Erst nach Auswertung und gegebenenfalls begrenzter Korrektur:
./Tools/TestUnderwaterFinishPerformance.ps1 -PackageDirectory Builds/WindowsUnderwaterFinalCandidateR3 -Pass confirmation
```

### 5. Stabilität, Abschlusslieferung und Übergabe an Überwasser

**Umfang:** Den unveränderten Paketkandidaten aus Iteration 4 abschliessend prüfen und als Unterwasser-Abschlussstand kennzeichnen. Das ist eine technische Abschlussprüfung; die gestalterische Abnahme des Nutzers bleibt gültig.

**Prüfung:** Mindestens zwei Stunden gerenderter Betrieb desselben Pakets mit einer dokumentierten Route, die Schwimmen, Nachladen und wiederholtes Auf-/Abtauchen enthält (PERF-010). Speicherverlauf und Fehlerprotokoll erfassen. Zusätzlich kurzer Spielablauf je Uferzugang: Menü → Einstieg → Tauchgang → Lampe/Computer/Pause → Rückkehr → Logbuch → Neustart. Vorhandene Automationsprüfungen und CI müssen für den finalen Quellstand bestehen. Die in Iteration 1–4 erzeugten Nachweise werden gebunden statt grundlos vollständig wiederholt; nach relevanten Änderungen nur betroffene Kontrollen erneut ausführen.

**Fertig, wenn:** Kein Absturz, Geräteverlust oder fehlgeschlagener Spielablauf; kein fortlaufendes Speicherwachstum bei wiederholtem gleichartigem Streckenabschnitt; finaler Paketstand, Launcher, Build-Commit, Prüfsummen und Ergebnisse stimmen überein. README und Restpunktliste benennen klar, was ausgeliefert und was vertagt ist. Alle fünf Iterationen tragen einen belegten Abschlussstatus.

**Ergebnis:** Gesicherter Unterwasser-Abschlussstand. **Die nächste Entwicklungsiteration arbeitet an der Überwasserlandschaft**: Gelände-/Felstopologie, Ufer, Wiesen und Wald, danach Materialien und atmosphärische Gesamtwirkung auf dem gewünschten Referenzniveau. Es wird keine sechste Unterwasser-Optimierungsrunde eröffnet.

**Dauerlauf beendet, Speicherkriterium offen:** Das unveränderte R3-Paket hat am 28. November 2026 den gerenderten Lauf mit 7’200,26 s Echtzeit und 7’200,14 s gemessener Simulationszeit beendet. 7,42 km, 31 Wendepunkte, fünf besuchte Geländezellen, wiederholtes Auf-/Abtauchen und 240 erfolgreiche Zwischensicherungen sind belegt; keine fehlgeschlagene Sicherung, kein Absturz oder Geräteverlust im Protokoll. Alle 53 Laufzeitdateien und 174 Projekteingaben stimmen weiterhin mit dem R3-Nachweis überein. [Vollständiger Mess- und Prüfbericht](UNDERWATER_FINISH_SOAK_R3.json).

Der lange, eigenständige Erkundungspfad erreicht 55,76 FPS im Mittel, P95 höchstens 21,75 ms, Maximum 278,20 ms; 202 von 240 Berichtsfenstern liegen unter 60 FPS. Sämtliche Verzögerungen, auch im zeitlichen Umfeld der periodischen Aufnahmen, bleiben enthalten. Das erfüllt die Zahlenkriterien der sechs anderen kurzen Leistungsstrecken nicht und erweitert deren bestandene Aussage nicht auf das ganze Spiel.

Alle 14 Aufnahmen sind gesichtet. Besonders die Aufnahme nach 50 Minuten zeigt eine schmale dunkle Linie am entfernten Bodenübergang. Die ergänzende Untersuchung reproduziert die Linie auch stationär bei geladenem Gelände und ohne dynamische Schatten. Ihre Ursache bleibt ungeklärt und wird unter UW-LATER-10 vertagt; die bewegte Nachladeprüfung ist separat abgeschlossen.

240 Host-Speicherproben sind vorhanden; 238 lassen sich über UTC-Intervalle den gespeicherten Streckenpositionen zuordnen. Nach den ersten zehn Minuten beträgt der Trend des privaten Prozessspeichers +55,21 MiB/h, in der letzten Stunde +58,61 MiB/h. Alle 26 vergleichbaren Gruppen aus Geländezelle, Blickrichtung, 2-m-Tiefenband und Oberflächenphase haben einen positiven Trend (Median +62,21 MiB/h). Der sinkende RAM-Arbeitssatz hebt diesen Befund nicht auf. **Das Kriterium ohne fortlaufendes Speicherwachstum ist nicht nachgewiesen und bleibt als Abweichung offen.** Eine Speicherleck-Ursache ist damit nicht bewiesen. Die Messung wird weder neu gerechnet noch durch eine Qualitätsänderung ersetzt; die anschliessende begrenzte Ursachendiagnose ist unten dokumentiert.

**Abschliessender Status:** Die begrenzte Runde 5 ist mit Abweichung beendet. Speicher- und vollständige reale Bedienkriterien bleiben unerfüllt; die neue örtliche Sichtprüfung ist abgeschlossen. Der [Übergabebericht](UNDERWATER_FINISH_HANDOVER.md) führt alle Ergebnisse zusammen.

**Begrenzte Ursachendiagnose abgeschlossen:** Der unveränderte Speicherpfad wurde separat mit Null-RHI geprüft: 240 wachsende, 60 gleich grosse und 60 kleine Sicherungen, alle erfolgreich. Bei wachsender Aufzeichnung steigt der zugesicherte Prozessspeicher; während der 60 gleich grossen Sicherungen beträgt die Endwertänderung +0,25 MiB bei 4,75 MiB Spannweite. Der eigene Messpunktpuffer belegt maximal 4,43 MiB. Das grenzt einen grössenabhängigen Beitrag der Aufzeichnung ein, ordnet aber nicht den gesamten Speicheranstieg im gerenderten Spiel zu. Eine unbeschränkte Schreibwarteschlange liegt nicht vor. [Diagnose, Grenzen und reproduzierbarer Aufruf](RECORDING_MEMORY_DIAGNOSIS.md). Keine Änderung am produktiven Speicherpfad oder am R3-Paket; das strikte Speicherkriterium bleibt offen. Eine weitergehende Umstellung der Serialisierung wird als UW-LATER-09 festgehalten; die begrenzte Diagnose wird nicht zu einer weiteren Optimierungsserie verlängert.

**Messverfahren:** `Tools/Soak.ps1 -ProductionWater` prüft den normalen VSM-/Wasserpfad bereits nach Beginn der Messung. Eine nachgewiesene falsche Konfiguration beendet nur die gerade vom Werkzeug gestartete Testinstanz; ein blosses Beobachtungszeitlimit beendet weiterhin keinen Prozess. Zusätzlich zu den nativen 30-s-Intervallen mit Streckenfortschritt und Arbeitssatz entsteht `HostMemory.jsonl` mit privatem Prozessspeicher, Arbeitssatz und virtuellem Adressraum. Die drei Grössen sind getrennt auszuwerten; virtueller Adressraum ist kein RAM-Verbrauch. Die Speicherabnahme bleibt eine eigene Auswertung wiederholter vergleichbarer Streckenabschnitte. Ein erfolgreich abgeschlossener Prozess oder der Marker `DIVE_SOAK_PASS` allein ist dafür kein Beweis.

`Tools/ReportSoak.py` bindet diese Beobachtungen bei einem Produktionslauf zwingend an den Bericht und verlangt eine hinreichend vollständige Messreihe nach bestätigter Renderkonfiguration. Die zehn gezielten Prüfungen von Soak-Integrität und Speicheraufzeichnung bestehen: Fehlende, verkürzte, ungeprüfte, ungültige oder zeitlich rückwärts laufende Messreihen werden zurückgewiesen; Warmup und privates Speicherwachstum bleiben sichtbar, auch wenn der Arbeitssatz konstant ist. Der Bericht kennzeichnet die Speicher- und Bildauswertung weiterhin als erforderlich. Historische Paketprüfsummen werden nicht durch neue Pakete ersetzt; bereits entfernte alte Testpakete liefern keinen erneut verifizierten Lauf.

```powershell
# Unabhängige Messung vorgezogen; exakt dasselbe unveränderte R3-Paket:
./Tools/Soak.ps1 -PackageDirectory Builds/WindowsUnderwaterFinalCandidateR3 -Seconds 7200 -Entry 0 -Quality 2 -ProductionWater
```

## Umfang begrenzen, Abschlüsse ehrlich halten

„100 % abgeschlossen“ bedeutet: alle vereinbarten Ergebnisse und Prüfungen der jeweiligen Iteration sind erledigt und belegt. Es bedeutet nicht, dass jede mögliche physikalische Verbesserung oder jede ursprünglich formulierte Gesamtspielanforderung erfüllt ist. Der Planstatus bleibt bis zur tatsächlichen Ausführung offen; insbesondere ist die heutige Source-Editor-Messung keine Paketabnahme.

Pro Iteration wird eine Ergebnisliste mit erledigten Punkten, konkreten Prüfnachweisen und verbleibenden Abweichungen geführt. Neue Wünsche und nebensächliche Befunde wandern in die Restpunktliste. Fehler, die durch diese fünf Iterationen eingeführt werden, werden innerhalb der verursachenden Iteration behoben oder die verursachende Änderung wird zurückgenommen. Ein nicht erfülltes Pflichtkriterium bleibt ausdrücklich offen; eine Vertagung wird nicht als erfolgreiche Implementierung bezeichnet. Die fünf Iterationen werden nicht durch immer neue Unterphasen verlängert.

## Für später dokumentierte Punkte

Diese Punkte blockieren die gestalterische Unterwasser-Abnahme und den anschliessenden Wechsel zur Überwasserwelt nicht. Sie werden nur bei einem späteren ausdrücklichen Arbeitsauftrag wieder geöffnet.

| ID | Restpunkt | Aktueller Umgang |
| --- | --- | --- |
| UW-LATER-01 | Indirektes Himmelslicht und Mehrfachstreuung im Wasservolumen | Als vereinfachtes Lichtmodell dokumentieren; kein neuer Forschungszweig in den fünf Iterationen |
| UW-LATER-02 | Vollständige sekundäre Brechungs-/Grenzflächenwechsel und spektrale Abstimmung sämtlicher Lichtwege | Bestehende geprüfte Darstellung verwenden; getrennte Experimente archiviert halten |
| UW-LATER-03 | Exakte Übereinstimmung von Schattenkarten und Strahlschatten in jeder Entfernung und jeder Bewegung | Begrenzte Randfallprüfung in Iteration 2; verbleibende Bias-/Auflösungsunterschiede ausdrücklich dokumentieren |
| UW-LATER-04 | Neue Sedimentformen, zusätzliche Pflanzen-/Felsassets, weitere Materialdetails und feinere Fischanimation | Keine erneute Gestaltung der abgenommenen Welt; frühere verworfene Versuche nicht automatisch wieder aufnehmen |
| UW-LATER-05 | Weitere Hardware, durchgehende 60 FPS in allen Szenen, 1440p/4K und macOS-Grafikgleichwertigkeit | Anforderungen erhalten; heutige lokale Windows-Messungen nicht als globale Erfüllung ausgeben |
| UW-LATER-06 | Vollständige reale Maus-/FPV-Ergonomieabnahme und zusätzliche Menü-/HUD-Wünsche | Bestehende UI erhalten und in Iteration 3/5 auf Regression prüfen; umfassende weitere Gestaltung bleibt eigene Arbeit. Beim realen R3-Test zeigt die Abschlussansicht noch den Rohwert `aborted`, das Logbuch dagegen korrekt „Abgebrochen“; spätere gezielte Textkorrektur |
| UW-LATER-07 | Verbliebener heller Saum und kurze breite blaue Streifen direkt beim Grenzflächendurchgang | In Iteration 1 mit altem und korrigiertem Material an festen Simulationszeiten nachgewiesen; Saum vermindert, keine vollständige Fehlerbehebung. Ausdrückliche Abweichung vom ursprünglichen Abschlusskriterium; spätere gezielte Grenzflächenarbeit, keine neue Unterwasser-Gestaltungsrunde |
| UW-LATER-08 | Software-Ersatzpfad ohne Hardware-Raytracing | Pakete R2 und R3 starten, zeigen jedoch blaue Unterwasserbänder und deutlich schwächere Reflexionen der Wasserunterseite. Keine Gleichwertigkeit zur abgenommenen Hardware-Darstellung; gesonderte spätere Kompatibilitätsarbeit |
| UW-LATER-09 | Speicherbedarf langer Aufzeichnungen und JSON-Sicherung | Begrenzte [Diagnose](RECORDING_MEMORY_DIAGNOSIS.md) abgeschlossen: grössenabhängiger Bedarf, 360 erfolgreiche Sicherungen, keine unbeschränkte Schreibwarteschlange. Vollständige Zuordnung des R3-Speichertrends und mögliche Serialisierungsoptimierung bleiben offen; kein Nachweis des strikten Speicherkriteriums aus Runde 5 |
| UW-LATER-10 | Schmale dunkle Farblinie am entfernten Bodenübergang | An angenäherter Dauerlaufpose stationär und ohne dynamische Schatten reproduziert. Bestätigte Editor-Tiefendarstellung ohne offensichtlichen grossen Spalt; Ursache unbewiesen. [Diagnose](UNDERWATER_STREAMING_VISUAL_R3.json), keine produktive Korrektur oder vollständige Fehlerfrei-Abnahme |

Quellen: [visuelle Vorgabe](VISUAL_TARGET.md), [Anforderungsbasis](REQUIREMENTS_CONTEXT.md), [Originalanforderungen](REQUIREMENTS.csv), [Testorte](TEST_ENTRY_LOCATIONS.md), [Wasserlinienkorrektur](WATERLINE_BLEND.md), [Source-Engine](SOURCE_ENGINE.md), [abgeschlossene Schattenoptimierung](REFLECTION_SHADOW_STUDY.md).

## Fortschritt

| Iteration | Status | Abschlussnachweis |
| --- | --- | --- |
| 1 – Wasserlinie | Prüfrunde beendet mit Abweichung | [Übergangsbericht](WATERLINE_SEAM_STUDY.md); ursprüngliches Fehlerfrei-Kriterium nicht erfüllt, Restpunkte ausdrücklich unter UW-LATER-07 vertagt |
| 2 – Licht-Randfälle | Bestanden | [Licht- und Schattenprüfung](REFLECTION_FINISH_VALIDATION.md); acht Grundkontrollen und vier physische Randfallkontrollen bestanden, vereinigter VSM-Pfad für Paketintegration ausgewählt |
| 3 – Spielpaket | Beendet mit Abweichung | [Paketintegration](UNDERWATER_PACKAGE_INTEGRATION.md): 18 native Fälle, 84 Bilder, zwölf Editorvergleiche, normaler Start und reale Teilprüfung bestanden. Vollständige reale Schwimm-/Rückkehrfolgen bleiben offen; UW-LATER-06/08 |
| 4 – Leistung | Zahlenkriterien und örtliche Sichtprüfung bestanden | [Sechs Messläufe](UNDERWATER_FINISH_PERFORMANCE_R3.json) und [bewegte Murg-Bildfolge](UNDERWATER_STREAMING_VISUAL_R3.json). Aussage auf geprüfte Strecken begrenzt; statischer Farblinienbefund UW-LATER-10 bleibt offen |
| 5 – Stabilität und Übergabe | Beendet mit Abweichung | [Dauerlauf](UNDERWATER_FINISH_SOAK_R3.json) ohne Absturz, 240 erfolgreiche Sicherungen; Speicher- und vollständige reale Bedienkriterien bleiben offen. [Abschluss und Schwerpunktwechsel](UNDERWATER_FINISH_HANDOVER.md) |
