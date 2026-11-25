# Langzeitprüfung des gerenderten Spiels

PERF-010 verlangt mindestens zwei Stunden Spielstabilität. Synthetische Zwei-Stunden-Protokolle und beschleunigte Kernprüfungen erfüllen diesen Laufzeitnachweis nicht.

`Tools/Soak.ps1` startet genau eine eigene Windows-Spielinstanz. Der Standard ist 7.200 gemessene Sekunden nach 15 Sekunden Aufwärmen, native 1920×1080, Qualitätsstufe 2, ohne dynamische Auflösung, VSync oder FPS-Limit. Die Testvorgaben werden nicht als persönliche Einstellungen gespeichert. Der Modus ist Exploration; Gasversorgung und Tarierung folgen dessen normalen Regeln. Die Physiologie wird nicht als klinisch validiert behandelt.

Der Automat steuert über die regulären Schwimmkräfte, Widerstand, Kollision und Trägheit. Er setzt die Spielfigur nicht entlang einer vorgegebenen Kurve um. Der Kurs pendelt zwischen 16 und 240 Metern vom Einstieg; Kamera/Schwimmrichtung drehen begrenzt mit 90°/s. Die Richtung ist für Weesen ostwärts, Murg nordwärts und Walenstadt westwärts. Vor dem Start werden die Kurslinie und ein Korridor von ±4 m alle acht Meter gegen die unveränderte vermessene Bathymetrie geprüft. Die lokale Gefällerichtung am Einstieg erwies sich in Weesen als ungeeignet: Sie verlässt nach ungefähr 120 m die vermessene Wasserfläche. Dieser erste abgewiesene Vorlauf bleibt unter `artifacts/soak-300s-entry0-20261125-100933` dokumentiert.

In jedem Fünf-Minuten-Zyklus werden zunächst acht Meter Tiefe angesteuert, begrenzt durch den örtlichen Bodenabstand. Die letzten 60 Sekunden verwendet der Automat aufwärts gerichtete Schwimmbewegung für die Oberfläche. Ein Lauf ab 300 Sekunden muss tatsächlich beide Phasen nachweisen. Regelmäßige Protokollsicherung, Beleuchtung, Wasser, Tiere, Pflanzen, Audio und neun lokal nachgeladene Geländekacheln bleiben aktiv. Der Tauchgang läuft ohne Pause; die normale Spielinstanz wird nicht ersetzt oder neu gestartet.

Eine vorausgerichtete Kapselabfrage erkennt feste Hindernisse. Bei einem Treffer steigt der Testtaucher mit regulärer Schwimmkraft an und hält vorübergehend mehr Abstand zum Boden; die Kollisionskörper werden weder abgeschaltet noch übersprungen. Der Vorlauf ohne diese Steuerung blieb an einem Felsen stehen und konnte deshalb den verlangten Kurswechsel nicht nachweisen. Seine Daten bleiben unter `artifacts/soak-300s-entry0-20261125-101304` erhalten.

## Messung und Nachweise

- Tatsächliche Abstände zwischen Spielticks werden mit einer monotonen Uhr gemessen. Es handelt sich um Bildintervalle des Spielprozesses, nicht um isolierte GPU-Zeiten.
- Eine feste Histogrammgröße bewahrt die Verteilung mit 0,25-ms-Bins. P95/P99 sind obere Bin-Grenzen; der Überlauf ab 1.024 ms verwendet das exakte Maximum. Gesamtdauer, Bildzahl und Überschreitungen von 16,667/33,333/100 ms werden exakt gezählt.
- Alle 30 Sekunden wird ein kompakter JSON-Zwischenbericht angehängt. Summen von Dauer, Bildern und Rucklern müssen zum Gesamtbericht passen. Ein fehlendes Intervall oder ausgelassener langer Frame wird abgewiesen. Es wird keine stundenlange vollständige Unreal-CSV-Tabelle im Speicher gesammelt.
- Prozessspeicher wird alle 30 Sekunden abgetastet. Das gemeldete Maximum ist daher ein **abgetasteter Wert**, kein garantierter Spitzenwert zwischen Abtastungen. Endgültiges Speichern und erneutes Lesen können den letzten Speicherwert erhöhen.
- Der Test hält native Start-/Endaufnahmen und bei längeren Läufen alle zehn Minuten eine Aufnahme fest. Aufnahmebedingte Verzögerungen während des Messfensters bleiben in den Bildzeiten enthalten. Eine hohe Einzelzeit wird nicht ohne zusätzliche Evidenz einer bestimmten Ursache zugeordnet.
- Der Endpunkt verlangt sowohl die angeforderte echte Dauer als auch entsprechend fortgeschrittene Simulationszeit, Bewegung, Kurswechsel, mindestens drei besuchte Kachelzentren, gültige Kachelzahlen und erfolgreiche automatische Sicherungen. Die endgültige Logdatei wird erneut gelesen und gegen die aufgezeichneten Messpunkte geprüft.
- Die endgültige Speicherung erfolgt nach dem eigentlichen Messfenster; ihre Dauer wird separat als `finalization_ms` gemeldet. Start-/Endaufnahmen und der gespeicherte Tauchgang werden zusätzlich in den lokalen Artefaktordner kopiert und gehasht.

`Host.json` hält Prozess-ID, Startzeit, ausführbare Datei/Prüfsumme, Quell-Commit und den Zustand des Arbeitsbaums fest. Bei Überschreiten der Beobachtungsfrist wird der Prozess nicht automatisch beendet oder neu gestartet. Ein laufender Test ist anhand dieser Prozess-ID und des Runtime-Logs zu überprüfen. Erst ein reguläres Ende mit `DIVE_SOAK_PASS` und erfolgreicher unabhängiger Auswertung ist ein bestandener Lauf.

Der am 25. November um 09:31:49 UTC begonnene 7.200-Sekunden-Lauf prüft Stabilität bei paralleler Entwicklungsarbeit. Zusätzliche Editor-/Package-Builds, Tests ohne Rendering und kurze gerenderte Dialogtests mit 30-FPS-Limit werden mit Start-/Endzeit in `artifacts/soak-7200s-entry0-20261125-103131/DevelopmentActivity.jsonl` festgehalten. Sie verwenden ein separates Package; das gemessene `WindowsSoak`-Package bleibt unverändert. Sämtliche dabei entstehenden Bildintervalle bleiben in der Messung. Seine FPS-Werte sind deshalb **kein ungestörter Leistungsbenchmark**; der Zweistunden-Stabilitätsnachweis bleibt bis zum regulären erfolgreichen Ende offen.

## Ausführen

```powershell
./Tools/Build.ps1 -Target Editor
./Tools/Build.ps1 -Target Package -PackageDirectory Builds/WindowsSoak
./Tools/Soak.ps1 -PackageDirectory Builds/WindowsSoak -Seconds 300
./Tools/Soak.ps1 -PackageDirectory Builds/WindowsSoak -Seconds 7200
```

Ein 300-Sekunden-Lauf ist ausschließlich eine Vorprüfung. Die Auswertung markiert `two_real_hours` nur, wenn beide gemessenen Zeiten mindestens 7.200 Sekunden betragen. Ein bestandener Test an einem Einstieg, bei einem Wetter und auf einem Rechner beweist keine universelle Stabilität, Mindesthardwareleistung oder macOS-Unterstützung. Leistungswerte und sichtbare Ruckler sind getrennt vom Kriterium „kein Absturz“ zu bewerten.

## Bestandene Vorprüfung vom 25. November 2026

`artifacts/soak-300s-entry0-20261125-102215` besteht mit 300,020 s echter Messzeit und 300,020 s fortgeschrittener Simulation. Der Taucher legt 330,27 m zurück, wendet einmal, besucht fünf Kachelzentren und erreicht Oberfläche sowie 8,18 m Tiefe. Zehn automatische Sicherungen gelingen ohne Fehler; das endgültig erneut gelesene Protokoll enthält 317 Messpunkte. Beide nativen Aufnahmen wurden visuell geprüft. Die Hindernissteuerung löst acht Ausweichvorgänge aus; sie ist eine Teststeuerung und kein Vorbild für sicheres Tauchen.

36.643 Bildintervalle ergeben 122,14 FPS im Mittel, P95 ≤9,75 ms und P99 ≤10,75 ms. Ein Intervall beträgt 180,92 ms und überschreitet alle drei Rucklerschwellen; es bleibt vollständig enthalten. Das abgetastete Prozessspeichermaximum liegt bei 2.427,64 MiB gegenüber 2.380,08 MiB zu Messbeginn. Der abschließende Schreibvorgang benötigt außerhalb des Messfensters 40,43 ms. Der zuvor geöffnete Unreal-Editor blieb während des Tests offen.

Editor- und Package-Build, C++-Kernprüfungen und 15 Python-Prüfungen bestehen. Werte, Build-Prüfsumme und Artefakt-Hashes sind in [SOAK_PRECHECK.json](SOAK_PRECHECK.json) festgehalten. Dieser Vorlauf wurde vor dem Commit mit entsprechend als verändert gekennzeichnetem Arbeitsbaum ausgeführt. Er erfüllt ausdrücklich noch nicht PERF-010.
