# Sediment-, Kies- und Felsoberflächen

Die Anforderungen WRLD-006/007 und GFX-001/002 verlangen differenzierte, realistische Unterwasseroberflächen. Der bisherige Seegrund und die Felsen verwendeten dasselbe mathematische Farbrauschen ohne gescannte Rauheits- oder Normalendetails. Die neue Materialfamilie verwendet drei fotografisch erfasste PBR-Oberflächen mit je einer Farb-, DirectX-Normalen- und kombinierten AO/Rauheit/Metall-Textur in 2048×2048 Pixeln.

## Herkunft

| Oberfläche | Quelle | Urheber laut Quelle | Wiederholungsmaß |
|---|---|---|---:|
| Feiner sandiger Untergrund | [Gravelly Sand](https://polyhaven.com/a/gravelly_sand) | Dario Barresi | 2,5 m |
| Kies / kleine Steine | [River Small Rocks](https://polyhaven.com/a/river_small_rocks) | Rob Tuytel, Rico Cilliers | 2,9 m |
| Freiliegender Fels | [Rock Boulder Dry](https://polyhaven.com/a/rock_boulder_dry) | Dimitrios Savva, Rico Cilliers | 1,8 m |

Alle drei Assets sind vom Anbieter unter [CC0](https://polyhaven.com/license) veröffentlicht. Die Originalkarten liegen unverändert unter `Art/BedMaterials`; URLs, Dateigrößen, Anbieter-MD5 und eigene SHA256 stehen in `BED_MATERIALS.json`. Der Import prüft SHA256, der reproduzierbare Download zusätzlich Größe/MD5. Es werden keine Anbieter-Vorschaurenderings als Spielmaterial übernommen.

Diese Oberflächen sind **keine lokalen Sedimentmessungen des Walensees**. Die Wahl und Mischung sind eine gestalterische Annäherung. Die vermessenen Höhen, geografische Positionen und Kollisionsmeshes werden durch diesen Materialaufbau nicht verändert. Die ursprünglichen prozeduralen Felsformen werden im aktuellen Quellstand durch drei detailliertere Formen ersetzt ([Felsgeometrie und Prüfstand](ROCK_GEOMETRY.md)); feinere Texturen und Formen ersetzen keine geologische oder vollständige fotorealistische Abnahme.

## Darstellung

`Tools/Shaders/Lakebed.ush` wird durch `Tools/build_bed_content.py` in `M_Lakebed`, `M_RockSurface` und `M_ShorePhoto` eingebettet. Weltkoordinaten sorgen für gleiche Maßstäbe und kontinuierliche Übergänge zwischen nachgeladenen Geländekacheln. Drei Projektionsrichtungen vermeiden die starke Streckung einer reinen Draufsicht an steilen Flächen. Normalendetails werden in Weltkoordinaten ausgerichtet und gemischt; bei einer flachen Normalenkarte bleibt die vorhandene Mesh-Normale erhalten.

Die ergänzten Ufermeshes verwenden unterhalb des mittleren Wasserspiegels denselben Shader wie die Bathymetrie. Zwischen Z=0 und Z=1,2 m wird weich auf die Landdarstellung überblendet: registrierte SWISSIMAGE-Luftaufnahme mit den unten beschriebenen Nahdetails. Die dynamische `Ortho`-Texturzuordnung bleibt erhalten. Auch ein separater Ufer-/Orthobild-Neuaufbau ruft diese gemeinsame Materialerzeugung auf, damit er die Unterwasseroberfläche nicht wieder durch das alte Kiesmuster ersetzt.

Hangneigung steuert den Anteil freiliegenden Felses. Ruhige großräumige Flecken mischen Sand und Kies; mit zunehmender Tiefe wird der Kiesanteil geringer. Das ist eine feste künstlerische Verteilungsregel, keine Sedimenttransport-Simulation. Beim Felsobjekt wird ausschließlich die Felsschicht verwendet. Die sandige Quelle wird teilweise entsättigt; Unterwassernässe dunkelt die Farbe ab und beeinflusst Rauheit. Metallanteil ist null. Caustics werden inzwischen als gemeinsame Beleuchtung angewendet ([CAUSTICS.md](CAUSTICS.md)); der Boden leuchtet nicht selbst.

Farbe wird in sRGB eingelesen, Normalen/ARM als lineare Daten. BC5-Normalen verwenden die DirectX-Konvention ohne nochmalige Grünkanalumkehr. Die Rauheit bleibt zwischen 0,55 und 0,98, damit der Grund nicht spiegelglatt wirkt. Neun feste 2K-Texturen behalten Mipmaps, sind aber resident: Für die zur Laufzeit erzeugte Bathymetrie fehlen verlässliche importierte UV-Dichtedaten. Der tatsächliche Speicher-/GPU-Aufwand ist deshalb mit dem gerenderten Build zu prüfen; dieses Material behauptet kein vollständiges bedarfsgerechtes Texturstreaming.

Der geöffnete Benutzer-Editor hält das ältere `M_Rock` gesperrt. Das neue `M_RockSurface` wird separat erzeugt und vom Laufzeitcode zugewiesen; das bestehende Material bleibt unangetastet. Ein vollständiger Content-Neuaufbau weist dem Felsmesh ebenfalls das neue Material zu.

## Reproduzieren

```powershell
python Tools/prepare_bed_materials.py
./Tools/Build.ps1 -Target Content -OnlyBed
./Tools/Build.ps1 -Target Editor
./Tools/Build.ps1 -Target Package -PackageDirectory Builds/WindowsBed
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsBed -Scenario underwater -Resolution 1080p
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsBed -Scenario stream -Resolution 1080p
```

Der volle Content-Build ruft dieselbe Materialerzeugung auf. Quellen und Unreal-Assets verwenden Git LFS. Aktuelle Ergebnisse und Grenzen werden in `VALIDATION.md` festgehalten.

## Bildvergleich

Unveränderte Kamera und Grundgeometrie im Unterwasser-Vergleichsablauf; Tierbewegungen und einzelne Partikel sind zeitabhängig. Die Bilder sind vollständige Spielaufnahmen, keine Material-Vorschaurenderings.

Vorher:

![Bisheriger Seegrund mit mathematischem Farbrauschen](Images/Bed_Before.png)

Mit den neuen Oberflächen:

![Gemischter Sand- und Kiesgrund mit gescannten Felsdetails](Images/Bed_After.png)

Nahansicht einer steilen Geländestelle:

![Weltprojizierte Materialien am geneigten Seegrund](Images/Bed_Steep.png)

Der gleiche Materialansatz im ergänzten Uferbereich:

![Unterwasserboden am Weesener Einstieg](Images/Bed_Shore.png)

## Nahdetails über Wasser

Der Stand `Builds/WindowsGround/divingsim.exe` ergänzt WRLD-008 und GFX-001/002 um Gras- und Waldboden in Ufer-, Küsten- und Hintergrundmaterialien. `GroundDetail.ush` verwendet dieselbe Weltprojektion und Normalenausrichtung wie der Seegrund. Zwei weitere CC0-Scans von Poly Haven liefern jeweils Farbe, DirectX-Normale und ARM in 2K: [Grass Ground](https://polyhaven.com/a/grass_ground), Charlotte Baglioni, 2,51 m Wiederholungsmaß; [Forest Ground 04](https://polyhaven.com/a/forest_ground_04), Rob Tuytel/Rico Cilliers, 3,15 m. Originale und Prüfsummen stehen unter `Art/GroundMaterials` beziehungsweise `GROUND_MATERIALS.json`. Steile Flächen verwenden die vorhandene Felsoberfläche.

Eine nordorientierte 4096×4096-Maske rastert die 4.013 bereits geprüften swissTLM3D-Waldpolygone einschließlich ihrer Innenringe. Sie umfasst E2723000–2745000/N1215000–1228000 (LV95); ein Pixel entspricht ungefähr 5,37×3,17 m. Die Belegung von 20.000 zufällig gewählten Pixelzentren wurde unabhängig mit einer Punkt-in-Polygon-Abfrage geprüft (`GROUND_MASK.json`). Das Raster wird bilinear gefiltert, seine Grenzen sind daher Darstellungsübergänge und keine zusätzliche Vermessung.

Innerhalb von 25 m wirken die Nahmaterialien vollständig; zwischen 25 und 120 m gehen Farbe, Normale, Rauheit und AO weich zum Luftbild zurück. Wald erhält Boden statt der im Luftbild fotografierten Baumkronen. Außerhalb der Waldmaske wird Gras anhand grüner Luftbildfarben gewichtet. Neutrale Flächen behalten das Luftbild; steile Flächen erhalten Fels. **Diese Materialklassen sind gestalterische Annäherungen**, keine vermessene Boden- oder Straßenklassifikation. Einzelflächen können falsch zugeordnet sein. Das gemeinsame Verfahren verschiebt weder Geometrie noch Kollision oder Luftbildregistrierung.

Sieben zusätzliche Texturen einschließlich Waldmaske bleiben mit ihren Mipmaps resident (32,19 MiB im Windows-Spiel). Ein gemeinsamer Sampler für Gras und Waldboden hält das kombinierte Ufermaterial innerhalb der Hardwaregrenze. Jeder Prüfstart kontrolliert die Zuordnung aller sieben Texturen in fünf dynamischen Materialinstanzen, Auflösung, Mip-Residenz, Farbraum, Kompression und Randadressierung. `-DiveGroundOff` deaktiviert die Details für Vergleichsmessungen; die reguläre Darstellung verwendet Stärke 1.

```powershell
python Tools/prepare_ground_materials.py
./Tools/.geodata-env/Scripts/python.exe Tools/prepare_ground_mask.py
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/Build.ps1 -Target Content -OnlyGround -EditorSandbox
./Tools/Build.ps1 -Target Package -EditorSandbox -BuildSlot GroundDetails -PackageDirectory Builds/WindowsGround
./Tools/TestGround.ps1
./Tools/TestGroundLauncher.ps1
```

Der Maskenaufbau benötigt NumPy, Pillow, rasterio und Shapely im Geodaten-Environment. `-OnlyImagery` und der vollständige Content-Build rufen ebenfalls die gemeinsame Bodenerzeugung auf. Der ältere Vegetations-Build bleibt separat erhalten.

Der gebaute Windows-Stand besteht **16 Paketprüfungen mit 52 frischen Bildern**: Gelände- und Nahansicht an drei Einstiegen, drei vollständige Ufer-/Tauch-/Rückkehrabläufe, drei Küstenüberquerungen mit Rückweg, zwei Vergleichsansichten ohne Details und zwei Ansichten ohne Nanite. Der normale Starter startet nachweislich das passende ausführbare Spiel und beendet einen zusätzlichen Tauchtest erfolgreich. Dateihashes, Quellstand, Bilder, Maskenprüfung und Starterprotokoll stehen in [GROUND_RESULTS.json](GROUND_RESULTS.json). `Tools/ReportGround.py` erhält den von `TestGround.ps1` ausgegebenen Ergebnisordner. Die [GitHub-Prüfung des Quellcommits 9c18f63](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45773397806) besteht auf drei Betriebssystemen; sie prüft den portablen Kern und Geometriewerkzeuge, keinen nativen macOS-Unreal-Build.

Die folgenden vollständigen Spielaufnahmen stammen aus demselben Paket bei 1600×900 und gleichem Kamerastandpunkt. Nur der Detailparameter ist im Vorher-Bild deaktiviert; Baumbewegungen können zwischen den Aufnahmen abweichen.

![Uferlandschaft mit deaktivierten Bodendetails](Images/Ground_Land_Before.png)

![Uferlandschaft mit Gras- und Waldbodendetails](Images/Ground_Land_After.png)

![Gras in der Nahansicht ohne Bodendetails](Images/Ground_Close_Before.png)

![Gras in der Nahansicht mit Bodendetails](Images/Ground_Close_After.png)

Die drei anschließenden Messungen verwenden dasselbe Paket: 60 Sekunden nach 15 Sekunden Aufwärmzeit, 1920×1080, Qualitätsstufe 2, 100 % Renderauflösung, ohne Bildratenlimit. Rechner: RTX 3090/Ryzen 9950X3D; der Benutzer-Editor blieb geöffnet. Am Ufer dreht die Kamera durch 360°. Vollständige Daten und Hashes: [GROUND_PERFORMANCE.json](GROUND_PERFORMANCE.json).

| Szene | Durchschnitt FPS | Langsamste 1 % FPS | Längster Frame | Frames über 33,3 ms |
|---|---:|---:|---:|---:|
| Unter Wasser | 75,17 | 61,70 | 17,17 ms | 0 / 4508 |
| Ufer | 77,93 | 63,06 | 17,18 ms | 0 / 4674 |
| Ufer ohne Nanite | 75,27 | 42,35 | 24,63 ms | 0 / 4514 |

Das sind kurze Messungen auf dem Entwicklungsrechner, keine Mindesthardware- oder Langzeitfreigabe. `Tools/ReportGroundPerformance.py` prüft die drei Messordner gegen die Paketdateien aus `GROUND_RESULTS.json`.

Offen bleiben unter anderem scharfe Details auf neutralen Luftbildflächen, verbindliche Straßenfreiräume, natürliche Übergänge ohne sichtbare Texturwiederholung, zusätzliche Bodengeometrie und saisonale Oberflächen. Die bereits zuvor stufigen Geländeformen bei Murg/Mols werden durch das Felsmaterial deutlicher; ihre lokale Gestaltung und Zuordnung als Fels, Böschung oder Stützmauer ist nicht abgenommen. Nahtexturen ersetzen weder eine fotorealistische Abnahme noch den Test auf Mindesthardware und macOS.
