# Bodenkontakt der Baum-Fernstufen

Stand: 28. November 2026. Die Bodenauflage und die sichtbare Stammgeometrie wurden getrennt geprüft. Der normale Uferlauf mit aktivem Gelände-Pilot und eine feste Ansicht in Murg lieferten native Baumpositionen, Detailstufen und unabhängige Schnittpunkte mit der tatsächlichen prozeduralen Uferkollision.

## Ursache und Korrektur

In der festen Ansicht stimmen 69 Uferproben bis auf maximal 0,000243 cm mit der Bodenabfrage überein. Die Instanzursprünge liegen wie vorgesehen 8 cm unter der abgetasteten Oberfläche. Eine pauschale Verschiebung der Bäume oder eine Änderung des Ufermodells wäre daraus nicht begründet.

Die bisherige freie Vereinfachung der Fernmeshes entfernte jedoch Teile der Stammfüsse. Die niedrigsten lokalen Punkte der letzten Detailstufe lagen bei 23,84 cm für den kleinen Laubbaum, 20,85 cm für Tanne A, 2,57 cm für Tanne B und 82,52 cm für Tanne C. Nach Skalierung einer Instanz kann das trotz korrekt verankertem Ursprung einen sichtbaren Abstand zum Boden erzeugen.

Der Blender-Export schützt nun die Kontaktpunkte innerhalb der unteren 2 cm. Diese erhalten Gewicht null; Kanten an solchen Punkten werden von Blenders Collapse-Algorithmus nicht zusammengelegt. Die anschliessende Prüfung verlangt, dass alle geschützten Positionen erhalten bleiben und die minimale Höhe höchstens 1 cm vom Ursprung abweicht. Grundlage: [Blenders Collapse-Implementierung](https://github.com/blender/blender/blob/main/source/blender/bmesh/tools/bmesh_decimate_collapse.cc) und [Übergabe der Vertexgewichte](https://github.com/blender/blender/blob/main/source/blender/modifiers/intern/MOD_decimate.cc). Der tatsächliche Export wurde mit der lokal installierten Version 5.3 ausgeführt.

Geschützt werden 25, 95, 137 beziehungsweise 41 Quellpunkte. Die letzte Detailstufe enthält danach 13.649, 10.498, 5.112 beziehungsweise 4.372 Dreiecke; vorher waren es 13.618, 10.357, 4.847 und 4.301. Die gewünschte Reduktion ist ein Zielwert; bereits die alten Exporte erreichten ihn wegen nicht weiter reduzierbarer Geometrie nicht immer.

Nur LOD2/LOD3 der vier vorhandenen Unreal-Baummeshes wurden neu importiert. Die Quell-FBX-Dateien von LOD0/LOD1 und die Buche wurden nicht neu erzeugt; die Materialbelegung blieb beim Import gleich. Die UAssets wurden dabei durch Unreal neu gespeichert und sind entsprechend nicht byteidentisch zum vorherigen Stand.

## Prüfung und Grenzen

Der Vorher-/Nachher-Vergleich nutzt denselben Standort, Blickrichtung 90°, Neigung −5°, 1920×1080 und Qualitätsstufe 2. Für diese Diagnose sind Nanite deaktiviert und die konventionelle Baumstufe 3 erzwungen. Beide Aufnahmen zeigen daher bewusst reduzierte Baumkronen und sind kein Vorschlag für die normale Spieldarstellung. Im Nachherbild erreicht der kleine Baum am linken Hang mit seinem vollständigen Stammfuss den Boden; der vorherige zugespitzte, abgehobene Fuss ist behoben.

Die nativen Messungen erfassen in beiden festen Ansichten 160 Bauminstanzen. Die importierten neuen Fernstufen aller vier betroffenen Modelle beginnen innerhalb von 0,01 cm um Z=0. Die Instanzplatzierungen bleiben gleich. Das belegt den korrigierten Meshkontakt, nicht die botanische Qualität jeder Baumkrone oder fehlerfreien Bodenkontakt sämtlicher Bäume im ganzen Seegebiet. Für die Buche und die höchsten Nanite-Detailstufen wird keine entsprechende Geometrieänderung behauptet.

Der zusätzliche 60-Sekunden-Uferlauf mit normalem Nanite-Pfad bestand: 77,02 FPS im Mittel, 58,57 FPS im langsamsten Prozent, 14,92 ms am 95. Perzentil. Ein Frame von 4.619 dauerte 34,51 ms. Der zuvor erfasste Editorlauf lag bei 76,84 FPS und hatte ebenfalls einen Frame über 33,33 ms (55,59 ms). Die zeitabhängigen Kamerabahnen endeten nicht exakt gleich; daraus wird weder eine Beschleunigung noch eine Behebung der bekannten kurzen Aussetzer abgeleitet. Der feste LOD3-Bildvergleich lag bei 55,21 bzw. 54,98 FPS über jeweils zehn Sekunden; das ist eine Diagnose unter erzwungenem Fallback und keine normale Leistungsfreigabe.

Diese Läufe sind Editor-Spieltests auf dem Entwicklungsrechner. Hinzu kommen 54 bestandene Python-Tests. Der bestehende Gelände-Pilot R1 und die Unterwasser-Abnahme R3 bleiben eingefroren; die neue Baumkorrektur ist noch nicht in diesen Paketen enthalten. Der nächste Paketbau muss einen neuen Ausgabeordner verwenden.

Die grossflächigen Fels-/Wiesenübergänge, Waldvolumen und Nahdetails bleiben der nächste Überwasser-Schwerpunkt. Die Unterwasserwelt erhält hierdurch keine weitere gestalterische Iteration.

## Reproduktion

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.3/blender.exe' --background --factory-startup --python-exit-code 1 --python Tools/create_vegetation_assets.py -- --far-only
& 'Builds/UE584/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'Builds/SourceEditorSandbox/divingsim/divingsim.uproject' -run=pythonscript '-script=O:/games/dive game/Tools/update_vegetation_roots.py' -unattended -nullrhi -nosplash
./Tools/Benchmark.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Scenario shore -Entry 1 -ProductionWater -LandscapePilot -TreeAnchorProbe -FixedYaw 90 -DisableNanite -TreeLOD 3 -Resolution 1080p -Quality 2 -Seconds 10
```

`-FixedYaw` verhindert die sonst zeitabhängige Kameradrehung und wird gegen die tatsächlich akzeptierte Kamera geprüft. `-TreeLOD` erfordert `-DisableNanite`; der Runner prüft den gesetzten Konsolenwert. `-TreeAnchorProbe` verlangt eine frische `TreeAnchors.json`. Der Runner prüft Aktivierung und Ausgabedateien; die Bildbeurteilung erfolgt separat.

[Eingefrorene Vorher-/Nachher-Daten und Ergebnisse](VEGETATION_ROOTS_RESULTS.json).
