# Räumlich begrenztes Wassermedium

Nachfolgender Stand: Das kombinierte Wassermedium ist über `-DiveWaterPreview` inzwischen auch im normalen Spielmenü verfügbar. [Paket, Start und Messungen](WATER_PREVIEW.md). Die einzelnen Diagnoseschalter und die folgende Versuchsreihe bleiben davon getrennt.

Der Modus `-DiveBoundedWater` begrenzt die native Volumenstreuung auf den Bereich unter der bewegten Wasseroberfläche. Zusammen mit `-DiveWaterInterface` macht er Himmel und Sonne im Sichtfenster nach oben wieder sichtbar. Er ergänzt die [getrennte Wasserunterseite](WATER_INTERFACE.md). Beide Schalter sind weiterhin auf Smoke-Test-Läufe beschränkt; die jeweils aktuelle Spielvorschau steht im [README](../README.md).

## Lichtweg und Darstellung

Der globale Exponential Height Fog verwendet in diesem Modus bei jeder Kamerahöhe die Wetterparameter der Luft: Luftdichte, Höhenabfall 0,2, keine additive Wasserfarbe und die normale atmosphärische Beleuchtung. Ein separater beweglicher Würfel mit einem Material der Domain `Volume` liefert die Wasserextinktion. Der Würfel ist weder Kollisionsfläche noch Bestandteil der Schatten-, Distanzfeld- oder Raytracing-Geometrie.

Der Materialshader berechnet die Höhe mit denselben Wellenkoeffizienten, Phasen, Uhr und derselben geometrischen Entfernungsausblendung wie die Wasserfläche. Unterhalb der Fläche gilt die Extinktion `1,45 / Sichtweite` pro Meter. Ein 20 cm breiter Übergang vermindert instabile Abtastung an der Grenze. Oberhalb liegt die Wasserdichte bei null. Unreal rechnet diese Materialwerte intern von Metern in Zentimeter um; der Shader enthält daher keinen zusätzlichen Faktor 1/100.

Der Würfel folgt der Kamera und reicht seitlich über die gesamte berechnete Nebeldistanz hinaus. Seine Oberkante deckt den grösstmöglichen Wellenhub ab, seine Unterkante liegt mindestens eine Nebeldistanz unter dem Auge. Unter Wasser beträgt die native Nebeldistanz vier Sichtweiten, begrenzt auf 20–160 Meter. Über Wasser bleiben die bisherigen 600 Meter erhalten; dazwischen gilt die bestehende Eintauchblende. Unreals eigene Entfernungsausblendung beginnt nach 60 % dieser Distanz. Die Grenzen und die endliche Auflösung des Volumengitters bleiben eine Näherung.

Der Kamerapass unterscheidet für dieses integrierte Wassermedium einen dritten Zustand (`NativeScattering = 2`). Er gibt das bereits integrierte Licht unverändert weiter. Eine nachträgliche, von der gesamten Szenentiefe abhängige Absorption würde auch den Nebel im Vordergrund nochmals absorbieren und erzeugte im ersten Versuch schwarze entfernte Konturen. Seit der anschliessenden Kamerakorrektur entfällt auch der zusätzliche analytische Umgebungsanteil: Das native Volumen berücksichtigt bereits die Lichtquellen und den Himmel. Der frühere Zusatz lieferte weiteres, unverschattetes Licht. Die analytische Ersatzdarstellung und der ältere globale Nebelversuch bleiben unverändert.

Das native Volumen verwendet **skalare Extinktion und farbige Streualbedo**, derzeit `(0,07; 0,24; 0,16)`. Das ist keine vollständige spektrale Lichtsimulation. Insbesondere darf der entfallene nachträgliche RGB-Faktor nicht als korrekt gelöste wellenlängenabhängige Absorption ausgegeben werden. Diese muss später an der passenden Stelle im Lichtweg berechnet werden, ohne bereits gestreutes Licht nochmals über die gesamte Hintergrundentfernung abzudunkeln. Auch der reflektierte Wasserweg, die vollständige Snell-Brechung und die Detailqualität der Reflexionen bleiben offen.

Bei ausgeschaltetem `r.VolumetricFog` meldet die Laufzeit den nativen Modus als inaktiv und verwendet wieder die vorhandene analytische, an der Wasseroberfläche begrenzte Kameradämpfung. Die Luftnebelparameter bleiben auch dann Luftparameter.

## Bildvergleich

Die zuvor graue Durchsicht des Grenzflächen-Prototyps:

![Sichtfenster mit globalem Wassernebel](Images/BoundedWater_Before.png)

Gleiche Kamera mit räumlich begrenztem Wassermedium:

![Sichtfenster mit sichtbarem Himmel und Sonne](Images/BoundedWater_Window.png)

Flacher Blick mit Spiegelung und erhaltener integrierter Trübung:

![Unterseite mit begrenzter Wasserstreuung](Images/BoundedWater_Surface.png)

Diese historischen Bilder belegen einen Fortschritt am Wasser-/Luftübergang. Die Gesamtszene erreicht weiterhin nicht die [Qualitätsreferenz](VISUAL_TARGET.md). Die Kontur des Sichtfensters, Reflexionsschärfe, Farbabstimmung, Bodenmaterialien, Pflanzen und die Lichtverteilung in grösseren Tiefen benötigen weitere Arbeit. Die damaligen Bodenansichten zeigten besonders helle, kreidig wirkende Felsen und dünne Pflanzen; spätere [Farbkorrekturen](BENTHIC_COLOR.md) und [Pflanzenmodelle](PLANT_GROWTH.md) werden unten erneut mit dem Volumen geprüft. Die Integration des Volumens mit den weiteren Wasser- und Oberflächenpässen ist noch nicht als vollständiger optischer Lichtweg abgenommen. Die Verbesserung dieser Einzelansicht ist keine visuelle Abnahme des gesamten Tauchspiels.

## Reproduktion und Prüfumfang

`Tools/Build.ps1 -EditorSandbox` baut das Modul. `-Target Content -OnlyWater` erzeugt zusätzlich `M_WaterVolume`; `-Target Content -OnlyOptics` baut den korrigierten Kamerapass. `Tools/TestBoundedWater.ps1` erfasst zwölf Fälle mit 33 Bildern: Sichtfenster vorher/nachher, eine Kontrolle mit Wasserdichte null, Boden vorher/nachher, drei Sonnenrichtungen, Sturm, Nacht mit/ohne Lampe, einen nächtlichen Blick in den Himmel, einen mondbeleuchteten Tauchgang und den Ersatzmodus ohne volumetrischen Nebel. Die Null-Dichte-Kontrolle lässt Kamera, Luftnebel und alle anderen Einstellungen gleich; dadurch lässt sich ein tatsächlicher Renderbeitrag des Volumens von einer bloss registrierten Komponente unterscheiden.

`Tools/ReportBoundedWater.py <Laufordner>` bindet Quellstände, Module, Materialien, Kameras, Protokolle und veröffentlichte Bilder im [Prüfbericht](BOUNDED_WATER_RESULTS.json). Die Wasserparameter und der Reflexionsmodus werden zur Laufzeit verglichen. Die dunkle Nacht verwendet die bestehenden Grenzwerte. Ein festgelegter Ausschnitt oberhalb der Wasserlinie prüft, ob die Luftansicht durch das neue Wasservolumen verändert wird; er beweist nicht die Unverändertheit sämtlicher Überwasseransichten. Die bekannten Startfehler der Editor-Testplugins werden separat aufgeführt.

Der erste Lauf stoppte beim Vergleich einer schwarzen RGBA-Nebelfarbe: Der neue Code setzte Alpha 1, während die Prüferwartung durch Multiplikation Alpha 0 erhielt. Die Erwartung ist korrigiert. Ein anschliessender Bildlauf zeigte das klare Sichtfenster, aber noch den schwarzen Fernstreifen durch die nachträgliche RGB-Absorption. Diese Zwischenbilder bleiben unter `artifacts/bounded-water-first-images` erhalten. Erst der folgende Lauf verwendet den korrigierten integrierten Kamerapass.

Die erste grössere Serie `artifacts/bounded-water-20261126-194238` ist kein vollständiger Erfolg: Im festgelegten Luftausschnitt betrug die mittlere RGB-Abweichung 2,354 statt der geforderten weniger als 2. Die auf 56 Meter verkürzte Nebeldistanz wurde dort auch über Wasser benutzt. Zudem stoppte der Sturmfall, weil die Sonnenaktualisierung die Uhr der Wasserfläche fortschrieb, ohne zugleich die Uhr des Volumens zu setzen. Die Korrektur erhält die bisherige Nebeldistanz über Wasser und synchronisiert den Volumentakt an derselben Stelle wie beide Wasserflächen. Die Prüfgrenzen wurden nicht gelockert.

## Übermässiges Restlicht aus der Himmelsaufnahme

Die nachfolgende Serie `artifacts/bounded-water-20261126-194918` bestand die Laufzeitprüfungen, scheiterte aber an der Bildprüfung der dunklen Nacht: Ohne Lampe betrug das 99,9-%-Perzentil 16 statt höchstens 2. Sonne und Mond lagen dabei bei −19,447° beziehungsweise −6,679° unter dem Horizont. Ein bloss erfolgreicher Smoke-Test genügte also nicht als Nachweis.

Die getrennten Lichtkontrollen unter `artifacts/bounded-night-isolation` zeigen: Ohne direktes Licht bleibt das Perzentil bei 16; ohne globale indirekte Beleuchtung liegt es bei 12; ohne Skylight fällt es auf 1. Das Schwärzen der unteren Himmelshalbkugel brachte keine Verbesserung und wurde zurückgenommen. Das Ausblenden aller Himmelsmeshes führte zu null. Die anschliessende Trennung unter `artifacts/bounded-night-layers` identifiziert den galaktischen Hintergrund (`DeepSkyComponent`): Ohne diesen fällt die Szene auf null; das einzelne Ausblenden von Sternen oder Bewegungs-Proxy lässt das Restlicht bestehen.

Die erste Korrektur schloss `DeepSkyComponent` aus Echtzeitaufnahmen aus. Der Lauf `artifacts/bounded-water-20261126-201138` bestand zwölf Laufzeitprüfungen, fiel aber bei der Luft-Bildkontrolle mit einer RGB-Abweichung von 3,930 durch. Zusätzlich zeigte die tatsächliche Aufnahme Unreals Warnung über ein fehlendes Himmelsmesh in der Lichtaufnahme. Dieser Ansatz wurde verworfen: Die Komponente enthält neben der galaktischen Textur auch `SkyAtmosphereViewLuminance` und die Sonnenscheibe. Sie ist deshalb kein unabhängiger galaktischer Hintergrund.

Die gezielte Korrektur erhält das Himmelsmesh in der Echtzeitaufnahme und setzt nur dessen Materialparameter `Background Intensity` auf null. Die unkalibrierte Milchstrassen-Textur des Plugin-Defaults (Intensität 0,8) wird vorerst nicht angezeigt. Atmosphäre, Sonne, Mond und der Sternkatalog bleiben erhalten. Das verändert weder Sternpositionen noch Mondphase, Belichtung oder Lampenleistung. Eine photometrisch kalibrierte Rückkehr der Milchstrassen-Textur bleibt offen; das Abschalten dieser Textur ist keine vollständige Simulation des natürlichen Nachthimmels.

Die fokussierte Nachtkontrolle `artifacts/bounded-night-background-zero.log` erreicht ohne Lampe einen Mittelwert und ein 99,9-%-Perzentil von null. Die abschliessende Gesamtprüfung `artifacts/bounded-water-20261126-202813` verwendet denselben korrigierten Quellstand und besteht alle zwölf Fälle mit 33 Aufnahmen. Der unveränderte Luftvergleich erreicht 0.846 (Grenze < 2), die Nacht ohne Lampe ein 99,9-%-Perzentil von 0 (Grenze ≤ 2). Mit Lampe sind 276573 Pixel im Prüfbereich deutlich beleuchtet. Der mondbeleuchtete Gegenfall erreicht bei einer Mondhöhe von 56.795° und 0.092388 Lux einen Szenenmittelwert von 20.012; die echte Mondbeleuchtung bleibt damit wirksam.

Die abschliessende Nacht-Himmelsaufnahme wurde ebenfalls angesehen: Die Warnung über ein fehlendes Himmelsmesh ist verschwunden. Das Bild ist bei der aktuellen Belichtung fast schwarz und liefert noch keinen Nachweis für eine überzeugende Sternenhimmel-Darstellung. Diese Prüfung gilt der Lichtkopplung, nicht der vollständigen visuellen Nachthimmel-Abnahme.

Paketbau, Leistungsmessung und Langzeitstabilität dieses Modus sind noch nicht belegt. Die Menü-/HUD-/FPV-Überarbeitung folgt weiterhin erst nach den beiden Grafikphasen.

## Kameralicht nach den Material- und Pflanzenkorrekturen

Der erneute Ausgangslauf `artifacts/bounded-water-20261127-001718` verwendet die korrigierten Scanfarben und die neuen Pflanzenformen. Er besteht zwölf Fälle mit 33 Bildern. Der Luftausschnitt erreicht eine mittlere RGB-Abweichung von 0,946; die dunkle Nacht ohne Lampe bleibt bei null. Diese aktualisierte Ausgangslage ist lokal in `Results.json` gebunden; der historische Prüfbericht oben bleibt erhalten.

Die anschliessende Isolation unter `artifacts/bounded-water-no-camera-scatter` setzt nur `ViewScatterGain` auf null. Drei feste Bodenkameras und drei Ansichten zur Sonne halten Geometrie, Beleuchtung und Wasserparameter gleich. Der mittlere Bildunterschied beträgt am Boden 2,168–2,825 auf der 8-Bit-RGB-Skala und in den Sonnenansichten 0,837–3,469. Das belegt einen kleinen zusätzlichen Lichtanteil, keine umfassende Behebung der flachen Grafik. Die zeitliche Rekonstruktion und bewegte Beleuchtung verursachen ebenfalls geringe Unterschiede zwischen Läufen.

`UnderwaterView.ush` gibt deshalb im begrenzten Volumenmodus das native Ergebnis direkt weiter. Der unverschattete Zusatz entfällt, ohne die integrierte Trübung wieder mit einer nachträglichen Absorption zu multiplizieren. Der normale Spielpfad und der Ersatzmodus ohne volumetrischen Nebel behalten ihre bisherigen Berechnungen. Die Korrektur ist weiterhin auf den bestehenden Grafikprototyp beschränkt.

Gleiche Bodenkamera vor der Korrektur:

![Begrenztes Wasservolumen mit zusätzlichem Kameralicht](Images/BoundedCamera_Before.png)

Nach der Korrektur:

![Begrenztes Wasservolumen ohne zusätzliches Kameralicht](Images/BoundedCamera_After.png)

Die kleine Helligkeits- und Farbänderung erfüllt die Referenzqualität noch nicht. Insbesondere bleiben das weiche Sichtfenster zur Oberfläche, die skalare statt spektraler Extinktion sowie die wenig differenzierte Lichtverteilung offen.

Der korrigierte Lauf `artifacts/bounded-water-20261127-002802` besteht erneut zwölf Fälle mit 33 Bildern. Der [aktuelle Prüfbericht](BOUNDED_CAMERA_RESULTS.json) bindet Materialbau, Quellen, Bilder, Laufzeitprotokolle und den vorher validierten Ausgangslauf. Zwischen beiden Serien ändern sich unter den erfassten Render-Eingaben ausschliesslich der Kamerashader und sein Unreal-Material. Alle Vergleichsposen stimmen überein. Der Luftausschnitt bleibt mit 0,962 unter der Grenze von 2; die Nacht ohne Lampe erreicht weiterhin null. Mit Lampe sind 255.488 Pixel im Prüfbereich beleuchtet. Der Mondlichtfall bleibt wirksam, und der Ersatzmodus ohne volumetrischen Nebel besteht. Die bekannten Editor-Plugin-Startfehler werden im Bericht weiterhin separat erfasst.

Die unveränderten analytischen Kontrollfälle unterscheiden sich zwischen den Serien im Bildmittel um 0,20–0,45 RGB-Stufen. Die drei korrigierten Bodenkameras liegen um 2,16–2,83 Stufen vom vorherigen Volumenpfad entfernt. Diese Werte quantifizieren den kleinen Eingriff und die zeitliche Renderstreuung; sie sind keine visuelle Qualitätsnote. Paketleistung und Langzeitstabilität des Volumenpfads sind weiterhin nicht gemessen. Die normale spielbare Vorschau bleibt `WindowsPlantGrowth`.

Reproduktion der isolierten Bodenkameras: `Tools/SmokeTest.ps1 -EditorSandbox -Scenario rocks -HabitatView -Entry 2 -WaterInterface -BoundedWater -ViewScatterGain 0 -Hour 12 -Date 2026-06-21 -MaxFPS 30`. Für die Sonnenansichten kommt `-HabitatSunView` hinzu. Die Isolation verwendet den vorherigen Shaderstand; nach der Korrektur hat `ViewScatterGain` im begrenzten Volumenmodus keinen Einfluss mehr.

Reproduktion der abschliessenden Serie: `Tools/Build.ps1 -EditorSandbox -Target Content -OnlyOptics`, danach `Tools/TestBoundedWater.ps1`. Den vollständigen `Saved/Logs/ContentBuild.log` aus der Editor-Sandbox vor einem weiteren Import sichern. Der Bericht verwendet `Tools/ReportBoundedWater.py <Laufordner> --skip-published --build-log <gesicherter ContentBuild.log> --prior-report <vorheriger Results.json> --output Documentation/BOUNDED_CAMERA_RESULTS.json`; die beiden veröffentlichten Bodenkopien werden ebenfalls per Hash geprüft.
