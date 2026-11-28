# Unterwasser-Abschlussstand und Übergabe an Überwasser

Stand: 28. November 2026. **Die fünf begrenzten Arbeitsrunden sind beendet. Die vollständige technische Abnahme ist nicht erreicht.** Die gestalterische Abnahme des Nutzers bleibt gültig. Gemäss seiner Vorgabe werden verbleibende Punkte dokumentiert und die Entwicklung wechselt zur Überwasserlandschaft; es gibt keine sechste Unterwasser-Optimierungsrunde.

## Ergebnis der fünf Runden

| Runde | Ergebnis | Noch nicht erfülltes Kriterium |
| --- | --- | --- |
| 1 – Wasserlinie | Beendet mit Abweichung: Saum vermindert, bestehende Artefakte eingegrenzt | Restsaum und blaue Grenzflächenstreifen; UW-LATER-07 |
| 2 – Licht | Bestanden: acht Grundkontrollen und vier gezielte Randfälle | Keine zusätzliche Einschränkung innerhalb dieses Prüfumfangs |
| 3 – Spielpaket | Beendet mit Abweichung: 18 native Fälle, 84 Bilder, zwölf Editorvergleiche; normaler Start und reale Menü-/Instrumentenprüfung | Vollständige reale Schwimm-/Rückkehrfolgen an allen drei Ufern nicht belegt; UW-LATER-06. Softwarepfad eingeschränkt; UW-LATER-08 |
| 4 – Leistung/Nachladen | Sechs kurze Messläufe bestehen; ergänzende bewegte Sichtprüfung in Murg ohne beobachtete Nachladelücke | Aussage gilt für geprüfte Strecken. Keine Garantie für den ganzen See oder durchgehend 60 FPS; UW-LATER-05. Statische Farblinie bleibt UW-LATER-10 |
| 5 – Stabilität/Übergabe | Beendet mit Abweichung: zwei Stunden gerenderter Betrieb ohne Absturz/Geräteverlust, 240 erfolgreiche Sicherungen; Paketidentität bestätigt | Striktes Speicherkriterium nicht erfüllt; UW-LATER-09. Vollständige reale Bedienfolgen bleiben unvollständig |

Eine beendete Arbeitsrunde ist bei ausgewiesener Abweichung **kein bestandenes Pflichtkriterium**. Der [ursprüngliche Plan](UNDERWATER_FINISH_PLAN.md) behält seine Kriterien; die offenen Anforderungen werden nicht nachträglich abgeschwächt. Die bisherigen JSON-Berichte bleiben unveränderte historische Messnachweise. Dieser Bericht ergänzt deren damalige offene Statusangaben um die abschliessende Sichtprüfung und die Entscheidung zum Schwerpunktwechsel.

## Ergänzte Nachladeprüfung in Murg

Der unveränderte R3-Kandidat durchschwamm einen sichtbaren flachen Bodenabschnitt über die 64-m-Zellgrenze und zurück: rund 52 m, 41,58 s Simulationszeit, zwei Zellübertritte. Der Streckenmarker meldet 52,012 m, die kurz danach abgeschlossene Aufzeichnung 52,074 m. Alle 215 fortlaufend nummerierten Bilder wurden in 18 Kontaktbögen gesichtet; sechs Bilder im Umfeld der beiden Übergänge und das zusätzliche Abschlussbild wurden in voller Auflösung geprüft. **Keine sichtbaren Bodenlöcher, schwarzen Ersatzflächen oder Aufblitzer an den geprüften Übergängen.**

Die Aufnahme beansprucht etwa fünf Bilder pro Sekunde und ist kein Bildratenbenchmark. Zwei explizite Zwischenaufnahmen wurden durch denselben Dateinamen überschrieben; eine lückenlose Aufzeichnung jedes gerenderten Bildes wird daher nicht behauptet. Die anfängliche Versetzung und die gesteuerten Blickwendungen gehören zum Prüfszenario. Die Eingaben stammen aus der nativen Teststeuerung, nicht aus einer vollständigen manuellen Bedienprüfung. Der abschliessende Logbuchstatus ist durch das Testende unterbrochen; kein vollständig abgeschlossener Nutzertauchgang wird daraus abgeleitet.

[Bildfolge, Protokolle, Prüfsummen und Grenzen](UNDERWATER_STREAMING_VISUAL_R3.json). Lokal: `artifacts/underwater-package-20261128/murg-coast-continuous/Murg-Diagnose-5fps.mp4` (43 s, 1600 × 900, feste Wiedergabe mit 5 FPS).

## Statischer Befund aus dem Dauerlauf

Die schmale entfernte dunkle Linie wurde an einer angenäherten Dauerlauf-Kameraposition mit vollständig geladenem lokalem Gelände reproduziert. Sie bleibt bei abgeschalteten dynamischen Schatten sichtbar. Eine davon getrennte grössere dunkle Schattenfläche verschwindet bei diesem Diagnoseschalter. Die bestätigte SceneDepth-Darstellung im Source-Editor zeigt keinen offensichtlichen grossen Tiefenspalt; die genaue Ursache der dünnen Linie ist damit **nicht bewiesen**.

Der SceneDepth-Versuch im Paket wurde mangels bestätigter Pufferumschaltung verworfen. Nur der bestätigte Editorlauf wird als Tiefendiagnose gewertet. Die neun gültigen Ansichten wurden gesichtet. Der Restbefund wird als UW-LATER-10 aufbewahrt; er ist weder als natürlicher Effekt erklärt noch durch eine produktive Änderung behoben.

## Unveränderter spielbarer Kandidat

- Start: `Builds/WindowsUnderwaterFinalCandidateR3/Start-DiveSimulator.cmd`.
- Kompilierte Projekteingaben: `6facc185f48d8975296b1f09acc169d7919eb429`.
- Alle 53 Laufzeitdateien und 174 ursprünglichen Projekteingaben stimmen erneut mit dem [Paketnachweis](PRODUCTION_PACKAGE_R3_EVIDENCE.json) überein.
- Die spätere editorseitige Speicherdiagnose ist eine zusätzliche, nur ausdrücklich gestartete Prüfung und nicht Bestandteil des R3-Pakets.
- Der bisherige Rückfallstand `Builds/WindowsWaterline/Start-WaterPreview.cmd` bleibt erhalten. R3 wird nicht als vollständig technisch abgenommene Veröffentlichung bezeichnet.

Die [Paketintegration](UNDERWATER_PACKAGE_INTEGRATION.md), [sechs kurzen Leistungsläufe](UNDERWATER_FINISH_PERFORMANCE_R3.json), der [zweistündige Dauerlauf](UNDERWATER_FINISH_SOAK_R3.json) und die [begrenzte Speicherdiagnose](RECORDING_MEMORY_DIAGNOSIS.md) bleiben getrennte Nachweise. Im Dauerlauf wurden 55,76 FPS im Mittel und ein Maximum von 278,20 ms gemessen; die guten kurzen Messläufe ersetzen diese Einschränkung nicht. Der positive Speichertrend ist nicht abschliessend zugeordnet und kein nachgewiesener Speicherleckbefund.

Das Nutzerfeedback bestätigt Murg und Weesen ohne Aufblitzen oder Bedienprobleme. Es wird nicht auf Mols oder nicht berichtete vollständige Rückkehr-/Logbuchfolgen ausgeweitet.

## Nächster Entwicklungsschwerpunkt

Die Überwasserlandschaft beginnt mit reproduzierbaren Paketansichten an Weesen, Murg und Mols, jeweils Richtung See/Berge und Richtung Land. Daraus werden die grössten sichtbaren Defizite bei Berg- und Felstopologie, Uferanschlüssen, Waldvolumen und Wiesen abgeleitet. Die tatsächliche Walensee-Geografie und die [visuelle Referenz](VISUAL_TARGET.md) bestimmen die Arbeit. Ein zusätzlicher Unterwasser-Gestaltungszyklus wird nicht eröffnet.
