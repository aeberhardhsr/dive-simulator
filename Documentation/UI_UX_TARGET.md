# Menüführung, HUD und FPV: Designanforderung

## Auftrag und Reihenfolge

Der Nutzer hat am 26. November 2026 fünf eigene Designentwürfe bereitgestellt. Gewünscht ist eine Anpassung an deren Gestaltungsidee, keine 1:1-Umsetzung. Seine Bewertung des aktuellen Layouts ist ausdrücklich festgehalten: zu chaotisch, unpassende Texte, unsaubere Umlaute und verwirrende Menüführung.

**Status: Umsetzung am 27. November 2026 begonnen; native Prüfung noch ausstehend.** Der Nutzer hat die Reihenfolge am 27. November ausdrücklich angepasst: Menü/HUD/FPV darf während des laufenden Unreal-Erstbuilds vorgezogen und parallel vorbereitet werden. Die ursprünglich vereinbarte Reihenfolge war:

1. Spielgrafik und Umgebung unter Wasser optimieren.
2. Spielgrafik über Wasser – insbesondere Berge, Wiesen, Wald, Gelände und Ufer – auf vergleichbares Realitätsniveau bringen.
3. Anschließend Menüführung, HUD und FPV nach dieser Designrichtung überarbeiten.

Die neue Priorisierung erlaubt jetzt die UI-Arbeit im Hauptprojekt. Der bereits laufende Engine-Build verwendet seine getrennte Quellkopie und wird nicht neu gestartet. Nach seinem Abschluss müssen die geänderten Projektmodule ausdrücklich neu gebaut werden, bevor UI- oder Grafiktests mit dem synchronisierten Projekt starten. Die grafischen Qualitätsziele bleiben vollständig bestehen.

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

Am 27. November zeigt das Abschlussbild der Walenstadt-Leistungsmessung im Paket `WindowsStreamBuild` den eigenen Kopf/Körper gross im Sichtfeld. Die Vergleichsmessung derselben EXE blickt in eine andere Richtung. Ursache und Reproduzierbarkeit dieses Kamerazustands sind noch ungeklärt. Bei der FPV-Überarbeitung sind Eigenkörper-Verdeckung, Kopf-/Kameraabstand und Körperausrichtung während Drehungen, Bodenkontakt und Schwimmen gezielt zu prüfen. Die [gebundenen Messaufnahmen](STREAMING_BUILD_RESULTS.json) bleiben als Befund erhalten. Die aktuelle Priorisierung steht im Abschnitt „Auftrag und Reihenfolge“.

## Umsetzungsstand während des Engine-Builds

Im Hauptprojekt vorbereitet, **noch nicht nativ kompiliert oder visuell abgenommen**:

- Drei feste Hauptbereiche mit gemeinsamer Navigation und Hauptaktion; getrennte Pause mit Übersicht aller aktiven Warnungen. Konfiguration bleibt während des laufenden Tauchgangs gesperrt.
- Gemessener Textumbruch, begrenzte Textflächen, sichtbarer Tastaturfokus, Tab/Pfeiltasten und Enter/Leertaste sowie vorhersehbare Esc-Rückkehr. Deutsche HUD-/Menübeschriftungen wurden korrigiert.
- Standard-, Kompakt- und Minimal-HUD mit gespeicherter Auswahl. Ortsstatus links, Kompass oben, gruppierte Instrumente unten, priorisierte Warnungen und einklappbares Tutorial rechts. Die Spielkarte lässt diese Bereiche frei.
- Ausrüstungstabelle und vorhandene 3D-Vorschau im neuen Raster; Werte stammen weiterhin aus der tatsächlichen Konfiguration beziehungsweise dem pausierten Simulationszustand.
- Wetter-/Wasserseite mit Temperatur-Tiefen-Kurve aus `Dive::Model::Temperature`, passender geplanter Tiefe und manuell gesetzter Sprungschicht. Die Tiefenachse umfasst je nach Plan 40–60 m, die Temperaturachse bleibt zum Vergleichen bei 0–30 °C. Automatische Profile verwenden dieselbe saisonale Interpolation wie die Simulation; es sind keine Live-Messwerte. Die Änderung ist noch nativ zu prüfen.
- Anzeigen und Eingabehinweise verwenden ° und °C; das portable Speicher-/Eingabeschema bleibt unverändert. Weitere deutsche Spielhinweise wurden auf korrekte Umlaute geprüft und berichtigt.

`DiveUISmoke.cpp` und `Tools/SmokeTest.ps1 -Scenario ui` bereiten einen nativen Test mit 14 Zuständen vor. Aufbau und Bildschirmaufnahme erfolgen in getrennten Schritten. Der Ablauf prüft Tastatureingaben über den PlayerController, vorhandene Hitboxen, Zurück-Navigation, Dezimalkomma-Eingabe und Speicherung der HUD-Auswahl. Er erstellt DE-/EN-Aufnahmen für Menü, Dialog, Steuerung und HUD einschließlich Karte, Tutorial und 13 künstlich gleichzeitig gesetzter Warnungen. Die Simulation wird nur in diesem expliziten UI-Test eingefroren. Die Warnungsfixture ist kein Nachweis für Tauchphysik oder medizinische Modellgüte.

Erforderliche nächste Prüfungen nach Ende des Engine-Builds:

1. Projektquellen in `SourceEditorSandbox` synchronisieren und dessen Editor-Projektmodule neu bauen. Der bereits laufende Erstbuild enthält eine ältere Projektkopie.
2. `SmokeTest.ps1 -Scenario ui -EditorSandbox -SandboxName SourceEditorSandbox -EngineRoot Builds/UE584 -Viewport 1600x900` ausführen; danach auch 1920x1080 und 1280x1024. Nur frische Bilder und vollständige Phasenprotokolle gelten. Erkannte Kürzung der Fixture-Texte lässt den Test fehlschlagen.
3. Bilder tatsächlich ansehen; insbesondere Warnung/Tutorial/Karte, vollständige Einheiten, Sonderzeichen, Credits und den Textdialog prüfen. Der automatische Test erkennt keine allgemeine grafische Überlappung und ersetzt diese Prüfung nicht.
4. Bestehende Konfigurations-, Ausrüstungs-, Karten-, Logbuch-, Profil-, Entdeckungs-, Steuerungs- und Tutorialtests wiederholen; echte Mausbedienung sowie lange Nutzernamen prüfen. Danach FPV-Kopf-/Körperverdeckung mit vorhandenen Avatar-Tests und Bewegung reproduzieren und korrigieren.

Bisher geprüft: PowerShell-Syntax des erweiterten Teststarters und `git diff --check`. Die GitHub-Prüfungen für den UI-Vorbereitungsstand `a557cd8` sind erfolgreich (Push `46013717545`, PR `46013728731`); sie prüfen die portable Suite, nicht die Unreal-Oberfläche. C++-Kompilierung, native UI-Aufnahmen und FPV-Abnahme stehen aus. Das zuletzt spielbare Paket wurde nicht ersetzt.

### FPV-Korrektur vorbereitet, native Reproduktion ausstehend

Das tatsächlich betrachtete Fehlerbild liegt unter `artifacts/perf-underwater-1080p-q2-20261127-094553-953/Frame.png`. Im zugehörigen `Runtime.log` springt die Blickanforderung bei einer blockierten Körperdrehung wiederholt zwischen den Halsgrenzen. Zwei im Quellcode nachvollziehbare Probleme wurden eingegrenzt:

- Die Kamera verwendet `APawn::GetViewRotation()` und bisher damit direkt die Controller-Rotation. Neue Eingaben können auf einem Renderframe eintreffen, bevor der nächste feste 20-ms-Kollisionsschritt die Kopfhaltung freigibt. Dann stimmen Kamerablick und gerenderter Körper nicht überein. Das Projekt überschreibt nun `GetViewRotation()` während des Tauchgangs mit `PhysicalPose.Gaze`; auch Kamerakomponente und HUD-Kompass verwenden diese akzeptierte Richtung. Der erweiterte Test `DiveSim.Contacts.BlockedBodyAllowsHeadTurn` prüft ausdrücklich einen solchen Zwischenframe und die Pause.
- Der Leistungstest schrieb pro Frame eine absolute Umlauf-Richtung, obwohl die Behandlung blockierter Drehungen weitere Eingaben als Differenz zur zuletzt akzeptierten Richtung aufnimmt. Er speiste dadurch dieselbe blockierte Anforderung mehrfach ein. Der Umlauftest gibt nun inkrementelle Drehung mit 6 Grad pro simulierter Sekunde ein; seine Anfangsneigung wird einmalig gesetzt. Streaming- und Soak-Steuerung bleiben separat. Neue Performance-Metadaten enthalten Kamera-, Controller- und physische Blickrichtung, Augenposition und blockierte Haltungsschritte.

Diese Änderungen sind noch **nicht nativ kompiliert oder im Spiel geprüft**. Sie beweisen noch nicht, dass jede Ursache der sichtbaren Verdeckung beseitigt ist. Nach dem Build sind der Kontakttest, Avatar-Blicktests, eine Walenstadt-Wiederholung und schnelle reale Mausdrehungen erforderlich. Frühere Bildratenmessungen mit anderer Blickführung dürfen nicht als Vergleich derselben sichtbaren Szene verwendet werden.

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
