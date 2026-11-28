# Abschlussrunde 3: normal startbares Wasserpaket

Stand: 28. November 2026. **In Bearbeitung. Nativer Editor-Nachbau und erster gerenderter Integrationslauf bestanden; Source-Game-Build fortgesetzt, noch kein neues Paket.** Die [zweite Abschlussrunde](REFLECTION_FINISH_VALIDATION.md) hat den vereinigten VSM-Reflexionsweg für diese Integration ausgewählt.

## Laufender Build und eingefrorene Eingaben

Der erste monolithische Source-Game-Build verwendete die getrennte Sandbox `Builds/SourceEditorSandbox/divingsim` mit Quellstand `edf27cd728b02eae05020e46536f19980d6d2dea`, vor der normalen Laufzeitintegration. Das Protokoll liegt unter `artifacts/underwater-package-20261128/source-game-build.log`; `FoundationInputs.json` bindet die damaligen Source-, Config- und Shaderdateien. Von 1’117 eingeplanten Aktionen waren 234 abgeschlossen, als der Build bewusst für die vorgezogene Integrationsprüfung unterbrochen wurde. Seine letzten Prozesse wurden anhand der tatsächlichen UnrealBuildTool-Kommandozeile identifiziert und beendet. Der resultierende Exitcode 1 ist kein erfolgreicher Abschluss und kein nachgewiesener Compilerfehler.

Der Grund für die Unterbrechung: Die parallele Integrationsvorbereitung war fertig; ein kurzer Editor-Nachbau konnte deren native Funktion vor dem restlichen langen Game-Build prüfen. Die bereits erzeugten Engine-Objekte bleiben erhalten. Erst nach dem bestätigten Prozessende wurde die Sandbox auf `0751e593504fc43bb0a4a1e7274d100dffeff07a` synchronisiert. Der Editor-Nachbau unter `production-editor-build.log` bestand nach elf Aktionen und 58,99 Sekunden. Zwei bereits vorhandene `double`-zu-`float`-Warnungen in der Kaustikprüfung verhindern den Build nicht. Nach erfolgreichem Renderlauf wurde der Game-Build unter `production-game-build.log` inkrementell fortgesetzt. **Eine laufende Sandbox weiterhin nicht synchronisieren.**

Bis zu vier Aktionen dürfen parallel laufen; tatsächlich begrenzen auch Abhängigkeiten und die Speicherprognose die Ausführung. Eine wegen Speicherdruck angehaltene `PCH.Engine.cpp`-Aktion wurde im Foundation-Lauf erfolgreich wiederholt. Verwaiste MSBuild-Hilfsprozesse des abgeschlossenen Build-Tool-Setups wurden nach Prüfung ihrer Herkunft und des fehlenden Elternprozesses geschlossen. Die Speicher-Schutzgrenzen wurden nicht gelockert.

## Vorbereitete Integration

- `[DiveRendering] ProductionWater=True` wählt auf der separat gepatchten Source-Engine den geprüften Renderweg für normale Starts. Die drei Binding-Versionen müssen vorhanden sein. Die unveränderte installierte Engine erhält dadurch keine inkompatible Szenenstruktur.
- Wasseroberfläche, begrenztes Volumen, Seegrenzenraster und Reflexionsmaske werden ohne Smoke-/Study-Schalter aktiviert. Das Profil bleibt `unified/vsm` mit ursprünglicher Spielbelichtung und ursprünglichen Lichtintensitäten. Die anderen Lichtmodell-Experimente werden nicht freigegeben.
- Der Editor und Cooker verwenden die separat vorbereiteten Shader unter `Intermediate/DiveReflectionProduction`. Ein gekochtes Spiel lädt seine enthaltenen Shader und benötigt keine lokalen Engine-Quelldateien oder `Intermediate`-Dateien. Shader-Experimente werden im neuen gekochten Spiel zurückgewiesen; ein Diagnoseprofil darf keinen normalen Start beeinflussen.
- Die Szenenstruktur bleibt beim Abschalten von Hardware-Raytracing bestehen. Der vorhandene Software-Ersatzpfad muss im Paket gesondert geprüft werden; eine gleichwertige Darstellung ist nicht zugesagt.
- `Start-DiveSimulator.cmd` startet die Paket-EXE ohne Grafik-Testschalter. Der bisherige Vorschau-Launcher bleibt zusätzlich vorhanden. Der neue Paketordner wird `Builds/WindowsUnderwaterFinalCandidate`; bisherige Paketordner werden nicht ersetzt.
- `SmokeTest.ps1 -ProductionWater` ist ausschliesslich eine Erwartungsprüfung des Testwerkzeugs: Es aktiviert keinen Renderzweig per Kommandozeile. Es verlangt die normalen Laufzeitmarker, Seegrenzen und passende Hardware-/Software-Auswahl.

Die Vorbereitung mit `PrepareReflectionTransport.py --production --unified --profile vsm --project divingsim` wurde ausgeführt. Alle 96 vorbereiteten Shaderdateien stimmen bytegenau mit dem ausgewählten VSM-Versuch überein. Ungültige Produktionsanforderungen (fehlende vereinigte Führung oder Diagnoseprofil) werden abgewiesen. Die geänderten PowerShell-Skripte und alle 43 lokalen Python-Prüfungen bestehen. Die GitHub-Prüfungen des Integrationsstands sind in [Push](https://github.com/aeberhardhsr/dive-simulator/actions/runs/46090206490) und [PR](https://github.com/aeberhardhsr/dive-simulator/actions/runs/46090211200) erfolgreich. **Das ersetzt weder Cook noch Paketstart.**

## Erster nativer Integrationsnachweis

`production-editor.log` und `production-editor-summary.log` belegen den erfolgreichen Lauf mit `-ProductionWater` als Werkzeugprüfung und ohne `-DiveWaterPreview`, `-DiveReflectionTransportStudy` oder `-DiveReflectionUnified` in der tatsächlichen Spielkommandozeile. Der Lauf meldet `profile=vsm diagnostic=0`, `unified=1`, `DIVE_PRODUCTION_WATER_READY cooked=0`, die registrierte Wasserszene und das Seegrenzenraster mit 15’586 × 3’003 Zellen. Alle drei Bilder wurden gesichtet: Wasserunterseite mit gespiegeltem Grund, Blickfenster zur Sonne und Überwasseransicht. Keine Shader-Ersatzmaterialien oder nativen Testfehler.

Der [Prüfnachweis](PRODUCTION_EDITOR_EVIDENCE.json) bindet Quellen, Konfiguration, Shader, Projekt-/Renderer-DLLs, Build-/Laufzeitprotokolle und Originalbilder. Eine unverwandte Meldung des experimentellen Editor-Plugins `ToolsetRegistry` über ein fehlendes Python-Attribut `PythonTestRunner` bleibt im Originalprotokoll erhalten; sie hat den nativen Bildtest nicht verhindert. Dieser Nachweis stammt aus dem Editor mit automatisierten Kameras, ausdrücklich noch nicht aus einem gekochten Paket oder einer interaktiven Menüabnahme.

![Erste normale Renderprüfung: zwei Unterwasseransichten und eine Überwasseransicht](Images/ProductionWater_Editor.jpg)

## Noch auszuführen

Der Editor-Nachbau und der erste Smoke-Test aus der folgenden Befehlsfolge sind bereits bestanden. Der Game-Build läuft. Nach dessen erfolgreichem Ende folgen Package und Paketserie; einen noch laufenden oder bereits bestandenen Schritt nicht grundlos nochmals starten:

```powershell
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Editor -MaxParallelActions 4
./Tools/SmokeTest.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -ProductionWater -Scenario rocks -WaterView -WaterStart 0 -WaterEnd 0 -MaxFPS 30 -TimeoutSeconds 1200 -RuntimeLog artifacts/underwater-package-20261128/production-editor.log
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Game -MaxParallelActions 4
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Package -MaxParallelActions 4 -PackageDirectory Builds/WindowsUnderwaterFinalCandidate
./Tools/TestProductionWater.ps1 -PackageDirectory Builds/WindowsUnderwaterFinalCandidate
```

`Build.ps1` bereitet für die Engine mit VSM-Binding die Produktionsshader automatisch vor, bevor ein Editor/Cooker gestartet wird. Der Smoke-Test soll zunächst belegen, dass Konfiguration, Shader und Szenendaten tatsächlich aktiv sind. Erst nach erfolgreichem Nachbau folgt der Cook.

Die Paketserie enthält drei Uferabläufe (Weesen, Murg, Mols), drei direkte Wasserstarts (Weesen, Murg, Walenstadt), je einen festen Wasserlinienvergleich an den drei Uferzugängen, klare/trübe Sicht, Nacht ohne/mit Lampe, physische Sonnen-/Mondabschattung, Software-Ersatzpfad, Menü/HUD und Handgelenkcomputer. Für die Mondkontrolle bleibt die ausdrücklich dokumentierte Messbelichtung auf diesen Test beschränkt. Die Serie protokolliert `CAPTURE_PASS`; Bildsichtung und Vergleich mit den Editorbelegen sind zusätzlich nötig. `-CaseName` erlaubt gezielte Wiederholungen betroffener Fälle.

Zusätzlich ist ein echter normaler Start ohne Smoke-/Study-Schalter und die Bedienung aus dem Menü zu prüfen. Ein Test der automatisierten Szenarien ersetzt diesen Schritt nicht. Erst danach werden Paketidentität, Launcher, Laufzeitkonfiguration, Bildvergleich, Einschränkungen und Build-/Cook-Protokolle als Abschlussnachweis gebunden. Leistungsprüfung und zweistündiger Dauerlauf bleiben die anschliessenden Runden 4 und 5.
