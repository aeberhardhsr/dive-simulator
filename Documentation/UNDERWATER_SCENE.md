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


## Zusammenhängende Fels- und Pflanzenbereiche

Die nächste Überarbeitung ersetzt die weitgehend gleichmäßige Verteilung einzelner großer Felsen durch unregelmäßige Gruppen. Eine Gruppe enthält einen größeren Fels, mehrere mittlere Steine und kleinere Bruchstücke. Die vorhandenen drei Felsformen werden dafür unterschiedlich skaliert, gedreht, teilweise eingegraben und an der vermessenen Bodenneigung ausgerichtet. Die kleinen Kiesinstanzen bleiben eine eigene Größenstufe. Freie Sedimentflächen bleiben zwischen den Gruppen erhalten.

Die Pflanzen folgen kontinuierlichen räumlichen Feldern statt wiederkehrenden Sinusbändern. Innerhalb eines Bereichs überwiegt eine Art, mit eingestreutem Mischbewuchs. Bestehende Grenzen für Tiefe, Sichtweite und Hangneigung gelten weiter; Felsflächen werden beim Pflanzen ausgespart. Das ist eine gestaltete Spielumgebung, keine kartierte Verteilung realer Walensee-Lebensräume.

Die Verteilung hängt von Weltkoordinaten ab. Felsgruppen werden einschließlich ihres Randbereichs berechnet; jeder Stein gehört genau der Zelle seiner endgültigen Position. Damit können Gruppen Zellgrenzen überschreiten, ohne doppelte Steine oder bei einer Rückkehr andere Anordnungen zu erzeugen. Einstiege und Zugangswege behalten ihre festen Freiräume. Instanzen werden gebündelt angelegt, bevor ihre Komponenten registriert werden.

`Tools/SmokeTest.ps1 -EditorSandbox -Scenario rocks -HabitatView -Hour 12` erzeugt drei Ansichten ohne HUD. Die Kamerawahl hängt ausschließlich vom vermessenen Gelände ab, nicht von der jeweils getesteten Objektverteilung. Der native Vergleich hält Positionen und Blickwinkel zwischen dem vorherigen Stand und der überarbeiteten Verteilung gleich. Er zeigt eine dichtere Staffelung und mehr Bewuchs; Materialien, Beleuchtung und die Gesamtwirkung liegen weiterhin deutlich unter der Nutzerreferenz. Die erste Variante mit weiter auseinanderliegenden Gruppen wurde nach Sichtprüfung verdichtet.

Vorher, native Spielaufnahme:

![Vorherige Verteilung von Felsen und Pflanzen](Images/Habitat_Before.png)

Überarbeitete Verteilung aus derselben Kamera:

![Gruppierte Felsen, Geröll und Pflanzen](Images/Habitat_After.png)


Die erste Paketserie (`artifacts/habitat-validation-20261126-142507`) wurde nach drei erfolgreichen Weesen-Fällen beim Murg-Kameratest abgebrochen. Die ursprüngliche Suche fand dort keinen flachen Bereich im vorgegebenen Tiefenband. Die Vergleichsprüfung erlaubt nun in einem zweiten Suchdurchlauf einen steileren vermessenen Hang und richtet den Blick an dessen Bodenverlauf aus. Weesens Vergleichskamera bleibt unverändert. Der abgebrochene Lauf bleibt erhalten und wird nicht als vollständige Paketfreigabe gewertet.


Bei der Sichtprüfung bleiben neben den einheitlichen Felsfarben und den dünnen Pflanzenoberflächen feine Linien auf einzelnen Bodenflächen sichtbar, unter anderem im Lediwracks-Vergleich. Ihre Ursache ist noch nicht isoliert; sie werden nicht als Bestandteil einer natürlichen Sedimentstruktur gewertet. Die nächste Bodenüberarbeitung muss auch diese sichtbaren Übergänge prüfen.


Die drei Streaming-Prüfungen des überarbeiteten Pakets erfassen im jeweiligen anfänglichen 3×3-Zellfenster folgende Instanzen. Die Zahlen beschreiben geladene Bereiche und nicht die gleichzeitig im Bild sichtbaren Objekte:

| Einstieg | Felsen und Geröll | Wasserpflanzen |
| --- | ---: | ---: |
| Weesen | 3.722 | 3.430 |
| Murg | 5.886 | 96 |
| Mols/Lediwracks | 5.750 | 723 |

Die unterschiedliche Pflanzendichte folgt den bestehenden Tiefen-, Sichtweiten- und Hanggrenzen. Alle drei Läufe prüfen eindeutige Zellzugehörigkeit, freien Einstiegsraum, sechs unverändert behaltene und neun identisch wiederhergestellte Zellen sowie die Ausweichbewegung aller fünf Fischarten.

Die vollständige abschließende Serie `artifacts/habitat-validation-20261126-143554` besteht mit **zwölf Fällen und 60 frischen Aufnahmen** sowie einem zusätzlichen Start-/Spiellauf über die Root-EXE. Das Paket `Builds/WindowsHabitat/divingsim.exe` wurde aus Laufzeitstand `c14eaa5` gebaut. Der [Prüfbericht](HABITAT_RESULTS.json) bindet EXE, Paketcontainer, Quellstände, Protokolle und Bilder über SHA-256. Die bekannte CelestialVault-Startmeldung bleibt im Bericht erhalten. Ein neuer Langzeitlauf ist damit nicht belegt. Die automatischen Simulationsprüfungen für diesen Quellstand bestehen auf GitHub ebenfalls ([Lauf](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45830285311)).

Die ergänzenden 1080p-Leistungsmessungen erreichen 68,18 FPS im Unterwasserlauf und 100,53 FPS auf der Murg-Strecke. Der Unterwasserlauf unterschreitet zeitweise 60 FPS, der Streckenlauf enthält zwei Bilder über 33,33 ms (Maximum 68,83 ms). Die Ansichten und Tiefen unterscheiden sich; die Zahlen sind kein isolierter Vergleich der Detailkosten. Schattenkosten und Nachladespitzen bleiben zu optimieren. Methodik und vollständige Grenzen: [Leistung](PERFORMANCE.md).

Die nachfolgende Vorschau `Builds/WindowsPopulationStaged/divingsim.exe` übernimmt diese Gestaltung unverändert und optimiert deren Nachladen. Begrenzte Wiederverwendung der Felslayouts und der Aufbau entfernter Zellen über mehrere Bilder senken im gleichen Murg-Messablauf das höchste Bild von 68,83 auf 37,12 ms; ein Bild bleibt über 33,33 ms. Drei Paketfälle mit neun festen Ansichten, Geometrie-/Fischkontinuität, Teleportprüfung und normaler Spielstart bestehen. Diese gezielte Prüfung wiederholt nicht die gesamte ältere Zwölf-Fälle-Serie. [Prüfbericht](POPULATION_STAGED_RESULTS.json), [Details](POPULATION_STREAMING.md). Boden- und Pflanzenmaterialien, Licht und die sichtbaren feinen Bodenlinien bleiben die nächsten Grafikthemen.

## Bewuchs auf Steinoberflächen und feine Geländelücken

Der gemeinsame Bodenshader ergänzt olivbraune Beläge und feine Ablagerungen auf Felsen, Kies und Sediment. Das räumliche Muster verbindet die Oberflächen über Objektgrenzen hinweg. Nach oben gerichtete Flächen tragen mehr Belag; zwischen 14 und 38 Metern Tiefe wird er ausgeblendet. Feine Farbunterschiede behalten die Struktur der vorhandenen PBR-Karten bei. Die Nassabdunklung ist etwas schwächer. Es entstehen weder zusätzliche Texturdateien noch neue Fels- oder Pflanzeninstanzen. Die Verteilung ist eine gestalterische Annäherung an Aufwuchs, keine Kartierung realer Algenarten oder Simulation ihres Wachstums.

Die Untersuchung der geraden dunklen Bodenlinien erfolgt aus derselben Lediwracks-Kamera. Ohne dynamische Schatten bleiben sie sichtbar. Eine unbeleuchtete Ansicht ohne Nachbearbeitung zeigt kleine schwarze Lücken; bei ausgeschaltetem Nanite verschwinden sie. Eine gezielte Erhöhung der Nanite-Positionspräzision an den vier benachbarten Uferkacheln beseitigt sie auch mit aktivem Nanite. Im festgelegten Bildausschnitt `(180,170)–(560,250)` sinkt die Zahl nahezu schwarzer Pixel von neun auf null. Diese Prüfung betrifft diesen Ausschnitt, nicht sämtliche möglichen Geländenähte.

Der reguläre Aufbau verwendet deshalb für Ufer- und Landschaftskacheln dieselbe Nanite-Positionsstufe von `2^-6 cm` (0,015625 cm) statt 1 cm. So ist die Rundung an Grenzen zur prozeduralen Bathymetrie erheblich kleiner. Quellhöhen, Dreiecke und Kollisionsflächen werden nicht verschoben; die Bake-Prüfungen vergleichen die Ausgangsgeometrie weiterhin mit den Geländedaten. Die neue Aufbauversion und die Laufzeitprüfungen verhindern, dass ältere Assets mit Zentimeterpräzision als aktueller Stand geladen werden.

Unreal stellt die Nanite-Aufbaueinstellungen nur im Editor bereit. Deshalb prüfen die Bake-Commandlets jedes Asset einschließlich seiner tatsächlichen Positionsstufe und schreiben diese in die Gelände-Manifeste. Das verpackte Spiel prüft den Manifestwert. Der erste Paketbau mit einem direkten Laufzeitzugriff auf die Editor-Einstellung scheiterte beim Kompilieren; dieser Lauf bleibt als fehlgeschlagener Versuch erhalten und zählt nicht zur Paketprüfung.

Reproduktion: `Tools/Build.ps1 -EditorSandbox -Target Content -OnlyBenthic` baut die vier gemeinsam verwendeten Materialgraphen; `-Target Coast` und `-Target Landscape` erzeugen die Geländedarstellungen. `Tools/SmokeTest.ps1 -EditorSandbox -Scenario rocks -HabitatView -Entry 2 -Hour 12 -DisableLighting` erzeugt die unbeleuchtete Diagnose. `-DisableShadows` und `-DisableNanite` erlauben getrennte Kontrollaufnahmen. Diese Optionen verändern keine gespeicherten Spiel-Grafikeinstellungen.

Die spielbare Vorschau **`Builds/WindowsBenthic/divingsim.exe`** enthält den Laufzeitstand `4f6e4e457996232a12a4a0bd1500c64602098498`. Die Spiel-EXE hat SHA-256 `1502b3ea65451f0b1435c43e8d5c3f50c78c97e22f0780923d417e326490475c`. Neun Paketfälle mit 20 frischen Bildern und der zusätzliche normale Launcher-Start bestehen: drei Tagesansichten, die Nanite-Ersatzdarstellung, eine feste Nachtansicht ohne Lampe, drei Ufer-Bodenansichten und ein zusätzlicher Nachtfall mit Lampe und geprüftem beleuchtetem Ziel. Die Nachtansicht ohne Lampe ist nahezu schwarz und belegt keine Materiallesbarkeit; der zusätzliche Lampenfall liefert dafür die sichtbare Kontrolle. Die scharf abgestuften Lampenschatten bleiben ein offenes Qualitätsthema.

Der [Prüfbericht](BENTHIC_RESULTS.json) bindet Quellstände, 573 Gelände-Assets, die vier Materialgraphen, Paketcontainer, Diagnosebilder und die Leistungsmessung. Auch der vollständig neu aufgebaute Stand enthält im festgelegten unbeleuchteten Diagnoseausschnitt null schwarze Lückenpixel. Ein neuer Langzeit- oder Streaming-Leistungsvergleich ist damit nicht belegt. Die 60-Sekunden-Unterwassermessung erreicht 68,34 FPS im Mittel und 55,21 FPS im langsamsten Prozent; durchgehend 60 FPS werden weiterhin nicht eingehalten. Details: [Leistung](PERFORMANCE.md).

Gleiche Kamera vor der Überarbeitung:

![Boden und Felsen vor Belägen und Präzisionskorrektur](Images/Benthic_Before.png)

Aktuelles Paket mit Belägen und korrigierter Geländepräzision:

![Aktueller Unterwasserstand mit fleckigen Felsbelägen](Images/Benthic_After.png)

Die Referenzqualität ist weiterhin offen. Besonders die dünnen Pflanzen, die gleichförmige Grundstruktur in größeren Bereichen sowie die Licht- und Schattenwirkung brauchen weitere Überarbeitung. Die Bilder belegen diesen Zwischenstand und keine vollständige visuelle Abnahme.
