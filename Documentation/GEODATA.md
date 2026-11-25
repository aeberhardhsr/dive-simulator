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

Diese Punkte wurden innerhalb vermessener flacher Wasserflächen gewählt. Der Taucher beginnt 1,8 m unter dem Szenariopegel, bei etwa 6 m Grundtiefe. Die Blickrichtung weist zunächst hangabwärts. Es handelt sich um Offshore-Spielstarts, nicht um verifizierte reale öffentliche Tauchzugänge. Oberirdisches Gelände und echte Uferzugänge sind weitere Arbeitspakete.
