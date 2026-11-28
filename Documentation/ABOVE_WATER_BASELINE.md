# Überwasserlandschaft: Ausgangsstand und erster Arbeitsschritt

Stand: 28. November 2026. Nach den [fünf begrenzten Unterwasser-Runden](UNDERWATER_FINISH_HANDOVER.md) liegt der Schwerpunkt auf der Landschaft über Wasser. **Die sechs Ausgangsansichten sind aufgenommen und gesichtet; eine grafische Verbesserung ist in diesem Bericht noch nicht umgesetzt.**

## Reproduzierbarer Ausgangsstand

Unverändertes Paket `WindowsUnderwaterFinalCandidateR3`, normaler Hardware-/VSM-Wasserpfad, 1600 × 900, Szenariodatum 21. Juni 2026 um 12:00 Uhr, Begrenzung auf 30 FPS für die Aufnahmen. Je zwei feste Ansichten an Weesen/Strandbad, Murg/West und Mols/Lediwracks: Richtung See und um 180° zurück zum Land. Alle sechs nativen Prüfungen bestehen; Uferanschlüsse, Kollision und Renderkonfiguration sind geprüft. Alle 53 Laufzeitdateien stimmen weiterhin mit R3 überein. Diese Reihe misst keine Bildrate und prüft keine bewegte Kamerafahrt.

[Protokolle, Originalbilder, Prüfsummen und genaue Grenzen](ABOVE_WATER_BASELINE_R3.json). Die lokalen Originalbilder liegen unter `artifacts/above-water-baseline-20261128/`; die grossen Laufzeitartefakte werden nicht als Git-Quellen behandelt.

| Blick | Sichtbarer Befund |
| --- | --- |
| Weesen → See | Erkennbare Bergsilhouette, jedoch weiche geglättete Felsflächen und dunkle vertikal gestreckte Strukturen; Zugangssteg ohne differenzierte Materialoberfläche |
| Weesen → Land | Räumliche Bäume und Schatten vorhanden; Boden dazwischen grossflächig eben wirkend, helle/verwaschene Luftbildflächen und abrupter Materialwechsel am Weg |
| Murg → See | Besonders deutliche senkrechte Farbstreifen an den gegenüberliegenden Felsbändern; ferner Wald überwiegend als aufliegendes Luftbild |
| Murg → Land | Gleichförmige Felsoberfläche an der steilen Böschung, grobe Übergänge zu Boden/Weg und neutral wirkende Gebäudeflächen |
| Mols → See | Starke Streckung der Bergoberflächen, flacher ferner Wald, verwaschene dunkle Flächen im nahen Uferbereich |
| Mols → Land | Terrassenartige Böschung mit breiten horizontalen Kanten; gleichförmige Felsstruktur und wenig räumliche Gras-/Bodenstruktur |

Die Bilder zeigen Defizite gegenüber der [Nutzerreferenz](VISUAL_TARGET.md). Die lokale geometrische Ursache der Böschungskanten ist noch nicht isoliert; die numerisch bestandene Anschlussprüfung macht diese Formen nicht automatisch natürlich oder korrekt gestaltet.

## Technisch eingegrenzter erster Schritt: entfernte Felsoberflächen

`Tools/Shaders/GroundDetail.ush` blendet die räumlichen Bodendetails derzeit zwischen 25 und 120 Metern aus. Dahinter bleibt das geografisch registrierte Luftbild. Die Berglandschaft verwendet `M_LandscapePhoto` auf einem 8-m-Raster (`DiveLandscape.cpp`). Das erklärt, weshalb eine ausschliessliche Verbesserung der Nahtexturen die gegenüberliegenden Bergflanken nicht erreicht. Die konkrete Bildursache wird im folgenden Vergleich isoliert; ein Materialwechsel ersetzt keine fehlende Geländeform.

Der erste begrenzte Überwasser-Versuch erhält eine eigene Landschaftsmaterial-Variante: neigungsabhängige, räumlich projizierte Felsstruktur auch in der mittleren und grossen Entfernung, mit geografisch gebundener Farbgliederung. Zuerst wird der Murg-Seeblick unter identischer Kamera und Beleuchtung verglichen. Die abgenommene Unterwasserdarstellung und der eingefrorene R3-Kandidat bleiben als Vergleich erhalten.

Dieser Versuch gilt nur als Verbesserung, wenn die senkrechte Farbstreckung im Murg-Bild sichtbar abnimmt, keine neue regelmässige Texturkachelung dominiert und Vegetations-/Felsgrenzen weiterhin plausibel bleiben. Weesen und Mols dienen als Kontrollansichten. Vor Übernahme folgen ein bewegter Uferblick, ein Vergleich bei anderem Sonnenstand und eine Leistungsmessung; ein erfolgreicher Shader-Build allein genügt nicht.

## Anschliessende Schwerpunkte

1. Gelände- und Felsformen sowie lokale Böschungs-/Uferanschlüsse anhand vorhandener Höhendaten prüfen; erst belegte Formdefizite korrigieren.
2. Waldvolumen über Nah- und Fernbereich verbinden. Die bisherige Vegetation endet ungefähr bei 1,15 km; jenseits davon bleibt das Luftbild. Das ist eine eigenständige Aufgabe, keine durch Felsmaterialien gelöste Lücke.
3. Wiesen, Waldboden, Wege und Zugangsmaterialien räumlich und im richtigen Massstab ausarbeiten; danach Licht und atmosphärische Entfernung gemeinsam abstimmen.

Diese Reihenfolge ist eine Arbeitspriorisierung, keine zusätzliche Serie von fünf Unterwasser-Runden und keine bereits erfüllte Überwasser-Abnahme.
