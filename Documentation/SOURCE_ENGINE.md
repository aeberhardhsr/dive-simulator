# Separater Unreal-Quellcode-Build für die Wasserbeleuchtung

Der Zugriff auf `EpicGames/UnrealEngine` funktioniert nach der vom Nutzer abgeschlossenen Kontoverknüpfung. Die Engine liegt ausschliesslich im ignorierten Arbeitsordner `Builds/UE584`. Sie wird nicht in das öffentliche Spiel-Repository übernommen. Die installierte Engine unter `C:/Program Files/Epic Games/UE_5.8` bleibt separat.

## Verifizierter Ausgangspunkt

- Offizieller Tag: `5.8.4-release`.
- Aufgelöster Commit: `f45729c0b6f097038e72b52d366a62baeca3275b`.
- `LightRendering.cpp` und `VolumetricFog.cpp` stimmen vor dem Patch bytegenau mit den geprüften installierten 5.8.4-Dateien überein.
- Die beiden kopierten Engine-Shader werden von `PrepareSolarShaderStudy.py` ebenfalls gegen ihre festgelegten Prüfsummen geprüft.
- GitDependencies hat die benötigten Abhängigkeiten erfolgreich geladen: 128.576 Dateien, 23.192,1 MiB Download laut Abschlussprotokoll.
- Build-Toolchain: MSVC 14.44.35230 und Windows SDK 10.0.22621.0. UnrealBuildTool wurde aus diesem Quellstand erfolgreich erzeugt.
- `ShaderCompileWorker` wurde erfolgreich gebaut: 289 Aktionen, 477,96 Sekunden Gesamtlaufzeit. Die EXE liegt im separaten Engine-Baum.

**Status: Shader-Compiler fertig; Editor-/Renderer-Build läuft. Noch kein gültiger Renderlauf mit der geänderten Engine.** Die bestehende Vorschau und die [fehlgeschlagene erste Wellenprobe](WAVE_TRANSPORT_STUDY.md) werden dadurch nicht rückwirkend geändert. Die Protokolle liegen unter `artifacts/source-engine-20261127/`; der [Zwischenbericht](SOURCE_ENGINE_RESULTS.json) bindet den abgeschlossenen Compiler-Build und die Eingangsdaten.

Inzwischen sind beide geänderten Übersetzungseinheiten `LightRendering.cpp` und `VolumetricFog.cpp` erfolgreich kompiliert, und `UnrealEditor-Renderer.lib` ist erzeugt. Die zwei Objektdateien, die Importbibliothek und ein unveränderlicher Ausschnitt des bisherigen Build-Protokolls sind im Zwischenbericht mit Prüfsummen erfasst. Zu diesem Prüfzeitpunkt existiert die Renderer-DLL noch nicht; der vollständige Editor-Build läuft weiter. Dies belegt die C++-Kompilierbarkeit der Änderung, noch nicht ihre Shader-Bindung oder Bildwirkung.

Bei der anschliessenden Quellprüfung wurde der Aufruf in `FSceneRenderer::ComputeVolumetricFog` auf die dort vorhandene Memberfunktion `GetSceneUniformBufferRef(GraphBuilder)` korrigiert. Die ursprüngliche Variante mit zwei Argumenten wäre durch diese Memberfunktion verdeckt und damit nicht auflösbar. Die Korrektur ist vor dem Kompilieren dieser Datei in den laufenden Quellbaum übernommen worden; der Build wurde nicht neu gestartet. Die aktuelle Vorbereitung unter `renderer-patch-v2` stimmt bytegenau mit den zwei geänderten Quelldateien überein. Der frühere Patch bleibt als unkompilierter Zwischenstand erhalten.

## Getrennte Projekt-Binaries

`-EditorSandbox -SandboxName SourceEditorSandbox` richtet `Builds/SourceEditorSandbox/divingsim` ein. Source, Config und Shaders werden wie in der bestehenden Sandbox kopiert; Content verweist über eine geprüfte Junction auf die vorhandenen Spielinhalte. Binaries, Intermediate und Saved bleiben getrennt von `Builds/EditorSandbox/divingsim`. Ein Sandbox-Name darf keine Pfadtrenner oder Punktsegmente enthalten. Ohne den neuen Parameter bleibt das bisherige Verhalten erhalten.

`TestCaustics.ps1` kann mit `-TimeoutSeconds` längere erste Shader-Kompilierungen beobachten. Ein Zeitlimit beendet den Prozess weiterhin nicht automatisch. Vor jedem erneuten Start ist der konkrete laufende Prozess zu prüfen. Im Testmanifest wird zusätzlich die Prüfsumme der tatsächlich ausgewählten Engine-Renderer-DLL erfasst; die EXE allein identifiziert diese Änderung nicht.

## Einrichtung und Build

Die folgenden Befehle gelten für dieses Windows-Arbeitsverzeichnis. Quellcodezugriff muss vorher bestehen. Der Clone-Schritt ist nur bei noch nicht vorhandenem Ziel auszuführen.

```powershell
git -c core.longpaths=true clone --depth 1 --single-branch --branch 5.8.4-release https://github.com/EpicGames/UnrealEngine.git Builds/UE584

$sourceEngineRoot = Join-Path $PWD 'Builds/UE584'
& "$sourceEngineRoot/Engine/Binaries/DotNET/GitDependencies/win-x64/GitDependencies.exe" `
    "--root=$sourceEngineRoot" --threads=8 `
    --exclude=Mac --exclude=Linux --exclude=LinuxArm64 --exclude=Android --exclude=IOS --exclude=TVOS

./Tools/.geodata-env/Scripts/python.exe Tools/PrepareWaveRendererPatch.py `
    --engine Builds/UE584 --output artifacts/source-engine-20261127/renderer-patch
# Nur im separaten, noch unveränderten Quellbaum anwenden:
git -C Builds/UE584 apply --check "$PWD/artifacts/source-engine-20261127/renderer-patch/renderer-scene-binding.patch"
git -C Builds/UE584 apply "$PWD/artifacts/source-engine-20261127/renderer-patch/renderer-scene-binding.patch"

& "$sourceEngineRoot/Engine/Build/BatchFiles/Build.bat" ShaderCompileWorker Win64 Development `
    -WaitMutex -NoHotReloadFromIDE -NoUBA -MaxParallelActions=2
./Tools/Build.ps1 -EngineRoot $sourceEngineRoot -EditorSandbox -SandboxName SourceEditorSandbox -Target Editor -MaxParallelActions 4
```

Die vorhandenen VC++-/GameInput-Laufzeitkomponenten bleiben erhalten. Es werden keine Dateizuordnungen auf die neue Engine umgestellt. Für diesen Windows-Prüfstand ausgelassene Plattformabhängigkeiten sind vor einem entsprechenden anderen Plattform-Build nachzuladen; dies ist kein macOS-Buildnachweis.

Auf Wunsch des Nutzers wurde der Erstbuild am 27. November nach 2.014 von 4.043 abgeschlossenen Aktionen kontrolliert angehalten und mit `-MaxParallelActions=4` statt `2` inkrementell fortgesetzt. Das neue Protokoll `artifacts/source-engine-20261127/editor-build-parallel4.log` bestätigt 2.029 verbleibende Aktionen; die abgeschlossenen Ergebnisse wurden übernommen. Drei gleichzeitig aktive Compilerprozesse wurden beobachtet. Der Rechner besitzt 16 physische Kerne und 32 logische Prozessoren bei 31,08 GiB nutzbarem RAM. Die ersten Messungen nach Wiederanlauf zeigten rund 8–9 GiB freien RAM; das ist eine Momentaufnahme, keine Speicherzusage für alle Module. Kompilierung, Linken und Abhängigkeiten führen zu wechselnder Parallelität. Aus unterschiedlichen Modulabschnitten lässt sich keine belastbare Beschleunigungsrate ableiten.

`Tools/Build.ps1` reicht den neuen Parameter `-MaxParallelActions` sowohl an direkte Editor-/Game-Builds als auch an den BuildCookRun-Kompilierschritt weiter. Ohne Angabe bleibt der bisherige Wert `2` erhalten. Der Parameter ändert keinen bereits laufenden Prozess. Eine weitere Erhöhung erfordert erneut eine Prüfung von physischem und zugesichertem Speicher; die Anzahl freier CPU-Kerne allein genügt nicht. Während des Wiederanlaufs wurden keine Projektquellen synchronisiert, damit der nachfolgend beschriebene Projekt-Nachbau weiterhin getrennt erfolgt.

Während dieses Erstbuilds hat der Nutzer die Menü-/HUD-Arbeit ausdrücklich vorgezogen. Änderungen im Hauptprojekt dürfen die laufende Sandbox-Kompilierung nicht nachträglich verändern. Nach Abschluss dieses Builds ist deshalb zuerst `Tools/Build.ps1 -EngineRoot $sourceEngineRoot -EditorSandbox -SandboxName SourceEditorSandbox -Target Editor -MaxParallelActions 4` erneut auszuführen: Dieser kurze Projekt-Nachbau synchronisiert die neuen UI-Quellen und erzeugt dazu passende Projektmodule. `TestCaustics.ps1` allein synchronisiert zwar Dateien, kompiliert sie aber nicht und darf diesen Schritt nicht ersetzen.

Erst nach erfolgreichen Builds folgen die Shader-Vorbereitung und **ein einzelner** nativer Tageslauf:

```powershell
./Tools/.geodata-env/Scripts/python.exe Tools/PrepareSolarShaderStudy.py `
    --engine Builds/UE584 --project Builds/SourceEditorSandbox/divingsim --volume --waves
./Tools/TestCaustics.ps1 -EngineRoot $sourceEngineRoot -EditorSandbox -SandboxName SourceEditorSandbox `
    -WaterPreview -SolarShaderStudy -SolarVolumeStudy -WaveTransportStudy -Case day -TimeoutSeconds 1200
```

Weitere Bildvergleiche, Seegrenzen, Laufzeitstabilität und Performance bleiben Teil der [Wellenlicht-Prüfung](WAVE_TRANSPORT_STUDY.md). Erfolgreiches Kompilieren allein ist keine Grafikabnahme.
