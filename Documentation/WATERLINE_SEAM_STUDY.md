# Wasserlinie: begrenzte Abschlussprüfung, Iteration 1

Stand: 28. November 2026. **Begrenzte Prüfrunde beendet mit dokumentierten Restpunkten; ursprüngliches Fehlerfrei-Kriterium nicht vollständig erfüllt, keine Paketfreigabe.** Die gestalterische Unterwasser-Abnahme bleibt unverändert. Der [Fünf-Iterationen-Plan](UNDERWATER_FINISH_PLAN.md) wird durch diese Untersuchung nicht um zusätzliche Iterationen erweitert.

## Befund und Änderung

Der schmale helle Saum im festen Fehlerblick bei −2 cm relativ zur Welle entsteht im Wasserreflexionsweg. Ohne Wasseroberfläche verschwindet er; das Abschalten der Postprocess-Materialien beseitigt ihn nicht. Die VSM-Diagnose zeigt auch im Saum die aktive Wasserintegration. Eine zusätzliche Diagnose zeigt dort erhöhte Resttransmission und nach oben gerichtete Reflexionsstrahlen. Die zurückgelegte Strahllänge ist dabei nicht einfach auf wenige Zentimeter verkürzt.

Der Materialkandidat korrigiert Reflexionsrichtungen, die wegen der Detailnormalen auf die falsche Seite des tatsächlich gerenderten Oberflächendreiecks zeigen. Dazu werden kamerarelative Ableitungen verwendet; Normalen- und Fresnelberechnung erhalten dieselbe Korrektur. Die bestehende Rauheit 0,02 bleibt erhalten. Dies reduziert den Saum, entfernt ihn aber **nicht vollständig**. Es handelt sich weder um einen dunklen Bildfilter noch um einen Nachweis, dass der verbleibende Saum ein natürlicher Glanzreflex ist.

Im festen Bildausschnitt x=80…899, y=425…449 sinkt die maximale Anzeigeluminanz im abschliessenden Lauf von 155,25 auf 115,97; die Zahl der Pixel über 80 von 2706 auf 841. Definition: 0,2126 R + 0,7152 G + 0,0722 B, 8-Bit-Bild. Diese Zahlen beschreiben die lokale Änderung und sind kein allgemeiner Qualitätsgrenzwert. Der Rest ist weiterhin sichtbar. Möglicher Zusammenhang: Aufwärtsstrahlen verlassen das gewellte Wasservolumen, während der gegenwärtige Reflexionsweg weitere Wassergrenzflächen nicht vollständig verfolgt. Das ist eine begründete Hypothese, noch kein isolierter Nachweis der vollständigen Ursache.

## Native Nachweise

Alle Ordner liegen lokal unter `artifacts/`; Quellstand, Shader und native Binärdateien sind in den jeweiligen Manifesten gebunden. Die neueren Läufe binden zusätzlich die Material-Assets und den Materialgenerator. Geprüft wird der separate Source-Editor, nicht das bisherige Windows-Spielpaket.

| Ordner `reflection-transport-20261128-…` | Kontrolle und Ergebnis |
| --- | --- |
| `001730` | Normal, ohne Wasseroberfläche, ohne Postprocess-Materialien; je drei feste Wasserhöhen |
| `002038` | DirectLighting-Showflag aus; Saum bleibt. Diese Kontrolle schaltet nicht nachweislich sämtliche Lichtwege ab |
| `002139` | `vsm-coverage`: Saum durchläuft den integrierten Reflexionsweg |
| `002525` | Erste `vsm-path`-Diagnose: R=Transmission, G=Extinktion, B=Strahllänge/200 m |
| `002846` | Überarbeitete `vsm-path`-Diagnose: R=Transmission, G=Extinktion, B=Aufwärtsrichtung; die unterschiedlichen Shader sind in den Manifesten gebunden |
| `003116` | Angleichung der analytischen Wellen an die entfernte Geometrie-Abflachung: keine Verbesserung, zurückgenommen |
| `003415` | Rauheit 0: keine Verbesserung, zurückgenommen |
| `003729` | Erste Normalkorrektur mit absoluten Weltkoordinaten: störende Artefakte, verworfen |
| `004018` | Kamerarelative, numerisch abgesicherte Normalkorrektur: Saum reduziert; alle drei festen Ansichten visuell geprüft |
| `004227` | Zusätzliche Rauheit 0 bringt keine Verbesserung; endgültiger Kandidat wieder 0,02 |
| `004637` | Sechs Nahkontrollen mit/ohne Bildschirmreflexionen; alle Kontaktansichten geprüft. Gespiegelter Grund, Blickfenster zur Sonne und Überwasseransicht erhalten |
| `005748` | Abschliessende drei feste Wasserhöhen mit dem endgültig neu erzeugten Kandidaten und korrigierter Testfixture; Saum weiterhin vermindert, nicht entfernt |

Die sechs Nahkontrollen weichen gegenüber `reflection-transport-20261127-235503` im ganzen Bild im Mittel um 0,18 bis 0,36 RGB-Anzeigestufen ab. Das ersetzt keine bewegte Übergangsprüfung.

## Bewegte Übergänge

`TestReflectionTransport.ps1 -Unified -Profile vsm -Suite sweep` erzeugt an jedem der drei tatsächlichen Uferzugänge eine Welle-relative Kamerafahrt von −20 cm nach +20 cm und zurück. Weesen/Strandbad, Murg/West und Mols/Lediwracks werden anhand ihrer jeweiligen Koordinaten etwa acht Meter seewärts der Uferposition geprüft; Wasserstart Index 2 wird nicht als Mols ausgegeben.

Pro Ort werden 201 native PNGs bei 1600×900 aufgenommen: feste Simulationsschritte von 0,1 s über 20 s, mit weiterlaufenden Wellen. Die Aufnahme dauert in Echtzeit länger. Dies ist keine Leistungsprüfung und kein Mitschnitt jedes dargestellten Bildes. `ReviewWaterlineSweep.py` erzeugt eine zeitlich zugeordnete GIF-Sequenz, eine Kontaktübersicht und die numerischen Änderungen benachbarter Bilder. Die numerischen Änderungen alleine entscheiden nicht über sichtbare Fehler.

Der erste Versuch `reflection-transport-20261128-004837` wurde durch die allgemeine Smoke-Zeitbegrenzung vorzeitig beendet und gilt als **fehlgeschlagen**, nicht als Grafikabsturz. Die Fels-/Wasser-Bildfixture besitzt jetzt ihren eigenen Abschluss; allgemeine Smoke-Bilder und automatische Beendigung greifen dort nicht zusätzlich ein. Der korrigierte Projekt-Build ist erfolgreich (`reflection-shadow-study/sweep-fixed-clock-build.log`).

Die Wiederholung `reflection-transport-20261128-005352` besteht alle drei Aufnahmeläufe mit zusammen 603 PNGs. Der Vergleich `reflection-transport-20261128-005906` verwendet das ursprüngliche Material und erzeugt ebenfalls 603 PNGs. Die protokollierten Kamera- und Wellenzustände werden bildweise auf Identität geprüft. Das ursprüngliche Material wurde nur für die Gegenprobe eingesetzt; anschliessend wurde der Kandidat wiederhergestellt.

Die Kontaktübersichten zeigen an allen drei Orten den vollständigen Auf-/Abtauchvorgang. In Murg und Mols sind bei Simulationszeit 5,0 s breite blaue Streifen unmittelbar an der Grenze sichtbar; Originalbilder mit dem bisherigen Material zeigen dieselbe Einschränkung. Auch der schmale helle Saum bleibt bei einzelnen Übergangsbildern sichtbar. Ein bestandener Aufnahmelauf bedeutet deshalb **keine bestandene Fehlerfrei-Abnahme**. Visuell geprüft werden die Kontaktübersichten und die grössten Bildabweichungen; die GIFs sind zeitlich zugeordnete Wiedergaben der 201 Stichproben, keine manuelle Vollprüfung sämtlicher präsentierter Frames.

Die bildweisen Vergleiche bei 800×450 ergeben im Mittel 0,407 RGB-Anzeigestufen in Weesen, 0,519 in Murg und 0,501 in Mols. Die grössten Einzelbildmittel sind 2,033 / 2,959 / 2,721. Die jeweils sechs grössten Abweichungen wurden nebeneinander visuell geprüft; sie zeigen Unterschiede der zeitlichen Reflexionsrekonstruktion beim Auftauchen, keine neue grossflächige Ersatzfläche durch die Normalkorrektur. Dies ist eine begrenzte Regressionskontrolle, keine Behauptung allgemeiner Fehlerfreiheit.

Der [maschinenlesbare Bericht](WATERLINE_SEAM_RESULTS.json) bindet Manifeste, Materialstände, Kameraprotokolle und Bildprüfsummen. Die 43 vorhandenen Python-Prüfungen bestehen. Diese Kontrollen ersetzen weder Paket- noch Dauerbetrieb.

## Fester Fehlerblick

Vorher:

![Ursprünglicher heller Saum](Images/WaterlineSeam_Before.png)

Teilweise korrigiert, Restsaum weiterhin sichtbar:

![Verminderter, nicht vollständig behobener Saum](Images/WaterlineSeam_Partial.png)

## Noch offen

Der Restsaum und die kurzen Grenzflächenstreifen erfüllen das ursprüngliche vollständige Abschlusskriterium von Iteration 1 nicht. Sie werden nicht als behoben bezeichnet. Gemäss Nutzervorgabe, offene Punkte für später zu dokumentieren und nach fünf begrenzten Runden weiterzugehen, werden sie ausdrücklich unter UW-LATER-07 vertagt. Die Prüfrunde wird damit **mit Abweichung**, nicht als vollständig fehlerfreie Umsetzung beendet. Eine weitergehende Lösung darf den bewusst zurückgestellten vollständigen sekundären Grenzflächenweg (UW-LATER-02) nicht stillschweigend zur neuen grossen Forschungsaufgabe machen. Als nächstes folgt die vereinbarte Iteration 2 zu Licht und Schatten.

Das startbare Paket `Builds/WindowsWaterline/Start-WaterPreview.cmd` bleibt unverändert. Der hier beschriebene Materialkandidat und der neue Reflexionsweg sind noch nicht in diesem Paket enthalten.
