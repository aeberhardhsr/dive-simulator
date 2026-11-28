# Wasseroberfläche von unten: getrennte Grenzfläche

Nachfolgender Stand: Das [räumlich begrenzte Wassermedium](BOUNDED_WATER.md) behebt die hier dokumentierte graue Durchsicht im kombinierten Testmodus. Die nachstehenden Befunde und Bilder beschreiben den ursprünglichen Grenzflächen-Prototyp.

Die anschliessende [Arbeit an den Spiegelungen](WATER_REFLECTIONS.md) reduziert die Materialrauheit, ergänzt eine Kontrolle mit Beleuchtung am Raytracing-Treffer und entfernt falsche Selbsttreffer auf der flachen Wasser-Hilfsgeometrie. Der kombinierte Pfad ist inzwischen über einen separaten Starter als [Spielvorschau](WATER_PREVIEW.md) verfügbar; die folgenden Befunde beschreiben die frühere native Untersuchung.

Der experimentelle Modus `-DiveWaterInterface` trennt die Ober- und Unterseite der Wasserfläche. Er ist ausschließlich zusammen mit `-DiveSmokeTest` aktiv. Das normale Spiel verwendet weiterhin `M_Water`; der damals geprüfte Spielstand war `Builds/WindowsRockAprons/divingsim.exe`. Neuere Spielpakete stehen im [README](../README.md). Die Referenzqualität ist noch nicht erreicht.

## Umsetzung

Die vorhandene Geometrie erhält eine zweite, deckungsgleiche Fläche mit umgekehrter Dreiecksreihenfolge und nach unten gerichteten Normalen. Beide Seiten sind einseitig. Wellenkoeffizienten, Phasen, Uhr, Wind und Position des Betrachters werden gemeinsam gesetzt. Der gesamte Wellenhub bleibt in den Sichtbarkeitsgrenzen enthalten. Kollision und Druckberechnung ändern sich nicht.

`M_WaterTop` übernimmt den bisherigen Single-Layer-Water-Aufbau für die Oberseite. `M_WaterUnder` ist ein beleuchtetes transparentes Material vor der Tiefenunschärfe. Hochwertige Lumen-Reflexionen der vordersten transparenten Fläche werden nur für Kameras im Testmodus eingeschaltet; die dafür nötige Projektunterstützung ist aktiviert. Die Installation von UE 5.8 setzt `CameraIsUnderWater` in den Single-Layer-Water-Aufrufern des Base-Pass und Composite-Pass fest auf `false`. Eine zweiseitige Oberfläche allein liefert daher dort keinen vollständigen Wechsel der optischen Grenzfläche. Engine-Dateien werden nicht geändert.

Die Unterseite berechnet den unpolarisierten Fresnel-Anteil aus dem tatsächlichen Blickwinkel und der gemeinsamen Wellennormale, mit Brechungsindizes 1,333 und 1. Bei senkrechtem Blick werden ungefähr 2,04 % reflektiert; oberhalb des Grenzwinkels von etwa 48,6° zur Flächennormale wird die Fläche vollständig reflektierend. Ein weisses metallisches Material liefert dabei die Reflexion ohne diffuse Oberflächenfarbe, die Transparenz gewichtet Reflexion und Durchsicht. Diese Kombination ist eine Rendering-Näherung und kein vollständig physikalisches Mikrofacettenmodell.

Ein erster Test mit vollständigem bildschirmbasiertem IOR-Versatz erzeugte rechteckige Ränder durch fehlende Bildinformation ausserhalb des Viewports. Der aktuelle Aufbau verzerrt deshalb nur die Wellenabweichung über Pixel-Normal-Offset. Das beseitigt die beobachteten Versatzränder, bildet jedoch nicht die vollständige Snell-Brechung der gesamten Überwasserwelt ab. Der Fresnel-Grenzwinkel wird davon unabhängig berechnet.

## Sichtbefund und offene Arbeit

Bei flachem Blick ersetzt die Unterseite die bisher sichtbare Überwasserlandschaft durch Spiegelungen der Unterwasserumgebung. Die Bojenleine und Steine sind darin erkennbar. Das ist ein Fortschritt bei der Grenzflächendarstellung, aber noch keine Freigabe für den normalen Spielbetrieb.

Die Durchsicht nach oben ist im neuen Modus zu grau. Der Kontrollfall mit Luftnebel zeigt wieder Himmel und Sonne: Der transparente Aufbau übernimmt einen Hintergrund, auf den zuvor bereits der globale Unterwassernebel wirkt. Dieser muss an der tatsächlich durch Wasser laufenden Strecke begrenzt werden. Der neue Darstellungsweg macht diese Kopplung sichtbar; sie darf nicht durch eine globale Aufhellung kaschiert werden. Auch die noch grobe Kontur am Übergang zur Totalreflexion und die unscharfen Reflexionen bleiben zu verbessern. Hardware-Lumen allein löst die Durchsicht nicht.

Gleiche Kamera, bisherige Darstellung:

![Bisherige Wasserunterseite mit sichtbarer Überwasserlandschaft](Images/WaterInterface_Standard.png)

Experimentelle Unterseite mit Software-Lumen:

![Experimentelle Unterseite mit Spiegelungen](Images/WaterInterface_Prototype.png)

Noch unzureichende Durchsicht bei steilem Blick:

![Graues Sichtfenster des experimentellen Aufbaus](Images/WaterInterface_Window.png)

Der nächste Schritt ist die Trennung des durch Wasser und durch Luft laufenden Lichtwegs für den Hintergrund dieser Grenzfläche. Erst danach sind eine Übernahme ins Spielpaket und eine repräsentative Leistungsmessung sinnvoll. Die Menü-/HUD-Anforderung bleibt gemäss [Priorität](UI_UX_TARGET.md) nach den beiden Grafikphasen eingeordnet.

## Reproduktion und Nachweis

Die abschliessende Serie `artifacts/water-interface-20261126-192338` besteht mit **neun Fällen und 25 frischen Aufnahmen**. Der Editor-Build und die Materialkompilierung im echten Renderlauf bestehen. Alle vorgesehenen Kameravergleiche stimmen in Position, Richtung und Belichtung überein; der Sturmfall erfasst alle drei Medienzustände. Ohne Lampe beträgt das 99,9-%-Perzentil der maskierten Nachtansicht 1 von 255, mit Lampe sind 273.657 Bildpunkte deutlich beleuchtet. Die mittlere Differenz beträgt 61,05. Das sind gezielte technische Nachweise, keine Freigabe der Bildgestaltung.

`Tools/Build.ps1 -EditorSandbox` kompiliert das Modul. `Tools/Build.ps1 -EditorSandbox -Target Content -OnlyWater` erzeugt die drei Wasser-Materialien. `Tools/TestWaterInterface.ps1` erfasst neun Fälle: bisherige Darstellung, Software-/Hardware-Lumen, Luftnebelkontrolle, zwei Bodenansichten als Vergleich, Sturm mit Medienwechsel sowie Nacht mit und ohne Lampe. `Tools/ReportWaterInterface.py <Laufordner>` bindet die Ergebnisse, Quellstände, Moduldatei, Materialien und veröffentlichten Bilder per SHA-256 in [den Bericht](WATER_INTERFACE_RESULTS.json).

Die Laufzeitprüfung vergleicht jede Position, Normale und Dreiecksreihenfolge beider Flächen sowie ihre gemeinsamen Wellenparameter. Die Kamera prüft ihren Reflexionsmodus. Die Testhülle verwirft fehlgeschlagene Materialkompilierungen und Ersatzmaterialien, auch wenn der übrige Spiellauf regulär endet. Für Bildvergleiche müssen Kameraposition, Richtung und Belichtung übereinstimmen. Die bisherigen Nachtgrenzen werden unverändert übernommen. Die Versuche belegen weder eine Paketprüfung noch Bildrate, Langzeitstabilität oder visuelle Abnahme.

Fehlversuche bleiben erhalten: Der erste Materialaufbau scheiterte am Python-Importpfad. Nach dessen Korrektur erkannte der erste echte Renderlauf einen unzulässigen Kameravektor im Vertexshader und zeigte das Ersatzmaterial. Dieser Lauf ist kein bestandener Rendernachweis. Die Custom-Nodes erhalten seither nur die jeweils benötigten Eingänge. Die nachfolgenden IOR- und Pixel-Normal-Offset-Bilder sind getrennt unter `artifacts/water-interface-ior` und `artifacts/water-interface-pno` gesichert. Die bekannten Editor-Startfehler des AutomationTest- und ToolsetRegistry-Plugins werden im Bericht aufgeführt und nicht als fehlerfreier Engine-Start bezeichnet.
