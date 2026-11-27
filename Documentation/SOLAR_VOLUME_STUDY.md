# Sonnenlicht am Streupunkt im Wasser

Die [zusätzliche Absorption des direkten Sonnenlichts](SOLAR_LIGHT_STUDY.md) ist jetzt auch im nativen Volumenrenderer erprobt. Oberflächen und Streupunkte verwenden dieselbe Berechnung für den farbabhängigen Verlust auf dem Weg von der Wasserfläche zum Empfänger. Die Änderung bleibt ein ausdrücklich aktivierter Versuch. Die spielbare Vorschau `Builds/WindowsPlantColor/Start-WaterPreview.cmd` wird dadurch noch nicht ersetzt.

## Bildwirkung

Der zusätzliche Volumenschritt nimmt dem Wasser einen Teil des rötlich-grauen Schleiers. Seine Wirkung ist deutlich kleiner als die zusätzliche [Korrektur des Kamerawegs](SPECTRAL_VIEW.md). Gemeinsam entsteht mehr entfernungsabhängiger Farbverlust, aber noch keine ausreichend differenzierte räumliche Beleuchtung. Freie Bodenflächen bleiben zu gleichförmig; die Oberflächenansicht wirkt weiterhin flach. Die Qualität der [Nutzerreferenzen](VISUAL_TARGET.md) ist nicht erreicht.

Nur Absorption des direkten Oberflächenlichts:

![Kontrolle mit direkter Oberflächenabsorption](Images/SolarVolume_Direct.png)

Zusätzlich Absorption am Streupunkt:

![Zusätzliche Volumenabsorption](Images/SolarVolume_Source.png)

Zusätzlich der Kameraweg:

![Kombination von Oberflächen-, Volumen- und Kameraweg](Images/SolarVolume_Combined.png)

Blick zur Wasseroberfläche mit allen drei Schritten:

![Kombinierter Blick zur Oberfläche](Images/SolarVolume_Surface.png)

Die Bilder sind unveränderte native Spielaufnahmen bei Weesen, 21. Juni, 12 Uhr. Die drei Bodenansichten haben dieselbe Kamera.

## Technischer Umfang

`SolarMedium.ush` enthält die gemeinsame Berechnung mit den unveränderten, gestalterisch definierten RGB-Koeffizienten `(0.12, 0.015, 0.035)` pro Meter. Der neue Eingriff multipliziert ausschliesslich den gerichteten Lichtbeitrag in `VolumetricFog.usf`, bevor dieser in das Streulicht eingeht. Er verwendet die Position jedes ausgewerteten Volumengitter-Samples. Lokale Lichtquellen, Schatten, Phasenfunktion und temporale Integration behalten ihren bisherigen Code.

Die folgenden Teile sind weiterhin offen und verhindern eine Standardaktivierung:

- Die **skalare** Abschwächung und Lichtmuster des bestehenden Volumen-Light-Functions stammen weiterhin aus einer zweidimensionalen Projektion auf eine repräsentative Tiefe nahe der Kamera. Nur der neue RGB-Zusatz verwendet die tatsächliche Streupunktposition. Räumliche Kaustikfokussierung und gebrochene Strahlrichtungen fehlen noch.
- Die neue Absorption verwendet die mittlere Wasserfläche `Z=0`, keine örtliche Wellenhöhe und keine horizontale Seemaske. Tief liegendes Gelände ausserhalb des Sees würde ebenfalls beeinflusst.
- Indirekte Beleuchtung und weitere Wege nach Spiegelung/Brechung sind nicht gemeinsam korrigiert. Die 16-Intervall-Näherung und der Übergang an der mittleren Wasserhöhe des Kamera-Passes bleiben bestehen.
- Die Koeffizienten sind keine Walensee-Messwerte. Es gibt für diese Erweiterung noch keinen Paket- oder Leistungsnachweis.

Die nächste notwendige Renderer-Arbeit ist deshalb eine gemeinsame Übergabe von Wellenzustand, Sichtweite und Wasserbegrenzung an Oberflächen- und Volumenbeleuchtung. Erst damit lässt sich die bisherige Projektion durch eine räumliche Auswertung ersetzen; eine stärkere pauschale Einfärbung behebt diese Ursache nicht.

## Prüfung und Reproduktion

`Tools/TestSolarVolume.ps1` vergleicht jeweils Oberflächenlicht, zusätzliches Volumenlicht und die Kombination mit dem Kameraweg in zwei Blickrichtungen. Hinzu kommen getrennte Vorher-/Nachher-Kontrollen für Flächen über Wasser und eine Nachtszene mit Tauchlampe. Der [Ergebnisbericht](SOLAR_VOLUME_RESULTS.json) enthält Kameraposen, Quellen-/Modulprüfsummen, Protokolle und Bildmessungen. Die LDR-Werte sind nach Tone Mapping gemessen und keine radiometrische Kalibrierung. Vorhandene Engine-/Plugin-Startdiagnosen bleiben in den Protokollen erhalten.

**Nativer Prüfstand vom 27. November: zehn Läufe mit 30 Aufnahmen bestehen.** Kameraposen stimmen innerhalb der jeweiligen Vergleichsreihen überein; die vorab erfassten Eingaben sowie beide installierten Engine-Shader bleiben unverändert.

| Vergleich | Gemessene Wirkung, Skala 0–255 |
| --- | --- |
| Zusätzlicher Volumenschritt, obere Wasserregion beim Bodenblick | RGB-Mittelwerte ändern sich um −6,172 / −0,778 / −1,421. |
| Zusätzlicher Volumenschritt, gleiche Bildregion beim Oberflächenblick | RGB-Mittelwerte ändern sich um −8,126 / −1,281 / −2,701. |
| Neutrale Testfläche über Wasser | Mittlere absolute RGB-Abweichung unter 0,3, sowohl beleuchtet als auch beschattet. |
| Zwei Nachtaufnahmen mit Tauchlampe | Mittlere absolute RGB-Abweichung 0,026 bzw. 0,069 im gesamten Bild. |

Der reine Volumeneffekt ist damit sichtbar nachgewiesen und grösser als die Abweichungen der Kontrollbilder. Die kombinierte Bildreihe bleibt eine gestalterische Untersuchung; das Bestehen der Laufzeit- und Bildkontrollen ist keine Referenzabnahme.

```powershell
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/.geodata-env/Scripts/python.exe Tools/PrepareSolarShaderStudy.py
./Tools/.geodata-env/Scripts/python.exe Tools/PrepareSolarShaderStudy.py --volume
./Tools/TestSolarVolume.ps1
# Den ausgegebenen Ergebnisordner einsetzen:
./Tools/.geodata-env/Scripts/python.exe Tools/ReportSolarVolume.py --study artifacts/solar-volume-YYYYMMDD-HHMMSS
```

Die beiden Varianten liegen getrennt in `Intermediate/DiveSolarStudy` und `Intermediate/DiveSolarVolumeStudy`. Beide Vorbereitungen prüfen die Engine-Originaldateien; die zusätzliche Variante prüft auch die exakte Fassung von `VolumetricFog.usf`. Installierte Engine-Dateien werden nicht verändert oder ins Repository kopiert. Zur manuellen nativen Szenenprüfung ergänzt `-SolarVolumeStudy` den bestehenden `-SolarShaderStudy`-Schalter; `-SpectralView` schaltet zusätzlich den Kameraweg ein. Alle Läufe erfolgen nacheinander in der Editor-Sandbox.

Die vereinbarte Reihenfolge bleibt Unterwasserwelt, Überwasserlandschaft und danach [Menü/HUD/FPV](UI_UX_TARGET.md).

Der folgende [Versuch mit gemeinsamem Wellenzustand](WAVE_TRANSPORT_STUDY.md) ist separat dokumentiert. Sein erster Lauf scheiterte an der fehlenden Szenenpuffer-Bindung im Engine-Renderer; er ersetzt die hier bestandenen Ergebnisse nicht.
