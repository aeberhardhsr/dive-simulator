# Gebogene Triebe und dichtere Blattbereiche

Die bisherigen Pflanzen wirkten in den geprüften Nahansichten wie dünne Drahtbäume: weitgehend gerade Stängel, regelmässige Astetagen und lange kahle Spitzen. Die drei allgemeinen Pflanzengruppen erhalten jetzt gebogene Triebe, ungleiche Abstände zwischen Blattansätzen und mehr Blattfläche im oberen Bereich. Stängel und grünes Gewebe bekommen getrennte, zurückhaltendere Farbtöne. Die bestehende Gruppenplatzierung bleibt erhalten.

Laichkraut trägt zwölf statt acht Blattansätze je Trieb; kleinere obere Blätter reichen bis an die Spitze. Armleuchteralgen verwenden unterschiedlich viele Seitenstrahlen und kurze Verzweigungen an mehreren Stellen statt gleicher gabelförmiger Enden. Tausendblatt behält vier fein gefiederte Blätter je Quirl, mit grösseren Fiederflächen und unregelmässiger Neigung. Die Blattquirle und gegenständigen Abschnitte sind als grobe Formreferenz beim [Naturhistorischen Museum Wien](https://flora.nhm-wien.ac.at/Seiten-Arten/Myriophyllum-spicatum.htm) beschrieben. Daraus wird keine genaue Artenrekonstruktion oder nachgewiesene lokale Verteilung abgeleitet; sämtliche Geometrie ist eigenständig erzeugt.

Die Farbwerte sind gestaltete lineare Reflexionswerte. Materialgraph, Lichtdurchlässigkeit und Bewegungsmodell behalten ihr bisheriges Verfahren. Die Höhenbegrenzung hält auch seitliche Triebe innerhalb der jeweiligen Habitat-Höhe. Bodenpositionen, Artengewichte und Felsabstände ändern sich nicht.

## Geometrie und Prüfung

| Gruppe | Bisher LOD0 | Neu LOD0 | Weitere Detailstufen | Modellhöhe |
| --- | ---: | ---: | --- | ---: |
| Laichkraut | 7.224 | 10.136 | 4.562 / 1.621 / 557 | 1,031 m |
| Armleuchteralgen | 27.744 | 48.366 | 21.764 / 7.738 / 2.660 | 0,439 m |
| Tausendblatt | 50.456 | 67.032 | 30.165 / 10.726 / 3.686 | 0,867 m |

Der erste Armleuchteralgen-Entwurf benötigte 100.320 Dreiecke und überschritt mit 0,471 m die vorgesehene Höhe von 0,45 m. Er wurde vor dem Bildvergleich überarbeitet: weniger Haupttriebe und Blattetagen sowie vereinfachte kleine Verzweigungen. Die endgültigen Formen erfüllen die Höhenbegrenzung. Blender prüft endliche Koordinaten und entartete Flächen; der Unreal-Import prüft Dreieckzahlen, beide UV-Kanäle, Vertexfarben und vier abnehmende Detailstufen.

Je zwölf native Modellaufnahmen vor und nach der Änderung zeigen die alten primitiven Vergleichsmodelle und die jeweils aktuellen Formen. Die veröffentlichten Paare vergleichen ausdrücklich die zuvor ausgelieferten detaillierten Modelle mit den neuen Modellen, nicht mit den primitiven Platzhaltern. Eine zusätzliche feste Weesener Umgebungskamera besteht die Platzierungs- und Nachladeprüfung mit weiterhin 2.171 Pflanzen, 840 niedrigen und 297 hohen Exemplaren.

Laichkraut vorher:

![Vorherige detaillierte Form](Images/PlantGrowth_Pondweed_before.png)

Laichkraut mit gebogenen Trieben und mehr Blattfläche:

![Neue detaillierte Form](Images/PlantGrowth_Pondweed_after.png)

Weitere Originalaufnahmen: Armleuchteralgen [vorher](Images/PlantGrowth_Stonewort_before.png) / [nachher](Images/PlantGrowth_Stonewort_after.png), Tausendblatt [vorher](Images/PlantGrowth_Milfoil_before.png) / [nachher](Images/PlantGrowth_Milfoil_after.png). Diese Bilder stammen aus der nativen Unreal-Laufzeit; zusätzliche Aufnahmen aus dem separat geprüften Paket sind im Paketbericht erfasst. Sie sind keine nachbearbeiteten Konzeptbilder.

## Spielbares Paket und Nachweise

`Builds/WindowsPlantGrowth/divingsim.exe` enthält den Asset- und Quellstand `6759dc0ead00034915c34a837059cf609328dc98`. Der C++-Spielcode bleibt unverändert; deshalb wird die bereits kompilierte Spiel-EXE aus dem vorherigen Build-Slot verwendet (SHA-256 `3beaee712b3092082996807b822f2f9e7e60dfcf999c596cc3cf49d064ce2e13`). Die neu erzeugten Inhaltscontainer enthalten die geänderten Pflanzen. Identische EXE-Prüfsummen allein wären hier kein Nachweis desselben Spiels; die Berichte binden ausdrücklich auch die Container und Assets.

Der Lauf `artifacts/plant-beds-20261127-000217` besteht neun Fälle mit 43 Aufnahmen: zwei Verteilungszustände mit den neuen Formen, Murg, Lediwracks, Ersatzdarstellung ohne Nanite, Schwimmen/Nachladen, Uferablauf, Fotoentdeckungen sowie zwölf Modellansichten. Der normale Launcher-Start besteht ebenfalls. Die zwei Verteilungszustände im Paket sind kein Vergleich alter und neuer Geometrie; dafür dienen die oben getrennt gesicherten nativen Modellpaare.

Beide Quellprüfungen bestehen: [Push](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45908192593), [Pull Request](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45908197909). Reproduktion: `Tools/TestPlantBeds.ps1 -Packaged -PlantForms -PackageDirectory Builds/WindowsPlantGrowth` und `Tools/TestPackageLauncher.ps1` mit demselben Paket. [Paketbericht](PLANT_GROWTH_PACKAGE_RESULTS.json), [Modellquellen und Vergleichsnachweis](PLANT_GROWTH_RESULTS.json).

Der 60-Sekunden-Unterwasserlauf bei 1080p/Hoch erreicht 72,97 FPS im Mittel und 61,37 FPS im 1%-Low, ohne Bilder über 33,33 ms. Die GPU-Zeit steigt gegenüber den älteren Modellen in der vorherigen Vorschau um rund 0,33 ms auf 13,09 ms; die Spielthreadzeit bleibt mit 3,50 ms nahezu gleich. Die 90-Sekunden-Nachladestrecke erreicht 100,50 FPS und enthält einen Ausreisser bis 45,07 ms. [Vollständige Messbedingungen und Grenzen](PERFORMANCE.md).

## Verbleibende Qualität

Die Formen wirken dichter und weniger starr, behalten aber eine erkennbare prozedurale Struktur. Feine Blattteile sind in grösserer Entfernung weiterhin sehr klein auf dem Bildschirm; die vier Detailstufen sind automatisch reduziert. Pflanzenoberflächen, Übergänge zum Sediment und die Lichtwirkung bleiben weitere Arbeit für die [visuelle Referenz](VISUAL_TARGET.md). Die Unterwasserwelt ist damit noch nicht vollständig abgenommen; danach folgen Überwasserlandschaft und [Menü/HUD/FPV](UI_UX_TARGET.md).
