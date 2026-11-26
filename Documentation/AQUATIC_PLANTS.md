# Unterwasserbewuchs: Formen und Oberflächen

Die Nutzerreferenz verlangt räumlich glaubwürdigen Bewuchs als Teil einer zusammenhängenden Unterwasserwelt. Die bisherigen breiten Blätter wirkten in echten Spielaufnahmen kantig und gleichmäßig grün. Der neue Entwicklungsstand ersetzt ihre flachen Stängelbänder durch räumliche, gekrümmte Stängel und detailliertere Blätter. Die fein verzweigten Gruppen erhalten weichere Verzweigungen und differenzierte Oberflächen. Die Gesamtqualität der Referenz ist damit noch nicht erreicht.

## Modelle und Material

`Tools/create_aquatic_plants.py` erzeugt drei eigene parametrische Modelle. `blender/AquaticPlants.blend`, die FBX-Dateien und das Manifest unter `blender/exports/Plants/` halten den Stand fest. Die älteren Modelle bleiben für den kontrollierten Vorher-/Nachher-Vergleich erhalten. Die neuen Modelle verwenden getrennte Unreal-Assets unter `/Game/Diving/Plants/`.

Laichkraut erhält gekrümmte Blattflächen mit einer ausgeformten Mittelrippe und unregelmäßigen Blattorientierungen. Armleuchteralgen und Tausendblatt besitzen räumliche Verzweigungen und variierende Stängelhöhen. Eine erste Spielprüfung zeigte die feinen Gruppen zu dünn; der überarbeitete Stand erhält ihre Gruppendichte und stärkt die sichtbaren Verzweigungen. Es handelt sich um gestaltete Modelle, nicht um Scans örtlicher Exemplare.

Das gemeinsame Material `M_AquaticPlant` nutzt blattbezogene Koordinaten für Adern, kleine Oberflächenunterschiede, Farbvariation und Rauheit. Die Muster bleiben beim Bewegen an der Pflanze. Feine Details werden bei zu geringer Bildschirmgröße abgeschwächt. Ein zweiseitiges Vegetationsmaterial berücksichtigt beleuchtete Vorder- und Rückseiten. Die Bewegung bleibt eine begrenzte, am Boden verankerte Spielanimation; sie ist keine hydrodynamische Berechnung.

Vier Detailstufen begrenzen den Aufwand in größerer Entfernung. Die Importprüfung kontrolliert Quelldreiecke, gespeicherte Farbgewichte und die zwei Koordinatenkanäle der Quelle. Im nativen Spiel werden die tatsächlichen Vertexdaten aller vier Stufen auf Farbgewichte und zwei Koordinatenkanäle geprüft. Die Editorfunktion `GetNumUVChannels` liest die Quell-MeshDescriptions; automatisch reduzierte Detailstufen besitzen diese nicht. Eine dort gemeldete Null ist daher kein Nachweis fehlender gerenderter Koordinaten.

## Einordnung und Quellen

Die bestehenden Gruppenbezeichnungen bleiben unverändert: [Potamogeton](https://www.infoflora.ch/de/flora/potamogeton.html), [Characeae](https://www.infoflora.ch/de/flora/armleuchteralgen.html) und [Myriophyllum](https://www.infoflora.ch/de/flora/myriophyllum.html). Diese Quellen ordnen die Gruppen ein; sie beweisen weder eine exakte Artbestimmung der Spielmodelle noch deren örtliche Verteilung. Beispielsweise unterscheiden sich die beschriebenen Blattformen einzelner Laichkrautarten erheblich; die generische Spielgruppe wird nicht als naturgetreue Darstellung einer bestimmten Art ausgegeben. Referenzfotos wurden nicht in Spieltexturen übernommen.

## Prüfung und offener Umfang

`Tools/SmokeTest.ps1 -EditorSandbox -Scenario rocks -PlantView` vergleicht alle drei Gruppen mit ihren Vorgängern aus zwei festen Blickwinkeln: zwölf echte Spielaufnahmen. Der nahe Blick zeigt Detailausschnitte, der weitere Blick die ganze Gruppe. Die gewöhnliche Umgebung und die aktuelle Beleuchtung bleiben aktiv; die Pflanzen bewegen sich auch während dieser Bildfolge. Ein zusätzliches `-GravelView` zeigt ihre Einbindung in eine natürlich bestückte Stelle.

Die erste Integration bestand den nativen Streaming- und Entdeckungsablauf. Die verfeinerten Formen bestanden anschließend den Vergleich aller drei Gruppen und die Einbindung in die Kies-Szene: zwölf Modellaufnahmen und vier Szenenaufnahmen unter `artifacts/aquatic-plant-refined/`. Dabei sind die breiten Blätter deutlich weicher gegliedert; die zuerst zu dünnen Verzweigungen bleiben nach der Überarbeitung besser sichtbar. Eine neue Paketprüfung einschließlich aller drei Einstiege, der Arten-/Fotosammlung und eines Leistungsvergleichs steht noch aus. Diese Entwicklungsversion ist noch nicht als neues Spielpaket freigegeben.

Offen bleiben insbesondere stärker zusammenhängender Bewuchs, Übergänge zu Belägen auf Felsen und Sediment, die gemeinsame Lichtwirkung sowie die abschließende anatomische und visuelle Abnahme. Das Projektziel umfasst weiterhin die vollständige Unterwasserwelt, danach die Landschaft über Wasser und anschließend die dokumentierte Menü-/HUD-Überarbeitung.
