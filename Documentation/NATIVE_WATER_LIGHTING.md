# Räumliche Wasserbeleuchtung: Entwicklungsstand

Die [Streulichtdiagnose](SCATTERING_INVESTIGATION.md) führte zu einer neuen Renderingvariante. Sie ist **experimentell, nur in Diagnoseläufen aktivierbar und kein neuer Spielstandard**. Der damalige spielbare Paketstand war `WindowsRockAprons`; neuere Spielpakete stehen im [README](../README.md). Die gewünschte [visuelle Qualität](VISUAL_TARGET.md) ist weiterhin offen.

## Implementierte Variante

Die bisherige Kameraformel schwächt das bereits von Unreal integrierte Nebellicht erneut entlang der Sichtstrecke ab. Die neue Variante überlässt die skalare Auslöschung und direkt beleuchtete Streuung dem nativen Volumennebel. Die zusätzliche Farbabsorption bleibt im Kameraeffekt und wird weiterhin an der lokalen Wasserhöhe abgeschnitten. Für noch nicht ausreichend dargestelltes diffuses Wasserlicht bleibt ein gestalteter, unbeschatteter Kamerazusatz mit 30 Prozent der bisherigen Stärke bestehen. Dies ist eine Näherung, keine vollständig physikalische Strahlungstransportlösung.

Die Volumen-Auslöschung wird auf den bisher verwendeten Koeffizienten `1,45 / Sichtweite` abgestimmt. UE 5.8 rechnet die globale Nebeldichte intern durch 1.000 und verwendet zusätzlich den Faktor 0,5; bei unserer Dichte `0,35 × 14 / Sichtweite` ergibt sich dafür ein Extinktionsfaktor von etwa 5,918. Er wird beim Wasserübergang weich eingeblendet. Die zusätzliche spektrale Absorption und geringe Höhendichteänderung sind darin nicht enthalten. Diese Zuordnung ist keine Kalibrierung gegen gemessene Walensee-Wasserwerte.

Die Sonnen-Lichtfunktion erhält außerdem einen separaten Pfad für Unreals gerichtete Volumenlichtprojektion. Dieser wertet die vorhandenen Wellen und die begrenzte Strahlenbündelnäherung auf einer repräsentativen Wassertiefe aus. Sie entspricht der Kameratiefe, begrenzt auf ein bis zwanzig Meter. Der native Renderer berechnet weiterhin Lichtstreuung und Schatten; die neue Sonnenmaske wird nicht auf die Tauchlampe angewandt. Wellenparameter und Uhr werden mit dem sichtbaren Wasser geteilt.

Die Projektion bleibt zweidimensional: Sie beschreibt weder eine vollständige dreidimensionale Kaustik noch die durch Brechung geänderte Richtung sämtlicher Schatten und Lichtstrahlen. Beim Ein-/Austauchen wird zwischen ursprünglicher und neuer Projektion überblendet. Bei ausgeschaltetem Experiment bleibt die ursprüngliche Lichtfunktionsauswertung erhalten; sie wird ausdrücklich nicht durch eine weiße Maske ersetzt.

`-NativeScattering` in `Tools/SmokeTest.ps1` aktiviert den Versuch. Ohne `-DiveSmokeTest` wird der zugrunde liegende Runtime-Schalter ignoriert. Wenn `r.VolumetricFog` deaktiviert ist, verwendet auch ein angeforderter Versuch wieder die bisherige Kameraformel und Nebelskalierung. Damit wird die native Integration nicht vorausgesetzt, wenn die betreffende Grafikfunktion gar nicht läuft.

## Bildvergleich und offene Probleme

Neue Sonnenkameras ergänzen die bisherigen Bodenansichten: leicht nach oben zur Sonne, direkt zur Sonne und in Gegenrichtung. Die Auswahl hängt vom aktuellen Sonnenstand ab; der Bericht vergleicht die tatsächlich protokollierten Posen.

Bisheriger Standard, Blick in Richtung Sonne:

![Bisherige Unterwasserdarstellung in Richtung Sonne](Images/NativeWaterLighting_Standard.png)

Experiment, identische Kamera:

![Experimentelle native Streuung und Sonnenprojektion](Images/NativeWaterLighting_Prototype.png)

Die neue Sonnenprojektion verändert die Volumenbeleuchtung sichtbar. Eine überzeugende Umsetzung der Referenz ergibt sich daraus noch nicht: Die Färbung ist zu grün, die räumliche Lichtstruktur noch unzureichend, und in Gegenrichtung bleibt ein zu dunkler Fernbereich. Insbesondere wirken Gelände und Himmel durch die Wasseroberfläche zu direkt sichtbar. Der erhöhte Lichtanteil darf daher nicht allein als Qualitätsverbesserung gewertet werden. Das Experiment wird vorläufig nicht als normales Grafikprofil ausgeliefert.

## Ursache an der Wasserunterseite

Im lokal installierten UE-5.8-Quellcode steht `CameraIsUnderWater` sowohl in `BasePassPixelShader.usf` (Zeile 1698) als auch in `SingleLayerWaterComposite.usf` (Zeile 68) konstant auf `false`. Die Funktion `EvaluateWaterVolumeLighting` besitzt zwar einen Unterwasserzweig, aber die genannten Aufrufer wählen ihn nicht. Das erklärt, warum eine zweiseitig sichtbare Single-Layer-Water-Fläche allein keine vollständige Grenzflächenberechnung von unten liefert. Der [offizielle Überblick zum Single-Layer-Water-Modell](https://dev.epicgames.com/documentation/en-us/unreal-engine/single-layer-water-shading-model-in-unreal-engine) beschreibt dessen getrennten Renderdurchlauf und die Mischung von Oberflächenreflexion und Volumenstreuung.

Als nächster Umsetzungsschritt ist deshalb die Wasserunterseite mit geeigneter Reflexion, Brechung und Totalreflexion außerhalb des Durchsichtwinkels aufzubauen. Die Kameradämpfung muss zur tatsächlich durch Wasser verlaufenden Strecke passen. Die vorhandene Überwasserdarstellung darf dabei nicht durch pauschal erhöhte Deckkraft ersetzt werden. Engine-Dateien wurden nicht geändert. Bodenmaterialien, Bewuchs und Felsvariationen bleiben zusätzliche, eigenständige Aufgaben.

## Prüfung

`Tools/TestNativeWaterLighting.ps1` vergleicht Standard/Experiment am Grund, in Sonnenrichtung und beim Blick durch die Oberfläche. Dazu kommen astronomische Nacht mit ein-/ausgeschalteter Lampe, die Rückfallvariante ohne Volumennebel und der bestehende kontrollierte Kaustiktest im normalen Modus. Der Test dokumentiert Runtime-Modul, Source- und Materialprüfsummen. `Tools/ReportNativeWaterLighting.py <Laufverzeichnis>` prüft Kameragleichheit, Runtime-Marker und die bereits verwendeten Nacht-/Kaustikschwellen. Die neuen Bilder ersetzen keine manuelle visuelle Abnahme.

Der [Ergebnisbericht](NATIVE_WATER_LIGHTING_RESULTS.json) enthält den tatsächlichen Prüfumfang und seine Grenzen. Es gibt für diesen Prototyp noch keinen Paket-, Performance- oder Langzeitnachweis. Die weitere Reihenfolge bleibt Unterwassergrafik, Überwassergrafik, anschließend [Menü/HUD/FPV](UI_UX_TARGET.md).

Der aktuelle Lauf `artifacts/native-water-lighting-20261126-185759` besteht neun Szenenfälle mit 25 Aufnahmen und zusätzlich den unveränderten kontrollierten Kaustikvergleich mit vier Aufnahmen. Die Kameraposen der drei Vergleichspaare stimmen exakt überein. In der bestehenden Nacht-Bildmaske liegt das 99,9. Perzentil bei ausgeschalteter Lampe bei null; mit Lampe liegen 276.209 Pixel über dem bisherigen Helligkeitsschwellwert. Der Rückfalllauf protokolliert `native=0 requested=1` bei deaktiviertem Volumennebel. Diese Ergebnisse bestätigen die technische Funktionsfähigkeit des Experiments, nicht seine visuelle Freigabe. Vorhandene Engine-Fehlerzeilen werden im Bericht erhalten und nicht als fehlerfreies Engine-Protokoll ausgegeben.
