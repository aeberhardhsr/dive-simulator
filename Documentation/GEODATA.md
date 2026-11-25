# Walensee-Geodaten in der Laufzeitwelt

Quelle: © swisstopo, swissBATHY3D Walensee, Vermessung 2020, originales 1-m-Raster. Download und Prüfsumme sind in `WALENSEE_DATA.json` dokumentiert. Der vollständige offene Datensatz ist verwendet, nicht die eingeschränkt nutzbare Testdatei.

## Koordinaten und Höhen

- Lage: CH1903+ / LV95, EPSG:2056. Höhe: LN02, EPSG:5728.
- Ursprung der Unreal-Welt: E 2726000 / N 1219000. Unreal X zeigt nach Norden, Y nach Osten, Z nach oben. Ein Meter entspricht 100 Unreal-Einheiten.
- Z=0 entspricht dem festen Szenariopegel 419 m LN02. Tatsächliche Wasserstände schwanken; es gibt keine Live-Pegelanbindung.
- Ein Höhenwert liegt in der Rasterzellenmitte. Die Dateien laufen zeilenweise von Norden nach Süden. Die Abfrage interpoliert zwischen vier gemessenen Zentren, auch über Dateigrenzen.
- Fehlende Werte bleiben fehlend. Dreiecke mit ungemessenen Eckpunkten werden nicht erzeugt. Bewegung über den Vermessungsrand wird begrenzt. Die sehr flache Uferzone ist daher noch nicht spielbar.

`prepare_bathymetry.py` prüft das Downloadarchiv und erzeugt Little-Endian-Float32-Dateien. `package_bathymetry.py` wandelt absolute Höhen in vorzeichenbehaftete Int16-Zentimeter relativ zur Wasseroberfläche um; -32768 markiert NoData. Die Quantisierung weicht höchstens 5 mm ab. Originale Höhenstatistiken im Manifest bleiben in Metern LN02. Laufzeitdaten und Quellenhinweis werden als UFS-Dateien in das Unreal-Package übernommen.

## Geländedarstellung

Die 46 kompakten Raster werden beim Start in den Arbeitsspeicher geladen. Gerenderte und kollidierbare Geometrie wird in maximal neun Kacheln von je 64 × 64 m rund um den Taucher erzeugt. Jede Kachel verwendet ein 65 × 65 großes Vertices-Raster. Beim Weiterbewegen werden entfernte Kacheln verworfen und neue aufgebaut, höchstens eine pro Frame. Das ist lokales Geometriestreaming, noch kein asynchrones Nachladen der Quelldateien und kein World-Partition-System.

Die Übersichtskarte verwendet eine feste 512 × 128-Textur mit gemessenen Tiefen, 10-m-Konturen, drei Spielstarts und der Tauchspur. Felsen, Vegetation und Tiere werden separat prozedural gesetzt; ihre Standorte stammen nicht aus dem Höhenmodell.

## Spielstarts

| Ort | Ost LV95 | Nord LV95 |
|---|---:|---:|
| Weesen / West | 2726086.5 | 1221674.5 |
| Murg / Süd | 2734890.5 | 1219849.5 |
| Walenstadt / Ost | 2741110.5 | 1220619.5 |

Diese Punkte wurden innerhalb vermessener flacher Wasserflächen gewählt. Der Taucher beginnt 1,8 m unter dem Szenariopegel, bei etwa 6 m Grundtiefe. Die Blickrichtung weist zunächst hangabwärts. Es handelt sich um Offshore-Spielstarts, nicht um verifizierte reale öffentliche Tauchzugänge. Begehbare Ufer und echte Zugänge bleiben offen.

## Landschaft über Wasser

Das umliegende Gelände stammt aus [swissALTI3D](https://www.swisstopo.admin.ch/en/height-model-swissalti3d), © swisstopo, ebenfalls LV95/LN02. 286 vollständige 1-km-Kacheln der Ausgabe 2026 decken E 2723000–2745000 / N 1215000–1228000 ab. Jede Quelldatei wurde gegen ihren offiziellen STAC-SHA256-Multihash geprüft. Download-URLs, Editionsjahre und Prüfsummen stehen in `Content/Diving/Geodata/landscape.json`.

`Tools/prepare_landscape.py` liest die originalen 2-m-Pixel und mittelt jeweils 4 × 4 Zellen zu einem 8-m-Raster. Es verwendet bewusst nicht die eingebetteten GeoTIFF-Vorschaustufen: Diese liefern keine verlässlichen Flächenmittel. Eine unabhängige Prüfung von 858 über alle Kacheln verteilten Zellen gegen native 4 × 4-Pixelblöcke ergab maximal 0,000062 m numerische Abweichung. Das ist ein Umwandlungstest, keine Aussage zur Vermessungsgenauigkeit oder zum Detailverlust durch Mittelung.

Die Laufzeit lädt `landscape.f32` und erzeugt ein fernes Geländemesh mit 32-m-Abstand (278.922 Vertices, 509.490 Dreiecke beim geprüften Build). Land innerhalb der vermessenen Seefläche wird ausgespart. Dieses Hintergrundmesh hat keine Spielerkollision; die begehbare Fläche ist weiterhin die Bathymetrie. Feine Ufergeometrie, Gebäude, echte Landbedeckung, Vegetation und Landschafts-LOD fehlen noch. Die Farben für Fels und Bewuchs werden illustrativ aus Neigung und Höhe abgeleitet.

Zur erneuten Aufbereitung:

```powershell
python -m venv Tools/.geodata-env
Tools/.geodata-env/Scripts/python.exe -m pip install -r Tools/requirements-geodata.txt
Tools/.geodata-env/Scripts/python.exe Tools/prepare_landscape.py
Tools/.geodata-env/Scripts/python.exe Tools/validate_landscape.py
```

Die Rohdaten und die lokale Python-Umgebung sind von Git ausgeschlossen. Das kompakte Laufzeitraster ist über Git LFS versioniert.

## Örtliche Uferergänzung

`shoreline.json` beschreibt drei zusätzliche, jeweils 384 × 384 m große Bereiche. Referenzorte und veröffentlichte Koordinaten stammen aus den Beschreibungen von [Weesen Strandbad](https://www.swiss-divers.ch/informationen/tauchplaetze/walensee-tauchplaetze/strandbad-weesen/), [Murg West](https://www.swiss-divers.ch/informationen/tauchplaetze/walensee-tauchplaetze/murg-west-murg/) und [Lediwracks bei Mols](https://www.swiss-divers.ch/informationen/tauchplaetze/walensee-tauchplaetze/lediwracks-mols/). Es sind Ortsreferenzen für das Spiel, keine aktuelle Prüfung realer Zugangsbedingungen.

Die Ergänzung enthält 1-m-Raster und ein gleich großes Herkunftsraster:

| Kennung | Herkunft |
|---|---|
| 1 | Vorhandene swissBATHY3D-Rasterzelle, unverändert außer Float32-Darstellung |
| 2 | Natives swissALTI3D-2-m-Landmodell, bilinear auf 1 m abgetastet, über dem festen Spielszenariopegel |
| 3 | Modellierte Verbindung zwischen den vermessenen Flächen; keine gemessene Tiefe |

Die Lücke wird als diskrete harmonische Fläche mit festgehaltenen vermessenen Randwerten ergänzt. Am äußeren Kachelrand gilt eine verschwindende Normalableitung. Insgesamt sind 10.700 von 442.368 Zellen modelliert. Die Original-Bathymetrie wird nicht verändert. Dieser Ansatz verbindet die Geometrie, rekonstruiert aber keine unbekannten Felsen, Uferbefestigungen oder Stufen.

`prepare_shoreline.py` prüft die SHA256 der benötigten originalen Landkacheln und protokolliert Quellen, Herkunftszahlen, Prüfsummen und Solver-Residuen. `validate_shoreline.py` prüft unabhängig sämtliche Bathymetriezellen, verteilte Landzellen direkt gegen die nativen Quellen, eine analytische Testfläche, Randwerterhaltung, harmonische Residuen und Höhenbegrenzung.

Unreal erzeugt drei getrennte Ufermeshes mit Kollision. Ihr 1-m-Vertexraster und ihre Dreiecksdiagonalen schließen an die gestreamte Bathymetrie an; vollständig vermessene Seebodendreiecke bleiben im bisherigen Streaming. Die Bodenabfrage interpoliert die tatsächlichen Dreiecksflächen. Das ferne Landmesh wird im Inneren der Uferbereiche ausgespart. Die äußere Überlappungszone ist noch keine fertige Landschafts-LOD-Lösung.

Die Uferoberfläche erhält eigenes prozedurales Kies-/Bodenmaterial mit entfernungsabhängig gefilterter Körnung und einem vereinfachten feuchten Bereich um den festen Pegel. Farbe, Körnung und Bewuchszuordnung sind illustrative Gestaltung. Gebäude, Treppen, Wege, reale Bodenbedeckung und ein spielbarer Land-Wasser-Bewegungswechsel sind noch nicht umgesetzt. Die normalen Spielstarts bleiben vorerst die bisherigen Offshore-Punkte.

Reproduktion:

```powershell
Tools/.geodata-env/Scripts/python.exe Tools/prepare_shoreline.py
Tools/.geodata-env/Scripts/python.exe Tools/validate_shoreline.py
./Tools/Build.ps1 -Target Content -OnlyShore
./Tools/SmokeTest.ps1 -Scenario shore-preview -Entry 0
```
