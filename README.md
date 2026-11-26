# Walensee Diving Simulator

Ein spielbarer Unreal-Prototyp für einen realitätsnahen Tauchsimulator. Der vermessene Walensee-Grund enthält Tarierung, Trägheit, Wasserwiderstand, tiefenabhängigen Gasverbrauch, gekoppelte Spiel-Vitalwerte, Instrumente, Fische, Pflanzen, Atmung und gespeicherte Tauchprotokolle. Die umgebende Landschaft verwendet 77 Nanite-Geländekacheln aus dem 8-m-Höhenraster.

**Entwicklungsstand:** 46 swisstopo-Rasterkacheln bilden den vermessenen Seegrund im Maßstab 1:1 ab. Drei Uferstarts bei Weesen Strandbad, Murg West und Mols/Lediwracks ermöglichen Gehen, Waten, Schwimmen und die Rückkehr an Land. Die Zugangsplattformen und Treppen sind modellierte Spielgeometrie auf örtlichen Höhendaten. Die bisherigen Offshore-Starts bleiben wählbar. Gelände wird um den Taucher nachgeladen; die Tiefenkarte zeigt Position und Spur. Umliegende Berge basieren auf swissALTI3D; Berge und Ufer tragen registrierte SWISSIMAGE-Luftbilder des Kacheljahrs 2025. 16.249 Gebäude aus swissBUILDINGS3D ergänzen die Landschaft mit vermessenen Außenflächen und Kollision. Räumliche Bäume folgen nun Waldflächen und Einzelbaumspitzen aus swissTLM3D; ihre Artenmischung und Walddichte sind gestaltet. Weitere Bodendetails, saisonale Vegetation, Fassadendetails, originalgetreue Zugangsanlagen, fotorealistische Grafik und die vollständige Anforderungsabnahme bleiben offen. Das Spielmodell ist kein Werkzeug zur Planung realer Tauchgänge.

## Starten

Die neue **Grafikvorschau** liegt unter **`Builds/WindowsUnderwaterDetail/divingsim.exe`**. Sie enthält detailliertere Felsen, räumliche Kiesfelder und überarbeitete Wasserpflanzen. Zwölf Paketprüfungen mit 74 Aufnahmen und der normale Spielstart bestehen. Ein einzelner 60-Sekunden-Unterwasserlauf erreichte auf der RTX 3090 bei 1080p/Qualität Hoch durchschnittlich 78,22 FPS (1%-Low 67,22 FPS); ein Dauerlauf dieses neuen Pakets steht aus. Bilder, Nachweise und Grenzen: [Unterwasserbewuchs](Documentation/AQUATIC_PLANTS.md) und [Unterwasserwelt](Documentation/UNDERWATER_SCENE.md). Die vom Nutzer vorgegebene [visuelle Zielqualität](Documentation/VISUAL_TARGET.md) ist damit noch nicht erreicht. Die vorherige Felsvorschau `Builds/WindowsRockDetail` bleibt erhalten.

Die vorherige Basis unter **`Builds/WindowsFishSteering/divingsim.exe`** bleibt erhalten und hat einen [zweistündigen Murg-Stabilitätslauf](Documentation/SOAK.md) bestanden. Ihre Fischbewegung ist auch in der neuen Grafikvorschau enthalten: Fische wenden in begrenzten Kurven, prüfen Hindernisse voraus und bremsen bei blockierten Wegen. Richtung und Tempo bleiben beim Pausieren und Nachladen erhalten. Umsetzung, Paketprüfungen und Grenzen: [Fischbewegung](Documentation/FISH_STEERING.md). Die Bewegungsparameter sind gestaltet; artspezifische Validierung und vollständige Körperkontakte bleiben offen.

Der vorherige Stand unter **`Builds/WindowsFinMotion/divingsim.exe`** bleibt erhalten. Seine eigenständige Brust-/Bauchflossenbewegung und kleineren Rücken-/Afterflossenausschläge sind im aktuellen Paket enthalten. Tempo und Kurven beeinflussen die Animation. Bilder und Prüfgrenzen: [Flossenbewegung](Documentation/FIN_MOTION.md). Kiemen-/Maulbewegung und die fotorealistische Qualitätsabnahme bleiben offen.

Der vorherige Stand unter **`Builds/WindowsFish/divingsim.exe`** bleibt erhalten. Seine überarbeiteten Modelle für Barsch, Rotauge, Hecht, Felchengruppe und Trüsche mit Augen, Schuppen-/Hautdetails und vier Detailstufen sind im aktuellen Paket enthalten. Bilder und Erstellung: [Fischmodelle](Documentation/SIMULATION.md#fish-geometry-and-surface-detail).

Der vorherige Stand unter **`Builds/WindowsDecompression/divingsim.exe`** bleibt erhalten. Seine Dekompressionsprognose ist auch im aktuellen Paket enthalten: Der Tauchcomputer berechnet den nächsten Stopp, dessen Wartezeit und die voraussichtliche Aufstiegsdauer bei 9 m/min. **C** hebt den Computer an, **R** wechselt durch vier Seiten. HUD und Computer unterscheiden berechnete Dekompressionsstopps vom optionalen Sicherheitsstopp; die Prognose pausiert mit dem Tauchgang. Modell, Bilder und Grenzen: [Dekompressionsprognose](Documentation/SIMULATION.md#ascent-forecast). Die Prognose ist ein Spielmodell; Gasreichweite für den Aufstieg und Wiederholungstauchgänge sind noch nicht berücksichtigt.

Der vorherige Stand unter **`Builds/WindowsGround/divingsim.exe`** bleibt erhalten. Gras- und Waldboden besitzen Nahdetails in Farbe, Normalen und Rauheit; die Ferndarstellung bleibt geografisch registriert. Quellen, Vergleichsbilder und Grenzen: [Bodenoberflächen](Documentation/BED_SURFACES.md#nahdetails-über-wasser). Der Vegetationsstand unter **`Builds/WindowsVegetation/divingsim.exe`** bleibt erhalten. Nahe Waldzellen werden geladen, Stämme besitzen vereinfachte Kollision und Blätter bewegen sich mit dem Szenariowind. Vergleichsbilder, Datenquellen und Grenzen: [Räumliche Vegetation](Documentation/VEGETATION.md). Der vorherige Küsten-Build bleibt separat erhalten.

Die bisher fehlenden Uferbereiche sind jetzt als modellierte Spielflächen integriert. An allen drei Wasserstarts lässt sich die frühere Vermessungsgrenze überqueren und wieder zurückschwimmen; die offene Bodenfläche in der Walenstadt-Fotoansicht ist geschlossen. 496 neue Geländemeshes verwenden geprüfte Kollisionen. Tiefenkarte und Wegpunktprüfung bleiben an vermessene Daten gebunden. Umsetzung, Bild und Tests: [Spielbare Uferergänzung](Documentation/COAST.md); Datenherkunft und Grenzen: [Ufermodell](Documentation/COAST_MODEL.md).

Der vorherige Windows-Build liegt lokal unter **`Builds/WindowsCoast/divingsim.exe`**. Mit **P** aufgenommene Unterwasserfotos schalten jetzt Steckbriefe frei und bleiben dem Tauchgang zugeordnet. **Arten und Fotos** im Menü öffnet die Sammlung; das Logbuch zeigt die Aufnahmen des gewählten Tauchgangs. Beschreibung, Quellen und Grenzen: [DISCOVERIES.md](Documentation/DISCOVERIES.md). Standardmäßig mit **V** lassen sich bis zu 32 eigene Wegpunkte auf der Karte oder an der aktuellen Position speichern. Ein gewähltes Ziel erscheint mit Entfernung und Peilung auf Karte und HUD; die Bearbeitung pausiert den Tauchgang. Namen und Zielwahl bleiben nach einem Neustart erhalten. Bedienung und Grenzen: [WAYPOINTS.md](Documentation/WAYPOINTS.md). Sonnen-Caustics wirken jetzt als wellenabhängige Beleuchtung auf undurchsichtige Unterwasserobjekte. Schatten, Windstille, Nacht und Tiefe werden berücksichtigt; die unabhängigen Muster in den Bodenfarben entfallen. Vergleichsbilder und Modellgrenzen: [CAUSTICS.md](Documentation/CAUSTICS.md). Feines Ufer und grobe Hintergrundlandschaft besitzen jetzt einen gemeinsamen Rand mit einem begrenzten Übergangsbereich für Geometrie, Normalen und Luftbilder. Vergleich und Grenzen: [SHORE_TRANSITION.md](Documentation/SHORE_TRANSITION.md). Die Landschaft enthält nun 16.249 vermessene Gebäude in 715 Nanite-Zellen mit eigener Kollision; Dächer verwenden registrierte Luftbilder, Fassaden vorerst illustrative Farben. Datenherkunft und Grenzen: [BUILDINGS.md](Documentation/BUILDINGS.md). Bei blockierter Körperdrehung lässt sich der Kopf nun seitlich bis 65° weiterdrehen; Maske und Ausrüstung bleiben kollisionsgeprüft. Details: [HEAD_TURN.md](Documentation/HEAD_TURN.md). Felsen und Bewuchs bleiben beim Weitertauchen an festen Positionen; nahe Fische behalten ihre Bewegung. Die frühere Neuerzeugung der gesamten Umgebung nach 55 Metern ist entfernt. Details: [POPULATION_STREAMING.md](Documentation/POPULATION_STREAMING.md). Der Einstieg lässt sich jetzt direkt auf der Walenseekarte auswählen. Beim Tauchen zeigt die Karte den tatsächlichen Startpunkt sowie Richtung und Entfernung dorthin; Details: [NAVIGATION.md](Documentation/NAVIGATION.md). Flossenschläge nahe am weichen Grund wirbeln jetzt lokale Sedimentwolken auf; sie treiben, verdünnen sich und setzen sich wieder ab. Umsetzung und Grenzen: [SEDIMENT.md](Documentation/SEDIMENT.md). Körper und Ausrüstung verwenden jetzt 24 gemeinsame Kollisionsformen: Flasche, Hände und Flossen werden bei Bewegung und Drehung gegen die Umgebung geprüft. Details und Grenzen: [CONTACTS.md](Documentation/CONTACTS.md). Kopf und Atemregler folgen jetzt der Blickrichtung; der Taucher behält beim Ablesen des Computers seine Schwimmlage. Mundposition an der Wasserlinie und Flaschensitz sind korrigiert; Details: [BODY_POSE.md](Documentation/BODY_POSE.md). Unter **3D ansehen** auf der Ausrüstungsseite lässt sich der Taucher drehen, vergrößern und mit Flossenschlag ansehen. Gelenkgewichteter Körper, zwei Anzüge, sechs Flaschen und drei BCD-Modelle folgen dem gewählten Plan; Umsetzung und offene Qualitätsgrenzen: [AVATAR.md](Documentation/AVATAR.md). Unter **Ausruestung: Gewicht und Auftrieb** lassen sich Stahl-/Aluminiumflasche und drei BCDs wählen. Einzelmassen, Gas und Verdrängung bestimmen Tarierung und Trägheit; Details und Grenzen: [LOADOUT.md](Documentation/LOADOUT.md). Exploration, Simulation und Hardcore verwenden unterschiedliche Regeln für Gas, Tarierung, körperliche Folgen, HUD und Hilfetexte. Hardcore enthält einen kälteabhängigen Reglerdefekt mit Gasverlust; Details und Modellgrenzen stehen in [MODES.md](Documentation/MODES.md). Automatische saisonale Wassertemperaturen, Sichtweite und Strömung bleiben unter **Wetter und Wasser** wählbar; Herkunft: [AUTOMATIC_ENVIRONMENT.md](Documentation/AUTOMATIC_ENVIRONMENT.md). Kalender/Himmel, drei Lampen, GPU-Schwebeteilchen und transparente Atemblasen sind enthalten. Der gesamte Paketordner wird benötigt; vorherige Builds einschließlich `Builds/WindowsDiscoveries`, `Builds/WindowsWaypoints`, `Builds/WindowsCaustics`, `Builds/WindowsShoreSeams`, `Builds/WindowsBuildings`, `Builds/WindowsHeadTurn`, `Builds/WindowsPopulation`, `Builds/WindowsNavigation`, `Builds/WindowsSediment`, `Builds/WindowsContacts`, `Builds/WindowsBody`, `Builds/WindowsAvatar`, `Builds/WindowsLoadout`, `Builds/WindowsModes` und `Builds/WindowsEnvironment` bleiben erhalten.

Nach einem lokalen Package-Build ohne eigenes Zielverzeichnis: `Builds/Windows/divingsim.exe` starten. Im Menü das Einstiegsfeld öffnen, einen Punkt oder eine Karteikarte auf der Walenseekarte wählen und mit **Auswahl übernehmen** speichern. **Zurück / Esc** verwirft die Vorschau. Ausrüstung, Umgebung und **Start: am Ufer / direkt im Wasser** einstellen, dann starten. Neue Einstellungen beginnen am Ufer; ältere gespeicherte Pläne behalten ihren Wasserstart. **Exploration** bietet unbegrenztes Gas, wählbare automatische/manuelle Tarierung und keine körperlichen Beeinträchtigungen. **Simulation** verwendet manuelle Tarierung, begrenztes Gas und gekoppelte Beeinträchtigungen. **Hardcore** ergänzt Reglervereisung und deaktiviert automatische Hilfetexte. Die Modelle bleiben gestaltete Spielmechaniken; die vollständige Anforderungsabnahme ist offen.

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
| V | Wegpunkte bearbeiten; pausiert den Tauchgang |
| P | Unterwasserfoto / Arten entdecken |
| H | Instrumente aus-/einblenden; in Exploration immer sichtbar |
| Esc | Pause und Menü |
| F1 | Steuerung und Hilfe öffnen |
| F | Nach dem Tauchen auf dem Startpodest abschließen; beim Wasserstart nahe der Einstiegsboje |

Die Karte mit **M** zeigt Position, Blickrichtung, Spur und den tatsächlichen Startpunkt. Richtung und Entfernung zum Start sind eine horizontale Luftlinie im LV95-Gitter, keine Routenplanung. Beim Kartenlesen läuft der Tauchgang weiter; Tiefe und Gas bleiben in der Kartenansicht sichtbar. Im Pausenmenü bleibt die Einstiegsauswahl während eines Tauchgangs gesperrt.

BCD-Gas dehnt sich beim Aufstieg aus. Gegensteuern und rechtzeitig entlüften. Im Pausenmenü steht die Simulation still; die Ausrüstung lässt sich während eines laufenden Tauchgangs nicht ändern.

Beim Uferstart führen Podest und Treppe ins Wasser. Du beginnst mit abgesetztem Atemregler und atmest Umgebungsluft ohne Flaschenverbrauch. Vor dem Eintauchen mit **T** den Regler einsetzen; nach der Rückkehr kannst du ihn wieder absetzen. Ohne Regler unter Wasser fehlt Atemluft: Die Warnung erscheint sofort; zunächst wird eine begrenzte Atemreserve verbraucht. Erst bei niedriger Reserve sinkt die simulierte SpO2. Belastung beschleunigt den Verbrauch, Atmen füllt die Reserve allmählich wieder auf. Die Prozentanzeige ist ein Spielwert, keine sichere Atemanhaltezeit. Das Jacket verwendet auch beim Atmen von Umgebungsluft Flaschengas. Wasserstarts beginnen mit eingesetztem Regler.

Der Wechsel zwischen Gehen/Waten und Schwimmen folgt Bodenkontakt und Wassertiefe; die Kamera bleibt auf Augenhöhe. Nach dem Tauchen zur selben Treppe und auf das flache Podest zurückkehren. Abkühlung berücksichtigt Lufttemperatur, Wind und eingetauchten Körperanteil. Dauer und Strecke enthalten den Landweg. Körperhaltung, Flossen an Land und Wärmehaushalt bleiben vereinfacht.

Die Tabelle zeigt die Standardbelegung. Unter **F1 / Steuerung und Hilfe** lassen sich alle 18 Spielaktionen auf Tastaturtasten oder rechte/mittlere/seitliche Maustasten legen. Eine belegte Taste tauscht die beiden Aktionen; Escape bricht die Auswahl ab. Escape, F1, F11, die Konsole, Alt/Windows und der linke Mausklick bleiben reserviert. Hinweise im Spiel zeigen die aktuelle Belegung. Mausgeschwindigkeit, Y-Invertierung und Sichtfeld sind ebenfalls einstellbar und werden gespeichert. **Standards wiederherstellen** setzt diese Steuerungsoptionen zurück.

**Steuerung kennenlernen** im Startmenü beginnt eine geführte Einführung mit acht Aufgaben. Fortschritt entsteht durch Umsehen, eigenes Schwimmen, Füllen/Entleeren des Jackets, die Vitalwerteseite des Computers, Lampe, Karte und Rückkehr zur Boje. Sie verwendet ein ruhiges Weesen-Szenario mit manueller Tarierung und begrenztem Gas; der vorherige Tauchplan wird nach Abschluss oder Abbruch wiederhergestellt. Der Abschluss wird gespeichert und die Einführung kann wiederholt werden. Sie erklärt die Spielbedienung und ist keine Tauchausbildung.

**Tauchprofile** unten im Menü speichert benannte Zusammenstellungen aus Ausrüstung, Einstieg, Umwelt und Tauchplan. Einen Namen eingeben und mit Enter oder **Speichern** bestätigen; Escape bricht den Dialog ab. Profile lassen sich laden, umbenennen, mit dem aktuellen Plan aktualisieren und archivieren. **Vorheriger Stand** stellt die letzte Änderung an Name oder Konfiguration wieder her, auch nach einem Neustart. Das Archiv erlaubt die Wiederherstellung; es gibt keine endgültige Löschfunktion. Ein laufender Tauchgang verhindert das Laden anderer Profile.

Bis zu 32 aktive und 128 Profile insgesamt liegen lokal in `divingsim/Saved/DiveProfiles.json`. Namen müssen eindeutig sein, auch gegenüber dem Archiv. Die Datei wird beim Speichern geprüft und über eine temporäre Datei ersetzt; unlesbare Dateien bleiben unangetastet. **Neu laden** liest Änderungen anderer Spielinstanzen erneut ein. Profile enthalten den Tauchplan; Sprache, Audio und Steuerung bleiben gemeinsame persönliche Einstellungen.

Mit C wird der sichtbare Handgelenkcomputer angehoben. R wechselt zwischen Tauchdaten, Vitalwerten, Gasdaten und Dekompressionsprognose. Tiefe, Druck und Zeit bleiben auf allen vier Seiten sichtbar, der Kurs auf den ersten drei. Die vierte Seite zeigt nächsten Stopp, Wartezeit, Aufstiegsprognose bei 9 m/min und aktuelle Dekompressionsgrenze. Er funktioniert auch bei mit H ausgeblendetem Bildschirm-HUD. Die sichtbare Handlampe verwendet denselben Schalter und Batteriestand wie das Licht im Spiel.

**Wetter und Wasser** öffnet die Umweltkonfiguration: heiter/bedeckt/Regen/Sturm, Windstärke und -richtung, Lufttemperatur (−10 bis 40 °C), Wassertemperatur an Oberfläche und in der Tiefe, Sprungschicht, Sichtweite, Strömung, Startdatum und Tageszeit. Die Wetterwahl setzt zunächst 3/5/8/22 m/s Wind; danach ist der Wert von 0 bis 24 m/s frei einstellbar. Wind erzeugt sichtbare Wellen und Wasserbewegung; Taucher, Bojen und Atemblasen reagieren auf dieselbe Oberfläche. Es handelt sich um offline konfigurierte Spielszenarien.

**Wasser: Auto** verwendet saisonale Eawag-Tiefenprofile. **Sicht: Auto** und **Strömung: Auto** berechnen gestaltete Szenariowerte aus Datum, Wetter und Wind. Für Strömung sind außerdem keine, leicht, stark und manuell wählbar. Eigene Werte bleiben beim Umschalten erhalten; Profile speichern die Auswahl. Die Lufttemperatur bleibt manuell. [Modell und Quellen](Documentation/AUTOMATIC_ENVIRONMENT.md).

Datum (2000–2099) und Uhrzeit legen den Start in Schweizer Ortszeit fest. Während des Tauchgangs laufen Datum und Uhrzeit mit der Simulation weiter, einschließlich Mitternacht und Sommerzeitwechsel; Pause hält die astronomische Zeit an. Die im Frühjahr übersprungene Stunde wird bei der Eingabe abgewiesen, die doppelte Herbststunde bezeichnet die erste Ausführung in Sommerzeit. Unreals Celestial Vault berechnet Sonne, Mondphase und Sternenhimmel für den Walensee. Die Belichtung erhält die Dämmerung und passt sich nachts allmählich an; die eingeschaltete Lampe begrenzt die Dunkeladaption. Die automatische Wassertemperatur verwendet das gewählte Startdatum; Wetterwahl, Vegetation und sommerliche Luftbilder werden nicht automatisch saisonal umgestellt. Details und Prüfgrenzen: [Kalender und Himmel](Documentation/CELESTIAL.md). **Ausruestung und Tauchplan** führt zu den übrigen Einstellungen zurück.

Zahlenfelder öffnen einen Eingabedialog: beispielsweise **213,5 bar**, **7,5 kg**, **22,5 m** oder **47 min**. Die Einheit gehört nicht in das Eingabefeld. Dezimalpunkt und Komma sind erlaubt; der Dialog nennt den gültigen Bereich. Die Startuhrzeit lässt sich minutengenau von **00:00 bis 23:59** einstellen. Enter oder **Speichern** übernimmt den Wert, Escape oder **Abbrechen** verwirft die Eingabe. Ungültige Werte verändern den Tauchplan nicht. Während eines laufenden Tauchgangs bleibt die Konfiguration gesperrt.

**Audio >** unten im Menü öffnet fünf Lautstärkeregler: Gesamt, Umgebung, Atem/Ausrüstung, Herzschlag und Warnungen. Mit **− / +** in 5-%-Schritten einstellen; 0 % schaltet den Kanal stumm. Einstellungen werden gespeichert und können auch in der Pause geändert werden. Atmung und Herzschlag folgen den simulierten Vitalwerten, Jacket-Geräusche dem tatsächlichen Gasfluss. Blasen steigen als räumliche Klangquelle auf; Wind, Wasser und Regen werden beim Eintauchen gedämpft. Die eigenen synthetischen Klänge sind ein Entwicklungsstand, keine Aufnahme echter Tauchausrüstung.

**Logbuch ansehen** im Start-/Pausenmenü oder **Logbuch / Verläufe** nach dem Tauchgang öffnet gespeicherte Tauchgänge. Links einen Eintrag wählen, mit den Pfeilen blättern oder nach Modus filtern. Vier Diagrammgruppen zeigen Tiefe, Gas, Nullzeit, Dekotiefe, Vitalwerte, Temperatur, Stress, Ermüdung und Strecke. Die Maus über den Kurven wählt den gemeinsamen Zeitpunkt; **< ! / ! >** springt zu Warnereignissen. Neue Protokolle zeigen am ausgewählten Zeitpunkt außerdem Atemquelle, Körperimmersion, gemischte Umgebungstemperatur und die Atemreserve. Esc führt zurück. Beim Lesen aus dem Pausenmenü bleibt der laufende Tauchgang angehalten.

Tauchprotokolle liegen unter `divingsim/Saved/DiveLogs`, Fotos mit Taste P unter `divingsim/Saved/Photographs`. Testfotos liegen separat unter `divingsim/Saved/PhotographsTest`, weitere Prüfbilder unter `divingsim/Saved/Screenshots`. In einem verpackten Development-Build befinden sich diese Verzeichnisse unter dessen eigenem `divingsim`-Ordner. Das Logbuch liest auch ältere Protokolle; automatisierte Testläufe werden in der normalen Ansicht ausgeblendet. Fehlende oder beschädigte Dateien verhindern das Lesen anderer Einträge nicht.

Laufende Tauchgänge werden alle 30 Sekunden Simulationszeit zwischengespeichert, beim regulären Verlassen als unterbrochen und beim Abschluss mit dem Ergebnis. Eine `.json.bak` bewahrt die vorherige gültige Version. Ist die Hauptdatei beschädigt oder fehlt sie, zeigt das Logbuch diesen älteren Stand mit einem Hinweis an. Zwischenstände sind Protokolle; sie erlauben noch kein Fortsetzen des Tauchgangs. Vor der ersten Sicherung und seit der letzten erfolgreichen Sicherung können bei einem Absturz Messpunkte verloren gehen. Schreibfehler während des Tauchgangs werden eingeblendet.

Die laufende Sicherung verarbeitet eine eigene Datenkopie im Hintergrund. Es gibt höchstens einen Schreibauftrag und einen wartenden aktuellen Stand; bei langsamem Datenträger wächst keine unbegrenzte Warteschlange. Abschluss und reguläres Verlassen warten auf die endgültige Speicherung, damit ein älterer Auftrag das Ergebnis nicht später überschreibt.

## Entwicklung und Prüfung

```powershell
./Tools/TestCore.cmd                  # Engineunabhängige Simulationsprüfungen
./Tools/TestBuildings.ps1 -PackageDirectory Builds/WindowsShoreSeams -ShoreSeams # Geländeanschluss, Gebäude, Fallback und Tauchregressionen
./Tools/TestNavigation.ps1 -PackageDirectory Builds/WindowsNavigation # Auswahl, Auflösungen und Spielregressionen
./Tools/TestPopulation.ps1 -PackageDirectory Builds/WindowsPopulation # Zellwechsel, Fischzustände und Spielregressionen
./Tools/TestHeadTurn.ps1 -PackageDirectory Builds/WindowsHeadTurn # Kopfdrehung, Ausrüstung, Ufer und Atmung
./Tools/TestSediment.ps1 -PackageDirectory Builds/WindowsSediment # Sediment, drei Uferwege, Atmung und Streaming
./Tools/TestAvatar.ps1 -PackageDirectory Builds/WindowsAvatar # Sechs 3D-Ausrüstungen und 48 Ansichten
python Tools/ReportAvatar.py artifacts/avatar-validation-YYYYMMDD-HHMMSS --output artifacts/avatar-report.json
./Tools/TestLoadout.ps1 -PackageDirectory Builds/WindowsLoadout # Vier Ausrüstungen, DE/EN, Speicherung, Trägheit
python Tools/ReportLoadout.py artifacts/loadout-validation-YYYYMMDD-HHMMSS # Den tatsächlichen Ergebnisordner angeben
python -m pip install -r Tools/requirements-buildings.txt
python -m unittest discover -s Tests -p 'test_*.py'
./Tools/Build.ps1 -Target Content     # Materialien, Importe und Trainingsmap
./Tools/Build.ps1 -Target Content -OnlyAvatar # Nur Körper und Ausrüstungsmodelle importieren
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
./Tools/Build.ps1 -Target Package -BuildSlot Celestial -PackageDirectory Builds/WindowsCelestial # Separater Programmname bei geöffnetem Projekt-Spiel
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
./Tools/SmokeTest.ps1 -Packaged -PackageDirectory Builds/WindowsCelestial -Scenario daycycle -Date 2026-03-29 -Hour (1+59/60) -MaxFPS 30 # 90 s Sommerzeitwechsel plus echte Pause
./Tools/SmokeTest.ps1 -Packaged -PackageDirectory Builds/WindowsCelestial -Scenario surface-calm -CelestialReferences -MaxFPS 30 # Sonne/Mond gegen JPL-Referenzen
./Tools/SmokeTest.ps1 -Packaged -PackageDirectory Builds/WindowsTwilightExposure -Scenario surface-calm -Entry 1 -Hour 18.1 -LampOff -MaxFPS 30 # Abendlicht über und unter Wasser
./Tools/SmokeTest.ps1 -Packaged -Scenario audio           # Audiomix mit Pause/Stummschaltung aufzeichnen
./Tools/TestControls.ps1              # Unreal-Prüfungen: Belegung, Konflikte, Persistenz, ungültige Werte
./Tools/TestProfiles.ps1              # Unreal-Prüfungen: Profile, Wiederherstellung, Datei-/Konfliktschutz
./Tools/SmokeTest.ps1 -Packaged -Scenario controls        # Engine-Tasteneingaben, Remapping, Pause, DE/EN
./Tools/SmokeTest.ps1 -Packaged -Scenario tutorial        # Alle acht Aufgaben, Abschluss, Planwiederherstellung
./Tools/SmokeTest.ps1 -Packaged -Scenario profiles        # Profilmenü, Namensdialog, Laden/Archiv/Undo, Tauchgang
./Tools/SmokeTest.ps1 -Packaged -Scenario shore-preview -Entry 0 # Uferansicht/Kollision; Entry 0, 1 oder 2
./Tools/SmokeTest.ps1 -Packaged -Scenario shore -Entry 0 # Gehen, Tauchen, Rueckkehr und Abschluss
python Tools/validate_audio_mix.py Builds/Windows/divingsim/Saved/AudioQA/DiveMix.wav artifacts/DiveSmoke.log
./Tools/SmokeTest.ps1 -Packaged -Entry 1 -Scenario wrist  # Vier Handgelenkseiten, HUD aus
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

Diese Geodaten-Schritte laden den vollständigen offenen Walensee-Datensatz, prüfen dessen SHA256-Prüfsumme und schreiben 46 Rasterkacheln nach `Geodata/Processed/Walensee`. Der zweite Schritt erzeugt die mit Git LFS versionierten Laufzeitdaten in `divingsim/Content/Diving/Geodata`; nach einem vollständigen LFS-Checkout ist kein erneuter Download nötig. Die ursprünglichen Höhen sind absolute LN02-Höhen in Metern, keine Wassertiefen. Das Spiel verwendet einen festen Bezugspegel von 419 m LN02, keine aktuelle Pegelmessung. Fehlende Messwerte bleiben als solche erhalten; gekennzeichnete modellierte Uferflächen ergänzen inzwischen den spielbaren Bereich jenseits der Vermessungsgrenze ([Ufermodell](Documentation/COAST_MODEL.md)). Details: [Geodaten und Koordinaten](Documentation/GEODATA.md). Quelle: [swissBATHY3D](https://www.swisstopo.admin.ch/de/hoehenmodell-swissbathy3d), © swisstopo; [Nutzungsbedingungen](https://www.swisstopo.admin.ch/de/nutzungsbedingungen-kostenlose-geodaten-und-geodienste).

Anforderungen und offene Abnahme: [Documentation/PROJECT.md](Documentation/PROJECT.md). Modellbeschreibung: [Documentation/SIMULATION.md](Documentation/SIMULATION.md). Gerenderte Leistungsmessung und Ergebnisse: [Documentation/PERFORMANCE.md](Documentation/PERFORMANCE.md). Asset-Herkunft: [Documentation/ASSETS.csv](Documentation/ASSETS.csv).
