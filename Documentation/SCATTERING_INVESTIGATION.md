# Untersuchung der Unterwasserbeleuchtung

Stand: 26. November 2026. Die [visuelle Referenz](VISUAL_TARGET.md) ist weiterhin nicht erreicht. Diese Untersuchung liefert Vergleichsbilder und eine reproduzierbare Trennung zweier Aufhellungsanteile. Sie enthält **keine neue freigegebene Beleuchtung und kein neues Spielpaket**. Der damals zuletzt geprüfte Spielstand war `WindowsRockAprons`; neuere Spielpakete stehen im [README](../README.md).

## Fragestellung und Versuch

Die Unterwasserwelt wirkt trotz geometrischer Details flach. Neben der nativen volumetrischen Beleuchtung existieren zwei gestaltete, gleichmäßige Aufhellungen: die Nebelfarbe in `DiveWorld::UpdateAtmosphere` und der additive Kameraterm in `UnderwaterView.ush`. Beide hängen von der Tageslicht-/Tiefenfunktion ab, berücksichtigen in ihrem jeweiligen Zusatzterm aber keine örtlichen Felsschatten.

Diagnoseschalter skalieren diese beiden Zusätze unabhängig zwischen null und eins. Sie werden nur mit `-DiveSmokeTest` ausgewertet. Ohne Schalter gelten unverändert die bisherigen Faktoren eins/eins. Die physikalischen Spielwerte, Sichtweite, direkte Beleuchtung, Belichtungsfunktion, Absorption, Nebeldichte, Sedimentbeleuchtung und Lampeneinstellungen werden nicht verändert. Insbesondere bedeutet Faktor null **nicht**, dass das gesamte Wasser oder die native volumetrische Streuung abgeschaltet ist.

Vier native Unreal-Läufe verwenden Lediwracks, 12 Uhr, 1600 × 900, eine Begrenzung auf 30 FPS und die drei vorhandenen Habitatkameras ohne Lampe. Der Bericht prüft identische protokollierte Kamerapositionen/-richtungen und Tageslichtwerte. Die Basis verwendet ausdrücklich keine neuen Schalter. Alle vier Läufe bestehen die vorhandenen Material-/Mediumprüfungen und liefern zusammen zwölf frische Aufnahmen. C++-Build: erfolgreich. Dies ist kein Paket-, Nacht-, Wasserlinien- oder Leistungstest.

## Beobachtung

In der dritten Kamerarichtung ergibt sich folgende Helligkeitsänderung. Die Werte sind aus dem angezeigten RGB-Bild abgeleitet, auf einer Skala von 0 bis 255; sie messen weder physikalische Strahldichte noch grafische Qualität. Bewegte Fische und temporale Renderhistorie können zusätzlich kleine Unterschiede verursachen.

| Variante | Nebelzusatz | Kamerazusatz | Mittlere Bildhelligkeit | 10. Perzentil | 90. Perzentil |
| --- | ---: | ---: | ---: | ---: | ---: |
| Bisheriger Standard | 1 | 1 | 52,06 | 33,26 | 78,97 |
| Ohne Nebelzusatz | 0 | 1 | 48,22 | 30,96 | 72,79 |
| Ohne Kamerazusatz | 1 | 0 | 34,64 | 3,93 | 72,21 |
| Ohne beide Zusätze | 0 | 0 | 29,61 | 2,22 | 64,92 |

Der Kamerazusatz hat in dieser Szene den stärkeren Einfluss auf die Aufhellung dunkler und entfernter Bereiche. Ohne ihn werden Felsschatten deutlicher, zugleich fällt der Hintergrund weitgehend ins Dunkle. Das vollständige Entfernen beider Zusätze erzeugt keine überzeugende Ersatzdarstellung für gestreutes Sonnenlicht. Deshalb werden die Nullwerte nicht als neues Grafikprofil übernommen. Die Aufnahmen blicken auf den Boden; sie beweisen nicht, dass Lichtstrahlen aus anderen Blickrichtungen grundsätzlich fehlen.

Bisheriger Standard:

![Native Habitatkamera mit bisherigen Streulichtzusätzen](Images/Scattering_Baseline.png)

Beide Zusätze entfernt, ausschließlich zur Diagnose:

![Identische Kamera ohne die beiden additiven Zusätze](Images/Scattering_NoAdditions.png)

## Technischer Befund und nächste Arbeit

Der lokal installierte UE-5.8-Quellcode zeigt eine relevante Grenze: `VolumetricFogLightFunction.usf` wertet die Sonnen-Lichtfunktion auf einer zweidimensionalen Projektionsfläche aus. `VolumetricFog.usf` liest diese Textur für die Volumenbeleuchtung. Unsere `SolarCaustics.ush` verwendet dagegen die tatsächliche Weltposition samt Tiefe für Brechung, Bündelung und Abschwächung. Die dreidimensionale Tiefenabhängigkeit dieser Funktion wird durch eine einzelne Projektion nicht vollständig abgebildet. Wie stark das konkrete Bild davon betroffen ist, wurde noch nicht durch eine isolierte GPU-Pass-Aufnahme bestimmt; es ist kein Nachweis, dass alle Sonnenlichtfunktionen im Volumen wirkungslos sind.

Die separate Shaderpermutation ist im lokalen Renderer als `LIGHT_TYPE == 3` für die gerichtete Volumenlichtfunktion erkennbar. Ein möglicher Prototyp muss die Bedeutung der Projektionsfläche ausdrücklich berücksichtigen. Eine an der Kameratiefe ausgewertete zweidimensionale Maske wäre lediglich eine Näherung und darf nicht als vollständige Lichtausbreitung unter Wasser bezeichnet werden. Es wurden keine Engine-Dateien geändert.

Als Nächstes ist die räumliche Verteilung des Sonnenstreulichts samt Wassergrenze und Felsschatten zu untersuchen, einschließlich eines Blicks in Sonnenrichtung. Eine gemeinsame Änderung der Nebel-Albedo zur Nachbildung von Sonnenmustern würde auch die Tauchlampe verändern; ein emissiver Ersatz wäre ohne zusätzliche Behandlung nicht korrekt beschattet. Deshalb genügt weder ein stärkeres Farboverlay noch bloßes Abdunkeln. Nach einer tatsächlichen Renderingänderung sind Sonnen-/Schattenfälle, Lampe/Nacht, unterschiedliche Sichtweiten, Wasserübergang und Paketleistung erneut zu prüfen.

Unabhängig davon bleiben die gleichförmigen Bodenflächen, sichtbare Wiederholungen der Felsformen, Bewuchs und Materialdetails offen. Menü/HUD/FPV folgen weiterhin erst nach Unterwasser- und Überwassergrafik gemäß [UI_UX_TARGET.md](UI_UX_TARGET.md).

## Reproduktion und Belege

`Tools/Build.ps1 -EditorSandbox` baut das Modul. Danach erzeugt `Tools/TestScattering.ps1` die vier Vergleiche. `Tools/ReportScattering.py <Laufverzeichnis> --build-log <Buildprotokoll>` prüft Modul-/Quellprüfsummen, Kamera-/Lichtgleichheit, Laufzeitprüfungen und Bildgrößen; die beiden veröffentlichten Bilder müssen mit Basis und Nullvariante der dritten Kamera übereinstimmen. Die Teststeuerung prüft ausdrücklich, dass angeforderte Faktoren vom Runtime-Modul übernommen wurden.

Aktueller Lauf: `artifacts/scattering-validation-20261126-183239`. Der [gebundene Diagnosebericht](SCATTERING_DIAGNOSIS.json) enthält zwölf Bilder, vier Laufzeitprotokolle, Modul- und Quellprüfsummen sowie die veröffentlichten Vergleichsbilder. Die visuellen Schlussfolgerungen oben beruhen auf Sichtprüfung; der Status `PASS` bestätigt die Vergleichsbedingungen und technischen Prüfungen, nicht die Erfüllung der Qualitätsreferenz.
