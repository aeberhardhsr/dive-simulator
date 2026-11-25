# Walensee Diving Simulator

Ein spielbarer Unreal-Prototyp für einen realitätsnahen Tauchsimulator. Der vermessene Walensee-Grund enthält Tarierung, Trägheit, Wasserwiderstand, tiefenabhängigen Gasverbrauch, gekoppelte Spiel-Vitalwerte, Instrumente, Fische, Pflanzen, Atmung und gespeicherte Tauchprotokolle. Die umgebende Landschaft verwendet 77 Nanite-Geländekacheln aus dem 8-m-Höhenraster.

**Entwicklungsstand:** 46 swisstopo-Rasterkacheln bilden den vermessenen Seegrund im Maßstab 1:1 ab. Drei Uferstarts bei Weesen Strandbad, Murg West und Mols/Lediwracks ermöglichen Gehen, Waten, Schwimmen und die Rückkehr an Land. Die Zugangsplattformen und Treppen sind modellierte Spielgeometrie auf örtlichen Höhendaten. Die bisherigen Offshore-Starts bleiben wählbar. Gelände wird um den Taucher nachgeladen; die Tiefenkarte zeigt Position und Spur. Umliegende Berge basieren auf swissALTI3D; Berge und Ufer tragen registrierte SWISSIMAGE-Luftbilder des Kacheljahrs 2025. Räumliche Gebäude/Vegetation, originalgetreue Zugangsanlagen, fotorealistische Grafik und die vollständige Anforderungsabnahme bleiben offen. Das Spielmodell ist kein Werkzeug zur Planung realer Tauchgänge.

## Starten

Nach einem lokalen Package-Build: `Builds/Windows/divingsim.exe` starten. Der gesamte benachbarte Ordner wird benötigt. Im Menü Ausrüstung, Umgebung und **Start: am Ufer / direkt im Wasser** einstellen, dann starten. Neue Einstellungen beginnen am Ufer; ältere gespeicherte Pläne behalten ihren Wasserstart. **Exploration** bietet unbegrenztes Gas und automatische Tarierung; **Simulation** verwendet manuelle Tarierung und begrenztes Gas. Hardcore ist derzeit als noch nicht vollständig implementiert gekennzeichnet.

Aus dem Quellprojekt:

```powershell
./Tools/Build.ps1 -Target Editor
./Tools/Play.ps1
```

Benötigt: Unreal Engine **5.8.4**, Visual Studio 2022 C++ Build Tools/MSVC 14.44, Windows SDK 10.0.22621 und .NET Framework 4.8 SDK. Das Skript erwartet Unreal unter `C:/Program Files/Epic Games/UE_5.8`; einen anderen Ort mit `-EngineRoot` übergeben. Binäre Projektdateien verwenden Git LFS: nach dem Klonen `git lfs pull` ausführen. Die vorhandenen Template-Assets bleiben erhalten.

## Bedienung

| Taste | Funktion |
|---|---|
| Maus | Blickrichtung |
| WASD | Am Boden gehen, im Wasser schwimmen; vorwärts / seitwärts |
| Leertaste / Strg | Nach oben / unten schwimmen |
| Umschalt | Schneller gehen / kräftiger Flossenschlag, höhere Belastung |
| Q / E | Jacket füllen / entleeren |
| T | Atemregler einsetzen / absetzen |
| L | Lampe |
| C | Handgelenkcomputer anheben / senken |
| R | Computerseite wechseln, wenn angehoben: Tauchdaten / Vitalwerte / Gas |
| M | Walensee-Tiefenkarte mit Position und Spur |
| P | Foto |
| H | Instrumente aus-/einblenden |
| Esc | Pause und Menü |
| F1 | Steuerung und Hilfe öffnen |
| F | Nach dem Tauchen auf dem Startpodest abschließen; beim Wasserstart nahe der Einstiegsboje |

BCD-Gas dehnt sich beim Aufstieg aus. Gegensteuern und rechtzeitig entlüften. Im Pausenmenü steht die Simulation still; die Ausrüstung lässt sich während eines laufenden Tauchgangs nicht ändern.

Beim Uferstart führen Podest und Treppe ins Wasser. Du beginnst mit abgesetztem Atemregler und atmest Umgebungsluft ohne Flaschenverbrauch. Vor dem Eintauchen mit **T** den Regler einsetzen; nach der Rückkehr kannst du ihn wieder absetzen. Ohne Regler unter Wasser fehlt Atemluft: Die Warnung erscheint sofort; zunächst wird eine begrenzte Atemreserve verbraucht. Erst bei niedriger Reserve sinkt die simulierte SpO2. Belastung beschleunigt den Verbrauch, Atmen füllt die Reserve allmählich wieder auf. Die Prozentanzeige ist ein Spielwert, keine sichere Atemanhaltezeit. Das Jacket verwendet auch beim Atmen von Umgebungsluft Flaschengas. Wasserstarts beginnen mit eingesetztem Regler.

Der Wechsel zwischen Gehen/Waten und Schwimmen folgt Bodenkontakt und Wassertiefe; die Kamera bleibt auf Augenhöhe. Nach dem Tauchen zur selben Treppe und auf das flache Podest zurückkehren. Abkühlung berücksichtigt Lufttemperatur, Wind und eingetauchten Körperanteil. Dauer und Strecke enthalten den Landweg. Körperhaltung, Flossen an Land und Wärmehaushalt bleiben vereinfacht.

Die Tabelle zeigt die Standardbelegung. Unter **F1 / Steuerung und Hilfe** lassen sich alle 17 Spielaktionen auf Tastaturtasten oder rechte/mittlere/seitliche Maustasten legen. Eine belegte Taste tauscht die beiden Aktionen; Escape bricht die Auswahl ab. Escape, F1, F11, die Konsole, Alt/Windows und der linke Mausklick bleiben reserviert. Hinweise im Spiel zeigen die aktuelle Belegung. Mausgeschwindigkeit, Y-Invertierung und Sichtfeld sind ebenfalls einstellbar und werden gespeichert. **Standards wiederherstellen** setzt diese Steuerungsoptionen zurück.

**Steuerung kennenlernen** im Startmenü beginnt eine geführte Einführung mit acht Aufgaben. Fortschritt entsteht durch Umsehen, eigenes Schwimmen, Füllen/Entleeren des Jackets, die Vitalwerteseite des Computers, Lampe, Karte und Rückkehr zur Boje. Sie verwendet ein ruhiges Weesen-Szenario mit manueller Tarierung und begrenztem Gas; der vorherige Tauchplan wird nach Abschluss oder Abbruch wiederhergestellt. Der Abschluss wird gespeichert und die Einführung kann wiederholt werden. Sie erklärt die Spielbedienung und ist keine Tauchausbildung.

**Tauchprofile** unten im Menü speichert benannte Zusammenstellungen aus Ausrüstung, Einstieg, Umwelt und Tauchplan. Einen Namen eingeben und mit Enter oder **Speichern** bestätigen; Escape bricht den Dialog ab. Profile lassen sich laden, umbenennen, mit dem aktuellen Plan aktualisieren und archivieren. **Vorheriger Stand** stellt die letzte Änderung an Name oder Konfiguration wieder her, auch nach einem Neustart. Das Archiv erlaubt die Wiederherstellung; es gibt keine endgültige Löschfunktion. Ein laufender Tauchgang verhindert das Laden anderer Profile.

Bis zu 32 aktive und 128 Profile insgesamt liegen lokal in `divingsim/Saved/DiveProfiles.json`. Namen müssen eindeutig sein, auch gegenüber dem Archiv. Die Datei wird beim Speichern geprüft und über eine temporäre Datei ersetzt; unlesbare Dateien bleiben unangetastet. **Neu laden** liest Änderungen anderer Spielinstanzen erneut ein. Profile enthalten den Tauchplan; Sprache, Audio und Steuerung bleiben gemeinsame persönliche Einstellungen.

Mit C wird der sichtbare Handgelenkcomputer angehoben. R wechselt zwischen Tauchdaten, Vitalwerten und Gasdaten; Tiefe, Druck, Zeit und Kurs bleiben auf allen Seiten sichtbar. Er funktioniert auch bei mit H ausgeblendetem Bildschirm-HUD. Die sichtbare Handlampe verwendet denselben Schalter und Batteriestand wie das Licht im Spiel.

**Wetter und Wasser** öffnet die Umweltkonfiguration: heiter/bedeckt/Regen/Sturm, Windstärke und -richtung, Lufttemperatur (−10 bis 40 °C), Wassertemperatur an Oberfläche und in der Tiefe, Sprungschicht, Sichtweite, Strömung und Tageszeit. Die Wetterwahl setzt zunächst 3/5/8/22 m/s Wind; danach ist der Wert von 0 bis 24 m/s frei einstellbar. Wind erzeugt sichtbare Wellen und Wasserbewegung; Taucher, Bojen und Atemblasen reagieren auf dieselbe Oberfläche. Die Werte sind frei gewählte Spielszenarien. Die eingestellte Uhrzeit ist die Startzeit: Während des Tauchgangs läuft sie sekundengenau mit der Simulation weiter, auch über Mitternacht. Pause hält die Uhr und den Sonnenstand an. Die übrigen Umweltvorgaben bleiben fest. **Ausruestung und Tauchplan** führt zu den übrigen Einstellungen zurück.

Zahlenfelder öffnen einen Eingabedialog: beispielsweise **213,5 bar**, **7,5 kg**, **22,5 m** oder **47 min**. Die Einheit gehört nicht in das Eingabefeld. Dezimalpunkt und Komma sind erlaubt; der Dialog nennt den gültigen Bereich. Die Startuhrzeit lässt sich minutengenau von **00:00 bis 23:59** einstellen. Enter oder **Speichern** übernimmt den Wert, Escape oder **Abbrechen** verwirft die Eingabe. Ungültige Werte verändern den Tauchplan nicht. Während eines laufenden Tauchgangs bleibt die Konfiguration gesperrt.

**Audio >** unten im Menü öffnet fünf Lautstärkeregler: Gesamt, Umgebung, Atem/Ausrüstung, Herzschlag und Warnungen. Mit **− / +** in 5-%-Schritten einstellen; 0 % schaltet den Kanal stumm. Einstellungen werden gespeichert und können auch in der Pause geändert werden. Atmung und Herzschlag folgen den simulierten Vitalwerten, Jacket-Geräusche dem tatsächlichen Gasfluss. Blasen steigen als räumliche Klangquelle auf; Wind, Wasser und Regen werden beim Eintauchen gedämpft. Die eigenen synthetischen Klänge sind ein Entwicklungsstand, keine Aufnahme echter Tauchausrüstung.

**Logbuch ansehen** im Start-/Pausenmenü oder **Logbuch / Verläufe** nach dem Tauchgang öffnet gespeicherte Tauchgänge. Links einen Eintrag wählen, mit den Pfeilen blättern oder nach Modus filtern. Vier Diagrammgruppen zeigen Tiefe, Gas, Nullzeit, Dekotiefe, Vitalwerte, Temperatur, Stress, Ermüdung und Strecke. Die Maus über den Kurven wählt den gemeinsamen Zeitpunkt; **< ! / ! >** springt zu Warnereignissen. Neue Protokolle zeigen am ausgewählten Zeitpunkt außerdem Atemquelle, Körperimmersion, gemischte Umgebungstemperatur und die Atemreserve. Esc führt zurück. Beim Lesen aus dem Pausenmenü bleibt der laufende Tauchgang angehalten.

Tauchprotokolle liegen unter `divingsim/Saved/DiveLogs`, Fotos mit Taste P unter `divingsim/Saved/Photographs`. Testaufnahmen liegen unter `divingsim/Saved/Screenshots`. In einem verpackten Development-Build befinden sich diese Verzeichnisse unter dessen eigenem `divingsim`-Ordner. Das Logbuch liest auch ältere Protokolle; automatisierte Testläufe werden in der normalen Ansicht ausgeblendet. Fehlende oder beschädigte Dateien verhindern das Lesen anderer Einträge nicht.

Laufende Tauchgänge werden alle 30 Sekunden Simulationszeit zwischengespeichert, beim regulären Verlassen als unterbrochen und beim Abschluss mit dem Ergebnis. Eine `.json.bak` bewahrt die vorherige gültige Version. Ist die Hauptdatei beschädigt oder fehlt sie, zeigt das Logbuch diesen älteren Stand mit einem Hinweis an. Zwischenstände sind Protokolle; sie erlauben noch kein Fortsetzen des Tauchgangs. Vor der ersten Sicherung und seit der letzten erfolgreichen Sicherung können bei einem Absturz Messpunkte verloren gehen. Schreibfehler während des Tauchgangs werden eingeblendet.

Die laufende Sicherung verarbeitet eine eigene Datenkopie im Hintergrund. Es gibt höchstens einen Schreibauftrag und einen wartenden aktuellen Stand; bei langsamem Datenträger wächst keine unbegrenzte Warteschlange. Abschluss und reguläres Verlassen warten auf die endgültige Speicherung, damit ein älterer Auftrag das Ergebnis nicht später überschreibt.

## Entwicklung und Prüfung

```powershell
./Tools/TestCore.cmd                  # Engineunabhängige Simulationsprüfungen
python -m unittest discover -s Tests -p 'test_*.py'
./Tools/Build.ps1 -Target Content     # Materialien, Importe und Trainingsmap
./Tools/Build.ps1 -Target Content -OnlyEnvironment # Nur Wasser, Wolken und Regen erneuern
./Tools/Build.ps1 -Target Content -OnlyWater       # Nur Wassermaterial erneuern
./Tools/Build.ps1 -Target Content -OnlyOptics # Nur den HDR-Unterwassereffekt erneuern
python Tools/prepare_bed_materials.py # Geprüfte CC0-Quellen für Sediment/Kies/Fels
./Tools/Build.ps1 -Target Content -OnlyBed # Nur die PBR-Unterwasseroberflächen erneuern
python Tools/create_audio.py         # Originale PCM-Klänge reproduzieren
./Tools/Build.ps1 -Target Content -OnlyAudio # Nur Audiomaterial importieren
./Tools/Build.ps1 -Target Landscape   # 8-m-Höhendaten als Nanite-Geländekacheln bauen
./Tools/Build.ps1 -Target LandscapeCheck # Höhen, UVs und Build-Aktualität prüfen
python Tools/validate_landscape_meshes.py # Kachelgrenzen und portable Quellenreferenz
./Tools/SmokeTest.ps1                 # 22 Sekunden automatisiertes Gameplay
./Tools/Build.ps1 -Target Package     # Eigenständiger Windows-Development-Build
./Tools/Build.ps1 -Target Package -BuildSlot DayCycle -PackageDirectory Builds/WindowsDayCycle # Separater Programmname bei geöffnetem Projekt-Spiel
./Tools/SmokeTest.ps1 -Packaged       # Derselbe Ablauf in der verpackten Anwendung
./Tools/Benchmark.ps1 -PackageDirectory Builds/Windows -Scenario underwater -Resolution 1080p # 60 s gerenderte Leistung
./Tools/SmokeTest.ps1 -Packaged -Scenario shore-preview -DisableNanite # Konventionellen Gelände-Fallback prüfen
./Tools/SmokeTest.ps1 -Packaged -Entry 1 -Scenario stream  # 90 s, Kachelwechsel
./Tools/SmokeTest.ps1 -Packaged -Scenario surface         # 38 s, Auftauchen
./Tools/SmokeTest.ps1 -Packaged -Scenario surface-calm    # Windstille
./Tools/SmokeTest.ps1 -Packaged -Scenario surface-calm -Hour 23 # Luft-/Wasserwechsel nachts
./Tools/SmokeTest.ps1 -Packaged -Scenario dive -Hour 23 -LampOff # Nachtvergleich ohne Lampe
./Tools/SmokeTest.ps1 -Packaged -Scenario surface-rain    # Regen und 12 m/s Wind
./Tools/SmokeTest.ps1 -Packaged -Scenario surface-storm -Entry 1 # Sturm und 22 m/s Wind
./Tools/SmokeTest.ps1 -Packaged -PackageDirectory Builds/WindowsDayCycle -Scenario daycycle -Hour (17+59/60) -MaxFPS 30 # 90 s Sonnenuntergang plus echte Pause
./Tools/SmokeTest.ps1 -Packaged -Scenario audio           # Audiomix mit Pause/Stummschaltung aufzeichnen
./Tools/TestControls.ps1              # Unreal-Prüfungen: Belegung, Konflikte, Persistenz, ungültige Werte
./Tools/TestProfiles.ps1              # Unreal-Prüfungen: Profile, Wiederherstellung, Datei-/Konfliktschutz
./Tools/SmokeTest.ps1 -Packaged -Scenario controls        # Engine-Tasteneingaben, Remapping, Pause, DE/EN
./Tools/SmokeTest.ps1 -Packaged -Scenario tutorial        # Alle acht Aufgaben, Abschluss, Planwiederherstellung
./Tools/SmokeTest.ps1 -Packaged -Scenario profiles        # Profilmenü, Namensdialog, Laden/Archiv/Undo, Tauchgang
./Tools/SmokeTest.ps1 -Packaged -Scenario shore-preview -Entry 0 # Uferansicht/Kollision; Entry 0, 1 oder 2
./Tools/SmokeTest.ps1 -Packaged -Scenario shore -Entry 0 # Gehen, Tauchen, Rueckkehr und Abschluss
python Tools/validate_audio_mix.py Builds/Windows/divingsim/Saved/AudioQA/DiveMix.wav artifacts/DiveSmoke.log
./Tools/SmokeTest.ps1 -Packaged -Entry 1 -Scenario wrist  # Drei Handgelenkseiten, HUD aus
./Tools/SmokeTest.ps1 -Packaged -Entry 1 -Scenario wrist-night # Display bei Nacht
./Tools/SmokeTest.ps1 -Packaged -PackageDirectory Builds/WindowsConfig -Scenario config -MaxFPS 30 # Direkte Werte, Uhrzeit, Fehler, Persistenz und Sperre
./Tools/TestLogbook.ps1              # Unreal-Tests: alte/neue Logs, zwei Stunden Daten, Fehlerfälle
./Tools/TestCheckpointRecovery.ps1 -PackageDirectory Builds/WindowsRecovery # Eigene Testinstanz hart beenden, Sicherungen neu öffnen
./Tools/Soak.ps1 -PackageDirectory Builds/WindowsSoak -Seconds 7200 # Echte zwei Stunden, Ablauf und Messgrenzen: Documentation/SOAK.md
./Tools/SmokeTest.ps1 -Packaged -Entry 1 -Scenario logbook # Speichern, neu laden, vier Diagrammgruppen
```

Ein laufendes Spiel kann geöffnet bleiben, wenn ein neuer Build mit `-PackageDirectory 'O:/games/dive game/Builds/WindowsLandscape'` in einen separaten Ordner geschrieben wird. Dem Smoke-Test dann denselben Parameter übergeben. Wenn die Entwicklungsdatei unter `divingsim/Binaries/Win64` selbst geöffnet ist, erzeugt `-BuildSlot Name` einen separaten Win64-Development-Programmnamen. Der Paketstarter bleibt `divingsim.exe`; die Prüfscripte lesen den tatsächlichen Programmnamen aus dem UAT-Manifest und wählen keine liegengebliebene ältere Datei.

Die Build-Skripte begrenzen C++-Parallelität für den Entwicklungsrechner. `Tools/TestCore.cmd` erwartet die lokale Installation unter `C:/BuildTools2022`; plattformunabhängig kann der Kern mit `cmake -S Tests -B artifacts/tests` gebaut und mit CTest geprüft werden. Die CI-Konfiguration prüft diesen Kern auf Windows, macOS und Linux, nicht das vollständige Unreal-Spiel. Ein macOS-Spiel-Build ist noch nicht getestet.

Die Unterwasserwelt enthält fünf unterschiedlich geformte Fischmodelle (Egli, Rotauge, Hecht, Felchengruppe, Trüsche) und drei Bewuchstypen. Tiefe, Grundabstand, Hangneigung und Sicht beeinflussen deren Platzierung. Fischgruppen, Flucht und Hindernisvermeidung sind implementiert; die Tier- und Pflanzenverteilung ist eine Spielsimulation. Details und Grenzen stehen in der Modellbeschreibung.

Blender-Quellen: `blender/DivingAssets.blend`; reproduzierbare Geometrie über `Tools/create_blender_assets.py` und `Tools/create_ecology_assets.py`, synthetischer Ton über `Tools/create_audio.py`. Blender-Skripte im Hintergrund mit `--python-exit-code 1` ausführen, damit Python-Fehler einen Fehlercode liefern. `Tools/preview_ecology_assets.py` rendert eine Übersicht nach `artifacts/EcologyAssets.png`. `Tools/build_unreal_content.py` wird durch den Content-Build in Unreal ausgeführt.

```powershell
python Tools/prepare_bathymetry.py
python Tools/package_bathymetry.py
```

Diese Geodaten-Schritte laden den vollständigen offenen Walensee-Datensatz, prüfen dessen SHA256-Prüfsumme und schreiben 46 Rasterkacheln nach `Geodata/Processed/Walensee`. Der zweite Schritt erzeugt die mit Git LFS versionierten Laufzeitdaten in `divingsim/Content/Diving/Geodata`; nach einem vollständigen LFS-Checkout ist kein erneuter Download nötig. Die ursprünglichen Höhen sind absolute LN02-Höhen in Metern, keine Wassertiefen. Das Spiel verwendet einen festen Bezugspegel von 419 m LN02, keine aktuelle Pegelmessung. Fehlende Messwerte bleiben erhalten; am Vermessungsrand endet der begehbare Bereich. Details: [Geodaten und Koordinaten](Documentation/GEODATA.md). Quelle: [swissBATHY3D](https://www.swisstopo.admin.ch/de/hoehenmodell-swissbathy3d), © swisstopo; [Nutzungsbedingungen](https://www.swisstopo.admin.ch/de/nutzungsbedingungen-kostenlose-geodaten-und-geodienste).

Anforderungen und offene Abnahme: [Documentation/PROJECT.md](Documentation/PROJECT.md). Modellbeschreibung: [Documentation/SIMULATION.md](Documentation/SIMULATION.md). Gerenderte Leistungsmessung und Ergebnisse: [Documentation/PERFORMANCE.md](Documentation/PERFORMANCE.md). Asset-Herkunft: [Documentation/ASSETS.csv](Documentation/ASSETS.csv).
