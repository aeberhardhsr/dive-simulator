# Wassertransport in Reflexionsstrahlen – nativer Versuch

Der getrennte Render-Versuch integriert Wasser entlang des reflektierten Strahls. Im festen Weesen-Blick verschwindet dadurch die zuvor [nachgewiesene falsche Himmelsfarbe](WATER_REFLECTION_DIAGNOSIS.md). Das Verfahren ist **noch nicht im spielbaren Paket aktiviert**. Bildschirmtreffer, indirekte Beleuchtung und mehrfache Grenzflächenwechsel benötigen weitere Arbeit, bevor dieser Pfad als vollständige Wasseroptik gelten kann.

## Vergleich

Kontrolle mit denselben Wellen, Seegrenzen und Raytracing-Einstellungen:

![Kontrolle ohne Integration des reflektierten Wasserwegs](Images/ReflectionTransport_Before.png)

Experiment mit aktivierter Integration:

![Wasserstreuung entlang des reflektierten Strahls](Images/ReflectionTransport_After.png)

Beide Aufnahmen sind native, unveränderte Bilder bei −20 cm relativ zur lokalen Welle. Sie belegen die Wirkung in diesem Blick, keine fertige Gestaltung der Unterwasserwelt.

## Umsetzung

`ReflectionWater.ush` wertet 16 Abschnitte mit quadratisch verteilten Grenzen aus. Jeder Abschnitt prüft die gemeinsame Wellenoberfläche und die rasterisierte Seegrenze. Die Strahltransmission folgt dem Beer-Lambert-Gesetz; die entlang des Strahls erzeugte Einfachstreuung wird von vorne nach hinten integriert. Verwendet werden der laufende Sichtweitenparameter und die bereits für `M_WaterVolume` gewählte Streualbedo `(0.07, 0.24, 0.16)`. Diese Parameter sind gestalterische Modellwerte, keine gemessenen optischen Walensee-Daten.

Die Lichtquelle wird aus Unreals Szenenlichtliste gelesen. Sonnenlicht erhält eine näherungsweise gebrochene Richtung durch eine mittlere Wasserfläche und Dämpfung der Wassersäule. Punkt- und Spotlichter verwenden ihre Position, Reichweite, Kegel, Farbe und Entfernung. Vier Abtastungen begrenzen den Wasseranteil ihres Lichtwegs. Opaque Schatten werden mit zusätzlichen Inline-Rayqueries geprüft. Es wird keine konstante grüne Ersatzfarbe anstelle des Himmels eingesetzt.

Der Versuch nutzt die vorhandene gemeinsame Scene-Uniform für Wellen, Medium und Seegrenze. Deren Aktivierung wurde von der ausschliesslichen Solarstudie getrennt. Bei der neuen Reflexionsstudie bleibt die bestehende Sonnen-Lichtfunktion auf den normalen Oberflächen aktiv. Die globalen Solarshader werden nicht gleichzeitig ersetzt.

`PrepareReflectionTransport.py` erzeugt nur eine lokale Shaderkopie im ignorierten `Intermediate` des Source-Editor-Sandboxes. Die installierte Engine wird nicht geändert. Der vorhandene separate Quellcode-Engine-Build stellt die Scene-Bindung bereit. `DiveReflectionTransportControl` lädt dieselben Mediumdaten und Seegrenzen, lässt aber den Reflexionsshader unverändert; damit entsteht eine passende Kontrolle.

## Auswahl der Wasserfläche

Der erste Versuch verwendete die gelesene GBuffer-Normale zur Auswahl der Unterseite. Das war in diesem Hardware-Reflexionsshader ungeeignet: Er besitzt keine Spezialisierung `FRONT_LAYER_TRANSLUCENCY` und liest an dieser Stelle die opaken Hintergrunddaten. Der blaue Bildbereich blieb nahezu unverändert, obwohl sich andere Reflexionsbereiche änderten. Die Serie `artifacts/reflection-transport-20261127-224026` ist deshalb als visuell verworfen dokumentiert.

Die überarbeitete Auswahl verlangt den Front-Layer-Reflexionspass, den exklusiven Wasser-Stencilwert 42 und eine zur Reflexionsursprungsfläche passende Custom-Depth. Zusätzlich werden Luftkameras und Ursprünge ausserhalb der aktuellen Wellenoberfläche ausgeschlossen. Die normale Spielkonfiguration aktiviert diese zusätzliche Markierung nicht.

## Prüfung und Aussageumfang

Die Serie `artifacts/reflection-transport-20261127-224536` erfasst Kontrolle, Transport ohne Bildschirmstrahlen und den üblichen gemischten Reflexionspfad bei −20 cm, −2 cm und +20 cm. Die Serie `artifacts/reflection-transport-20261127-225042` ergänzt die Kontrolle ohne Bildschirmstrahlen, Sichtweiten 4 m und 28 m, zwei Blicke aus 1,5 m Tiefe und Nacht mit ausgeschalteter beziehungsweise eingeschalteter Lampe. Der Lampentest verwendet eine ausdrückliche Wasser-Inspektionsoption und protokolliert den tatsächlichen Sichtbarkeitszustand, die Intensität und Position der Lampe.

Der [Ergebnisbericht](REFLECTION_TRANSPORT_RESULTS.json) bindet Module, Quell- und Shaderstände, Kameraprotokolle sowie die Rohbilder. Die Prüfung des ursprünglichen blauen Bildbereichs verlangt dessen Beseitigung bei erhaltener Helligkeit; eine schwarze Ersatzfläche besteht sie nicht. Der über Wasser aufgenommene Vergleich wird separat auf Bildabweichung geprüft. Die weiteren Reaktionsmessungen sind beschreibend: Ein optisch unendlich ausgedehntes beleuchtetes Medium muss bei höherer Extinktion nicht pauschal dunkler werden.

Alle elf Fälle mit 33 Aufnahmen liefen durch und wurden bildlich geprüft. Im festen Reflexionsbereich fällt der Anteil blau dominierter Pixel bei beiden Wasserlinienpositionen von 100 % auf 0 %. Die mittlere Anzeigehelligkeit bleibt bei 32,19 beziehungsweise 28,74 von 255. Über Wasser beträgt die mittlere absolute RGB-Abweichung 0,35 Stufen. In der Nachtaufnahme bei −20 cm steigt die mittlere Helligkeit des reflektierten Lampenbereichs gegenüber der Lampenkontrolle von 0,93 auf 8,45; ohne Lampe bleibt dieser Bereich schwarz. Nahe Bodenspiegelungen und das helle Sichtfenster durch die Oberfläche sind in den 1,5-m-Aufnahmen weiter erkennbar. Diese Aussagen beziehen sich auf die geprüften Kameras und ersetzen keine Abnahme der gesamten Unterwasserwelt.

Eine separate CPU-Rechnung vergleicht die 16-Schritt-Quadratur mit der analytischen Integration einer homogenen, von der Sonne beleuchteten Wassersäule. Für 48 Kombinationen aus Extinktion und Strahlrichtungen beträgt die grösste relative Abweichung rund 1,71 %. Das prüft die Integrationsnäherung, nicht die GPU-Ressourcenbindung, Schatten oder das vollständige gerenderte Bild.

## Noch offene Grenzen

- Der neue Pfad verarbeitet rohe Raygen-Ergebnisse. Bereits aus dem Bildschirm gelesene Strahldichte enthält den Kameraweg und wird derzeit ausgelassen, um ihn nicht doppelt zu integrieren. Diese Pfade stimmen optisch noch nicht vollständig überein.
- Direkte Einfachstreuung ist vorhanden; indirekte Himmelsbeleuchtung des Volumens und Mehrfachstreuung fehlen. Das kann Schattenbereiche zu dunkel machen.
- Zusätzliche native Verstärkungsfaktoren für Volumenstreuung werden nicht aus der Raytracing-Lichtliste übernommen; insbesondere verwendet das bisherige Sonnenvolumen einen Faktor 2. Eine gemeinsame photometrische Abstimmung steht noch aus.
- Sonnenbrechung nutzt eine mittlere Fläche und konstante 98 % Eintrittstransmission. Wellenkaustik, winkelabhängige Eintrittsverluste und korrekt geknickte Schattenstrahlen sind hier noch nicht kombiniert.
- Schattenabfragen behandeln Geometrie als opak und nutzen die nahe Raytracing-Szene mit maximal 100 m Sonnen-Schattenweg. Entfernte Bergabschattung und transparente Blattkanten werden dadurch nicht vollständig abgebildet.
- Der Strahl wird spätestens bei 200 m beziehungsweise zehn mittleren freien Weglängen beendet. Seegrenzen werden pro Abschnitt abgetastet; sehr dünne Übergänge können dabei übergangen werden.
- Die sichtbare Wassergeometrie bleibt für sekundäre Raytracing-Strahlen ausgeschlossen. Mehrfache Reflexionen/Brechungen an der Wasserfläche sind damit weiterhin nicht gelöst.
- Nahe der Wasserlinie ist im Versuch ohne Bildschirmstrahlen teilweise eine dünne helle Übergangslinie sichtbar. Es gibt noch keine Leistungsabnahme für dieses Verfahren und keinen neuen Paketbau.

## Reproduktion

```powershell
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -Target Editor -EditorSandbox -SandboxName SourceEditorSandbox -MaxParallelActions 4
./Tools/TestReflectionTransport.ps1
./Tools/TestReflectionTransport.ps1 -Suite response
Tools/.geodata-env/Scripts/python.exe Tools/ReportReflectionTransport.py --waterline artifacts/reflection-transport-20261127-224536 --response artifacts/reflection-transport-20261127-225042
```

Die Testläufe müssen nacheinander ausgeführt werden. Für neue Serien sind die neu ausgegebenen Verzeichnisse im Berichtsaufruf einzusetzen. Das spielbare Paket bleibt `Builds/WindowsWaterline/Start-WaterPreview.cmd`.
