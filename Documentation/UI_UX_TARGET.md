# Menüführung, HUD und FPV: Designanforderung

## Auftrag und Reihenfolge

Der Nutzer hat am 26. November 2026 fünf eigene Designentwürfe bereitgestellt. Gewünscht ist eine Anpassung an deren Gestaltungsidee, keine 1:1-Umsetzung. Seine Bewertung des aktuellen Layouts ist ausdrücklich festgehalten: zu chaotisch, unpassende Texte, unsaubere Umlaute und verwirrende Menüführung.

**Status: Anforderung und Originalreferenzen gesichert; Umsetzung noch nicht begonnen.** Die verbindliche Reihenfolge lautet:

1. Spielgrafik und Umgebung unter Wasser optimieren.
2. Spielgrafik über Wasser – insbesondere Berge, Wiesen, Wald, Gelände und Ufer – auf vergleichbares Realitätsniveau bringen.
3. Anschließend Menüführung, HUD und FPV nach dieser Designrichtung überarbeiten.

Dieser Nachtrag ergänzt die bestehenden 236 Anforderungen und das [visuelle Qualitätsziel](VISUAL_TARGET.md). Bestehende Funktionen, Spielmodi, Instrumente, Lokalisierungs- und Bedienanforderungen bleiben Teil des Gesamtziels. Einzelne Ansichten dürfen später sinnvoll zusammengefasst oder anders angeordnet werden, wenn der Ablauf dadurch verständlicher wird.

## Gestalterische Leitlinie

Ruhige, dunkle und leicht transparente Flächen, zurückhaltende türkisgrüne Akzente, klare Abstände und eine erkennbare Schriftgrößenhierarchie verbinden Menü und Spielansicht. Der Tauchort bleibt sichtbar und prägt die Atmosphäre. Informationen erhalten feste, nachvollziehbare Plätze; Warnungen müssen sich klar vom normalen Status abheben. Der Blick auf die Spielwelt bleibt frei.

Die eingebetteten Zahlen, Tasten, Überschriften und Zähler sind Entwurfsinhalte. Sie werden nicht ungeprüft als Simulationswerte, Standardkonfiguration oder Tastenbelegung übernommen. Beispielsweise müssen Gasdruck, Atemreserve und Sauerstoffsättigung eindeutig getrennt und Einheiten konsistent bleiben. Die Live-Test-Regler sowie eventuell sichtbare Zeichen-/Entwurfswerkzeuge sind keine automatische Vorgabe für Bedienelemente im ausgelieferten Spiel.

## Anforderungen und Abnahme

| ID | Anforderung | Nachweis bei der späteren Umsetzung |
| --- | --- | --- |
| UX-REF-01 | Menüinhalte werden in verständliche Themen gegliedert, orientiert an Tauchplan, Wetter/Wasser und Ausrüstung/Tarierung. | Vollständiger Weg von Einstieg/Modus über Konfiguration bis zum Tauchstart im Spiel; jede Einstellung ist auffindbar. |
| UX-REF-02 | Stabile Navigation, klarer aktiver Bereich, eindeutige Hauptaktion sowie vorhersehbares Zurück-/Pause-Verhalten ersetzen verschachtelte oder widersprüchliche Abläufe. | Bedienprüfung mit Maus und Tastatur, sichtbarer Fokus, keine Sackgassen; Rückkehr aus Logbuch, Profilen, Arten/Fotos und Einstellungen. |
| UX-REF-03 | Layout, Textmenge und Schriftgröße passen zusammen. Inhalte überlappen nicht, verschwinden nicht hinter Nachbarfeldern und werden nicht auf unlesbare Größe verkleinert. | Prüfung aller Ansichten mit langen realen Inhalten, verschiedenen unterstützten Auflösungen/Seitenverhältnissen und UI-Skalierungen; gezielter Umbruch bzw. Scrollbereich bei Platzmangel. |
| UX-REF-04 | Deutsche Umlaute und Sonderzeichen werden durchgängig korrekt gespeichert und dargestellt. | Sichtprüfung von ä, ö, ü, Ä, Ö, Ü, ß, ° und verwendeten Einheiten in Menüs, HUD, Hinweisen, Logbuch und exportierten Texten; passende Schriftzeichen statt Ersatzkästchen oder verstümmelter Zeichen. |
| UX-REF-05 | Beschriftungen sind kurz, verständlich und konsistent; vorhandene Sprachvarianten bleiben vollständig bedienbar. | Deutsch/Englisch mit tatsächlich längeren Texten prüfen; keine unübersetzten Platzhalter, widersprüchlichen Begriffe oder abgeschnittenen Einheiten. |
| UX-REF-06 | HUD und FPV besitzen eine klare Informationshierarchie und halten die zentrale Sicht auf Umgebung und Handlung frei. | In Bewegung und mit Ausrüstung prüfen: kompakter Orts-/Zustandsbereich, nachvollziehbare Navigation/Kompass, gruppierte Tauchwerte und sparsame Bedienhinweise. Positionen aus dem Entwurf sind Orientierung, keine festen Pixelvorgaben. |
| UX-REF-07 | Hinweise, Tutorial, Warnungen und geöffnete Instrumente sind aufeinander abgestimmt. Relevante Warnungen bleiben erkennbar, ohne sich gegenseitig oder wichtige Werte zu verdecken. | Gleichzeitige Meldungen, aktives Tutorial, Handgelenk, Karte, Lampe und Pause kombinieren; Priorität, Lesbarkeit, Ausblenden und Wiederaufruf prüfen. |
| UX-REF-08 | Die Gestaltung unterstützt eine übersichtliche Standardansicht und eine reduzierte Darstellung. Umfang und Bedienung der Varianten werden bei der Umsetzung festgelegt. | Standard/kompakte/minimale Zustände anhand der Referenzidee vergleichen; erforderliche Warnungen und modusspezifische Instrumentregeln bleiben wirksam. |
| UX-REF-09 | Tabellen, Diagramme und Ausrüstungsvorschau erklären die tatsächlichen Spielwerte. Bearbeiten derselben Einstellung an mehreren Stellen darf keine abweichenden Zustände erzeugen. | Ausrüstung/Tarierung, Wetter/Wasserprofil und Tauchkonfiguration mit Änderungen und Rücknavigation prüfen; Zahlen stimmen mit dem Simulationszustand überein. |
| UX-REF-10 | Der Gesamteindruck muss die dokumentierten Probleme sichtbar beheben und zur weiterentwickelten Spielgrafik passen. | Vergleichbare echte Aufnahmen und durchgespielte Abläufe im fertigen Unreal-Paket; ein statischer Entwurf oder technische Layouttests allein gelten nicht als Designabnahme. |

## Zusätzlicher FPV-Prüfbefund

Am 27. November zeigt das Abschlussbild der Walenstadt-Leistungsmessung im Paket `WindowsStreamBuild` den eigenen Kopf/Körper gross im Sichtfeld. Die Vergleichsmessung derselben EXE blickt in eine andere Richtung. Ursache und Reproduzierbarkeit dieses Kamerazustands sind noch ungeklärt. Bei der FPV-Überarbeitung sind Eigenkörper-Verdeckung, Kopf-/Kameraabstand und Körperausrichtung während Drehungen, Bodenkontakt und Schwimmen gezielt zu prüfen. Die [gebundenen Messaufnahmen](STREAMING_BUILD_RESULTS.json) bleiben als Befund erhalten; die Meldung ist keine Änderung der vereinbarten Arbeitsreihenfolge.

## Originalreferenzen

Alle fünf Originalbilder liegen unverändert unter `References/`; Dateigrößen und SHA-256-Prüfsummen sind in [UI_REFERENCE_MANIFEST.json](References/UI_REFERENCE_MANIFEST.json) festgehalten. Die Bilder sind Nutzerentwürfe, keine bereits erreichten Spielaufnahmen. Ihre Hintergründe ersetzen keine tatsächliche Spielwelt.

### HUD und FPV

![HUD-Grundansicht als Designreferenz](References/HUD_QualityTarget.png)

![HUD-Zustände und Testansicht als Designreferenz](References/HUD_States_QualityTarget.png)

### Tauchplan und Menüstruktur

![Tauchplan als Designreferenz](References/Menu_DivePlan_QualityTarget.png)

### Wetter und Wasser

![Wetter- und Wasserkonfiguration als Designreferenz](References/Menu_Environment_QualityTarget.png)

### Ausrüstung und Tarierung

![Ausrüstung und Tarierung als Designreferenz](References/Menu_Equipment_QualityTarget.png)
