# Kalenderzeit und astronomischer Himmel

Das Szenariodatum und die Startuhrzeit bestimmen den Sonnen-/Mondstand und Sternenhimmel am Walensee. Die frühere feste 06:00-/18:00-Bahn entfällt. `ACelestialVaultDaySequenceActor` aus dem installierten Unreal 5.8.4 berechnet die Himmelskörper; die Umgebung übernimmt seine Licht-, Himmels-, Wolken- und Nebelkomponenten.

## Uhr und Eingabe

- Datum als `YYYY-MM-DD`, Bereich 2000–2099; Vorgabe 2026-06-21 für bisherige Sommerprofile.
- Schweizer CET/CEST-Regel: letzter Sonntag im März bzw. Oktober, Wechsel um 01:00 UTC. Die aktuelle Regel wird auch für zukünftige Szenarien angenommen; Gesetzesänderungen werden nicht vorhergesagt.
- Die im Frühjahr nicht existierende Uhrzeit wird im Dialog abgewiesen, ohne Werte zu verändern. Bei doppelter Herbstzeit bezeichnet die Eingabe die erste Stunde (CEST). Das steht im Datumdialog.
- UTC läuft mit den Simulationssekunden monoton weiter. Pause und beendete Tauchgänge halten sie an. Ortszeit und Datum werden daraus abgeleitet; Schaltjahre und Tages-/Jahreswechsel sind enthalten. Schaltsekunden werden nicht modelliert.
- Datum wird in Einstellungen, Profilen und Log-Umgebung als zusätzliches `scenario_date` gespeichert. Alte Dateien bleiben lesbar. Logs unterscheiden weiterhin den tatsächlichen Aufzeichnungszeitpunkt `started_at_utc` von der Szenariozeit. Szenario-Zeitzone, Start-UTC-Sekunden und Anfangsoffset werden ebenfalls gespeichert.

Die Himmelssequenz ist pausiert und wird explizit in UTC gestellt. Die plugininterne Sommerzeit bleibt ausgeschaltet. Bei Wechsel des UTC-Datums wird die native Ephemeriden-/Sternkonfiguration erneuert. Der Adapter begrenzt die an Unreal übergebene Float-Uhrzeit auf den letzten darstellbaren Wert unter 24 Stunden: Sonst rundet ein Double-Zeitpunkt unmittelbar vor Mitternacht auf 24, das native `SetTimeOfDay` macht daraus 0 bei noch altem Datum und erzeugt einen Tagesfehler. Diese Begrenzung lässt die logische UTC unverändert und verursacht weniger als 7 ms Rundungsabweichung am Tagesende. Der ursprüngliche 90-Sekunden-Grenztest reproduzierte diesen Fehler und bleibt Teil der Regression. Kamera, Unterwasserlicht und Nebel verwenden denselben Sonnenstand. Die Vorlage animiert auch Wolkenparameter; nach ihrer Auswertung werden die ausdrücklich gewählten Wetterwerte wieder angewendet.

## Bezug und Licht

Der feste Kartenursprung LV95 E2726000 / N1219000 entspricht WGS84 47.1100024957° N / 9.0988184839° E. PROJ/pyproj transformiert EPSG:2056 nach EPSG:4326; das geodätische Azimut zum 1 km nördlichen Rasterpunkt beträgt 1.212471°. Damit wird die native Ost-/Süd-Achse auf die Nord-/Ost-Achse der Karte gedreht. Der lokale Kartenkompass zeigt weiterhin **Gitternord**.

Der native Tangentialbezug verwendet 0 m Ellipsoidhöhe. Die 419 m LN02 des Wasserspiegels werden nicht fälschlich als Ellipsoidhöhe ausgegeben. Die Himmelsberechnung bezieht sich auf den festen Ursprung; es gibt noch keine fortlaufende geodätische Umrechnung beim Schwimmen durch den See.

Die Sonne bleibt als Quelle oberhalb der Atmosphäre bei 80.000 Lux mal Wetterfaktor. Sky Atmosphere berechnet Erdschatten und atmosphärische Streuung. Der Mond verwendet maximal 0,1 Lux vor Wetterabschwächung. Die native 5.8.4-Vorlage leitet seine Phase nur aus der mittleren synodischen Monatslänge ab; der Laufzeittest zeigte dadurch bis zu 11,536 Prozentpunkte Fehler. Der Adapter berechnet deshalb den Phasenwinkel aus den tatsächlichen ephemeridischen Sonnen-/Mondpositionen und dem Beobachter. Die Geometrie wird einmal pro Simulationssekunde und sofort nach einer Konfigurationsänderung erneuert; der native Material-/Lichtparameter wird nach jeder Sequenzauswertung damit gesetzt. Sein oberhalb des Horizonts liegender Anteil fließt in die vereinfachte Wasserstreuung ein. Eine zusätzliche, begrenzte Dunkeladaption zwischen −6° und −18° Sonnenhöhe macht die physisch schwachen Himmelskörper sichtbar; bei eingeschalteter Lampe wird sie begrenzt. Die Kamera nähert sich der Zielbelichtung allmählich.

Dies ist eine gestalterische Augen-/Wasserlichtantwort, keine gemessene Walensee-Radiometrie oder physiologische Dunkeladaptionssimulation. Der Sky-Light-Multiplikator bleibt 3. Mondoberfläche/Bloom, Sternsichtbarkeit bei Mondlicht, Lichtverschmutzung, Uferlichter und reale Lampencharakteristik sind noch nicht vollständig abgestimmt. Die verwendete Engine-Erweiterung ist als Beta gekennzeichnet. Im ersten Package fehlte der vom nativen Konstruktor verwendete alte Materialpfad: Er bezeichnet eine Editor-Assetweiterleitung. Die Sternkomponente fiel dadurch auf das Planetenatlas-Material des gemeinsamen Meshes zurück; riesige Bildflächen verdeckten den Himmel. Der Adapter lädt ausdrücklich den kanonischen Pfad `MI_Stars_EnergyConservative`, weist ihn nach der Sequenzauswertung zu und prüft die Zuordnung zur Laufzeit. Die native Konstruktorreferenz erzeugt weiterhin eine bekannte Startmeldung über das fehlende alte Asset; der Adapter behebt die tatsächlich verwendete Materialzuordnung anschließend, ohne die installierte Engine zu verändern.

## Prüfung

Unabhängige Referenzen: [CELESTIAL_REFERENCES.json](CELESTIAL_REFERENCES.json) enthält die NASA/JPL-Horizons-Abfrage und sechs DE441-Vergleichszeitpunkte ohne atmosphärische Refraktion. Geprüft werden tatsächliche Richtungen der gerenderten Sonnen-/Mondlichter; die Mondlichtintensität prüft außerdem den angewendeten Phasenanteil. Toleranzen: 0,05° Sonne, 0,15° Mond und 0,2 Prozentpunkte Beleuchtungsanteil. Der native Mond wird auf eine 380.000-km-Hülle projiziert, wodurch seine Parallaxe angenähert ist. Diese Stichproben sind kein Fehlernachweis für den gesamten Jahresbereich.

Die portablen Tests prüfen Gregorianik über ein Jahrhundert, Schaltjahr-/Jahreswechsel, Sommerzeitlücke, doppelte Herbststunde, monotone UTC und den Lichtabfall. Die drei Unreal-Profiltests prüfen auch alte Profile, ungültige Daten, atomare Ablehnung und Wiederherstellung. Die Menüprüfung bedient den echten Textdialog, speichert/lädt das Datum und testet beide Wege zur ungültigen Frühjahrszeit (Datum oder Uhrzeit ändern).

Am 25.11.2026 bestand das getrennte Windows-Development-Paket `Builds/WindowsCelestial` alle **13 gerenderten Szenarien**: Sommer-/Wintermittag, Sommerdämmerung, beide Nachtansichten, Datumseingabe, Profile, Frühjahrs-/Herbstumstellung, UTC-/lokale Mitternacht, Nachtlampe und Sturm. Es entstanden 55 frische PNGs. Die Tests liefen seriell mit 30 FPS Begrenzung. Bei Testbeginn war die ursprüngliche Unreal-Editor-Instanz (PID 23600) geöffnet; beim späteren Leistungslauf wurde eine andere Editor-Instanz (PID 15260, Start 14:05:30 Ortszeit) erfasst. Die Hintergrundlast war somit nicht durchgehend identisch. Die anfänglich fehlgeschlagenen Versuche gelten nicht als Abnahme; der vollständige erfolgreiche Wiederholungslauf liegt unter `artifacts/celestial-validation-20261125-135501`.

`Tools/ReportCelestial.py` bestätigt die exportierten Start- und Endzeiten unabhängig mit Python `zoneinfo` / IANA `Europe/Zurich` (`tzdata` 2025.3). Alle vier Grenzfälle einschließlich jeweils mindestens vier Sekunden Menüpause stimmen überein. Die sechs JPL-Stichproben ergeben maximal **0,002006° Sonnenfehler**, **0,037320° Mondfehler** und **0,006028 Prozentpunkte Phasenfehler**. Die geprüfte EXE hat SHA-256 `9EA1159DD8DD1AF471B9F87D2838E5B053B4138F71D924909017672DEA9224B5`. Laufzeitmarker, Zeitvergleich, Quell- und Artefaktprüfsummen stehen in [CELESTIAL_RESULTS.json](CELESTIAL_RESULTS.json).

Die portablen C++-Modelltests, 15 Python-Tests und drei Unreal-Profiltests bestanden ebenfalls. Die Aufnahmen zeigen weiterhin die bekannten Grenzen der Prototypgrafik: breite Wasserreflexionen, überhelle Mondscheibe bei starker Nachtadaption und noch nicht abgestimmte Sternsichtbarkeit bei Mondlicht. Der Nachtlampentest prüft Lampe und Instrumente im offenen Wasser; seine überwiegend schwarze Aufnahme ist keine Abnahme eines realistischen Lichtkegels auf dem Seegrund.

Reproduzierbarer Lauf aus dem Repository:

```powershell
./Tools/Build.ps1 -Target Package -BuildSlot Celestial -PackageDirectory Builds/WindowsCelestial
./Tools/TestCelestial.ps1 -PackageDirectory Builds/WindowsCelestial
python Tools/ReportCelestial.py artifacts/celestial-validation-YYYYMMDD-HHmmss
```

Die Berichtprüfung benötigt Python mit `tzdata` und verwendet `zoneinfo` als unabhängige CET-/CEST-Referenz. Sie prüft alle exportierten Tauchgangszeiten, die vier Zeit-/Datumsgrenzen, sechs Himmelsreferenzen und Laufzeitmarker; sie ersetzt keine visuelle Sichtung. Die Testserie läuft seriell und überschreibt ihre gemeinsamen Screenshot-/Laufzeitdateien erst nach dem Sichern des jeweiligen Falls.



Ein zusätzlicher 60-Sekunden-Unterwasserlauf nach 15 s Aufwärmen mit 1920 × 1080, Qualitätsstufe 2 und 100 % Renderauflösung erfasste 5,379 Bilder: 89.68 FPS im Mittel, 52.75 FPS im langsamsten Prozent, 44 Bilder über 16,67 ms und keines über 33,33 ms. Hardware: RTX 3090 / Ryzen 9 9950X3D; bei Messbeginn war Editor-Instanz PID 15260 geöffnet. Dies ist eine einzelne Szene mit Hintergrundlast, kein isolierter Leistungsnachweis für alle Tageszeiten, Sichtweiten oder Wetterlagen. Rohdaten: `artifacts/perf-underwater-1080p-q2-20261125-140722-970`; die Auswertung ist im Ergebnisbericht enthalten.

Auch der Root-Starter `Builds/WindowsCelestial/divingsim.exe` startete nachweislich `divingsim-Celestial.exe` und bestand den 22-Sekunden-Tauchtest mit 23 gespeicherten Messpunkten. Die ursprüngliche Projekt-EXE blieb unverändert (SHA-256 `C8904BBA4D9A4B40BA2EA1B4AF9A94FBC7FAED9C032FB5C3D2248C79B455B263`).

## Aufnahmen aus dem Windows-Paket

![Sommerabend am 21. Juni um 21:20 CEST](Images/Celestial_Dusk.png)

![Teilbeleuchteter Mond und Sterne am 21. Juni um 23:30 CEST](Images/Celestial_SummerNight.png)

![Gespeichertes Schaltjahresdatum im Umweltmenü](Images/Celestial_Date.png)

## Grenzen zur Spezifikation

Ein späterer Test der Kartenauswahl am östlichen Einstieg deckte einen zusätzlichen Fehler bei pausierter Uhr auf: Die native Beobachter-Neuzentrierung überschreibt nach mehr als zehn Kilometern Versatz die ephemeridische Mondphase. Der Build `WindowsNavigation` stellt die berechnete Phase nach diesem nativen Tick wieder her. Ursache, Umsetzung und zugehörige Prüfungen stehen in [NAVIGATION.md](NAVIGATION.md).

ENV-001/ENV-002 und der Datumsteil von CONF-003 sind erweitert. Jahreszeitabhängige Temperaturen, Wetter, Sichtweite, Eis/Schnee, Vegetation und Tierpopulation sind damit nicht umgesetzt. Die sommerlichen Orthobilder bleiben auch im Winter sichtbar. Die vollständige optische/fachliche Abnahme und die gesamte Requirements Specification bleiben offen.

## Primärquellen

- [Epic: Celestial Vault](https://dev.epicgames.com/documentation/en-us/unreal-engine/celestial-vault-plugin-for-unreal-engine), zusätzlich API und Implementierung der lokal installierten Version 5.8.4.
- [NASA/JPL Horizons API](https://ssd-api.jpl.nasa.gov/doc/horizons.html) und [Definition der Beobachtergrößen](https://ssd.jpl.nasa.gov/horizons/manual.html#obsquan).
- [METAS: Schweizer Zeit](https://www.metas.admin.ch/de/schweizer-zeit).
