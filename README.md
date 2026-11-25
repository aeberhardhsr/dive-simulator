# Walensee Diving Simulator

Ein spielbarer Unreal-Prototyp für einen realitätsnahen Tauchsimulator. Der vermessene Walensee-Grund enthält Tarierung, Trägheit, Wasserwiderstand, tiefenabhängigen Gasverbrauch, gekoppelte Spiel-Vitalwerte, Instrumente, Fische, Pflanzen, Atmung und gespeicherte Tauchprotokolle.

**Entwicklungsstand:** 46 swisstopo-Rasterkacheln bilden den vermessenen Seegrund im Maßstab 1:1 ab. Drei Offshore-Spielstarts liegen bei Weesen, Murg und Walenstadt. Gelände wird um den Taucher nachgeladen; die Tiefenkarte zeigt Position und Spur. Umliegende Berge basieren auf swissALTI3D, die Wasseroberfläche verwendet Absorption, Streuung und Reflexion. Echte Uferzugänge, detaillierte Landschaft, fotorealistische Grafik, sämtliche Tierarten und die vollständige Anforderungsabnahme sind offen. Das Spielmodell ist kein Werkzeug zur Planung realer Tauchgänge.

## Starten

Nach einem lokalen Package-Build: `Builds/Windows/divingsim.exe` starten. Der gesamte benachbarte Ordner wird benötigt. Im Menü Ausrüstung und Umgebung einstellen, dann **Abtauchen** wählen. **Exploration** bietet unbegrenztes Gas und automatische Tarierung; **Simulation** verwendet manuelle Tarierung und begrenztes Gas. Hardcore ist derzeit als noch nicht vollständig implementiert gekennzeichnet.

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
| WASD | Schwimmen in Blickrichtung / seitwärts |
| Leertaste / Strg | Nach oben / unten schwimmen |
| Umschalt | Kräftiger Flossenschlag, höhere Belastung |
| Q / E | Jacket füllen / entleeren |
| L | Lampe |
| C | Handgelenkcomputer anheben / senken |
| R | Computerseite wechseln, wenn angehoben: Tauchdaten / Vitalwerte / Gas |
| M | Walensee-Tiefenkarte mit Position und Spur |
| P | Foto |
| H | Instrumente aus-/einblenden |
| Esc | Pause und Menü |
| F | Tauchgang an der Einstiegsboje nahe der Oberfläche abschließen |

BCD-Gas dehnt sich beim Aufstieg aus. Gegensteuern und rechtzeitig entlüften. Im Pausenmenü steht die Simulation still; die Ausrüstung lässt sich während eines laufenden Tauchgangs nicht ändern.

Mit C wird der sichtbare Handgelenkcomputer angehoben. R wechselt zwischen Tauchdaten, Vitalwerten und Gasdaten; Tiefe, Druck, Zeit und Kurs bleiben auf allen Seiten sichtbar. Er funktioniert auch bei mit H ausgeblendetem Bildschirm-HUD. Die sichtbare Handlampe verwendet denselben Schalter und Batteriestand wie das Licht im Spiel.

**Wetter und Wasser** öffnet die Umweltkonfiguration: heiter/bedeckt/Regen, Windstärke und -richtung, Temperatur an Oberfläche und in der Tiefe, Sprungschicht, Sichtweite, Strömung und Tageszeit. Wind erzeugt sichtbare Wellen und Wasserbewegung; Taucher, Bojen und Atemblasen reagieren auf dieselbe Oberfläche. Die Werte sind frei gewählte Spielszenarien und bleiben während eines Tauchgangs fest. **Ausruestung und Tauchplan** führt zu den übrigen Einstellungen zurück.

**Logbuch ansehen** im Start-/Pausenmenü oder **Logbuch / Verläufe** nach dem Tauchgang öffnet gespeicherte Tauchgänge. Links einen Eintrag wählen, mit den Pfeilen blättern oder nach Modus filtern. Vier Diagrammgruppen zeigen Tiefe, Gas, Nullzeit, Dekotiefe, Vitalwerte, Temperatur, Stress, Ermüdung und Strecke. Die Maus über den Kurven wählt den gemeinsamen Zeitpunkt; **< ! / ! >** springt zu Warnereignissen. Esc führt zurück. Beim Lesen aus dem Pausenmenü bleibt der laufende Tauchgang angehalten.

Tauchprotokolle liegen unter `divingsim/Saved/DiveLogs`, Fotos mit Taste P unter `divingsim/Saved/Photographs`. Testaufnahmen liegen unter `divingsim/Saved/Screenshots`. In einem verpackten Development-Build befinden sich diese Verzeichnisse unter dessen eigenem `divingsim`-Ordner. Das Logbuch liest auch ältere Protokolle; automatisierte Testläufe werden in der normalen Ansicht ausgeblendet. Fehlende oder beschädigte Dateien verhindern das Lesen anderer Einträge nicht.

## Entwicklung und Prüfung

```powershell
./Tools/TestCore.cmd                  # Engineunabhängige Simulationsprüfungen
python -m unittest discover -s Tests -p 'test_*.py'
./Tools/Build.ps1 -Target Content     # Materialien, Importe und Trainingsmap
./Tools/Build.ps1 -Target Content -OnlyEnvironment # Nur Wasser, Wolken und Regen erneuern
./Tools/SmokeTest.ps1                 # 22 Sekunden automatisiertes Gameplay
./Tools/Build.ps1 -Target Package     # Eigenständiger Windows-Development-Build
./Tools/SmokeTest.ps1 -Packaged       # Derselbe Ablauf in der verpackten Anwendung
./Tools/SmokeTest.ps1 -Packaged -Entry 1 -Scenario stream  # 90 s, Kachelwechsel
./Tools/SmokeTest.ps1 -Packaged -Scenario surface         # 38 s, Auftauchen
./Tools/SmokeTest.ps1 -Packaged -Scenario surface-calm    # Windstille
./Tools/SmokeTest.ps1 -Packaged -Scenario surface-rain    # Regen und 12 m/s Wind
./Tools/SmokeTest.ps1 -Packaged -Entry 1 -Scenario wrist  # Drei Handgelenkseiten, HUD aus
./Tools/SmokeTest.ps1 -Packaged -Entry 1 -Scenario wrist-night # Display bei Nacht
./Tools/TestLogbook.ps1              # Unreal-Tests: alte/neue Logs, zwei Stunden Daten, Fehlerfälle
./Tools/SmokeTest.ps1 -Packaged -Entry 1 -Scenario logbook # Speichern, neu laden, vier Diagrammgruppen
```

Ein laufendes Spiel kann geöffnet bleiben, wenn ein neuer Build mit `-PackageDirectory 'O:/games/dive game/Builds/WindowsLandscape'` in einen separaten Ordner geschrieben wird. Dem Smoke-Test dann denselben Parameter übergeben.

Die Build-Skripte begrenzen C++-Parallelität für den Entwicklungsrechner. `Tools/TestCore.cmd` erwartet die lokale Installation unter `C:/BuildTools2022`; plattformunabhängig kann der Kern mit `cmake -S Tests -B artifacts/tests` gebaut und mit CTest geprüft werden. Die CI-Konfiguration prüft diesen Kern auf Windows, macOS und Linux, nicht das vollständige Unreal-Spiel. Ein macOS-Spiel-Build ist noch nicht getestet.

Die Unterwasserwelt enthält fünf unterschiedlich geformte Fischmodelle (Egli, Rotauge, Hecht, Felchengruppe, Trüsche) und drei Bewuchstypen. Tiefe, Grundabstand, Hangneigung und Sicht beeinflussen deren Platzierung. Fischgruppen, Flucht und Hindernisvermeidung sind implementiert; die Tier- und Pflanzenverteilung ist eine Spielsimulation. Details und Grenzen stehen in der Modellbeschreibung.

Blender-Quellen: `blender/DivingAssets.blend`; reproduzierbare Geometrie über `Tools/create_blender_assets.py` und `Tools/create_ecology_assets.py`, synthetischer Ton über `Tools/create_audio.py`. Blender-Skripte im Hintergrund mit `--python-exit-code 1` ausführen, damit Python-Fehler einen Fehlercode liefern. `Tools/preview_ecology_assets.py` rendert eine Übersicht nach `artifacts/EcologyAssets.png`. `Tools/build_unreal_content.py` wird durch den Content-Build in Unreal ausgeführt.

```powershell
python Tools/prepare_bathymetry.py
python Tools/package_bathymetry.py
```

Diese Geodaten-Schritte laden den vollständigen offenen Walensee-Datensatz, prüfen dessen SHA256-Prüfsumme und schreiben 46 Rasterkacheln nach `Geodata/Processed/Walensee`. Der zweite Schritt erzeugt die mit Git LFS versionierten Laufzeitdaten in `divingsim/Content/Diving/Geodata`; nach einem vollständigen LFS-Checkout ist kein erneuter Download nötig. Die ursprünglichen Höhen sind absolute LN02-Höhen in Metern, keine Wassertiefen. Das Spiel verwendet einen festen Bezugspegel von 419 m LN02, keine aktuelle Pegelmessung. Fehlende Messwerte bleiben erhalten; am Vermessungsrand endet der begehbare Bereich. Details: [Geodaten und Koordinaten](Documentation/GEODATA.md). Quelle: [swissBATHY3D](https://www.swisstopo.admin.ch/de/hoehenmodell-swissbathy3d), © swisstopo; [Nutzungsbedingungen](https://www.swisstopo.admin.ch/de/nutzungsbedingungen-kostenlose-geodaten-und-geodienste).

Anforderungen und offene Abnahme: [Documentation/PROJECT.md](Documentation/PROJECT.md). Modellbeschreibung: [Documentation/SIMULATION.md](Documentation/SIMULATION.md). Asset-Herkunft: [Documentation/ASSETS.csv](Documentation/ASSETS.csv).
