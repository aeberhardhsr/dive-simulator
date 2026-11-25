# Spielmodi und gekoppelte Beeinträchtigungen

Der gemeinsame `Dive::Model` verwendet `ModeRules`; Grafik, See und Bedienung bleiben in allen Modi gleich. Diese Modelle sind gestaltete Spielmechaniken, keine klinischen Vorhersagen oder Tauchplanung. Die fachliche und spielerische Gesamtabnahme bleibt offen.

| Regel | Exploration | Simulation | Hardcore |
|---|---|---|---|
| Flaschengas | Unbegrenzt | Begrenzt | Begrenzt |
| Tarierung | Automatisch oder manuell wählbar | Manuell | Manuell |
| Gewebe / Dekompression | Aus | Aktiv | Aktiv |
| Körperliche Folgen | Aus | Aktiv | Aktiv |
| Reglervereisung | Aus | Aus | Aktiv |
| Instrumente auf dem Bildschirm | Immer sichtbar | Mit H ausblendbar | Mit H ausblendbar |
| Automatische Hilfetexte | Aktiv | Aktiv | Aus |

F1-Hilfe, manuell ausgelöste Bedienungsrückmeldungen, Speicherfehler und tatsächliche Instrumentalarme bleiben auch in Hardcore verfügbar. Der Handgelenkcomputer ist mit C erreichbar, wenn das Bildschirm-HUD ausgeblendet ist. Ein neuer Tauchgang beginnt mit gewartetem Regler. Die frühere interne Option `auto_buoyancy` wird weiterhin eingelesen, kann die Regeln von Simulation/Hardcore aber nicht übersteuern. Alte Exploration-Profile behalten die automatische Tarierung; die neue manuelle Auswahl wird separat gespeichert.

## Wahrnehmbare Auswirkungen

Anstrengung, Stress und Ermüdung bestimmen Atmung, Puls und Gasbedarf. Panik wächst aus Stress, verbrauchter Atemreserve und Belastung und erhöht wiederum Puls und Atemgasbedarf. Ohne Atemluft sinkt zunächst die Reserve, anschließend die simulierte Sättigung. Wiederhergestellte Atmung führt zu allmählicher Erholung.

Narkosewirkung wächst mit dem Stickstoffpartialdruck und nimmt bei geringerem Druck ab. Kältebelastung folgt Umgebungstemperatur, Dauer und Anzug. Eine länger unterschrittene modellierte Dekompressionsgrenze erzeugt eine langsam abklingende Belastung. Diese Größen verringern gemeinsam die Schwimmkraft und Gehgeschwindigkeit. Sichtbeeinträchtigung verwendet Entsättigung und eine dunklere Bildkante ohne erzwungene Kamerabewegung. Exploration behält volle Bewegungsleistung, Atemreserve, Sättigung und Körpertemperatur; Puls und Atemrhythmus reagieren vereinfacht auf Aktivität.

## Nachvollziehbare Spielparameter

Alle folgenden Zahlen sind **gestaltete Parameter**, nicht gemessene Toleranzen:

- Panikziel: `clamp(1.8*(Stress-.45)+.5*(1-Reserve)+.2*Belastung*Stress,0,1)`, Annäherung mit 0,08/s; zusätzlicher Puls 18/min, Atemfrequenz 8/min und Oberflächenverbrauch 12 L/min bei voller Panik.
- Narkoseziel: `clamp((pN2-2.8)/3.5,0,1)`, Annäherung mit 0,12/s.
- Kälteziel: `clamp((18-Umgebungstemperatur)/18,0,1)`; beim Trockentauchanzug Faktor 0,25, Zeitkonstante 600 s. Das ist eine separate qualitative Kältebelastung, keine behauptete Kerntemperaturmessung.
- Dekobelastung steigt pro Sekunde um `max(0,Ceiling-Tiefe-.5)/600`. Ohne Grenzverletzung klingt sie mit einer Zeitkonstante von 1800 s ab. Sie ist weder Erkrankungsdiagnose noch Risikowahrscheinlichkeit; auch ein Profil ohne angezeigte Belastung ist keine Zusicherung realer Sicherheit.
- Bewegungsfaktor: `clamp(1-.35*Ermüdung-.25*Kälte-.25*Narkose-.2*Panik-.45*Deko-.7*Hypoxie,.15,1)`, mit `Hypoxie=clamp((96-SpO2)/36,0,1)`. Dieser Faktor multipliziert Antriebskraft beziehungsweise Gehgeschwindigkeit, nicht Strömung oder Auftrieb.
- Sichtfaktor: `clamp(.35*Narkose+.25*Panik+.35*Deko+.9*Hypoxie,0,1)`; globale Farbsättigung `1-.8*Sichtfaktor`, Vignette `.35+.6*Sichtfaktor`.

## Hardcore-Reglerdefekt

Ein fiktiver Regler kühlt abhängig vom tatsächlichen Atem- und Jacketgasfluss ab. Seine Temperatur nähert sich `Umgebungstemperatur - 3.5*Oberflächengasfluss_L/s` mit einer Zeitkonstante von 20 s. Unter Wasser und bei Gasfluss summieren sich negative Reglertemperaturen als Kälteexposition; positive Temperaturen bauen sie wieder ab. Ab 60 Gradsekunden tritt ein bleibender freier Gasfluss auf. Er verbraucht bis zu 6 lokale Liter pro Sekunde, auf Oberflächenvolumen umgerechnet und durch das verbleibende Flaschengas begrenzt. Eine leere Flasche emittiert weder Gas noch Defektblasen. Der Defekt bleibt für diesen Tauchgang bestehen; es gibt keine Sofortreparaturtaste.

Das Spiel zeigt den Defekt als fortlaufende Blasen, separaten Strömungsgeräuschkanal und Instrumentalarm. Die Gasrestzeit berücksichtigt den zusätzlichen Verlust. Der Spieler behält seine normalen Bewegungs-, Tarierungs- und Abbruchmöglichkeiten. Weitere Defekttypen, Redundanzsysteme, Flaschenventilbedienung und Wartungsmodelle sind nicht implementiert.

Die qualitative Kopplung von Expansion, Abkühlung und freiem Gasfluss wird durch einen [DAN-Fallbericht](https://dan.org/safety-prevention/diver-safety/case-summaries/frigid-waters-cause-regulator-to-ice/) gestützt. Seine einzelne Ereignisfolge wurde **nicht** als universelle Ausfallzeit übernommen. Die progressive und reversible Narkosewirkung orientiert sich qualitativ am [CMAS-Faktenblatt](https://www.cmas.org/fact-sheets/nitrogen-narcosis-eng.html). Der [CDC-Überblick](https://www.cdc.gov/yellow-book/hcp/environmental-hazards-risks/scuba-diving-decompression-illness-and-other-dive-related-injuries.html) erläutert die unterschiedlichen Mechanismen von Narkose und Dekompressionserkrankungen. Keine dieser Quellen validiert unsere Formeln oder Spielparameter.

## Aufzeichnung und Prüfung

JSON-Schema 5 speichert Wirkungen, Bewegungs- und Sichtfaktor, Reglertemperatur, Vereisungsexposition, Defektzustand und verlorenes Gas pro Messpunkt. Die Modellkennungen sind `coupled_impairments_v2` und `fictional_cold_regulator_v1`. Das Logbuch liest weiterhin Schema 1–4 und unterscheidet fehlende historische Daten von tatsächlich gemessenen Nullwerten. Neue Dateien mit fehlenden oder ungültigen Wirkungsdaten werden zurückgewiesen.

`Tools/TestCore.cmd` prüft unter anderem Modusgrenzen, manuelle Tarierung, Druckabhängigkeit und Erholung, Kälte/Anzug, anhaltende Dekobelastung, Reglerausfall, Gasbegrenzung und Invarianz gegenüber Zeitschritt-Aufteilung. Unreal-Automation prüft Profilkompatibilität und Logbuchdaten. `Tools/TestModes.ps1` prüft sechs gerenderte Szenarien mit echten Laufzeitschritten, Tasteneingaben, Vorher/Nachher-Bildern und wieder eingelesenen Protokollen. Der Defekt wird dabei nicht direkt in den Zustand injiziert.

Am 25. November 2026 bestehen alle sechs Fälle mit **18 frischen Bildern in 1600 × 900**. Menübilder zeigen Deutsch und Englisch; Tastentests prüfen H und T. Exploration behält in beiden Tarierungsarten exakt 300 bar und 100 % Bewegungsleistung. Im kalten Hardcore-Test erscheint der Defekt erstmals im Messpunkt bei 48,02 s; bis zum Ende gehen 128,92 Oberflächenliter durch den Defekt verloren. Bei gleicher Belastung in Simulation und bei warmem Wasser in Hardcore tritt kein Defekt auf. Im Atemluftmangeltest sinkt der Bewegungsfaktor bis auf 56,95 %; der Sichtfaktor wächst auf 0,519. Diese Werte beschreiben ausschließlich die Testbedingungen, keine realen Toleranzen.

Maschinelle Ergebnisse mit EXE- und Bild-Hashes: [MODE_RESULTS.json](MODE_RESULTS.json). Paket: `Builds/WindowsModes/divingsim.exe`; tatsächliche EXE-SHA256: `4ffd2c9ecbb4d154ad8eac5b8c993ae6bc1b849cdc7575013063ce147efabfbc`. Rohdaten: `artifacts/mode-validation-20261125-172105`. Alle 87 Source-/Config-Dateien stimmen bytegenau mit dem kompilierten Editor-Sandbox-Stand überein. Elf Unreal-Automationstests, der portable C++-Kern und 15 Python-Tests bestehen. Narkose-/Deko-Erholung ist im Kern geprüft; diese Matrix enthält keinen separaten gerenderten Tieftauchgang. Ein aktueller zweistündiger Lauf und ein nativer macOS-Test bleiben offen.

Vier zusätzliche Paket-Regressionen bestehen: Lampe, vollständiger Atemluftmangel mit Logbuch, Einführung und automatische Umgebung. Sie liefern weitere 29 frische Aufnahmen, insgesamt somit **zehn Paketfälle und 47 Bilder**; [MODE_REGRESSION.json](MODE_REGRESSION.json). Der Root-Starter öffnet die richtige `divingsim-Modes.exe`, beendet den Standardtauchtest mit Exitcode 0 und erzeugt ein gültiges Protokoll; [MODE_LAUNCHER.json](MODE_LAUNCHER.json). Der vorherige Environment-Build ist unverändert.

Die gerenderte Standard-Unterwasserszene erreicht auf RTX 3090 / Ryzen 9 9950X3D bei 1920 × 1080, Qualität 2 und 100 % Renderauflösung über 60 s nach 15 s Aufwärmzeit **90,75 FPS im Mittel und 71,46 FPS im langsamsten Prozent**. Von 5.443 Bildern überschreitet eines sowohl 16,67 als auch 33,33 ms; Maximum 33,4404 ms. Der Lauf ist damit kein Nachweis einer ausnahmslos eingehaltenen 30-FPS-Bildzeitgrenze. Neun Geländekacheln bleiben geladen. [MODE_PERFORMANCE.json](MODE_PERFORMANCE.json) enthält die Messwerte und Hintergrundprozesse. Die Messung verwendet Exploration; ein gesonderter Leistungsnachweis bei maximaler Defektblasenlast und die allgemeine Hardwarefreigabe bleiben offen.

```powershell
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/Build.ps1 -Target Package -EditorSandbox -BuildSlot Modes -PackageDirectory Builds/WindowsModes
./Tools/TestModes.ps1
python Tools/ReportModes.py artifacts/mode-validation-YYYYMMDD-HHMMSS --output Documentation/MODE_RESULTS.json
```
