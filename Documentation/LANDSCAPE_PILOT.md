# Spielbare Gelände-Testversion

Stand: 28. November 2026. Die bislang nur im Editor geprüften Kacheln 04_02 und 05_02 erhalten einen gemeinsamen Laufzeitpfad für Editor und gebautes Windows-Spiel. Dies ist ein begrenzter Schritt zur detaillierten Überwasserwelt, keine grafische Abnahme der gesamten Landschaft.

## Inhalt und Aktivierung

`-DiveLandscapePilot` aktiviert genau zwei Ersatzmeshes: den Westnachbarn mit korrigierter Randgeometrie und die 2-m-Kachel östlich davon. Beide halten ihre Nanite-Seiten vollständig vor. Alle anderen Kacheln bleiben unverändert. Die 32-m-Randangleichung, Quellpositionen, korrigierte konventionelle Ersatzgeometrie und ursprünglichen Meshgrenzen werden aus der bereits geprüften Variante übernommen.

Die endgültigen Assets liegen in `Content/Diving/LandscapePilot`, die Höhendaten und ihre Herkunft in `Content/Diving/Geodata/LandscapePilot`. Der Laufzeitpfad lädt diese beiden Meshes direkt. Er benötigt keine Editor-Metadaten, MeshDescription-Funktionen oder Diagnosemeshes aus `LandscapeStudy`. Die bisherigen Landschaftskacheln werden bei diesen beiden Ersetzungen nicht zuvor geladen. Das normale Spiel ohne den Schalter behält seinen bisherigen Pfad.

Vor der Aktivierung prüft der Loader Registrierungsdaten, Rastergrösse, Abstand, Herkunftshashes der Grund- und Detailhöhen sowie gültige Höhenwerte. Bei den Meshes müssen alle Nanite-Seiten resident sein, Seiten-/Fallback-Dreieckszahlen zum Manifest passen und die Grenzen plausibel sein. Die beiden Assets dürfen zusammen höchstens 2’048 Root-Seiten belegen, entsprechend 64 MiB Seitenplätzen in der verwendeten Engineversion. Aktuell sind es 1’330 Seiten. Diese Grenze ist kein Budget für den gesamten Grafikspeicher.

Die Bodenabtastung verwendet im gebauten Spiel dieselben 2-m-Dreiecke wie die Editorprüfung. Ausserhalb der feinen Kachel behalten bestehende Ufer- und Küstenflächen Vorrang; nur zuvor nicht unterstütztes Binnenland nutzt die alten 8-m-Dreiecke. Die Landschaftsmeshes selbst bleiben ohne Mesh-Kollision. Vollständige Körperkollision an beliebigen Felswänden ist damit weiterhin nicht belegt.

## Erzeugung und Prüfung

Die Erzeugung kontrolliert die Herkunftskette der feinen und korrigierten Ausgangsassets. Die nativen Residency-Prüfungen vergleichen LOD0- und HiRes-Quellpositionen sowie Meshgrenzen. Anschliessend werden die Laufzeitdaten erzeugt und durch den tatsächlichen Loader gelesen. `VerifyOnly` prüft die gespeicherten Daten erneut ohne deren Erzeugung. Der Paketbau verlangt diese Prüfung vor dem Cook.

```powershell
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Editor -MaxParallelActions 4 -RefreshSources
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target LandscapeFallback -LandscapePilot
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Package -LandscapePilot -BuildSlot LandPilotR1 -MaxParallelActions 4 -PackageDirectory Builds/WindowsLandscapePilotR1
./Tools/SmokeTest.ps1 -Packaged -PackageDirectory Builds/WindowsLandscapePilotR1 -LandscapePilot -LandscapeTurnWalk -Scenario land-detail -Entry 1 -ProductionWater -Viewport 1920x1080 -TimeoutSeconds 300
```

Der Paketbau legt einen Starter ab, der `-DiveLandscapePilot` an den normalen Bootstrap-Launcher übergibt. Die separate Testversion liegt unter `Builds/WindowsLandscapePilotR1/Start-DiveSimulator.cmd`. Der direkte Aufruf von `divingsim.exe` ohne diesen Schalter verwendet weiterhin das bisherige Gelände. Die eingefrorene Unterwasser-Abnahmeversion R3 bleibt erhalten.

## Ergebnis im gebauten Windows-Spiel

Editor- und Spielkompilierung, erneute Assetprüfung und Cook sind bestanden. Der abschliessende BuildCookRun dauerte 388,90 Sekunden. Der Paketinhalt enthält genau die beiden Pilotmeshes (vier IoStore-Chunks), zusammen 94.987.980 logische bzw. 45.720.312 komprimierte Bytes. Das ist nur deren Mesh-Nutzlast, nicht die Paketgrösse. Diagnosemeshes aus `LandscapeStudy` fehlen im Cook; dessen alte Höhendaten werden derzeit noch als UFS-Daten mitgeliefert.

Der native Geh-/Wendetest legte 105,35 m zurück und überquerte die gemeinsame Grenze zweimal, einschliesslich einer tatsächlichen Kamerawende um 180°. Die Laufzeitprüfung kontrollierte Bodenauflage und Streckenführung; 105 Messpunkte wurden protokolliert. Alle sechs gespeicherten Bilder wurden geprüft: keine offene Grenze an dieser Teststrecke. Dies belegt weder sämtliche Kachelgrenzen noch das Ausbleiben kurzer Bildfehler während einer beliebigen Spielstrecke.

Der normale Bootstrap-Launcher startete die erwartete ausführbare Datei mit aktivem Pilot. Das native Tauchszenario in Murg und das Uferszenario in Weesen bestanden ebenfalls. Die Weesen-Prüfung enthält einen gesteuerten Positionswechsel für die Rückkehr und ist kein durchgehender manueller Rundweg. Dazu kamen 54 bestandene Python-Tests und erfolgreiche Syntaxprüfungen der vier angepassten PowerShell-Werkzeuge.

## Leistung und Speicher

Je ein Lauf im selben Spielpaket: RTX 3090, 1920×1080, Qualitätsstufe 2, Auflösungsskala 100 %, 15 Sekunden Aufwärmzeit und 60 Sekunden Messung, drehender Blick am Ufer von Murg, unbegrenzte Bildrate. Die Basis verwendet das bisherige 8-m-Gelände; der Pilot ersetzt die zwei Kacheln. Dies ist kein Vergleich derselben feinen Geometrie mit unterschiedlichen Streaming-Einstellungen.

| Messwert | Bisheriges Gelände | Pilot |
| --- | ---: | ---: |
| Durchschnitt | 77,75 FPS | 77,90 FPS |
| Langsamstes 1 % | 64,26 FPS | 64,22 FPS |
| 95. Perzentil | 14,77 ms | 14,76 ms |
| Längster Frame | 16,21 ms | 16,61 ms |
| Frames über 33,33 ms | 0 / 4.663 | 0 / 4.672 |
| GPU-Zeit, Mittel | 12,25 ms | 12,22 ms |
| Nanite-Root-Pool, reserviert | 48 MiB | 96 MiB |
| Prozessspeicher, Maximum | 4.976,07 MiB | 4.985,43 MiB |

In diesem Messpaar ist kein relevanter Bildratenunterschied erkennbar. Es ist keine allgemeine Leistungsfreigabe oder Aussage über lange Sitzungen. Der Root-Pool wächst um 48 MiB; der zusätzlich protokollierte `RootDataSizeMB`-Wert steigt von 40,6562 auf 82,1562. Letzterer beschreibt den Adressbereich des Root-Seitenallocators einschliesslich möglicher Lücken, nicht die komprimierte CPU-Nutzlast und nicht den gesamten GPU-Speicher. Die früher beobachteten CPU-/GC-Aussetzer sind durch zwei kurze Läufe ohne solche Aussetzer nicht grundsätzlich behoben.

Die beiden abschliessenden Uferbilder wurden ebenfalls geprüft. Grobe Materialübergänge und einzelne schwebende Bäume sind bereits in der Basis sichtbar und bleiben offen. Die Pilotkacheln liegen im Binnenland; ihr Detailgewinn ist deshalb vor allem in der gesonderten Bergstrecke zu beurteilen.

[Messwerte, Dateihashes und eingefrorene Nachweise](LANDSCAPE_PILOT_RESULTS.json) dokumentieren den genauen Prüfstand. Das Paket ist eine spielbare technische Testversion, keine Abnahme des angestrebten fotorealistischen Überwasserbildes.

Bekannte offene Punkte aus der [bewegten Grenzprüfung](LANDSCAPE_RESIDENT_WALK.md): eckige Schattierungs-/Materialkante im westlichen Rückblick, Vegetationsnachladen, grosse Editorassets, CPU-/GC-Aussetzer und die noch nicht erreichte Detailqualität von Wald, Wiesen, Felsen und Ufer. Die akzeptierte Unterwasserwelt wird nicht als weitere gestalterische Iteration geöffnet.
