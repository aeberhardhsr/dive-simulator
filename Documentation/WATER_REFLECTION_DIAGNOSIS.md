# Herkunft der blauen Unterwasserreflexion

Die blaue Fläche unmittelbar unter der Wasseroberfläche stammt im geprüften Weesen-Blick aus dem Himmels-Ersatzwert von Lumen. Die betreffenden Reflexionsstrahlen zeigen dabei nach unten. Es handelt sich somit nicht um den direkt durch die Oberfläche gesehenen Himmel. Die Darstellung benötigt eine Berechnung des Wassermediums entlang des reflektierten Lichtwegs. **Diese Untersuchung liefert die Diagnose, noch keine Korrektur im Spielpaket.**

Der zuvor korrigierte [Kameramaterial-Übergang](WATERLINE_BLEND.md) bleibt davon getrennt. Das aktuell spielbare Paket ist weiterhin `Builds/WindowsWaterline/Start-WaterPreview.cmd`.

## Native Vergleichsbilder

Gewöhnliche Wasservorschau, 20 cm unter der eingefrorenen Welle:

![Gewöhnliche Wasservorschau mit blauer Reflexionsfläche](Images/WaterReflection_Normal.png)

Diagnose ohne Bildschirmreflexionen: Magenta markiert den primären Himmels-Ersatzwert, Grün ein aufgelöstes Reflexionsergebnis.

![Himmels-Ersatzwert magenta markiert](Images/WaterReflection_classification.png)

Zweiter Diagnoselauf: Rot markiert nach unten gerichtete Strahlen ohne Treffer; nach oben gerichtete Fehlstrahlen würden Cyan erscheinen. Aufgelöste Ergebnisse bleiben Grün.

![Nach unten gerichtete Fehlstrahlen rot markiert](Images/WaterReflection_direction.png)

Alle Bilder sind unveränderte native Aufnahmen. Die Diagnosefarben ersetzen die Strahldichte im Reflexionsshader und sind ausdrücklich keine vorgeschlagene Gestaltung.

## Verfahren und Grenzen

`PrepareReflectionRayStudy.py` kopiert die Lumen-Shader in das ignorierte Projektverzeichnis `Intermediate/DiveReflectionRayStudy`. Nur diese Kopie wird geändert. Die installierte Engine bleibt unverändert; die SHA-256-Prüfung begrenzt den Versuch auf die untersuchte Shaderrevision von UE 5.8.4. Die Zuordnung wird ausschliesslich mit `DiveReflectionRayStudy`, `DiveSmokeTest` und `DiveWaterPreview` aktiviert. Der gewöhnliche Start erhält sie nicht. Gleichzeitige Solar-Shaderexperimente werden abgewiesen.

Die Instrumentierung sitzt in `LumenReflectionHardwareRayTracing.usf` vor dem Schreiben von `RWTraceRadiance`. Sie merkt sich, ob `ApplySkyLight != 0 && !Result.bIsHit` den Himmels-Ersatzwert auslöst. Im Richtungsmodus wird zusätzlich das Vorzeichen von `Ray.Direction.z` ausgewertet. Grün bedeutet ein aufgelöstes Ergebnis: je nach Pfad kann das Geometrie, ein entfernter Bildschirmtreffer oder ein Cache sein. Sekundäre Himmelsbeiträge innerhalb rekursiver Trefferauswertung werden nicht getrennt markiert.

Beide Serien vergleichen gewöhnliche Wasservorschau, Diagnose und Diagnose ohne Bildschirmstrahlen an denselben drei Kamerapositionen: −20 cm, −2 cm und +20 cm relativ zur lokalen Welle. Tageszeit, Wellenzeit und Blick sind fixiert. Der [Ergebnisbericht](WATER_REFLECTION_DIAGNOSIS_RESULTS.json) bindet Quellenmanifeste, Module, Protokolle und 18 Rohbilder. Die bildliche Aussage ist auf diesen Blick begrenzt; sie ist keine allgemeine Abnahme aller Tauchorte.

Im festen Bereich x=400–1199, y=220–359 der beiden Unterwasserbilder ist der Himmels-Ersatzwert nahezu flächendeckend markiert. Die Auswertung verlangt mindestens 99 % passende Diagnosefarbe. Gemessen werden präsentierte 8-Bit-Bildwerte, keine radiometrischen Grössen. Der ursprüngliche Klassifikationslauf liegt in `artifacts/waterline-20261127-222103`, der Richtungslauf in `artifacts/waterline-20261127-222746`.

## Konsequenz für die Umsetzung

Der geprüfte Lumen-Pfad ruft für Reflexionsnebel `GetFogOnLuminance` in `LumenTracingCommon.ush` auf. Diese Funktion integriert den allgemeinen exponentiellen Höhennebel ab dem Reflexionsursprung; sie integriert nicht das räumlich begrenzte Wasservolumen entlang dieses Strahls. Eine reine Verlängerung der geometrischen Suchdistanz würde deshalb den fehlenden Wassertransport nicht lösen.

Die nächste Umsetzung muss die am Strahlende gefundene Strahldichte mit der Transmission des Wasserwegs multiplizieren und das entlang desselben Wegs eingestreute Licht addieren. Nur Absorption würde den Himmelsfehler durch eine unphysikalisch schwarze Fläche ersetzen. Dabei sind Wellenoberfläche, Seegrenze, Sichtweite, spektrale Absorption und die Beleuchtung entlang des Wegs zu berücksichtigen. Kameraweg und Reflexionsweg dürfen nicht doppelt integriert werden.

Der Einstieg ist auf die Unterseitenreflexion der vordersten transparenten Wasserschicht zu begrenzen (`ReflectionPass == 2` plus tatsächliche Wasserzugehörigkeit). Diese Passkennung allein identifiziert noch kein Wasser. Ein Eingriff in die gemeinsame Nebelfunktion würde auch andere Lumen-Pfade beeinflussen. Bildschirmtreffer, Radiance-Cache-Ergebnisse und Hardwaretreffer müssen am Ende konsistent behandelt werden.

Die bereits integrierte Kameranebeltextur kann nicht unverändert als Lichtfeld für beliebige Reflexionsrichtungen verwendet werden: Sie ist an das Kamerafrustum und die kamerabezogene Streurichtung gebunden. Ihre Nutzung wäre eine gesondert zu prüfende Näherung. Für eine belastbare Lösung sind deshalb zusätzliche Medium-/Lichtdaten oder eine eigene Auswertung entlang des Reflexionsstrahls nötig.

Abnahme der kommenden Korrektur: Erhalt naher Felsspiegelungen; zunehmende Dämpfung mit Wasserweglänge; Reaktion auf Sichtweite, Tageszeit und Lampe; unveränderte Luftspiegelungen; stetiger Wasserübergang; Vergleich mit und ohne Bildschirmstrahlen; anschliessend Bildratenmessung. Mehrfache Grenzflächenwechsel bleiben eine zusätzliche Anforderung, solange das sichtbare wellenverformte Wassermesh nicht entsprechend im Raytracing vertreten ist.

## Reproduktion

```powershell
./Tools/Build.ps1 -Target Editor -EditorSandbox -MaxParallelActions 4
./Tools/TestWaterline.ps1 -Study reflection -ReflectionMode classification
./Tools/TestWaterline.ps1 -Study reflection -ReflectionMode direction
Tools/.geodata-env/Scripts/python.exe Tools/ReportReflectionRays.py --classification artifacts/waterline-20261127-222103 --direction artifacts/waterline-20261127-222746
```

Die beiden Testaufrufe müssen nacheinander laufen, da sie dieselbe lokale Shaderkopie vorbereiten. Für neue Läufe sind dem Berichtsaufruf deren ausgegebene Verzeichnisse zu übergeben. Es wurde kein neues Paket und keine Leistungszusage aus dieser Diagnose abgeleitet.
