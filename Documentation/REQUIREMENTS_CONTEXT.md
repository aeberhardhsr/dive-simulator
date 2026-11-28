# Abnahmebasis aus der Requirements Specification v0.1

**Nutzerentscheidung vom 28. November 2026:** Die Unterwasserwelt ist gestalterisch abgenommen. Nach der abgeschlossenen laufenden Optimierung folgen [genau fünf begrenzte Abschlussiterationen](UNDERWATER_FINISH_PLAN.md), danach die Überwasserwelt. Die ursprüngliche Anforderungstabelle bleibt als Quelle erhalten. Diese Teilabnahme ist keine pauschale Erfüllung aller Rendering-, Hardware-, Plattform- oder Gesamtspielanforderungen.

Quelle: [Requirements-Spezifikation Tauchspiel](https://chatgpt.com/c/6b0615d3-17c8-83eb-891b-8af1042b27eb), erneut vollständig über die Tabellen der angemeldeten Browseransicht am 25. November 2026 gelesen. `REQUIREMENTS.csv` übernimmt alle 236 nummerierten Einträge einschließlich Optionen, Einheiten, Zielwerten und ursprünglichen Prioritäten. Trennzeichen ist `|`, Zeichensatz UTF-8. Die 236 Datenzeilen ohne abschließenden Zeilenumbruch ergeben 12.040 Zeichen und FNV-1a32 `ff6027b6`; dieser Wert wurde gegen die aus dem Browser gelesenen Zeilen verglichen.

Enthalten sind 150 MUST-, 38 SHOULD-, fünf COULD- und sieben FUTURE-Einträge. Die 36 nichtfunktionalen Einträge PERF/GFX/MAINT besitzen im Original keine separate Priorität; das leere Feld bedeutet keine Freigabe zum Weglassen. Diese Liste ist eine Anforderungsquelle, keine Erledigtliste. Die übergeordneten Vorgaben und Abnahmekriterien gelten zusätzlich. Der Unreal-Auftrag ersetzt die Unity/HDRP-spezifische technische Umsetzung, nicht die fachlichen Anforderungen.

## Nicht nummerierte Vorgaben

Die Welt soll geografisch authentisch, zusammenhängend und frei erkundbar sein. Fotorealismus betrifft insbesondere Wasser, Tiere, Pflanzen, Felsen und Sedimente. Die tatsächlichen Arten/Lebensräume und visuelle Referenzstandards müssen vor der finalen Umsetzung fachlich geprüft werden. Die Geometrie allein beweist diese Qualität nicht.

Physiologische Größen müssen zusammenhängend auf Umwelt und Verhalten reagieren; unabhängige Zufallswerte erfüllen die Anforderung nicht. Belastung erhöht Verbrauch, Stress erhöht Atmung/Puls, Kälte und Ermüdung reduzieren Bewegungsleistung. Fehlende Atemluft führt zu zunehmender Beeinträchtigung; Narkose, Panik und Dekompressionsprobleme sollen wahrnehmbare Folgen haben. Plausibilitätsprüfung ist vor Veröffentlichung erforderlich. Ohne entsprechende Validierung dürfen weder medizinische Genauigkeit noch Eignung zur Tauchausbildung suggeriert werden.

Die Spezifikation sieht lokale Speicherung ohne erforderliche Internetverbindung vor. SQLite wird als Datenbank vorgeschlagen; der derzeitige JSON-Dateispeicher weicht technisch davon ab. Entitäten umfassen Spielerprofil, Einstellungen, Gebiet/Einstieg, Tauchkonfiguration/-session/-telemetrie/-ereignisse, Ausrüstung/Loadout/Gas, Arten/Entdeckungen, Fotos und Erfolge. Nicht jede genannte Entität ist bereits implementiert.

Vorgeschlagene Frequenzen: Rendering mindestens 60 FPS, Physik 50 Hz, Eingaben pro Frame, Vitalwerte/Gas/HUD 10 Hz, Dekompression/Wetter/Telemetrie 1 Hz, nahe Tier-KI 10–20 Hz, entfernte Tier-KI 1–2 Hz, Zwischenspeicherung alle 30 s. Laut Quelle sind dies initiale Entwicklungsparameter, die durch Profiling/Tests zu optimieren sind. Vor allem die regelmäßige Zwischenspeicherung ist vom Speichern erst am Tauchgangende zu unterscheiden.

Die technische Architektur soll Rendering und Simulation trennen, erweiterbare Datenpipelines besitzen und große Weltabschnitte bedarfsgerecht laden. Hardware-Raytracing darf nicht zwingend erforderlich sein. Windows verwendet DirectX 12, macOS Metal; Wasser, Shader und Partikeleffekte sind tatsächlich auf beiden Plattformen zu prüfen. Die portablen Kernprüfungen ersetzen keinen macOS-Spielbuild.

## Spielmodi aus Abschnitt 5.12

| Funktion | Exploration | Simulation | Hardcore |
|---|---|---|---|
| Frei erkundbare Welt / realistische Grafik | ja | ja | ja |
| Begrenzter Atemgasvorrat | nein | ja | ja |
| Vitalwertsimulation | vereinfacht | ja | ja |
| Manuelle Tarierung | optional | ja | ja |
| Dekompressionsmodell | nein | ja | ja |
| Ausrüstungsdefekte | nein | nein | ja |
| Permanente HUD-Anzeige | ja | optional | optional |
| Automatische Hilfestellungen | ja | ja | nein |
| Konsequenzen bei Fehlverhalten | nein | ja | ja |
| Physiologische Beeinträchtigungen | nein | ja | ja |

Alle Modi müssen dieselbe Simulationsengine verwenden; Unterschiede entstehen durch Konfiguration. Hardcore ist nicht allein durch einen geänderten Namen oder verstecktes HUD umgesetzt.

## Zwanzig Kriterien aus Abschnitt 15

1. Das Spiel kann unter Windows und macOS gestartet werden.
2. Die Topografie des Walensees basiert auf realen geografischen Daten.
3. Die Spielwelt kann ohne sichtbare Ladeunterbrechungen erkundet werden.
4. Mindestens drei qualitativ ausgearbeitete Taucheinstiegspunkte stehen zur Verfügung.
5. Die Unterwasserwelt erfüllt definierte visuelle Referenzstandards.
6. Mindestens fünf animierte Tierarten sind vorhanden.
7. Mindestens drei unterschiedliche Pflanzen- oder Bewuchstypen sind vorhanden.
8. Wasserbewegungen, Luftblasen und Lichtbrechung werden dynamisch dargestellt.
9. Tauchgänge können individuell konfiguriert werden.
10. Der Taucher unterliegt einer funktionierenden Auftriebssimulation.
11. Atemgasverbrauch und Vitalwerte werden kontinuierlich simuliert.
12. Ein funktionierender Tauchcomputer ist integriert.
13. Warnmeldungen reagieren auf definierte simulierte Zustände.
14. Exploration-, Simulation- und Hardcore-Modus sind spielbar.
15. Tauchgänge werden automatisch im Logbuch gespeichert.
16. Sämtliche MUST-Anforderungen sind implementiert und getestet.
17. Die definierten Performanceziele werden auf Referenzhardware erreicht.
18. Das Spiel ist mindestens zwei Stunden ohne Absturz spielbar.
19. Alle verwendeten Assets und Geodaten sind ausreichend lizenziert.
20. Die Grenzen des physiologischen Modells sind dokumentiert.

Vorläufige Hardwareziele der Quelle: Windows Minimum Ryzen 5/Core i5, 16 GB RAM, RTX 3060/vergleichbar, 1080p/30 FPS; empfohlen Ryzen 7/Core i7, 32 GB RAM, RTX 4070/vergleichbar, 1440p/60 FPS. macOS Minimum M2/16 GB/30 FPS, empfohlen M4 Pro/32 GB/60 FPS. Diese Angaben sind Entwicklungsziele, keine bereits bestätigten Systemanforderungen. Aktuelle lokale Hardware wird durch die Messprotokolle belegt und nicht aus einer Hardwareannahme im Ursprungstext übernommen.
