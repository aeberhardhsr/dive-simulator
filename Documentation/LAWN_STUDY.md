# Wiesen-Nahansicht in Weesen

Stand: 28. November 2026. Diese Überwasser-Erweiterung ersetzt die farbabhängige Graserkennung innerhalb einer begrenzten, gestalteten Wiesenfläche. Sie ergänzt echte kurze Grashalme. Die Unterwassergestaltung und die bisher geprüften Spielpakete werden nicht verändert.

## Herkunft und Umfang

Die rund 977 m² grosse Fläche wurde im gebundenen SWISSIMAGE-Luftbild am Weesener Einstieg interpretiert. Die Polygonpunkte sind eine gestalterische Zuordnung, keine Vermessung von Rasen, Grundstücken oder Einzelobjekten. Strasse, Parkplatz und Steinbank werden bei der Interpretation ausgespart. Fotografierte Gegenstände innerhalb der Wiese verschwinden aus dem Bodenmaterial; ihre ursprünglichen dreidimensionalen Gegenstücke werden dadurch noch nicht rekonstruiert.

`prepare_lawn.py` erzeugt eine nordorientierte 1024²-Maske mit einem 75 cm breiten Übergang nach innen. Der unabhängige Prüfer verwendet Strahlkreuzungen und eigene Punkt-/Segmentabstände statt der Shapely-Berechnung des Erzeugers. 40.000 Pixelprüfungen, darunter 8.996 mit einem nichtnull Maskenwert, stimmen überein. Fünf benannte Kontrollpunkte prüfen Wiese, Strasse, Parkplatz, Steinbank und Wasser. Diese Stichproben belegen die geprüfte Registrierung, keine amtliche Bodenbedeckung.

`M_ShoreLawn` ist eine separate Kopie des bisherigen Weesener Bankmaterials. Innerhalb der Maske erhält Gras Vorrang vor der Luftbild-Farbheuristik und dem Waldmaskenwert. Die Steinbank bleibt in der anschliessenden Materialstufe vorrangig. Wasserhöhen- und Entfernungsüberblendung bleiben erhalten. Andere Einstiege verwenden ihre bisherigen Materialien.

## Grashalme und Bodenanschluss

Die zweite Variante verwendet 192 gekrümmte Halme pro Nahmesh und 64 beziehungsweise 20 Halme in den beiden Fernstufen. Die drei Stufen besitzen 960, 192 und 20 Dreiecke. Die Ursprungshöhe liegt zwischen 4 und 10 cm; die Instanzskalierung variiert zusätzlich zwischen 0,85 und 1,15. Die Beleuchtungsnormalen sind gestalterisch nach oben gewichtet, damit dünne Halme nicht als schwarze Linien erscheinen. Das ist eine bewusste Näherung der Grasbüschel-Beleuchtung.

Von 3.059 deterministisch verteilten Kandidaten werden aktuell 2.981 platziert. Treppenfläche, zu steile oder zu tiefe Standorte und ungeeignete Bodenanschlüsse werden ausgespart. Neun Abfragen pro Büschel prüfen den Anschluss an das vorhandene Bodenraster; die 26.829 Prüfpunkte liegen zwischen −3,983 und +0,500 cm zum Raster. Dies ist keine Einzelprüfung jedes Halmwurzelpunkts und kein GPU-Abgleich der gerenderten Bodenfläche. Die Halme besitzen keine Kollision. Die Geometrie ist statisch; Windbewegung und Verformung beim Betreten sind noch offen.

Die erste Variante blieb zu dunkel und vereinzelt; ihre überbreite Zugangsaussparung erzeugte eine auffällige leere Fläche. Ihr Quellstand, ihre Assets und ihre genaue Editor-DLL bleiben separat archiviert. Die zweite Variante reduziert die Aussparung auf die Treppengeometrie mit zusätzlichem Büschelabstand und verbessert Dichte, Länge und Beleuchtung.

## Prüfstand

Die unabhängige Datenprüfung, 54 portable Python-Tests, der native Editor-Build und der Inhaltsimport bestehen. Zwei fehlgeschlagene Importversuche bleiben in den Protokollen erhalten: ein falsch benannter Vertex-Farbausgang und ein im Commandlet nicht initialisiertes Editor-Subsystem. Beide Ursachen wurden vor den gültigen Rendervergleichen korrigiert.

Das separate Paket `Builds/WindowsLawnLandscapeR1/Start-DiveSimulator.cmd` ist gebaut und geprüft. Der Haupt-Launcher besteht den Murg-Tauchtest. Die Uferstrecken in Weesen, Murg und Mols sowie Mols mit 30 FPS bestehen mit je neun frischen Aufnahmen und aufrechter Rückkehr an Land. Der zusätzliche Weesener Wiesenlauf prüft nach einmaliger Startplatzierung normales Gehen, eine 180°-Wende und Rückkehr über insgesamt gut 13 m. Acht Pflichtbilder entstehen; der laufend geprüfte Höhenfehler bleibt innerhalb 0,5 cm. Das belegt diese Strecke, keine vollständige Kollisionsabnahme aller Ufer.

Die festen Editorvergleiche verwenden dieselbe archivierte DLL: Nordblick 84,19 FPS ohne und 76,21 FPS mit Wiese, Seitenblick 73,72 und 65,41 FPS. Es sind jeweils kurze 10-Sekunden-Messungen. Das nachfolgende Spielpaket enthält zusätzlich nur den automatischen Wiesen-Gehversuch; die Gestaltung bleibt gleich.

Die folgenden Paketläufe verwenden dieselbe EXE und dieselben Container, RTX 3090 / Ryzen 9 9950X3D, 1920 × 1080, Qualität 2 und 100 % Renderauflösung. Gelände-Pilot, natürliche Bodenmaterialien und Ufersteine bleiben aktiv; nur die Wiesen-Erweiterung wechselt im Überwasserpaar. Nach 15 Sekunden Einlaufzeit werden rund 60 Sekunden mit umlaufender Kamera gemessen.

| Lauf | Durchschnitt FPS | 1%-Low FPS | Längster Frame | Frames über 33,33 ms |
| --- | ---: | ---: | ---: | ---: |
| Weesen ohne Wiese | 70,78 | 58,87 | 17,79 ms | 0 / 4245 |
| Weesen mit Wiese | 66,69 | 50,27 | 22,38 ms | 0 / 3999 |
| Weesen unter Wasser mit Wiese | 62,62 | 55,25 | 19,80 ms | 0 / 3755 |

Die zusätzliche Geometrie hat messbare Kosten: im umlaufenden Paketpaar sinkt der Durchschnitt von 70,78 auf 66,69 FPS (rund 5,8 %), der 1%-Low-Wert von 58,87 auf 50,27 FPS. Kein Frame dieses Paars überschreitet 33,33 ms; durchgehend 60 FPS werden dennoch nicht erreicht. Die festen Editor-Nahansichten zeigen ebenfalls einen Rückgang. Ein einzelnes umlaufendes Paketpaar ist keine statistische oder allgemeine Leistungsgarantie. Der Unterwasserlauf ist eine einseitige Regressionsprüfung; die abgenommene Unterwassergestaltung wird dadurch nicht neu bearbeitet.

Die PowerShell-Syntaxprüfung und 15 gebundene Inhaltsdateien bestehen. Die vorherigen Paket- und Prüfnachweise wurden erneut auf unveränderte Hashes geprüft. Der bereits früher offengelegte überschriebene historische `EditorBuild.log` bleibt ausdrücklich ausgenommen; dessen alter Bericht wird nicht nachträglich umgeschrieben. In den Editorläufen treten die bekannten Editor-Testregistrierungsfehler auch in den Kontrollläufen auf; die nativen Prüfmarker bestehen.

Der [gebundene Prüfbericht](LAWN_RESULTS.json) enthält Quellstand, genaue Module, Paketdateien, Messungen, ausgewählte Bildsichtungen und die archivierte verworfene Variante. Die Referenzqualität der Überwasserwelt ist noch nicht erreicht.

## Fester Vorher-/Nachher-Blick

Gleiche Editor-DLL, Kamera, Belichtung und Grafikeinstellung; nur die Wiesen-Erweiterung wechselt.

![Weesen vorher](Images/Lawn_NorthBefore.png)

![Weesen mit lokaler Wiese](Images/Lawn_NorthAfter.png)


## Offene Grenzen

- Der Versuch betrifft zunächst Weesen; Murg, Mols und die übrigen Wiesen bleiben offen.
- Fotoabdrücke ausserhalb des Polygons, Baumkontakte, Waldvolumen und Bergwände bleiben sichtbar.
- Die gesichteten Bewegungsbilder sind keine lückenlose Prüfung von Flimmern oder Gras-Fernstufenwechseln; natürliche Unregelmässigkeit und Farbwirkung bleiben zu verbessern.
- Die Halme sind gestaltet, keine kartierte Artenzusammensetzung oder vollständige Vegetationssimulation.
- Die bekannten Wasserbänder und seitlichen Geländerkontakte werden hier nicht allgemein behoben.
