# Feinere Geländeform gegenüber Murg

Stand: 28. November 2026. Eine 2048 × 2048 Meter grosse Bergkachel gegenüber Murg ist als separate **2-Meter-Geometrie** aufgebaut und mit der bisherigen 8-Meter-Geometrie verglichen. In den Bildern werden Felskanten und Rinnen feiner gegliedert. Die gestreckte Luftbildfarbe und harte Materialübergänge bleiben sichtbar. **Dies ist eine lokale Gelände- und Bodenstudie, noch keine Übernahme in das normale Spiel.**

## Belegter Detailverlust in der bisherigen Aufbereitung

Der neue Prüflauf verifiziert alle 286 ursprünglichen swissALTI3D-Dateien und vergleicht ihre 2-m-Pixelzentren mit der tatsächlichen Dreiecksinterpolation des 8-m-Ausgangsmeshs. Er verwendet dieselbe Diagonale wie der Meshbau, keine bilineare Ersatzfläche. Drei analytische Tests prüfen Ebene, Diagonale und Randbereich. Die ursprüngliche 8-m-Fläche besteht aus 4 × 4 Flächenmitteln der Quelldaten.

| Bereich | Geprüfte Punkte | Median der absoluten Höhendifferenz | P95 | P99 |
| --- | ---: | ---: | ---: | ---: |
| Land über 421 m LN02 | 65’272’585 | 0,20 m | 1,52 m | 4,49 m |
| Kartierter Fels / lockerer Fels | 3’413’992 | 1,03 m | 7,83 m | 15,80 m |
| Nördliches Seeufer, kartierter Fels | 711’508 | 1,34 m | 10,30 m | 21,56 m |

Das sind **vertikale Unterschiede zur Datenquelle**. An sehr steilen Flächen bedeuten sie weder denselben senkrechten Abstand zur Oberfläche noch einen entsprechend grossen sichtbaren Bildfehler. Die 2-m-Quelle ist ebenfalls ein Höhenfeld und enthält keine vermessenen Überhänge. Der nördliche Teilbereich ist ein ausdrücklich definiertes geografisches Fenster, keine Sichtbarkeitsprüfung. [Vollständiger Datenvergleich mit Grenzen und Quellen](LANDSCAPE_RESOLUTION_AUDIT.json).

## Vergleich im Spielrenderer

Murg/West, 21. Juni um 12:00 Uhr, 1600 × 900, jeweils zwölf Sekunden Vorlauf, normaler Hardware-/VSM-Wasserpfad. Die Kamera und das Material bleiben innerhalb eines Paares gleich.

Kartiertes Felsmaterial, bisherige 8-m-Geometrie:

![Kartierter Fels mit 8-m-Geometrie](Images/Terrain_Murg_Mapped8m.png)

Dasselbe Material mit der einzelnen feineren Bergkachel:

![Kartierter Fels mit lokaler 2-m-Geometrie](Images/Terrain_Murg_Mapped2m.png)

Die feineren Kanten und Rinnen innerhalb der Kachel zeigen einen sichtbaren Beitrag der Geländeauflösung. Andere Teile der Berglandschaft bleiben beim bisherigen Stand; ein durchgehend neues Panorama wird nicht behauptet.

Die zweite Gegenprobe hält das bisherige Luftbildmaterial fest:

![Luftbild mit 8-m-Geometrie](Images/Terrain_Murg_Photo8m.png)

![Luftbild mit lokaler 2-m-Geometrie](Images/Terrain_Murg_Photo2m.png)

Hier bleibt die senkrechte Farbstreckung trotz feinerer Geometrie deutlich bestehen. Materialprojektion und Geländeform müssen daher gemeinsam weiterbearbeitet werden. Die [kartierte Materialstudie](LANDSCAPE_CLIFF_COVERAGE.md) hat zusätzlich offene Helligkeitsgrenzen; die [visuelle Referenz](VISUAL_TARGET.md) ist nicht erreicht.

## Umsetzung und Prüfungen

Die separate Kachel `SM_LandDetail_05_02` liegt vollständig im Berggebiet, mindestens rund 157 Meter über dem Szenariowasserspiegel. Sie verwendet 1’050’625 Vertices und 2’097’152 Dreiecke als Nanite-Quelle. Die vorhandenen 2-m-Höhen sind bilinear auf das an die bisherige Kachel ausgerichtete 2-m-Vertexraster abgebildet. Innerhalb eines 32-m-Randes gehen Höhen und Normalen zur bisherigen Kante über. Es wurde kein erfundenes Felsrauschen auf die Geländeform addiert.

Der native Meshbau und eine zweite Prüfung nach erneutem Laden des gespeicherten Assets bestehen. Sämtliche Quellvertices, UVs und Normalen sind geprüft. Die grösste Randabweichung beträgt **0,013916 cm**, die grösste Abweichung zur vorbereiteten Höhe **0,007812 cm**. Diese Werte betreffen Quellgeometrie und gespeicherte Attribute, nicht eine lückenlose Prüfung aller während Bewegung gewählten Nanite-Stufen.

Vier native Renderläufe bestehen und alle vier Bilder sind gesichtet. Bei aktiver Studie wird genau eine von weiterhin 77 Landschaftskacheln ersetzt. Die Materialauswahl, der Wasserpfad sowie die bestehenden Uferanschlüsse und Uferkollisionen sind bestätigt. Alle **77 bisherigen Landschaftsassets, 44 Materialien und 53 Laufzeitdateien des eingefrorenen R3-Pakets** sind bytegleich erhalten. [Prüfbericht mit archivierten Eingaben, Bildern und Protokollen](LANDSCAPE_DETAIL_STUDY_RESULTS.json).

Der erste Bau erforderte eine erneute Erfassung der Quelldateien durch UnrealBuildTool; dafür unterstützt `Build.ps1` jetzt `-RefreshSources`. Ein Namenskonflikt und ein anschliessender Include-Fehler wurden korrigiert. Beim ersten Meshlauf war die generische SHA-256-Plattformfunktion dieser Windows-Engine nicht implementiert. SHA-256 bleibt deshalb die Offline-Identitätsprüfung; die native Laufzeit verwendet zusätzlich Core-SHA-1-Prüfungen gegen veraltete oder beschädigte Eingaben. Die fehlgeschlagenen Versuche bleiben im Prüfbericht erhalten; sie zählen nicht zu den bestandenen Läufen.

## Grenzen vor einer spielbaren Übernahme

Die isolierte Editorstudie verwendet jetzt für die Bodenstützung dieselben 2-m-Dreiecke wie das neue Quellmesh. Ausserhalb der Kachel ergänzt sie im bisher nicht unterstützten Binnenland eine Abfrage der bestehenden 8-m-Dreiecke. **Korrektur zum ersten Bericht:** Im normalen Spiel war die Bodenabfrage auf Seegrund, Ufer und Küstenmodell beschränkt; das gesamte Binnenland war nicht bereits als 8-m-Fläche begehbar. Der neue Binnenlandpfad wird nur zusammen mit der geprüften Detailstudie aktiviert.

Der native Abgleich prüft zwei baryzentrische Punkte je gespeichertem Dreieck: **4’194’304 Abfragen**, höchstens **0,015625 cm** Abweichung. Weitere **49’164 Randabfragen** liegen auf sowie 1 mm beiderseits der vier Kachelgrenzen; die grösste Differenz zur alten Fläche beträgt **0,015887 cm**. Die Bodenhöhe an allen drei bestehenden Einstiegen bleibt bitgleich.

Eine native Gehprobe verläuft von 8 m ausserhalb des Westrands bis 44 m innerhalb und zurück. Sie durchquert damit auch den 32-m-Übergang. Nach einmaliger Startplatzierung bewegen Vorwärts-/Rückwärtseingaben die Spielfigur über die normale Bewegungssimulation; Stützhöhe und Gehzustand werden währenddessen geprüft. Rund **105 m** wurden ohne Verlust der Bodenstützung zurückgelegt. Dies prüft die Höhenstützung, **keine vollständige Körperkollision mit Bergwänden**: Die Landschaftskomponenten bleiben `NoCollision`. Der numerische Vergleich betrifft die Quellfläche, nicht jede zur Laufzeit gewählte Nanite-Vereinfachung.

Die Bildprüfung verhindert eine vorschnelle Freigabe: Unmittelbar nach dem Versetzen der Testfigur erscheint ein himmelfarbener Spalt. Die neue und die alte Geometrie zeigen am festen Blickpunkt mit Nanite nach zwölf Sekunden Vorlauf keinen solchen Spalt. **Mit deaktiviertem Nanite zeigt die Detailkachel dagegen auch nach dem Vorlauf eine deutliche offene Kante.** Auch die bisherige 8-m-Ersatzdarstellung zeigt rechts einen kleineren Spalt; die feinere Kachel vergrössert ihn deutlich. Dieser Fehler der Ersatzdarstellung ist noch offen; ein Zusammenhang mit dem ersten kurzzeitigen Spalt ist plausibel, aber nicht bewiesen. Die vollständige Bewegungsfolge wurde nicht Bild für Bild gesichtet. [Neuer Prüfbericht zur Bodenstützung und zur offenen Ersatzdarstellung](LANDSCAPE_GROUND_STUDY_RESULTS.json).

Fester Blickpunkt mit Nanite nach zwölf Sekunden Vorlauf:

![Geschlossene Oberfläche im geprüften Nanite-Blick](Images/Terrain_Ground_Nanite.png)

Derselbe Blick mit deaktiviertem Nanite; die offene Kante bleibt sichtbar:

![Offene Kante in der Ersatzgeometrie](Images/Terrain_Ground_FallbackGap.png)

## Korrektur der konventionellen Ersatzgeometrie

Die oben dokumentierten Bilder bleiben als Vorher-Zustand erhalten. Die tatsächlichen LOD0-Renderpuffer zeigen, dass die bisherige Vereinfachung die offenen Kachelränder verschiebt. Eine unabhängige Auswertung prüft 4’065 Schnittpunkte im Abstand von 0,5 m an der gemeinsamen Kante, unter Ausschluss von je 8 m an den Enden. Im bisherigen 8-m-Paar beträgt die grösste räumliche Trennung über diese gesamte Kante 6,35 m, beim alten Westnachbarn mit der feinen Ostkachel 4,08 m. Diese Maxima sind keine Messung der Breite des lokalen Spalts im Screenshot.

Die isolierte Korrektur bewahrt sämtliche offenen Randvertices und Randsegmente während der Vereinfachung. Das ursprüngliche Quellmesh bleibt unverändert als HiRes-Geometrie für Nanite erhalten; das eigens erzeugte LOD0 dient als konventionelle Ersatzdarstellung. Unreal vereinfacht dieses LOD0 nicht erneut. Die 10-cm-Toleranz begrenzt den geprüften Kollapspunkt während der Vereinfachung; sie ist **keine nachgewiesene globale Oberflächen- oder Kollisionsfehlerschranke**.

Native Prüfungen vor dem Speichern und nach erneutem Laden bestätigen für alle drei Testassets die erhaltenen Randvertices und offenen Randsegmente in den Renderpuffern sowie die unveränderten HiRes-Positionen und Anzahlen. Die unabhängige Schnittprüfung ergibt jetzt **0 cm** im korrigierten 8-m-Paar und höchstens **0,01171875 cm** zwischen korrigiertem Westnachbarn und feiner Ostkachel, ohne horizontale Trennung. Vier analytische Tests prüfen unter anderem geteilte Rendervertices, kollineare Unterteilungen und fehlende Schnittabdeckung.

Alle drei neuen festen Vergleichsbilder sind gesichtet: alte Geometrie mit korrigiertem Fallback, feine Geometrie mit korrigiertem Fallback und feine Geometrie mit Nanite. Der zuvor sichtbare Spalt fehlt nach zwölf Sekunden Vorlauf auch bei ausgeschaltetem Nanite:

![Korrigierte Ersatzgeometrie ohne sichtbaren Spalt am festen Testblick](Images/Terrain_Ground_FallbackClosed.png)

Die Korrektur gilt bisher für **zwei Kachelpositionen**, mit drei Assets für die alte und feine Variante. Die anderen 75 Kacheln und die Anschlüsse zu deren unveränderten Ersatzmeshes sind damit nicht allgemein repariert. Die Landschaft ist weiterhin eine Editorstudie. [Prüfbericht mit tatsächlichen Renderpuffern, Eingaben und Messläufen](LANDSCAPE_FALLBACK_STUDY_RESULTS.json).

Die erhaltenen Grenzen kosten zusätzliche Geometrie: Die Fallback-Dreieckszahlen steigen von 13’159 auf 88’350 im Westnachbarn, von 20’514 auf 114’332 in der alten Ostkachel und von 49’921 auf 588’570 in der feinen Ostkachel. Das feine korrigierte **Editorasset umfasst 507’973’110 Bytes**, gegenüber 41’972’228 Bytes des bisherigen feinen Assets. Diese Dateigrösse ist keine Messung des gekochten Pakets oder des GPU-Speichers. Eine flächendeckende Übernahme ist damit noch nicht gerechtfertigt.

`LandscapeStudy` ist in den Projekteinstellungen ausdrücklich vom Cook ausgeschlossen, damit der allgemeine Cook-Eintrag für `/Game/Diving` die experimentellen Meshes nicht in das nächste normale Paket aufnimmt. Die Auswahl im Spielcode ist ausschliesslich für Editor-Builds verfügbar. Ein neuer Paketbau zur Überprüfung dieses Ausschlusses steht noch aus; das eingefrorene R3-Paket bleibt unverändert.

Vier aufeinanderfolgende 60-s-Messungen am Einstieg Murg mit umlaufender Blickrichtung, 15 s Vorlauf, 1920 × 1080, Qualitätsstufe 2, 100 % Renderauflösung und normalem Wasserpfad ergeben:

| Darstellung der feinen Kachel | FPS Mittel | 1-%-Low | Langsamster Frame | Max. Prozessspeicher |
| --- | ---: | ---: | ---: | ---: |
| Bisheriger Fallback, Nanite aus | 47,72 | 18,61 | 64,82 ms | 9’700,8 MiB |
| Korrigierter Fallback, Nanite aus | 47,53 | 18,51 | 61,86 ms | 10’423,9 MiB |
| Korrigiertes Asset, Nanite an | 76,79 | 61,08 | 34,42 ms | 10’638,4 MiB |
| Bisheriges Asset, Nanite an | 77,30 | 61,44 | 30,73 ms | 9’738,6 MiB |

Die Mittelwerte liegen innerhalb der jeweiligen Darstellungsart nahe beieinander. Der Prozessspeicher ist mit der Korrektur in diesen Läufen rund 723 beziehungsweise 900 MiB höher. Es ist jeweils **nur eine Messung**, in fester Reihenfolge und mit unterschiedlicher Hintergrundlast. Dies belegt weder einen exakten kausalen Mehrbedarf noch eine allgemeine Leistungsfreigabe. Alle inneren Frames einschliesslich Aussetzern bleiben in der Auswertung; nur die beiden CSV-Randframes werden wie im vorhandenen Analysewerkzeug ausgeschlossen. Die vier abschliessenden Benchmarkbilder sind gesichtet. Die zuvor beobachteten stärkeren CPU-/GC-Aussetzer gelten durch diese kurzen Läufe nicht als behoben. Vor einer Ausweitung muss besonders der Speicherbedarf der Studienassets reduziert beziehungsweise erklärt werden.

Die wiederholte Gehprobe mit korrigierten Assets besteht über **105,463 m und 105 protokollierte Stützpunkte**. Alle drei gespeicherten Bilder sind gesichtet. Das erste Bild direkt nach der einmaligen Startplatzierung zeigt mit aktivem Nanite weiterhin einen deutlichen Spalt; die Bilder innerhalb der Kachel und nach der Rückkehr zeigen ihn nicht. Die Dauer und genaue Ursache sind nicht gemessen. Damit ist ausdrücklich belegt: **Die Fallback-Korrektur allein behebt diesen Startbefund nicht.** Eine Untersuchung der während des Nachladens gewählten Nanite-Geometrie bleibt notwendig; sie darf nicht durch einen verlängerten Vorlauf aus der Fehlerbilanz verschwinden.

![Weiterhin offener Nanite-Startbefund trotz korrigierter Ersatzgeometrie](Images/Terrain_Ground_NaniteStartGap.png)

Die anschliessende [stationäre Nanite-Gegenprobe](LANDSCAPE_NANITE_BOUNDARY.md) reproduziert den Startspalt ohne Bewegung und zeigt eine bereits im ersten Bild geschlossene Kante bei vollständig residenten Nanite-Daten der beiden Testkacheln. Damit gibt es einen geprüften lokalen Lösungsansatz; eine allgemeine Übernahme und die Speicherstrategie stehen weiterhin aus.

Lückenlose bewegte Sichtprüfung, weitere Sonnenstände und Paketvalidierung bleiben offen. Die zuvor beobachteten CPU-/Speicherbereinigungs-Aussetzer sind weiterhin offen; die alten FPS-Werte gelten nicht als Messung dieser neuen Geometrie. Waldvolumen, Wiesen, Nahbereich und Materialübergänge bleiben eigenständige Aufgaben. Nach Bewegungs- und Leistungstests sind die Kosten der Korrektur zu bewerten, Fels-/Vegetationsübergänge zu verbessern und die CPU-Aussetzer einzugrenzen. Die fünf Unterwasser-Abschlussrunden bleiben abgeschlossen.

## Reproduktion

```powershell
./Tools/.geodata-env/Scripts/python.exe Tools/prepare_landscape_detail.py
./Tools/.geodata-env/Scripts/python.exe -m unittest discover -s Tests -p test_landscape_resolution.py
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Editor -MaxParallelActions 4 -RefreshSources
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target LandscapeDetail
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target LandscapeDetailCheck
./Tools/SmokeTest.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -ProductionWater -Scenario shore-preview -Entry 1 -Hour 12 -Date 2026-06-21 -MaxFPS 30 -ShorePreviewSeconds 12 -FarCliffStudy -LandscapeDetailStudy
```

Für die Geometriekontrolle `-LandscapeDetailStudy` weglassen; für das Luftbildpaar zusätzlich in beiden Läufen `-FarCliffStudy` weglassen. Die Quelldateien stammen aus dem vorhandenen verifizierten Geodatensatz. Die Aufbereitung verändert weder `landscape.f32` noch die alten Landschaftsmeshes.

Die Bodenprüfung ist in `LandscapeDetailCheck` enthalten. Die zusätzliche Bewegungsprüfung und der feste Vergleichsblick verwenden:

```powershell
./Tools/SmokeTest.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -ProductionWater -Scenario land-detail -Entry 1 -Hour 12 -Date 2026-06-21 -MaxFPS 30 -LandscapeDetailStudy -FarCliffStudy -TimeoutSeconds 240
./Tools/SmokeTest.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -ProductionWater -Scenario shore-preview -Entry 1 -Hour 12 -Date 2026-06-21 -MaxFPS 30 -ShorePreviewSeconds 12 -LandscapeDetailView -LandscapeDetailStudy -FarCliffStudy
```

Für den festen 8-m-Vergleich `-LandscapeDetailStudy` weglassen; für die konventionelle Ersatzgeometrie zusätzlich `-DisableNanite` setzen. Diese Aufnahmen sind Funktionstests, keine Leistungsmessungen.

Die isolierte Randkorrektur wird separat gebaut und geprüft:

```powershell
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target LandscapeFallback
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target LandscapeFallbackCheck
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target LandscapeFallbackAudit -BoundaryStudy
./Tools/.geodata-env/Scripts/python.exe Tools/audit_landscape_fallback.py Builds/SourceEditorSandbox/divingsim/Saved/LandscapeFallbackAudit --output artifacts/fallback-audit.json --require-closed
```

Bei Bild-, Geh- und Leistungstests aktiviert `-LandscapeFallbackStudy` die korrigierten Ersatzmeshes. Für den ursprünglichen Pufferexport bei `LandscapeFallbackAudit` den Schalter `-BoundaryStudy` weglassen. Die beiden Exporte vor einem erneuten Lauf getrennt archivieren: Der Commandlet verwendet dasselbe Ausgabeverzeichnis.
