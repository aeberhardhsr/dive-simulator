# Aktuelles spielbares Windows-Paket

Aktueller Nachfolger: **`Builds/WindowsExposure/Start-WaterPreview.cmd`** mit aufgeteilter Zellvorbereitung und korrigierter Startbelichtung. [Belichtungsvergleich und Paketprüfung](EXPOSURE_START.md), [vorherige Leistungsmessungen](POPULATION_PREPARATION.md). Der folgende Bericht dokumentiert weiterhin unverändert das Vergleichspaket `WindowsCurrent` und dessen UI-/Ausrüstungsprüfungen.

**Start: `Builds/WindowsCurrent/Start-WaterPreview.cmd`.** Das gesamte Verzeichnis wird benötigt. Das Paket enthält die überarbeitete Menüführung, Standard-/Kompakt-/Minimal-HUD, korrigierte deutsche Beschriftungen, die tiefere entspannte Armhaltung und die freigestellte Hinweisposition beim Ablesen des Handgelenkcomputers. Die bisherige Wasservorschau ist eingeschaltet. Die neuen experimentellen Licht- und Seegrenzenverfahren der Quellcode-Engine bleiben getrennt.

Das Paket wurde mit der installierten Unreal Engine 5.8.4 gebaut. Die selbst gebaute Quellcode-Engine ist erfolgreich kompiliert, aber nicht die Grundlage dieses Pakets. Die [Grafikreferenz](VISUAL_TARGET.md) und die vollständige Design-/Anforderungsabnahme sind weiterhin offen.

## Paketprüfung vom 27. November 2026

- Erste Paketfassung: Menü/HUD bei 1600 × 900, 1920 × 1080 und 1280 × 1024; jeweils 28 Eingabe-/Zustandsschritte und 14 frische Bilder, kein gemeldeter Textüberlauf. Alle 42 Bilder wurden als Kontaktbögen betrachtet; ausgewählte schmale Originalaufnahmen zusätzlich vollständig gelesen.
- Sechs Ausrüstungsvarianten: 150 Phasen und 66 Aufnahmen bestanden. Alle Kontaktbögen und die erste Originalaufnahme des Handgelenkcomputers wurden betrachtet. Die Prüfungen enthalten Blick nach oben/unten, Ablesen, Pause, Regler und die Ausrüstungsvorschau. Sie ersetzen keine Abnahme sämtlicher realer Mausbewegungen oder Kollisionssituationen.
- Danach wurden sechs Winkelbeschriftungen in Steuerung, Karte, Profilen und Wegpunkten von `deg` auf `°` vereinheitlicht. Nur vier UI-Quelldateien änderten sich; **alle fünf Inhaltsarchive sind bytegleich**. Die zweite EXE ist separat gebunden. Der erneute 1280-×-1024-Lauf besteht 28 Schritte und 14 Aufnahmen; die korrigierte Steuerungsseite wurde im Original gelesen.
- In der endgültigen Paketfassung bestanden Profile, Wegpunkte, Einstiegsauswahl, Uferroute und Tauchablauf. Die Aufnahmen dieser fünf Läufe wurden betrachtet. Die Uferroute enthält Einstieg, Abtauchen, Auftauchen, Rückkehr und Abschluss; sie ist eine programmierte Prüfung.
- Der normale Start über die Haupt-EXE öffnet das Menü mit aktiver Wasservorschau. Die anschliessende reale Mausprüfung wurde zweimal durch das darüberliegende Windows-Fenster `PickerHost.exe` zurückgewiesen. Ein erneutes Aktivieren und frisches Bild änderten dies nicht. Es kam kein erfolgreicher Klick zustande. Die eigens gestartete Testinstanz wurde anschliessend geschlossen; der Nutzer wurde um Beenden der Auswahloberfläche gebeten.

Die nativen UI-Tests verwenden simulierte Tasteneingaben beziehungsweise HUD-Hitbox-Aufrufe. Sie sind kein Ersatz für die noch offene reale Mausprüfung. Weitere offene UI-Arbeiten sind lange Nutzernamen, zusätzliche Skalierungen und einzelne Temperaturbeschriftungen in Profil-/Logbuchansichten, die noch `C` statt `°C` anzeigen. Die Hauptseiten, Steuerung und geprüften HUD-Werte enthalten lesbare Umlaute und Sonderzeichen.

## Bildrate

Je 60 Sekunden Messintervall, RTX 3090 / Ryzen 9950X3D / 32 GB RAM, 1920 × 1080, hohe Qualität (Stufe 2), 100 % Auflösung, gewöhnliche Wasservorschau, keine Bildratenbegrenzung. Während dieser Messungen liefen weder Compiler noch Unreal-Editor. Andere Systemlast wird im Messbericht festgehalten.

| Szene | Durchschnitt FPS | 1-%-Low FPS | Längster Frame | Frames über 33,33 ms |
| --- | ---: | ---: | ---: | ---: |
| Unterwasser, Walenstadt | 75.92 | 54.51 | 32.85 ms | 0 / 4554 |
| Oberfläche, Weesen | 67.66 | 58.84 | 18.31 ms | 0 / 4058 |
| Nachladen beim Schwimmen, Walenstadt | 79.61 | 30.24 | 205.64 ms | 13 / 4775 |
| Nachladen beim Schwimmen, Walenstadt (Wiederholung) | 79.12 | 29.83 | 205.90 ms | 14 / 4746 |

Die Werte gelten für diese programmierten Wege und den aufgezeichneten Rechnerzustand. Durchschnittswerte sind keine Zusicherung durchgehender 60 FPS. Die Abschlussbilder wurden betrachtet; ein Bild allein beweist nicht den gesamten Bewegungspfad. Frühere Editor- und Paketmessungen mit abweichender Blickführung sind kein direkter Geschwindigkeitsvergleich.

**Offener Nachladeruckler:** Beide Streaming-Läufe enthalten nach rund 106 m Gesamtdistanz einen Frame von etwa 206 ms. Das ist mit dieser Wiederholung nicht als einmaliger Kaltstarteffekt erklärt. Im vorausgehenden CPU-Sample liegen 148,02 beziehungsweise 148,98 ms im Timer `DivePopulation/GameThread/PrepareCell`; der gesamte Game-Thread benötigt etwa 205 ms, während die GPU bei etwa 12–13 ms bleibt. Die CSV-Framezeit zeigt die davor ausgeführte Arbeit zeitversetzt. Die übergeordneten Timer sind nicht zu addieren. Weitere langsamere Frames fallen auf Registrierung von Instanzkörpern und Physik-Wartezeiten. Nächster Optimierungsschritt: die bisher synchrone Vorbereitung der Fels-/Pflanzenverteilung ebenfalls portionieren und danach diesen Weg erneut messen. Die 30-FPS-Grenze ist auf dieser Streaming-Strecke ausdrücklich nicht durchgängig bestanden.

## Nachweise und Reproduktion

Der [gebundene Ergebnisbericht](PLAYABLE_CURRENT_RESULTS.json) enthält beide EXE-/Archivstände, Quellprüfsummen, Buildprotokolle, Bilder, Funktionsprotokolle und Messdaten. Die Inhaltsaufbereitung meldete bei fünf aus der Quellcode-Engine gespeicherten Materialien eine leere Engine-Versionsangabe; der Paketbau war erfolgreich und die tatsächlichen Materialien wurden in den Laufprüfungen verwendet. Das ist kein Versprechen allgemeiner Kompatibilität mit anderen Engine-Versionen.

Die Paketprotokolle enthalten ausserdem eine Startfehlermeldung des CelestialVault-Standardobjekts: `MI_StarsStar_EnergyConservative` wird nicht gefunden. Die geprüften Spielabläufe laufen durch; die Meldung ist im Ergebnisbericht erhalten und wird nicht als fehlerfreier Start gewertet. Die konkrete Auswirkung auf den Sternenhimmel bei Nacht bleibt gesondert zu untersuchen.

```powershell
./Tools/Build.ps1 -EditorSandbox -Target Package -BuildSlot Current -PackageDirectory Builds/WindowsCurrent -MaxParallelActions 4
./Tools/TestPlayableUI.ps1 -PackageDirectory Builds/WindowsCurrent -WaterPreview
./Tools/TestAvatar.ps1 -PackageDirectory Builds/WindowsCurrent -WaterPreview
./Tools/SmokeTest.ps1 -Packaged -PackageDirectory Builds/WindowsCurrent -WaterPreview -Scenario shore
./Tools/Benchmark.ps1 -PackageDirectory Builds/WindowsCurrent -WaterPreview -Scenario underwater -Entry 2 -Resolution 1080p -Quality 2 -Seconds 60
```

## Native Aufnahmen

![Menü im endgültigen Paket](Images/Current_Menu.png)

![Korrigierte Winkelanzeige](Images/Current_Controls.png)

![Ablesen des Handgelenkcomputers, vor der abschliessenden Winkeltextkorrektur](Images/Current_Wrist.png)
