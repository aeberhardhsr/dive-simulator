# Begehbare Geländegrenze und Nanite-Speicher

Stand: 28. November 2026. Fortsetzung der [stationären Residency-Prüfung](LANDSCAPE_NANITE_BOUNDARY.md). Geprüft wird weiterhin nur die gemeinsame Grenze der Kacheln 04_02 und 05_02 im isolierten Editorversuch. Die akzeptierte Unterwasserwelt und das eingefrorene R3-Spielpaket bleiben unverändert.

## Korrektur am Testablauf

Die erste Wiederholung der Gehstrecke hat einen Testfehler aus der vorherigen Erweiterung gefunden: Der Zeitstempel `TutorialFinishedAt` wurde beim anfänglichen Platzieren sowohl für die stationäre Probe als auch für die Gehprobe gesetzt. `ADivePawn::Tick` beendet bei einem positiven Wert die Simulation vor dem Bewegungsschritt. Die Figur blieb deshalb stehen; die Gehprobe brach korrekt nach ihrem Zeitlimit ab. Dieser fehlgeschlagene Lauf ist aufbewahrt und wird nicht als bestandener Test gezählt.

Jetzt wird dieser Zeitstempel beim Start nur für die ausdrücklich stationäre Variante gesetzt. Die normale Spielsteuerung war nicht betroffen. Die früher dokumentierten Gehstrecken entstanden vor der Einführung dieser Regression; die stationären Bilder bleiben als Standbildversuch gültig.

Die zusätzliche Option `-LandscapeTurnWalk` prüft die zweite Blickrichtung: Die Figur geht über die Grenze, bremst mit der gewöhnlichen Bewegungsphysik ab, dreht die Kamera über 1,5 Sekunden um 180 Grad und geht vorwärts zurück. Bewegung und Körperkontaktprüfung bleiben während der Wende aktiv. Nur die anfängliche Platzierung ist eine Versetzung. Kameraausrichtung erfolgt automatisiert über den Controller; dies ersetzt keine Prüfung manueller Mauseingaben.

Bodenstützung, Gehzustand und seitliche Abweichung werden in jedem Test-Tick kontrolliert. Die Schrittprotokolle enthalten Position und Fuss-/Bodenhöhe ungefähr pro zurückgelegtem Meter. Die ursprüngliche Variante erzeugt drei Bilder, die Wendevariante sechs, darunter beide Grenzübertritte und die Kamerawende. Einzelbilder belegen keine lückenlose Abwesenheit kurzer Aufblitzer zwischen den Aufnahmen. Diese Läufe dienen der Funktionsprüfung, nicht der FPS-Messung.

Beide Wiederholungen mit demselben neu gebauten Modul sind bestanden:

| Gehstrecke mit residenten Daten | Zurückgelegter Weg | Protokollierte Bodenproben | Grösste protokollierte Fuss-/Bodenabweichung |
| --- | ---: | ---: | ---: |
| Vorwärts hin, rückwärts zurück | 105,3441 m | 105 | 0 cm |
| Vorwärts hin, 180°-Wende, vorwärts zurück | 105,3505 m | 105 | 0 cm |

Alle neun Bilder wurden in voller Auflösung geprüft. In den geprüften Ansichten ist kein offener Geländespalt sichtbar. Der Rückblick nach Westen zeigt weiterhin eine eckige Schattierungs- oder Materialkante am Hang; deren Ursache ist durch diese Funktionsprobe nicht bestimmt. Auch die nachladende Vegetation und die noch gleichförmigen Wiesen bleiben sichtbar. Es gibt deshalb keine allgemeine Freigabe der Grafikqualität. Der native Build, die PowerShell-Syntaxprüfung und alle 54 bestehenden Python-Tests sind erfolgreich.

## Speicherbedarf aus den bestehenden Leistungsläufen

Die bereits eingefrorenen 60-Sekunden-Aufnahmen enthalten die nativen CSV-Zähler des Nanite-Streamingmanagers. Ihre erneute Auswertung verändert weder die Originalaufnahmen noch deren bisherigen Bericht. Verwendet werden dieselben inneren Frames wie im bestehenden Leistungsanalysator; Start- und Stopframe bleiben ausgeschlossen.

| Engine-Zähler | Gestreamte Kontrolle | Zwei vollständig residente Kacheln |
| --- | ---: | ---: |
| Zugewiesener globaler Root-Pool | 48 MiB | 96 MiB |
| Adressspanne des Root-Allocators | 40,6875–40,75 MiB | 82,25–82,3125 MiB |

Der Enginecode verwendet 32 KiB pro Root-Seite. Die beiden vollständig residenten Assets besitzen zusammen 1’330 Root-Seiten und beanspruchen damit 41,5625 MiB an Root-Seitenplätzen. Der Pool wächst in diesem Speicherpfad in Schritten von 16 MiB; im beobachteten Vergleich ergeben sich **48 MiB zusätzliche Root-Pool-Zuweisung**. Die entsprechenden Enginequellen und Zähler sind mit Hashwerten im lokalen Snapshot festgehalten.

`RootAllocationMB` bildet die Root-Pool-Zuweisung des Streamingmanagers ab. `RootDataSizeMB` verwendet trotz seines Namens die Adressspanne des Root-Allocators, nicht die komprimierten CPU-`RootData` der Assets. Freie Bereiche im Allocator können enthalten sein. Beide Werte beschreiben weder den gesamten Grafikspeicherverbrauch noch alle Hierarchie-, Fallback-, Textur- oder Renderzielpuffer. Die aus den Assetdaten zuvor ermittelten 36,9 MiB CPU-Daten sind eine andere Messgrösse.

Die Editorstudie lädt zudem Quellmeshes und Diagnoseklone, auch wenn nur die ausgewählten Klone gerendert werden. Ein gebautes Spiel mit direktem Laden der endgültigen Assets muss separat gemessen werden. Aus dieser lokalen Probe folgt keine Empfehlung, sämtliche 77 Landschaftskacheln vollständig resident zu halten.

## Nächster Umsetzungsschritt

Der begrenzte Pilot bleibt auf diese beiden Geländekacheln beschränkt. Für eine spielbare Übernahme müssen die endgültigen Assets ausserhalb des vom Cook ausgeschlossenen Studienordners liegen und direkt geladen werden. Die zum feinen Mesh passende Bodenabtastung braucht denselben geprüften Laufzeitpfad im gebauten Spiel. Erst ein separater Cook mit anschliessender Streckenprüfung und Speicheraufnahme kann diese Übernahme bestätigen. Das bisherige Spielpaket bleibt bis dahin die unveränderte Vergleichsbasis.

Die grossen Editorassets, bekannte CPU-/Garbage-Collection-Aussetzer und die sichtbare Qualität von Wald, Wiesen, Felsübergängen und Uferdetails bleiben offen. Die technischen Grenzprüfungen sind keine grafische Abnahme der Überwasserwelt.

Reproduktion nach dem in der Vorgängerdokumentation beschriebenen Residency-Bake:

```powershell
./Tools/SmokeTest.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -ProductionWater -Scenario land-detail -Entry 1 -LandscapeDetailStudy -LandscapeFallbackStudy -LandscapeResidentStudy -LandscapeTurnWalk -Viewport 1920x1080 -TimeoutSeconds 300
```

Für den Rückweg ohne Kamerawende `-LandscapeTurnWalk` weglassen. Die Bilder vor dem nächsten Lauf getrennt archivieren, da das Screenshot-Verzeichnis wiederverwendet wird.

[Messdaten und eingefrorene Nachweise](LANDSCAPE_RESIDENT_WALK_RESULTS.json).

Fortsetzung: Der [spielbare Gelände-Pilot](LANDSCAPE_PILOT.md) setzt den beschriebenen nächsten Schritt in einem separaten Windows-Paket um. Die Nachweise dieser Editorstudie bleiben unverändert erhalten.
