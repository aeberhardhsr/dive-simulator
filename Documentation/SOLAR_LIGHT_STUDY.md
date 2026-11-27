# Farbverlust auf dem Weg vom Sonnenlicht zur Oberfläche

Ein nativer Rendererversuch ergänzt die bisherige skalare Abschwächung des Sonnenlichts unter Wasser um unterschiedliche Absorption für Rot, Grün und Blau. Die Änderung wirkt auf das direkte gerichtete Licht vor der Oberflächenbeleuchtung. Sie ist standardmässig ausgeschaltet und noch nicht in ein Spielpaket übernommen. Die zuletzt geprüfte spielbare Vorschau bleibt `Builds/WindowsPlantColor/Start-WaterPreview.cmd`.

## Wirkung und verbleibende Bildprobleme

Die Felsen und der Seegrund verlieren einen Teil ihres graubraunen Eindrucks. Die grossen freien Bodenflächen, die flache Lichtwirkung und die wenig räumliche Anordnung sind damit weiterhin nicht auf dem Niveau der [Nutzerreferenzen](VISUAL_TARGET.md). Dieser Versuch ist ein begrenzter technischer Schritt, keine visuelle Abnahme.

Bisherige Beleuchtung:

![Native Kontrollaufnahme](Images/SolarLight_Before.png)

Zusätzliche Absorption des direkten Sonnenlichts:

![Native Aufnahme mit zusätzlicher Absorption](Images/SolarLight_After.png)

Beide Aufnahmen zeigen dieselbe Kamera bei Weesen, 21. Juni, 12 Uhr. Die separat untersuchte [Kameraweg-Korrektur](SPECTRAL_VIEW.md) ist in beiden Bildern ausgeschaltet. Die Aufnahmen wurden nicht nachträglich eingefärbt.

## Umsetzung und Grenzen

`SolarTransport.ush` berechnet aus der Empfängerhöhe unter der mittleren Wasserfläche und dem gebrochenen Sonnenwinkel eine Wasserweglänge. Die zusätzliche Transmission folgt `exp(-RGBAbsorption * Weg)`. Die gestalterisch gewählten Koeffizienten sind `(0.12, 0.015, 0.035)` pro Meter; sie sind keine Messwerte des Walensees. Die vorhandene skalare Wasserabschwächung, Lichtmuster und Schatten bleiben erhalten.

Der Eingriff betrifft nur gerichtetes direktes Licht im Deferred Renderer. Lokale Punkt-/Spotlichter erhalten den Faktor 1, ebenso Empfänger über Wasser. Indirektes Licht, Volumenlicht, reflektierte Wege und die bewegte Wassergrenze werden dadurch noch nicht gemeinsam korrigiert. Die mittlere Wasserhöhe ist fest `Z=0`; die ersten 20 cm blenden den Zusatz ein. Auch gerichtetes Mondlicht kann unter Wasser betroffen sein. Die Änderung ist keine vollständige spektrale Transportsimulation.

Die Wasserzuordnung verwendet in diesem ersten Versuch ausschliesslich die Höhe. Eine horizontale Begrenzung auf die tatsächliche Seefläche fehlt noch: Gelände ausserhalb des Sees unterhalb `Z=0` würde ebenfalls abgeschwächt. Der Test oberhalb der Wasserfläche deckt diesen Sonderfall nicht ab. Vor einer Standardaktivierung muss die Seemaske in diese Zuordnung einbezogen werden.

`PrepareSolarShaderStudy.py` kopiert die installierten privaten Shader in den ignorierten `Intermediate`-Ordner der Editor-Sandbox und ändert ausschliesslich die Kopie von `DeferredLightPixelShaders.usf`. Der Versuch registriert `/Engine/Private` auf diesen lokalen Ordner. Die installierten Engine-Dateien bleiben unverändert und werden nicht ins Repository aufgenommen. Der vorbereitende Schritt akzeptiert nur die geprüfte SHA-256-Fassung der vorhandenen UE-5.8.4-Datei; eine andere Engine-Version erfordert erneute Prüfung.

## Reproduktion

```powershell
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/.geodata-env/Scripts/python.exe Tools/PrepareSolarShaderStudy.py

# Vergleich: gleicher Aufruf einmal ohne und einmal mit -SolarShaderStudy.
./Tools/SmokeTest.ps1 -EditorSandbox -Scenario rocks -Entry 0 -HabitatView -HabitatLowView -WaterPreview -PopulationChecks -Hour 12 -Date 2026-06-21 -MaxFPS 30 -SolarShaderStudy
./Tools/TestCaustics.ps1 -EditorSandbox -WaterPreview -Case day -SolarShaderStudy
./Tools/TestCaustics.ps1 -EditorSandbox -WaterPreview -Case air -SolarShaderStudy
./Tools/SmokeTest.ps1 -EditorSandbox -Scenario torch -Entry 0 -Hour 0 -Date 2026-06-21 -NoBubbles -ParticleScale 0 -WaterPreview -MaxFPS 30 -SolarShaderStudy
```

Alle Läufe müssen nacheinander erfolgen. Die Szenen benötigen gemeinsam `-DiveSmokeTest -DiveWaterPreview -DiveSolarShaderStudy`; Paketprüfungen weisen den Versuchsschalter ausdrücklich zurück. Die vorbereiteten Shader müssen zum aktuellen Quellstand passen.

Die [Ergebnisdatei](SOLAR_SHADER_RESULTS.json) bindet Quell- und Modulprüfsummen, Engine-Original/Kopie, Protokolle, Kameraposen und Originalaufnahmen. Die Bildauswertung verwendet die Skala 0–255 nach Tone Mapping. Sie prüft sichtbare Änderung und begrenzte Bildabweichungen, keine radiometrische Genauigkeit. Die vollständigen Protokolle enthalten auch vorhandene Engine-/Plugin-Startdiagnosen; bestandene Szenenprüfungen bedeuten nicht, dass jedes Protokoll frei von Warnungen ist.

**Nativer Prüfstand vom 27. November: acht Läufe mit 26 Aufnahmen bestehen.** Alle 150 vorab erfassten Quell-/Moduldateien bleiben über den Vergleich unverändert; die installierte Engine-Shaderdatei stimmt weiterhin mit ihrer ursprünglichen Prüfsumme überein. Kontroll- und Versuchsaufnahmen der Habitat- und Testflächen verwenden identische Kameraposen.

| Vergleich | Gemessene Bildänderung |
| --- | --- |
| Beleuchtete neutrale Testfläche in 3,2 m Tiefe | RGB-Mittelwerte ändern sich um −19,749 / −1,300 / −3,006; Rot verliert deutlich stärker als Grün und Blau. |
| Beschatteter Bereich derselben Fläche | Mittlere absolute RGB-Abweichung 0,535. |
| Testfläche 4 m über Wasser | Mittlere absolute RGB-Abweichung 0,158 im Licht und 0,271 im Schatten. |
| Zwei Nachtaufnahmen mit eingeschalteter 1.000-Lumen-Lampe | Mittlere absolute RGB-Abweichung 0,060 bzw. 0,069 im gesamten Bild; Lampenfleck und Lichtkegel bleiben sichtbar. |

Diese Kontrollen stützen die beabsichtigte Beschränkung auf direktes gerichtetes Unterwasserlicht. Sie ersetzen keinen Paket- oder Leistungstest. Eine gemeinsame Darstellung mit zusätzlicher Kameraweg-Korrektur wurde in dieser Reihe nicht bewertet.

Die zuvor untersuchten [kleinen Sedimentrücken](SEDIMENT_BEDFORM_INVESTIGATION.md) wurden wegen unzureichender Bildwirkung zurückgenommen. Nächste grafische Arbeit bleibt das Zusammenspiel von Beleuchtung, grösserer Geländeform und Oberflächendetail. Überwasserlandschaft und anschliessend [Menü/HUD/FPV](UI_UX_TARGET.md) folgen in der vereinbarten Reihenfolge.
