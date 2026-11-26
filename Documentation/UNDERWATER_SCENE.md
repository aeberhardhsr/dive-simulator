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

Die Pflanzenformen und -oberflächen wurden inzwischen weiterentwickelt. Das neue eigenständige Paket `WindowsUnderwaterDetail` enthält Felsen, Kies und die drei neuen Pflanzengruppen. Zwölf Paketprüfungen mit 74 Aufnahmen sowie der normale Spielstart bestehen; Details und Paketidentität stehen in [UNDERWATER_DETAIL_RESULTS.json](UNDERWATER_DETAIL_RESULTS.json). Echte Vorher-/Nachher-Aufnahmen sind unter [Unterwasserbewuchs](AQUATIC_PLANTS.md) dokumentiert.

Die visuelle Gesamtabnahme ist weiter offen: fehlende mittlere Geröllstrukturen, zu vereinzelt wirkender Bewuchs und insbesondere die gemeinsame Wirkung von Licht und Wasser. Die Referenzqualität ist deutlich noch nicht erreicht. Der [zweistündige Murg-Lauf](SOAK.md) betrifft das ältere unveränderte Paket `WindowsFishSteering`; er wird nicht als Dauerlaufnachweis für die neuen Modelle ausgegeben. Rechenzeiten aus parallel laufender Entwicklung sind keine isolierte Bildratenmessung.

## Licht und Wasseroberfläche

Die weitergehende Arbeit an Pflanzenformen und ihren Oberflächen ist in [Unterwasserbewuchs](AQUATIC_PLANTS.md) dokumentiert. Die bisherigen groben Blätter und die neue Integration bleiben in kontrollierten Spielansichten vergleichbar.

Die normale Aufnahme `artifacts/rock-validation-20261126-113456/stream0/DiveUnderwater_0_stream.png` zeigt eine sehr dunkle Wasseroberfläche bei geringer Tiefe. Das ist zunächst ein visueller Befund; Schattenlage, Belichtung, Volumen und Oberfläche sind getrennt zu untersuchen.

Ein konkreter technischer Ansatzpunkt ist der installierte UE-5.8-Renderpfad: `Engine/Shaders/Private/BasePassPixelShader.usf` setzt bei Single Layer Water `CameraIsUnderWater = false` (Zeile 1698); `SingleLayerWaterComposite.usf` tut dies ebenfalls (Zeile 68). `SingleLayerWaterShading.ush` berechnet mit diesem Zustand die optische Weglänge hinter der Oberfläche statt zwischen Unterwasserkamera und Oberfläche. Unser bisheriges zweiseitiges Single-Layer-Water-Material verwendet auch für die Unteransicht diesen Pfad. Eine korrekte Unteransicht ist damit nicht allein durch die vorhandenen Materialparameter belegt.

Der kontrollierte Vergleich vom 26. November isoliert zwei getrennte Probleme. Die erste Testkamera stand genau auf den XY-Koordinaten der Einstiegsboje und damit innerhalb ihrer zweiseitig sichtbaren Leine. Die nahezu einfarbigen Aufnahmen aus den ersten `artifacts/water-diagnostic-*`-Versuchen und den anschließenden Abschaltversuchen sind deshalb **kein Nachweis für einen Shaderfehler**. Erst der Vergleich aus freiem, vermessenem Wasser ist dafür geeignet. Boje und Leine liegen nun zwei Meter neben dem unveränderten Tauchstart; der Untergrund wird dort geprüft. Das verhindert dieselbe Verdeckung beim direkten Spieleinstieg. Die Vergleichskamera hält zusätzlich Abstand zu den tatsächlichen Leinenpositionen und protokolliert Kamera, Seegrund und aktive Nachbearbeitung.

Mit freier Kamera zeigte sich: Bei unverändertem Sonnenstand, Sichtweite, Belichtung, Wellen und Kamera-Nachbearbeitung verschwindet die starke falsche Abdunklung nach oben, wenn Single Layer Water unterhalb der Oberfläche keine Wasserabsorption für den dahinterliegenden Luftraum mehr anwendet. `UpdateAtmosphere` setzt deshalb `OpticalScale = 14 / Sichtweite × (1 − Kamerawasseranteil)`. Der Kamerawasseranteil verwendet den bestehenden glatten Übergang über 0,5 Meter. Über Wasser bleibt die bisherige Volumenwirkung erhalten; die Unterwasser-Nachbearbeitung für den Weg zwischen Kamera und Oberfläche bleibt aktiv. Globale Helligkeit, Nebeldichte und Himmelshöhe wurden dafür nicht verändert. Die vorübergehenden Shader-Bypass- und Atmosphärenhöhen-Experimente wurden entfernt.

`Tools/SmokeTest.ps1 -Scenario rocks -WaterView -WaterStart 0 -WaterEnd 1` erzeugt sechs Vergleichsbilder: Zustand 0 erzwingt das frühere Oberflächenvolumen, Zustand 1 deaktiviert es bei vollständig eingetauchter Kamera. Je Zustand werden zwei aufwärts gerichtete Unterwasserblicke und ein Blick über Wasser aufgenommen. Die erweiterten Diagnosezustände 2–7 kombinieren Luftnebel und abgeschaltete Kameraabsorption; 8/9 sind Kontrollen ohne Nebel und Kameraabsorption, mit ausgeblendeter/eingeblendeter Wasserfläche. Diese Abschaltungen gelten ausschließlich im ausdrücklich gestarteten Vergleichstest. Ein bestandener Testmarker bestätigt Ablauf und frische Aufnahmen, nicht die Erfüllung des visuellen Qualitätsziels.

Die Korrektur ist ein begrenzter Fortschritt bei der Unteransicht, kein vollständiger physikalischer Unterwasser-Renderer. Insbesondere korrekte winkelabhängige Brechung, Totalreflexion bzw. Snell-Fenster und die räumliche Wirkung von Licht, Trübung und Bewuchs bleiben weiterzuentwickeln. Das geforderte Ergebnis umfasst einen nachvollziehbaren Blick durch die Oberfläche, dieselbe Wellengeometrie und einen konsistenten Übergang beim Auftauchen. Nacht, größere Tiefe und trübe Bedingungen bleiben Teil der Abnahme. Engine-Dateien wurden nur gelesen und nicht verändert.

Das Paket **`Builds/WindowsWaterOptics/divingsim.exe`**, gebaut aus Laufzeitstand `13abf1e`, besteht sieben Prüfungen mit 26 frischen Aufnahmen sowie einen zusätzlichen Start-/Spiellauf über die Root-EXE. Der [Prüfbericht](WATER_OPTICS_RESULTS.json) bindet EXE, Paketcontainer, Laufprotokolle, Quellstände und Bilder über SHA-256. Geprüft wurden der kontrollierte Tag-/Nachtvergleich, Auf-/Abtauchen an allen drei Einstiegen, Sturm und eine Nachtansicht mit Lampe bei vier Metern Sichtweite. Die bekannten CelestialVault-Startmeldungen sind im Bericht erhalten. Ein Dauerlauf oder isolierter Bildratenvergleich dieses Pakets wurde nicht durchgeführt.

Erzwungene frühere Volumenbehandlung im neuen Paket, feste freie Kamera:

![Vergleich: bisherige Absorption hinter der Oberfläche](Images/Underwater_Surface_Previous.png)

Dieselbe Kamera mit korrigierter Volumenbehandlung; Unterwassertrübung bleibt aktiv:

![Vergleich: Luftraum hinter der Oberfläche](Images/Underwater_Surface_Current.png)

Normaler Tauchlauf im Paket; die Umgebung bleibt deutlich unter dem angestrebten Detailniveau:

![Spielansicht mit korrigierter Wasseroptik](Images/Underwater_Surface_Game.png)
