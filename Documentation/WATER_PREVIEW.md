# Spielbare Vorschau des neuen Wasserpfads

Dieser Bericht dokumentiert das ursprüngliche Paket. Die nachfolgende [Korrektur der Nebelkante](SURFACE_NEAR_FIELD.md) verwendet eine feinere Tiefenabtastung in Kameranähe; die damaligen Ergebnisse unten bleiben als Vergleich erhalten.

`Builds/WindowsWaterPreview/Start-WaterPreview.cmd` startet das normale Spielmenü mit dem neuen Wasserpfad. Der gesamte Paketordner wird benötigt. Der Starter setzt `-DiveWaterPreview`; er startet keinen automatischen Test. Die direkte `divingsim.exe` ohne diesen Schalter verwendet weiterhin den bisherigen Wasserpfad.

Die Vorschau verbindet die [an der Wasseroberfläche begrenzte Lichtstreuung](BOUNDED_WATER.md) mit den [klareren Spiegelungen an der Wasserunterseite](WATER_REFLECTIONS.md). Die optischen Einstellungen gelten für diesen Spielstart und werden nicht als neue Spielereinstellungen gespeichert. Unterstützt die aktive Grafikschnittstelle Hardware-Raytracing, verwendet die Vorschau die Beleuchtung am Reflexionstreffer. Andernfalls bleibt die Softwaredarstellung verfügbar, deren unvollständige Reflexionen des Seegrunds weiterhin sichtbar sind.

## Umfang und Grenzen

Der neue Pfad besitzt getrennte Ober- und Unterseiten mit gemeinsamen Wellen. Das Wasservolumen endet an der bewegten Wasseroberfläche; ein zusätzlicher pauschaler Aufhellungsschleier in der Kamera entfällt bei aktiver Volumenstreuung. Ohne Volumennebel greift die analytische Ersatzdarstellung. Die Wasserfläche selbst bleibt für sekundäre Raytracing-Strahlen unsichtbar, damit ihre noch flache Hilfsgeometrie keine falschen Spiegelungstreffer erzeugt.

Die Vorschau ist ein weiterer Entwicklungsstand. Vollständige Brechung des Durchblicks nach Snell, Dämpfung entlang reflektierter Wasserwege und mehrfach reflektierte Wasserflächen sind nicht umgesetzt. Im Oberflächen-Messbild ist links im nahen Wasser eine gerade Helligkeitskante sichtbar; deren Ursache ist noch zu prüfen. Die Bodenübergänge, Vegetation und Lichtwirkung erreichen die [Nutzerreferenzen](VISUAL_TARGET.md) noch nicht. Menü, HUD und FPV bleiben wie vereinbart bis nach der Unterwasser- und Überwassergrafik zurückgestellt.

## Aufnahmen aus dem Spielpaket

Spiegelung des Grunds mit Hardware-Raytracing:

![Spiegelung an der Wasserunterseite](Images/WaterPreview_Reflection.png)

Unterwasseransicht in Weesen; noch sichtbar vereinfachte Boden- und Bewuchsübergänge:

![Weesener Grund im Wasserprofil](Images/WaterPreview_Bed.png)

Gleiche Reflexionsansicht ohne Raytracing; die fehlende Bodenabdeckung bleibt erkennbar:

![Software-Reflexion mit verbleibenden Lücken](Images/WaterPreview_Software.png)

## Reproduktion

Die Paketserie `artifacts/water-preview-20261127-012033` besteht 13 Fälle mit 46 Aufnahmen: drei Einstiegsorte, Oberfläche, Sturm, Nacht mit/ohne Lampe, Ersatzdarstellungen ohne Volumennebel, Nanite bzw. Raytracing, Nachladen, Uferwechsel und Sediment. Der Haupt-EXE-Launcher besteht zusätzlich seinen automatisierten Tauchlauf. Der [Prüfbericht](WATER_PREVIEW_RESULTS.json) bindet Quellen, EXE, Inhaltscontainer, Bilder und Messungen.

Ein separater Start ohne `-DiveSmokeTest` zeigt das normale Menü und meldet `smoke=0 hardware=1`. Die geplante Bedienprüfung konnte nicht abgeschlossen werden: Die Klicks wurden zweimal vom Windows-Fenster `PickerHost.exe` abgefangen, auch nach erneutem Aktivieren des Spiels. Die eigene Testinstanz wurde beendet. Menüaufnahme und Startprotokoll liegen unter `artifacts/water-preview-manual`; ein vollständig durchgespielter normaler Menüstart wird damit nicht behauptet.

Die drei Leistungsläufe erreichen auf RTX 3090 / Ryzen 9 9950X3D bei 1080p/Hoch mit Wasserprofil im Mittel 69,26 FPS unter Wasser, 66,74 FPS an der Oberfläche und 97,06 FPS auf der Nachladestrecke. Dort bleiben zwei Frames über 33,33 ms, maximal 45,69 ms. Einzelheiten und Grenzen stehen in [PERFORMANCE.md](PERFORMANCE.md). Ein aktueller Langzeittest und die vollständige visuelle Abnahme bleiben offen.

```powershell
./Tools/Build.ps1 -EditorSandbox -Target Package -BuildSlot WaterPreview -PackageDirectory Builds/WindowsWaterPreview
./Tools/TestWaterPreview.ps1 -PackageDirectory Builds/WindowsWaterPreview
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsWaterPreview -WaterPreview -Scenario underwater -Resolution 1080p -Quality 2 -Seconds 60
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsWaterPreview -WaterPreview -Scenario surface -Resolution 1080p -Quality 2 -Seconds 60
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsWaterPreview -WaterPreview -Scenario stream -Entry 1 -Resolution 1080p -Quality 2 -Seconds 90
```

Die Qualitätsbezeichnung der Messung lautet **1080p/Hoch mit Wasserprofil**. Das Profil schaltet die Reflexion der vordersten transparenten Fläche und die Beleuchtung am Reflexionstreffer zusätzlich frei; Unreal schaltet diese Optionen in der normalen Stufe Hoch sonst ab. Die Messprotokolle erfassen die tatsächlich aktiven Konsolenvariablen. Ergebnisse früherer Spielpakete gelten nicht für diesen aufwendigeren Pfad.
