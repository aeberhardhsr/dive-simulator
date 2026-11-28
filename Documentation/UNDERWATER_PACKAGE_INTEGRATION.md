# Abschlussrunde 3: normal startbares Wasserpaket

Stand: 28. November 2026. **In Bearbeitung. Noch kein neues Paket und noch kein nativer Nachweis der unten beschriebenen Laufzeitintegration.** Die [zweite Abschlussrunde](REFLECTION_FINISH_VALIDATION.md) hat den vereinigten VSM-Reflexionsweg für diese Integration ausgewählt.

## Laufender Build und eingefrorene Eingaben

Der erste monolithische Source-Game-Build verwendet die getrennte Sandbox `Builds/SourceEditorSandbox/divingsim` mit Quellstand `edf27cd728b02eae05020e46536f19980d6d2dea`, vor der normalen Laufzeitintegration. Das Protokoll liegt unter `artifacts/underwater-package-20261128/source-game-build.log`; `FoundationInputs.json` bindet die eingefrorenen Source-, Config- und Shaderdateien. Unreal hat 1’117 Aktionen eingeplant. Vier Aktionen dürfen gleichzeitig laufen; die tatsächliche Zahl hängt von Abhängigkeiten und Speicher ab. Eine wegen Speicherdruck angehaltene `PCH.Engine.cpp`-Aktion wurde im weiteren Verlauf erfolgreich abgeschlossen. Das ist kein abgeschlossener Buildnachweis.

Während dieses Builds werden Änderungen nur im Hauptprojekt vorbereitet. **Die laufende Sandbox nicht synchronisieren.** Nach erfolgreichem Abschluss sind zuerst der Editor und dann das Game mit den neuen Projektquellen nachzubauen. Der erste Game-Build allein enthält die folgende Integration noch nicht.

## Vorbereitete Integration

- `[DiveRendering] ProductionWater=True` wählt auf der separat gepatchten Source-Engine den geprüften Renderweg für normale Starts. Die drei Binding-Versionen müssen vorhanden sein. Die unveränderte installierte Engine erhält dadurch keine inkompatible Szenenstruktur.
- Wasseroberfläche, begrenztes Volumen, Seegrenzenraster und Reflexionsmaske werden ohne Smoke-/Study-Schalter aktiviert. Das Profil bleibt `unified/vsm` mit ursprünglicher Spielbelichtung und ursprünglichen Lichtintensitäten. Die anderen Lichtmodell-Experimente werden nicht freigegeben.
- Der Editor und Cooker verwenden die separat vorbereiteten Shader unter `Intermediate/DiveReflectionProduction`. Ein gekochtes Spiel lädt seine enthaltenen Shader und benötigt keine lokalen Engine-Quelldateien oder `Intermediate`-Dateien. Shader-Experimente werden im neuen gekochten Spiel zurückgewiesen; ein Diagnoseprofil darf keinen normalen Start beeinflussen.
- Die Szenenstruktur bleibt beim Abschalten von Hardware-Raytracing bestehen. Der vorhandene Software-Ersatzpfad muss im Paket gesondert geprüft werden; eine gleichwertige Darstellung ist nicht zugesagt.
- `Start-DiveSimulator.cmd` startet die Paket-EXE ohne Grafik-Testschalter. Der bisherige Vorschau-Launcher bleibt zusätzlich vorhanden. Der neue Paketordner wird `Builds/WindowsUnderwaterFinalCandidate`; bisherige Paketordner werden nicht ersetzt.
- `SmokeTest.ps1 -ProductionWater` ist ausschliesslich eine Erwartungsprüfung des Testwerkzeugs: Es aktiviert keinen Renderzweig per Kommandozeile. Es verlangt die normalen Laufzeitmarker, Seegrenzen und passende Hardware-/Software-Auswahl.

Die Vorbereitung mit `PrepareReflectionTransport.py --production --unified --profile vsm --project divingsim` wurde ausgeführt. Alle 96 vorbereiteten Shaderdateien stimmen bytegenau mit dem ausgewählten VSM-Versuch überein. Ungültige Produktionsanforderungen (fehlende vereinigte Führung oder Diagnoseprofil) werden abgewiesen. Die geänderten PowerShell-Skripte bestehen die Syntaxprüfung. **Das ersetzt weder C++-Nachbau noch Cook oder Paketstart.**

## Noch auszuführen

Nach Ende des laufenden Foundation-Builds, nacheinander:

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
