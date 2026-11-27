# Gemeinsame Wellen für Oberflächen- und Volumenlicht

**Status: vorbereiteter Versuch, nicht visuell validiert und nicht für die spielbare Vorschau aktiviert.** Der erste native Lauf am 27. November 2026 ist an einer fehlenden Renderer-Parameterbindung gescheitert. Es gibt aus diesem Versuch keine gültigen Spielaufnahmen und keinen Leistungsnachweis. Die vorherigen [Volumenlicht-Ergebnisse](SOLAR_VOLUME_STUDY.md) gelten nur für die dort geprüfte Variante.

## Ziel und vorbereiteter Umfang

Die bisherigen skalaren Kaustiken werden auf eine repräsentative Tiefe projiziert. Der neue Versuch soll stattdessen Wellenhöhe, -neigung und -krümmung am tatsächlichen Lichteintritt auswerten. Oberflächen und Volumenstreupunkte sollen dieselben vier Wellen, denselben Simulationszeitpunkt, Windzustand, Sichtweitenkoeffizienten und dieselbe Kaustikstärke erhalten.

`DiveWaterScene` übergibt diese Werte pro Szene an den Render-Thread und ergänzt den erweiterbaren Unreal-Szenenpuffer. `WaveSolarMedium.ush` verwendet die vorhandene gemeinsame Wellenoptik für eine Näherung des gebrochenen Strahlwegs, der Fokussierung und der Absorption. `WaveSolarSurface.ush` nutzt das Ergebnis für direktes Sonnenlicht; die vorbereitete Volumenvariante nutzt es an jedem Streupunkt und berücksichtigt die gebrochene Richtung in der Phasenfunktion. Die alte Sonnen-Light-Function wird ausschliesslich in diesem Versuch deaktiviert, um doppelte skalare Abschwächung zu vermeiden.

Diese Implementierung ist noch keine physikalische oder visuelle Abnahme. Seegrenzen, energieerhaltende Mehrwege-Fokussierung, angepasste Schattenstrahlen und indirekte Lichtwege fehlen. Auch räumliche Lichtbündel und zeitliche Stabilität sind erst anhand tatsächlicher Bilder zu beurteilen.

## Tatsächlicher Fehler und Gegenmassnahme

Der erste Lauf registrierte den Szenenpuffer erfolgreich. Beim Kompilieren von `FDeferredLightPS`, Permutation 12, meldete Unreal danach jedoch einen nicht gebundenen Parameter `Scene` in `ShaderParameterStruct.cpp`. Die C++-Parameterstruktur des vorhandenen Lichtdurchlaufs enthält diesen Puffer nicht. Dass die Projektmodule kompilieren oder der Puffer registriert wurde, beweist deshalb noch keine gültige Shader-Anbindung. Der Hauptdurchlauf für volumetrische Lichtstreuung benötigt dieselbe Ergänzung.

Der Testaufruf erreichte zunächst sein Zeitlimit; die noch laufende Engine wurde weiter beobachtet und nach Feststellung dieses Bindungsfehlers gezielt beendet. Es wurde kein zweiter Grafiklauf parallel gestartet. Protokoll: `artifacts/caustics-day-validation-20261127-123212/Runtime.log`. Der Fehler wird ausdrücklich nicht als erwartete Engine-Startdiagnose ignoriert.

Der Versuch verlangt nun beim Modulstart die kompilierte Renderer-Fähigkeit `r.DiveWater.SceneBindingVersion=1`. Ohne diese bricht er vor Registrierung des erweiterten Shader-Layouts mit `DIVE_WAVE_RENDERER_UNAVAILABLE` ab. Der Wert wird vom vorbereiteten Engine-Patch als schreibgeschützte Variable bereitgestellt; er ist kein Schalter zum Umgehen der Prüfung. Die beiden Testwerkzeuge behandeln zusätzlich Meldungen über nicht gebundene Parameter als Fehler.

## Separat vorbereitete Renderer-Änderung

`Tools/PrepareWaveRendererPatch.py` liest die geprüften Originalfassungen von `LightRendering.cpp` und `VolumetricFog.cpp` aus Unreal 5.8.4. Es erzeugt ausschliesslich unter dem ignorierten Ordner `artifacts/wave-renderer-patch`:

- die Ergänzung des Szenenpuffers in beiden Shader-Parameterstrukturen;
- dessen Zuweisung in beiden Helfern für Deferred-Licht und im Volumen-Lichtdurchlauf;
- eine kompilierte Fähigkeitskennung, einen lesbaren Diff und ein Prüfsummenmanifest.

Installierte Engine-Dateien werden nicht verändert. Die vollständigen modifizierten Epic-Dateien werden nicht in dieses Projekt-Repository übernommen. Der vorbereitete Patch ist **noch nicht als Renderer kompiliert**. Ein anderer Engine-Quellstand wird vor der Ausgabe zurückgewiesen und muss separat geprüft werden.

Die [gebundenen Prüfergebnisse](WAVE_TRANSPORT_RESULTS.json) dokumentieren den erfolgreichen Projekt-Editor-Build, den normalen Tageslauf mit vier Aufnahmen und den erwarteten sofortigen Abbruch des Wellenversuchs auf der unveränderten Engine. Die Patch-Erzeugung ist deterministisch; `git apply --check` gegen die Originale besteht. Ein veränderter Quellstand und eine Ausgabe innerhalb des Engine-Verzeichnisses werden vor dem Schreiben abgewiesen. Diese Prüfungen ersetzen ausdrücklich keinen Build des geänderten Renderers. Die vorhandenen AutomationTest-/ToolsetRegistry-Startdiagnosen sind im normalen Lauf weiterhin enthalten.

```powershell
./Tools/.geodata-env/Scripts/python.exe Tools/PrepareWaveRendererPatch.py
# Nur vorbereiten/prüfen: die Ausgabe nicht in die installierte Engine kopieren.
```

Für den nächsten nativen Lauf ist eine separate, aus autorisiert bezogenem vollständigem Quellcode gebaute Engine nötig. Der aktuelle lokale Git-Zugang und der GitHub-Connector liefern für `EpicGames/UnrealEngine` keinen Zugriff. Das allein sagt nicht, ob das Benutzerkonto bereits berechtigt ist. Die Zugangsklärung ist angefragt. Epics [Anleitung zum Quellcodezugriff](https://dev.epicgames.com/documentation/unreal-engine/downloading-source-code-in-unreal-engine) beschreibt die Kontoverknüpfung und die Einladung zur Organisation.

## Noch erforderliche Prüfung

1. Den vorbereiteten Patch im passenden separaten Engine-Quellbaum prüfen, Renderer/Editor bauen und Projektmodule gegen diese Engine neu kompilieren.
2. Zunächst einen einzelnen Tageslauf mit gültiger Szenenbindung und ohne neue Shader-/Parameterfehler abschliessen. Registrierungs- und Upload-Marker allein reichen nicht.
3. Identische Kameraposen mit ruhigem und bewegtem Wasser vergleichen; mehrere Zeitpunkte auf räumliche Bewegung, Flackern und überhelle Fokussierung prüfen.
4. Nacht/Tauchlampe, Überwasserflächen, tiefe Bereiche und Land-Wasser-Grenzen prüfen; die noch fehlende horizontale Seemaske bleibt ein Ausschlussgrund für die Standardaktivierung.
5. Erst nach Bildprüfung, Pakettest und Leistungsmessung über eine Übernahme entscheiden.

Die normale Vorschau bleibt `Builds/WindowsPlantColor/Start-WaterPreview.cmd`. Die vereinbarte Reihenfolge bleibt Unterwasserwelt, Überwasserlandschaft, danach [Menü/HUD/FPV](UI_UX_TARGET.md).
