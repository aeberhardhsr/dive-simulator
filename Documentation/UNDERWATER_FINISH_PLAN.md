# Unterwasserwelt: Abnahme und genau fünf Abschlussiterationen

Stand: 28. November 2026. Status: **2 von 5 Arbeitsrunden beendet: Runde 1 mit dokumentierter Abweichung, Runde 2 bestanden; als Nächstes folgt Iteration 3.** Das ursprüngliche Fehlerfrei-Kriterium der ersten Runde ist nicht vollständig erfüllt und wird nicht als bestanden gezählt.

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

### 5. Stabilität, Abschlusslieferung und Übergabe an Überwasser

**Umfang:** Den unveränderten Paketkandidaten aus Iteration 4 abschliessend prüfen und als Unterwasser-Abschlussstand kennzeichnen. Das ist eine technische Abschlussprüfung; die gestalterische Abnahme des Nutzers bleibt gültig.

**Prüfung:** Mindestens zwei Stunden gerenderter Betrieb desselben Pakets mit einer dokumentierten Route, die Schwimmen, Nachladen und wiederholtes Auf-/Abtauchen enthält (PERF-010). Speicherverlauf und Fehlerprotokoll erfassen. Zusätzlich kurzer Spielablauf je Uferzugang: Menü → Einstieg → Tauchgang → Lampe/Computer/Pause → Rückkehr → Logbuch → Neustart. Vorhandene Automationsprüfungen und CI müssen für den finalen Quellstand bestehen. Die in Iteration 1–4 erzeugten Nachweise werden gebunden statt grundlos vollständig wiederholt; nach relevanten Änderungen nur betroffene Kontrollen erneut ausführen.

**Fertig, wenn:** Kein Absturz, Geräteverlust oder fehlgeschlagener Spielablauf; kein fortlaufendes Speicherwachstum bei wiederholtem gleichartigem Streckenabschnitt; finaler Paketstand, Launcher, Build-Commit, Prüfsummen und Ergebnisse stimmen überein. README und Restpunktliste benennen klar, was ausgeliefert und was vertagt ist. Alle fünf Iterationen tragen einen belegten Abschlussstatus.

**Ergebnis:** Gesicherter Unterwasser-Abschlussstand. **Die nächste Entwicklungsiteration arbeitet an der Überwasserlandschaft**: Gelände-/Felstopologie, Ufer, Wiesen und Wald, danach Materialien und atmosphärische Gesamtwirkung auf dem gewünschten Referenzniveau. Es wird keine sechste Unterwasser-Optimierungsrunde eröffnet.

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
| UW-LATER-06 | Vollständige reale Maus-/FPV-Ergonomieabnahme und zusätzliche Menü-/HUD-Wünsche | Bestehende UI erhalten und in Iteration 3/5 auf Regression prüfen; umfassende weitere Gestaltung bleibt eigene Arbeit |
| UW-LATER-07 | Verbliebener heller Saum und kurze breite blaue Streifen direkt beim Grenzflächendurchgang | In Iteration 1 mit altem und korrigiertem Material an festen Simulationszeiten nachgewiesen; Saum vermindert, keine vollständige Fehlerbehebung. Ausdrückliche Abweichung vom ursprünglichen Abschlusskriterium; spätere gezielte Grenzflächenarbeit, keine neue Unterwasser-Gestaltungsrunde |

Quellen: [visuelle Vorgabe](VISUAL_TARGET.md), [Anforderungsbasis](REQUIREMENTS_CONTEXT.md), [Originalanforderungen](REQUIREMENTS.csv), [Testorte](TEST_ENTRY_LOCATIONS.md), [Wasserlinienkorrektur](WATERLINE_BLEND.md), [Source-Engine](SOURCE_ENGINE.md), [abgeschlossene Schattenoptimierung](REFLECTION_SHADOW_STUDY.md).

## Fortschritt

| Iteration | Status | Abschlussnachweis |
| --- | --- | --- |
| 1 – Wasserlinie | Prüfrunde beendet mit Abweichung | [Übergangsbericht](WATERLINE_SEAM_STUDY.md); ursprüngliches Fehlerfrei-Kriterium nicht erfüllt, Restpunkte ausdrücklich unter UW-LATER-07 vertagt |
| 2 – Licht-Randfälle | Bestanden | [Licht- und Schattenprüfung](REFLECTION_FINISH_VALIDATION.md); acht Grundkontrollen und vier physische Randfallkontrollen bestanden, vereinigter VSM-Pfad für Paketintegration ausgewählt |
| 3 – Spielpaket | Geplant | Ausstehend |
| 4 – Leistung | Geplant | Ausstehend |
| 5 – Stabilität und Übergabe | Geplant | Ausstehend |
