# Seegrenze und Habitat in der tatsächlichen Landschaft

**Status: sieben native Läufe mit 15 gesichteten Bildern; Grafikziel weiterhin nicht erreicht.** Die Aufnahmen verwenden den Quellcode-Editor, die gemeinsame Wellenbeleuchtung und die geografische Begrenzung von Wassermedium und sichtbarer Oberfläche. Sie ergänzen den [freigelegten Oberflächentest](LAKE_SURFACE.md), ersetzen aber weder bewegte Uferprüfungen noch ein Spielpaket.

## Beobachtungen

Die drei vorhandenen Uferstarts in Weesen, Murg und Walenstadt rendern mit aktivierter Seegrenze und bestehen die bisherigen Ufer-/Kollisionsprüfungen. Die gezeigten Übergänge enthalten keine auffälligen grossen Löcher. Das ist eine Sichtprüfung dieser drei Kameras, keine Prüfung sämtlicher Uferpixel, Inseln, Wellenphasen oder Pegel.

Die [Weesener Uferaufnahme](Images/LakeLandscape_Weesen_Shore.png) zeigt den weiterhin offenen Abstand zur Landschaftsreferenz: grobe beziehungsweise gestreckte Fernhänge, weitgehend flächiger Wald und einfache Einstiegsmaterialien. Im [bestehenden Murg-Habitat](Images/LakeLandscape_Murg_Bed.png) sind die Scanoberflächen und grossen Felsgruppen lesbar. Die Beleuchtung bleibt gedämpft; Boden- und Bewuchsübergänge tragen die Szene noch nicht wie in der Nutzerreferenz. Mehr einzelne Steine allein beheben diesen Eindruck nicht.

## Vermessene Kamerakandidaten

Die jeweils ersten Kandidaten aus [BATHYMETRY_VIEW_CANDIDATES.json](BATHYMETRY_VIEW_CANDIDATES.json) wurden erstmals im nativen Spiel besucht. Die drei Ansichten je Position verwenden die vorgeschlagene Richtung sowie ±35°. Gelände, Felsen, Bewuchs und Wasser werden am tatsächlichen Punkt geladen; kein Prüfboden ersetzt die Umgebung. Die Wellengeometrie wird nach dem Kamerawechsel neu zentriert, obwohl die Inspektion den gewöhnlichen Welttick anhält.

| Gebiet | LV95 E / N | Gemessener Boden relativ zum Szenariopegel | Kamera über gemessenem Boden | Kollisionskugel der Kamera |
| --- | --- | ---: | ---: | --- |
| Weesen | 2726262,5 / 1221666,5 | −10,30 m | 2,00 m | Radius 20 cm frei |
| Murg | 2734801,5 / 1219845,5 | −18,08 m | 2,00 m | Radius 20 cm frei |
| Walenstadt | 2741080,5 / 1220624,5 | −15,18 m | 2,00 m | Radius 20 cm frei |

Alle neun Aufnahmen bestätigen diese Bodenabfrage und die freie Kamerakugel. Sie beweisen weder Freiraum für den gesamten Taucher noch eine durchgehend begehbare beziehungsweise schwimmbare Route.

Die [Weesener Ansicht](Images/LakeLandscape_Weesen_Candidate.png) bietet Felsgruppen, freie Sedimentflächen und kleine Bewuchszonen. [Murg](Images/LakeLandscape_Murg_Candidate.png) und [Walenstadt](Images/LakeLandscape_Walenstadt_Candidate.png) zeigen kräftigere Hangformen; die hangabwärts gerichteten Kameras verlieren den Boden jedoch schnell in der Wassersäule. **Die Offline-Kandidaten sind daher nicht automatisch geeignete Hauptansichten.** Für einen zusammenhängend gestalteten Tauchbereich sind als nächstes Ansichten quer zum Hang und ein tatsächlich durchschwommener Weg zu beurteilen. Die Grafik darf nicht allein durch eine günstig ausgewählte Standkamera als erreicht gelten.

## Nachweis und Reproduktion

Der [Ergebnisbericht](LAKE_LANDSCAPE_RESULTS.json) enthält die getrennten Quell-/DLL-Stände der ursprünglichen Landschaftsläufe und der ergänzten Kameraprüfung, Materialprüfsummen, Laufparameter, Protokolle und 15 Bildprüfsummen. Die erste Gruppe umfasst das bestehende Murg-Habitat und drei Uferstarts; die zweite Gruppe die drei Kandidaten. Alle Unterwasser-Kontaktbögen und die drei Uferoriginale wurden betrachtet. Bekannte Engine-Startdiagnosen bleiben im Bericht erhalten; neue Material-/Shaderfehler traten in den abgeschlossenen Läufen nicht auf.

Der erste Kompilierungsversuch verwendete einen falschen Headerpfad; der anschliessende Build korrigierte ihn auf `String/LexFromString.h`. Beim ersten Kandidatenstart beendete Unreals Standardparser das kommagetrennte Argument nach dem ersten Wert. Die Kameraprüfung wurde daraufhin beendet und lieferte keine gültige Bildserie. Nach Umstellung auf die Parseroption, die nur an Leerraum trennt, bestanden Projektbau und alle drei Kandidatenläufe. Der verworfene Lauf bleibt separat aufgeführt.

```powershell
./Tools/TestLakeLandscape.ps1 -Cases habitat1,shore0,shore1,shore2
./Tools/TestLakeLandscape.ps1 -Cases habitat0,habitat1,habitat2 -SurveyCandidates
```

Vorausgesetzt werden die gebaute `SourceEditorSandbox`, die Wassermaterialien und die vorbereitete Wellen-Shaderabbildung. `-EngineRoot` akzeptiert einen anderen Engine-Pfad. `-CandidateIndex 0..2` wählt einen Kandidaten pro Gebiet, `-Hour` einen anderen Tageszeitpunkt. Ein zusätzlicher Aufruf mit `-SurfaceOff` deaktiviert nur die geografische Oberflächenbegrenzung und dient einem gezielten Kontrollvergleich; diese Option wurde in der vorliegenden Landschaftsserie nicht verwendet.

`SmokeTest.ps1 -HabitatView -HabitatPose X,Y,Z,Yaw,Pitch` dient ausschliesslich der Inspektion. Der native Code akzeptiert nur endliche, begrenzte Koordinaten über vermessenem Unterwasserboden und protokolliert den Freiraum. Die Aufnahmen dürfen auch bei einer blockierten Kamerakugel entstehen, damit der ungeeignete Kandidat diagnostiziert werden kann; das Bild allein ist dann ausdrücklich kein gültiger Freiraumnachweis.
