# Pflanzenfarben: Korrektur der FBX-Farbkodierung

Die Pflanzenoberfläche dekodiert die importierten RGB-Vertexfarben jetzt einmal von sRGB nach linear. Damit verwendet sie wieder die im Blender-Modell vorgesehenen Reflexionsfarben. Zuvor wurden die helleren, kodierten Werte direkt als lineare Materialfarbe verwendet. Die Korrektur gilt für Laichkraut, Armleuchteralgen und Tausendblatt, einschliesslich der aus derselben Farbe abgeleiteten Lichtdurchlässigkeit.

## Nachgewiesene Ursache

Die Untersuchung verbindet drei tatsächliche Datenquellen: die gespeicherten Farbattribute in `blender/AquaticPlants.blend`, die vorhandenen FBX-Dateien und die LOD0-Farbbuffer der nativen Unreal-Laufzeit. Sie vergleicht Komponentenbereiche; sie behauptet keine Prüfung jeder Vertexzuordnung oder eine Messung realer Pflanzenreflexion.

| Pflanze | Grünmaximum in Blender, linear | Grünmaximum im FBX, sRGB | Unreal-Pufferbyte | Nach Dekodierung |
| --- | ---: | ---: | ---: | ---: |
| Laichkraut | 0,21340 | 0,49928 | 127 | 0,21223 |
| Armleuchteralgen | 0,18193 | 0,46370 | 118 | 0,18116 |
| Tausendblatt | 0,18938 | 0,47241 | 120 | 0,18782 |

Die kleinen Restabweichungen entstehen durch den 8-Bit-Farbpuffer. Der vollständige [Farbaudit](PLANT_COLOR_AUDIT.json) enthält RGB- und Alpha-Bereiche, Fehlergrenzen sowie Prüfsummen von Blender-Datei, FBX, Meshes, Shader, Material und Laufzeitprotokoll.

Die lokale Unreal-5.8-Quelle bestätigt den Weg: `FbxStaticMeshImport.cpp` liest die FBX-Werte als Farbbytes und wandelt sie in lineare Mesh-Attribute um; `StaticMeshBuilder.cpp` schreibt sie mit `ToFColor(true)` wieder kodiert in den GPU-Puffer. `LocalVertexFactory.ush` reicht diese Werte als VertexColor ohne RGB-Dekodierung an das Material weiter. `AquaticPlant.ush` übernimmt deshalb die Rückwandlung vor der bisherigen Blattoberflächenberechnung.

Der Alpha-Kanal bleibt linear: Er enthält die Wurzel-/Höhengewichte für die Bewegung. FBX-Dateien, Pflanzenmeshes, Detailstufen, UV-Koordinaten, Platzierung und Dichte wurden für diese Korrektur nicht neu erzeugt. Der Generator nennt `colors_type='SRGB'` jetzt ausdrücklich, statt sich auf den bisherigen Blender-Standard zu verlassen. Der neue Material-Build kann die Oberfläche allein aktualisieren.

## Visueller Vergleich und Grenzen

Die Vergleiche verwenden dieselben nativen Kameras, Tageszeit, Datum und Wasserparameter. Beim Modellvergleich wird nur das gesicherte bisherige Material gegen das korrigierte Material getauscht. Die alte Materialdatei wird danach wieder durch die korrigierte ersetzt. Die Bilder sind unveränderte Unreal-Aufnahmen.

Vorher:

![Laichkraut mit bisheriger Farbauswertung](Images/PlantColor_Before.png)

Korrigiert:

![Laichkraut mit dekodierten Reflexionsfarben](Images/PlantColor_After.png)

Die Blätter wirken weniger ausgebleicht. Das beseitigt einen konkreten Farbfehler, erreicht aber noch nicht die [visuelle Referenzqualität](VISUAL_TARGET.md). Harte Unterschiede zwischen beleuchteten und verschatteten Blättern, kleine flimmeranfällige Zweige, gleichförmige freie Sedimentflächen und die gemeinsame Unterwasserlichtwirkung bleiben offen. Die in Blender definierten Farben sind künstlerische Modellwerte, keine spektrometrisch vermessenen Pflanzenmaterialien.

Vorherige Diagnoseversuche mit zusätzlicher tiefenabhängiger Material-AO sowie ausgeschaltetem Pflanzenglanz bzw. ausgeschalteter Lichtdurchlässigkeit wurden verworfen und ihre Materialdateien vollständig zurückgesetzt. Sie sind nicht Bestandteil dieser Korrektur. Die AO-Variante dunkelte vor allem Schatten ab und stellte keinen physikalischen Wasserlichttransport her. Die getrennte [RGB-Kameraweg-Studie](SPECTRAL_VIEW.md) bleibt weiterhin ausdrücklich experimentell und im normalen Spielstart ausgeschaltet.

## Reproduktion

Das geprüfte Paket startet über `Builds/WindowsPlantColor/Start-WaterPreview.cmd`. Sieben Paketfälle mit 34 Aufnahmen und der automatische Launcher-Start bestehen. Zusätzlich sind 32 native Aufnahmen für Habitat-, Modell- und Farbpufferprüfungen erfasst. Die [Ergebnisdatei](PLANT_COLOR_RESULTS.json) bindet EXE, Inhaltscontainer, Quell-/Assetdateien, Logs und Bilder über Prüfsummen. Die native Vergleichsserie tauscht die Materialien; die Paketfälle verwenden durchgehend das korrigierte Material. Bestehende Engine-/Plugin-Startdiagnosen bleiben in den vollständigen Logs sichtbar.

Bei 1080p/Hoch erreicht das Paket im 60-Sekunden-Weesen-Lauf 69,79 FPS im Mittel und 58,22 FPS im langsamsten Prozent. Die 90-Sekunden-Murg-Nachladestrecke erreicht 97,66 FPS bei maximal 27,79 ms pro Frame. Beide Läufe bleiben ohne Frame über 33,33 ms. Der Murg-Blick zeigt jedoch offenes Wasser; daraus folgt keine Leistungszusage für dichte sichtbare Vegetation. [Messbedingungen und Grenzen](PERFORMANCE.md).

1. `Tools/Build.ps1 -Target Editor -EditorSandbox`
2. `Tools/Build.ps1 -Target Content -EditorSandbox -OnlyPlantMaterial`
3. `Tools/SmokeTest.ps1 -EditorSandbox -Scenario dive -WaterPreview -PlantColorProbe -RuntimeLog artifacts/PlantColorProbe.log`
4. Blender im Hintergrund mit `blender/AquaticPlants.blend` öffnen und `Tools/AuditPlantColors.py` ausführen; nach `--` die Argumente `--native-log artifacts/PlantColorProbe.log --output artifacts/PlantColorAudit.json` übergeben.
5. Paket bauen und `Tools/TestPlantColor.ps1 -PackageDirectory Builds/WindowsPlantColor` ausführen.

Die Farbpuffer-Prüfung läuft nur im Editor und nur bei explizitem Testschalter. Sie liest jedes Mesh einmal; im ausgelieferten Spiel entsteht dadurch kein zusätzlicher CPU-Pufferbedarf. Die Paketprüfung umfasst Nahansichten, Habitat, Ersatzdarstellung ohne Nanite, Nachtlampe, Uferablauf, Nachladen und den normalen Launcher-Start. Der Nachtlampentest prüft den allgemeinen Lichtkegel am Boden, keine beleuchtete Nahaufnahme jeder Pflanzenart.

Die Reihenfolge bleibt Unterwasserwelt, Überwasserlandschaft, danach [Menüführung, HUD und FPV](UI_UX_TARGET.md).
