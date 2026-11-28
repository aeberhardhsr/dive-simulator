# Lichtverteilung im aktuellen Wasservolumen

**Ergebnis: Die neue Abstimmung wird nicht übernommen.** Die Spielvorschau bleibt `Builds/WindowsPlantCover/Start-WaterPreview.cmd`. Der Versuch erzeugt blauere Wasserbereiche, löst aber weder die flache Gesamtwirkung noch die wenig überzeugende Wasserunterseite. Das [visuelle Qualitätsziel](VISUAL_TARGET.md) ist weiterhin offen.

## Kontrollierte Vergleiche

Vier Läufe des unveränderten Spielpakets isolieren bei Weesen/West den Einfluss der Lichtquellen: Mittag, Mittag ohne Himmelslicht, Mittag ohne direktes Licht und Morgen um 07:30 Uhr. Alle verwenden dieselben drei niedrigen Habitatkameras, 14 Meter Sichtweite und das Wasser-Vorschauprofil. Ohne Himmelslicht wird die Ansicht dunkler und brauner; dessen vollständiges Abschalten ist keine geeignete Verbesserung. Der Morgen zeigt bereits deutlich längere Felsschatten. Diese Kontrollschalter sind Diagnosen, keine alternative Spielbeleuchtung.

Anschliessend vergleichen sechs native Editor-Spielstarts den bisherigen Zustand mit einer kombinierten Änderung: Streualbedo `(0,07; 0,24; 0,16)` → `(0,09; 0,32; 0,30)` und Phasenparameter `0,55` → `0,75`. Letzterer verstärkt die Vorwärtsrichtung der Streuung. Unreals gemeinsamer Phasenparameter wurde anhand der Kameratiefe eingeblendet; das ist keine räumlich getrennte Phasenfunktion für Luft, Wasser und Sediment. Die Koeffizienten sind gestalterische Versuchsparameter, keine Messwerte des Walensees.

Sonnenleistung, Belichtung, Sichtweite, Oberflächen und Geometrie bleiben innerhalb jedes Vergleichspaars gleich. Die drei Fälle sind Mittagsblick auf den Grund, drei Sonnenrichtungen und Morgenblick auf den Grund. Die protokollierten Kameraposen stimmen paarweise exakt überein. Die zwei Parameter wurden gemeinsam verändert; ihre Einzelwirkungen sind damit nicht getrennt nachgewiesen.

## Sichtbefund

Am Grund vorher:

![Bisherige Lichtwirkung am Grund](Images/WaterScattering_Bed_Before.png)

Geprüfte Abstimmung:

![Blauere Streuung ohne ausreichende räumliche Verbesserung](Images/WaterScattering_Bed_Study.png)

In Richtung Sonne vorher:

![Bisherige Streuung in Sonnenrichtung](Images/WaterScattering_Sun_Before.png)

Geprüfte Abstimmung:

![Geänderte Streuung in Sonnenrichtung](Images/WaterScattering_Sun_Study.png)

Die Farbverschiebung ist sichtbar, die Lichtstruktur bleibt breit und weich. Die Wasserunterseite wirkt weiterhin wie eine zusammenhängende reflektierende Fläche über der Szene. Der Blick direkt zur Sonne zeigt einen sehr deutlich begrenzten Durchsichtbereich. Der [Morgenvergleich vorher](Images/WaterScattering_Morning_Before.png) und [mit Versuch](Images/WaterScattering_Morning_Study.png) bestätigt die begrenzte Wirkung. Eine andere Färbung ersetzt weder natürliche Fels-/Bodenübergänge noch einen überzeugenden Lichtweg durch Wasser und Grenzfläche.

## Nachweis und Rücknahme

`artifacts/light-balance-20261127` enthält vier Paketfälle mit zwölf Bildern; `artifacts/water-scattering-balance-20261127-064936` sechs native Fälle mit 18 Bildern. Der [Ergebnisbericht](WATER_SCATTERING_BALANCE_RESULTS.json) bindet Eingaben, Modul, Paketdateien, Protokolle, Kameras und unveränderte veröffentlichte PNG-Kopien per SHA-256. Bekannte Startfehler der Editor-Plugins sind enthalten. Der erste C++-Build scheiterte an einem vor seiner Deklaration verwendeten lokalen Zeiger; der korrigierte Build und der Materialbau bestehen.

Die Änderung samt Teststeuerung ist als [Patch](Experiments/water-scattering-balance.patch) archiviert. Reproduktion auf dem dokumentierten Ausgangsstand: Patch anwenden, `Tools/Build.ps1 -EditorSandbox`, anschliessend `Tools/Build.ps1 -EditorSandbox -Target Content -OnlyWater` und `Tools/TestWaterScatteringBalance.ps1 -Preview` ausführen. Der Materialbau erzeugt auch die Wasseroberflächen neu; deren unveränderte Originaldateien wurden im dokumentierten Versuch vor den Bildläufen wiederhergestellt. Kein neues Spielpaket wurde gebaut. Nacht, Sturm, Wasserübergang, Leistung und Langzeitstabilität des Kandidaten wurden nicht geprüft.

Die sieben gesicherten Quell-/Assetdateien sind bytegenau wiederhergestellt; der zusätzliche Teststarter ist aus dem aktiven Werkzeugbestand entfernt. Der [Wiederherstellungsnachweis](WATER_SCATTERING_RESTORATION.json) bestätigt den erfolgreichen Editor-Build und einen Kontrolllauf mit drei Bildern. Die mittlere RGB-Abweichung von der nativen Ausgangslage liegt bei 0,29–0,37 auf der Skala 0–255; temporale Rekonstruktion und bewegte Szene verhindern Pixelidentität. Der aktive Spielcode und die Inhaltsassets haben keinen Git-Unterschied zum Ausgangsstand.

## Konsequenz

Weitere Wasseroptik muss reflektiertes Oberflächenlicht und bereits integriertes Volumenlicht unterscheiden. Ein pauschaler Farb- oder Helligkeitsfaktor behebt diese Grenze nicht. Im lokalen UE-Quellcode ist das integrierte Volumenlicht über `FViewInfo::VolumetricFogResources.IntegratedLightScatteringTexture` zugänglich. Eine eigene Rendering-Erweiterung wäre ein zu prüfender Ansatz; sie ist hier weder implementiert noch als korrekter spektraler Transport nachgewiesen. Geometrie, Bewuchs und Bodenanschlüsse bleiben eigenständige Aufgaben. Danach folgen Überwasserwelt und [Menü/HUD/FPV](UI_UX_TARGET.md).
