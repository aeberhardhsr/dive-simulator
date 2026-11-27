# Kameramaterial am Wasserübergang

Die schwarze Fläche unmittelbar unter der Wasseroberfläche entsteht im geprüften Blick durch die Parameterüberblendung des Kamera-Materials. Das korrigierte Windows-Paket erhält dort die bereits integrierte grüne Wassertrübung. Die blaue Fläche in der Oberflächenreflexion bleibt ein separates offenes Problem; die Referenzqualität ist damit nicht erreicht.

Start: **`Builds/WindowsWaterline/Start-WaterPreview.cmd`**. Der gesamte Paketordner wird benötigt. Menü/HUD/FPV, aufgeteilte Zellvorbereitung und korrigierte Startbelichtung sind enthalten. Die experimentellen Verfahren der Quellcode-Engine bleiben deaktiviert.

## Ursache und Änderung

`UMaterialInterface::OverrideBlendableSettings` in Unreal 5.8.4 überblendet bei einem Materialgewicht zwischen null und eins die skalaren Parameter mit den Standardwerten des Elternmaterials. Es wird nicht einfach das fertige Bild mit dem Original gemischt. Das bisherige `CameraWaterBlend` wurde als solches Materialgewicht verwendet.

20 cm unter der Oberfläche beträgt dieses Gewicht 0,352. Der Modusparameter `NativeScattering=2` wurde dadurch im tatsächlich gerenderten Material zu 0,704. Statt die bereits integrierte Wasserstreuung unverändert durchzulassen, führte der Shader erneut analytische Absorption aus. Bei 2 cm Eintauchtiefe fiel der Modus sogar auf 0,009344. Die CPU-Prüfung des ursprünglichen Materialparameters konnte diese nachträgliche Engine-Überblendung nicht erkennen.

Der Kamerapass erhält jetzt oberhalb des Wassers Gewicht null und darunter Gewicht eins. Der neue Parameter `Immersion` trägt den stetigen Übergang in den Shader. Analytische Renderpfade mischen ihre resultierende Strahldichte mit der ursprünglichen Szene; Sichtweite, Tiefe und der gewählte Modus werden dabei nicht verfälscht. Im begrenzten nativen Wassermedium bleibt der Kamerapass neutral. Bei vollständigem Eintauchen bleiben die bisherigen Endpunktformeln erhalten. Das Material wurde mit `build_optics_content.py` neu erzeugt.

## Kontrollbilder

Vorher, 20 cm unter der eingefrorenen Welle:

![Schwarze Fläche durch fehlerhafte Materialüberblendung](Images/WaterlineBlend_Before.png)

Korrigierter Lauf bei gleicher Kamera:

![Erhaltene native Wassertrübung nach der Korrektur](Images/WaterlineBlend_After.png)

Die Aufnahmen sind unveränderte Spielbilder. Sie zeigen eine Fehlerkorrektur, keine fertige Unterwassergestaltung.

## Prüfung

`TestWaterline.ps1` erfasst denselben Blick bei −20 cm, −2 cm und +20 cm relativ zur lokalen Welle. Wellenzeit, Blickwinkel, Datum und Uhrzeit sind festgelegt. Vorherige Kamera, Isolation ohne Postprozessmaterialien und korrigierte Kamera werden getrennt gestartet. Zeitliche Rekonstruktion kann deshalb kleine Bildunterschiede verursachen.

Im festen leeren Wasserbereich x=400–1199, y=500–699 bei 1600×900 sinkt im nativen Editorlauf der Anteil der Pixel mit Anzeigehelligkeit unter 5 von 99,93 % beziehungsweise 100 % auf null. Die mittlere Anzeigehelligkeit steigt bei −20 cm von 3,85 auf 41,80 und bei −2 cm von nahezu null auf 23,94. Das sind Werte auf der 8-Bit-Anzeigeskala, keine photometrische Messung. Die korrigierten Bilder unterscheiden sich vom Kontrolllauf ohne Postprozessmaterialien im RGB-Mittel um 0,22–0,33 Stufen. Über Wasser beträgt die Abweichung zum ursprünglichen Lauf 0,11 Stufen.

Die Serie `artifacts/waterline-20261127-215750` besteht drei Fälle mit neun Bildern: normales Wasserprofil, ausgeschaltete Postprozessmaterialien und Ersatzmodus ohne Volumennebel. Der Ersatzmodus schaltet zugleich auf analytische Kameradämpfung um; er ist daher keine reine Isolation des Volumenpasses. Sein flacher Übergang und die blaue Fernfarbe sind weiterhin vereinfachte Darstellungen.

Die entsprechende Paketserie `artifacts/waterline-20261127-220217` besteht ebenfalls alle drei Fälle mit neun Bildern und den gleichen Bildgrenzen. Die beiden veröffentlichten Vergleichsbilder zeigen den ursprünglichen Editorlauf und das korrigierte Paket. Die Bildprüfungen und ihre Manifeste werden durch `Tools/ReportWaterline.py` gebunden.

Beim nachfolgenden Nachttest erfasste die historische Messmaske irrtümlich einen Teil des neu gestalteten Atemregler-HUDs: Die 2.184 helleren Pixel ausserhalb des Fadenkreuzes lagen vollständig bei x=200–355, y=160–173. Das 99,9-%-Perzentil von 47 war deshalb kein Nachweis für beleuchtetes Wasser. Die aktualisierte Maske beginnt bei y=200, lässt die übrigen Grenzen und das ausgeschlossene Fadenkreuz erhalten und verwendet unveränderte Helligkeitsgrenzen. Im freien Szenenbereich beträgt das Perzentil ohne Lampe null; mit Lampe werden 246.116 zuvor dunkle Pixel deutlich beleuchtet. Beide Rohmessungen und die geänderte Maske werden dokumentiert. Beleuchtung und Bilder wurden dafür nicht verändert.

Die vorangegangenen Kontrollen von Reflexionen, Schatten, Himmelskomponenten, Nebelreichweite, Nebelkomposition und zeitlicher Nebelrekonstruktion beseitigten die schwarze Fläche nicht. Entscheidend war `ShowFlag.PostProcessMaterial 0` im Lauf `artifacts/waterline-20261127-215517`. Die Tiefen-/Normalenprüfung `artifacts/waterline-20261127-214231` bestätigte fehlende sichtbare Bodennormalen im betroffenen Hintergrundbereich, belegte aber keine Ursache im Nebel. Die erste Buffer-Serie `214056` ist ausdrücklich ungültig: Unreal erkannte den damals falsch geschriebenen Ansichtsmodus nicht. Die Serie `213440` umfasst nur einen erfolgreichen Lichtfunktionsfall; der zweite Fall startete wegen einer später korrigierten Parameterprüfung nicht. Der erste Kameraversuch `212620` scheiterte an einer zu strengen Abfrage ausschliesslich vermessenen Bodens am modellierten Ufer.

Die Diagnoseoptionen verändern keine normalen Spielvorgaben. Manifestdateien binden Quellen, Modul, Materialien, Einstellungen und Bilder der einzelnen Läufe. Die Diagnosefixture wurde während der Untersuchung erweitert; frühere Manifeste gehören zu ihrem jeweiligen Zwischenstand.

```powershell
./Tools/Build.ps1 -EditorSandbox -Target Editor -MaxParallelActions 4
./Tools/Build.ps1 -EditorSandbox -Target Content -OnlyOptics
./Tools/TestWaterline.ps1 -Study correction
```

Die weiteren Studien sind über `-Study standard|volume|light|shadow|geometry|sky|composition|depth|camera` wählbar. Die Paketprüfung verwendet zusätzlich `-PackageDirectory`. Das Skript verweigert den Start bei einer bereits laufenden Unreal-/Spielinstanz.

## Spielbares Paket

Der Paketbau mit der installierten Engine 5.8.4 endet erfolgreich. Die drei Wasserlinienfälle mit neun Bildern sowie die 13 Fälle unter `artifacts/water-preview-20261127-220345` mit 46 Bildern bestehen: drei Tauchplätze, Oberfläche, Sturm, Nacht mit/ohne Lampe, Ersatzdarstellungen ohne Volumennebel, Nanite und Hardware-Raytracing, Nachladen, Uferwechsel und Sediment. Alle 55 Paketbilder wurden als Kontaktbögen betrachtet. Der Uferlauf bestätigt die korrigierte Wassertrübung auch während des bewegten Ein- und Auftauchens; er erreicht Rückkehr und Auswertung. Der automatische Start über die Haupt-EXE erreicht ebenfalls `DIVE_SMOKE_PASS`.

Der [Prüfbericht](WATERLINE_BLEND_RESULTS.json) bindet Quellen, Modul, EXE, Inhaltsarchive, Materialbau, Kameras, Bilder, Nachtprüfung und Starter. Die Helligkeitsgrenzen prüfen den beschriebenen Fehler und keine allgemeine Grafikqualität. Es gibt keine neue Leistungs- oder manuelle Menüabnahme für diese Änderung. Die blaue Reflexionsfläche, vollständige gebrochene/reflektierte Lichtwege sowie Detail- und Lichtqualität der Welt bleiben offen.

```powershell
./Tools/TestWaterline.ps1 -Study correction -PackageDirectory Builds/WindowsWaterline
./Tools/TestWaterPreview.ps1 -PackageDirectory Builds/WindowsWaterline
./Tools/TestPackageLauncher.ps1 -PackageDirectory Builds/WindowsWaterline -WaterPreview -OutputDirectory artifacts/waterline-launcher
```
