# Walensee Diving Simulator

## Verbindliche Ausgangsbasis

- Nutzerauftrag vom 25. November 2026: spielbarer, realitätsnah wirkender Tauchsimulator im bestehenden Unreal-Projekt.
- [Grundlagen und Engineentscheidung](https://chatgpt.com/share/6b05f884-87c8-83eb-95d0-248d93ef2139), vollständig im angemeldeten Browser gelesen.
- [Requirements Specification v0.1](https://chatgpt.com/c/6b0615d3-17c8-83eb-891b-8af1042b27eb), im angemeldeten Browser gelesen.
- [GitHub](https://github.com/aeberhardhsr/dive-simulator).

Die vollständigen 236 nummerierten Anforderungen stehen in [REQUIREMENTS.csv](REQUIREMENTS.csv). Nicht nummerierte Modell-/Modusvorgaben und die zwanzig MVP-Abnahmekriterien sind in [REQUIREMENTS_CONTEXT.md](REQUIREMENTS_CONTEXT.md) festgehalten. Die folgende Übersicht ersetzt diese Einzelanforderungen nicht.

Die fachlichen Anforderungen bleiben gültig. Die Unity/HDRP/C#-Angaben der Spezifikation werden durch die ausdrücklich gewählte Unreal-Engine-Basis ersetzt. Lokal vorhanden: Unreal Engine 5.8.4 und Blender 5.3. Windows zuerst; gemeinsamer Quellstand für macOS, dort separater Build und Test auf dem vorhandenen Mac mit Xcode. macOS ist ohne diese Prüfung nicht als unterstützt freizugeben.

## Erstes ausführbares Inkrement

Spielbarer Ablauf: Konfiguration → Uferstart → Gehen/Waten → Tauchen und Tarieren → Rückkehr auf das Startpodest → Auswertung. Die bisherigen Offshore-Spielstarts bleiben wählbar. Der ursprüngliche synthetische Testgrund ist durch die reale swissBATHY3D-Bathymetrie ersetzt. Drei örtliche Uferbereiche, lokale Geländekacheln und eine Tiefenkarte sind implementiert. Über Wasser liegen 77 swissALTI3D-Nanite-Kacheln mit 8-m-Quellgeometrie; Berge und Ufer tragen registrierte SWISSIMAGE-Luftbilder des Kacheljahrs 2025. Die begehbaren Zugänge sind modellierte Plattformen/Treppen auf den Höhendaten; originalgetreue Anlagen, räumliche Landschaftsdetails und fotorealistische Qualitätsabnahme bleiben offen.

Simulation in engineunabhängigem C++, Unreal-Pawn und Kamera als Adapter; Blueprint-einstellbare Parameter, reproduzierbare Blender- und Editor-Pipelines. Einheiten: Kern SI (Meter, Sekunden, Liter, bar), Unreal Zentimeter und Z nach oben. Mittlerer Wasserspiegel Z=0; lokale Wellen bewegen die Oberfläche.

Unterwassergrund, ergänzte Uferbereiche und Felsobjekte verwenden inzwischen unterschiedliche gescannte Sand-/Kies-/Felsmaterialien mit weltbezogener Projektion, Normalen und Rauheit. Die räumliche Verteilung ist gestalterisch, keine lokale Sedimentmessung. Herkunft, Vergleichsbilder und Modellgrenzen: [BED_SURFACES.md](BED_SURFACES.md). Diese Verbesserung erfüllt noch nicht die gesamte visuelle Referenzabnahme.

## Fachlicher Gesamtumfang und noch zu prüfende Abnahme

| Bereich | Umfang aus Spezifikation | Abnahme |
|---|---|---|
| WRLD | Reale Walensee-Geodaten, 1:1, Streaming, mindestens drei Einstiegspunkte, Felsen/Sedimente, Landschaft über Wasser, Tiefenkarte | Bathymetrie, Hintergrundlandschaft und registrierte SWISSIMAGE-Texturen vorhanden; drei örtliche Ufermeshes mit gekennzeichneter Lückeninterpolation und Kollision integriert. Modellierte Plattformen/Treppen mit spielbarem Land-Wasser-Wechsel vorhanden; räumliche Landschaftsdetails, originalgetreue Anlagen und Gesamtabnahme offen |
| WATR | Wellen, Brechung, Reflexion, Absorption, God Rays, Caustics, Partikel, Strömung, Atemblasen mit Expansion, Thermokline, Oberflächenübergang | Geometrische Wellen, Tiefendruck und Wasserbewegung gekoppelt; Sicht steuert auch Oberflächenabsorption. Optische/fachliche Gesamtabnahme und Uferwechselwirkung offen |
| BIOL | Mindestens fünf anatomisch plausible animierte Tierarten, drei Bewuchstypen, Schwärme, Flucht, Kollision, Habitat und Pflanzenbewegung | Fünf Fischmodelle (Felchen als Artengruppe), drei Bewuchstypen und Habitat-/Bewegungsregeln implementiert; anatomische und visuelle Abnahme offen |
| DIVR | First Person, 3D-Bewegung, Auftrieb, Widerstand, Trägheit, Anstrengung, Atmung, BCD, Blei, Kollision, sichtbare Ausrüstung | Schwimmen/Tarierung und aufrechtes Gehen/Waten mit Bodenkollision implementiert; Ganzkörperhaltung, Flossen an Land und physikalische Gesamtabnahme offen |
| CONF | Einstieg, Uhrzeit, Wetter, Sicht, Temperatur, Strömung, Flasche 10/12/15 L, Druck, Luft, Anzug, Blei, Lampe, Modus, geplante Tiefe/Dauer, Profile | Konfigurationsmenüs und benannte lokale Tauchprofile mit Vorschau, Laden, Umbenennen, vorherigem Stand und Archiv implementiert; Gesamtabnahme offen |
| VITAL | Gekoppelte Herz-/Atemfrequenz, simulierte SpO2, Kerntemperatur, Belastung, Stress, Ermüdung, Stickstoff; keine unabhängigen Zufallswerte | Gekoppelte qualitative Spielmodelle einschließlich endlicher Atemreserve und verzögerter Sättigungsreaktion implementiert; unabhängige physiologische Validierung und Gesamtabnahme offen |
| GAS | Druck, tiefen-/belastungs-/stressabhängiger Verbrauch, ppO2, Restzeit, Nullzeit, Gewebeaufnahme/-abgabe, Warnungen, Stopps | Offen |
| EQPM | Datengetriebene Ausrüstung, BCD, Atemregler, Computer, Lampe, Kompass, Isolation und Gewicht, 3D-Modelle | Sichtbare Unterarme/Handschuhe, Handgelenkcomputer und Handlampe implementiert; manuell ein-/absetzbarer Atemregler mit drei Atemzuständen vorhanden; vollständiger Ausrüstungskatalog und Körpermodelle offen |
| HUD | Tiefe/max., Dauer, Druck, Nullzeit, Puls, SpO2, Atmung, Temperatur, Aufstiegsrate, Kompass, Warnungen; digital und Handgelenk | Bildschirm-HUD und dreiseitiger 3D-Handgelenkcomputer vorhanden; vollständige Qualitätsabnahme offen |
| ENV | Tag/Nacht, Sonnenstand, Bewölkung, Wetter/Wellen, Tiefentemperatur/Thermokline, Sichtweite | Einstellbare Wetter-/Wind-/Temperaturprofile, volumetrische Wolken und gemeinsames Wellenmodell implementiert. HDR-Licht mit expliziten Lux/Lumen und gekoppeltem Tag-/Nacht-/Tiefenverhalten vorhanden; astronomischer Sonnenstand, visuelle und fachliche Gesamtabnahme offen |
| AUD | Atmung und Blasen synchron, Belastung, Herzschlag, Tiefe, Oberfläche, Dämpfung, Warnungen, BCD, räumlich | Zustandsgekoppelte Ein-/Ausatmung, räumliche Blasen, Puls, Ventile, gefilterte Wetter-/Wasserkulisse und fünf persistierte Lautstärkeregler implementiert; akustische Qualitätsabnahme offen |
| MODE | Exploration, Simulation, Hardcore auf gemeinsamem Kern; Unterschiede transparent konfiguriert | Offen |
| NAV | Kompass, Walenseekarte, Tiefenkarte, Einstieg; optional Wegpunkte, Fotos und Entdeckungen | Offen |
| LOG | Automatische Speicherung, Tiefe/Gas/Vitalwerte/Temperatur/Strecke/Warnungen, grafische Auswertung | Automatische JSON-Speicherung, Logbuchbrowser mit Modusfilter, vier Diagrammgruppen und Warnereignissen implementiert; vollständige Abnahme offen |
| UI | Menü, Gebiet/Ausrüstung/Konfiguration, Logbuch, Grafik/Audio, Tastenbelegung, Pause, Tutorial, DE/EN, Profil, persistierte Einstellungen, Komfort | 17 belegbare Aktionen mit Konflikttausch, dynamische Tastenhinweise, Maus-/Sichtfeldoptionen, achtteilige Einführung und Tauchkonfigurationsprofile implementiert; vollständige Abnahme offen |
| PERF/GFX | PBR, LOD/Streaming/Culling, GPU-Partikel, 1080p–4K, 60 FPS empfohlen/30 FPS Minimum, zwei Stunden stabil | Reproduzierbare gerenderte Windows-Messung in 1080p/1440p/4K vorhanden, Ergebnisse und Grenzen in PERFORMANCE.md. Einzelner Ruckler protokolliert; Mindesthardware, zweistündiger Dauertest, macOS und Gesamtabnahme offen |
| MAINT | Module, Tests, Konfiguration, Erweiterbarkeit, versionierte Persistenz, Windows/macOS-Builds, Modellbeschreibung | Offen |

## Reihenfolge

1. Toolchain, reproduzierbarer Build, Simulationskern und spielbarer Tauchablauf.
2. Wasserbild, animierte Tiere/Pflanzen, Ton und Instrumente im kleinen Ausschnitt prüfen.
3. Schweizer Geodaten integrieren; drei reale Einstiegspunkte. Vollständiger swisstopo-Walensee-Datensatz ist mit geprüfter SHA256 in 46 Rasterkacheln eingebunden. Die aktuelle Implementierung verwendet neun lokal erzeugte Geländekacheln mit Nachladen statt World Partition. Zusätzlich zu den Offshore-Starts bestehen drei örtliche Uferstarts mit modellierten begehbaren Zugängen. Originalgetreue Anlagen und Landbedeckung bleiben offen. Provenienz: `WALENSEE_DATA.json`, Laufzeitformat: `GEODATA.md`.
4. Vollständige Ausrüstung, Physiologie/Dekompression, UI/Logbuch, Lokalisierung und Eingaben.
5. Referenzbasierte Grafikabnahme, Plattformtests, Performance und zweistündiger Dauertest.

Die ursprüngliche MVP-Abnahme umfasst alle MUST-Anforderungen, beide Plattformen, reale Geografie, fünf Tierarten, drei Bewuchstypen, drei Einstiegspunkte und validierte Performance. Ein früher Prototyp erfüllt diese Abnahme ausdrücklich noch nicht.

## Modellgrenzen

Das Produkt ist ein Spiel. Gasvorrat (Pressluft), Sauerstoffanteil, Partialdruck und simulierte Sauerstoffsättigung sind getrennte Werte. Physiologie und Dekompression sind unvalidierte Spielmodelle, keine medizinische Simulation und kein Werkzeug zur Planung realer Tauchgänge. Sicht-/Temperaturprofile, Felsen, Pflanzen und Tiere sind keine lokalen Messdaten; der Geländegrund stammt aus der Vermessung.
