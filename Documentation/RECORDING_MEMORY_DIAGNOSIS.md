# Speicherpfad: begrenzte Diagnose nach dem R3-Dauerlauf

Stand: 28. November 2026. **Diagnose abgeschlossen; Speicherkriterium des gerenderten Spiels weiterhin offen.** Das R3-Paket und seine produktive Aufzeichnungslogik wurden nicht verändert. Es wurde ausschliesslich ein ausdrücklich aufrufbarer Editor-Diagnosetest ergänzt.

## Ergebnis

Der Speicherpfad allein zeigt bei wachsender Aufzeichnung einen steigenden Speicherbedarf. Bei anschliessend gleicher Datenmenge ist in diesem begrenzten Versuch keine vergleichbare fortlaufende Zunahme zu sehen. Das ist mit einem grössenabhängigen Allokationsbedarf und zurückgehaltenen Heap-Seiten vereinbar; die einzelnen Allokationseigentümer wurden nicht verfolgt. Eine Speicherleck-Ursache ist weder bewiesen noch ausgeschlossen. Der gesamte Anstieg des gerenderten R3-Dauerlaufs lässt sich damit noch nicht zuordnen.

Der Windows-Prozessspeicher wurde nach jeder abgeschlossenen Sicherung über `FPlatformMemory::GetStats().UsedVirtual` erfasst. Diese Engine-Version liest dort `PROCESS_MEMORY_COUNTERS.PagefileUsage`, also den zugesicherten Prozessspeicher. Das ist **nicht** der reservierte virtuelle Adressraum und **nicht** der RAM-Arbeitssatz. Kurzzeitige Spitzen während der Serialisierung fehlen in diesen Messpunkten.

| Abschnitt | Kontrolllauf: Kopie ohne Schreiben | Produktiver Speicherpfad |
| --- | ---: | ---: |
| 240 wachsende Aufzeichnungen, 31 bis 7’201 Messpunkte | +9,27 MiB zwischen erster/letzter Probe | +66,04 MiB zwischen erster/letzter Probe |
| 60 weitere Aufzeichnungen mit je 7’201 Messpunkten | 0,00 MiB Endwertänderung | +0,25 MiB Endwertänderung; gesamte Spannweite 4,75 MiB |
| 60 weitere Aufzeichnungen mit je 31 Messpunkten | 0,00 MiB Endwertänderung | 0,00 MiB Endwertänderung |
| Nach Freigabe des Messpunktarrays, gegenüber Start | 0,00 MiB | +63,29 MiB |
| Maximale Kapazität des eigenen Messpunktarrays | 4,43 MiB | 4,43 MiB |

Die Dauer der gesamten beschleunigten Messung beträgt 0,056 s im Kontrolllauf und 198,16 s mit Schreiben. **Die Prozesse sind nicht zeitlich abgeglichen**; die Differenz ihrer Kurven darf nicht als exakt isolierter Speicheranteil abgezogen oder auf MiB pro Spielstunde hochgerechnet werden. Der Schreibprozess gibt zu Beginn zudem Speicher frei, bevor der wachsende Verlauf einsetzt. Der Abschnitt bei konstanter grosser Datenmenge umfasst etwa 67 s, keinen weiteren zweistündigen Nachweis.

Alle 360 Sicherungen erfolgreich, beide Diagnoseprozesse mit Exit 0 beendet. Eine grosse Sicherung benötigt im konstanten Abschnitt durchschnittlich 1’114 ms auf dem Hintergrund-Worker. Das ist keine entsprechende Bildzeit auf dem Spielthread. Der Rückgang auf kleine Aufzeichnungen verwendet denselben Dateipfad; beim ersten Wechsel wird daher noch die grosse vorherige Generation als Backup behandelt. Er ist kein vollständiger Menü-Neustart eines neuen Tauchgangs.

[Maschinenlesbare Auswertung mit Prüfsummen](RECORDING_MEMORY_DIAGNOSIS.json). [Diagramm](../artifacts/recording-memory-r3-isolation-20261128/RecordingMemory.png) lokal vorhanden und visuell geprüft. Rohdaten, Protokolle, Modulidentität und automatischer Testbericht bleiben im referenzierten Artefaktordner.

## Erklärung aus dem vorhandenen Code

`FDiveRecordingWriter` hält maximal einen aktiven Worker und eine zusammengefasste ausstehende Aufzeichnung. Eine unbeschränkt wachsende Auftragswarteschlange ist hier nicht vorhanden.

`WriteRecording` baut dagegen bei jeder Sicherung den gesamten bisherigen Verlauf als JSON-Objektbaum und Zeichenfolge auf. `DiveLogbook::Write` liest die neue Zeichenfolge zur Validierung nochmals ein und prüft die vorherige Datei vor der Backupsicherung. Der ursprüngliche Objektbaum und die Messpunktkopie bestehen währenddessen weiter. Die Aufzeichnung selbst wächst im Spiel um einen Messpunkt pro Sekunde. Sowohl notwendige Nutzdaten als auch temporärer Allokationsbedarf nehmen dadurch mit der Tauchdauer zu. Diese Codebefunde erklären einen plausiblen Beitrag, aber nicht quantitativ den gesamten R3-Prozessspeicher.

## Begrenzung und Folgerung

Die Diagnose nutzt die unveränderten produktiven Funktionen in einem separaten Source-Editor mit Null-RHI. Es gibt keinen laufenden Renderer, keine Nachladeroute, keine Fotos und keinen fortschreitenden Spieltick. Ein wiederholter gültiger Modellzustand ersetzt den realen Tauchgang. Daher wird **das ursprüngliche R3-Speicherkriterium nicht grün gesetzt** und kein neuer Grafikstand ausgeliefert.

Eine spätere Optimierung kann den vollständigen JSON-Objektbaum beim Schreiben vermeiden beziehungsweise früher freigeben. Sie muss Dateischema, Validierung, Backup und Wiederherstellung erhalten und braucht einen eigenen Vergleich. Dafür wird hier keine zusätzliche Unterwasser-Gestaltungsrunde eröffnet. Die Diagnose ist beendet; die verbleibenden begrenzten Abschlussarbeiten betreffen sichtbares Nachladen und die noch unbelegten Spielabläufe.

## Wiederholung

Der Test liegt absichtlich ausserhalb der normalen `DiveSim`-Abnahmeserie und verändert deren Testanzahl nicht. Der Build dauerte 20,88 s, einschliesslich vier Projektaktionen; kein kompletter Engine-Neubau.

```powershell
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Editor -MaxParallelActions 4
./Tools/DiagnoseRecordingMemory.ps1 -ResultDirectory artifacts/recording-memory-new-run
```

Ein neuer Zielordner ist erforderlich. Das Werkzeug startet Kontroll- und Schreiblauf nacheinander, prüft Exitcode, jeweils einen bestandenen Automationstest sowie alle 362 Speicherproben. Es bewertet kein Speicher-Abnahmekriterium automatisch. Ein Beobachtungszeitlimit lässt den Prozess zur Untersuchung bestehen und nennt seine ID; vor einer Wiederholung ist dessen Zustand zu klären.
