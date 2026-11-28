# Kräftigere Körper für die grossen Felsgruppen

Die grossen Gruppenkerne verwenden zwei zusätzliche geschlossene Scanformen mit deutlicheren Blockflächen und kräftigeren Körpern. Die bisherigen drei Felsformen bleiben für lose Blöcke und Geröll erhalten. Damit unterscheiden sich zusammenhängende Aufschlüsse stärker vom umgebenden Kleingestein. Das [visuelle Gesamtziel](VISUAL_TARGET.md) ist weiterhin nicht erreicht; insbesondere Bodenanschlüsse, Bewuchs und Wasseroptik bleiben offen.

## Modelle und Platzierung

Quellen sind [Namaqualand Boulder 03](https://polyhaven.com/a/namaqualand_boulder_03) von Dario Barresi/Jenelle van Heerden und [Namaqualand Boulder 04](https://polyhaven.com/a/namaqualand_boulder_04) von Jenelle van Heerden, veröffentlicht unter [CC0](https://polyhaven.com/license). Das sind keine Walensee-Scans. Skalierung, Gruppenbildung und Unterwasserbelag sind gestaltet. [Quellmanifest mit Autoren, Downloadadressen und Prüfsummen](ROCK_FORMATION_ASSETS.json).

Die Prüfung der Originalgeometrie findet bei beiden Modellen keine offenen oder nichtmannigfaltigen Kanten. Ihre 64.826 beziehungsweise 59.066 Dreiecke bleiben für die Nahansicht erhalten. Vier Raster-Detailstufen und Nanite werden erzeugt; nach erneutem Laden liegen die Raster-Schwellen bei 1 / 0,20 / 0,07 / 0,02. Die unmittelbar nach dem Import abgefragten Renderdaten meldeten noch vier Nullen; deshalb gilt die separate Prüfung der gespeicherten, neu geladenen Assets. Zwei beziehungsweise ein konvexer Kollisionskörper approximieren die Modelle.

Passende 4K-Farb-, Normalen- und AO/Rauheits/Metallkarten nutzen die Original-UVs und den bestehenden nassen Oberflächenbelag. Die bisherigen drei Modelle, ihre Texturen und die vermessene Bathymetrie werden nicht ersetzt. Offene Felswand-Scans wurden ebenfalls geprüft, aber nicht als frei umschwimmbare Geometrie importiert.

Die neue Formauswahl betrifft ausschliesslich die vier Kernkandidaten einer bestehenden Aufschlussgruppe. Eine separate deterministische Auswahl lässt die bisherige Zufallsfolge für Position, Grösse, Ausrichtung und lose Steine unverändert. Zahl und konservative Hindernishüllen der Felsen bleiben gleich. Tatsächliche Oberflächenkontakte und Reflexionen verändern sich mit der neuen Geometrie. Pflanzen- und Fischprüfungen verwenden weiterhin die vorhandenen Hindernisabfragen.

## Native Vorprüfung

`artifacts/rock-formations-20261127-071115` besteht drei Fälle mit zehn Aufnahmen: bisherige und neue Gruppen an identischen Kameras sowie je zwei Ansichten der neuen Einzelmodelle. Beide Gruppenläufe laden 13.834 Felsen und 4.810 Pflanzen. Im neuen Lauf verwenden 592 Kerne die zusätzlichen Formen, verteilt auf 295 und 297 Instanzen. Alle 40 von oben und unten ausgeführten Kontaktproben treffen einen eingebetteten grossen Block. Zellwechsel, schnelles Umkehren, Teleport, Geröllkontakte und Fischbewegung bestehen. [Nativer Vorprüfbericht](ROCK_FORMATIONS_NATIVE_RESULTS.json).

Der Vergleich zeigt markantere, unterschiedlich gegliederte Felswände. Die grossen ebenen Sedimentflächen werden dadurch noch nicht natürlicher. Die Kontaktproben erfassen ausgewählte konvexe Körper; sie belegen keine dreiecksexakte Kollision sämtlicher Überhänge. Die ursprünglichen Geometriedaten sind geschlossen, aber ihre platzierte Gruppierung ist kein vermessenes lokales Felsmodell.

## Prüfung des Spielpakets

`WindowsFormations` besteht unter `artifacts/rock-formations-20261127-072254` zehn Fälle mit 40 Aufnahmen: vorher/nachher, beide Einzelmodelle, die drei direkten Unterwasserstarts, Rasterdarstellung ohne Nanite, Nachladen während Bewegung, Uferweg bei Weesen und aufgewirbeltes Sediment. Der automatische Start über die Haupt-EXE besteht ebenfalls. Die Formprüfung zählt 592 neue Kerne bei Weesen, 783 bei Murg und 832 bei Walenstadt. Die direkten Unterwasserstarts sind nicht mit den separat benannten Uferzugängen gleichzusetzen; nur Uferzugang 0 wurde hier geprüft.

Vier Messungen derselben EXE bei 1080p/Hoch mit Wasserprofil ergeben 69,53 FPS bei Weesen, 68,37 FPS in der Kontrolle mit bisherigen Kernformen, 72,91 FPS bei Walenstadt und 95,96 FPS auf der 90-sekündigen Nachladestrecke bei Murg. Kein gemessener Frame überschreitet 33,33 ms; einzelne Frames liegen über dem 60-FPS-Zeitbudget. Die Einzelläufe sind kein Langzeit- oder Mindesthardwarebeleg. [Vollständige Leistungswerte](PERFORMANCE.md), [Paketbericht mit Prüfsummen und Originalaufnahmen](ROCK_FORMATIONS_RESULTS.json).

Der bereits im vorherigen Paket protokollierte fehlende CelestialVault-Sternmaterialverweis bleibt bestehen und ist im Bericht enthalten. Die bestandenen Tages- und Unterwasserprüfungen sind deshalb keine Aussage über ein vollständig fehlerfreies Engine-Protokoll oder die Nachtbeleuchtung.

## Bildvergleich im Spielpaket

Beide Nahansichten verwenden dieselbe Kamera, Tageszeit, Wasserparameter und Objektanzahl. Die Kontrolle schaltet nur die Auswahl der zusätzlichen Kernformen ab. Der Vordergrund und die grosse Wand rechts zeigen die geänderten Körper; freie Sedimentflächen bleiben sichtbar eben. Die Übersicht zeigt denselben Bereich aus einer höheren Kameraposition. Es sind unveränderte Aufnahmen des gerenderten Spiels.

![Bisherige Kernformen](Images/RockFormations_Before.png)

![Neue Kernformen](Images/RockFormations_After.png)

![Neue Felsgruppen in der Übersicht](Images/RockFormations_Wide.png)

Die neue Laufzeit verwaltet fünf statt drei Felsvarianten. Einschliesslich der drei Pflanzenvarianten sind damit bis zu 72 statt 54 aktive Modellkomponenten in neun Zellen vorgesehen. Auch die Kontrolle derselben EXE behält diese Struktur; sie ist kein vollständiger Rückbau zum vorherigen Paket. Die beiden zusätzlichen Felsgruppen bleiben in der Kontrolle ohne Instanzen.

## Reproduktion

`Tools/prepare_formation_assets.py` prüft oder lädt die gepinnten Quellen. Blender erzeugt mit `Tools/create_rock_assets.py -- --formations` die zwei zusätzlichen FBX-Dateien und `blender/FormationRocks.blend`. `Tools/Build.ps1 -EditorSandbox -Target Content -OnlyFormations` importiert ausschliesslich die neuen Modelle und Oberflächen. Die normalen Aufrufe der bisherigen Felswerkzeuge behalten ihre drei ursprünglichen Modelle.

`Tools/TestRockFormations.ps1 -Preview` prüft die nativen Vergleichsansichten. Ohne `-Preview` kommen direkte Unterwasserstarts bei Weesen, Murg und Walenstadt, Rasterdarstellung, Nachladen, Sediment und der Uferweg bei Weesen hinzu. `-LegacyFormations` aktiviert in Smoke-Test und Benchmark die vorherige Kerngeometrie. Die zusätzliche `-FormationView`-Ansicht zeigt nur die zwei neuen Einzelmodelle. Menü/HUD/FPV bleiben gemäss Nutzerauftrag nach Unterwasser- und Überwassergrafik eingeordnet.
