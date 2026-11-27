# Wasseroberfläche entlang der Seegrenze

**Status: optionale Integration gebaut und nativ geprüft; sieben Vergleichsläufe mit 28 gesichteten Bildern bestanden.** Noch nicht als Standard aktiviert oder als neues Spielpaket ausgeliefert.

## Änderung

Die sichtbare Wasseroberfläche verwendet dieselbe geprüfte R16F-Distanztextur und dieselbe Koordinatenabbildung wie die [geografische Begrenzung von Wassermedium und Licht](LAKE_FOOTPRINT.md). Die prozedurale Geometrie bleibt bestehen; ihre Pixel werden ausserhalb der Nullkontur verworfen beziehungsweise transparent. Damit folgt auch die von unten sichtbare Grenzfläche dem vorhandenen Seeumriss und seinen Inseln.

Die Oberseite verwendet Single Layer Water mit `Masked` und einer binären Deckung `distance >= 0`. Diese Kombination wird von [Unreals Single-Layer-Water-Modell](https://dev.epicgames.com/documentation/unreal-engine/single-layer-water-shading-model-in-unreal-engine) unterstützt. Die Unterseite bleibt transparent und multipliziert ihre vorhandene Fresnel-Deckung mit derselben geografischen Maske. Ihre Custom-Depth-Schwelle beträgt 0,0001: Bei Deckung null soll sie keine unsichtbare Tiefengrenze hinterlassen; die Wasser-/Luft-Fresnel-Deckung liegt oberhalb dieser Schwelle. Wellenverschiebung, Normalen und Fresnel-Funktion bleiben unverändert.

`Tools/build_lake_surface_mask.py` erzeugt den gemeinsamen Materialknoten. Der Laufzeitcode bindet die bereits vorhandene Textur an beide Materialinstanzen; es entsteht keine zusätzliche Rasterkopie. Materialien ohne den neuen Aktivierungsparameter werden beim Uferexperiment ausdrücklich zurückgewiesen. `Build.ps1 -Target Content -OnlyWater` baut Oberseite, Unterseite und Volumenmaterial gemeinsam neu.

Die Aktivierung erfolgt weiterhin nur über `-DiveLakeFootprint` innerhalb des expliziten Wellenlicht-Versuchs. `-DiveLakeSurfaceOff` schaltet ausschliesslich die geografische Begrenzung der sichtbaren Grenzfläche aus. Die geografische Licht- und Volumenbegrenzung bleibt dabei aktiv, damit der Bildvergleich die Oberflächenwirkung isoliert. Ohne Uferexperiment ist der neue Materialparameter null; die Deckung bleibt vollständig.

## Prüfanordnung

`TestCaustics.ps1 -Case shore-top` und `-Case shore-under` verwenden dieselbe vermessene Uferposition wie der bestehende Lichttest: E2726254,938 / N1221730,728. Das Gelände wird nur für die Diagnose ausgeblendet. Der Prüfkörper, das Wassermedium und die sichtbare Wasserfläche bleiben erhalten. Die Kameras liegen bei Unreal X272842,8 / Y25493,8 cm, oben bei Z350 cm mit −65° Neigung, unten bei Z−350 cm mit +65° Neigung. Diese Ansichten sind freigelegte Prüfkörper, keine natürliche Uferlandschaft und kein spielbarer Einstieg.

Je Ansicht werden vier Bilder erzeugt: Uhr 20 s mit Lichtfokussierung, dieselbe Uhr ohne Fokussierung, Uhr 22 s mit Fokussierung und eine eingefrorene Wiederholung. Beide Oberflächenseiten werden mit und ohne Flächenbegrenzung verglichen. Zusätzlich aktiviert `-LakeSurfaceDepthDebug` in `shore-under` die vorhandene native Custom-Depth-Diagnose: Rot kennzeichnet eine vor dem Hintergrund liegende Wassergrenze, Blau deren Abwesenheit. Dies prüft auch den transparenten Tiefenpass und nicht nur das sichtbare Farbbild.

Nach dem Projekt- und Materialbau sowie der in der [Wellenstudie](WAVE_TRANSPORT_STUDY.md) beschriebenen Shader-Vorbereitung:

```powershell
$study=@{
    EngineRoot='O:/games/dive game/Builds/UE584'
    EditorSandbox=$true; SandboxName='SourceEditorSandbox'; WaterPreview=$true
    SolarShaderStudy=$true; SolarVolumeStudy=$true; WaveTransportStudy=$true
    LakeFootprint=$true; TimeoutSeconds=600
}
./Tools/TestCaustics.ps1 @study -Case shore-top
./Tools/TestCaustics.ps1 @study -Case shore-top -LakeSurfaceOff
./Tools/TestCaustics.ps1 @study -Case shore-under
./Tools/TestCaustics.ps1 @study -Case shore-under -LakeSurfaceOff
./Tools/TestCaustics.ps1 @study -Case shore-under -LakeSurfaceDepthDebug
./Tools/TestCaustics.ps1 @study -Case shore-under -LakeSurfaceDepthDebug -LakeSurfaceOff
./Tools/TestCaustics.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -WaterPreview -Case shore-top -TimeoutSeconds 600
```

Die Befehle starten jeweils einen eigenen nativen Prozess und erzeugen frische Bilder. Die letzte Zeile prüft die Wasseroberseite im normalen Vorschaupfad ohne Wellenlicht- oder Uferexperiment. Ein anderes Engine-Verzeichnis muss als absoluter Pfad eingesetzt werden.

## Ergebnis

Der [Ergebnisbericht](LAKE_SURFACE_RESULTS.json) bindet beide verwendeten Projektstände, DLLs, Materialien, Raster, Protokolle und alle 28 Bilder mit Prüfsummen. Der Projektbau und die Materialerzeugung sind erfolgreich. Die bekannten AutomationTest-, EditorDataStorageUI- und ToolsetRegistry-Startdiagnosen bleiben im Bericht erhalten; neue Materialkompilierungs- oder Shaderbindungsfehler traten nicht auf. Alle sieben Kontaktbögen wurden gesichtet, ausgewählte Oberseiten-, Unterseiten- und Tiefenbilder zusätzlich in Originalauflösung.

Die [begrenzte Oberseite](Images/LakeSurface_Top_Masked.png) endet an der Uferkontur; im [Kontrolllauf ohne Flächenbegrenzung](Images/LakeSurface_Top_Unmasked.png) überdeckt das Wasser auch die Landseite. In beiden Läufen sind Licht und Volumen geografisch begrenzt. Ebenso endet die [transparente Unterseite](Images/LakeSurface_Under_Masked.png) an der Kontur. Diese Unteransicht ist kontrastarm und keine Abnahme der Unterwasser-Bildqualität.

In der [Tiefendiagnose](Images/LakeSurface_Depth_Masked.png) bleibt die Landseite blau. Die vor dieser Tiefenprüfung ausgewählten inneren Bildregionen liefern in allen vier Phasen:

| Region | Wasser-Tiefenfläche mit Begrenzung | Ohne Begrenzung |
| --- | ---: | ---: |
| Landseite, 200 × 100 Pixel | 0 % | 100 % |
| Seeseite, 200 × 200 Pixel | 100 % | 100 % |

Die landseitige Farbdifferenz zwischen begrenzter und unbegrenzter Variante beträgt im ausgewählten Ausschnitt durchschnittlich 96,13 RGB8-Stufen von oben und 18,02 von unten. Seeseitig sind es 0,31 beziehungsweise 0,22 Stufen. Diese beschreibenden Bildwerte sind keine radiometrische Kalibrierung und keine Aussage über jeden Uferpixel. Die genaue Lage der Ausschnitte steht im Bericht.

Zwei frühe Unteransichten bei Z−180 cm wurden als Ufernachweis verworfen: Ihr Bildausschnitt enthielt keine Landseite. Die Unterprüfung wurde bei Z−350 cm wiederholt und der Diagnoseboden auf Z−600 cm abgesenkt, damit er den Blick nicht verdeckt. Die Oberseitenaufnahmen stammen vom zuvor kompilierten Stand; der Bericht hält beide Eingabestände getrennt fest. Die Änderung betrifft dort nur den Unterseiten-Prüfaufbau.

Die abschliessende normale Vorschau verwendet weiterhin eine vollständig gedeckte Oberfläche und die bisherigen projizierten Kaustiken. Die neue Maske wurde dadurch nicht versehentlich im Standardpfad aktiviert.

## Grenzen

Die gemeinsame Geodatenabfrage wurde zuvor an 42.793 GPU-Positionen geprüft. Diese Änderung ersetzt die historische Prüfung nicht und behauptet keine neue Vermessungsgenauigkeit. Der 1-m-Rasterumriss hat die bereits dokumentierte Unsicherheitszone von ±0,75 m; die sichtbare Oberfläche schneidet an dessen Nullkontur, während das Medium innerhalb der Unsicherheitszone überblendet.

Die Maske bildet einen festen Seepegel ab. Wellenauflauf, Ufererosion, Überflutung und variable Pegel sind nicht enthalten. Seitlich über das Ufer verlaufende Lichtwege werden weiterhin nicht vollständig in Wasser-/Luftabschnitte zerlegt. Geometrieaufwand, Texturspeicher, Leistung und Paketprüfung bleiben offen. Ebenso fehlen natürliche Vergleichsansichten an mehreren Ufern und Inseln sowie die Abnahme der vom Nutzer vorgegebenen Landschaftsqualität. Der vorhandene spielbare Launcher enthält diese experimentelle Änderung noch nicht.
