# Kopfdrehung bei blockiertem Körper

Der Spieler kann seitlich weitersehen, wenn eine angeforderte Körperdrehung an Flasche, Flosse oder anderer Ausrüstung scheitert. Die normale Maussteuerung bleibt bestehen. Zunächst versucht die Simulation weiterhin die vollständige Bewegung. Scheitern auch die vorhandenen Ausweichversuche für Schlagphase, Hände und Schwimmhaltung, prüft sie eine Kopfdrehung mit unveränderter Körperausrichtung. Der vorgesehene seitliche Bereich beträgt ±65°. Das ist eine Animationsvorgabe, keine anatomische Validierung. Ein Blick ganz nach hinten benötigt weiterhin eine Körperdrehung und entsprechenden Platz.

`FDivePoseState.HeadYaw` hält den akzeptierten Unterschied zwischen Blick- und Körperrichtung fest. Die ausstehende Mausanforderung bleibt erhalten. Gibt ein Hindernis den Weg frei, kann die vollständige Drehung nachgeholt werden. Die bestehende Augenverankerung bleibt erhalten: Die Körperposition wird aus Augenpunkt und Halsgelenk neu berechnet. Dadurch können kleine Körperverschiebungen entstehen; auch diese durchlaufen die Kollisionsprüfung. Es handelt sich nicht um einen unabhängig durch Wände beweglichen Kameramodus.

Kopf und Körper werden über Quaternionen verbunden. Maske, Kopfhaube und Atemregler folgen der akzeptierten Kopforientierung. Der sichtbare Mund bleibt deckungsgleich mit dem für Atmung, Wassergrenze und Blasen verwendeten physischen Mundpunkt. Hände, Lampe, Flossen, Kompass am Handgelenk und Vortrieb folgen bei der seitlichen Kopfdrehung weiterhin der Körperrichtung. Auch Vorwärtsgehen am Ufer behält diese Ausrichtung; ein seitlicher Blick lenkt den Körper nicht automatisch seitwärts. Kontaktformen und sichtbare Ausrüstung verwenden denselben Zustand. Die Zwischenhaltungen einer Drehung werden ebenfalls geprüft; der Kopfbereich wird dabei nicht übersprungen. Die Übertragung auf Unreal-Szenenkomponenten erhält steile Winkel ohne die bisher umgangene Euler-Rundung nahe 90°.

## Gezielte Prüfungen

Die nativen Prüfungen verwenden echte Unreal-Kollisionsabfragen und importierte Ausrüstungsgeometrie. Ein Hindernis im Flaschenweg blockiert die Körperdrehung, erlaubt aber den begrenzten Kopfblick. Nach Freigabe des Hindernisses holt der tatsächliche Pawn die aufbewahrte Drehung nach. Ein zweites Hindernis an der gedrehten Maske verhindert die Kopfdrehung. Die Übereinstimmung zwischen gerendertem Rig und Kontaktformen wird für sechs Ausrüstungsvarianten, vier Haltungen und jeweils −65°, 0° und +65° Kopfversatz geprüft. Weitere Tests vergleichen Augen, Hals, Mund und Regler über steile Blickwinkel und den ±180°-Übergang hinweg.

Die Windows-Regressionsmatrix in `Tools/TestHeadTurn.ps1` umfasst drei Sedimentläufe, drei Uferläufe, zwei Avatar-/Ausrüstungsläufe, Atmung, Computer und einen längeren Schwimmweg mit Populationstest. `Tools/ReportHeadTurn.py` prüft Protokolle und frische Bilder und erfasst tatsächlich akzeptierte seitliche Kopfdrehungen. Diese Prüfungen ersetzen keine manuelle Spielgefühl-Abnahme.

Reproduktion mit vorhandenen Projekt-Assets:

```powershell
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/Build.ps1 -Target Package -EditorSandbox -BuildSlot HeadTurn -PackageDirectory Builds/WindowsHeadTurn
./Tools/TestHeadTurn.ps1 -PackageDirectory Builds/WindowsHeadTurn
```

Die native Suite lässt sich im frisch gebauten Sandbox-Editor mit `Automation RunTest DiveSim` starten. Für die JSON-Auswertung wird das vom Spieltest ausgegebene Ergebnisverzeichnis an `Tools/ReportHeadTurn.py` übergeben; die Python-Umgebung benötigt Pillow.

## Grenzen

Die Änderung ergänzt den Fall einer blockierten Körperdrehung. Ein jederzeit frei bedienbarer, vom Bewegungsziel unabhängiger Blickmodus sowie eine weichere zeitliche Abstimmung zwischen Kopf und Schultern bleiben mögliche Weiterentwicklungen. Die Kamera bleibt am physischen Augenanker. Die Körperkontakte sind konservative Kapseln und Begrenzungsboxen. Fotorealismus, anatomische Plausibilität, Mindesthardware, natives macOS und die vollständige Anforderungsabnahme sind damit nicht nachgewiesen.

## Geprüfter Windows-Build (26.11.2026)

`Builds/WindowsHeadTurn/divingsim.exe` ist der normale Starter. Der fertige Stand besteht 30 native Unreal-Prüfungen und elf gerenderte Spielabläufe mit 83 Bildschirmaufnahmen und einer exportierten Computertextur. Der Weesen-Sedimentlauf bestätigt einen akzeptierten 65°-Kopfblick und die spätere vollständige Körperdrehung. Die 112 Quell-/Konfigurationsdateien stimmen mit der Build-Sandbox überein; die 25 Avatar- und neun Fisch-/Pflanzen-/Fels-Assets sowie der vorherige Population-Build bleiben unverändert. Auch der normale Starter besteht den Gameplay-Test.

Ein separater 60-Sekunden-Lauf am Seegrund erreicht bei 1080p, Qualität 2 und 100 % Renderauflösung 90,9 FPS im Mittel und 74,4 FPS als 1%-Low. RTX 3090 / Ryzen 9950X3D; der Benutzer-Editor blieb geöffnet. Dieser einzelne Lauf belegt weder Mindesthardware noch den ungünstigsten Fall einer blockierten Kopfdrehung.

Nachweise: [Build und native Tests](HEAD_TURN_BUILD.json), [Spielabläufe und Bilder](HEAD_TURN_RESULTS.json), [Leistungsmessung](HEAD_TURN_PERFORMANCE.json), [Spielstarter](HEAD_TURN_LAUNCHER.json).
