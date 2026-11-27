# Geografische Begrenzung des Wassermediums

**Status: aus den vermessenen Uferlinien erzeugt und offline geprüft; noch nicht in den Renderer eingebunden.** Die bestehende Wasserfläche reicht als Geometrie weit über den eigentlichen See hinaus. Das bisherige begrenzte Wasservolumen prüft die Wellenhöhe, aber keinen horizontalen Seeumriss. Auch der [neue Wellenlicht-Versuch](WAVE_TRANSPORT_STUDY.md) benötigt diese räumliche Begrenzung, damit Wasserabsorption nicht allein von der Höhe unter `Z=0` abhängt.

## Daten und Darstellung

Die vorhandene, bereits geprüfte swissTLM3D-Uferquelle umfasst den Walensee und drei Inseln. Der Generator verwendet das unveränderte Polygon mit der in [WALENSEE_COAST_SOURCE.json](WALENSEE_COAST_SOURCE.json) festgelegten SHA-256-Prüfsumme. Es wird keine neue Uferlinie aus fehlenden Tiefenwerten oder aus dem Hintergrundbild geschätzt.

Das Ergebnis beschreibt den horizontalen Abstand zur nächsten Uferlinie: positiv im Wasser, negativ an Land, einschliesslich der Inseln. Abstände jenseits von 32 m werden auf diesen Betrag begrenzt. Das Raster hat 1 m Abstand, 15.586 Spalten und 3.003 Zeilen. Pixelzentren liegen von Westen nach Osten und von Norden nach Süden. Float16-Werte werden ohne Farbraumumrechnung gespeichert. Der Weltursprung kommt aus dem bestehenden Laufzeit-Geodatenmanifest: Unreal X zeigt nach Norden, Y nach Osten, eine Einheit entspricht einem Zentimeter.

Die Erzeugung arbeitet in kleinen Blöcken mit einer räumlichen Suche nach relevanten Ufersegmenten. Ein Block ohne nahe Uferlinie wird vollständig als Wasser oder Land klassifiziert. Dadurch muss das vollständige Raster nicht gleichzeitig als geometrische Punktmenge im Arbeitsspeicher liegen. Die Ergebnisdatei unter `Geodata/Processed/WaterFootprint` ist ignoriert und wird noch nicht mit dem Spiel ausgeliefert.

## Prüfergebnis

Der [Ergebnisbericht](LAKE_FOOTPRINT_RESULTS.json) bindet Quelldaten, Generator, Abfragecode und Raster. Die Referenzabfrage misst gegen das vollständige Vektorpolygon; sie verwendet nicht die blockweise Auswahl des Generators.

| Prüfung | Ergebnis |
| --- | --- |
| Vollständiges Raster | Alle Werte endlich und innerhalb ±32 m. |
| 5.000 Pixelzentren gegen die ursprüngliche Ufergeometrie | Maximal 0,0078 m Abweichung durch Float16-Quantisierung. |
| 37.786 frei liegende Punkte an äusserem Ufer und Inseln | Maximal 0,4328 m; 99 % der Interpolationsabweichungen höchstens 0,1268 m. |
| Wasser-/Land-Vorzeichen ausserhalb der vorab definierten Unsicherheitszone von 0,75 m | Keine Fehlklassifikation in diesen Stichproben. |
| Drei repräsentative Inselinnenpunkte | Alle als Land klassifiziert. |

Die 0,75-m-Zone berücksichtigt die bilineare Abfrage eines 1-m-Distanzrasters und dessen Quantisierung. Die Werte beschreiben die Abweichung der Rasterdarstellung vom vorhandenen Polygon, **nicht** die Vermessungsgenauigkeit der realen Küste. Direkt an Uferkanten und kleinen geometrischen Details ist das Raster eine Näherung. Die CPU-Tests prüfen ausserdem eine analytisch bekannte rechteckige Wasserfläche mit einer Insel, unterschiedliche Blockgrössen, Achsenzuordnung, Pixelzentren und Abfragen ausserhalb des Rasters.

## Nächste Integration und Grenzen

- Dieselbe geografische Zuordnung muss im Volumenmaterial und in der Oberflächen-/Volumen-Lichtberechnung verwendet werden. Ein separater Licht-Test kann nicht beweisen, dass das gesamte Wassermedium korrekt begrenzt ist.
- GPU-Upload, Texturkoordinaten und tatsächliche Bildwirkung sind noch ungeprüft. Eine unkomprimierte Textur dieser Auflösung benötigt rund 89,3 MiB vor Treiberoverhead; Streaming, Detailstufen und Leistung müssen vor einer Übernahme bewertet werden.
- Das Polygon beschreibt einen festen horizontalen Seeumriss. Es simuliert keine veränderlichen Pegel, Wellenauflauf, Überschwemmungen oder Bodentiefe. Es ersetzt weder die gemessene Bathymetrie noch die Geländekollision.
- Vor der Standardaktivierung sind feste Vergleichsposen an Ufern, Inseln und im offenen See nötig, zusätzlich zu den bestehenden Unterwasser-, Nacht- und Überwasserkontrollen.

Der laufende [separate Engine-Build](SOURCE_ENGINE.md) wird durch diese Offline-Arbeit nicht neu gestartet. Die vorhandene spielbare Vorschau bleibt unverändert.

```powershell
./Tools/.geodata-env/Scripts/python.exe -m pip install -r Tools/requirements-footprint.txt
./Tools/.geodata-env/Scripts/python.exe -m unittest discover -s Tests -p test_lake_footprint.py
./Tools/.geodata-env/Scripts/python.exe Tools/prepare_lake_footprint.py
./Tools/.geodata-env/Scripts/python.exe Tools/ReportLakeFootprint.py
```

Die Befehle setzen das lokal verfügbare, geprüfte Uferpolygon voraus; dessen ursprüngliche Beschaffung ist in `Tools/download_lake_boundary.py` dokumentiert. Alle von dieser Vorbereitung geschriebenen Raster bleiben unter den ignorierten Verzeichnissen `Geodata/Processed` oder `artifacts`.
