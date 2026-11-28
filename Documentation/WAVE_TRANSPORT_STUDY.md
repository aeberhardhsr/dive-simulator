# Gemeinsame Wellen für Oberflächen- und Volumenlicht

**Status: erster Tageslauf und zehn paarweise Vergleichsläufe mit separat gebautem Renderer bestanden; 40 Vergleichsbilder gesichtet. Gesamtfreigabe steht noch aus. Nicht für die spielbare Vorschau aktiviert.** Der ursprüngliche Lauf am 27. November 2026 scheiterte an einer fehlenden Renderer-Parameterbindung. Nach dem vollständigen Engine-Build endete der neue Lauf unter `artifacts/caustics-day-validation-20261127-172402` erfolgreich mit vier frischen Aufnahmen und ohne Shader-Bindungsfehler. Alle vier Bilder wurden betrachtet: Lichtmuster erscheinen auf Boden, Kugel und senkrechter Fläche und verschwinden in der Aufnahme mit Stärke null. Sie ändern sich zwischen den beiden simulierten Zeitpunkten. Das beweist noch keine physikalisch richtige Gesamtbeleuchtung, Leistung oder Qualität der Spielwelt. Die vorherigen [Volumenlicht-Ergebnisse](SOLAR_VOLUME_STUDY.md) gelten nur für die dort geprüfte Variante.

## Ziel und vorbereiteter Umfang

Die bisherigen skalaren Kaustiken werden auf eine repräsentative Tiefe projiziert. Der neue Versuch soll stattdessen Wellenhöhe, -neigung und -krümmung am tatsächlichen Lichteintritt auswerten. Oberflächen und Volumenstreupunkte sollen dieselben vier Wellen, denselben Simulationszeitpunkt, Windzustand, Sichtweitenkoeffizienten und dieselbe Kaustikstärke erhalten.

`DiveWaterScene` übergibt diese Werte pro Szene an den Render-Thread und ergänzt den erweiterbaren Unreal-Szenenpuffer. `WaveSolarMedium.ush` verwendet die vorhandene gemeinsame Wellenoptik für eine Näherung des gebrochenen Strahlwegs, der Fokussierung und der Absorption. `WaveSolarSurface.ush` nutzt das Ergebnis für direktes Sonnenlicht; die vorbereitete Volumenvariante nutzt es an jedem Streupunkt und berücksichtigt die gebrochene Richtung in der Phasenfunktion. Die alte Sonnen-Light-Function wird ausschliesslich in diesem Versuch deaktiviert, um doppelte skalare Abschwächung zu vermeiden.

Diese Implementierung ist noch keine physikalische oder visuelle Abnahme. Vollständige Lichtwege über Seegrenzen, energieerhaltende Mehrwege-Fokussierung, angepasste Schattenstrahlen und indirekte Lichtwege fehlen. Auch räumliche Lichtbündel und zeitliche Stabilität sind erst anhand tatsächlicher Bilder zu beurteilen.

Für die Seegrenzen ist inzwischen ein [geografisches Distanzraster](LAKE_FOOTPRINT.md) mit den drei ausgesparten Inseln vorbereitet. Seine Offline-Prüfung und eine nachfolgende optionale GPU-Anbindung sind bestanden. 42.793 GPU-Abfragen und ein freigelegter Ufer-Prüfkörper sind im [separaten Bericht](LAKE_FOOTPRINT_GPU_RESULTS.json) erfasst. Die unten dokumentierte ursprüngliche Zehn-Fälle-Reihe verwendete noch keine Seemaske. Die anschliessende [Begrenzung der sichtbaren Wasseroberfläche](LAKE_SURFACE.md) hat einen eigenen Prüfstand; vollständige Lichtwege an Ufern bleiben offen.

## Tatsächlicher Fehler und Gegenmassnahme

Der erste Lauf registrierte den Szenenpuffer erfolgreich. Beim Kompilieren von `FDeferredLightPS`, Permutation 12, meldete Unreal danach jedoch einen nicht gebundenen Parameter `Scene` in `ShaderParameterStruct.cpp`. Die C++-Parameterstruktur des vorhandenen Lichtdurchlaufs enthält diesen Puffer nicht. Dass die Projektmodule kompilieren oder der Puffer registriert wurde, beweist deshalb noch keine gültige Shader-Anbindung. Der Hauptdurchlauf für volumetrische Lichtstreuung benötigt dieselbe Ergänzung.

Der Testaufruf erreichte zunächst sein Zeitlimit; die noch laufende Engine wurde weiter beobachtet und nach Feststellung dieses Bindungsfehlers gezielt beendet. Es wurde kein zweiter Grafiklauf parallel gestartet. Protokoll: `artifacts/caustics-day-validation-20261127-123212/Runtime.log`. Der Fehler wird ausdrücklich nicht als erwartete Engine-Startdiagnose ignoriert.

Der Versuch verlangt nun beim Modulstart die kompilierte Renderer-Fähigkeit `r.DiveWater.SceneBindingVersion=1`. Ohne diese bricht er vor Registrierung des erweiterten Shader-Layouts mit `DIVE_WAVE_RENDERER_UNAVAILABLE` ab. Der Wert wird vom vorbereiteten Engine-Patch als schreibgeschützte Variable bereitgestellt; er ist kein Schalter zum Umgehen der Prüfung. Die beiden Testwerkzeuge behandeln zusätzlich Meldungen über nicht gebundene Parameter als Fehler.

## Separat vorbereitete Renderer-Änderung

`Tools/PrepareWaveRendererPatch.py` liest die geprüften Originalfassungen von `LightRendering.cpp` und `VolumetricFog.cpp` aus Unreal 5.8.4. Es erzeugt ausschliesslich unter dem ignorierten Ordner `artifacts/wave-renderer-patch`:

- die Ergänzung des Szenenpuffers in beiden Shader-Parameterstrukturen;
- dessen Zuweisung in beiden Helfern für Deferred-Licht und im Volumen-Lichtdurchlauf;
- eine kompilierte Fähigkeitskennung, einen lesbaren Diff und ein Prüfsummenmanifest.

Installierte Engine-Dateien werden nicht verändert. Die vollständigen modifizierten Epic-Dateien werden nicht in dieses Projekt-Repository übernommen. Beide geänderten C++-Dateien, die Renderer-DLL, der vollständige Editor und die aktuellen Projektmodule sind inzwischen erfolgreich gebaut; der [Build-Nachweis](SOURCE_ENGINE_RESULTS.json) bindet die Ergebnisse. Ein anderer Engine-Quellstand wird vor der Ausgabe zurückgewiesen und muss separat geprüft werden.

Die [gebundenen Prüfergebnisse](WAVE_TRANSPORT_RESULTS.json) dokumentieren den erfolgreichen Projekt-Editor-Build, den normalen Tageslauf mit vier Aufnahmen und den erwarteten sofortigen Abbruch des Wellenversuchs auf der unveränderten Engine. Die Patch-Erzeugung ist deterministisch; `git apply --check` gegen die Originale besteht. Ein veränderter Quellstand und eine Ausgabe innerhalb des Engine-Verzeichnisses werden vor dem Schreiben abgewiesen. Diese Prüfungen ersetzen ausdrücklich keinen Build des geänderten Renderers. Die vorhandenen AutomationTest-/ToolsetRegistry-Startdiagnosen sind im normalen Lauf weiterhin enthalten.

```powershell
./Tools/.geodata-env/Scripts/python.exe Tools/PrepareWaveRendererPatch.py
# Nur vorbereiten/prüfen: die Ausgabe nicht in die installierte Engine kopieren.
```

Die native Prüfung verwendet die separat gebaute Engine aus dem vom Nutzer autorisierten Quellcodezugriff: Tag `5.8.4-release`, aufgelöster Commit `f45729c0b6f097038e72b52d366a62baeca3275b`, im ignorierten Ordner `Builds/UE584`. Beide betroffenen Renderer-Dateien stimmten vor der Änderung bytegenau mit den geprüften installierten Originalen überein. Der Patch ist ausschliesslich in dieser Kopie angewendet. Projekt-Binaries für diese Engine liegen getrennt in `Builds/SourceEditorSandbox/divingsim`; die Build-/Testwerkzeuge unterstützen dafür `-SandboxName SourceEditorSandbox` zusammen mit `-EditorSandbox`. Epics [Anleitung zum Quellcodezugriff](https://dev.epicgames.com/documentation/unreal-engine/downloading-source-code-in-unreal-engine) bleibt die Referenz für eine erneute Einrichtung.

## Noch erforderliche Prüfung

Einrichtung, Versionsbindung und aktueller Build-Stand der separaten Engine stehen in [SOURCE_ENGINE.md](SOURCE_ENGINE.md).

1. Erledigt: Patch im passenden separaten Engine-Quellbaum geprüft, Renderer/Editor gebaut und aktuelle Projektmodule neu kompiliert.
2. Erledigt: einzelner Tageslauf mit gültiger Szenenbindung, vier geprüften Bildern und ohne neue Shader-/Parameterfehler. Registrierungs- und Upload-Marker allein wurden nicht als Nachweis verwendet.
3. Erledigt für die feststehenden Prüfkörper: identische Kameraposen mit ruhigem und bewegtem Wasser, zwei Zeitpunkte und eingefrorene Wiederholung. Bewegte Habitatansichten und längere Zeitreihen bleiben offen.
4. Nacht ohne Lampe, Überwasserflächen und tiefe Prüfkörper sind geprüft. Tauchlampen und natürliche Land-Wasser-Ansichten bleiben offen. Eine optionale horizontale Empfängermaske wurde nachfolgend nativ geprüft; fehlende Begrenzung der sichtbaren Wassergeometrie und vollständiger Uferlichtwege verhindert weiterhin eine Standardfreigabe.
5. Erst nach Bildprüfung, Pakettest und Leistungsmessung über eine Übernahme entscheiden.

### Abgeschlossener paarweiser Receiver-Vergleich

`Tools/TestWaveTransport.ps1` bereitet nach bestandenem einzelnen Tageslauf zehn sequenzielle Aufnahmenläufe vor: je Projektions- und Wellenvariante für `day`, `calm`, `deep`, `air` und `night`. Beide Varianten verwenden dieselbe separate Engine, dieselben Projektmodule und dieselbe Kamerafixierung. Vier Phasen pro Lauf ergeben insgesamt 40 Bilder: Stärke ein/aus zum gleichen Zeitpunkt, ein weiterer Zeitpunkt und dessen Wiederholung als Kontrolle für zeitliches Renderrauschen.

Vor dem Start werden laufende Unreal-Builds, Editor-/Spielprozesse und Shader-Worker zurückgewiesen. Engine-Renderer, Projektmodule, relevante Quellen und vorbereitete Shader werden mit Prüfsummen erfasst; nach den Läufen werden unveränderte Eingaben verlangt. Die Fälle bleiben ausdrücklich `visual_status=NOT_REVIEWED`, auch wenn alle Laufzeitmarker vorliegen. Habitatansichten, Lampen, Seegrenzen und Performance sind zusätzliche Prüfungen.

```powershell
# Erst nach erfolgreichem Engine-Build und geprüftem einzelnen Wellen-Tageslauf:
./Tools/.geodata-env/Scripts/python.exe Tools/PrepareSolarShaderStudy.py `
    --engine Builds/UE584 --project Builds/SourceEditorSandbox/divingsim --volume
./Tools/TestWaveTransport.ps1
```

Die Vergleichsreihe unter `artifacts/wave-transport-source-20261127` endete am 27. November erfolgreich: zehn Läufe, 40 frische Bilder mit 1600 × 900 Pixeln. Der vollständige Integritätsbericht wurde vor nachfolgenden Änderungen am Kameratest erzeugt und geprüft. Er bindet den damaligen Quellstand `a34822f381659d1e12cb03afb1f66db3f0e89715`, die tatsächlichen DLLs, Shader und Bilder. Spätere Projekt-Nachbauten ändern diesen historischen Nachweis nicht; eine erneute Prüfung gegen inzwischen geänderte Eingabedateien wäre kein identischer Prüfstand.

Alle 40 Bilder wurden in zehn beschrifteten Kontaktbögen gesichtet; ausgewählte Tages-, Tiefen- und Überwasserbilder zusätzlich in Originalauflösung. Der automatisch erzeugte Bericht behält `NOT_REVIEWED`; die getrennte Sichtungsnotiz in [WAVE_TRANSPORT_RESULTS.json](WAVE_TRANSPORT_RESULTS.json) beschreibt den begrenzten Prüfumfang. Die bekannte AutomationTest-Startdiagnose und die ToolsetRegistry-/PythonTestRunner-Diagnose bleiben in den Protokollen erhalten. Es traten keine neuen Shader-Bindungs- oder Kompilierfehler auf.

Mittlere absolute RGB8-Differenz im beleuchteten Bodenbereich, jeweils Wellenvariante:

| Fall | Stärke ein/aus | Uhr 20/22 s | Eingefrorene Wiederholung |
| --- | ---: | ---: | ---: |
| Tag | 4,140 | 5,903 | 0,388 |
| Ruhiges Wasser | 0,360 | 0,363 | 0,362 |
| Tiefe | 1,157 | 1,623 | 0,460 |
| Über Wasser | 0,330 | 0,331 | 0,332 |
| Nacht ohne Lampe | 0,000 | 0,000 | 0,000 |

Im Tagesfall bewegen sich die Muster auf Boden, Kugel und senkrechter Fläche; in der Tiefe sind sie schwächer. Ruhiges Wasser und Überwasserflächen ändern sich ungefähr auf dem Niveau der eingefrorenen Wiederholung. Die Nachtbilder zeigen kein künstliches Aufleuchten, sind aber weitgehend schwarz und belegen weder Lampenqualität noch Nachtsichtbarkeit. Die dunklen unregelmässigen Flächen des Tiefenfalls sind in beiden Varianten vorhandene Schatten der Weltobjekte über dem abgesenkten Prüfkörper. Die Projektions-/Wellenunterschiede betreffen auch Farbabsorption und Helligkeit; die regelmässige Musterstruktur bleibt sichtbar. Diese LDR-Pixelwerte sind keine radiometrische Kalibrierung. Lange Flackerprüfungen, gebrochene Schatten, die eigentliche Habitatqualität und die Zielgrafik aus den Nutzervorlagen sind damit nicht abgenommen.

`Tools/ReportWaveTransport.py --study <Vergleichsordner>` prüft nach diesen Läufen die vollständigen zehn Fälle, Eingabe-/Engine-Prüfsummen, Variantenmarker, Bildgrössen und identischen Kameraposen. Es erfasst Farbunterschiede für Stärke ein/aus, veränderte Zeit und eingefrorene Wiederholung sowie zwischen beiden Beleuchtungsvarianten. Die Mittelwerte werden mit 64-Bit-Summierung berechnet; reine 32-Bit-Summierung verfälschte im Test mit konstanten Pixelwerten einen Kanal um über drei RGB8-Stufen. Alle Warnungen und Fehlerzeilen bleiben im Bericht sichtbar. Das Ergebnis heisst bewusst `CAPTURE_INTEGRITY_CHECKED` mit `visual_status=NOT_REVIEWED`, nicht Grafikabnahme. Lokale Voraussetzungen für die Bildauswertung sind NumPy und Pillow.

Fünf portable Werkzeugtests prüfen ungültige Phasen, Uhrzeit, Wind, Stärke, Kamerabewegung, falsche Varianten, Fehler trotz Erfolgsmarker und vorzeichenrichtige Pixelmittelwerte. Diese bestehen; sie verwenden synthetische Kontrollwerte und ergänzen die jetzt vorliegenden nativen Wellenaufnahmen.

Die normale Vorschau bleibt `Builds/WindowsPlantColor/Start-WaterPreview.cmd`. Unterwasserwelt und Überwasserlandschaft bleiben die Grafikprioritäten. Die [Menü-/HUD-Arbeit](UI_UX_TARGET.md) wurde auf ausdrücklichen Nutzerwunsch während des Engine-Builds vorgezogen; das ersetzt die offenen Grafik- und FPV-Prüfungen nicht.
