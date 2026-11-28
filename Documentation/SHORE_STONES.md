# Räumliche Uferbefestigung in Weesen

Stand: 28. November 2026. Dieser Schritt ergänzt einen begrenzten Abschnitt am Weesener Einstieg um dreidimensionale Steine und ein dazu passendes Bodenmaterial. Er verbessert die Nahansicht; die Überwasserwelt erreicht damit noch nicht das gewünschte Referenzniveau.

## Ausgangspunkt und sichtbare Änderung

Die Uferluftbilder besitzen bereits 18,75 cm Pixelabstand. Trotzdem wirken fotografierte Gegenstände und Steine auf dem geglätteten Gelände flach und in Schrägsicht verwaschen. Das geprüfte Luftbild zeigt eine steinerne Uferbefestigung mit einer kleinen gebogenen Einbuchtung westlich des Einstiegs. Die Ergänzung folgt einer von Hand interpretierten, rund 103 m langen Linie in diesem Bild. Es handelt sich nicht um vermessene Einzelsteine oder eine neue amtliche Geländeaufnahme.

Bisherige Darstellung, gleicher Standort und Blickwinkel:

![Weesen vor der räumlichen Ergänzung](Images/ShoreBank_Before.png)

Die dritte Variante ergänzt räumliche Formen, passende Schatten und eine ruhigere graue Farbpalette:

![Weesen mit räumlicher Uferbefestigung](Images/ShoreBank_After.png)

Eine erste Variante mit zwei dünnen Steinreihen blieb zu vereinzelt. Vier versetzte Reihen und eine Ausrichtung an der Geländeneigung ergeben eine geschlossenere Befestigung. In der dritten Variante wurden die deutlichen Braun-/Grau-Unterschiede der verwendeten Scans reduziert und der ergänzende Bodenbereich verbreitert. Unscharfe Fotoabdrücke und helle Ränder ausserhalb dieses Bereichs bleiben sichtbar. Die ausgesparte Zugangszone und verworfene nasse Standorte erzeugen bewusst Unterbrechungen; die Ergänzung ist keine lückenlose Rekonstruktion der gesamten Ufermauer.

## Geometrie, Material und Begrenzung

`prepare_shore_stones.py` bindet die verwendete SWISSIMAGE-Datei per SHA-256. Aus der interpretierten Linie entstehen deterministisch 512 Kandidaten. Die Laufzeit platziert 183 davon, verwirft 285 wegen der Höhen-/Wasserbegrenzung und hält 44 aus dem Zugangskorridor fern. Die vorhandenen Gelände- und Kollisionsraster bleiben unverändert. Drei vorhandene Scanformen werden instanziert, geneigt und teilweise eingebettet. Ihre rotierten Begrenzungsboxen liegen mindestens 5,061 cm oberhalb der Referenzwasserfläche bei 419 m LN02. Diese Aussage betrifft die Referenzebene, nicht jeden möglichen Wellenberg.

Die Steine besitzen Kollision. Senkrechte Abfragen durch alle 183 Steinstandorte treffen die jeweiligen Instanzkomponenten. Bei sich überlappenden Steinen kann die Abfrage auch eine benachbarte Instanz derselben Komponente treffen; dies ist kein vollständiger Geh-, Kletter- oder Kontaktversuch an jeder einzelnen Steinfläche.

Eine separate 1024²-Maske steuert ausschliesslich die gestaltete Uferzone. Ihre volle Breite beträgt 4,6 m, mit 1,2 m Übergang auf jeder Seite. Sie bezeichnet keine amtliche Bodenbedeckung. Die unabhängige Prüfung verwendet Polygonlinien-Abstände aus Shapely statt der vorbereitenden NumPy-Berechnung: 30.000 Pixelmittelpunkte, darunter 4.803 mit einem nichtnull Maskenwert, ergeben null Unterschiede. Die native Prüfung bestätigt alle elf residenten Mipstufen und 720.896 Byte Texturspeicher. Der Pixelvergleich ist ein Offline-Nachweis, kein GPU-Readback.

`M_ShoreBank` wird nur für die Weesener Uferfläche gewählt. Drei separate `M_ShoreBankRock`-Materialien verwenden die vorhandenen Scantexturen mit zurückhaltenderer Farbe. Die ursprünglichen zwölf betroffenen Gelände-/Felsmaterialien und Felsmeshes stimmen mit den LFS-Prüfsummen des vorherigen Commits überein. Der Materialbeitrag bleibt unterhalb der Referenzwasserfläche null; die akzeptierte Unterwassergestaltung wird nicht neu bearbeitet.

Aktivierung: `-DiveLandscapePilot -DiveNaturalGround -DiveShoreStones`. Der Paketbau prüft zusätzlich die [gebundenen Inhaltsdateien](SHORE_BANK_CONTENT.json); veränderte Rezepte, Daten oder Assets erfordern eine erneute Inhaltsprüfung.

## Prüfstand

Die drei Aufnahmen der endgültigen Editor-Variante verwenden 1920 × 1080, Qualitätsstufe 2, 100 % Renderauflösung, normales Nanite und Produktionswasser. Nach 15 Sekunden Einlaufzeit werden jeweils zehn Sekunden gemessen. Das feste Vergleichspaar bei 260° erreicht 71,76 FPS ohne und 71,41 FPS mit Ufersteinen; die längsten Frames dauern 15,95 bzw. 17,49 ms. Die zusätzliche 285°-Ansicht erreicht 71,91 FPS. Kein Frame dieser drei kurzen Läufe überschreitet 33,33 ms. Das ist kein Dauertest oder statistischer Leistungsnachweis.

Der erste vollständige Editor-Uferablauf bestand spielerisch, scheiterte aber an einer fehlenden Pflichtaufnahme: `ShoreDive` und das allgemeine `DiveSmoke` forderten im selben Frame einen Screenshot an. Unreal ersetzte die erste Anforderung durch die zweite. Die allgemeinen Testaufnahmen warten jetzt, solange bereits eine Aufnahme ansteht. Bewegungs-, Kollisions-, Logbuch- und Abschlusskriterien bleiben bestehen. Die Wiederholung besteht einschliesslich aller neun frischen Aufnahmen, vollständiger aufrechter Rückkehr und Menüwechsel. Der fehlgeschlagene erste Nachweis bleibt erhalten.

Die festen Bildvergleiche entstanden vor dieser reinen Aufnahmekorrektur. Ihre genaue Editor-DLL ist separat archiviert; die abschliessende DLL und der Paketbau gehören zum korrigierten Stand. Die Bildgestaltung und die Steinplatzierung sind zwischen diesen Builds unverändert.

Das separate Paket `Builds/WindowsShoreLandscapeR1` ist gebaut und geprüft. Der echte Haupt-Launcher startet den erwarteten Kindprozess und besteht den Murg-Tauchablauf. Die vollständigen Uferstrecken in Weesen, Murg und Mols sowie Mols mit 30 FPS bestehen mit jeweils neun frischen Pflichtaufnahmen. Alle vier erreichen aufrechte Rückkehr mit Schwimmüberblendung null und Höhenfehler null, danach Abschluss und Menüwechsel. Das belegt die automatischen mittigen Strecken; seitliche Geländerannäherungen bleiben ein bekannter Restpunkt.

Die Paketmessungen verwenden dieselbe EXE und dieselben Container, 1920 × 1080, Qualität 2, 100 % Renderauflösung und normales Nanite auf RTX 3090 / Ryzen 9 9950X3D. Die natürlichen Bodenmaterialien und der Gelände-Pilot bleiben in beiden Überwasserläufen aktiv; nur die Uferergänzung wird zugeschaltet. Nach jeweils 15 Sekunden Einlaufzeit werden rund 60 Sekunden gemessen:

| Lauf | Durchschnitt FPS | 1%-Low FPS | Längster Frame | Frames über 33,33 ms |
| --- | ---: | ---: | ---: | ---: |
| Weesen ohne Uferergänzung | 70,98 | 58,30 | 18,16 ms | 0 / 4257 |
| Weesen mit Uferergänzung | 71,22 | 59,18 | 18,64 ms | 0 / 4271 |
| Weesen unter Wasser mit Uferergänzung | 62,02 | 54,35 | 19,33 ms | 0 / 3719 |

Das einzelne Überwasserpaar zeigt keinen erkennbaren Leistungseinbruch. Es begründet keinen statistischen Geschwindigkeitsvorteil und keine Garantie für alle Einstellungen oder lange Sitzungen. Der Unterwasserlauf ist eine einseitige Regressionsprüfung. Die bekannten horizontalen Wasserbänder sind in den Bildern weiterhin sichtbar.

54 portable Python-Tests, die PowerShell-Syntaxprüfung und die Inhaltsprüfung aller 16 gebundenen Dateien bestehen. 18 ausgewählte vollständige Aufnahmen der endgültigen Editor-Variante, der korrigierten Editor-Strecke und des Pakets wurden gesichtet; weitere erzeugte Aufnahmen sind archiviert, aber nicht als einzeln gesichtet ausgewiesen. Die zwölf ursprünglichen Assets und die geprüften Vorgängerpakete bleiben unverändert. Der bereits im vorherigen Bericht offengelegte einzelne überschriebene historische Build-Log bleibt eine Ausnahme; sein alter Nachweis wird nicht umgeschrieben.

Der [Prüfbericht](SHORE_STONES_RESULTS.json) bindet Quellstand, genaue Module, Paketdateien, Laufprotokolle, Bilder und Messwerte per SHA-256. Die ersten beiden Gestaltungsvarianten und die fehlgeschlagenen Inhalts-/Aufnahmeversuche bleiben im lokalen Nachweis erhalten. Die vollständige Überwasser-Referenzqualität ist weiterhin offen.


## Offene Punkte

- Die übrigen Uferabschnitte, Murg und Mols erhalten durch diesen Schritt keine vergleichbare Steinergänzung.
- Fotoabdrücke ausserhalb der gestalteten Maske, Wiesenstruktur, Waldvolumen, einzelne Baumkontakte und die Bergwände bleiben offen.
- Eine vollständige Begehung oder Bekletterung aller Steinoberflächen ist nicht geprüft. Der freigehaltene Treppenzugang wird separat als Spielroute getestet.
- Die bereits dokumentierten horizontalen Wasserbänder und die seitliche Annäherung an Geländer werden durch diese Änderung nicht allgemein behoben.
