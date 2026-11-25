# Wählbare Tauchlampen

Die Ausrüstungsauswahl enthält drei Lampen. Ein Klick wechselt das Modell; `L` schaltet die Lampe im Tauchgang ein oder aus. Die Wahl wird mit Einstellungen und Tauchprofilen gespeichert. Ältere Profile verwenden Allround. Während eines laufenden Tauchgangs bleibt die Auswahl gesperrt. Der Handgelenkcomputer zeigt den verbleibenden Akku auf seiner Gasseite.

| Spielmodell | Nennlichtstrom | Voller äußerer Lichtkegel | Laufzeit bis leer |
|---|---:|---:|---:|
| Allround | 1.000 lm | 54° | 120 min |
| Spot | 700 lm | 16° | 180 min |
| Wide | 3.000 lm | 110° | 55 min |

Dies sind selbst definierte Spielmodelle, keine Herstellerdaten. Die innere, gleichmäßig helle Zone verwendet jeweils 24°, 10° und 90° Vollwinkel. Unterschiedliche Gehäuse sind in Blender modelliert und werden tatsächlich in der rechten Hand ausgetauscht. Die Lichtquelle sitzt vor der Linse. Gleicher Lichtstrom bedeutet bei unterschiedlichen Kegeln keine gleiche Beleuchtungsstärke auf dem Grund.

## Akku und gespeicherte Messwerte

Der Akku entlädt sich bei eingeschalteter Lampe linear in Simulationszeit. Ausgeschaltet oder pausiert wird keine Energie verbraucht. Auch Exploration verwendet einen begrenzten Lampenakku. Bis 10 % bleibt der Nennlichtstrom erhalten; darunter sinkt er linear bis null. Ein neuer Tauchgang beginnt mit vollem Akku. Temperatur, Druck, Akkualterung, Wechselakkus und Ladegeräte sind nicht modelliert.

Tauchprotokolle enthalten das Objekt `torch` mit Modell, Nennlichtstrom, Kegel, Laufzeit und Restladung; jeder Messpunkt enthält `lamp_battery_percent`. Schema 4 bleibt durch diese zusätzlichen optionalen Felder lesbar. Im Logbuch gibt es derzeit kein eigenes Akkudiagramm.

## Unterwasserlicht

Die native Unreal-Spotlampe verwendet Lumen, quadratischen Abstandabfall und volumetrische Schatten. Eine Light Function dämpft den Weg von der Lampe zur beleuchteten Oberfläche mit `exp(-1.45 * Wasserweg / Sichtweite)`. Der Wasseranteil wird an einer lokalen ebenen Oberfläche geschnitten. Die bestehende Unterwasser-Nachbearbeitung behandelt den Rückweg zur Kamera einschließlich Farbverlust. Die Dämpfungskonstante ist eine gestalterische Näherung, keine Messung des Walensees.

Unter Wasser konzentriert sich das volumetrische Raster auf den sichtbaren Nahbereich. In Luft reicht es weiterhin 600 m weit. Schwebeteilchen verwenden jetzt ein beleuchtetes Material und ein reproduzierbares räumliches Zufallsfeld. Sie folgen nicht mehr starr der Kameraverschiebung. Die 640 Instanzen werden auf der CPU aktualisiert; eine GPU-Partikelsimulation ist damit nicht umgesetzt.

Grenzen: Die Positionsabhängigkeit der Light Function ist nicht als vollständige Dämpfung innerhalb des volumetrischen Nebels verifiziert. Nebel und Oberflächendämpfung sind gekoppelte Näherungen; Mehrfachstreuung, spektraler Hinweg, Brechung des Lampenstrahls an Wellen, reale Reflektorkurven und vollständige radiometrische Kalibrierung fehlen. Sichtbare Atemblasen und Schwebeteilchen sind unterschiedliche Systeme.

## Reproduzieren

```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.3/blender.exe' --background --python Tools/create_torch_assets.py
./Tools/Build.ps1 -Target Editor -EditorSandbox
./Tools/Build.ps1 -Target Content -OnlyTorch -EditorSandbox
./Tools/TestCore.cmd
./Tools/TestProfiles.ps1 -EditorSandbox
./Tools/Build.ps1 -Target Package -EditorSandbox -BuildSlot Torch -PackageDirectory Builds/WindowsTorch
./Tools/TestTorches.ps1 -PackageDirectory Builds/WindowsTorch
Tools/.geodata-env/Scripts/python.exe Tools/ReportTorches.py artifacts/torch-validation-YYYYMMDD-HHmmss
```

`-EditorSandbox` kopiert Source, Config und Projektdatei nach `Builds/EditorSandbox/divingsim`, mit eigenen Binär-/Zwischendateien. Dadurch kann ein bereits geöffneter Editor seine geladene DLL behalten. Content ist eine geprüfte Junction zum ursprünglichen Projekt; Asset-Erzeugung schreibt weiterhin echte Projektassets. Nach Quelländerungen muss der isolierte Editor erneut gebaut werden. `SmokeTest.ps1 -EditorSandbox` und `TestProfiles.ps1 -EditorSandbox` verwenden diesen vorhandenen Build.

Die Lampenserie prüft drei Nachtkegel, Aus/Leer/5-%-Akku, 3 m und 30 m Sicht, alle drei Gehäuse bei Tag, Einstellungen/Profile sowie ruhige und stürmische Oberflächenübergänge. Die Nachtaufnahme blickt aus ausreichendem Bodenabstand auf echte Bathymetrie; ein Strahltest kontrolliert einen Treffer zwischen 1 und 20 m. Bilder und exakte Tauchprotokolle werden pro Fall gesichert. Die Pixelprüfung zeigt die Lichtreaktion, ersetzt aber keine visuelle Abnahme oder Photometrie.

## Geprüfter Windows-Stand vom 25.11.2026

Alle 15 Paketfälle in `artifacts/torch-validation-20261125-143944` bestehen, mit 46 frischen PNGs. Die unabhängige Protokollauswertung bestätigt Modellparameter, 23 Messpunkte pro Lampenfall und Akkuwerte mit weniger als 0,000001 Prozentpunkten Abweichung. Einstellungen, alte/neue Profile und ungültige Modellnummern bestehen zusätzlich drei native Unreal-Profiltests; die C++-Kernprüfungen und 15 Python-Tests sind erfolgreich. Der Content-Build nach der UV-Korrektur meldet keine Warnungen oder Fehler.

Im HUD-freien Bildbereich sind gegenüber „Lampe aus“ 61.990 Pixel bei Allround, 22.365 bei Spot und 154.517 bei Wide um mehr als zehn RGB-Stufen heller. Aus/Leer stimmen im mittleren Bildwert bis auf 0,002 Stufen überein. Bei 5 % Restladung ist der sichtbare Lichtfleck schwächer. Wegen der stärkeren Nachtadaption bei ausgeschalteter Lampe ist die Gesamthelligkeit des Bildes kein Maß für den Lichtstrom.

Ein früherer funktional bestandener Editorlauf blickte mit der Lampe zu dicht am Boden und war schwarz; er zählt nicht als Bildnachweis. Der Test wurde auf tatsächlichen Bodenabstand und Strahlkontakt korrigiert. Ein erster Paketlauf scheiterte anschließend beim Sichern eines relativen Logpfads im Testskript. Erst die oben genannte vollständige Serie ist der erfolgreiche Gesamtlauf. Die bekannte native Celestial-Vault-Startmeldung zum alten Sternmaterialpfad bleibt bestehen; das tatsächlich geladene kanonische Material wird weiterhin geprüft.

Binärdatei: `Builds/WindowsTorch/divingsim/Binaries/Win64/divingsim-Torch.exe`, SHA-256 `10DDB46CD8BC6CA23670D1990404B989F1459C6A9E7BBB86E46C5960650D3E77`. Modellcode und Config des isolierten Buildprojekts stimmen mit dem Quellprojekt überein. [TORCH_RESULTS.json](TORCH_RESULTS.json) enthält Laufzeit-/Bildprüfsummen und Messwerte. Der frühere Celestial-Build und die ursprüngliche Projekt-EXE blieben unverändert.

Ein zusätzlicher Unterwasserlauf mit 1920 × 1080, Qualität 2 und 100 % Renderauflösung erfasst nach 15 s Aufwärmen 60 s ohne FPS-Limit: 5.328 Bilder, 88,83 FPS im Mittel, 69,59 FPS im langsamsten Prozent. Ein Bild benötigt 33,61 ms und überschreitet damit knapp das 30-FPS-Zeitbudget. RTX 3090 / Ryzen 9 9950X3D; beim Messbeginn läuft kein weiterer Unreal-Editor oder Tauchspielprozess. Andere Desktop-/GPU-Nutzung ist damit nicht ausgeschlossen. Der frühere Kalenderlauf hatte einen geöffneten Editor und ist kein kontrollierter direkter Vergleich. Diese einzelne Szene ersetzt keinen Dauertest oder Leistungsnachweis für alle Lampen/Sichtweiten.

Auch der normale Starter `Builds/WindowsTorch/divingsim.exe` ruft nachweislich `divingsim-Torch.exe` auf und besteht einen eigenen 22-Sekunden-Tauchtest. Seine Testdaten liegen isoliert unter `Saved_torchbootstrap`.

![Allround-Lampe auf realem Seegrund](Images/Torch_Allround.png)

![Enger Spot mit sichtbarem Lichtweg](Images/Torch_Spot.png)

![Breiter Lichtkegel und größeres Lampengehäuse](Images/Torch_Wide.png)

![Lampenwahl mit Kegel und Akkulaufzeit](Images/Torch_Menu.png)

## Bezug zur Spezifikation

CONF-014, EQPM-006, EQPM-007 und EQPM-008 sind um diese spielbaren Funktionen erweitert. Die drei Gehäuse leisten einen weiteren Teil von EQPM-012. Beleuchtete Schwebeteilchen erweitern WATR-008; GFX-008 verlangt darüber hinaus GPU-Partikel. Ausrüstungsspezifische Masse/Auftrieb für diese Lampen sowie die vollständige optische und fachliche Abnahme bleiben offen.

## Primärquellen und Assets

- [Epic: Physical Lighting Units](https://dev.epicgames.com/documentation/unreal-engine/using-physical-lighting-units-in-unreal-engine?lang=en-US), zusätzlich die API der lokal installierten UE 5.8.4.
- [Epic: Spot Lights](https://dev.epicgames.com/documentation/unreal-engine/spot-lights-in-unreal-engine).
- [Epic: Light Functions](https://dev.epicgames.com/documentation/unreal-engine/using-light-functions-in-unreal-engine).
- [Epic: Volumetric Fog](https://dev.epicgames.com/documentation/unreal-engine/volumetric-fog-in-unreal-engine?lang=en-US).
- Originale prozedurale Gehäuse: `Tools/create_torch_assets.py`, `blender/TorchAssets.blend`, drei FBX-Dateien unter `blender/exports`. Keine übernommenen Produktmodelle oder Markenzeichen.
