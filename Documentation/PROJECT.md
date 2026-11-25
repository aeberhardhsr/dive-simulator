# Walensee Diving Simulator

## Verbindliche Ausgangsbasis

- Nutzerauftrag vom 25. November 2026: spielbarer, realitätsnah wirkender Tauchsimulator im bestehenden Unreal-Projekt.
- [Grundlagen und Engineentscheidung](https://chatgpt.com/share/6b05f884-87c8-83eb-95d0-248d93ef2139), vollständig im angemeldeten Browser gelesen.
- [Requirements Specification v0.1](https://chatgpt.com/c/6b0615d3-17c8-83eb-891b-8af1042b27eb), im angemeldeten Browser gelesen.
- [GitHub](https://github.com/aeberhardhsr/dive-simulator).

Die fachlichen Anforderungen bleiben gültig. Die Unity/HDRP/C#-Angaben der Spezifikation werden durch die ausdrücklich gewählte Unreal-Engine-Basis ersetzt. Lokal vorhanden: Unreal Engine 5.8.4 und Blender 5.3. Windows zuerst; gemeinsamer Quellstand für macOS, dort separater Build und Test auf dem vorhandenen Mac mit Xcode. macOS ist ohne diese Prüfung nicht als unterstützt freizugeben.

## Erstes ausführbares Inkrement

Spielbarer Ablauf: Konfiguration → Einstieg → Erkunden und Tarieren → Auftauchen → Auswertung. Der ursprüngliche synthetische Testgrund ist durch die reale swissBATHY3D-Bathymetrie ersetzt. Drei Offshore-Spielstarts, lokale Geländekacheln und eine Tiefenkarte sind implementiert. Über Wasser ist ein swissALTI3D-Hintergrundmesh der umliegenden Berge vorhanden. Detaillierte Landschaft, begehbare Ufer, reale Zugänge und fotorealistische Qualitätsabnahme bleiben offen.

Simulation in engineunabhängigem C++, Unreal-Pawn und Kamera als Adapter; Blueprint-einstellbare Parameter, reproduzierbare Blender- und Editor-Pipelines. Einheiten: Kern SI (Meter, Sekunden, Liter, bar), Unreal Zentimeter und Z nach oben. Wasseroberfläche Z=0.

## Fachlicher Gesamtumfang und noch zu prüfende Abnahme

| Bereich | Umfang aus Spezifikation | Abnahme |
|---|---|---|
| WRLD | Reale Walensee-Geodaten, 1:1, Streaming, mindestens drei Einstiegspunkte, Felsen/Sedimente, Landschaft über Wasser, Tiefenkarte | Offen |
| WATR | Wellen, Brechung, Reflexion, Absorption, God Rays, Caustics, Partikel, Strömung, Atemblasen mit Expansion, Thermokline, Oberflächenübergang | Offen |
| BIOL | Mindestens fünf anatomisch plausible animierte Tierarten, drei Bewuchstypen, Schwärme, Flucht, Kollision, Habitat und Pflanzenbewegung | Offen |
| DIVR | First Person, 3D-Bewegung, Auftrieb, Widerstand, Trägheit, Anstrengung, Atmung, BCD, Blei, Kollision, sichtbare Ausrüstung | Offen |
| CONF | Einstieg, Uhrzeit, Wetter, Sicht, Temperatur, Strömung, Flasche 10/12/15 L, Druck, Luft, Anzug, Blei, Lampe, Modus, geplante Tiefe/Dauer, Profile | Offen |
| VITAL | Gekoppelte Herz-/Atemfrequenz, simulierte SpO2, Kerntemperatur, Belastung, Stress, Ermüdung, Stickstoff; keine unabhängigen Zufallswerte | Offen |
| GAS | Druck, tiefen-/belastungs-/stressabhängiger Verbrauch, ppO2, Restzeit, Nullzeit, Gewebeaufnahme/-abgabe, Warnungen, Stopps | Offen |
| EQPM | Datengetriebene Ausrüstung, BCD, Atemregler, Computer, Lampe, Kompass, Isolation und Gewicht, 3D-Modelle | Offen |
| HUD | Tiefe/max., Dauer, Druck, Nullzeit, Puls, SpO2, Atmung, Temperatur, Aufstiegsrate, Kompass, Warnungen; digital und Handgelenk | Offen |
| ENV | Tag/Nacht, Sonnenstand, Bewölkung, Wetter/Wellen, Tiefentemperatur/Thermokline, Sichtweite | Offen |
| AUD | Atmung und Blasen synchron, Belastung, Herzschlag, Tiefe, Oberfläche, Dämpfung, Warnungen, BCD, räumlich | Offen |
| MODE | Exploration, Simulation, Hardcore auf gemeinsamem Kern; Unterschiede transparent konfiguriert | Offen |
| NAV | Kompass, Walenseekarte, Tiefenkarte, Einstieg; optional Wegpunkte, Fotos und Entdeckungen | Offen |
| LOG | Automatische Speicherung, Tiefe/Gas/Vitalwerte/Temperatur/Strecke/Warnungen, grafische Auswertung | Offen |
| UI | Menü, Gebiet/Ausrüstung/Konfiguration, Logbuch, Grafik/Audio, Tastenbelegung, Pause, Tutorial, DE/EN, Profil, persistierte Einstellungen, Komfort | Offen |
| PERF/GFX | PBR, LOD/Streaming/Culling, GPU-Partikel, 1080p–4K, 60 FPS empfohlen/30 FPS Minimum, zwei Stunden stabil | Offen |
| MAINT | Module, Tests, Konfiguration, Erweiterbarkeit, versionierte Persistenz, Windows/macOS-Builds, Modellbeschreibung | Offen |

## Reihenfolge

1. Toolchain, reproduzierbarer Build, Simulationskern und spielbarer Tauchablauf.
2. Wasserbild, animierte Tiere/Pflanzen, Ton und Instrumente im kleinen Ausschnitt prüfen.
3. Schweizer Geodaten integrieren; drei reale Einstiegspunkte. Vollständiger swisstopo-Walensee-Datensatz ist mit geprüfter SHA256 in 46 Rasterkacheln eingebunden. Die aktuelle Implementierung verwendet neun lokal erzeugte Geländekacheln mit Nachladen statt World Partition. Drei vermessene Offshore-Startpunkte sind vorhanden; Ufer und reale Zugänge fehlen noch. Provenienz: `WALENSEE_DATA.json`, Laufzeitformat: `GEODATA.md`.
4. Vollständige Ausrüstung, Physiologie/Dekompression, UI/Logbuch, Lokalisierung und Eingaben.
5. Referenzbasierte Grafikabnahme, Plattformtests, Performance und zweistündiger Dauertest.

Die ursprüngliche MVP-Abnahme umfasst alle MUST-Anforderungen, beide Plattformen, reale Geografie, fünf Tierarten, drei Bewuchstypen, drei Einstiegspunkte und validierte Performance. Ein früher Prototyp erfüllt diese Abnahme ausdrücklich noch nicht.

## Modellgrenzen

Das Produkt ist ein Spiel. Gasvorrat (Pressluft), Sauerstoffanteil, Partialdruck und simulierte Sauerstoffsättigung sind getrennte Werte. Physiologie und Dekompression sind unvalidierte Spielmodelle, keine medizinische Simulation und kein Werkzeug zur Planung realer Tauchgänge. Sicht-/Temperaturprofile, Felsen, Pflanzen und Tiere sind keine lokalen Messdaten; der Geländegrund stammt aus der Vermessung.
