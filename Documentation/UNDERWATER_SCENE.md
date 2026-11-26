# Unterwasserwelt: Arbeit am visuellen Ziel

Maßstab ist die [vom Nutzer bereitgestellte Qualitätsreferenz](VISUAL_TARGET.md), nicht die Abnahme einzelner Modelle. Der aktuelle Stand erreicht diese Gesamtreferenz noch nicht.

## Größenstufen des Seegrunds

Die drei großen Felsformen ergänzen bereits den bisherigen Grund. Eine weitere Größenstufe ist im Entwicklungsstand integriert: `create_gravel_assets.py` leitet aus denselben dokumentierten CC0-Formen drei geschlossene Steinmeshes mit je 1.200 Dreiecken ab. Die fertigen Formen werden nach der Vereinfachung erneut zentriert und auf einen bekannten Radius normiert. Originale und große Felsen bleiben erhalten. `GravelAssets.blend` und `blender/exports/Gravel/Manifest.json` halten die Ableitung fest.

`build_gravel_content.py` importiert sie separat mit vier Detailstufen, vollständiger Nanite-Ersatzgeometrie und je einer einfachen konvexen Kontakthülle. Ein eigenes Material nutzt die vorhandenen CC0-Kieskarten des Seegrunds. Die tatsächlichen Rasterstufen enthalten 1.200 / 420 / 120 / 64 Dreiecke; Unreal erhält bei der letzten Stufe mehr als die angeforderten 30 Dreiecke.

CPU-Platzierung und Bodenmaterial verwenden dieselbe räumliche Patch-Funktion mit ganzzahligem Hash. Das erzeugt unregelmäßige Kiesfelder und freiere Sedimentflächen ohne regelmäßiges Raster. Die Steine orientieren sich an der Vermessung, liegen teilweise im Boden und meiden große Felsen. Ihre horizontalen Halbachsen reichen bis 24,5 cm; die Halbachse entlang der Bodennormalen ist auf 6 cm begrenzt. Die Verteilung bleibt gestaltet und darf nicht als lokale Sedimentmessung bezeichnet werden.

Der Nahbereich umfasst höchstens neun Zellen von jeweils 32 m mit jeweils 4.096 Platzierungskandidaten und drei Instanzkomponenten. Zeichnung endet nach 30 m. Beim Schwimmen wird eine neue Zelle pro Bild aufgebaut; Einstiege, größere Ortswechsel und die feste Vergleichsansicht laden das vollständige Fenster. Beibehaltene Zellen behalten ihre Komponenten, erneut besuchte Zellen erhalten dieselben Transformationen.

## Bisherige Sichtprüfung und Grenzen

Der erste native Vergleich (`artifacts/gravel-first-comparison/`) zeigte zu wenige, auffällig helle Steine. Im zweiten Vergleich (`artifacts/gravel-refined-comparison/`) passen die verwendeten Kieskarten und die höhere Dichte besser zum Grund. Beide Vergleiche enthalten dieselben zwei Kameras mit ausgeblendeten und eingeblendeten Steinen. Das sind Aufnahmen aus dem Unreal-Entwicklungsstand; die bereits veröffentlichte Vorschau `WindowsRockDetail` enthält diese Änderung noch nicht.

Die abschließende native Prüfung umfasst fünf Fälle und elf Bilder: jeweils einen Streaminglauf für Weesen, Murg und Lediwracks sowie die zwei Kameras mit ausgeblendeten/eingeblendeten Steinen für Nanite und Rasterdarstellung. Alle fünf Laufprüfungen bestehen. Prüfsummen, Buildprotokolle und genaue Testmarker stehen in [GRAVEL_INTEGRATION_RESULTS.json](GRAVEL_INTEGRATION_RESULTS.json).

| Einstieg | Steine im Ausgangsfenster | erfolgreiche Kontaktproben |
| --- | ---: | ---: |
| Weesen | 5.208 | 192 |
| Murg | 5.941 | 196 |
| Lediwracks | 4.743 | 144 |

Jeder Rückwegtest erhält sechs bestehende Zellen unverändert und stellt neun entladene Zellen mit identischen Transformationen wieder her. Der Aufbau über mehrere Bilder wird ausdrücklich mit sieben, acht und neun aktiven Zellen geprüft. Die Verteilungstests prüfen Grenzen, Tiefenabnahme, freie und dichte Bereiche sowie kontinuierliche Übergänge. Die Editorstarts enthalten weiterhin den bereits im vorigen Fels-Test vorhandenen ToolsetRegistry-Pythonfehler und 17 frühe `Condition failed`-Meldungen. Diese sind im Bericht erhalten; die nativen Spielprüfungen sind keine Aussage über bestandene Engine-Selbsttests.

Entwicklungsansicht ohne neue Steine:

![Gleiche Kamera, Kiesgeometrie ausgeblendet](Images/Gravel_Hidden.png)

Entwicklungsansicht mit neuen Steinen – weiterhin deutlich unter der Gesamtqualität der Nutzerreferenz:

![Gleiche Kamera, Kiesgeometrie eingeblendet](Images/Gravel_Visible.png)

Die visuelle Gesamtabnahme ist weiter offen: zu einfache Pflanzenformen und -oberflächen, fehlende mittlere Geröllstrukturen sowie die gemeinsame Wirkung von Licht, Bewuchs und Wasser. Die endgültige Abnahme in Bewegung und ein neues eigenständiges Spielpaket stehen aus. Rechenzeiten aus parallel laufender Entwicklung, Langzeittest und zeitweise geöffneter Nutzervorschau sind keine isolierte Bildratenmessung.

## Licht und Wasseroberfläche

Die normale Aufnahme `artifacts/rock-validation-20261126-113456/stream0/DiveUnderwater_0_stream.png` zeigt eine sehr dunkle Wasseroberfläche bei geringer Tiefe. Das ist zunächst ein visueller Befund; Schattenlage, Belichtung, Volumen und Oberfläche sind getrennt zu untersuchen.

Ein konkreter technischer Ansatzpunkt ist der installierte UE-5.8-Renderpfad: `Engine/Shaders/Private/BasePassPixelShader.usf` setzt bei Single Layer Water `CameraIsUnderWater = false` (Zeile 1698); `SingleLayerWaterComposite.usf` tut dies ebenfalls (Zeile 68). `SingleLayerWaterShading.ush` berechnet mit diesem Zustand die optische Weglänge hinter der Oberfläche statt zwischen Unterwasserkamera und Oberfläche. Unser bisheriges zweiseitiges Single-Layer-Water-Material verwendet auch für die Unteransicht diesen Pfad. Eine korrekte Unteransicht ist damit nicht allein durch die vorhandenen Materialparameter belegt.

Der nächste kontrollierte Vergleich muss die Unteransicht bei festem Sonnenstand, Kamera, Sichtweite und Belichtung isolieren. Das geforderte Ergebnis umfasst einen nachvollziehbaren Blick durch die Oberfläche, winkelabhängige Reflexion, dieselbe Wellengeometrie und einen konsistenten Übergang beim Auftauchen. Eine pauschale Erhöhung von Helligkeit oder Türkisanteil ersetzt diese Prüfung nicht. Nacht, größere Tiefe und trübe Bedingungen bleiben Teil der Abnahme. Die Engine-Dateien werden für diese Untersuchung nur gelesen; bisher wurde kein eigener Unterseiten-Renderpfad umgesetzt.
