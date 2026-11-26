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

Diese Punkte wurden innerhalb vermessener flacher Wasserflächen gewählt. Der Taucher beginnt 1,8 m unter dem Szenariopegel, bei etwa 6 m Grundtiefe. Die Blickrichtung weist zunächst hangabwärts. Es handelt sich um die optionalen Offshore-Spielstarts, nicht um verifizierte reale öffentliche Tauchzugänge. Die zusätzlichen Uferstarts sind unten beschrieben.

## Landschaft über Wasser

Das umliegende Gelände stammt aus [swissALTI3D](https://www.swisstopo.admin.ch/en/height-model-swissalti3d), © swisstopo, ebenfalls LV95/LN02. 286 vollständige 1-km-Kacheln der Ausgabe 2026 decken E 2723000–2745000 / N 1215000–1228000 ab. Jede Quelldatei wurde gegen ihren offiziellen STAC-SHA256-Multihash geprüft. Download-URLs, Editionsjahre und Prüfsummen stehen in `Content/Diving/Geodata/landscape.json`.

`Tools/prepare_landscape.py` liest die originalen 2-m-Pixel und mittelt jeweils 4 × 4 Zellen zu einem 8-m-Raster. Es verwendet bewusst nicht die eingebetteten GeoTIFF-Vorschaustufen: Diese liefern keine verlässlichen Flächenmittel. Eine unabhängige Prüfung von 858 über alle Kacheln verteilten Zellen gegen native 4 × 4-Pixelblöcke ergab maximal 0,000062 m numerische Abweichung. Das ist ein Umwandlungstest, keine Aussage zur Vermessungsgenauigkeit oder zum Detailverlust durch Mittelung.

`DiveLandscapeBakeCommandlet` erzeugt aus `landscape.f32` gekachelte statische Unreal-Meshes mit 8-m-Abstand. Eine volle Kachel umfasst 256 × 256 Rasterzellen (2048 × 2048 m), Randkacheln sind entsprechend kleiner. Alle Kacheln teilen an ihren Grenzen dieselben Höhen und aus dem Gesamtraster berechneten Normalen. Die frühere Darstellung ließ drei von vier Rasterzeilen und -spalten aus und hatte 32 m Abstand.

Die Meshes verwenden Nanite mit ungekürzter Quellgeometrie und 1 cm Positionsquantisierung. Nanite übernimmt die sichtabhängige Detailauswahl und das Streaming seiner Geometrieseiten; die Kachelkomponenten selbst werden beim Start geladen. Für Rendering ohne Nanite wird ein vereinfachtes Fallback-Mesh mitgebaut. Das ist keine Aussage zur macOS-Spielkompatibilität oder zur noch offenen Performance-Abnahme. Technischer Hintergrund: [Epic Nanite-Dokumentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/nanite-virtualized-geometry-in-unreal-engine).

Land innerhalb der vermessenen Seefläche und der örtlichen Uferbereiche wird ausgespart. Die Hintergrundkacheln haben keine Spielerkollision; begehbar sind die Bathymetrie und die drei örtlichen Uferergänzungen. Die Oberfläche verwendet die unten beschriebenen SWISSIMAGE-Luftbilder. Vermessene Gebäude sind separat ergänzt; räumliche Vegetation fehlt noch. Das 8-m-Raster glättet weiterhin Gelände unterhalb dieser Auflösung; Orthobilder enthalten bereits Schatten und liefern keine vertikale Fassaden- oder Felsstruktur.

`Content/Diving/Geodata/landscape_meshes.json` enthält die Kachelgrenzen, Weltpositionen und Geometriezahlen. Ein MD5 über die tatsächlichen Höhen-/Maskendateien und ihre Manifeste dient ausschließlich der Erkennung veralteter Build-Ergebnisse; JSON-Zeilenenden werden für Windows/macOS auf LF vereinheitlicht. Die offiziellen Quelldaten werden separat mit SHA256 geprüft. Der Editor-Prüflauf vergleicht sämtliche gespeicherten Quellvertices mit dem Höhenraster und sämtliche Vertex-Instanz-UVs mit der geografischen Registrierung. Er prüft außerdem Normalen, dazu senkrechte Tangenten und vorhandene Nanite-Daten. Packaging führt diese Prüfung automatisch aus und bricht bei fehlenden oder veralteten Assets ab.

Zur erneuten Aufbereitung:

```powershell
python -m venv Tools/.geodata-env
Tools/.geodata-env/Scripts/python.exe -m pip install -r Tools/requirements-geodata.txt
Tools/.geodata-env/Scripts/python.exe Tools/prepare_landscape.py
Tools/.geodata-env/Scripts/python.exe Tools/validate_landscape.py
./Tools/Build.ps1 -Target Editor
./Tools/Build.ps1 -Target Landscape       # Geänderte Geodaten in Nanite-Kacheln umsetzen
./Tools/Build.ps1 -Target LandscapeCheck  # Nur prüfen, keine Assets schreiben
python Tools/validate_landscape_meshes.py # Unabhängige Eingangs-/Kachelmanifestprüfung
```

Die Rohdaten und die lokale Python-Umgebung sind von Git ausgeschlossen. Laufzeitraster und gebaute Landschaftsassets sind über Git LFS versioniert.

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

Unreal erzeugt drei getrennte Ufermeshes mit Kollision. Ihr 1-m-Vertexraster und ihre Dreiecksdiagonalen schließen an die gestreamte Bathymetrie an; vollständig vermessene Seebodendreiecke bleiben im bisherigen Streaming. Die Bodenabfrage interpoliert die tatsächlichen Dreiecksflächen. Der äußere Rand wird inzwischen auf das 8-m-Hintergrundraster erweitert und teilt dessen Kanten; die bisherige Überlappung entfällt. Ein 32-m-Band gleicht abgeleitete Geometrie, Normalen und Luftbilder an. Inneres Gelände und vermessene Bathymetrie bleiben unverändert; das Band ist eine Darstellungs-/Kollisionsannäherung und keine neue Vermessung. Details, Vergleich und Grenzen: [SHORE_TRANSITION.md](SHORE_TRANSITION.md).

Die Uferoberfläche verwendet nahe dem Wasser eigenes prozedurales Kies-/Bodenmaterial mit gefilterter Körnung und einem vereinfachten feuchten Bereich. Zwischen 0 und 1,2 m über dem Szenariopegel geht es in die unten beschriebenen SWISSIMAGE-Luftbilder über. Die Kiesgestaltung ist illustrativ; fotografierte Wege und Bodenbedeckung sind in den Bildern enthalten. Vermessene Gebäude sind separat ergänzt; originalgetreuer Wegeausbau und räumliche Vegetation fehlen.

## Modellierte spielbare Zugänge

`DiveAccess.cpp` erzeugt an jedem Uferstart eine 1,8 m breite Plattform und eine Treppe mit Geländern. Die Konstruktion liegt über dem unveränderten Höhenmodell: 18 cm Steigung und 32 cm Auftritt, ein passendes flaches Podest und ein Treppenende etwa 1,8 m unter dem Szenariopegel. Weesen hat 24 Stufen / 11,68 m Gesamtlänge, Murg 37 / 12,84 m, Mols 36 / 15,52 m. Diese Abmessungen sind Spielparameter, keine Vermessung der tatsächlichen Anlagen. Auch bei Weesen ist diese Konstruktion eine Spielhilfe und keine Behauptung über die örtliche Strandgestaltung.

Die sichtbaren Stufen verwenden eine durchgehende Kollisionsrampe; Geländer blockieren die Spielfigur. Die Bodenabfrage berücksichtigt dieselbe Rampe. Der Ufermodus verwendet eine aufrechte Kapsel mit 0,9 m halber Höhe und eine Kamera 1,76 m über dem Standniveau. Gehen/Waten setzt Bodenkontakt voraus; der Übergang zum Schwimmen liegt bei 1,55 m Wasser über dem Boden, die Rückkehr zum Gehen bei 1,45 m. Eintauchtiefe gewichtet Gewichtskraft, Auftrieb und Wasserwiderstand. Das ist eine vereinfachte aufrechte Körperdarstellung, noch keine schwimmende Ganzkörperpose.

Die ursprünglichen Offshore-Starts bleiben unverändert auswählbar. Das additive Einstellungsfeld `start_on_shore` wird in Einstellungen und Tauchprofilen gespeichert; ältere Dateien ohne dieses Feld behalten den Wasserstart.

Reproduktion:

```powershell
Tools/.geodata-env/Scripts/python.exe Tools/prepare_shoreline.py
Tools/.geodata-env/Scripts/python.exe Tools/validate_shoreline.py
./Tools/Build.ps1 -Target Content -OnlyShore
./Tools/SmokeTest.ps1 -Scenario shore-preview -Entry 0
```

## SWISSIMAGE-Luftbilder

Die Landschaft und die drei Uferbereiche tragen georeferenzierte [SWISSIMAGE-Orthobilder](https://www.swisstopo.admin.ch/de/orthobilder-swissimage-10-cm), © swisstopo. Für enthaltene Seeflächen gilt zusätzlich: **Includes material © (2016-2018) Planet. All rights reserved.** Diese Quellenangaben stehen auch im Spielmenü und in `Content/Diving/Geodata/IMAGERY_NOTICE.txt`. Nutzungsbedingungen: [swisstopo OGD](https://www.swisstopo.admin.ch/de/nutzungsbedingungen-kostenlose-geodaten-und-geodienste).

Die Pipeline verwendet 286 originale 2-m-GeoTIFFs für den Landschaftsausschnitt sowie vier originale 0,1-m-GeoTIFFs für die drei örtlichen Ufer. Alle ausgewählten Kacheln tragen das Aufnahmejahr 2025. Eine Kacheljahresangabe beschreibt den überwiegenden Bildstand, keinen einheitlichen Aufnahmetag. Jede vollständige Quelldatei wurde gegen den SHA256-Multihash des offiziellen STAC-Assets geprüft.

| Textur | LV95-Abdeckung | Laufzeitgröße | Horizontale Pixelabstände |
|---|---|---|---|
| Landschaft | E 2723000–2745000, N 1215000–1228000 | 4096 × 2048 | 5,3711 × 6,3477 m |
| Weesen Strandbad | örtliches 384 × 384-m-Uferfeld | 2048 × 2048 | 0,1875 × 0,1875 m |
| Murg West | örtliches 384 × 384-m-Uferfeld | 2048 × 2048 | 0,1875 × 0,1875 m |
| Mols/Lediwracks | örtliches 384 × 384-m-Uferfeld | 2048 × 2048 | 0,1875 × 0,1875 m |

Die nativen RGB-Pixel werden ohne Verwendung der COG-Vorschaustufen gelesen, in LV95 zusammengefügt und flächengewichtet in gespeicherten sRGB-Farbwerten verkleinert. Die Rechtecke sind exakt mit den zugehörigen Höhenrastern registriert; unterschiedliche Pixelabstände der Landschaft werden über die rechteckigen UV-Koordinaten berücksichtigt. Die Bildzeilen laufen von Nord nach Süd. Kein Bild verändert die Gelände- oder Kollisionshöhen.

`imagery.json` versioniert für jedes Bild Ausdehnung, Größe, Pixelabstände, Ausgangsdateien, URLs, Aufnahmejahr und Quell-/PNG-Prüfsummen. Standardmäßig reproduziert `prepare_imagery.py` diese festgeschriebenen Quellen. Nur `--refresh` wählt neuere veröffentlichte Kacheln. Die Rohdaten und vorbereiteten PNGs sind lokal; die Unreal-Texturassets und das Manifest sind versioniert. Der Content-Build kann vorhandene Assets anhand ihrer Quellen-Metadaten weiterverwenden. Bei fehlenden bzw. abweichenden Assets ist zunächst die Datenaufbereitung erforderlich.

Die vier Texturen behalten ihre vollständige Mipmap-Kette im Speicher, weil prozedurale Meshes keine zuverlässige importierte UV-Dichte für Unreals Texture-Streamer liefern. Im geprüften Windows-Build melden sie zusammen 14.090.240 Bytes (13,44 MiB) Texturspeicher. Das ersetzt keinen vollständigen Performance-Benchmark.

Die Bilder enthalten bereits aufgenommenes Licht und Schatten. Fotografierte Bäume bleiben flach auf das Geländemesh projiziert. Gebäude besitzen inzwischen eigene vermessene Außenflächen, Kollision und wechselnde Schatten; Herkunft und Grenzen stehen in [BUILDINGS.md](BUILDINGS.md). Steile Hänge, Übergänge, nahe Vegetation und Gebäudefassaden benötigen weitere Modellierung. Die Luftbilder sind deshalb ein Schritt zur realen Ortsdarstellung, keine abgeschlossene fotorealistische Landschaft.

Reproduktion und unabhängige Prüfung:

```powershell
Tools/.geodata-env/Scripts/python.exe Tools/prepare_imagery.py
Tools/.geodata-env/Scripts/python.exe Tools/validate_imagery.py
./Tools/Build.ps1 -Target Content -OnlyImagery
```

Der unabhängige Prüfer kontrolliert sämtliche 290 offiziellen Quellenprüfsummen und vergleicht 675 Ausgabepixel einschließlich Kachelgrenzen mit selbst berechneten gewichteten Flächenmitteln der nativen Quellen. Maximale Abweichung: 0,499867 von 255, innerhalb der 8-Bit-Rundung. Unreal prüft zusätzlich Materialzuordnung, Auflösung, sRGB, Randbegrenzung, geladene Mipmaps und räumliche Übereinstimmung der Mesh-UVs mit den LV95-Rechtecken.
