# Geografische Begrenzung des Wassermediums

**Status: optionale GPU-Anbindung gebaut und nativ geprüft: 42.793 GPU-Abfragen sowie vier Grafikläufe mit 16 gesichteten Bildern bestanden. Nicht als Standard aktiviert; vollständige Ufer-/Grafikabnahme offen.** Die bestehende Wasserfläche reicht als Geometrie weit über den eigentlichen See hinaus. Das bisherige begrenzte Wasservolumen prüft die Wellenhöhe, aber keinen horizontalen Seeumriss. Auch der [neue Wellenlicht-Versuch](WAVE_TRANSPORT_STUDY.md) benötigt diese räumliche Begrenzung, damit Wasserabsorption nicht allein von der Höhe unter `Z=0` abhängt.

## Daten und Darstellung

Die vorhandene, bereits geprüfte swissTLM3D-Uferquelle umfasst den Walensee und drei Inseln. Der Generator verwendet das unveränderte Polygon mit der in [WALENSEE_COAST_SOURCE.json](WALENSEE_COAST_SOURCE.json) festgelegten SHA-256-Prüfsumme. Es wird keine neue Uferlinie aus fehlenden Tiefenwerten oder aus dem Hintergrundbild geschätzt.

Das Ergebnis beschreibt den horizontalen Abstand zur nächsten Uferlinie: positiv im Wasser, negativ an Land, einschliesslich der Inseln. Abstände jenseits von 32 m werden auf diesen Betrag begrenzt. Das Raster hat 1 m Abstand, 15.586 Spalten und 3.003 Zeilen. Pixelzentren liegen von Westen nach Osten und von Norden nach Süden. Float16-Werte werden ohne Farbraumumrechnung gespeichert. Der Weltursprung kommt aus dem bestehenden Laufzeit-Geodatenmanifest: Unreal X zeigt nach Norden, Y nach Osten, eine Einheit entspricht einem Zentimeter.

Die Erzeugung arbeitet in kleinen Blöcken mit einer räumlichen Suche nach relevanten Ufersegmenten. Ein Block ohne nahe Uferlinie wird vollständig als Wasser oder Land klassifiziert. Dadurch muss das vollständige Raster nicht gleichzeitig als geometrische Punktmenge im Arbeitsspeicher liegen. Die ursprüngliche Ergebnisdatei unter `Geodata/Processed/WaterFootprint` bleibt ignoriert. Für den optionalen nativen Versuch wird inzwischen eine identische Kopie unter `Content/Diving/Geodata` bereitgestellt (siehe Integration unten); ein aktualisiertes Spielpaket liegt noch nicht vor.

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

- Im optionalen Versuch verwenden Volumenmaterial und Oberflächen-/Volumen-Lichtberechnung dieselbe geografische Zuordnung und Shaderfunktion. Vollständige Lichtwege sind noch nicht geografisch begrenzt. Die anschliessende Begrenzung der sichtbaren Wasserfläche ist separat in [LAKE_SURFACE.md](LAKE_SURFACE.md) dokumentiert.
- GPU-Upload und Texturkoordinaten sind im optionalen nativen Versuch geprüft, ebenso ein freigelegter Prüfkörper an der Uferkontur. Natürliche Habitat-/Uferansichten bleiben offen. Eine unkomprimierte Textur dieser Auflösung benötigt rund 89,3 MiB vor Treiberoverhead; Streaming, Detailstufen und Leistung müssen vor einer Übernahme bewertet werden.
- Das Polygon beschreibt einen festen horizontalen Seeumriss. Es simuliert keine veränderlichen Pegel, Wellenauflauf, Überschwemmungen oder Bodentiefe. Es ersetzt weder die gemessene Bathymetrie noch die Geländekollision.
- Vor der Standardaktivierung sind feste Vergleichsposen an Ufern, Inseln und im offenen See nötig, zusätzlich zu den bestehenden Unterwasser-, Nacht- und Überwasserkontrollen.

Die Offline-Vorbereitung entstand während des [separaten Engine-Builds](SOURCE_ENGINE.md), der inzwischen abgeschlossen ist. Die vorhandene spielbare Vorschau bleibt unverändert.

```powershell
./Tools/.geodata-env/Scripts/python.exe -m pip install -r Tools/requirements-footprint.txt
./Tools/.geodata-env/Scripts/python.exe -m unittest discover -s Tests -p test_lake_footprint.py
./Tools/.geodata-env/Scripts/python.exe Tools/prepare_lake_footprint.py
./Tools/.geodata-env/Scripts/python.exe Tools/ReportLakeFootprint.py
```

Die Befehle setzen das lokal verfügbare, geprüfte Uferpolygon voraus; dessen ursprüngliche Beschaffung ist in `Tools/download_lake_boundary.py` dokumentiert. Alle von dieser Vorbereitung geschriebenen Raster bleiben unter den ignorierten Verzeichnissen `Geodata/Processed` oder `artifacts`.

## Optionale native Integration

`StageLakeFootprint.py` übernimmt das unveränderte 1-m-Raster nach `Content/Diving/Geodata`. Die vorhandene UFS-Verpackungsregel umfasst diesen Ordner. Der Loader prüft Bytezahl, Rasteridentität und den bestehenden LV95-Weltursprung, bevor er eine R16F-Textur mit einem Mip, bilinearer Abfrage und ohne sRGB erzeugt. Der GPU-Speicherbedarf des unkomprimierten Rasters beträgt rund 89,3 MiB; Paketgrösse und Laufzeitleistung sind noch nicht abgenommen.

Die neue Funktion in `Shaders/LakeFootprint.ush` wird sowohl im Volumenmaterial als auch in der direkten Sonnenbeleuchtung auf Oberflächen und Volumenpunkten verwendet. Die Abfrage liefert ausserhalb des Rasters Land. Um die Nullkontur wird über die dokumentierte Unsicherheitszone von −0,75 bis +0,75 m überblendet. Diese erste Integration begrenzte noch nicht die sichtbare Wasserfläche; der anschliessende Schritt ist in [LAKE_SURFACE.md](LAKE_SURFACE.md) beschrieben. Seitlich über Ufer laufende Lichtwege, Pegeländerungen und Wellenauflauf bleiben offen. Die Empfängerklassifikation ersetzt keine Integration aller wassergefüllten Teilstücke eines Strahls.

Die Funktion wird ausschliesslich mit `-DiveLakeFootprint` innerhalb des expliziten Wellenlicht-Versuchs aktiviert. Ohne diesen Schalter bleiben die bisherigen Vergleichsmodi erhalten. Ein veraltetes Volumenmaterial ohne den Texturparameter wird zurückgewiesen. `-OnlyWaterVolume` baut nur dieses Material; die bestehende Wellenverschiebung wurde unverändert in eine gemeinsame Shaderdatei ausgelagert.

Der erste C++-Linkversuch enthielt die neue Quelldatei noch nicht in der UBT-Dateiliste. Der anschliessende inkrementelle Aufruf mit `-gather` kompilierte sie und verlinkte beide Projektmodule erfolgreich. Die Engine selbst wurde dafür nicht erneut vollständig gebaut.

Für die GPU-Prüfung werden 42.793 Positionen vorbereitet: Pixelzentren, beidseitige Abstände an äusserem Ufer und allen drei Inseln, Inselinnenpunkte und Positionen ausserhalb des Rasters. Der Compute-Durchlauf verwendet den tatsächlichen Szenenpuffer mit derselben Textur, demselben Sampler, derselben Zuordnung und derselben Shaderfunktion. Die Rücklese erfolgt asynchron über Unreals [GPU-Readback-API](https://dev.epicgames.com/documentation/unreal-engine/API/Runtime/RHI/FRHIGPUBufferReadback). Ein solcher Vergleich beweist die GPU-Abfrage; echte Uferbilder, vollständige Lichtwege und Leistung bleiben zusätzliche Prüfungen.

```powershell
./Tools/.geodata-env/Scripts/python.exe Tools/StageLakeFootprint.py
./Tools/Build.ps1 -EngineRoot Builds/UE584 -EditorSandbox -SandboxName SourceEditorSandbox -Target Content -OnlyWaterVolume
./Tools/.geodata-env/Scripts/python.exe Tools/PrepareSolarShaderStudy.py --engine Builds/UE584 --project Builds/SourceEditorSandbox/divingsim --volume --waves
./Tools/TestCaustics.ps1 -EngineRoot Builds/UE584 -EditorSandbox -SandboxName SourceEditorSandbox -WaterPreview -SolarShaderStudy -SolarVolumeStudy -WaveTransportStudy -LakeFootprint -LakeFootprintProbeFile artifacts/lake-footprint-gpu/Probes.f32 -Case day -TimeoutSeconds 1200
./Tools/.geodata-env/Scripts/python.exe Tools/ReportLakeFootprintGPU.py --probes artifacts/lake-footprint-gpu --capture <Ergebnisordner>
```

Drei portable Auswertungstests bestehen: kleine Interpolationsabweichung, falsche Positions-/Zeilenzuordnung und fehlerhafte beziehungsweise nicht endliche Distanz-/Gewichtswerte. Der native Erstlauf unter `artifacts/caustics-day-validation-20261127-184356` ist ebenfalls bestanden; seine Rücklesewerte sind von den synthetischen Werkzeugtests getrennt erfasst.

### Ergebnis des nativen Versuchs

Der [gebundene Ergebnisbericht](LAKE_FOOTPRINT_GPU_RESULTS.json) enthält Raster-, Quellcode-, DLL-, Material-, Protokoll- und Bildprüfsummen sowie die getrennten Stände vor und nach Ergänzung der Uferfixture. Die GPU liefert über alle 42.793 Positionen maximal 0,0078125 m Abweichung zur unabhängigen CPU-Rasterabfrage; das 99-%-Quantil liegt bei 0,003986 m. Der Wasseranteil weicht maximal um 0,005254 ab. Ausserhalb der ±0,75-m-Zone gibt es keine falschen Wasser-/Land-Vorzeichen in diesen Stichproben. Alle drei Inselinnenpunkte und alle vier Abfragen ausserhalb des Rasters sind trocken. Diese Zahlen beschreiben numerische Rasterabfragen, nicht die Vermessungsgenauigkeit der Uferlinie.

Vier native Läufe mit je vier Aufnahmen sind abgeschlossen: Tagesbeleuchtung mit GPU-Abfrage, Uferfixture mit und ohne Maske und normale Wasservorschau ohne Wellen-/Uferexperiment. Die 16 Bilder wurden in Kontaktbögen gesichtet; beide Uferbilder mit Stärke eins zusätzlich in Originalauflösung. Die normale Vorschau zeigt weiterhin ihre bisherigen Kaustiken. Die bekannten AutomationTest-/ToolsetRegistry-Startdiagnosen bleiben in den Protokollen erhalten; neue Material-/Shader-Bindungsfehler traten nicht auf.

Die Uferfixture liegt auf der vermessenen Kontur bei E2726254,938 / N1221730,728. Sie blendet Weltgeometrie einschliesslich der sichtbaren Wasserfläche aus, um die Lichtwirkung auf einem Prüfkörper unterhalb des Geländes zu zeigen. Dies ist ein Diagnosebild, keine fertige Uferlandschaft. Auf der Landseite verschwinden die bewegten Lichtmuster; auf der Wasserseite bleiben sie bestehen. [Mit geografischer Begrenzung](Images/LakeFootprint_Shore_Masked.png) und [ohne Begrenzung](Images/LakeFootprint_Shore_Unmasked.png) verwenden identische Kamerapositionen und dieselben Prüfkörper.

Mittlere absolute RGB8-Abweichung in nach Bildsichtung ausgewählten Regionen (beschreibende Werte, keine radiometrische Kalibrierung):

| Region | Maske | Kaustikstärke ein/aus | Uhr 20/22 s | Eingefrorene Wiederholung |
| --- | --- | ---: | ---: | ---: |
| Landseitige Bodenfläche | ein | 0,271 | 0,271 | 0,272 |
| Landseitige Bodenfläche | aus | 2,875 | 4,301 | 0,417 |
| Wasserseitige Bodenfläche | ein | 3,978 | 5,458 | 0,412 |
| Wasserseitige Bodenfläche | aus | 3,976 | 5,455 | 0,410 |

Die Uferfixture wird mit `TestCaustics.ps1 -Case shore` und den oben aufgeführten Wellenversuchsparametern gestartet, einmal mit und einmal ohne `-LakeFootprint`. Die erste GPU-Prüfung kompilierte den erweiterten Szenenpuffer und die betroffenen Materialien neu; nachfolgende Läufe nutzten den Cache. Diese einmalige Kompilierzeit ist keine Messung der Spielleistung. Vor einer Standardaktivierung bleiben sichtbare Wassergeometrie, Uferlichtwege, natürliche Ufer-/Inselansichten, Leistung und Paketprüfung offen.
