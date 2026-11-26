# Felsoberflächen mit passenden Scandetails

Die drei vorhandenen Felsformen verwenden nun jeweils die zugehörigen Farb-, Normalen- und AO/Rauheitskarten. Die bisherige gemeinsame, weltprojizierte Steintextur passte nicht zu ihren Rissen und Vertiefungen und erschien bereits ohne Beleuchtung auffällig weiss. Die UV-Koordinaten der importierten Modelle sind erhalten; alle drei besitzen UV0. Die Normalenkarten werden über Unreals normalen Texture-Sample-Knoten als Tangent-Space-Normalen verarbeitet.

Das spielbare Paket liegt unter **`Builds/WindowsRockSurfaces/divingsim.exe`**, gebaut aus `f0e8fb34e7573b0ac3fab9fe2fe6557ad0353fbf`. Neun native Fälle mit 45 Aufnahmen und acht Paketfälle mit 37 Aufnahmen bestehen, ebenso der tatsächliche Root-Launcher-Start. [Native Ergebnisse](ROCK_SURFACES_NATIVE_RESULTS.json), [Paket, Launcher und Leistungsmessungen](ROCK_SURFACES_RESULTS.json). Der Paketbuild meldet `BUILD SUCCESSFUL` in `artifacts/rock-surfaces-package-build.log`.

Bei 1080p/Hoch erreicht der 60-Sekunden-Unterwasserlauf 73,17 FPS im Mittel und 62,04 FPS im langsamsten Prozent. Die 90-Sekunden-Nachladestrecke erreicht 100,79 / 79,80 FPS, enthält jedoch einen Ruckler mit 36,82 ms. [Messbedingungen und Grenzen](PERFORMANCE.md). Dies ist weder die vollständige visuelle Abnahme noch ein neuer Langzeittest.

## Herkunft und Gestaltung

Die Karten stammen aus denselben Poly-Haven-Scans wie die [vorhandene Geometrie](ROCK_GEOMETRY.md): [Boulder 01](https://polyhaven.com/a/boulder_01), [Namaqualand Boulder 05](https://polyhaven.com/a/namaqualand_boulder_05) und [Namaqualand Boulder 06](https://polyhaven.com/a/namaqualand_boulder_06). Die Assets sind nach der [Anbieterlizenz CC0](https://polyhaven.com/license) verwendbar. [ROCK_SURFACES.json](ROCK_SURFACES.json) hält Autoren, Downloadadressen, Anbieter-MD5, Dateigrössen und SHA-256 fest. Die neun unveränderten 2K-PNGs umfassen zusammen 161.554.924 Bytes.

Es sind keine geologischen Aufnahmen des Walensees. Insbesondere bleiben die Herkunft der südafrikanischen Formen und die dort aufgenommenen Oberflächenspuren eine gestalterische Annäherung. Die Farbsättigung wird im Material auf 70 % gemischt. Die vorhandenen weltfesten nassen Beläge, Sedimentablagerungen und die tiefenabhängige Bewuchsmischung werden aus dem gemeinsamen Lakebed-Shader übernommen. Das ist eine künstlerische Anpassung, keine Messung lokaler Mineralien oder Biologie. Die Originaldateien werden nicht umgefärbt.

Die drei neuen Materialien `M_RockScan1..3` sind undurchsichtig, einseitig und für Nanite sowie instanzierte Geometrie freigegeben. Farbtexturen verwenden sRGB; Normalen- und ARM-Karten lineare Daten. Das ARM-Blau wird nicht als Metallanteil verwendet: Die Felsen bleiben Dielektrika. Die neun Texturen nutzen normale Mipmaps und Texturstreaming mit maximal 2.048 Pixeln, da diese statischen Meshes importierte UVs besitzen. Die bisherige Sonderbehandlung der prozeduralen Bodenflächen bleibt davon getrennt.

Geometrie, LODs, Platzierung, physische Abmessungen, Ausschlussradien und Kollisionshüllen ändern sich nicht. Die Streaming-Zellen übernehmen das Material ihrer jeweiligen Felsvariante. Der Streamingtest kontrolliert zusätzlich, dass diese Materialzuordnung erhalten bleibt. `-DiveLegacyRockSurface` aktiviert ausschliesslich in Smoke-Test-Läufen die vorherige gemeinsame Oberfläche. Im festen Formvergleich erhält auch die alte Kugelform weiterhin das alte Material; die drei Scanformen erhalten jeweils ihre passende Oberfläche.

## Prüfung und Grenzen

Die unbeleuchtete Vorprüfung isoliert die Materialwirkung. Sie ist kein farbmetrischer Nachweis: Der Unlit-Modus verändert die Renderingkette. Die anschliessende beleuchtete Unterwasseransicht zeigt zusammenhängendere Bruchkanten und Vertiefungen. Für einen kontrollierten Vergleich verwenden alter und neuer Zustand dieselbe Geometrie, Kamera, Tageszeit und Wasserbeleuchtung.

`Tools/TestRockSurfaces.ps1` prüft Formen vorher/nachher, Rasterdarstellung ohne Nanite, die Unterwasserumgebung mit beiden Materialvarianten im begrenzten Wasservolumen, die normale Spielbeleuchtung, Nachladen der Umgebung und einen Nachtfall mit Lampe. Im nativen Editor kommt eine unbeleuchtete Ansicht hinzu. Jeder Fall kontrolliert die tatsächlich geladenen Materialpfade und frische Aufnahmen. `Tools/ReportRockSurfaces.py` bindet die Ergebnisse an Quellstände, Materialien, Texturen und gegebenenfalls das ausführbare Paket.

Die Gesamtgrafik ist damit noch nicht auf dem [gewünschten Referenzniveau](VISUAL_TARGET.md). Pflanzen, Bodenvariation, natürliche Übergänge, Reflexionen und Lichtverteilung benötigen weitere Arbeit. Das begrenzte Wasservolumen ist weiterhin ein eigener experimenteller Modus. Die neue Felsoberfläche verbessert die bestehende Spielwelt auch mit deren normaler Wasserbeleuchtung; sie aktiviert diesen Modus nicht automatisch.

## Bildvergleich

Vorher, gemeinsame Steinprojektion im Windows-Paket:

![Bisherige Felsoberfläche](Images/RockSurface_Before.png)

Danach, dieselbe Form und Kamera mit passenden Scandetails:

![Fels mit passenden Scandetails](Images/RockSurface_After.png)

## Reproduktion

```powershell
./Tools/.geodata-env/Scripts/python.exe Tools/prepare_rock_surfaces.py
./Tools/Build.ps1 -EditorSandbox
./Tools/Build.ps1 -EditorSandbox -Target Content -OnlyRockSurfaces
./Tools/TestRockSurfaces.ps1
```

Der vollständige Content-Build, `-OnlyRocks` und `-OnlyBenthic` bauen ebenfalls die passenden Oberflächen. Der reine Geometrie-Audit `-DiveRockAuditOnly` bleibt ohne diese Materialänderung.
