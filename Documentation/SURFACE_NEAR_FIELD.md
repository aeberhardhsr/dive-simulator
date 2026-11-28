# Wasserlinie ohne gerade Nebelkante

Die neue Wasservorschau verwendet eine feinere Tiefenverteilung für den Volumennebel in Kameranähe. Im untersuchten Oberflächenblick verschwindet dadurch die gerade Helligkeitskante, während Wasserstreuung und Uferspiegelungen aktiv bleiben. Das ist eine gezielte Korrektur des Wasserpfads, keine Abnahme der gesamten [visuellen Zielqualität](VISUAL_TARGET.md).

Start: `Builds/WindowsWaterNearField/Start-WaterPreview.cmd`. Der gesamte Paketordner wird benötigt. Der Starter öffnet das normale Menü mit `-DiveWaterPreview`; die direkte EXE ohne diesen Schalter verwendet weiterhin den bisherigen Wasserpfad. Das Menü-/HUD-Redesign ist noch nicht umgesetzt.

## Ursache und Kontrollreihe

Die vorherige Vorschau `WindowsWaterPreview` zeigte bei 1080p/Hoch im unteren Wasserbereich einen waagerechten Helligkeitssprung. Fünf getrennte Läufe desselben unveränderten Pakets grenzen die Ursache ein:

| Kontrolle | Beobachtung | Größter mittlerer RGB-Zeilensprung im untersuchten Bereich |
| --- | --- | ---: |
| Bisherige Einstellung | Gerade Kante sichtbar | 7,75 |
| Bildschirmreflexionen aus | Kante bleibt; entfernte Uferspiegelungen fehlen teilweise | 8,24 |
| Gesamter Volumennebel aus | Kante verschwindet; Volumenstreuung fehlt | 0,54 |
| Konservative Tiefenoptimierung aus | Kante bleibt | 7,32 |
| Nur Wasservolumen-Dichte null | Kante verschwindet; allgemeiner Luftnebel bleibt | 1,36 |
| Tiefenverteilung 8 statt 32 | Kante verschwindet bei aktiver Wasserstreuung und Reflexion | 0,51 |

Die Messregion liegt bei 1920×1080 zwischen x=100–419 und y=585–674, unterhalb des Horizonts und außerhalb der Instrumente. Der Wert beschreibt den größten Sprung zwischen benachbarten, horizontal gemittelten RGB-Zeilen. Er ist ein lokaler Nachweis für diese Kante, kein allgemeiner Bildqualitätswert. Die rotierenden Vergleichsläufe enden innerhalb von 0,2 Simulationssekunden um denselben Zeitpunkt; Kameras und Wellen sind deshalb ähnlich, aber nicht pixelidentisch.

Die Engine verteilt den Nebel logarithmisch in Tiefenschichten. Ihre Funktion `CalculateGridZParams` in `RenderUtils.h` verwendet die Near-Clip-Distanz zuzüglich 9,5 cm. Mit der Projekt-Near-Clip-Distanz 0,5 cm, 600 m Luftsichtbereich, 64 Schichten und Verteilung 32 liegt der nominale Mittelpunkt der ersten Schicht rund 2,28 m entfernt. Die Abtastung schwankt zusätzlich durch zeitliches Jitter. Eine nur wenige Zentimeter entfernte Wassergrenze kann so mit einem großen, bereits nassen Volumenelement vermischt werden.

Bei Verteilung 8 liegt der erste nominale Mittelpunkt rund 20 cm entfernt. Die Kontrollreihe stützt diese grobe Nahabtastung als Ursache der beobachteten Kante. Die neue Einstellung gilt im Wasserprofil für den jeweiligen Spielstart; der Sichtbereich, die Zahl der Schichten, Bildschirmreflexionen, Schatten und Volumenbeleuchtung bleiben erhalten. Engine-Dateien wurden nicht geändert. Als Gegenleistung wird die Tiefenauflösung in weiter entfernten Nebelbereichen gröber.

## Vergleich

Bisheriger Oberflächenlauf:

![Gerade Kante im bisherigen Wasserprofil](Images/SurfaceNearField_Before.png)

Neuer Oberflächenlauf aus dem korrigierten Paket:

![Wasseroberfläche nach feinerer Nahabtastung](Images/SurfaceNearField_After.png)

Der abschließende Paketlauf erreicht in derselben Messregion 1,00 statt 7,75 RGB-Stufen. Die leichte Abweichung vom diagnostischen Wert 0,51 entsteht bei unterschiedlichen Wellen-/Kamera-Endzuständen. Beide Aufnahmen sind unveränderte Spielbilder. Die neue Darstellung wurde auch visuell auf die gerade Kante und erhaltene Uferspiegelungen geprüft.

## Paketprüfung und Reproduktion

`artifacts/water-preview-20261127-015527` besteht alle 13 Szenen mit 46 Aufnahmen: die drei Einstiegsorte, Oberfläche, Sturm, Nacht mit/ohne Lampe, Ersatzdarstellungen ohne Volumennebel, Nanite bzw. Raytracing, Nachladen, Uferwechsel und Sediment. Zusätzlich besteht der automatische Tauchlauf über die Haupt-EXE. Diese technische Prüfung bestätigt keine vollständige visuelle Abnahme; in der Sturmaufnahme bleiben beispielsweise störende Muster an bewegten Wellen sichtbar.

Der [Prüfbericht](SURFACE_NEAR_FIELD_RESULTS.json) verknüpft Quell-, EXE-, Inhaltscontainer- und Bildprüfsummen mit den Kontrollen und Messungen. Die normal bediente Strecke vom Menü bis zum Tauchgang wurde für dieses Paket noch nicht manuell durchgespielt. Der frühere Menüversuch und sein Windows-Eingabehindernis sind im [Vorgängerbericht](WATER_PREVIEW.md) dokumentiert.

```powershell
./Tools/TestWaterPreview.ps1 -PackageDirectory Builds/WindowsWaterNearField
./Tools/TestPackageLauncher.ps1 -PackageDirectory Builds/WindowsWaterNearField -WaterPreview -OutputDirectory artifacts/water-preview-20261127-015527/launcher
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsWaterNearField -WaterPreview -Scenario underwater -Resolution 1080p -Quality 2 -Seconds 60
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsWaterNearField -WaterPreview -Scenario surface -Resolution 1080p -Quality 2 -Seconds 60
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsWaterNearField -WaterPreview -Scenario stream -Entry 1 -Resolution 1080p -Quality 2 -Seconds 90
```

Neue Prüfungen erzeugen neue Zeitstempelverzeichnisse; den Ausgabepfad des Launcher-Tests entsprechend zuordnen. Einzelne kurze Messungen ersetzen keinen aktuellen Langzeittest.

Bei 1080p/Hoch mit Wasserprofil erreicht die RTX 3090 im Mittel 69,15 FPS unter Wasser, 66,87 FPS an der Oberfläche und 97,33 FPS beim Schwimmen/Nachladen. Das liegt nahe am vorherigen Wasserprofil. Zwei Nachladeruckler bis 42,81 ms bleiben bestehen; die [Messdetails](PERFORMANCE.md) belegen keine durchgehend gehaltenen 60 FPS.

Die insgesamt noch vereinfachte Landschaft, Software-Reflexionslücken, vollständige Brechung und Dämpfung entlang reflektierter Wasserwege bleiben offen. Auch die mögliche doppelte Behandlung eines von oben betrachteten Wasserwegs durch Oberflächenmaterial und Volumenmedium ist durch diese Rasterkorrektur nicht abschließend geklärt. Die Menü-/HUD-Neugestaltung folgt weiterhin nach Unterwasser- und Überwassergrafik.
