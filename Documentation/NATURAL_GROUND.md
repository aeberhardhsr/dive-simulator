# Natürlichere Bodenmaterialien – Überwasser-Kandidat R2

Stand: 28. November 2026. Dieser Schritt ergänzt den bereits geprüften Gelände-Pilot und die korrigierten Baum-Fernstufen um zwei eigenständige Bodenmaterialien. Er ist eine begrenzte Verbesserung der Nahansicht; die Überwasserwelt hat das gewünschte Referenzniveau noch nicht erreicht.

## Sichtbare Änderung

Fels-/Wiesenübergänge folgen weiterhin der Hangneigung, werden aber durch ortsfeste, unterschiedlich grosse Variationen aufgelockert. Die Detailtexturen bleiben bis in grössere Entfernung wirksam: Ausblendung zwischen 65 und 260 Metern statt bisher 25 und 120 Metern. Gras, Waldboden und Stein erhalten zurückhaltende Farb- und Normalenvariation. Neutrale Flächen behalten ihre fotografierte Farbe und Anordnung, erhalten aber feine Helligkeits- und Oberflächenstruktur aus der vorhandenen Felsdetailtextur.

Die Waldzuordnung stammt weiterhin aus der vorhandenen kartierten Waldmaske. Die Wiesenzuordnung bleibt eine gestalterische Heuristik aus den Grünanteilen des Luftbilds; es handelt sich nicht um eine neue vermessene Wiesenklasse. Es werden keine neuen Geländeformen, Wege oder Nutzungsgrenzen behauptet. Meshes, Höhenabfragen und Kollision bleiben durch diese Materialänderung unverändert.

Die feste Vorher-/Nachher-Ansicht in Murg zeigt weniger harte grüne Konturen am Fels. Eine erste Variante wurde wegen zu trockener Grasfarbe überarbeitet. Die endgültige Variante erhält mehr Grün und feine Struktur auf neutralen Flächen. In Weesen ist die Änderung dezent; grosse unscharfe Luftbildbereiche bleiben deutlich sichtbar. In Mols fallen die grünen Flecken am Felshang weniger auf. Alle sieben gültigen Editor-Aufnahmen wurden visuell geprüft, einschliesslich der verworfenen ersten Farbvariante.

## Technische Begrenzung

`M_LandscapeNatural` und `M_ShoreNatural` sind Kopien der akzeptierten Materialien. `Tools/build_natural_ground.py` ersetzt ausschliesslich den vorhandenen Abschnitt `GroundDetail.ush` durch `NaturalGround.ush`. Der übrige Custom-Shader bleibt identisch. Die Übergangsmaske von 0 bis 120 cm über dem Wasserspiegel bleibt erhalten; unterhalb des Wasserspiegels beträgt ihr Gewicht null. Die bisherigen Materialien werden nicht überschrieben.

Die neuen Materialien werden nur mit `-DiveNaturalGround` ausgewählt. Der separate Starter kombiniert diesen Schalter mit `-DiveLandscapePilot`. Die beiden eingefrorenen Pakete für Unterwasser-Abnahme R3 und Gelände-Pilot R1 werden nicht aktualisiert. Die Unterwasser-Abnahme bleibt bestehen; der Pakettest ist eine Regression und keine sechste Gestaltungsiteration.

Vor dem Paketbau prüft `-NaturalGroundVerify` die tatsächlich gespeicherten Shader gegen den aktuellen Quelltext und die gespeicherten Hashwerte, ohne die Assets zu ändern. Im gerenderten Test müssen das Landschaftsmaterial und alle vier Ufer-/Küstenmaterialinstanzen aktiviert sein. Ein erfolgreicher `nullrhi`-Materiallauf allein gilt nicht als Grafiktest: Der erste gerenderte Versuch deckte einen fehlenden `PatchNoise`-Helper in der älteren Materialbibliothek auf. Der Shader verwendet jetzt deren vorhandenen `Noise`-Helper; erst die nachfolgenden echten SM6-Läufe gelten als gültig.

## Editor-Vergleich

RTX 3090, Ryzen 9 9950X3D, 1920×1080, Qualitätsstufe 2, 100 % Renderauflösung, normale Nanite-Darstellung und Produktionswasser. Je 15 Sekunden Aufwärmphase und 10 Sekunden Messung, fester Standort und Blickwinkel. Beide Seiten jedes Paars nutzen dasselbe Editor-Modul; Wind, Vegetation und Simulationszeit sind nicht pixelgenau synchronisiert.

| Ort | Blickrichtung / Neigung | Bisher FPS | Kandidat FPS | Kandidat p95 | Kandidat Maximum |
| --- | --- | ---: | ---: | ---: | ---: |
| Murg | 90° / −5° | 80,00 | 79,46 | 13,63 ms | 18,78 ms |
| Weesen | 0° / −20° | 82,60 | 82,75 | 13,08 ms | 13,97 ms |
| Mols | 180° / −12° | 84,41 | 82,66 | 13,05 ms | 14,68 ms |

Keiner dieser sechs kurzen Läufe enthält einen Frame über 33,33 ms. Daraus folgt keine Garantie für längere Spielstrecken oder eine Behebung der bekannten CPU-/GC-Aussetzer.

## Spielpaket R2

`Builds/WindowsNaturalLandscapeR2/Start-DiveSimulator.cmd` startet den Kandidaten ohne Editor. Der normale Bootstrap-Launcher mit Murg-Tauchgang besteht. Die automatisierten Uferabläufe in Weesen, Murg und Mols bestehen bis zum gespeicherten Abschluss und Menüwechsel. Ein zusätzlicher Mols-Lauf mit 30 FPS besteht ebenfalls. Alle vier Rückkehrprüfungen melden vollständige Stehhaltung, Schwimmanteil 0 und keine Höhendifferenz zum Startpodest. Die Routen verwenden die unten beschriebene mittige Teststeuerung.

Der 60-Sekunden-Vergleich nutzt dieselbe Paket-EXE und dieselben Inhaltscontainer, jeweils mit aktiviertem Gelände-Pilot; allein `-DiveNaturalGround` unterscheidet die beiden Läufe. Bei 1080p/Hoch erreicht die bisherige Materialwahl 77,28 FPS, die neue 76,84 FPS. Das langsamste Prozent liegt bei 63,00 bzw. 63,26 FPS, p95 bei 14,88 bzw. 14,95 ms. Die längsten Frames dauern 16,83 bzw. 17,50 ms; kein Frame überschreitet 33,33 ms. Dies ist ein einzelnes Messpaar mit Kameradrehung, kein statistischer Leistungsnachweis oder Dauertest. Beide Endbilder wurden geprüft.

Die zusätzliche 60-Sekunden-Unterwasser-Kontrolle erreicht 77,54 FPS, 63,72 FPS im langsamsten Prozent, p95 14,85 ms und maximal 16,45 ms; kein Frame überschreitet 33,33 ms. Dies ist eine einzelne Regression mit dem Kandidaten, kein Unterwasser-A/B-Vergleich. Das Endbild zeigt geladene Fels-/Kiesoberflächen und Wassertrübung. Horizontale Wasserlinien-/Volumenbänder bleiben in den Auftauchbildern sichtbar; die technische Wasserlinienabnahme wird hierdurch nicht nachträglich erklärt.

Insgesamt wurden 37 ausgewählte vollständige Standbilder aus Editorvergleich, beiden Paketkandidaten und Diagnose gesichtet. Die Bildliste und alle zugeordneten Messdateien stehen im [eingefrorenen Ergebnisbericht](NATURAL_GROUND_RESULTS.json). Hinzu kommen 54 bestandene Python-Tests, native Editor-/Paketkompilierung und die Syntaxprüfung der geänderten PowerShell-Werkzeuge. Der R2-Paketbau endete erfolgreich; die im ersten Bau automatisch wiederholte Zen-Verbindung wurde vom Buildsystem erfolgreich wiederhergestellt.

## Offene Überwasser-Arbeiten

- Die grossen verwaschenen Fotoabdrücke in Weesen und Murg benötigen eine gezielte Behandlung der Nahflächen und ihrer tatsächlichen Bodenbedeckung. Mehr Materialrauschen ersetzt keine passenden Strukturen.
- Waldvolumen, Vegetationsvielfalt und die Fernansicht der Bergwände bleiben eigenständige Aufgaben.
- In den normalen Rückkehransichten von Weesen und Murg wirken einzelne Baumkontakte weiterhin abgehoben. Die frühere Korrektur der vier konventionellen Fernstufen ist daher keine Freigabe sämtlicher Nanite-Bäume oder Platzierungen.
- Die fotografischen Schatten und gestreckten Farben an steilen Wänden verschwinden durch diesen Schritt nicht vollständig.
- Die zwei geprüften Gelände-Pilotkacheln belegen keine lückenlose Prüfung aller Geländegrenzen im Seegebiet.

## Reproduktion

```powershell
& 'Builds/UE584/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' 'Builds/SourceEditorSandbox/divingsim/divingsim.uproject' -run=pythonscript '-script=O:/games/dive game/Tools/build_natural_ground.py' -unattended -nullrhi -nosplash
./Tools/Benchmark.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Scenario shore -Entry 1 -ProductionWater -LandscapePilot -NaturalGround -FixedYaw 90 -Resolution 1080p -Quality 2 -Seconds 10
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Package -PackageDirectory 'Builds/WindowsNaturalLandscapeR2' -BuildSlot NaturalLandR2 -LandscapePilot -NaturalGround -MaxParallelActions 4
```

Der Paketbefehl dient zur Reproduktion in einem neuen, noch nicht eingefrorenen Ausgabeordner. Bestehende Nachweispakete dürfen dabei nicht ersetzt werden.

## Gefundener Fehler im Rückwegtest

Der erste Paketkandidat R1 bestand den Murg-Launcher-Test und den Weesen-Uferablauf. In Mols gelang die Rückkehr und das Speichern des abgeschlossenen Tauchgangs, doch die Körperhaltung blieb teilweise in Schwimmstellung: Das Protokoll zeigt einen Schwimmanteil von 0,734 und wiederholt blockierte Übergänge des stehenden Kollisionskörpers. Der Taucher erreichte das Podest rund 59 cm seitlich der Mittellinie. Der gleichartige Kontrolllauf mit den ursprünglichen Bodenmaterialien bestand bei rund 53 cm seitlichem Versatz. Damit ist kein funktionaler Materialfehler nachgewiesen; der bisherige automatische Rückweg war gegenüber Abdrift und Zeitpunkt empfindlich.

Die Teststeuerung in R2 gleicht diesen Versatz beim Rückweg über die normalen Seitwärtstasten aus. Sie berücksichtigt die seitliche Geschwindigkeit, um vor der Mittellinie abzubremsen. Kollision, aufrechte Endhaltung, zulässiger Abschlussbereich, Logbuchprüfung und bisherige Erfolgskriterien bleiben erhalten. Die Änderung gilt nur für den automatisierten Ufertest, nicht als neue automatische Hilfe beim normalen Spielen. R1 bleibt als fehlgeschlagener Testkandidat erhalten.

Die erfolgreiche mittige Route ersetzt keine Prüfung jeder seitlichen Annäherung an das Geländer. Die im ersten Lauf beobachtete blockierte Aufrichtung wird durch diese Testkorrektur im normalen Spiel nicht pauschal beseitigt; sie bleibt ein gesonderter Bewegungsrestpunkt.

Die festen Editor-Bildpaare entstanden vor dieser Teständerung. Ihr gemeinsamer Modulhash ist in den Messdateien erfasst; die damalige DLL wurde vor dem erneuten Kompilieren nicht zusätzlich archiviert. Der endgültige Quellcode-/Modulsnapshot gehört zum R2-Stand. Materialshader und Benchmarkimplementierung sind zwischen diesen beiden Builds unverändert.

## Einschränkung eines historischen Nachweises

Die erste Ablage dieses Schritts verwendete versehentlich den bereits belegten Ordner `artifacts/above-water-ground-20261128`. Dabei wurde dessen älteres `EditorBuild.log` überschrieben. Die Prüfung der 46 Dateireferenzen im historischen Bericht `LANDSCAPE_GROUND_STUDY_RESULTS.json` findet genau diese eine Hashabweichung; die anderen 45 Dateien stimmen weiterhin. Unter 4.150 kleinen lokalen Log-/Textdateien wurde keine identische Sicherung gefunden. Der historische Bericht wird nicht auf den neuen Inhalt umgeschrieben; sein ursprünglicher Buildprotokoll-Nachweis ist damit lokal nicht mehr vollständig erhalten.

Die neue Ablage lautet `artifacts/above-water-natural-ground-20261128`. Das betroffene historische Protokoll wird nicht als Nachweis des neuen Kandidaten verwendet. Die eingefrorenen Quellcode-Snapshots und Spielpakete sind von dieser Ablagekollision nicht betroffen. Der neue Ergebnisbericht führt den Vorfall gesondert auf.
