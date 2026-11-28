# Kartierte Felsflächen im Landschaftsmaterial

Stand: 28. November 2026. Die [erste Felsmaterial-Studie](LANDSCAPE_CLIFF_STUDY.md) verwendet jetzt die vorhandenen swissTLM3D-Flächen **Fels** und **Fels locker** zur Auswahl. Damit hängt die Auswahl nicht mehr von der grünen Farbe gestreckter Luftbilder ab. **Die Daten- und Laufzeitprüfung besteht; die gestalterischen Übergänge sind noch nicht für das normale Spiel freigegeben.**

## Sichtbarer Fortschritt und Grenzen

Vier identische Editor-Kontrollpaare sind aufgenommen und gesichtet: Weesen, Murg und Mols um 12:00 Uhr sowie Murg um 17:00 Uhr, jeweils am 21. Juni, 1600 × 900, mit zwölf Sekunden Einlaufzeit und normalem Hardware-/VSM-Wasserpfad. Die Steuerung startet dieselbe Kamera; die Variante ändert nur das Landschaftsmaterial.

Murg, bisheriges Material:

![Murg Kontrolle](Images/CliffCoverage_Murg_Control.png)

Murg, kartierte Auswahl:

![Murg kartierte Felsflächen](Images/CliffCoverage_Murg_Mapped.png)

Die grossen Felsbänder erhalten räumliche Steinstruktur und weniger helle vertikale Farbstreckung. Das zeigt sich auch am anderen Sonnenstand. Teilweise regelmässig wirkende Materialstruktur, grüne Reststreifen, grobe Bergformen und flacher Fernwald bleiben sichtbar. In Mols verbessert sich vor allem das obere Felsband; Teile der unteren Steilwand bleiben gestreckt.

Weesen zeigt die wichtigste Einschränkung:

![Weesen Kontrolle](Images/CliffCoverage_Weesen_Control.png)

![Weesen kartierte Felsflächen](Images/CliffCoverage_Weesen_Mapped.png)

Helle Materialflecken treffen auf dunkle Luftbildbereiche und bilden auffällige Grenzen. Eine geografisch korrekte Flächenauswahl garantiert noch keinen überzeugenden räumlichen Übergang. Die Darstellung erreicht die [Nutzerreferenz](VISUAL_TARGET.md) nicht; es wird keine neue Spielversion mit dieser Variante ausgeliefert.

## Daten und Laufzeit

Die bereits geprüfte kategoriale [Bodenbedeckung](SURFACE_COVER_MASK.json) wird vor jeder Filterung in zwei unabhängige Kanäle dekodiert: R für Fels, G für Fels locker. B bleibt null, A vollständig deckend. Überlappungen bleiben erhalten; unklassifizierte Flächen werden nicht als Wiese oder Fels umgedeutet. Der gestaltete Beitrag für lockeren Fels beträgt 0,45. Neigung und Entfernung steuern zusätzlich die Mischung mit dem bisherigen Material.

Die lineare RGBA8-Textur hat 4096 × 4096 Pixel und 13 vollständig residente Mipstufen. Unreal mittelt die bereits dekodierte Flächenbedeckung; die ursprünglichen Bitwerte werden nie bilinear gemischt. Filterung ist trilinear, Adressierung geklemmt, Speicherbedarf laut nativer Messung **89’522’176 Byte, rund 85,4 MiB**. Die unkomprimierte Textur ist eine bewusste Vereinfachung für diesen Versuch, keine nachgewiesen optimale Speicherlösung.

Die unabhängige Polygonprüfung verwendet 30’000 deterministische Stichproben je Klasse, einschliesslich Polygonlöchern und der Weltkoordinaten-/UV-Zuordnung. Für beide Klassen gibt es null Abweichungen. Original- und Ausgabedateien sind per SHA-256 gebunden. Das prüft Offline-Pixel und die deklarierte Registrierung; es ist kein GPU-Texel-Readback. Die kartierte horizontale Fläche beschreibt zudem weder Gesteinsfarbe noch die vollständige dreidimensionale Ausdehnung einer Steilwand. [Dekodierung](CLIFF_COVERAGE.json), [unabhängige Prüfung](CLIFF_COVERAGE_AUDIT.json).

Alle acht nativen Aufnahmen bestehen mit bestätigtem Material, vollständig geladener Maske, Uferanschluss, Kollision und normalem Wasserpfad. Kein Material-Kompilierungsfehler oder Ersatzmaterial wurde beobachtet. Die bereits bei der Kontrolle vorhandenen Meldungen des experimentellen Engine-ToolsetRegistry-Plugins bleiben davon getrennt. Der Editor-Modulbau und der isolierte Content-Bau sind erfolgreich. [Bilder, Protokolle, archivierte Eingaben und Prüfsummen](LANDSCAPE_CLIFF_COVERAGE_RESULTS.json).

## Leistung: noch nicht freigegeben

Vier Source-Editor-Rundblicke in Murg, je 15 Sekunden Aufwärmen und 60 Sekunden Messung, RTX 3090 / Ryzen 9950X3D, 1920 × 1080, Qualität 2, 100 % Renderauflösung. Reihenfolge: Kontrolle, kartierte Variante, kartierte Wiederholung, Kontrollwiederholung. Die Wiederholung wurde durch den ersten grossen Aussetzer ausgelöst. Es lief jeweils nur eine Spielinstanz; die übrige Hostlast ist in den Rohdaten erfasst.

| Lauf | Mittel FPS | 1%-Low FPS | P95 ms | Maximum ms | Bilder >33,33 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Kontrolle | 70,72 | 52,13 | 16,58 | 42,08 | 1 / 4’241 |
| Kartierte Variante | 69,71 | 38,65 | 17,08 | 230,56 | 2 / 4’180 |
| Kartierte Wiederholung | 74,44 | 39,26 | 15,47 | 375,89 | 1 / 4’464 |
| Kontrollwiederholung | 76,96 | 42,83 | 15,03 | 132,98 | 6 / 4’609 |

**Keine Leistungsfreigabe:** Beide kartierten Läufe enthalten einen langen Aussetzer nach ungefähr 47,5 Messsekunden. Der jeweils vorhergehende CSV-Timing-Sample zeigt 230,44 beziehungsweise 375,84 ms auf dem Game-Thread, darin 116,11 beziehungsweise 163,15 ms exklusive Speicherbereinigungszeit. Die benachbarte GPU-Zeit liegt bei rund 12 ms. Das stützt einen CPU-/Speicherbereinigungs-Zusammenhang, belegt aber noch nicht die vollständige Ursache oder eine alleinige Verursachung durch die neue Textur. Auch die Kontrollen zeigen Speicherbereinigungs- und weitere Render-Thread-Aussetzer. Alle Aussetzer bleiben in Mittelwerten und Rohdaten enthalten; keine Auswahl des günstigeren Wiederholungslaufs.

Die vier Abschlussbilder sind gesichtet. Der zeitbasierte Rundblick bleibt ortsfest; die Abschlusskurse unterscheiden sich leicht. Eine kontinuierliche Sichtprüfung der Bewegung, statistische Gleichwertigkeit und Paketleistung sind damit nicht nachgewiesen. Die Messungen ersetzen keine Freigabe des normalen Spiels. [Vollständige CSVs, Hostdaten und Aussetzeranalyse](LANDSCAPE_CLIFF_COVERAGE_RESULTS.json).

## Reproduktion

```powershell
./Tools/.geodata-env/Scripts/python.exe Tools/prepare_cliff_coverage.py
./Tools/.geodata-env/Scripts/python.exe Tools/validate_cliff_coverage.py
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Editor -MaxParallelActions 4
./Tools/Build.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -Target Content -OnlyFarCliffs
./Tools/SmokeTest.ps1 -EngineRoot 'O:/games/dive game/Builds/UE584' -EditorSandbox -SandboxName SourceEditorSandbox -ProductionWater -Scenario shore-preview -Entry 1 -Hour 12 -Date 2026-06-21 -MaxFPS 30 -ShorePreviewSeconds 12 -FarCliffStudy
```

Für die Kontrolle `-FarCliffStudy` weglassen. Uferindizes: Weesen 0, Murg 1, Mols 2. Die benötigten ursprünglichen Geodaten sind im Quellenbericht aufgeführt. Die 43 vorher bestehenden Materialien und alle 53 Laufzeitdateien des eingefrorenen R3-Pakets sind unverändert. Der normale Materialpfad bleibt `M_LandscapePhoto`; der Versuch ist nur im Editor aktiv.

## Nächster Schritt

Helligkeits- und Auswahlübergänge anhand der Weesen-Ansicht untersuchen, einschliesslich schmaler Kartenflächen auf steilen Wänden. Die wiederholten CPU-/Speicherbereinigungs-Aussetzer müssen vor einer Leistungsfreigabe mit einem Laufzeit-Trace eingegrenzt werden. Parallel dazu den Anteil von Geländeabtastung und Darstellung an den groben Formen prüfen: Die vorhandenen 2-m-Quelldaten wurden für die Landschaft zu 8-m-Flächenmitteln zusammengefasst. Ein Vergleich mit diesen Quelldaten muss einer Änderung der Geländeform vorausgehen. Danach folgen Waldvolumen und Nahbereiche. Bewegte Sichtprüfung und Paketvalidierung bleiben vor normaler Übernahme erforderlich; die fünf Unterwasser-Runden bleiben abgeschlossen.
