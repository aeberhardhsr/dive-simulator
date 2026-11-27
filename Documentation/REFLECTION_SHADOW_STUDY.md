# Abschluss: vorhandene Schattenkarten für Wasserreflexionen

Stand: 28. November 2026. Die laufende Schattenoptimierung ist als begrenzter Render-Versuch abgeschlossen. In der geprüften Szene steigt die durchschnittliche Bildrate von 55,19 auf 62,29 FPS (+12,86 %). Alle 16 Wasserabschnitte bleiben erhalten. Die Variante ist ein Kandidat für die spätere Paketintegration; das bisherige spielbare Paket wurde in diesem Schritt nicht geändert.

## Umsetzung

Der separate Quellengine-Build bindet die VSM-Ressourcen der aktuellen Ansicht an den Lumen-Hardware-Reflexionspass. Der Shader verwendet eine Schattenkarte nur bei gültigen Ressourcen und genau einer passenden gerichteten Lichtquelle. Fehlende oder mehrdeutige Zuordnung, ungültige Kartenseiten sowie ausgeschlossene grobe Nicht-Nanite-Geometrie behalten den bisherigen Schattenstrahl. Punkt- und Spotlichter verwenden weiterhin ihre bisherigen Strahlen.

`PrepareReflectionShadowPatch.py` prüft vier bekannte Engine-Dateistände und erzeugt vor der Anwendung einen Diff und ein Manifest. Die bereits vorhandene Wasser-Scene- und Reflexionsbindung ist Voraussetzung. Nur `Builds/UE584` wird verändert; die installierte Engine bleibt unverändert. Die Engine meldet ihre Bindungsversion über `r.DiveWater.ReflectionShadowBindingVersion`. Projekt und Testwerkzeuge prüfen angefordertes Profil, vorbereitete Shader und Modulstände.

Die Profile sind `vsm` (normales Bild), `vsm-fallback` (erzwungener bisheriger Schattenstrahl) und `vsm-coverage` (farbige Diagnose). Die normale Werkzeugvorgabe bleibt `full`. Der neue Weg ist weiterhin an den ausdrücklich gestarteten Reflexionsversuch gebunden.

## Nachweise

Der korrigierte Source-Editor-Build war erfolgreich (141,43 s). Die vier angewendeten Engine-Dateien stimmen mit dem vorbereiteten Manifest überein. Der erste Buildversuch scheiterte an einer `struct`- statt `class`-Vorwärtsdeklaration; diese ist vor allen hier berichteten nativen Prüfungen korrigiert worden.

Neun neue native Fälle mit 27 Aufnahmen wurden erfolgreich ausgeführt und bildlich gegen sechs vorhandene Referenzaufnahmen geprüft:

| Prüfung | Fälle / Bilder | Ergebnis |
| --- | ---: | --- |
| Schattenkarten-Diagnose | 2 / 6 | Ausgewählter Reflexionsbereich vollständig grün; tatsächliche Kartennutzung nachgewiesen |
| Normale Uferansichten | 2 / 6 | Unterwasser, Blick zur Sonne und Luftkontrolle erhalten |
| Erzwungener Strahl-Rückfall | 2 / 6 | Mittlere absolute RGB-Abweichung zur vollständigen Referenz 0,249–0,513 Anzeigestufen |
| Wasserlinie | 3 / 9 | Korrigierte Unterwasserreflexion erhalten; Luftkontrolle 0,255 RGB-Stufen Abweichung |
| Vollständige frühere Referenz | 2 / 6 | Bereits geprüfte Vergleichsbilder; kein erneuter Lauf dieser sechs Bilder |

Die Tabelle enthält zusätzlich sechs bestehende Referenzbilder, insgesamt 33 geprüfte Vergleichsbilder. Die farbige Diagnose klassifiziert Bildpixel und ist kein exakter GPU-Abfragezähler. Die normalen VSM-Uferbilder weichen im Mittel um 0,247–0,506 RGB-Stufen von der Referenz ab. Im festgelegten Wasserlinienbereich beträgt der Anteil blau dominierter Pixel in beiden Unterwasseransichten 0 %. Der schmale helle Saum direkt an der Grenzfläche ist weiterhin sichtbar und wird nicht als behoben ausgegeben.

## Leistung

Zwei aufeinanderfolgende bewegte Source-Editor-Läufe auf RTX 3090 / Ryzen 9950X3D, jeweils 15 s Aufwärmen und 60 s Messung, 1920 × 1080, Qualität 2, 100 % Renderauflösung, Blickneigung 25° nach oben. Beide verwenden identische Renderer-, Render- und Spielmodule.

| Messwert | Vollständige Strahlenreferenz | Schattenkarten mit Rückfall |
| --- | ---: | ---: |
| Durchschnitt | 55,19 FPS | 62,29 FPS |
| Langsamstes 1 % | 42,04 FPS | 45,93 FPS |
| 95. Perzentil Bildzeit | 19,43 ms | 17,38 ms |
| Mittlere GPU-Zeit | 17,47 ms | 15,40 ms |
| GPU-Pass LumenReflections | 5,82 ms | 3,81 ms |
| Bilder über 33,33 ms | 3 / 3310 | 3 / 3735 |

Die Abschlussbilder wurden geprüft. Schlusskurs und Simulationsdauer unterscheiden sich um 0,36° beziehungsweise 0,06 s; die Endtiefe beträgt 4,35 beziehungsweise 4,15 m. Unterschiedliche Kollisionsschritte erklären einen leicht unterschiedlichen Kamerapfad. Dies ist ein belastbarer Vergleich dieser Szene, kein identischer Einzelbildtest oder Nachweis durchgehender 60 FPS im ganzen Spiel. GPU-Passzeiten sind nicht addierbar. Beide Source-Editor-Läufe enthalten den vorhandenen PythonTestRunner-Initialisierungsfehler des experimentellen Engine-Plugins; die nativen Spiel- und Messabläufe wurden vollständig abgeschlossen.

## Entscheidung und Grenzen

Der Versuch ist erfolgreich abgeschlossen: vorhandene Schattenkarten reduzieren die gemessenen Reflexionskosten, während die geprüften Ansichten und der Strahl-Rückfall erhalten bleiben. Weitere Unterwasserthemen werden nicht an diesen Schritt angehängt. Der Nutzer hat die Unterwasserwelt gestalterisch abgenommen; diese Abnahme setzt keine physikalisch vollständige Lichtsimulation voraus.

Vor einer Übernahme in ein neues Spielpaket bleiben gezielte Randfallprüfungen für tatsächliche Abschattung, Sonne/Mond, Lampen und wechselnde Kartenseiten nötig. Schattenkarten besitzen andere Auflösung und Bias als binäre Nahbereichsstrahlen. Indirektes Himmelslicht, Mehrfachstreuung, weitere Grenzflächenwechsel und vollständige photometrische Abstimmung sind weiterhin mögliche spätere Arbeiten. Sie sind kein unerledigter Bestandteil dieses abgeschlossenen Optimierungsversuchs. Der Source-Game-Build und die Aktivierung beim normalen Paketstart wurden hier nicht vorgenommen.

## Reproduktion

Auf den passenden, noch nicht mit dieser VSM-Bindung gepatchten separaten Engine-Dateien zunächst den Diff vorbereiten und prüfen, dann einmalig anwenden und bauen:

```powershell
Tools/.geodata-env/Scripts/python.exe Tools/PrepareReflectionShadowPatch.py
Tools/.geodata-env/Scripts/python.exe Tools/PrepareReflectionShadowPatch.py --apply
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -Target Editor -EditorSandbox -SandboxName SourceEditorSandbox -MaxParallelActions 4
./Tools/TestReflectionTransport.ps1 -Unified -Suite consistency -Profile vsm-coverage
./Tools/TestReflectionTransport.ps1 -Unified -Suite consistency -Profile vsm
./Tools/TestReflectionTransport.ps1 -Unified -Suite consistency -Profile vsm-fallback
./Tools/TestReflectionTransport.ps1 -Unified -Profile vsm
```

Die beiden Leistungsprofile nacheinander vorbereiten und messen; im folgenden Beispiel `full` anschliessend an beiden Stellen durch `vsm` ersetzen:

```powershell
Tools/.geodata-env/Scripts/python.exe Tools/PrepareReflectionTransport.py --unified --profile full
./Tools/Benchmark.ps1 -EditorSandbox -SandboxName SourceEditorSandbox -EngineRoot 'O:/games/dive game/Builds/UE584' -WaterPreview -ReflectionTransportStudy -ReflectionUnified -ReflectionProfile full -Scenario underwater -Entry 0 -LookPitch 25 -Seconds 60 -Resolution 1080p -Quality 2
```

`ReportReflectionShadows.py` bindet Bilder, Rohmessungen, Einstellungen, Module und Engine-Patch an den [Ergebnisbericht](REFLECTION_SHADOW_RESULTS.json). Die dort erfassten Verzeichnisse sind für diesen Abschluss massgeblich. Für neue Läufe die entsprechenden neuen Verzeichnisse übergeben. Bereits gepatchte Engine-Dateien werden absichtlich nicht erneut überschrieben.
