# Sediment-, Kies- und Felsoberflächen

Die Anforderungen WRLD-006/007 und GFX-001/002 verlangen differenzierte, realistische Unterwasseroberflächen. Der bisherige Seegrund und die Felsen verwendeten dasselbe mathematische Farbrauschen ohne gescannte Rauheits- oder Normalendetails. Die neue Materialfamilie verwendet drei fotografisch erfasste PBR-Oberflächen mit je einer Farb-, DirectX-Normalen- und kombinierten AO/Rauheit/Metall-Textur in 2048×2048 Pixeln.

## Herkunft

| Oberfläche | Quelle | Urheber laut Quelle | Wiederholungsmaß |
|---|---|---|---:|
| Feiner sandiger Untergrund | [Gravelly Sand](https://polyhaven.com/a/gravelly_sand) | Dario Barresi | 2,5 m |
| Kies / kleine Steine | [River Small Rocks](https://polyhaven.com/a/river_small_rocks) | Rob Tuytel, Rico Cilliers | 2,9 m |
| Freiliegender Fels | [Rock Boulder Dry](https://polyhaven.com/a/rock_boulder_dry) | Dimitrios Savva, Rico Cilliers | 1,8 m |

Alle drei Assets sind vom Anbieter unter [CC0](https://polyhaven.com/license) veröffentlicht. Die Originalkarten liegen unverändert unter `Art/BedMaterials`; URLs, Dateigrößen, Anbieter-MD5 und eigene SHA256 stehen in `BED_MATERIALS.json`. Der Import prüft SHA256, der reproduzierbare Download zusätzlich Größe/MD5. Es werden keine Anbieter-Vorschaurenderings als Spielmaterial übernommen.

Diese Oberflächen sind **keine lokalen Sedimentmessungen des Walensees**. Die Wahl und Mischung sind eine gestalterische Annäherung. Die vermessenen Höhen, geografische Positionen und Kollisionsmeshes werden dadurch nicht verändert. Auch die groben Felsformen bleiben bislang prozedurale Modelle; feinere Texturen ersetzen keine anatomische/geologische oder vollständige fotorealistische Abnahme.

## Darstellung

`Tools/Shaders/Lakebed.ush` wird durch `Tools/build_bed_content.py` in `M_Lakebed`, `M_RockSurface` und `M_ShorePhoto` eingebettet. Weltkoordinaten sorgen für gleiche Maßstäbe und kontinuierliche Übergänge zwischen nachgeladenen Geländekacheln. Drei Projektionsrichtungen vermeiden die starke Streckung einer reinen Draufsicht an steilen Flächen. Normalendetails werden in Weltkoordinaten ausgerichtet und gemischt; bei einer flachen Normalenkarte bleibt die vorhandene Mesh-Normale erhalten.

Die ergänzten Ufermeshes verwenden unterhalb des mittleren Wasserspiegels denselben Shader wie die Bathymetrie. Zwischen Z=0 und Z=1,2 m wird weich auf die bereits registrierte SWISSIMAGE-Luftaufnahme überblendet. Die dynamische `Ortho`-Texturzuordnung bleibt erhalten. Auch ein separater Ufer-/Orthobild-Neuaufbau ruft diese gemeinsame Materialerzeugung auf, damit er die Unterwasseroberfläche nicht wieder durch das alte Kiesmuster ersetzt.

Hangneigung steuert den Anteil freiliegenden Felses. Ruhige großräumige Flecken mischen Sand und Kies; mit zunehmender Tiefe wird der Kiesanteil geringer. Das ist eine feste künstlerische Verteilungsregel, keine Sedimenttransport-Simulation. Beim Felsobjekt wird ausschließlich die Felsschicht verwendet. Die sandige Quelle wird teilweise entsättigt; Unterwassernässe dunkelt die Farbe ab und beeinflusst Rauheit. Metallanteil ist null. Die vorhandene vereinfachte dynamische Caustic-Modulation bleibt als schwacher Farbmultiplikator erhalten; der Boden leuchtet nicht selbst.

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
