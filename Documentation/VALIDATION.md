# Prüfung des spielbaren Walensee-Inkrements

Stand: 25. November 2026. Windows 11, Unreal Engine 5.8.4, MSVC 14.44, Windows SDK 10.0.22621. Lokaler Entwicklungsrechner; keine allgemeine Hardwarefreigabe.

| Prüfung | Ergebnis |
|---|---|
| Engineunabhängiger C++-Kern | Bestanden: 0 Fehler bei Druck, Gas, BCD, Kopplungen, Geweben, Stopptimer, Sanitierung, Rasterabfragen über Kachelgrenzen sowie Fisch-/Pflanzen-Habitatgrenzen |
| Python-Bathymetrieparser | 3 Tests bestanden: Reihenfolge/NoData, unvollständiges Raster, falsche Zeilenbreite |
| Unreal-Editor-Modul | Development-Build erfolgreich |
| Asset-/Map-Erzeugung | Commandlet erfolgreich, letzte Content-Erzeugung ohne Warnungen oder Fehler |
| Eigenständiger Windows-Build | BuildCookRun erfolgreich; Spiel startet ohne Editor |
| Automatisches Gameplay im Package, drei Startorte | Jeweils DIVE_SMOKE_PASS: 22 s und 23 gespeicherte Samples; Weesen 5,67 m maximale Tiefe / 18,77 m Strecke, Murg 11,45 m / 23,10 m, Walenstadt 9,64 m / 21,85 m |
| Geländestreaming im aktuellen Biologie-Package | DIVE_SMOKE_PASS: 90 s Exploration bei Murg, 110,92 m horizontale Entfernung vom Start, maximal neun Kacheln über den gesamten Lauf, 91 gespeicherte Samples; Habitat-/Felsabstandsprüfung besteht auch beim Neuaufbau der Populationen |
| Auftauchen im aktuellen Landschafts-Package | Alle drei Orte DIVE_SMOKE_PASS: jeweils 38 s, Kamera oberhalb Z=0 erreicht, 39 gespeicherte Samples. Unterwasser-/Oberflächenaufnahmen geprüft; keine Material-Kompilierungsfehler oder Ersatzmaterialien. Lokaler Ausgabeordner: `Builds/WindowsLandscape` |
| Manuelle Bedienung im Package | DE/EN-Umschaltung, Flaschenwahl, Start, Pause, Abbruch, Auswertung und Rückkehr zum Menü geprüft |
| Walensee-Geodaten | Offizielle Archiv-SHA256 stimmt; alle 46 Kacheln im Package geladen. Vergleich sämtlicher Laufzeitwerte mit Float32-Quelle: 23.334.479 Messwerte, 13.808.385 NoData-Werte erhalten, maximal 0,005 m Quantisierungsabweichung |
| Tiefenkarte | Package-Screenshot geprüft: korrekte Nordorientierung, drei Marker, aktuelle Position, Konturen und Quellenangabe |
| Landschaftsdaten | 286 swissALTI3D-Quelldateien der Ausgabe 2026 mit offiziellen SHA256 geprüft; 858 native 4 × 4-Pixelblöcke stimmen mit dem 8-m-Raster bis auf weniger als 0,000062 m numerische Abweichung überein |
| Wassermaterial | Single Layer Water mit Streuung/Absorption/Reflexion erzeugt; Content-Commandlet erfolgreich, 0 Fehler und 0 Warnungen |
| Wiederholte Asset-Erzeugung | Zwei aufeinanderfolgende Content-Builds und anschließender Editor-Spieltest bestanden; keine Material-Kompilierungsfehler oder Ersatzmaterialien |
| Biologie-Assets | Fünf Fischmodelle und drei Bewuchstypen in Blender erzeugt und als Übersicht gerendert; Flossenansätze nach Bildprüfung korrigiert. Letzter Content-Build: 0 Fehler, 0 Warnungen |
| Biologie im Windows-Package | Alle drei Oberflächenläufe mit je 38 s und 39 Samples bestanden; Modelle, Habitatgrenzen und Felsabstände in jedem Frame geprüft. Bei Murg alle fünf Fischtypen und drei Bewuchstypen vorhanden. Ausgabe: `Builds/WindowsEcology` |
| 3D-Handgelenkcomputer im Windows-Package | Tages-/Nachtlauf bei Murg bestanden, jeweils 22 s und 23 Samples, über 200 Display-Aktualisierungen, alle drei Seiten tatsächlich gezeichnet, angehobene Pose erreicht, Bildschirm-HUD ausgeblendet. Haupt-, Vital- und Gasseite im Bild geprüft. Ausgabe: `Builds/WindowsEquipment` |
| Auftauchen mit sichtbarer Ausrüstung | Murg im Equipment-Package bestanden: 38 s, 11,45 m maximale Tiefe, 29,33 m Strecke, 39 gespeicherte Samples; Oberfläche erreicht, maximal neun Geländekacheln, 359 Display-Aktualisierungen |
| Manuelle Handgelenkbedienung | Im Equipment-Package geprüft: Exploration, englische Anzeige, Nassanzug, C anheben/senken, R durch alle drei Seiten, H HUD aus/ein, L betätigt, Pause, Abbruch und Auswertung. Nach 2:50 min zum deutschen Startmenü mit Simulation, Weesen und Trockenanzug zurückgekehrt |
| Logbuch-Reader und Dateiverzeichnis | Drei Unreal-Automationstests bestanden: Schema 1/2, Datumsdarstellung, 7.201 Messpunkte über zwei Stunden, Zeitpunktsuche, Warnereignisse, fehlende/ungültige Werte, nicht monotone Reihen, unbekannte Versionen, temporäre Dateien, Testlauf-Filter und erneutes Lesen von Festplatte. Testdaten liegen isoliert unter `Saved/Automation`, nicht im Spieler-Logbuch |
| Logbuch im Windows-Package | 22 s Tauchgang bei Murg gespeichert und erneut gelesen: 23 Messpunkte, 11,45 m maximale Tiefe, 197,38 bar Restdruck. Danach alle vier Diagrammgruppen mit DE/EN-Beschriftung gezeichnet und aufgenommen; `DIVE_LOGBOOK_PASS samples=23 groups=15`. Ausgabe: `Builds/WindowsLogbook` |
| Manuelle Logbuchbedienung | Im abschließenden Package geprüft: leere Ansicht mit ausgeblendeten Testläufen, Aufruf aus Start- und Pausenmenü, Rückkehr mit Esc, neuer 11-s-Tauchgang mit zwölf Messpunkten, Öffnen aus der Auswertung, Vitalwertseite, Zeitcursor bei 00:04, Sprung zur Aufstiegswarnung bei 00:11, leeres Exploration-Filter, passendes Simulation-Filter und DE/EN-Wechsel. Testinstanz anschließend regulär beendet |
| Wellenmodell | Kernprüfungen bestanden: Windstille, Amplitudengrenze, analytische Raum-/Zeitableitungen gegen numerische Differenzen, Dispersionsbeziehung, Orbitalgeschwindigkeit an der Oberfläche und Druck-/Geschwindigkeitsabnahme bis 100 m. Konfigurierbarer Thermoklinen-Mittelpunkt geprüft |
| Wetter und bewegte Oberfläche im Windows-Package | Jeweils 38 s und 39 gespeicherte Messpunkte: Weesen bei Regen/12 m/s Wind (5,72 m maximale Tiefe, 31,50 m Strecke), Murg bei Windstille (11,45 m / 29,33 m), Walenstadt bei 3 m/s Wind (9,64 m / 25,77 m). Alle erreichen die lokale bewegte Oberfläche. Laufzeitprüfung kontrolliert Geometrie, vertikale Bounds und Übereinstimmung der CPU-/Material-Wellenkoeffizienten; keine Ersatzmaterialien. Ausgabe: `Builds/WindowsWeather` |
| Umweltmenü und Speicherung | Manuell auf Deutsch/Englisch geprüft, zunächst auf ultrabreitem Vollbild und danach im Fenster: Wetter, Windrichtung/-stärke, beide Temperaturen, Schichttiefe/-breite, Sicht, Strömung und Uhrzeit geändert. Neustart erhält alle Werte (u. a. Regen, 6 m/s nach 135°, 20/8 °C und 15/2,5 m Thermokline). Im pausierten Tauchgang bleiben Wetter/Wind/Temperatur nach Klick unverändert. Der 5-s-Probetauchgang speichert genau diese Umweltwerte; Testinstanz regulär beendet und deren Szenario-Einstellungen anschließend auf die Ausgangswerte zurückgesetzt |

![Tiefenkarte aus der verpackten Anwendung bei Murg](Images/WalenseeMap.png)

![Oberfläche und swissALTI3D-Landschaft im Package bei Weesen](Images/WalenseeSurface.png)

![3D-Handgelenkcomputer mit Live-Daten im Package](Images/WristComputer.png)

![Vitalwertseite bei Nacht und ausgeblendetem Bildschirm-HUD](Images/WristComputerNight.png)

![Logbuch mit gespeichertem Tiefen- und Gasprofil, Testdaten sichtbar im Prüflauf](Images/Logbook.png)

![Gespeicherte Vitalwerte im Logbuch](Images/LogbookVitals.png)

![Geometrische Wellen und windgetriebener Regen bei Weesen im abschließenden Windows-Package](Images/WeatherWaves.png)

![Windstille und Oberflächenreflexion bei Murg](Images/CalmWater.png)

Reproduktionsbefehle stehen in der README. Laufzeit-Protokolle und lokale Build-Ausgaben sind unter `artifacts/` abgelegt und nicht Bestandteil der Versionsverwaltung. Das automatische Gameplay startet mit fest definierten Einstellungen, speichert ein eigenes Protokoll, prüft dessen Inhalt und verändert keine Benutzereinstellungen. Maus- und Tastaturbindungen sind während des Skriptlaufs deaktiviert. Fischzustände werden in jedem Frame auf gültige Modelle, endliche Werte, Habitatgrenzen und Felsabstände geprüft.

## Behobene Fehler bei dieser Prüfung

- Ein erneuter vollständiger Asset-Import traf auf vom geöffneten Editor gesperrte Dateien. `Build.ps1 -Target Content -OnlyEnvironment` erzeugt jetzt gezielt Wasser, Wolken und Regen, ohne andere Assets neu zu schreiben. Der gezielte Lauf besteht ohne Fehler/Warnungen; Material-Speicherfehler brechen das Skript ausdrücklich ab. Ein vollständiger Import benötigt weiterhin freigegebene Zieldateien.

- Wechselnde Ziffern auf dem 3D-Computer hinterließen zunächst temporale Nachbilder. Der Export der Quelltextur war scharf und grenzte den Fehler auf die zeitliche Bildrekonstruktion ein. Das Display wird jetzt danach gezeichnet, mit expliziter Tiefenprüfung und Mip-Filterung. Tages-/Nachtaufnahmen zeigen einzelne ablesbare Werte statt überlagerter Ziffern.

- Der Habitat-Grenztest verglich Gleitkommawerte zunächst ohne Toleranz. AppleClang auf macOS erzeugte 37 Abweichungen durch zusammengefasste Multiplikation/Subtraktion. Eine unabhängige Nachrechnung reproduzierte genau diese 37 Fälle mit höchstens 1,78 × 10⁻¹⁵ m Differenz. Der Test verwendet jetzt eine explizite FMA-Referenz mit 10⁻¹² m Toleranz; das Laufzeitmodell bleibt unverändert.

- Laufzeitlicht verwendete zunächst eine nur im Editor vorhandene Hilfsfunktion; jetzt wird die Runtime-Komponente angesprochen.
- Logspeicherung wertete den booleschen Rückgabewert von `IFileManager::Move` falsch aus; erfolgreich geschriebene Dateien wurden als Fehler gemeldet.
- Schrift benötigt in Canvas einen gültigen Font-Asset-Verweis; dieser wird jetzt explizit geladen und mitgekocht.
- `-NoLiveCoding` verursachte beim monolithischen Development-Build einen inkompatiblen `WITH_LIVE_CODING`-Wert gegenüber den vorgebauten Engine-Bibliotheken. Der resultierende Absturz bei der Klassenregistrierung wurde im Dump untersucht; das Build-Skript erhält jetzt die Engine-Vorgabe. Der erneute Package-Start und Gameplay-Test bestehen.
- Die GeoTIFF-Vorschaustufen erzeugten beim Herunterskalieren abweichende Höhen statt der beabsichtigten Flächenmittel. Die Pipeline mittelt jetzt direkt die nativen 2-m-Pixel; `Tools/validate_landscape.py` prüft Quell- und Laufzeitprüfsummen sowie verteilte Pixelblöcke unabhängig nach.
- Unreals `DeleteAllMaterialExpressions` verändert in dieser Engineversion die Sammlung während ihrer Iteration und lässt dadurch Knoten zurück. Die Asset-Pipeline löscht jetzt über eine separate Liste und prüft, dass der Graph leer ist. Das verhindert doppelte Wasser-Ausgabeknoten und daraus folgende Ersatzmaterialien. Der Smoke-Test schlägt bei Material-Kompilierungsfehlern nun ausdrücklich fehl.

## Noch nicht abgenommen

Fische, Pflanzen, sichtbare Hände und synthetische Geräusche sind Prototypen. Fotorealismus, originalgetreue Zugangsanlagen, detaillierte Landschaft mit Landbedeckung, vollständige Lokalisierung, optische Qualitätsabnahme des Oberflächenübergangs, brechende Wellen und Uferwechselwirkung, anatomische Abnahme der fünf Fischmodelle (Felchen als Artengruppe) und drei Bewuchstypen, vollständiger Ausrüstungskatalog und Körpermodelle, Hardcore-Unterschiede, weitere UI-Funktionen und weitere MUST-Anforderungen bleiben offen. Wellen und Wetter sind gekoppelte Spielszenarien, keine vermessene Strömungs-/Wettervorhersage. Das ferne Land ist ein Hintergrundmesh ohne Spielerkollision; drei örtliche Uferbereiche und modellierte Zugänge sind begehbar. Kein macOS-Spieltest, kein kontrollierter Performance-Benchmark und kein zweistündiger Dauertest wurden durchgeführt. Die Kern-/Parser-CI auf Windows, macOS und Linux ist keine Prüfung des vollständigen Spiels auf diesen Plattformen.

## Audio-Erweiterung (25. November 2026)

- Engineunabhängige Prüfungen: 20 Ein-/Ausatmungen und 90 Herzschläge in einer Minute mit festen Vitalwerten; Leerflasche/letzter Atemzug; tatsächliche Jacket-Flüsse; Tiefendämpfung, Wetter, Warnwiederholung, Gesamtstummschaltung und ungültige Einstellungen. Alle Prüfungen bestanden.
- Fünf Python-Tests einschließlich Audiogenerator: Originalquellen besitzen Signal und Pegelreserve, geringen Gleichanteil und unauffällige Schleifenübergänge; Stereo-Umgebung ist reproduzierbar und dekorreliert.
- Audio-Import ohne Änderung von Materialien/Meshes: 0 Fehler und 0 Warnungen. Editor- und eigenständiger Windows-Development-Build erfolgreich.
- Erster verpackter Audiolauf: 38 s Simulation plus 3 s Pause, 12 Einatmungen, 11 Ausatmungen, 57 Herzereignisse, 3 Warnungen, 200 Inflations- und 201 Ventilschritte. Tauchen/Auftauchen, Telemetriespeicherung und vorhandene Umwelt-/Ausrüstungsprüfungen bestehen.
- Tatsächlich aufgezeichneter Mastermix: 40,94 s, Stereo, 48 kHz PCM16; aktiver RMS-Pegel 0,01372, Spitzenwert 0,1392 ohne Übersteuerung. Mittlere Pausensekunde exakt still, mittlere Stummsekunde RMS 0,00000470; Ton nach Wiederaufnahme vorhanden. Messungen sind technische Signalprüfungen, keine Hörabnahme.
- Manuelle Menüprüfung zunächst durch den Windows-Firewall-Dialog beim Erststart angehalten. Keine Netzwerkfreigabe erteilt. Die automatisierte Menü-/Speicherprüfung verwendet eine separate `DiveSettingsTest.json` und verändert keine Benutzereinstellungen.
- Abschließender Build `Builds/WindowsAudioFinal`: Editor-Build und Packaging erfolgreich. Wiederholter 38-s-Audiolauf einschließlich Menü-/Speichertest besteht. Alle fünf Minus-Schaltflächen über den tatsächlichen HUD-Handler ausgelöst; gespeicherte Werte 75/70/80/30/75 % nach Zurücksetzen erneut geladen und verglichen. Englisch-Umschaltung und gerenderte Menüansicht geprüft (siehe `Images/AudioMenu.png`). Das prüft Handler, Persistenz und Darstellung; die manuelle Mausklickprüfung bleibt wegen des Systemdialogs offen.
- Abschließender Mastermix: 41,00 s, aktiver RMS 0,01372, Spitze 0,1422; Pause RMS 0, Stummschaltung RMS 0,00000489, Wiederaufnahme vorhanden. Protokolle: `artifacts/runtime-audio-final.log`, `artifacts/smoke-audio-final.log`; Tondatei: `Builds/WindowsAudioFinal/divingsim/Saved/AudioQA/DiveMix.wav`.

## Belegung und Einführung (25. November 2026)

- Engineunabhängige Tutorial-Prüfungen bestehen: tatsächliche Blickänderung einschließlich Kompassübergang, Schwimmstrecke statt passiver Drift, Jacket-Zufuhr und -Abgabe, Computer/Vitalwerteseite/Absenken, Lampe aus/ein, Karte auf/zu, Tiefe und Einstiegsnähe sowie erfolgreicher Abschluss.
- Zwei Unreal-Automationstests bestehen: `DiveSim.Controls.BindingPersistence` und `DiveSim.Controls.InvalidAndLegacySettings`. Geprüft wurden Konflikttausch, vollständiger JSON-Roundtrip, Maustaste, Komfortwerte, reservierte/unbekannte/doppelte Eingaben und ältere Dateien ohne Steuerungsfelder. Bericht: `artifacts/ControlsTests/index.json`.
- Abschließender Editor-Build und Windows-Development-Package erfolgreich; ausführbarer Stand unter `Builds/WindowsControls`.
- Belegungstest im verpackten Spiel: echte Engine-Eingabeereignisse durchlaufen den neuen PlayerController. Vorwärts auf I und Lampe auf T geändert; alte W-/L-Belegung wirkungslos; Escape bricht Auswahl ab und führt aus der Hilfe zurück; F1 öffnet die Hilfe; Konflikttausch, Speicherung/Wiederherstellung, Sensitivität 0,15, Y-Invertierung und 95-Grad-Sichtfeld bestehen. Anschließender 22-s-Tauchlauf mit Pause: 5,65 m maximale Tiefe, 17,85 m Strecke, 198,47 bar Restdruck, 23 gespeicherte Samples. Frische DE-/EN-Menüaufnahmen erzeugt und visuell geprüft.
- Einführungstest im verpackten Spiel: alle acht Aufgaben über Engine-Eingaben durchlaufen, einschließlich Absenken des Computers; Abschluss nach 12,78 s automatisierter Spielzeit, 13 gespeicherte Samples. Der Automat bedient deutlich schneller als ein neuer Spieler; dies ist kein Usability- oder Langzeittest. Alle zehn geforderten Aufnahmen (acht Schritte, geöffnete Karte, Auswertung) sind frisch und vollständig.
- Vorherigen normalen Plan absichtlich auf Exploration, Walenstadt, 15 L/250 bar, Nassanzug, 10 kg Blei, Regen, 6 m/s Wind und 0,16 m/s Strömung gesetzt. Speichern während der Übung bewahrt diesen Plan; nach Abschluss werden Plan und Abschlusskennzeichen erneut von der Datei gelesen und geprüft. Die Übung selbst verwendet Simulation mit ruhigem Weesen-Szenario.
- Laufzeitbelege: `artifacts/runtime-controls-verified.log`, `artifacts/runtime-tutorial-verified.log`. Ausgewählte Bilder: `Images/Controls.png`, `Images/Tutorial.png`, `Images/TutorialComplete.png`.
- Manuelle Tastatur-/Mausprüfung und weitere Tastaturlayouts bleiben offen. Zur angefragten Schließung des Windows-Sicherheitsdialogs des früheren manuellen Audiotests liegt noch keine Rückmeldung vor; dafür wurde keine Firewall-Regel angelegt. Automatisierte Tests verwenden separate Einstellungen und ignorieren reale Desktop-Eingaben.

## Tauchkonfigurationsprofile (25. November 2026)

- Drei Unreal-Automationstests bestanden: `DiveSim.Profiles.Configuration`, `PersistenceAndRecovery` und `FileProtectionAndLimits`. Bericht: `artifacts/ProfileTests/index.json`.
- Alle Konfigurationsfelder einschließlich Sauerstoffanteil, Atmosphärendruck, automatischer Tarierung und Einstieg werden gespeichert und verglichen. Profile lehnen fehlende, ungültige oder außerhalb des Wertebereichs liegende Daten ab; ältere allgemeine Einstellungen erhalten Standards.
- Geprüft: Unicode-Namen, persistierte Wiederherstellung des vorherigen Konfigurations-/Namensstands, Archiv und Wiederherstellung nach erneutem Einlesen, doppelte Namen auch im Archiv, aktive Profilgrenze, beschädigte Originaldatei, Schreibfehler mit Rücknahme der Speicheränderung und Konflikt zwischen zwei nacheinander schreibenden Instanzen.
- Der Menülauf erzeugt ausschließlich Testdateien, bedient die tatsächlichen HUD-Handler und setzt Text in das Slate-Eingabefeld. Er prüft Laden, Namensdialog, Dubletten, Umbenennen, Aktualisieren, Wiederherstellen nach Disk-Reload, Archiv, DE/EN, Navigation zur Steuerung sowie die Sperre des Ladens während eines laufenden Tauchgangs. Dies ist keine manuelle Tastatur-/Mausprüfung.
- Abschließender Editor- und Windows-Development-Build unter `Builds/WindowsProfiles` erfolgreich. Der verpackte Profiltest besteht in allen elf Phasen. Anschließender 22-s-Tauchlauf: 5,67 m maximale Tiefe, 18,78 m Strecke, 197,99 bar Restdruck, 23 gespeicherte Samples. Laufzeitbeleg: `artifacts/runtime-profiles-final.log`.
- Frische Aufnahmen von Namensdialog, Dublette, DE-/EN-Profilseite, Archiv und Sperre während des Tauchgangs erzeugt. Die Fehlermeldung für doppelte Namen war zunächst hinter dem Dialog zu schwach sichtbar; sie wird jetzt im Dialog selbst angezeigt. Die vergrößerten Dialogschaltflächen und der korrigierte Fehlerhinweis wurden im abschließenden Package-Bild geprüft.
- Regression im selben Windows-Package: Einführung mit allen acht Aufgaben nach 12,72 s automatisierter Spielzeit abgeschlossen, 13 Samples gespeichert. Ursprünglicher Tauchplan und Abschlusskennzeichen nach erneutem Laden geprüft; alle zehn Aufnahmen frisch. Beleg: `artifacts/runtime-tutorial-profiles.log`. Die manuelle Bedienprüfung bleibt offen.

![Gespeicherte Tauchkonfiguration mit Vorschau](Images/Profiles.png)

![Namensdialog mit sichtbarem Hinweis auf ein bereits vorhandenes Profil](Images/ProfileName.png)

## Örtliche Uferdaten und Kollision (25. November 2026)

- Drei Uferbereiche auf Basis der vorhandenen swissBATHY3D- und nativen swissALTI3D-Daten erzeugt. 10.700 Zellen sind als modellierte Verbindung gekennzeichnet; die übrigen 431.668 Zellen stammen aus den Quellen. Details und Ortsreferenzen: `GEODATA.md`, Quellen-/Herkunftsmanifest: `Content/Diving/Geodata/shoreline.json`.
- Unabhängige Quellenprüfung bestanden: alle enthaltenen Bathymetriezellen höchstens 0,00000367 m Float32-Abweichung; verteilte native Landzellen höchstens 0,00000763 m Umwandlungsabweichung. Das sind Rechengenauigkeiten, keine Angaben zur Vermessungsgenauigkeit. Solver-Residuum unter 0,000008 m; analytische Ebene, unveränderte Randwerte und Höhenbegrenzung geprüft. Beleg: `artifacts/validate-shoreline.log`.
- Drei kollidierbare Ufermeshes mit zusammen 447.485 Dreiecken. Die Dreiecksdiagonalen entsprechen dem benachbarten Seebodenraster. Ein anfänglicher Unterschied zwischen bilinearer Höhenabfrage und ebenen Kollisionsdreiecken wurde durch Abfrage derselben Dreiecksflächen behoben.
- Windows-Package: Ortsprüfungen für alle drei Bereiche bestanden. Die Kollisionsstrahlen an den drei Startpositionen treffen bei 227,89 / 422,47 / 411,03 cm über dem Szenariopegel; die berechnete Dreieckshöhe stimmt damit überein. Die Prüfung beweist diese Positionen, noch keinen vollständigen begehbaren Zugang.
- Unveränderter Tauchablauf im Package besteht: 22 s, maximale Tiefe 5,67 m, Strecke 18,78 m, Restdruck 197,99 bar, 23 Samples. Neun gestreamte Seebodenkacheln bleiben begrenzt. Beleg: `artifacts/runtime-shore-regression.log`.
- Beim Build nach dem vorigen Commit wurden Namenskollisionen privater HUD-Hilfen durch Unreals adaptive Unity-Zusammenfassung sichtbar. Das kleine Spielmodul wird jetzt ausdrücklich mit getrennten C++-Übersetzungseinheiten gebaut, unabhängig vom Git-Arbeitsstatus.
- Dieser historische Prüfschritt betrifft die Geländebasis. Die anschließende Erweiterung um Uferstarts und Geh-/Schwimmwechsel ist unten separat dokumentiert. Wege, reale Bodenbedeckung, Landschafts-LOD und Fotorealismus bleiben offen.
- Abschließender Materialimport: 0 Fehler/0 Warnungen; Windows-Package `Builds/WindowsShoreline` erfolgreich. Drei wiederholte Ortsläufe mit frischen Bildern bestehen (`artifacts/runtime-shore-0-final.log` bis `runtime-shore-2-final.log`). Eine regelmäßige Körnung durch ungenaue Sinus-Hashes bei großen Weltkoordinaten wurde durch Ganzzahl-Hashing entfernt. Das Material bleibt illustrative prozedurale Gestaltung.

![Ufermodell bei Weesen mit Blick über den See](Images/Shore_0.png)

![Ufermodell bei Murg West](Images/Shore_1.png)

![Ufermodell bei Mols/Lediwracks](Images/Shore_2.png)

## Spielbare Uferstarts (25. November 2026)

Eigenständiger Windows-Build: `Builds/WindowsAccess/divingsim.exe`. BuildCookRun erfolgreich, Beleg: `artifacts/package-access-verified.log`.

| Vollständiger Ablauf im Package | Dauer inkl. Landweg | Maximale Tiefe | Strecke | Gespeicherte Messpunkte |
|---|---:|---:|---:|---:|
| Weesen / Strandbad | 41,10 s | 6,12 m | 44,65 m | 42 |
| Murg / West | 41,84 s | 4,21 m | 46,30 m | 42 |
| Mols / Lediwracks | 46,62 s | 4,76 m | 51,83 m | 47 |

Alle drei Läufe bestehen `DIVE_SHORE_PASS` und `DIVE_SMOKE_PASS`. Die Tests betätigen die regulären Eingaben für Gehen, Abtauchen, Rückweg und Abschluss. Sie prüfen Bodenkontakt, Augenhöhe, den Wechsel zum Schwimmen, die Rückkehr auf das flache Startpodest und das erneut gelesene abgeschlossene Logbuch. Ein vorzeitiges Beenden vor dem ersten Abtauchen wird abgewiesen. Die Startart bleibt während des Tauchgangs gesperrt; danach kann sie gewechselt werden. Einstellungs-Roundtrip und frische Bilder für Start, Wasserwechsel, Tauchen, Rückkehr, Auswertung und Menü sind vorgeschrieben.

- Kollisionsstrahlen treffen die drei Plattformen bei Z 249,54 / 472,82 / 455,50 cm, jeweils passend zur Bodenabfrage. Zusätzliche Abfragen entlang aller drei Konstruktionen unterscheiden deren Oberseite vom Seegrund darunter. Ein Schwimmer unter der Treppe wird nicht auf ihre Oberseite versetzt.
- Der Rückweg hat in keinem 20-ms-Schritt einen Höhensprung über 20 cm. Die Zahl gestreamter Seebodenkacheln bleibt auf neun begrenzt; vorhandene Wasser-, Audio-, Ausrüstungs- und Habitatprüfungen laufen während der Tauchgänge mit.
- Der engineunabhängige Kern besteht einschließlich trockener/halber/vollständiger Immersion, Schwerkraft, erhaltenem Unterwasserauftrieb, Bodenkontakt, Übergangshysterese und ermüdungsabhängigem Gehtempo.
- Drei Unreal-Profiltests bestehen: neuer Startmodus im vollständigen Konfigurations-Roundtrip und Weiterverwendung älterer Profile ohne `start_on_shore`.
- Regressionen im selben Package bestehen: bisheriger Wasserstart (22 s, 5,67 m, 18,78 m, 23 Messpunkte), Einführung (acht Stufen, 12,82 s, neun frische Aufnahmen plus Abschluss), elf Phasen der Profilbedienung mit anschließendem Tauchgang und Ufer-Kollisionsstrahlen an allen drei Orten. Belege: `artifacts/runtime-access-regression-*.log`. Insgesamt sieben erfolgreiche Package-Läufe einschließlich der drei vollständigen Uferabläufe.
- Der Bildschirmkompass zeigt die zur Gradzahl passende Himmelsrichtung. Nebel, Regen und Audio berücksichtigen im Ufermodus die erhöhte Augenposition.
- Belege: `artifacts/runtime-access-packaged-0.log` bis `runtime-access-packaged-2.log`, `artifacts/runtime-access-preview-editor.log` und `artifacts/profiles-access.log`.

Die Aufnahmen wurden geprüft. Die Landschaft ist weiterhin grob und ohne Gebäude/Landbedeckung; sichtbare Materialübergänge zwischen Uferergänzung und Seeboden sind noch nicht optisch abgestimmt. Plattformen, Treppen und Geländer sind erfundene Spielgeometrie auf den Höhendaten, keine originalgetreue Ortsrekonstruktion. Der Körper bleibt eine aufrechte Kollisionskapsel; der Atemregler bleibt an Land angelegt, die Uhr zählt den Landweg mit und das Temperaturmodell unterscheidet noch keine Luftphase. Damit ist der spielbare Uferwechsel geprüft, die vollständige Realismus- und Grafikabnahme bleibt offen.

![Spielbarer Uferstart bei Weesen](Images/Access_Weesen.png)

![Spielbarer Uferstart bei Murg](Images/Access_Murg.png)

![Spielbarer Uferstart bei Mols](Images/Access_Mols.png)

![Rückkehr auf das Startpodest](Images/Access_Return.png)

![Auswahl des Uferstarts](Images/Access_Menu.png)

## Georeferenzierte Landschaftstexturen (25. November 2026)

Eigenständiger Windows-Build: `Builds/WindowsImagery/divingsim.exe`. Abschließender BuildCookRun erfolgreich: `artifacts/package-imagery-final.log`. Die Höhen- und Kollisionsgeometrie bleibt unverändert.

- 290 offizielle SWISSIMAGE-Quelldateien des Kacheljahrs 2025 vollständig gegen die veröffentlichten SHA256 geprüft: 286 Landschaftskacheln mit 2 m und vier örtliche Kacheln mit 0,1 m Pixelabstand. Herkunft, Nutzungsbedingungen und zusätzliche Planet-Quellenangabe stehen in `GEODATA.md` und im versionierten Laufzeitmanifest.
- Vier Texturen aus nativen RGB-Pixeln erzeugt: Landschaft 4096 × 2048, drei Uferbereiche jeweils 2048 × 2048. Ein unabhängiger Prüfer berechnet 675 flächengewichtete Pixel einschließlich Kachelgrenzen erneut; größte Abweichung 0,499867 von 255 durch Ganzzahlrundung. Das ist eine Rechenprüfung, keine Aussage zur Lagegenauigkeit der Luftbilder.
- Erneutes Erzeugen mit den im Manifest festgehaltenen Quellen liefert identische Bild- und Manifestprüfsummen. Belege: `artifacts/validate-imagery-pinned.log` und `artifacts/imagery-reproduction.log`. Die vier Unreal-Asset-Metadaten stimmen mit den PNG-Prüfsummen überein (`artifacts/imagery-asset-audit.json`).
- Gezielter Materialimport einschließlich erneutem Ufermaterialaufbau: 0 Fehler, 0 Warnungen. Editor- und Windows-Development-Build erfolgreich.
- Die Laufzeit prüft Ausdehnung, Nordorientierung und verteilte Mesh-UVs gegen LV95 sowie Texturgröße, sRGB, Clamp und vollständig residente Mip-Stufen. Alle vier Texturen bestehen `DIVE_IMAGERY_PASS`; 13 Landschafts- und jeweils 12 Ufer-Mips sind geladen. Gemeldeter Texturspeicher zusammen 14.090.240 Bytes (13,44 MiB).

| Vollständiger Uferablauf im Package | Dauer inkl. Landweg | Maximale Tiefe | Strecke | Gespeicherte Messpunkte |
|---|---:|---:|---:|---:|
| Weesen / Strandbad | 41,10 s | 6,11 m | 44,64 m | 42 |
| Murg / West | 41,92 s | 4,27 m | 46,50 m | 42 |
| Mols / Lediwracks | 46,60 s | 4,75 m | 51,78 m | 47 |

Alle drei Abläufe einschließlich Rückkehr und erneut gelesenem Logbuch bestehen. Weitere erfolgreiche Package-Läufe: bisheriger Wasserstart (22 s, 5,67 m maximale Tiefe, 18,78 m Strecke, 23 Messpunkte), Handgelenkcomputer bei Nacht mit allen drei Seiten und Ufer-Vorschau mit Kollisionsprüfung an allen drei Plattformen. Belege: `artifacts/imagery-packaged-0-full.log` bis `imagery-packaged-2-full.log` sowie `imagery-regression-*-full.log`. Nach dem abschließenden identischen Materialneuaufbau und Packaging besteht die erneute Vorschau bei Mols einschließlich aller vier Texturen (`artifacts/runtime-imagery-final-preview.log`; ausführliches Laufzeitprotokoll `artifacts/DiveSmoke.log`).

Die Aufnahmen zeigen geografisch zugeordnete Wald-, Feld- und Wegflächen. Gebäude und Bäume bleiben in die flache Oberfläche fotografiert; nahe Ansichten wirken dadurch weich und flach. Grobe Berggeometrie, dunkle steile Flächen und bereits im Luftbild enthaltene Schatten bleiben sichtbar. Das ist keine fotorealistische Qualitätsabnahme. Räumliche Landschaftsdetails, Gelände-LOD und originalgetreue Zugangsanlagen bleiben offen.

![SWISSIMAGE-Oberfläche beim Start in Weesen](Images/Imagery_Start_0.png)

![SWISSIMAGE-Oberfläche beim Start in Murg](Images/Imagery_Start_1.png)

![SWISSIMAGE-Oberfläche beim Start in Mols](Images/Imagery_Start_2.png)

![Rückkehr zum Ufer mit fotografierter Bodenbedeckung](Images/Imagery_Return.png)

![Menü mit Quellenangaben](Images/Imagery_Menu.png)

## Gekachelte Nanite-Landschaft (25. November 2026)

Die Landschaft verwendet jetzt alle 8-m-Höhensamples statt eines einzelnen Meshes mit 32-m-Abstand. 77 Kacheln enthalten insgesamt 4.501.560 Quellvertices einschließlich gemeinsamer Randvertices und 8.167.046 Dreiecke nach Aussparung von See und Uferergänzungen. Die Quelle bleibt das aus nativen 2-m-Daten gemittelte swissALTI3D-Raster; die höhere Meshdichte fügt keine Vermessungsdetails unterhalb von 8 m hinzu.

- Die erneute unabhängige Datenprüfung bestätigt alle 286 offiziellen SHA256 sowie 858 native 4 × 4-Pixelmittel mit höchstens 0,00006104 m Float32-Abweichung. Beleg: `artifacts/validate-landscape-nanite.log`.
- Der Bake-/Prüfcommandlet vergleicht sämtliche gespeicherten Quellvertices mit dem Raster (Toleranz 0,02 cm für Float32 in Unreal-Zentimetern), sämtliche Vertex-Instanz-UVs mit der Kartenregistrierung und prüft Einheitsnormalen, senkrechte Tangenten, Topologieanzahl und Nanite-Daten. Nanite verwendet ungekürzte Quelldreiecke und 1 cm Positionsquantisierung; die später ausgewählte Renderdetailstufe und der konventionelle Fallback sind davon zu unterscheiden.
- `Tools/validate_landscape_meshes.py` bestätigt unabhängig 77 eindeutige Kacheln, die vollständigen Randzeilen/-spalten, Weltursprünge, Vertex-/Dreiecksummen und identische Eingangsprüfsummen bei LF-/CRLF-Zeilenenden. Die Unreal-Quellassetprüfung bleibt zusätzlich erforderlich.
- Die Buildprüfung wurde mit einer geänderten Quellenmanifest-Referenz ausgelöst: `LandscapeCheck` lehnt den veralteten Stand mit Exitcode 1 ab, bevor Assets geschrieben werden. Beleg: `artifacts/land-check-stale-full.log`. Anschließend wurden die Assets mit den endgültigen Quellenreferenzen neu gebaut.
- Das Landschaftsmaterial erhält ausdrücklich die Nanite-Verwendungsfreigabe. Im Editor wartet das Laden auf die fertig kompilierte Textur: Ein zuerst beobachteter 32 × 32-Platzhalter ist damit behoben, ohne die Dimensions-/Mip-Prüfung abzuschwächen.
- Diagnose vor der Umstellung: Bilder mit unveränderter Beleuchtung, ohne GI, ohne Lumen-Screen-Traces und ohne Beleuchtung wurden verglichen (`artifacts/land-light-*.png`). Das Abschalten der Screen-Traces beseitigt die dunklen Bergflanken nicht; die unbeleuchtete Ansicht zeigt dort Bilddetails. Daraus folgt noch keine abschließend nachgewiesene einzelne Ursache.

Die neue Geometrie verbessert Konturen und kleine Geländekanten. Die dunklen Flanken, im Orthobild enthaltene Schatten, flache fotografierte Bäume/Gebäude und gestreckte Texturen an steilen Flächen bleiben sichtbar. Beleuchtung und Nahansichten sind weiterhin nicht fotorealistisch abgenommen. Alle Landschaftskomponenten werden beim Start geladen; Nanite streamt seine Geometrieseiten. Fernes Land bleibt ohne Spielerkollision. Ein Render-Fallback unter Windows ist kein Nachweis für einen vollständigen macOS-Spielbuild, und kurze Smoke-Läufe ersetzen weder Performance-Abnahme noch zweistündigen Dauertest.

Abschließender Editor-Build und Windows-Package `Builds/WindowsTerrain/divingsim.exe` erfolgreich. Die vorgeschaltete Prüfung bestätigt alle 77 Assets; Belege: `artifacts/build-terrain-verified.log` und `artifacts/package-terrain-verified.log`. Beim Neuaufbau gab es vorübergehende Dateisperren; Unreals Wiederholungsversuche und die anschließende vollständige Assetprüfung waren erfolgreich.

| Vollständiger Uferablauf im endgültigen Package | Dauer inkl. Landweg | Maximale Tiefe | Strecke | Gespeicherte Messpunkte |
|---|---:|---:|---:|---:|
| Weesen / Strandbad | 41,08 s | 6,11 m | 44,64 m | 42 |
| Murg / West | 41,92 s | 4,27 m | 46,50 m | 42 |
| Mols / Lediwracks | 46,64 s | 4,76 m | 51,83 m | 47 |

Alle drei Läufe bestehen Uferwechsel, Rückkehr, Bodenkontakt, Sprunggrenze und das erneut gelesene abgeschlossene Logbuch. Nachtregression: 22,02 s, 5,67 m maximale Tiefe, 23 Messpunkte und alle drei Handgelenkseiten. Regen-/Oberflächenregression: 38,02 s, 5,72 m maximale Tiefe, 31,50 m Strecke und 39 Messpunkte. Gestreamte Seebodenkacheln bleiben auf neun begrenzt. Belege: `artifacts/terrain-shore-0-full.log` bis `terrain-shore-2-full.log`, `terrain-wrist-night-full.log` und `terrain-surface-rain-full.log`.

Der zusätzliche Package-Lauf mit `-DisableNanite` bestätigt `r.Nanite = "0"`, die weiterhin sichtbare Landschaft, vollständige Texturen und die drei Ufer-Kollisionsstrahlen. Der konventionelle Fallback enthält insgesamt 897.526 Dreiecke. Ein siebter Lauf mit `-ExecCmds="r.Nanite.Visualize Clusters"` zeigt die Landschaft in der tatsächlichen Nanite-Clusteransicht. Beide Bilder wurden geprüft; Belege: `artifacts/terrain-fallback-full.log` und `artifacts/terrain-nanite-clusters.log`. Alle sieben Package-Läufe bestehen `DIVE_SMOKE_PASS`, `DIVE_LANDSCAPE_PASS` und `DIVE_IMAGERY_PASS`.

![Feineres Geländeraster bei Weesen](Images/Terrain_Start_0.png)

![Feineres Geländeraster bei Murg](Images/Terrain_Start_1.png)

![Feineres Geländeraster bei Mols](Images/Terrain_Start_2.png)

![Konventioneller Render-Fallback bei ausgeschaltetem Nanite](Images/Terrain_Fallback.png)

![Nanite-Clusterdiagnose der tatsächlich gerenderten Berggeometrie](Images/Terrain_NaniteClusters.png)

## Getrennte Luft- und Wasseroptik (25. November 2026)

Eigenständiger Windows-Build: `Builds/WindowsMedium/divingsim.exe`. Editor-Build und abschließender BuildCookRun erfolgreich; die vorgeschaltete unveränderte Landschaftsprüfung bestätigt erneut die 77 Quellassets. Belege: `artifacts/build-medium-sync.log` und `artifacts/package-medium-verified.log`.

- Luftnebel verwendet weiße Partikel und die Himmelsatmosphäre statt des grünen Unterwasserfarbtons. Sichtweite unter Wasser verändert den Luftdunst nicht; Regen erhöht dessen Dichte von 0,002 auf 0,012. Unter Wasser bleiben bei 14 m Sichtweite 0,35 Dichte und der bisherige grüne Farbton erhalten.
- Nebel und Kamera-Postprozess verwenden dieselbe glatte Überblendung über 0–0,5 m Kameratiefe und die aktuelle Augenposition einschließlich Atembewegung. Die Aktualisierung des Kameraversatzes erfolgt vor der optischen Auswertung. Auch im Pausenmenü wird der Nebel an der Kamera gehalten; Wellen und Spielphysik bleiben angehalten.
- Zwölf zusätzliche engineunabhängige Prüfungen decken Endpunkte, glatten Übergang, Trennung von Wetter und Unterwassersichtweite sowie endliche Ersatzwerte ab. Der gesamte Kern besteht mit null Fehlern (`artifacts/core-medium.log`).
- Der Laufzeittest prüft nach dem Pawn-Tick die Übereinstimmung von Kamera-Tint und Nebelmedium. Beim ersten Erreichen jedes Mediums prüft er zusätzlich tatsächliche Nebeldichte, Höhenabfall, Farbe, Himmelsbeitrag und Partikelalbedo. Die Oberflächen- und Uferabläufe müssen Luft, Übergang und Wasser tatsächlich durchlaufen.
- `-Hour 23` überschreibt die Tageszeit nur für den automatisierten Test. Relative Package-Ausgabepfade werden jetzt vor dem UAT-Aufruf gegen das Repository aufgelöst; der abschließende Build belegt das korrekte Archivziel im Projektordner.

| Ablauf im endgültigen Package | Maximale Tiefe | Strecke | Messpunkte | Ergebnis |
|---|---:|---:|---:|---|
| Auftauchen bei Windstille, Tag | 5,67 m | 21,54 m | 39 | Alle drei Medien |
| Auftauchen bei Regen, 12 m/s Wind | 5,72 m | 31,50 m | 39 | Alle drei Medien |
| Auftauchen um 23 Uhr | 5,67 m | 21,54 m | 39 | Alle drei Medien |
| Murg: Ufer, Tauchen, Rückkehr und Abschluss | 4,26 m | 46,48 m | 42 | Alle drei Medien, abgeschlossenes Logbuch |
| Handgelenkcomputer nachts | 5,67 m | 18,78 m | 23 | Alle drei Anzeigeseiten |

Alle fünf Läufe bestehen `DIVE_SMOKE_PASS`, `DIVE_LANDSCAPE_PASS` und `DIVE_IMAGERY_PASS`; es gibt keine gemeldete Medium-, Wasser-, Ausrüstungs- oder Habitatverletzung. Die Oberflächenläufe bestätigen Luftdichte 0,002 beziehungsweise 0,012 bei Regen, Wasserdichte 0,35 und Himmelsbeitrag 1 in Luft beziehungsweise 0 unter Wasser. Vollständige Belege: `artifacts/medium-{day,rain,night,shore,wrist}-full.log`. Ein zusätzlicher Editor-Oberflächenlauf besteht (`artifacts/medium-editor-surface-full.log`).

Die frischen Bilder wurden visuell geprüft: Luft ist weniger grün; die Nachtansicht über Wasser ist nahezu schwarz. Unter Wasser bleibt nachts ein künstlicher grüner Grundton sichtbar. Die dunklen Bergflanken, flachen/gestreckten Orthobilddetails und vereinfachte Nachtbeleuchtung bleiben offen. Das Abschalten des Nebels in einer vorherigen Vergleichsaufnahme machte die dunklen Bergflächen noch dunkler; der Luft-/Wasserfehler erklärt sie daher nicht vollständig. Diese Änderung ist keine physikalische Beleuchtungs- oder Fotorealismusabnahme.

![Luftansicht nach dem Auftauchen am Tag](Images/Medium_Air_Day.png)

![Luftansicht um 23 Uhr ohne künstlich grünen Luftdunst](Images/Medium_Air_Night.png)

![Unterwasseransicht nachts mit weiterhin vereinfachtem grünen Grundton](Images/Medium_Water_Night.png)

## HDR-Beleuchtung und Nachtlampe (25. November 2026)

Eigenständiger Windows-Build: `Builds/WindowsLighting/divingsim.exe`. Editor- und Package-Build erfolgreich; gezielter Materialaufbau mit `-OnlyOptics` meldet 0 Fehler/0 Warnungen. Belege: `artifacts/build-lighting-units.log`, `content-lighting-units.log` und `package-lighting.log`. Die Landschaftsassets wurden vor dem Packaging erneut geprüft, nicht neu erzeugt.

Die Untersuchung fand zwei konkrete Fehler: Das Unterwassermaterial addierte einen konstanten grünen Farbwert nach dem Tonemapping, und die Lampe verwendete 14.000 Legacy-Einheiten statt einer expliziten photometrischen Einheit. Das neue Material arbeitet vor Bloom/Tonemapping; sein Umgebungslicht folgt Tageszeit, Wetter und Kameratiefe. Nachts ist dieser Anteil null. Die Lampe verwendet 1.000 Lumen, die Sonne maximal 80.000 Lux und die Kamera eine begrenzte, umgebungsabhängige Belichtung.

Vergleichsaufnahmen mit 16-fachem Himmelslicht und ausgeschalteter Himmelsabschattung (`artifacts/light-sky16.png` und `light-sky-no-shadow.png`) zeigen, dass mehr Himmelslicht Details auf den dunklen Flanken sichtbar macht; das Abschalten der Schatten allein hilft deutlich weniger. Die endgültige Einstellung verwendet einen neutralen Sky-Light-Multiplikator von 3 ohne zusätzliche Multiplikation mit Tageslicht. Das ist eine gestalterische Abstimmung, keine unabhängige radiometrische Kalibrierung der Landschaft.

- Vierzehn zusätzliche Kernprüfungen bestehen: Nacht ohne direkte Sonne/ambienten Wasserlichtanteil, periodische Stunden, Tiefen-/Sicht-/Wetterkopplung, endliche Ersatzwerte und begrenzte Belichtung. Gesamter Kern: null Fehler (`artifacts/core-lighting-final.log`).
- Laufzeitprüfungen lesen die tatsächlichen Sonnen-/Himmelslichtwerte, Lampeneinheit, Intensität und Schaltzustand. Der geladene Materialstand muss den Parameter `AmbientLight` enthalten, vor Bloom laufen und Unreals Pre-Exposure-Konvertierung aktiv lassen.
- Der erste HDR-Tageslauf zeigte eine sichtbare Lumen-/Sky-Cache-Clippingwarnung. Die Engine-Quelle beschreibt für den Cache-Offset 8 den sicheren Bereich ungefähr EV -4 bis 16. Die Projektkonfiguration verwendet diesen Wert; sämtliche endgültigen Package-Logs bestätigen dessen Anwendung. Die frischen Tages-/Regenbilder enthalten die Warnung nicht mehr.

| Ablauf im endgültigen Package | Maximale Tiefe | Strecke | Messpunkte |
|---|---:|---:|---:|
| Auftauchen, Tag/windstill | 5,67 m | 21,54 m | 39 |
| Auftauchen, Regen/12 m/s Wind | 5,72 m | 31,50 m | 39 |
| Nacht, Lampe an | 5,67 m | 18,78 m | 23 |
| Nacht, Lampe aus | 5,67 m | 18,78 m | 23 |
| Handgelenkcomputer nachts, alle drei Seiten | 5,67 m | 18,78 m | 23 |
| Weesen: Ufer, Tauchen, Rückkehr und Abschluss | 6,11 m | 44,63 m | 41 |

Alle sechs Abläufe bestehen, ebenso ein zusätzlicher Einführungslauf mit Lampenwechsel: acht Stufen in 12,88 s und neun reguläre Aufgaben-/Kartenaufnahmen plus Abschluss. Die bestehenden Gelände-, Bildtextur-, Wasser-, Habitat- und Ausrüstungsprüfungen bleiben aktiv. Belege: `artifacts/lighting-{day,rain,night-on,night-off,wrist,shore,tutorial}-full.log`.

`Tools/validate_lighting_frames.py` vergleicht die beiden 1600×900-Nachtaufnahmen außerhalb von HUD, Randarmen und Fadenkreuz. Im unbeleuchteten Prüfbereich sind sämtliche Pixel schwarz. Mit Lampe liegen 74.986 Pixel über Kanalwert 20 von 255; der Mittelwert des jeweils größten RGB-Kanals steigt von 0 auf 7,492. Die Prüfung besteht (`artifacts/lighting-frame-check.log`). Dies belegt einen tatsächlich gerenderten Lichtkegel und das entfernte konstante Hintergrundleuchten, keine physikalische Lichtmessung. Der Vergleich verwendet `dive -Hour 23` mit und ohne `-LampOff`.

Die Bilder wurden visuell geprüft. Handgelenktext bleibt nachts lesbar, die Lampe beleuchtet den Seegrund lokal, und die Tagesansicht bleibt spielbar. Steile Bergflanken sind weiterhin teils zu dunkel, Orthobilddetails flach/gestreckt und Oberflächenreflexionen sichtbar verrauscht. Sonnenbahn und Augenanpassung bleiben vereinfachte Spielmodelle; Mond/Sterne/Uferbeleuchtung, reale Licht-/Wasserreferenzen und die gesamte Fotorealismusabnahme sind offen.

![Tagesansicht mit HDR-Licht ohne Cache-Clippingwarnung](Images/Lighting_Day.png)

![Nachtaufnahme mit eingeschalteter 1000-Lumen-Lampe](Images/Lighting_Torch_On.png)

![Derselbe Nachtablauf mit ausgeschalteter Lampe](Images/Lighting_Torch_Off.png)


## Atemquelle und Luft-/Wasser-Temperatur (25. November 2026)

Eigenständiger Windows-Build: `Builds/WindowsBreathing/divingsim.exe`. Der Uferstart verwendet Umgebungsluft; mit der frei belegbaren Taste T wird der Atemregler eingesetzt/abgesetzt. Der Kern unterscheidet Regler, Umgebungsluft und fehlende Atemluft. Lufttemperatur, Wind und eingetauchter Körperanteil beeinflussen das qualitative Kältemodell. Neue Logdateien verwenden Schema 3 und speichern den Atem-/Umgebungskontext pro Messpunkt; die Anzeige liest weiterhin Schema 1/2.

- Kernprüfungen: null Fehler (`artifacts/core-breathing-final.log`). Zusätzliche Fälle prüfen freien Luftatem ohne Flaschenverbrauch, Verbrauch mit Regler über Wasser, unabhängigen Jacketverbrauch, fehlende Versorgung unter Wasser, Erholung nach Wiedereinsetzen, leeren Regler an Land, Luft-/Wasser-Abkühlung und ausbleibende Reglergeräusche bei Umgebungsluft.
- Acht Unreal-Automationstests bestehen: drei Logbuch-, drei Profil- und zwei Steuerungstests. Enthalten sind Schema-3-Kontext, Warnbit 256, ungültige/fehlende Kontextwerte, alte Logdateien, alte Profile ohne Lufttemperatur sowie die neue Tastenbelegung.
- Der neue Gameplay-Ablauf `breathing` besteht sowohl im Editor-Spielmodus als auch im Package: Regler absetzen nach vier Sekunden, drei Sekunden ohne Versorgung unter Wasser, wieder einsetzen, auftauchen und auf Umgebungsluft wechseln. Ohne Regler bleibt der Flaschendruck konstant; simulierte SpO2 sinkt und erholt sich nach Wiedereinsetzen. Die vier geprüften Phasen enden bei 198,0995 bar, 92,872 % SpO2 und 81,8 % Körperimmersion. Frische Warnungs-/Oberflächenbilder sind verpflichtender Teil des Testskripts.
- An der welligen Oberfläche wechseln die aufgezeichneten Atemquellen bei abgesetztem Regler zeitweise zwischen Luft und fehlender Versorgung. Kurzes Überspülen hat im derzeitigen Modell bereits Auswirkungen; Atemanhaltereserve und alveolare Gasdynamik fehlen. Dieser Test belegt die implementierte Kopplung, keine realistische medizinische Reaktionszeit.
- Steuerungstest mit T/L-Konflikttausch, achtteilige Einführung, Audio und vier Logbuchseiten bestehen. Die Steuerungsseite mit 17 Aktionen und neue Kontextzeile im Logbuch wurden visuell geprüft; keine Überlagerung bei 1600×900.
- Die tatsächlich gerenderte Tonaufnahme besteht `validate_audio_mix.py`: 40,85 s Stereo/48 kHz, aktiver RMS 0,01393 und Spitzenpegel 0,1404. Pause ist still, Master-Stummschaltung unterschreitet die Prüfschwelle und der Ton kehrt zurück (`artifacts/breathing-audio-mix.log`). Natürliche Atmung ohne Regler besitzt noch keinen eigenen Klang.

Belege: `artifacts/breathing-editor-full.log`, `breathing-package-*-full.log`, `build-breathing-final.log` und `package-breathing.log`. Physiologie, Wärmehaushalt und Körpergeometrie bleiben ausdrücklich vereinfachte Spielmodelle. Dauer/Strecke enthalten weiterhin den Landweg; die vollständige Anforderungsabnahme bleibt offen.

Alle drei Uferabläufe bestehen mit zusätzlicher Prüfung der Umgebungsluft beim Start/Abschluss und unverändertem Anfangsdruck vor Einsetzen des Reglers:

| Einstieg | Maximale Tiefe | Strecke | Messpunkte |
|---|---:|---:|---:|
| Weesen / Strandbad | 5,75 m | 43,84 m | 40 |
| Murg / West | 4,23 m | 46,41 m | 42 |
| Mols / Lediwracks | 4,76 m | 51,87 m | 47 |

Auch der Profilablauf mit Speichern, Laden, Rückgängig, Archiv und gesperrter Konfiguration während des Tauchgangs besteht. Insgesamt neun Package-Szenarien: Atmung, Steuerung, Einführung, Audio, Logbuch, Profile und drei Uferstarts.

![Uferstart mit Umgebungsluft und Reglertaste](Images/Breathing_ShoreAir.png)

![Warnung nach Absetzen des Reglers unter Wasser](Images/Breathing_NoSupply.png)

Eine unabhängige Auswertung der neun erzeugten JSON-Protokolle bestätigt 281 konsistente Messpunkte: alle drei Atemquellen vorhanden, Warnbit 256 genau bei fehlender Versorgung, gültige Immersion und die gespeicherte Umgebungstemperatur passend zur Luft-/Wassergewichtung.

Nach den abschließenden Textkorrekturen wurden Editor und Windows-Package erneut erfolgreich gebaut. Atmungs- und Logbuchablauf bestehen auch im endgültigen Package (`artifacts/breathing-final-full.log`, `breathing-final-logbook-full.log`, `package-breathing-final.log`).


## Endliche Atemreserve und verzögerte Sättigungsreaktion (25. November 2026)

Der Folgestand ersetzt die unmittelbare SpO2-Abnahme bei jedem Mund-/Wasserkontakt durch eine endliche, belastungsabhängige Atemreserve. Erholung erfolgt kontinuierlich statt beim Umschalten des Reglers. Die Anzeige heißt ausdrücklich „Atemreserve (Spiel)“; es gibt keine Anzeige einer sicheren Atemanhaltezeit. Die begründende Fachquelle und die unabhängig davon gewählten Spielparameter sind in `SIMULATION.md` getrennt dokumentiert.

Kernprüfungen bestehen (`artifacts/core-reserve.log`): kurze Unterbrechung mit stabiler Sättigung, Verbrauch bei Anstrengung, schrittweise Erholung ohne sofortiges Zurücksetzen, vollständige Entleerung mit Abbruchschwellwert, wiederholte Wellenkontakte, gleiche Ergebnisse bei großen/kleinen Integrationsaufrufen und Warnton-Sperre. Das Modell bleibt unvalidiert; die Tests beweisen die implementierten Beziehungen, keine klinische Genauigkeit.

Drei Unreal-Logbuchtests bestehen (`artifacts/test-reserve-logbook.log`). Schema 4 verlangt eine endliche Reserve zwischen 0 und 1. Ein gezielt als Text statt Zahl gespeicherter Messwert deckte Unreals implizite Zahlkonvertierung auf; der Leser prüft nun den tatsächlichen JSON-Zahlentyp. Fehlende, falsch typisierte und außerhalb des Bereichs liegende Reservewerte werden abgewiesen. Versionen 1–3 bleiben lesbar, ohne Reservewerte zu erfinden.

Im Editor-Spielmodus bestehen beide Atemabläufe:

| Ablauf | Beobachtung |
|---|---|
| Kurzer Reglerwechsel und Auftauchen | Nach 25 s: 198,0995 bar, SpO2 98 %, Reserve 97,37 %. Im vorherigen Stand lagen bei demselben Ablauf 92,872 % SpO2 an. |
| Regler nach 4 s dauerhaft abgesetzt | Bei 8 s Reserve ca. 94 %, SpO2 98 %; nach 80 s ca. 15 % Reserve und 95 % SpO2. |
| Vollständiger Reserveverlust | Abbruch bei 118,74 s, Reserve 0, SpO2 59,992 %, Flasche weiterhin 199,8108 bar. 119 Messpunkte gespeichert und alle vier Logbuchseiten gerendert. |

Belege: `artifacts/reserve-editor-breathing-full.log` und `reserve-editor-loss-full.log`. Der neue Test `reserve-loss` prüft den tatsächlichen Abbruchgrund, unveränderten Flaschendruck während fehlender Versorgung, Speicherung/erneutes Lesen und frische Aufnahmen bis zur Auswertung. Der erste Editorlauf zeigte beim späten Kontrollbild noch die automatisch geöffnete Testkarte; das Zeitfenster dieses Szenarios wurde auf 150 s gesetzt, damit die Endkontrollen sichtbar bleiben.


Der eigenständige Build `Builds/WindowsReserve/divingsim.exe` wurde erfolgreich erstellt (`artifacts/package-reserve.log`). Beide Atemszenarien bestehen im Package mit denselben Ergebnissen wie im Editor. Eine unabhängige Auswertung der 119 gespeicherten Ausfall-Messpunkte bestätigt konstante Flaschenfüllung ab Sekunde 5, monoton sinkende Reserve, stabile Sättigung bis Sekunde 60 und den gespeicherten terminalen Zustand. Die endgültigen Reserve-/Abbruch-/Logbuchbilder wurden visuell geprüft; das späte Bild wird nicht mehr von der Testkarte verdeckt.

![Niedrige Atemreserve im eigenständigen Spiel](Images/Reserve_Low.png)

![Gespeicherter Verlauf mit verzögerter SpO2-Abnahme](Images/Reserve_Logbook.png)


Alle fünf Package-Abläufe bestehen: `breathing`, `reserve-loss` einschließlich aller vier Logbuchseiten, `shore` (Weesen: 5,75 m / 43,77 m / 40 Messpunkte), `tutorial` (acht Stufen in 12,94 s) und `audio`. Die gerenderte Tonaufnahme besteht ebenfalls: 40,81 s Stereo/48 kHz, aktiver RMS 0,01396, Spitzenpegel 0,1381; Pause und Stummschaltung liegen innerhalb der Prüfschwellen. Belege: `artifacts/reserve-package-*-full.log` und `reserve-audio-mix.log`. Dies ist weiterhin keine vollständige physiologische, akustische oder zweistündige Stabilitätsabnahme.

## Gerenderte Performance (25. November 2026)

Editor und eigenständiger Development-Build `Builds/WindowsPerformance/divingsim.exe` wurden erfolgreich gebaut (`artifacts/build-performance-final.log`, `package-performance.log`). Ein neuer expliziter Benchmark-Modus erfasst mit dem Unreal-CSV-Profiler tatsächliche CPU-/GPU-Bildzeiten, Prozessspeicher und Gelände-Kachelzahl. Native Auflösung, feste Qualitätsstufe und abgeschlossene Dateien werden geprüft; Screenshots entstehen nach der Messung. Normale Spielstarts aktivieren diesen Modus nicht.

Acht 60-Sekunden-Messungen nach jeweils 15 Sekunden Aufwärmen sind abgeschlossen: Unterwasser in 1080p/1440p/4K, Oberfläche in 1080p/4K, Ufer und Nachladen in 1080p sowie eine gezielte Wiederholung des 1080p-Unterwasserlaufs. Auf RTX 3090 / Ryzen 9 9950X3D ergeben sich bei Qualitätsstufe 2 ungefähr 127–139 FPS in den 1080p-Szenen, 98 FPS bei 1440p und 60 FPS bei 4K. Von 52.175 berücksichtigten Bildern überschreitet eines 33,333 ms (40,46 ms); der gezielte Wiederholungslauf reproduziert diesen Ausreißer nicht. Er bleibt in den Ergebnissen enthalten. Vollständige Methodik, Einzelwerte und Einschränkungen: [PERFORMANCE.md](PERFORMANCE.md), maschinenlesbare Ergebnisse und Prüfsummen: [PERFORMANCE_RESULTS.json](PERFORMANCE_RESULTS.json).

Die zehn portablen Python-Prüfungen bestehen (`artifacts/tests-performance.log`), darunter fünf Prüfungen des CSV-Lesers, der Perzentile und des Umgangs mit Rucklern/unvollständigen Daten. Beide normalen Package-Abläufe `breathing` und `shore` bestehen nach Integration des Messmodus (`artifacts/performance-breathing-full.log`, `performance-shore-full.log`). Die zusätzliche Messung verändert keine persönlichen Grafikvorgaben auf Datenträger und senkt für die gemeldeten Ergebnisse die Renderauflösung nicht ab. Mindesthardware, andere Einstiegspunkte/Wetterlagen, eine echte zweistündige Laufzeit und das macOS-Spiel bleiben ungeprüft.

## Gescannte Seegrund-, Fels- und Ufermaterialien (25. November 2026)

Der eigenständige Build `Builds/WindowsBed/divingsim.exe` enthält gemeinsame PBR-Materialien für Bathymetrie, Felsobjekte und die untergetauchten Uferergänzungen. Neun unveränderte 2K-Quellkarten aus drei CC0-Assets wurden anhand Größe, Anbieter-MD5 und SHA256 geprüft. Weltbezogene Projektion, Normalen, Rauheit und die Überblendung zum registrierten Orthobild ersetzen die bisherigen einfachen Farb-/Kiesmuster. Herkunft, Reproduktion und gerenderte Vergleichsbilder: [BED_SURFACES.md](BED_SURFACES.md).

Editor-Build, Materialerzeugung und endgültiges Windows-Package bestehen (`artifacts/build-bed-editor.log`, `build-bed-shore-content.log`, `package-bed-shore.log`). Der tatsächliche Uferablauf im endgültigen Package besteht mit 40 Messpunkten, maximal 5,74 m Tiefe und 43,76 m Strecke (`artifacts/bed-final-shore-full.log`). Nachtläufe mit ein- und ausgeschalteter Lampe bestehen ebenfalls. In der bildbasierten Prüfung sind 77.637 Pixel mit Lampe heller als 20/255; der mittlere maximale Farbkanal beträgt 7,593 gegenüber 0,0 ohne Lampe. Die Szene leuchtet ohne Lampe im geprüften Ausschnitt nicht selbst. Der Prüfausschnitt beginnt nun bei Y=160 statt 120, um die hinzugekommene Atemquellen-HUD-Zeile auszuschließen; Schwellenwerte bleiben unverändert (`artifacts/bed-final-lighting-frames.log`).

Vier abschließende Messungen erfassen jeweils 60 s nach 15 s Aufwärmen, Qualitätsstufe 2, native Auflösung, kein VSync oder FPS-Limit, RTX 3090 / Ryzen 9 9950X3D. Der bestehende Benutzer-Editor bleibt im Hintergrund geöffnet; dieser Zustand ist in den Metadaten enthalten.

| Szene | Auflösung | Mittel FPS | 1%-Low FPS | P95 ms | P99 ms | Maximum ms | Bilder >33,333 ms |
|---|---|---:|---:|---:|---:|---:|---:|
| Unterwasser | 1920×1080 | 123,34 | 93,40 | 9,30 | 10,29 | 12,44 | 0 / 7398 |
| Unterwasser | 3840×2160 | 56,71 | 46,22 | 19,51 | 21,23 | 22,40 | 0 / 3401 |
| Nachladen | 1920×1080 | 122,34 | 90,82 | 9,30 | 10,54 | 15,16 | 0 / 7339 |
| Ufer | 1920×1080 | 132,85 | 99,31 | 8,58 | 9,59 | 11,64 | 0 / 7969 |

Alle 26.107 berücksichtigten Bilder dieser vier Läufe bleiben unter 33,333 ms. Beim Nachladen sinkt die Kachelzahl für vier Bilder auf sieben/acht und erreicht wieder neun. Gegenüber den vergleichbaren vorherigen Läufen fällt die mittlere Bildrate um ungefähr 4–5 %. 4K erreicht damit im Mittel weniger als 60 FPS. Dies belegt den Aufwand der verbesserten Oberfläche auf diesem Rechner; es ist keine universelle Leistungs- oder Langzeitgarantie. Rohdatenreferenzen, Einzelwerte und SHA256 von Bildern, Logs, CSV, Executable sowie allen ausgelieferten Pak-/IoStore-Dateien stehen in [BED_PERFORMANCE.json](BED_PERFORMANCE.json). Auch Content-Änderungen ohne geändertes Executable sind so unterscheidbar.

Die vollständigen 236 nummerierten Anforderungen wurden zusätzlich aus der angemeldeten Originalansicht übernommen und gegen deren Zeilen-Prüfsumme abgeglichen: [REQUIREMENTS.csv](REQUIREMENTS.csv), ergänzende Vorgaben in [REQUIREMENTS_CONTEXT.md](REQUIREMENTS_CONTEXT.md). Diese Erfassung und die Materialverbesserung markieren keine vollständige Anforderung als abgenommen. Lokale Sedimentverteilung, feinere Geometrie, visuelle Referenzqualität, Mindesthardware, macOS-Spiel und zweistündige Stabilität bleiben offen.

## Zwischenstände und Wiederherstellung nach Prozessabbruch (25. November 2026)

Die bereits vorhandene Sicherung alle 30 Sekunden Simulationszeit ist jetzt gegen einen beschädigten oder fehlenden Hauptstand abgesichert. Vor dem Ersetzen einer gültigen `.json` wird ihr Inhalt separat in `.json.bak` gesichert. Neue Daten werden geprüft, in eine temporäre Datei geschrieben und zurückgelesen. Kann die Vorgängerversion nicht gesichert werden, bleibt die Hauptdatei erhalten. Eine beschädigte Hauptdatei überschreibt keine gesunde Sicherung. Diese zweite Generation ist nötig, weil Unreals generische Dateioperation `Move` ein vorhandenes Ziel vor dem Umbenennen löscht; dieser Aufruf allein ist keine atomare Ersetzung.

Das Logbuch entdeckt auch Sicherungen ohne Hauptdatei, erzeugt aus Hauptdatei/Sicherung keine doppelten Einträge und lädt bei einer unlesbaren Hauptdatei die geprüfte Vorgängerversion. Es zeigt deren tatsächliche Dauer/Messpunkte und einen deutschen oder englischen Hinweis auf möglicherweise fehlende neuere Daten. Schreibfehler beim laufenden Tauchgang werden nun gemeldet statt ignoriert. Die Schema-Version bleibt 4; Versionen 1–3 bleiben lesbar.

Vier Unreal-Automationstests bestehen (`artifacts/test-log-recovery-final.log`). Die neue Prüfung deckt zwei aufeinanderfolgende Sicherungen, Ablehnung ungültiger Ersatzdaten, beschädigte/fehlende Hauptdatei, Auffinden einer alleinigen Sicherung, Reparatur ohne Überschreiben einer gesunden Sicherung sowie einen blockierten Sicherungs-Schreibpfad ab. Temporäre Dateien werden nicht als Tauchgänge gelesen. Der bestehende synthetische Zwei-Stunden-Datensatz bleibt lesbar; das ist weiterhin kein zweistündiger Laufzeittest.

Editor und eigenständiges Package `Builds/WindowsRecovery/divingsim.exe` wurden erfolgreich gebaut (`artifacts/build-log-recovery-final.log`, `package-log-recovery-final.log`). `Tools/TestCheckpointRecovery.ps1` startet eine eigene Spielinstanz und beendet genau diesen Prozess hart, sobald zwei abgeschlossene Zwischenstände auf Datenträger vorliegen. Kein `EndPlay` oder normaler Tauchabschluss wird dafür ausgeführt. Danach öffnen zwei neue Spielprozesse den Originalstand und eine isolierte beschädigte Kopie mit gesunder Sicherung:

| Fall | Geladene Dauer | Messpunkte | Beobachtung |
|---|---:|---:|---|
| Hauptstand nach harter Beendigung | 60,020 s | 60 | Nicht abgeschlossen; alle vier Kurvenseiten gerendert |
| Beschädigte Testkopie, Sicherung gültig | 30,020 s | 30 | Älterer Stand gekennzeichnet; alle vier Kurvenseiten gerendert |
| Normaler Ufer-Tauchabschluss | 39,680 s | 40 | Status `completed`; Sicherung mit 30 Messpunkten bleibt erhalten |

Der erste Integrationslauf fand einen Pfadvergleichsfehler: Eine absolute Dateiangabe traf nicht auf den relativen Pfad desselben Logbucheintrags. Auswahl und Prüfung vergleichen jetzt normalisierte Pfade. Der vollständige Absturz-/Wiederöffnungsablauf besteht anschließend mit dem neu gebauten Package (`artifacts/checkpoint-20261125-093019`). Die Originaldatei bleibt beim Öffnen nachweislich unverändert. Deutsche und englische Wiederherstellungsansicht wurden visuell geprüft. Der normale Uferablauf besteht mit maximal 5,74 m Tiefe und 43,76 m Strecke (`artifacts/recovery-shore-full.log`). Ergebnisse, Executable- und Artefakt-Prüfsummen: [CHECKPOINT_RECOVERY.json](CHECKPOINT_RECOVERY.json).

![Zwischenstand nach harter Prozessbeendigung](Images/Checkpoint_Primary.png)

![Älterer Sicherungsstand mit sichtbarem Wiederherstellungshinweis](Images/Checkpoint_Recovery.png)

Diese Funktion bewahrt Tauchprotokolle; sie setzt keine laufende Simulation fort. Vor der ersten Sicherung und seit der letzten erfolgreichen Sicherung können Messpunkte fehlen. Geprüft wurden Prozessbeendigung und gezielt beschädigte/fehlende Dateien, kein Stromausfall oder defekter Datenträger. Die Speicherung arbeitet derzeit synchron; ihr Einfluss bei langen Tauchgängen sowie die vollständige zweistündige Stabilität müssen noch gemessen und gegebenenfalls verbessert werden.

## Hintergrundspeicherung für lange Protokolle (25. November 2026)

Die nachfolgende Messung bestätigt den Engpass des vorigen Stands: Das Prüfen und Ersetzen einer bereits vorhandenen Datei mit 7.201 synthetischen Messpunkten dauert auf diesem Rechner 294,874 ms, ohne die vorherige JSON-Erzeugung. Der Build `Builds/WindowsBackgroundLogs/divingsim.exe` verlagert deshalb JSON-Erzeugung, Validierung, Sicherung und Dateizugriffe laufender Zwischenstände in einen Threadpool-Auftrag. Im Spielthread bleiben die Kopie der aufgezeichneten Zustände und die Übergabe. Der Auftrag enthält keine Actor-/World-Referenzen; Konfiguration, Koordinatenursprung und Messpunkte gehören der Kopie.

Ein laufender Auftrag und höchstens ein wartender aktueller Stand begrenzen den Speicherbedarf bei langsamem Datenträger. Neuere Zwischenstände ersetzen einen noch wartenden Zwischenstand. Beim Abschluss/regulären Verlassen wird ein laufender Auftrag abgewartet und anschließend der endgültige Stand vollständig geschrieben; ein älterer Auftrag kann ihn dadurch nicht später überschreiben. Die bisherige Sicherungs-/Wiederherstellungslogik und alle 52 geschriebenen JSON-Feldnamen bleiben erhalten. Schema-Version und Lesbarkeit älterer Protokolle ändern sich nicht. Fehler abgeschlossener Hintergrundaufträge werden beim nächsten Spieltick gemeldet.

Sechs Unreal-Tests bestehen (`artifacts/test-background-log-final.log`). Hinzugekommen sind die Kostenmessung sowie Prüfungen für eine unabhängige Datenkopie, LV95-Koordinaten, eine begrenzte Warteschlange, den Vorrang des Abschlusses vor alten Schreibaufträgen und Fehlerrückmeldung mit anschließendem erneutem Speichern. Die gezielt erzeugte Warnung bei einem ungültigen Messwert wird genau einmal erwartet. Alle 7.201 Messpunkte bleiben beim erneuten Lesen erhalten.

| Messung mit synthetischem Zwei-Stunden-Datensatz | Gemessene Dauer |
|---|---:|
| Vorheriger synchroner Prüf-/Schreibpfad, ohne JSON-Erzeugung | 294,874 ms |
| Datenkopie und Übergabe an Hintergrundauftrag | 0,132 ms |
| Erste vollständige Dateierzeugung im Hintergrund | 311,154 ms |
| Nachfolgender Hintergrundstand mit Sicherung | 480,330 ms |

Die Arbeit entfällt nicht; sie läuft außerhalb des Spielthreads. Die Tabelle ist eine isolierte Kostenmessung, keine zweistündige Spiel- oder FPS-Messung. Der endgültige Abschluss kann weiterhin auf Speicherung warten (im synthetischen Test 476,376 ms für das Schreiben selbst).

Editor und Windows-Package wurden erfolgreich gebaut (`artifacts/build-background-log-final.log`, `package-background-log.log`). Der echte Prozessabbruch-/Wiederöffnungsablauf besteht erneut: 60,020 s / 60 Messpunkte aus der Hauptdatei und 30,020 s / 30 Messpunkte aus der Sicherung einer beschädigten Testkopie; alle vier Logbuchseiten werden in neuen Prozessen gelesen und gerendert (`artifacts/checkpoint-20261125-094450`). Der Wiederherstellungshinweis wurde visuell geprüft. Auch der normale Uferablauf besteht mit 40 Messpunkten, 5,74 m maximaler Tiefe und 43,76 m Strecke (`artifacts/background-shore-full.log`). Seine Zwischenstand-Übergabe benötigt 0,031 ms; die anschließenden 12,199 ms Schreibarbeit laufen separat. Der Abschluss wird vor Rückmeldung vollständig gespeichert.

Zwei gerenderte 60-Sekunden-Läufe nach je 15 s Aufwärmen, native 1080p/Qualitätsstufe 2 auf RTX 3090 / Ryzen 9 9950X3D:

| Lauf | Mittel FPS | 1%-Low FPS | P95 ms | P99 ms | Maximum ms | Bilder >33,333 ms |
|---|---:|---:|---:|---:|---:|---:|
| Erster Lauf | 117,83 | 61,35 | 10,08 | 15,24 | 18,91 | 0 / 7068 |
| Gezielte Wiederholung | 120,45 | 91,33 | 9,73 | 10,54 | 12,04 | 0 / 7225 |

Im ersten Lauf liegen erhöhte GPU-/Renderzeiten unter anderem bei Simulationssekunden 27,8–28,4 vor. In den Zeitfenstern ±0,1 s um die Sicherungen bei 30,02 und 60,02 s bleibt die Spielthread-Zeit unter 2,75 beziehungsweise 2,41 ms. Diese Zähler beweisen keine eindeutige Ursache der GPU-Schwankung. Der erste Lauf wird nicht durch den günstigeren Wiederholungslauf ersetzt. Alle 14.293 Bilder bleiben unter 33,333 ms; eine universelle Leistungszusage folgt daraus nicht. Die gerenderten Läufe enthalten normale kurze Tauchprotokolle, nicht den synthetischen Zwei-Stunden-Datensatz.

Einzelwerte, Rohdatenreferenzen, Wiederherstellungsdaten und SHA256-Prüfsummen: [BACKGROUND_LOG_RESULTS.json](BACKGROUND_LOG_RESULTS.json). Die tatsächliche zweistündige Spielstabilität, lange Protokolle während des Renderns und die Prüfung auf Mindesthardware bleiben offen.

## Direkte Konfigurationseingaben (25. November 2026)

Die bisher festen Schaltfolgen für Startdruck, Blei, geplante Tiefe und Dauer erfüllten die freie Eingabe aus CONF-009/013/017/018 nicht. Vierzehn Zahlenfelder öffnen jetzt einen gemeinsamen Eingabedialog. Er zeigt den jeweiligen Bereich, akzeptiert Dezimalpunkt oder Komma und übernimmt nur gültige Werte; falsche Zahlen, Einheiten im Text, überschüssige Nachkommastellen und Grenzüberschreitungen verändern die Konfiguration nicht. CONF-002 erhält die minutengenaue Eingabe von 00:00 bis 23:59. Bei laufendem Tauchgang sind diese Felder gesperrt. Konfiguration und Profilvorschau zeigen Bruchteile und Minuten; das bestehende Speicherformat bleibt erhalten.

Die engineunabhängigen Prüfungen bestehen einschließlich aller Feldgrenzen, ungültiger Zeitformate, Dezimalkomma, negativer Lufttemperatur und unveränderter Werte nach Fehlern. Drei Unreal-Profiltests bestehen mit 213,5 bar, 7,5 kg, 22,5 m, 47 min und 23:59 Uhr auch nach Speicherung, erneutem Laden und Wiederherstellung. Editor- und Package-Build sind erfolgreich (`artifacts/build-config-editor-final-ui.log`, `package-config-final-ui.log`).

Das Paket `Builds/WindowsConfig` besteht sieben gerenderte Konfigurationsphasen bei 1600×900 und 30-FPS-Limit: alle 14 Feldkennungen, Eingabe, Bereichsfehler, Uhrzeitfehler, Persistenz, Abbrechen und Sperre bei aktivem Tauchgang. Enter für ungültige und gültige Werte sowie Escape zum Verwerfen werden durch Slates tatsächliche Tastatur-Ereignisverarbeitung geschickt. Die Prüfung bestätigt außerdem, dass der gestartete Simulationskern die gewählten Werte erhält. Die bestehende Profiloberfläche besteht alle elf Phasen und den anschließenden kurzen Tauchgang erneut. Sechs Konfigurationsaufnahmen und der gemeinsame Profilnamendialog wurden visuell geprüft; lange Hinweise umbrechen innerhalb des Dialogs.

![Direkte Eingabe eines Flaschendrucks](Images/ConfigPressure_DE.png)

![Minutengenaue Uhrzeit und frei eingegebene Umweltwerte](Images/ConfigValues_EN.png)

Artefakte und SHA256-Prüfsummen: [CONFIG_INPUT_RESULTS.json](CONFIG_INPUT_RESULTS.json). Automatische Umweltwerte, Sturm und wählbare Lampenmodelle bleiben außerhalb dieser Änderung offen. Die parallel laufende Zweistundenprüfung verwendet unverändert `WindowsSoak`; zusätzliche Build- und Dialogtestlast ist mit Uhrzeiten dokumentiert, siehe [SOAK.md](SOAK.md). Ihr Abschluss ist hier noch nicht nachgewiesen.

## Sturm als viertes Wetterszenario (25. November 2026)

CONF-004 erhält die Auswahl Sturm/Storm. Die Wetterwahl setzt Wind auf 3/5/8/22 m/s; danach bleiben manuelle Werte zwischen 0 und 24 m/s erhalten. Die bisherigen Wetterkennungen 0–2 bleiben kompatibel. Wolken, direkte/diffuse Beleuchtung, Luftnebel, Regenmenge und Audioparameter beziehen ihre Werte aus einer gemeinsamen Szenariotabelle. Unterwasser-Sichtweite bleibt davon unabhängig. Wellenlängen wachsen oberhalb 12 m/s mit der Amplitude, sodass die Einzelkomponenten nicht steiler werden; die Rendergrenzen decken den maximalen Ausschlag ab. Das bleibt ein qualitatives Spielmodell, kein gemessenes Walensee-Sturmspektrum.

Kernprüfungen bestehen für starke Windwerte, Wellenableitungen, Dispersion, Druckabfall in der Tiefe, Luft/Wasser-Optik und Audioparameter. Drei Unreal-Profiltests prüfen zusätzlich alle vier Wetterkennungen, gespeicherte Windvorgaben, nachträgliche 21,5 m/s und Ablehnung unbekannter Kennungen. Die sieben gerenderten Konfigurationsphasen bestätigen Auswahl bis Sturm, tatsächliche Speicherung/erneutes Laden und Übernahme des manuellen Windwerts in den gestarteten Simulationskern. Editor- und Windows-Package-Build sind erfolgreich.

Im separaten Package `Builds/WindowsStorm` besteht der Oberflächenablauf bei Murg mit 22 m/s: Luft-, Übergangs- und Unterwasserphase, 38 s Tauchdauer, 39 Messpunkten, 11,49 m maximaler Tiefe und 44,78 m Strecke. Die Beobachtung über Wasser bestätigt 2.021 Regeninstanzen, Wolkendeckungsparameter 0,90, Dichte 0,018 und 9.272,9 Lux direkte Lichtquelle. Die konservative Summe der Wellenamplituden beträgt 2,269 m; dies ist keine signifikante Wellenhöhe. Screenshots prüfen Oberfläche, Unterwasseransicht und Konfiguration. Der normale Regenablauf besteht im selben Package als Regression. Alle gerenderten Tests sind auf 30 FPS begrenzt und laufen neben dem separat protokollierten Dauertest; sie sind keine Leistungsmessung.

Das Wassermaterial filtert die feinen prozeduralen Normalen anhand ihrer Pixel-Fußabdrücke. Der Vorher/Nachher-Vergleich zeigt dennoch deutliche breitere Streifen/Bänder an der bewegten Oberfläche. Diese Darstellungsfehler sind **nicht behoben**; die Grafik ist noch nicht abgenommen. Brechende Wellen, Schaum, Gischt sowie Blitz/Donner fehlen. Die stärkeren Sturm-Audioparameter sind im Kern geprüft; eine eigene Aufnahme und Hörabnahme des fertigen Sturmmixes steht aus. Die konfigurierte Tageszeit schreitet weiterhin nicht fort.

![Sturm im aktuellen Entwicklungsstand mit sichtbaren Wasserbändern](Images/Storm_Surface.png)

![Sturmkonfiguration mit manuell angepasstem Wind](Images/Storm_Menu.png)

Messwerte, Artefakte und SHA256-Prüfsummen: [STORM_RESULTS.json](STORM_RESULTS.json). Der unveränderte zweistündige Lauf von `WindowsSoak` ist zum Zeitpunkt dieser Änderung noch aktiv und noch nicht als bestanden gewertet.

## Wasserreflexionen und Feinwellen (25. November 2026)

Die Sturmaufnahmen machten zwei unterschiedliche Oberflächenprobleme sichtbar: regelmäßige kammartige Rippel und ein körniges Raster in der Spiegelung. Zwei isolierte Läufe des unveränderten `WindowsStorm`-Packages deaktivierten Reflexionen beziehungsweise verwendeten ausschließlich Himmel/Cubemaps. Die Bänder verschwanden ohne Reflexionen, blieben bei Cubemaps jedoch bestehen. Ein eigener `WindowsWater`-Build ohne Feinnormalen glättete die kammartige Kontur; die großen Wellen blieben erhalten. Damit ließen sich die Feinwellen von der großskaligen Geometrie und von der Szenen-Spiegelung getrennt beurteilen. Die Diagnosevarianten sind keine empfohlenen Spieleinstellungen.

Das Wassermaterial verwendet nun drei bewegte, unterschiedlich gedrehte Rauschgradienten mit glatten Zellübergängen. Sie ersetzen das weltweit wiederholte Muster aus vier Sinusfunktionen. Windrichtung und Simulationszeit steuern ihre Bewegung; bei Windstille ist ihr Beitrag null. Die bereits vorhandene Filterung nach Pixelgröße bleibt erhalten. Geometrische Wellen, Druck, Auftrieb und Wasserbewegung ändern sich dadurch nicht. Ein separater Vergleich mit Unreal 5.8s `r.Water.SingleLayer.Reflection.Denoising=1` reduziert das sichtbare körnige Raster; dieser Filter ist jetzt standardmäßig aktiviert. Szenenreflexionen bleiben eingeschaltet. Grundlage des Renderpfads: [Epic Single Layer Water](https://dev.epicgames.com/documentation/en-us/unreal-engine/single-layer-water-shading-model-in-unreal-engine); die genaue Konsolenvariable und ihre Wirkung sind zusätzlich im installierten Engine-Quelltext `SingleLayerWaterRendering.cpp` überprüft.

Der finale Content-Build, die anschließende Paketierung und die drei gerenderten Oberflächenabläufe für Sturm, Windstille am Tag und Windstille um 23 Uhr bestehen. Alle drei Laufzeitprotokolle bestätigen die Reflexionsfilterung aus der Projektkonfiguration. Der erste reguläre Paketbau scheiterte beim erneuten Linken an der inzwischen durch Explorer geöffneten Projekt-Spielinstanz; diese bleibt unberührt. Da ausschließlich Material und Konfiguration geändert wurden, verwendet die anschließende UAT-Paketierung `-skipbuild`. Vorher wurde die bestehende Programmdatei per SHA256 mit dem bereits getesteten Sturm-Package verglichen; beide sind identisch. Der fehlgeschlagene Lauf bleibt als Artefakt erhalten. Die Tests nutzen 1600×900 mit 30-FPS-Limit und prüfen Luft-/Wasserübergang, Materialien, Licht, Speicherung und regulären Abschluss. Der parallel laufende Zweistundentest ist unverändert; zusätzliche Last steht in seinem `DevelopmentActivity.jsonl`. Daraus folgt keine ungestörte FPS-Messung. Vergleichsbilder stammen aus demselben geskripteten Ablauf, sind aber durch leicht unterschiedliche Aufnahmezeitpunkte keine pixelgenau ausgerichteten Bildpaare.

Die Änderung ist eine sichtbare Verbesserung der Feinstruktur, keine vollständige Wasserabnahme. Dunkle Bänder bei flachem Blickwinkel bleiben erkennbar; Schaum, Gischt, brechende Wellen, Uferwechselwirkungen und die Referenzabnahme sind weiter offen. Rohdaten, Diagnoseläufe und Prüfsummen: [WATER_RENDER_RESULTS.json](WATER_RENDER_RESULTS.json).

![Sturm mit unregelmäßigen Feinwellen und gefilterten Reflexionen; dunkle Bänder bleiben sichtbar](Images/WaterReflections_Storm.png)

![Windstille mit erhaltener Landschaftsspiegelung](Images/WaterReflections_Calm.png)

## Fortlaufende Tageszeit und unabhängiger Programmbau (25. November 2026)

ENV-001 war bisher trotz auswählbarer Tag-/Nachtbeleuchtung unvollständig: Die Sonne behielt den beim Start gewählten Stand. Jetzt ergibt sich die Ortszeit aus Startuhrzeit plus tatsächlich verstrichener Simulationszeit, im Verhältnis 1:1 und mit Mitternachtswechsel. Sonne, Atmosphärenlicht, Unterwasser-Umgebungslicht und Belichtung folgen dieser gemeinsamen Zeit. Die Kamera fragt den aktuellen Wert direkt ab, sodass ihre Aktualisierung vor dem World-Tick keinen veralteten Lichtwert nutzt. Das HUD zeigt zusätzlich die aktuelle Stunde/Minute. Eine Pause hält die Simulationszeit und damit den Sonnenstand an; auch ein abgeschlossener Tauchgang lässt die Uhr stehen. Gespeicherte Startuhrzeit und Tauchdauer bleiben unverändert im bestehenden Logformat erhalten.

Kernprüfungen bestehen für den Zweistundenfortschritt, Mitternacht, mehrere Tage, große/ungültige Werte und das Überschreiten von Sonnenaufgang/-untergang. Editor und Windows-Development-Package wurden neu kompiliert. Drei echte 90-Sekunden-Tauchgänge starten um 05:59, 17:59 und 23:59. Jeder pausiert für mindestens vier Sekunden Engine-Laufzeit; die Simulationssekunden und Ortszeit bleiben dabei exakt unverändert. Anschließend werden die Stundengrenze, Bilder vor/nach dem Wechsel, 91 gespeicherte Messpunkte und der reguläre Abschluss geprüft. Die Laufzeitprüfung kontrolliert zudem tatsächliche Intensität und Rotation des Unreal-Sonnenlichts. Es gibt keine beschleunigte Uhr und keine künstlich gesetzten Simulationssprünge. Alle drei Abläufe sowie die Sturmregression bestehen bei 1600×900 und 30-FPS-Limit. Sechs Zeitwechsel-Aufnahmen wurden visuell geprüft.

Die ursprüngliche Lichtnäherung bleibt bewusst sichtbar dokumentiert: Sonnenaufgang liegt stets bei 06:00, Sonnenuntergang bei 18:00. Datum, Jahreszeit und tatsächlicher astronomischer Sonnenstand sind noch nicht umgesetzt. Nach Sonnenuntergang ist die Landschaft fast schwarz; plausible Dämmerung, Mond-/Sternenlicht und die optische Gesamtabnahme bleiben offen. Der fortschreitende Zyklus ist damit implementiert, die realistische Himmelsbeleuchtung noch nicht vollständig.

Eine vom Nutzer über Explorer geöffnete Projekt-Spielinstanz sperrte zuvor die normale Entwicklungsdatei. Der optionale Build-Parameter `-BuildSlot DayCycle` erzeugt deshalb `divingsim-DayCycle.exe` im gleichen Binärverzeichnis. Engine-ABI und Zielplattform bleiben unverändert; UAT verpackt die neue Datei über seinen Build-Receipt. Der Paketstarter bleibt `divingsim.exe`; ein zusätzlicher 22-Sekunden-Tauchtest über genau diesen Starter bestätigt den Aufruf von `divingsim-DayCycle.exe` und den gespeicherten Abschluss. Smoke-, Benchmark-, Recovery- und künftige Soak-Aufrufe wählen die im UAT-Manifest verzeichnete Programmdatei, statt eine eventuell ältere Standarddatei zu starten. Das alte Entwicklungsprogramm blieb geöffnet und seine SHA256-Prüfsumme unverändert. Das ist kein Überspringen der C++-Kompilierung. Der laufende zweistündige Test verwendet weiterhin sein unverändertes älteres Package.

Ergebnisse und Prüfsummen: [DAY_CYCLE_RESULTS.json](DAY_CYCLE_RESULTS.json). Zusätzliche Build-/Testlast ist zeitlich im Dauertest protokolliert; diese Abläufe sind keine ungestörte Leistungsmessung.

![Fortschreitende Ortszeit kurz vor dem modellierten Sonnenuntergang](Images/DayCycle_Sunset.png)

![Mitternachtswechsel ohne Rücksetzen der Tauchdauer](Images/DayCycle_Midnight.png)


## Atmosphärische Dämmerung und begrenzte Belichtung (25.11.2026)

Das Abschalten der Sonne am Horizont ließ die Landschaft unmittelbar nach 18:00 schwarz werden. Die Richtungslichtquelle bleibt nun oberhalb der Atmosphäre mit 80.000 Lux und dem Wetterfaktor aktiv; Unreal berechnet atmosphärische Abschwächung, Färbung und Erdschatten. Eine stetige, ausdrücklich angenäherte diffuse Lichtkurve führt das Unterwasserlicht durch die Dämmerung. Die Kamera gleicht Helligkeitsverlust nur teilweise aus, damit Abendlicht und Tiefe sichtbar dunkler werden. Die Dunkelgrenze bleibt bei 5,5 EV Belichtungskorrektur.

Der getrennte Build `Builds/WindowsTwilightExposure` wurde als Editor und Windows-Development-Paket erfolgreich gebaut. Sieben gerenderte Läufe in Murg bei 05:45, 12:00, 17:45, 18:00, 18:06, 18:30 und 23:00 bestanden die Gameplay-, Luft-/Wasser-, Licht-, Material-, Gelände- und Speicherprüfungen. Je Lauf wurden drei frische PNGs mit 1600 × 900 Pixeln gesichert. Die Oberflächenbilder aller sieben Zeitpunkte wurden visuell geprüft. Alle Läufe verwendeten 30 FPS Begrenzung und eine ausgeschaltete Lampe. Zusätzliche Tests bestanden für den 90-sekündigen Sonnenuntergang mit vier Sekunden echter Menüpause, Sturm bei 22 m/s sowie Nachttauchen mit eingeschalteter 1.000-Lumen-Lampe. Die Aufnahme des Nachttests blickt überwiegend in offenes dunkles Wasser; sie ersetzt keine optische Abnahme eines realen Lampenmodells.

[Vorher um 18:06](Images/Twilight_Before.png) und [nach der Korrektur](Images/Twilight_After.png) zeigen dieselbe Testansicht. Im HUD-freien Geländeausschnitt `(1000,180)–(1500,320)` lag der mittlere kodierte PNG-RGB-Wert zuvor bei `(0,0,0)`, mit zunächst überstarker Belichtung bei `(148,129,139)` und im abgestimmten Ergebnis bei `(40,31,35)`. Das sind Bildvergleichswerte, keine photometrische Messung. Die Sonnenuntergangsaufnahmen vor/nach dem Stundenwechsel zeigen durchgehend Abendlicht statt eines Sprungs ins Schwarz.

Die portablen Modellprüfungen decken kontinuierliches, monoton abnehmendes Dämmerlicht, identische Morgen-/Abendnäherung, Tiefenabschwächung, endliche Belichtung sowie die unveränderte Dunkelgrenze ab. [TWILIGHT_RESULTS.json](TWILIGHT_RESULTS.json) enthält Zeitpunkte, Laufzeitmarker, Protokollreferenzen und SHA-256-Prüfsummen. Auch der Start über `Builds/WindowsTwilightExposure/divingsim.exe` wurde separat bis zum erfolgreichen 22-Sekunden-Spieltest mit 23 gespeicherten Messpunkten geprüft; der Starter lud nachweislich `divingsim-TwilightExposure.exe`. Das ursprüngliche, vom Benutzer gestartete Programm blieb geöffnet; seine EXE-Prüfsumme blieb unverändert.

Die feste 06:00-/18:00-Sonnenbahn, fehlende Jahreszeiten und fehlender Mond-/Sternenhimmel bleiben Einschränkungen. Späte Dämmerung und unbeleuchtete Nacht sind weiterhin fast schwarz; breite Reflexionsbänder auf dem Wasser sind noch sichtbar. Die Änderungen verbessern ENV-001/ENV-002, schließen aber weder die optische Gesamtabnahme noch die vollständige Spezifikation ab. Parallel liefen der unveränderte Zweistundentest einer älteren Version, Editor und Benutzerspiel; diese Bildprüfungen sind deshalb kein Leistungsnachweis. Die Aktivitäten sind im Dauertestprotokoll vermerkt.
