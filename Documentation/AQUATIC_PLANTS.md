# Unterwasserbewuchs: Formen und Oberflächen

## Weiterentwicklung der feinen Blattstruktur

Der nächste Stand ersetzt beim Tausendblatt die röhrenförmigen Blattfiedern durch schmale, gebogene, zweiseitig schattierte Blattflächen mit einer flachen Mittelrippe. Die Blattquirle stehen zum Triebende dichter; zwei kleinere Quirle schließen die vorher kahlen Spitzen. Pro Blatt entstehen sechs bis zehn unterschiedlich lange Fiederpaare. Die Armleuchteralgen behalten ihre räumlichen Röhren; das Material unterscheidet nun zwischen Stängeln, lichtdurchlässigeren grünen Seitenzweigen und Blattflächen. Nur Blattflächen erhalten die Blattadern. Pflanzendichte, Habitatverteilung und bestehende Bewegung bleiben unverändert.

Die höchste Tausendblatt-Detailstufe steigt von 45.304 auf 50.456 Dreiecke. Ihre weiteren Stufen besitzen 22.706, 8.073 und 2.776 Dreiecke. Laichkraut und Armleuchteralgen behalten ihre bisherigen Dreieckszahlen. Die zwölf nativen Vergleichsaufnahmen bestehen; Nah- und Gesamtansicht zeigen die neue Struktur.

Das eigenständige Paket `Builds/WindowsPlantCanopy/divingsim.exe` aus Quellstand `68b9022` besteht fünf Paketfälle mit 23 frischen Bildern: zwölf Modellansichten, je drei Habitatansichten an allen drei Einstiegen sowie zwei Bilder aus der Schwimm-/Nachladeprüfung. Zusätzlich besteht der normale Launcher-Start. Die Habitat- und Nachladefälle prüfen die tatsächlichen Farb- und Koordinatenpuffer aller vier Pflanzen-Detailstufen, Populationsgrenzen und das gestaffelte Nachladen einschließlich Teleport-Nähe. `Tools/TestPlantCanopy.ps1` wiederholt die Prüfung. Die Quellprüfung auf [GitHub](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45848799284) besteht ebenfalls.

Der zugehörige 60-Sekunden-Unterwasserlauf bei 1080p/Hoch auf RTX 3090 / Ryzen 9 9950X3D erreicht 67,71 FPS im Mittel, 54,71 FPS für das langsamste Prozent und höchstens 19,32 ms pro Bild. 514 von 4.061 ausgewerteten Bildern überschreiten 16,67 ms, keines 33,33 ms. Es lief keine weitere Unreal-Instanz. Das ist keine durchgehende 60-FPS-Abnahme und kein Langzeittest. [Paket, Quelldaten, Bilder und Leistungsnachweis](PLANT_CANOPY_RESULTS.json) sind über Prüfsummen verbunden; [Messbedingungen und Grenzen](PERFORMANCE.md) bleiben maßgeblich. Der bekannte fehlende CelestialVault-Sternmaterialverweis erscheint weiterhin im Paketstartprotokoll und ist im Bericht erhalten.

Gleiche Modellkamera, bisheriger nativer Stand und neues fertiges Paket; die gewöhnliche Pflanzenbewegung bleibt aktiv:

![Bisherige röhrenförmige Blattfiedern](Images/PlantCanopy_Before.png)

![Neue schmale Blattflächen und kleinere Triebspitzen](Images/PlantCanopy_After.png)

Beim Erzeugen wurde ein Windows-/Blender-Speicherproblem erkannt: Das Ersetzen der vorhandenen `.blend` über Blenders Umbenennungs-/Backupfolge schlug fehl. Die anschließende FBX-Prüfsummenprüfung verhinderte einen Import mit veraltetem Manifest. Der Generator speichert deshalb zunächst in eine frische temporäre Datei und kopiert das geschlossene Ergebnis an den versionierten Zielpfad; das Manifest wird erst nach erfolgreichem Abschluss geschrieben. Für automatisierte Blender-Aufrufe ist `--python-exit-code 1` zu verwenden.

Die Nutzerreferenz verlangt räumlich glaubwürdigen Bewuchs als Teil einer zusammenhängenden Unterwasserwelt. Die bisherigen breiten Blätter wirkten in echten Spielaufnahmen kantig und gleichmäßig grün. Der neue Entwicklungsstand ersetzt ihre flachen Stängelbänder durch räumliche, gekrümmte Stängel und detailliertere Blätter. Die fein verzweigten Gruppen erhalten weichere Verzweigungen und differenzierte Oberflächen. Die Gesamtqualität der Referenz ist damit noch nicht erreicht.

## Bisherige Modelle und Materialgrundlage

`Tools/create_aquatic_plants.py` erzeugt drei eigene parametrische Modelle. `blender/AquaticPlants.blend`, die FBX-Dateien und das Manifest unter `blender/exports/Plants/` halten den Stand fest. Die älteren Modelle bleiben für den kontrollierten Vorher-/Nachher-Vergleich erhalten. Die neuen Modelle verwenden getrennte Unreal-Assets unter `/Game/Diving/Plants/`.

Laichkraut erhält gekrümmte Blattflächen mit einer ausgeformten Mittelrippe und unregelmäßigen Blattorientierungen. Armleuchteralgen und Tausendblatt besitzen räumliche Verzweigungen und variierende Stängelhöhen. Eine erste Spielprüfung zeigte die feinen Gruppen zu dünn; der überarbeitete Stand erhält ihre Gruppendichte und stärkt die sichtbaren Verzweigungen. Es handelt sich um gestaltete Modelle, nicht um Scans örtlicher Exemplare.

Das gemeinsame Material `M_AquaticPlant` nutzt blattbezogene Koordinaten für Adern, kleine Oberflächenunterschiede, Farbvariation und Rauheit. Die Muster bleiben beim Bewegen an der Pflanze. Feine Details werden bei zu geringer Bildschirmgröße abgeschwächt. Ein zweiseitiges Vegetationsmaterial berücksichtigt beleuchtete Vorder- und Rückseiten. Die Bewegung bleibt eine begrenzte, am Boden verankerte Spielanimation; sie ist keine hydrodynamische Berechnung.

Vier Detailstufen begrenzen den Aufwand in größerer Entfernung. Die Importprüfung kontrolliert Quelldreiecke, gespeicherte Farbgewichte und die zwei Koordinatenkanäle der Quelle. Im nativen Spiel werden die tatsächlichen Vertexdaten aller vier Stufen auf Farbgewichte und zwei Koordinatenkanäle geprüft. Die Editorfunktion `GetNumUVChannels` liest die Quell-MeshDescriptions; automatisch reduzierte Detailstufen besitzen diese nicht. Eine dort gemeldete Null ist daher kein Nachweis fehlender gerenderter Koordinaten.

## Einordnung und Quellen

Die bestehenden Gruppenbezeichnungen bleiben unverändert: [Potamogeton](https://www.infoflora.ch/de/flora/potamogeton.html), [Characeae](https://www.infoflora.ch/de/flora/armleuchteralgen.html) und [Myriophyllum](https://www.infoflora.ch/de/flora/myriophyllum.html). Diese Quellen ordnen die Gruppen ein; sie beweisen weder eine exakte Artbestimmung der Spielmodelle noch deren örtliche Verteilung. Beispielsweise unterscheiden sich die beschriebenen Blattformen einzelner Laichkrautarten erheblich; die generische Spielgruppe wird nicht als naturgetreue Darstellung einer bestimmten Art ausgegeben. Referenzfotos wurden nicht in Spieltexturen übernommen.

## Prüfung und offener Umfang

`Tools/SmokeTest.ps1 -EditorSandbox -Scenario rocks -PlantView` vergleicht alle drei Gruppen mit ihren Vorgängern aus zwei festen Blickwinkeln: zwölf echte Spielaufnahmen. Der nahe Blick zeigt Detailausschnitte, der weitere Blick die ganze Gruppe. Die gewöhnliche Umgebung und die aktuelle Beleuchtung bleiben aktiv; die Pflanzen bewegen sich auch während dieser Bildfolge. Ein zusätzliches `-GravelView` zeigt ihre Einbindung in eine natürlich bestückte Stelle.

Die erste Integration bestand den nativen Streaming- und Entdeckungsablauf. Die verfeinerten Formen bestanden anschließend den Vergleich aller drei Gruppen und die Einbindung in die Kies-Szene: zwölf Modellaufnahmen und vier Szenenaufnahmen unter `artifacts/aquatic-plant-refined/`. Dabei sind die breiten Blätter deutlich weicher gegliedert; die zuerst zu dünnen Verzweigungen bleiben nach der Überarbeitung besser sichtbar.

Das eigenständige Paket `Builds/WindowsUnderwaterDetail/divingsim.exe` enthält jetzt Felsen, Kies und alle drei neuen Pflanzengruppen. Zwölf Paketprüfungen mit 74 frischen Bildern bestehen: Modellvergleiche, Kies mit/ohne Nanite, Streaming und Uferübergänge an allen drei Einstiegen sowie die Arten-/Fotosammlung. Der normale Start über die Haupt-EXE wurde gesondert geprüft. [UNDERWATER_DETAIL_RESULTS.json](UNDERWATER_DETAIL_RESULTS.json) bindet diese Nachweise an EXE, Inhaltscontainer, Quellstand `262e1e4`, Modelle und importierte Materialien. Die portablen Prüfungen desselben Quellstands bestehen auf Windows, macOS und Linux; dies ist kein nativer Unreal-Mac-Build.

Die Paketstarts melden weiterhin das bereits vorher fehlende CelestialVault-Sternmaterial `MI_StarsStar_EnergyConservative`. Diese Meldung ist in den Ergebnisdaten erhalten. Die erfolgreichen Spielprüfungen bedeuten nicht, dass sämtliche Engine-Startmeldungen behoben sind.

Eine anschließende 60-Sekunden-Messung desselben Pakets lief ohne weitere Unreal-/Spielinstanz: RTX 3090, Ryzen 9 9950X3D, 1920×1080, Hoch, feste 100 % Renderauflösung, ohne VSync oder Bildratenlimit. Nach 15 Sekunden Aufwärmen wurden 4.691 vollständige Bildintervalle ausgewertet (59,975 s; zwei Aufzeichnungsrandbilder ausgenommen). Ergebnis: 78,22 FPS im Mittel, 67,22 FPS für das langsamste Prozent, P95 14,06 ms, Maximum 16,77 ms und kein Intervall über 33,33 ms. [UNDERWATER_DETAIL_PERFORMANCE.json](UNDERWATER_DETAIL_PERFORMANCE.json) enthält Messwerte, Hardware, Paketidentität und Rohdatenprüfsummen. Das gilt für diese Unterwasserszene, nicht für alle Orte, Einstellungen oder Hardware; ein direkter Leistungsgewinn gegenüber früheren Messungen mit anderen Hintergrundlasten wird nicht behauptet. Der zweistündige ältere Murg-Lauf deckt dieses neue Paket nicht ab.

Direkter Vergleich aus dem fertigen Paket, gleicher Standort und Kamerawinkel; die gewöhnliche Pflanzenbewegung bleibt aktiv:

![Bisherige kantige Blätter](Images/AquaticPlant_Previous.png)

![Neue gebogene Blätter und räumliche Stängel](Images/AquaticPlant_Current.png)

![Neue Pflanzen und Kies im fertigen Spielpaket](Images/Underwater_Detail_Package.png)

Offen bleiben insbesondere stärker zusammenhängender Bewuchs, Übergänge zu Belägen auf Felsen und Sediment, die gemeinsame Lichtwirkung sowie die abschließende anatomische und visuelle Abnahme. Das Projektziel umfasst weiterhin die vollständige Unterwasserwelt, danach die Landschaft über Wasser und anschließend die dokumentierte Menü-/HUD-Überarbeitung.
