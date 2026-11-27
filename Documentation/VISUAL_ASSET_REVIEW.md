# Abgleich vorhandener Assets mit dem visuellen Ziel

Stand 27. November 2026. Dieser Abgleich bereitet konkrete Grafikarbeiten während des Engine-Builds vor. Er ist keine neue Spielaufnahme oder visuelle Abnahme. Die Nutzerreferenzen und der volle Qualitätsanspruch bleiben in [VISUAL_TARGET.md](VISUAL_TARGET.md) verbindlich.

## Betrachtete Aufnahmen und Befund

Tatsächlich betrachtet wurden `Images/SolarVolume_Combined.png` (experimenteller Unterwasser-Lichtstand), `Images/Ground_Land_After.png` (Bodendetails im früheren Paket) und `Images/SurfaceNearField_After.png` (Wasserprofil im früheren Paket). Die Bilder stammen aus verschiedenen Prüfständen; sie bilden keinen gemeinsamen, identisch aufgenommenen aktuellen Build ab. Insbesondere ist aus der westwärts gerichteten Weesen-Aufnahme keine vollständige Abnahme des Bergpanoramas möglich.

| Bereich | Bereits vorhanden | Sichtbare beziehungsweise aus dem Code belegte Lücke | Nächste konkrete Arbeit |
| --- | --- | --- | --- |
| Unterwasserfels | Drei lose Scanformen und zwei kräftigere Gruppenkerne, passende PBR-Oberflächen, Geröll und Felsfuss-Ablagerungen | Einzelne Felsen sind detaillierter als die weitgehend freien, gleichförmigen Zwischenflächen; zusammenhängende Felszüge und Übergänge tragen die Szene noch nicht wie in der Referenz | Nach Prüfung des neuen Lichttransports einen vollständigen durchschwimmbaren Bereich gestalten; grosse Formen, eingebettete Blöcke, Zwischenräume und Bewuchs gemeinsam beurteilen |
| Unterwasserlicht | Sichtweg-Dämpfung und experimenteller direkter/volumetrischer Sonnenlichttransport | Der betrachtete Versuchsstand bleibt dunkel und wenig räumlich gegliedert; mehr Geometriedetail allein beseitigt diesen Eindruck nicht | Source-Renderer fertigstellen und die vorbereiteten gepaarten Tag-/Nacht-/Tiefen-/Oberflächentests tatsächlich rendern; anschliessend Licht und Habitat zusammen prüfen |
| Pflanzen | Drei modellierte Wasserpflanzentypen, Wuchsgruppen und korrigierte FBX-Farbdekodierung | Dünne, teilweise gleichförmige Bestände; empfindliche Zweige und harte Lichtunterschiede | Vorhandene Modelle in zusammenhängenden Zonen abstimmen und in Bewegung auf Detailwechsel/Flimmern prüfen; konkrete Formlücken erst danach durch neue Assets ergänzen |
| Landboden | Gras- und Waldboden-PBR, Waldmaske, Felsmaterial auf steilen Flächen | In der betrachteten Nahaufnahme bleiben unscharfe Strukturen des Luftbilds sichtbar. `GroundDetail.ush` klassifiziert Gras aus RGB-Verhältnissen und lässt neutrale Flächen beim Luftbild; die Mischung fällt zwischen 25 und 120 m zurück | Materialzuordnung gezielter machen, Neutralflächen getrennt behandeln und den Übergang anhand echter begehbarer Ansichten einstellen |
| Fels-/Geröllhänge | swissALTI3D, Fels-PBR, bereits lokal gespeicherte swissTLM3D-Bodenbedeckung | Das Material wählt Fels bisher wesentlich über Hangneigung. Kartierte Fels-/Geröllklassen werden noch nicht benutzt | Die unten vorbereiteten Klassen kontrolliert in die Materialzuordnung integrieren; keine pauschale Felsbelegung aller steilen Flächen |
| Wald und Wiesen | Fünf Baumvarianten aus vorhandenen Quellen, räumliche Platzierung, LODs und etwa 1,15 km Renderreichweite | Der betrachtete Boden bleibt zwischen Stämmen räumlich leer; weiter entfernte Wälder stammen laut Implementierung weiterhin aus dem Luftbild | Unterwuchs und bodennahes Volumen gezielt anlegen; für fernere Wälder ein zusammenhängendes Volumen-/LOD-Konzept mit gemessenen Kosten prüfen |
| Bebauung am Ufer | Geografisch platzierte Gebäudekörper | In der betrachteten Wasserlinienansicht überwiegend helle, kaum gegliederte Fassaden/Dächer | Nahe Fassaden und Dachmaterialien detaillieren; gestaltete Fenster-/Gebrauchsspuren von vermessener Gebäudegeometrie unterscheiden |

Quellinventar: [lose Felsen](ROCK_ASSETS.json), [Gruppenkerne](ROCK_FORMATION_ASSETS.json), [Felsoberflächen](ROCK_SURFACES.json), [Vegetation](VEGETATION_ASSETS.json), [Buche](VEGETATION_BEECH_SOURCE.json), [Bodenmaterialien](GROUND_MATERIALS.json). Diese vorhandenen Bibliotheken werden weiterverwendet. Die verworfenen kleinen Sedimentrücken bleiben verworfen; [deren Vergleich](SEDIMENT_BEDFORM_INVESTIGATION.md) belegt keine ausreichende Verbesserung der freien Bodenflächen.

## Neu vorbereitete kartierte Oberflächenklassen

`Tools/prepare_surface_cover.py` liest die bereits vorhandenen, per SHA-256 an [die Vegetationsprovenienz](WALENSEE_VEGETATION_SOURCE.json) gebundenen Originaldateien von swissTLM3D 2026-02. Es lädt nichts herunter und ändert keine Unreal-Inhalte, Terrainhöhen, Kollisionen oder Laufzeitmaterialien.

| Originalklasse | Flächen | Innenringe | Bit |
| --- | ---: | ---: | ---: |
| Fels | 1.972 | 315 | 1 |
| Fels locker | 293 | 25 | 2 |
| Lockergestein | 593 | 23 | 4 |
| Lockergestein locker | 101 | 4 | 8 |
| Felsbloecke | 6 | 0 | 16 |
| Felsbloecke locker | 8 | 0 | 32 |
| Feuchtgebiet | 294 | 38 | 64 |

Die 3.267 Flächen und 405 Innenringe bleiben in `Geodata/SurfaceCoverSource/cover.geojson` mit Originalklasse und Quell-UUID erhalten. Die 4096×4096-Maske `Art/GroundMaterials/SurfaceCoverMask.png` verwendet den gleichen nordorientierten LV95-Ausschnitt wie die Waldmaske: E2723000–2745000, N1215000–1228000. Ein Pixel umfasst ungefähr 5,37×3,17 m. Sie ist deshalb keine zentimetergenaue Nahbereichs- oder Substratvermessung.

Jede Klasse besitzt ein eigenes Bit; 49 Pixel mit Klassenüberlappung werden nicht durch eine willkürliche Priorität überschrieben. Null bedeutet, dass keine dieser sieben Klassen vorliegt, **nicht** Gras, Strasse oder bebaute Fläche. Locker verteilte Felsen dürfen nicht als geschlossene Felsdecke interpretiert werden. Die Daten enthalten keine gemessenen Materialfarben oder Pflanzenarten.

Die unabhängige Punkt-in-Polygon-Abfrage an 30.000 deterministischen Pixelzentren stimmt mit dem Raster überein. Zusätzlich wird die gespeicherte PNG verlustfrei zurückgelesen und verglichen. Original-Prüfsummen, Ausgabedateien, Klassenbelegungen und Grenzen sind in [SURFACE_COVER_MASK.json](SURFACE_COVER_MASK.json) gebunden.

**Noch nicht im Rendering aktiv.** Bei einem späteren Import muss das Bitfeld ohne sRGB, verlustbehaftete Kompression, bilineare Byte-Interpolation oder gemittelte Mipmaps erhalten bleiben. Erst nach dem Dekodieren dürfen gestalterisch definierte Übergänge zwischen Klassen gemischt werden. Die Laufzeitintegration erfordert anschliessend Import-/Shaderprüfung, echte Nah-/Fernaufnahmen und Leistungsprüfung; die Offline-Maske allein beweist keine bessere Spielgrafik.

Reproduktion mit dem vorhandenen Geodaten-Environment:

```powershell
./Tools/.geodata-env/Scripts/python.exe Tools/prepare_surface_cover.py
```
