# Belichtung beim Einstieg

**Aktuelle spielbare Vorschau: `Builds/WindowsExposure/Start-WaterPreview.cmd`.** Das gesamte Paketverzeichnis wird benötigt. Es enthält die bisherigen Menü-/HUD-/FPV-Anpassungen und die aufgeteilte Zellvorbereitung. Neu beginnt die Kamera beim gewählten Szenenlicht: Der direkte Tageseinstieg zeigt keine zunächst fast weisse Unterwasseransicht mehr; beim Nachteinstieg ist der Lampenkegel sofort erkennbar.

## Ursache und Änderung

Beim direkten Start interpolierte die Kamera vom Standardwert zur Belichtung des tatsächlichen Standorts. Im geprüften Tagesfall lag der erste Wert dadurch rund 5,19 Belichtungsstufen über dem Ziel. Jetzt initialisiert der erste Kameratick nach `StartDive` die Belichtung am tatsächlichen Augenpunkt. Die anschliessende kontinuierliche Anpassung während des Tauchens bleibt erhalten. Materialien, Lichtquellen und Gelände wurden dabei nicht geändert.

Die alte Initialisierung bleibt ausschliesslich als Diagnoseoption für automatische Vergleichsläufe verfügbar. Vorher und nachher wurden innerhalb derselben neuen EXE geprüft. Alle fünf Inhaltscontainer sind zudem bytegleich mit `WindowsPrepare`; unter den gemeinsam erfassten Spielquellen änderten sich nur `DivePawn.cpp` und `DivePawn.h`.

Vorher, erster Frame beim direkten Tageseinstieg:

![Bisherige Startbelichtung](Images/ExposureStart_Before.png)

Nachher, gleiche protokollierte Kameraposition und Blickrichtung:

![Initialisierte Startbelichtung](Images/ExposureStart_After.png)

## Paketprüfung vom 27. November 2026

Acht bestandene Fälle mit 38 betrachteten Aufnahmen: Tag vorher/nachher, Nacht mit Lampe vorher/nachher, Nacht ohne Lampe, Uferroute und Sediment vorher/nachher. Die Uferroute umfasst auch Abtauchen, Auftauchen und Rückkehr. Der normale Paketstarter wurde zusätzlich erfolgreich geprüft. Die drei Vergleichspaare haben auf Protokollgenauigkeit identische erste Kamerapositionen und Blickrichtungen.

Die folgenden Helligkeitswerte stammen aus dem Bildausschnitt x=400–1499, y=200–649 bei 1600 × 900 Pixeln. Es sind gewichtete RGB-Bildwerte auf einer Skala von 0 bis 255, keine physikalische Lichtmessung. „Sehr hell“ bedeutet mindestens 200; es ist keine Aussage über abgeschnittene Farbkanäle.

| Erster Frame | Mittlere Bildhelligkeit vorher | Nachher | Sehr helle Bildpunkte vorher → nachher |
| --- | ---: | ---: | ---: |
| Direkter Tageseinstieg | 224,75 | 56,78 | 98,75 % → 0 % |
| Sediment-Szenario | 224,82 | 60,99 | 98,29 % → 0 % |
| Nacht mit Lampe | 0,39 | 31,10 | — |

Ohne Lampe bleibt der Nachteinstieg dunkel: mittlerer Bildwert 0,0042. Die späteren Sedimentzustände und die Sichtbarkeitsschalter bestehen weiterhin ihre Prüfungen. EXE, Quellen, Container, Aufnahmen und Protokolle sind im [maschinenlesbaren Prüfbericht](EXPOSURE_START_RESULTS.json) gebunden.

Gebaut mit der installierten Unreal Engine 5.8.4. EXE: `Builds/WindowsExposure/divingsim/Binaries/Win64/divingsim-Exposure.exe`, SHA-256 `e49b707008880d9cfd71fe17d5ed858b717760c48443e1b0fbd9abaecdfcb2b6`. Editor- und Paketbuild waren erfolgreich. Die experimentellen Licht-/Wasserverfahren der Quellcode-Engine sind im normalen Paket weiterhin nicht aktiviert.

Ein vorheriger Editor-Vergleich wurde **nicht als Bildnachweis akzeptiert**: Die erste Aufnahme enthielt noch Platzhaltergeometrie und ladende Texturen. Dieser Lauf wurde beendet; die vollständige Prüfung erfolgte anschliessend im fertigen Paket. Die bekannte Startmeldung zum alten CelestialVault-Sternmaterial tritt weiterhin auf; die Laufzeitprüfungen bestehen, ein fehlerfreies Engine-Protokoll wird nicht behauptet.

## Offene Grenzen

Harte blaue beziehungsweise dunkle Bänder und Reflexionsfehler unmittelbar an der Wasserlinie sind weiterhin sichtbar. Auch temporales Bildrauschen direkt nach dem Start und die gesamte Unter-/Überwasser-Referenzqualität bleiben offen. Die hier gezeigte Verbesserung betrifft die Initialisierung der Belichtung, keine vollständige Lichtabnahme.

Es wurde keine neue Bildratenmessung vorgenommen. Die bisherigen [Messungen zur Zellvorbereitung](POPULATION_PREPARATION.md) gehören zum Paket `WindowsPrepare`; dessen Codeänderung ist enthalten. Die [reale Mausprüfung und weitere UI-Grenzen](PLAYABLE_CURRENT.md) bleiben offen.

Reproduktion:

```powershell
./Tools/TestExposureStart.ps1 -Packaged -PackageDirectory Builds/WindowsExposure
# Den ausgegebenen Laufordner als erstes Argument verwenden:
Tools/.geodata-env/Scripts/python.exe Tools/ReportExposureStart.py artifacts/exposure-start-20261127-211030 --build artifacts/exposure-start/package-build.log --launcher artifacts/exposure-start/launcher --rejected-editor-review artifacts/exposure-start-20261127-210236/ReviewStatus.json --previous-package-report Documentation/POPULATION_PREPARATION_RESULTS.json --output Documentation/EXPOSURE_START_RESULTS.json
```
