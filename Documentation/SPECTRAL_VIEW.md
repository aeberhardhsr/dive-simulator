# RGB-Absorption entlang des Kamerawegs

## Status und Ziel

Dieser Versuch ergänzt eine entfernungsabhängige RGB-Absorption im Unterwasserbild. Er ist ausdrücklich **nicht standardmässig aktiviert** und noch keine Abnahme der [visuellen Referenz](VISUAL_TARGET.md). Die spielbare Grafikvorschau bleibt `Builds/WindowsStreamBuild/Start-WaterPreview.cmd`.

Die bisherige native Volumenstreuung liefert bereits Licht und Schatten im Wasser, verwendet aber eine skalare Extinktion. Eine nachträgliche Multiplikation des gesamten fertigen Bildes mit der Entfernung des Hintergrunds würde auch den nahen Wassernebel wie einen weit entfernten Felsen abdunkeln. Genau diesen Fehler soll der neue Pass vermeiden.

## Umsetzung

Das neue Modul `DiveRendering` greift vor der Tiefenunschärfe auf das kumulierte Streulicht des aktuellen Unreal-Volumengitters zu. Es zerlegt dessen Kamerapfad in 16 Intervalle und gewichtet jedes Intervall mit seiner eigenen zusätzlichen RGB-Transmission. Der verbleibende Oberflächenanteil erhält die Transmission bis zur ersten sichtbaren Grenze. Beide Eingaben liegen in derselben Vorbelichtung; es wird kein zusätzliches Licht erzeugt.

Die Koeffizienten betragen aktuell `(0,12; 0,015; 0,035) m⁻¹` für Rot, Grün und Blau, multipliziert mit einer Teststärke von 0 bis 2. Das sind entworfene Werte, keine Messdaten des Walensees. Stärke 0 ist rechnerisch die unveränderte Ausgabe. Die Aufnahmen behalten kleine Unterschiede durch zeitliche Beleuchtung und Rauschen zwischen getrennten Läufen.

Für den Blick zur Oberfläche schreibt nur im aktivierten Versuch das bewegte Wasser seine Tiefe und die Kennung 42 in den Custom-Depth-/Stencil-Puffer. Der Pass verwendet die jeweils nähere Grenze aus Wasser und undurchsichtiger Geometrie. Somit wird ein gespiegelter Hintergrund nicht fälschlich zur Länge des ersten Wasserwegs. `M_WaterUnder` erlaubt dafür transparente Tiefenschreibvorgänge; der normale Start schaltet sie am Wasserobjekt nicht ein.

Ein erster Ansatz begrenzte den Weg an einer ebenen Wasserhöhe und übersprang darüber liegende Endpunkte. Er erzeugte eine sichtbare horizontale Farbkante und wurde korrigiert. Die ausgewertete Fassung verwendet die tatsächliche, durch Wellen verschobene Oberflächentiefe. Die Diagnoseaufnahme markiert Wassergrenzen rot und davor liegende undurchsichtige Geometrie blau.

Die Implementierung nutzt private Renderer-Strukturen der installierten **UE 5.8** (`FViewInfo` und dessen `IntegratedLightScatteringTexture`). Ein Engine-Upgrade muss diese Anbindung erneut kompilieren und visuell prüfen. Engine-Dateien wurden nicht verändert. Die offizielle [Dokumentation zur volumetrischen Nebelbeleuchtung](https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-fog-in-unreal-engine) beschreibt das zugrunde liegende System; die konkrete Schnittstelle wurde im lokalen 5.8-Quelltext geprüft.

## Vergleichsbilder

Die folgenden Bilder stammen aus echten nativen Unreal-Läufen mit festen Kameras, Mittagssonne, Sichtweite 14 m und identischer Umgebung. Sie sind keine aus der Nutzerreferenz erzeugten Hintergründe.

Vorher, bodennahe Ansicht bei Weesen:

![Bodenansicht ohne zusätzlichen RGB-Pass](Images/SpectralView_Bed_before.png)

Mit zusätzlicher RGB-Absorption:

![Bodenansicht mit zusätzlicher RGB-Absorption](Images/SpectralView_Bed_after.png)

Vorher, Blick zur Sonne:

![Sonnenansicht ohne zusätzlichen RGB-Pass](Images/SpectralView_Sun_before.png)

Mit zusätzlicher RGB-Absorption:

![Sonnenansicht mit zusätzlicher RGB-Absorption](Images/SpectralView_Sun_after.png)

Entfernte Flächen verlieren stärker Rot als nahe Details; der Übergang zum Wasser bleibt in diesen Ansichten ohne die zuvor beobachtete harte Kante. Die Beleuchtung ist weiterhin flach, freie Bodenbereiche bleiben zu gleichförmig und Bewuchs wirkt stellenweise zu hell und punktförmig. Die Änderung allein erreicht die gewünschte Licht- und Raumwirkung nicht.

## Prüfverfahren

`Tools/TestSpectralView.ps1` prüft jeweils unveränderte Ausgabe, Nullstärke und Stärke 1. Drei Reihen bei Weesen mit Boden-/Sonnenblick und bei Walenstadt ergeben neun Läufe mit 27 Bildern. Kameraposen müssen innerhalb jeder Reihe exakt übereinstimmen. Quellen, Wasserassets, Konfiguration und native Module werden vor dem Lauf gehasht und als Eingaben gesichert; nach dem Lauf werden die Eingaben erneut geprüft. Der [maschinenlesbare Bericht](SPECTRAL_VIEW_RESULTS.json) bindet Aufnahmen, Protokolle, Differenzen und Leistungsmessungen an diese Dateien.

Die mittlere absolute RGB-Abweichung der Nullstärke liegt in den neun Bildpaaren zwischen 0,281 und 0,450 auf der Skala 0–255. Das belegt eine nahezu unveränderte Nullkontrolle, keine physikalische Kalibrierung. Die Diagnose zeigt beim Blick zur Sonne vollständig die Wassergrenze; beim flacheren Blick liegen Felsen und Fische korrekt davor.

Die nativen Editor-Protokolle enthalten auch in den Kontrollläufen `LogAutomationTest: Condition failed` während der Engine-Initialisierung und einen Python-Startfehler des experimentellen Engine-Plugins `ToolsetRegistry` (`PythonTestRunner` fehlt). Diese Meldungen sind im Bericht vollständig erhalten. Der Prüfstatus gilt für die ausdrücklich ausgewerteten Spiel-/Renderprüfungen und bedeutet nicht, dass das gesamte Editor-Protokoll fehlerfrei ist.

Im Paket bleibt die bereits in `WindowsStreamBuild` vorhandene Meldung zum fehlenden CelestialVault-Sternmaterial `MI_StarsStar_EnergyConservative` bestehen. Sie wird ebenfalls aufgeführt; eine vollständige Prüfung des nächtlichen Überwasserhimmels ist nicht Teil dieses Versuchs.

`Tools/TestSpectralPackage.ps1` prüft zusätzlich den gekochten Shader im separaten Versuchspaket: Basis, Nullstärke, Absorption, Nacht ohne Lampe, abgeschalteten Volumennebel mit/ohne Versuch, den Uferwechsel sowie einen Vergleich mit eingeschalteter Lampe bei Murg. Diese Prüfungen betreffen Laufzeit und Rückfallverhalten. Sie ersetzen weder eine gestalterische Abnahme noch eine Sturm-/Wellenlinienprüfung.

**Paketstand vom 27. November:** Alle neun Fälle mit 31 Bildern und der Haupt-EXE-Start bestehen. Die Paket-Nullkontrolle liegt bei 0,259–0,356, der Rückfallvergleich bei 0,227–0,294 von 255 Farbwerten. Die Nacht ohne Lampe erzeugt keinen sichtbaren Hintergrundschein; mit Lampe bleiben beleuchtete Bodenflächen und der Lichtkegel erhalten. Der normale Launcher aktiviert den zusätzlichen Pass nicht. Die geprüfte EXE `Builds/WindowsSpectralStudy/divingsim/Binaries/Win64/divingsim-SpectralStudy.exe` hat SHA-256 `3b6d714b4ba60fa8061f99ace88b375972d60f1000050198267e5013db0fcd91`. Das Paket dient der Untersuchung und ersetzt `WindowsStreamBuild` nicht.

Der erste Paketlauf wurde durch eine zu strenge Prüfung einer Startmeldung angehalten. `ExecCmds` kann den Nebel abschalten, während ein früher Renderframe noch unterwegs ist; dessen einmaliges `READY` ist keine Aussage über den Zustand der späteren Aufnahmen. Die drei Rückfall-Bildpaare unterschieden sich nur um 0,225 bis 0,319 von 255 Farbwerten. Die korrigierte Prüfung fordert den Rückfallzustand vor den Vergleichskameras und zusätzlich nahezu unveränderte Bildpaare. Eine späte `READY`-Meldung während der Aufnahmen bleibt ein Fehler. Der Renderer wurde für diese Korrektur des Prüfverfahrens nicht verändert.

Die Leistungsprüfung verwendet vollständige native CSV-Aufnahmen bei 1920 × 1080, Qualität Hoch und 100 % Renderauflösung, ohne FPS-Limit. Bei Weesen werden 60 Sekunden, auf der Murg-Nachladestrecke 90 Sekunden nach dem Aufwärmen ausgewertet. Alle inneren langen Frames bleiben enthalten. Die Mittelwerte beschreiben den gesamten Lauf, nicht isoliert die GPU-Kosten des neuen Passes. Rechnerlast und zeitlich unterschiedliche Blickrichtungen begrenzen den Vergleich.

| Ort / Versuch | FPS Mittel | 1%-Low FPS | Frame P95 | Längster Frame | Frames über 33,33 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| Weesen, aus | 69,59 | 56,62 | 15,62 ms | 39,58 ms | 2 / 4173 |
| Weesen, an | 68,47 | 55,48 | 15,84 ms | 43,11 ms | 1 / 4106 |
| Murg-Nachladen, aus | 97,48 | 69,89 | 11,11 ms | 26,98 ms | 0 / 8771 |
| Murg-Nachladen, an | 95,59 | 69,73 | 11,28 ms | 26,39 ms | 0 / 8601 |

Die mittlere Framezeit steigt in diesen einzelnen Läufen um 0,234 ms bei Weesen und 0,202 ms bei Murg. Die CPU-Momentaufnahmen vor/nach den Läufen liegen zwischen 118 und 296 Prozent eines logischen Prozessors bei 32 logischen Prozessoren; sie erfassen keinen kontinuierlichen Verlauf. Die Ergebnisse sind kein Nachweis einer Mindestbildrate oder einer exakten isolierten Shaderzeit.

## Reproduktion

```powershell
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/TestSpectralView.ps1
./Tools/TestSpectralView.ps1 -SunView
./Tools/TestSpectralView.ps1 -Entry 2

./Tools/SmokeTest.ps1 -EditorSandbox -Scenario rocks -WaterPreview -SpectralView -SpectralDepthDebug -HabitatView -HabitatLowView -HabitatSunView -Hour 12

./Tools/Build.ps1 -Target Package -EditorSandbox -BuildSlot SpectralStudy -PackageDirectory Builds/WindowsSpectralStudy
./Tools/TestSpectralPackage.ps1
```

Der Pass erfordert gemeinsam `-DiveSmokeTest -DiveWaterPreview -DiveSpectralView`. Die Teststärke wird über `-DiveAbsorptionGain=0..2` gesetzt. Ohne diese ausdrückliche Kombination bleibt die Erweiterung inaktiv. Die Testwerkzeuge müssen nacheinander laufen; native Tests verwenden die Editor-Sandbox.

## Offene Grenzen

Die anschliessende [native Volumenreihe](SOLAR_VOLUME_STUDY.md) untersucht diesen Kamera-Pass gemeinsam mit zusätzlicher Absorption des direkten Oberflächen- und Volumenlichts. Sie ersetzt die hier dokumentierten Paketmessungen nicht und löst die folgenden Grenzen nur teilweise.

- Nur der erste Kameraweg wird korrigiert. Die Beleuchtung vom Licht zum Streupunkt sowie weitere Wege nach Spiegelung/Brechung sind noch nicht gemeinsam als RGB-Transport gelöst.
- Die 16 Intervalle sind eine Näherung des vorhandenen Volumengitters; es gibt keine vollständige spektrale Mehrfachstreuung oder belastbare Walensee-Kalibrierung.
- Die Aktivierung am Auge blendet zwischen 5 und 50 cm unter dem mittleren Wasserstand ein. Sie verwendet noch nicht die lokale Wellenhöhe; vor einer Standardaktivierung ist der Übergang bei Sturm gezielt zu korrigieren und zu prüfen.
- Transparente Blasen und Schwebstoffelemente besitzen keine getrennten Weglängen in diesem Pass. Helle Nahbeleuchtung, niedrige Renderauflösung und weitere Qualitätsstufen brauchen zusätzliche Prüfung.
- Die native Renderer-Anbindung ist auf der vorhandenen Windows-/D3D12-Konfiguration geprüft; andere Plattformen und Engine-Versionen sind damit nicht abgedeckt.

Die Reihenfolge des Gesamtprojekts bleibt Unterwasserqualität, Überwasserlandschaft und anschliessend [Menü/HUD/FPV](UI_UX_TARGET.md).
