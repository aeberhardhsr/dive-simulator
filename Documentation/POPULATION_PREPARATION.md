# Zellvorbereitung ohne langen Einzelblock

**Spielbare Vorschau: `Builds/WindowsPrepare/Start-WaterPreview.cmd`.** Sie enthält Menü, HUD und FPV aus `WindowsCurrent` sowie die aufgeteilte Fels-/Pflanzenvorbereitung. Das bisherige Paket bleibt als Vergleich erhalten. Gebaut mit der installierten Unreal Engine 5.8.4; die experimentellen Verfahren der Quellcode-Engine sind weiterhin nicht eingeschaltet.

Der reproduzierbare Walenstadt-Nachladeruckler sinkt in zwei Paketmessungen von rund 206 auf 55–57 ms. Die Vorbereitung selbst sinkt von 148–149 auf 7,82–8,10 ms pro Frame. Das ist eine Verbesserung der Spielbarkeit, keine neue Grafikstufe und noch keine durchgehend ruckelfreie Darstellung.

## Vollständiger Vergleich vom 27. November 2026

RTX 3090, Ryzen 9 9950X3D, 32 GB RAM; Walenstadt, Nachladestrecke, 1920 × 1080, Hoch, 100 Prozent Renderauflösung, Wasserprofil, ohne VSync oder FPS-Limit. Jeder Lauf misst 60 Sekunden nach 15 Sekunden Aufwärmen. Keine weitere Spiel-/Editor-/Compilerinstanz lief während der Messungen. Hintergrundanwendungen und schwankende Rechnerlast bleiben mögliche Einflüsse.

| Paket / Lauf | Mittel | 1%-Low | Längster Frame | Frames über 33,33 ms | Zellvorbereitung maximal |
| --- | ---: | ---: | ---: | ---: | ---: |
| WindowsCurrent / 1 | 79,61 FPS | 30,24 FPS | 205,64 ms | 13 / 4775 | 148,02 ms |
| WindowsCurrent / 2 | 79,12 FPS | 29,83 FPS | 205,90 ms | 14 / 4746 | 148,98 ms |
| WindowsPrepare / 1 | 79,82 FPS | 36,09 FPS | 56,69 ms | 9 / 4787 | 7,82 ms |
| WindowsPrepare / 2 | 80,08 FPS | 35,84 FPS | 55,19 ms | 9 / 4803 | 8,10 ms |

Alle inneren Frames bleiben enthalten; nur je ein Randframe wird wie im bestehenden Auswerter verworfen. Die fünf Inhaltscontainer sind zwischen beiden Paketen bytegleich. EXE, Quellen, Container, vier vollständige CSV-Aufzeichnungen, Abschlussbilder und Testprotokolle sind im [Prüfbericht](POPULATION_PREPARATION_RESULTS.json) gebunden. Die neue EXE hat SHA-256 `334faab7f46f150863f658de8c6563c82a86148f8c14c58ebc136d525e376e62`.

Die verbleibenden Spitzen liegen weiter auf dem Spielthread. Am jeweils längsten Frame zeigt der vorhergehende CSV-Datensatz rund 55–56 ms Spielthreadzeit, davon unter anderem 18–19 ms Warten auf Physik; der GPU-Wert liegt dort um 12 ms. Diese verschachtelten Zeitbereiche dürfen nicht addiert werden. Weitere Arbeit beim Zellwechsel bleibt zu untersuchen. Zwei kurze Läufe beweisen weder Mindesthardwaretauglichkeit noch Langzeitstabilität.

## Änderung und Grenzen

Zuvor berechnete `BeginPopulationCell` alle benötigten Felslayouts und Pflanzen in einem Aufruf. Jetzt werden vollständige Ankerreihen nacheinander verarbeitet. Zwischen Reihen wird ein Zielbudget von 2 ms geprüft, zusätzlich höchstens 32 Reihen je normalem Aufruf. Eine einzelne Reihe darf dieses Ziel überschreiten; gemessen wurden bis 8,10 ms. Das ist ausdrücklich kein hartes Frame-Limit.

Zufallsfolgen, Reihenfolge, Nachbarschaftsränder und Zellbesitz bleiben erhalten. Unfertige Felslayouts gelangen nicht in den gemeinsam genutzten Cache. Die Pflanzen berücksichtigen weiterhin benachbarte Felsen. Erst nach vollständiger Vorbereitung beginnt die bisherige Registrierung von höchstens einer Instanzkomponente je Aufruf. Sichtbar wird weiterhin nur eine vollständige Zelle.

Der vollständige räumliche Felsindex und die 80-m-Nachbarschaft bleiben verfügbar. Einstieg, nahe fehlende Zellen und Teleports dürfen weiterhin sofort fertiggestellt werden. Ein abgebrochener Aufbau verwirft seine Vorbereitungsdaten und führt bereits registrierte Komponenten dem bestehenden Abbau zu. Der alte diagnostische Pflanzenverteilungsmodus behält seinen zusammenhängenden Zufallsstrom.

## Geprüft

- Editorvergleich: vollständiger und aufgeteilter Aufbau, sechs betrachtete Aufnahmen; beide Fälle bestanden.
- Paket: sieben Fälle mit 30 betrachteten Aufnahmen — Weesener Uferroute, vollständiger/aufgeteilter Aufbau in Weesen, Murg, Walenstadt, Nachladestrecke und Sediment.
- Frisch vollständig erzeugte Felslayouts stimmen ohne Toleranz mit den aufgeteilt erzeugten Cachelayouts überein. Pflanzen werden unabhängig mit vollständigen Nachbarlayouts geprüft; für die quantisierten Instanzdaten gelten 0,25 cm Positionstoleranz und 0,00001 bei Skalierung/Rotation.
- Prüfungen für erhaltene/wiederhergestellte Zellen, nahe Ortswechsel, Fischzustände, Kontakte und begrenzte Caches bestehen. Abbruch während der Vorbereitung und nach der ersten Registrierung wird separat geprüft.
- Haupt-EXE startet über den Bootstrap, aktiviert das Wasserprofil und beendet den automatischen Tauchtest erfolgreich.

Die Startdiagnose zum fehlenden CelestialVault-Standardmaterial bleibt vorhanden und ist im Bericht enthalten. Die Bildprüfung zeigt weiterhin flache Unterwasserbeleuchtung, harte Übergänge an der Wasserlinie, eine sehr helle Sediment-Startaufnahme und unzureichende Oberflächendetails an Land. [Grafikreferenz](VISUAL_TARGET.md), vollständige [Menü-/HUD-/FPV-Abnahme](UI_UX_TARGET.md) und echte Mausprüfung bleiben offen.

Reproduktion: `Tools/TestStreamingBuild.ps1 -Packaged -PackageDirectory Builds/WindowsPrepare`; danach zweimal `Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsPrepare -WaterPreview -Scenario stream -Entry 2 -Resolution 1080p -Quality 2 -Seconds 60`. Der Bericht wird mit `Tools/ReportPopulationPreparation.py` aus den gebundenen Test- und Messordnern erzeugt.
