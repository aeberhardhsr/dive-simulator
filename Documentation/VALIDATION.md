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
