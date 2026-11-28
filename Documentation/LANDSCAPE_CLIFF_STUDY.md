# Räumliche Felsoberflächen der entfernten Bergflanken

Stand: 28. November 2026. **Erste sichtbare Verbesserung im Source-Editor; noch keine Übernahme ins normale Spielpaket.** Die neue Landschaftsvariante vermindert in den Murg-Vergleichen die hellen senkrechten Luftbildstreifen und ergänzt räumlich projizierte Steinstruktur. Gestreckte Grünanteile, teilweise harte Felsgrenzen und grobe Geländeformen bleiben sichtbar. Die Referenzqualität ist nicht erreicht.

## Vergleich mit identischer Kamera

Murg/West, 21. Juni, 12:00 Uhr, 1600 × 900, gleicher Source-Editor, normaler Hardware-/VSM-Wasserpfad. Beide Aufnahmen nach zwölf Sekunden Einlaufzeit:

Vorher:

![Murg mit bisherigem Landschaftsmaterial](Images/Cliffs_Murg_Before.png)

Neue Variante:

![Murg mit räumlicher Felsstruktur](Images/Cliffs_Murg_Study.png)

Der zusätzliche Murg-Vergleich um 17:00 Uhr zeigt die Struktur auch bei anderem Sonnenstand. Weesen und Mols sind mit der neuen Variante gesichtet; dort dienen die früheren Paketbilder als Kontext, nicht als neue identische Editor-Kontrollpaare. In Weesen werden grobe Geländeformen und harte Materialgrenzen deutlicher. In Mols bleiben grün gefärbte senkrechte Streifen bestehen. Die aktuelle Auswahl anhand Neigung und Luftbildfarbe reicht deshalb noch nicht für die normale Integration.

## Umsetzung und Begrenzung

`M_LandscapeCliffStudy` ist eine eigene Kopie des bestehenden Landschaftsmaterials. Zwei räumlich projizierte Gesteinsskalen, grossflächige Variation und ein niedrig aufgelöster geografischer Farbton ersetzen die stark gestreckte Projektion auf ausgewählten steilen Flächen. Flachere und grüne Bereiche bleiben überwiegend beim Luftbild. Das ist gestaltete Materialstruktur, keine zusätzlich vermessene Geologie; es entstehen keine neuen Geländeformen.

Die Auswahl erfolgt ausschliesslich im Editor mit `-DiveFarCliffStudy`. Der normale Pfad verwendet weiter `M_LandscapePhoto`. Alle 43 zuvor vorhandenen Materialdateien sind bytegleich erhalten; ebenso alle 53 Laufzeitdateien des eingefrorenen R3-Pakets. Der neue Content-Bau ruft den gemeinsamen Ufer-/Unterwasser-Materialbau nicht auf.

## Prüfung und verworfene Ergebnisse

Acht native Läufe bestehen und ihre Bilder sind gesichtet: zwei frühe Murg-Ansichten nach drei Sekunden, sechs Ansichten nach zwölf Sekunden (Murg Kontroll-/Studienpaar, Weesen, Mols, Murg Kontroll-/Studienpaar am Abend). Uferanschlüsse, Kollision, richtige Materialauswahl und normaler Wasser-Renderpfad sind bestätigt. [Protokolle, Bilder, Eingaben und Prüfsummen](LANDSCAPE_CLIFF_STUDY_RESULTS.json).

Ein früher Content-Bau scheiterte an einem Python-Property-Zugriff. Danach wies die erste Renderprüfung einen Shaderfehler nach: Die gespeicherte ältere Materialkopie besitzt `Noise`, aber noch nicht `PatchNoise`. Beide Fehler sind korrigiert. Der Lauf mit Ersatzmaterial wurde ausdrücklich verworfen und wird nicht zu den acht gültigen Läufen gezählt.

Das erste gültige Studienbild nach drei Sekunden zeigt auffällig dunkles Wasser. In den zwölfsekündigen Mittags- und Abendvergleichen besteht dieser Unterschied nicht fort. Der Einlauf-/Startzustand trägt damit zum Befund bei; die genaue Renderer-Ursache ist nicht isoliert. Das dunkle Frühbild bleibt als Einschränkung dokumentiert. Die neue einstellbare Vorlaufzeit im Testwerkzeug ändert keinen normalen Spielstart.

Die Source-Editor-Läufe enthalten auch bereits im Kontrolllauf vorhandene Meldungen des experimentellen Engine-ToolsetRegistry-Python-Plugins. Sie werden nicht als neuer Materialfehler ausgegeben; eine völlig meldungsfreie Engine wird nicht behauptet.

## Erste Leistungsmessung

Je ein 60-Sekunden-Uferrundblick in Murg nach 15 Sekunden Aufwärmen, Source-Editor auf RTX 3090/9950X3D, 1920 × 1080, Qualität 2, 100 % Renderauflösung, ohne Bildratenlimit. Gleiche Spiel-/Renderer-Module und dieselben Eingaben; keine parallele Spielinstanz. Die beiden Abschlussbilder sind gesichtet. Die Kamera dreht, die Spielfigur bleibt am Ort.

| Variante | Mittel FPS | 1%-Low FPS | P95 ms | Maximum ms | Bilder >33,33 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Bisheriges Material | 74,50 | 53,22 | 15,71 | 62,10 | 1 / 4’468 |
| Neue Felsvariante | 74,33 | 57,47 | 15,67 | 36,00 | 1 / 4’458 |

Die ähnlichen Mittelwerte zeigen in diesem einzelnen Paar keine grosse Verschlechterung. Sie beweisen weder Kostenfreiheit noch eine Beschleunigung, statistische Gleichwertigkeit oder durchgehend 60 FPS. Editorwerte sind keine Paketmessung. Die Endkurse können wegen des zeitbasierten Rundblicks geringfügig abweichen; eine bildgenau identische Kamerafahrt wird nicht behauptet. Die kontinuierliche Bildprüfung des bewegten Blicks bleibt offen; numerische Messung und Abschlussbild ersetzen sie nicht. Vollständige CPU-/GPU-Werte und Rohdaten sind im [Prüfbericht](LANDSCAPE_CLIFF_STUDY_RESULTS.json) gebunden.

## Reproduktion

```powershell
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Editor -MaxParallelActions 4
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Content -OnlyFarCliffs
./Tools/SmokeTest.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -ProductionWater -Scenario shore-preview -Entry 1 -Hour 12 -Date 2026-06-21 -MaxFPS 30 -ShorePreviewSeconds 12 -FarCliffStudy
```

Für die Kontrolle `-FarCliffStudy` weglassen. `-Entry 0` ist Weesen, `-Entry 2` hier Mols; das sind Uferansichten, keine Offshore-Startindizes. `-Hour 17` reproduziert den Abendvergleich. Der Content-Bau verändert nur das zusätzliche Studienmaterial.

## Nächste notwendige Verbesserung

Die Fels-/Vegetationsauswahl muss stärker auf geografischen Flächen statt auf gestreckten Luftbildfarben beruhen. Danach sind die Übergänge auf den groben Geländeformen erneut zu beurteilen. Ein Material kann fehlende räumliche Felsgliederung nicht ersetzen. Bewegte Sichtprüfung und spätere Paketvalidierung bleiben vor normaler Übernahme erforderlich; keine weitere Unterwasser-Gestaltungsrunde wird eröffnet.
