# Korrekte Quellfarben für Sand, Kies und Felsen

Spielbares Paket: **`Builds/WindowsBenthicColor/divingsim.exe`**, gebaut mit Quellstand `4dd63bfdf36ced320321c0d4ba880de1e52fd2ca`. Je sechs native und Paketfälle mit je 15 Aufnahmen bestehen, ebenso der tatsächliche Root-Launcher-Start. [Native Ergebnisse](BENTHIC_COLOR_NATIVE_RESULTS.json), [Paket und Launcher](BENTHIC_COLOR_RESULTS.json). EXE-SHA-256: `a5d8df9872a7fde1716bbc9897aa51ecb32973b9693ec18be518bb5b2478b34f`. Der Build meldet `BUILD SUCCESSFUL` in `artifacts/benthic-color-package-build.log`. Beide [GitHub-Quellprüfungen](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45891660634) und [weiteren Workflow-Prüfungen](https://github.com/aeberhardhsr/dive-simulator/actions/runs/45891655066) sind erfolgreich.

Die diffuse Farbe der gescannten Oberflächen wurde bei den 16-Bit-PNGs zu hell verarbeitet. Die sechs Farbkarten besitzen eingebettete Profile `sRGB IEC61966-2.1`; fünf davon sind in Unreal als `TSF_RGBA16` gespeichert. Zuvor hatten alle sechs `SourceColorSettings.EncodingOverride = None`, obwohl das Ausgabeattribut `SRGB` aktiviert war.

Unreals ImageCore behandelt 16-/32-Bit-Quellformate standardmässig als linear. Das Ausgabeattribut allein beschreibt deshalb nicht die notwendige Dekodierung dieser PNG-Quellen. Im lokalen UE-5.8-Quellcode ist dies in `Runtime/ImageCore/Public/ImageCore.h`, `ERawImageFormat::GetFormatNeedsGammaSpace`, und in `Developer/TextureCompressor/Private/TextureCompressorModule.cpp`, `LinearizeToWorkingColorSpace`, nachvollziehbar. Die gespeicherte Sand-Quellprobe im Bildzentrum beträgt beispielsweise RGB `(0.585946, 0.423835, 0.288594)`: Das sind weiterhin die kodierten Originalwerte, keine bereits linearisierten Farben.

`Tools/texture_color.py` setzt für diese Farbkarten jetzt explizit die Quellkodierung **sRGB** und für Normalen-/ARM-Karten **Linear**. Die normalen Content-Builder verwenden dieselbe Einstellung. Die Umrechnung geschieht beim Texturaufbau. Es gibt keine zusätzliche Gammafunktion im Material, keine neue Shader-Abfrage und keine pauschale Abdunklung der Beleuchtung. Original-PNGs, UVs, Geometrie, Verteilung und Materialgraphen bleiben erhalten. Die bisherige künstlerische Entsättigung und die nassen Beläge bleiben Teil des Materials.

## Nachweise

Der neue, lesende `DiveTextureAudit`-Commandlet prüft die tatsächlich erzeugten Windows-Texturformate, sRGB-Flags, Quellkodierungen, Abmessungen und Mipmaps. Alle 18 Karten besitzen 2.048 × 2.048 Pixel und zwölf Mipstufen. Die Farbkarten und ARM-Karten verwenden `PF_DXT1`, die Normalenkarten `PF_BC5`. Damit ist die ursprüngliche Vermutung eines ungeeigneten GPU-Formats ausgeschlossen. Die Änderung betrifft die Interpretation der Quelle vor der Kompression.

Ein erster Versuch mit `-nullrhi` lieferte keine Plattformdaten und schlug entsprechend fehl. Die gültigen Vorher-/Nachher-Prüfungen laufen mit D3D12 und `-AllowCommandletRendering`. Die ursprünglichen Dateien und die im Commandlet ausgelesenen Quellproben bleiben gleich. Der Ergebnisbericht prüft zusätzlich die Original-SHA-256-Werte sowie die eingebetteten ICC-Profile.

Die Sichtprüfung verwendet echte Unreal-Aufnahmen mit identischen Kameraangaben bei Mols/Lediwracks. Sand und Felsflächen wirken weniger ausgebleicht; feine Farbunterschiede treten stärker hervor. Der Seegrund ist weiterhin zu gleichförmig, und Pflanzen, Materialübergänge und Lichtverteilung erreichen die [Qualitätsreferenz](VISUAL_TARGET.md) noch nicht.

`Tools/TestBenthicColor.ps1` prüft sechs Fälle: Weesen, Murg, Lediwracks aus der Nähe, Lediwracks mit dem experimentellen Wasservolumen, Nacht mit Lampe und einen Uferblick. Das sind 15 frische Aufnahmen je vollständigem Lauf. Der Uferblick zeigt überwiegend den modellierten Zugang und Gras: Er ist eine zusätzliche Start-/Renderingprüfung, keine Abnahme der Ufermaterialien oder der Überwasserwelt. `Tools/ReportBenthicColor.py` bindet Quell-/Asset-Prüfsummen, Plattformprüfung, ICC-Profile, Laufzeitprotokolle und Bilder zusammen. Diese kurzen Prüfungen sind keine neue Leistungs- oder Langzeitmessung.

## Bildvergleich

Native Unreal-Spielaufnahmen mit identischer Kamera bei Lediwracks, zuerst vorher:

![Bisherige Interpretation der 16-Bit-Farbquellen](Images/BenthicColor_Before.png)

Mit expliziter sRGB-Quellkodierung:

![Korrigierte Sand- und Felsfarben](Images/BenthicColor_After.png)

Die Bilder entsprechen `artifacts/bed-color-before/Habitat_0_1.png` und `artifacts/benthic-color-20261126-212828/ledi-close/Habitat_0_1.png`. Alle sechs nativen Fälle bestehen: [Ergebnisbericht mit Vorher-/Nachher-Prüfsummen](BENTHIC_COLOR_NATIVE_RESULTS.json).

## Reproduktion

1. `Tools/Build.ps1 -EditorSandbox` baut den aktuellen Editor-Code.
2. `Tools/Build.ps1 -EditorSandbox -Target Content -OnlyBenthicColor` aktualisiert nur die 18 Textur-Einstellungen.
3. `UnrealEditor-Cmd.exe <Sandbox-Projekt> -run=DiveTextureAudit -RequireSourceEncoding -Output=<JSON-Datei> -unattended -AllowCommandletRendering -d3d12 -nosplash` prüft die erzeugten Texturen. Hier kein `-nullrhi` verwenden.
4. `Tools/TestBenthicColor.ps1` prüft den nativen Stand; mit `-Packaged -PackageDirectory <Paketordner>` das gebaute Spiel.
5. `Tools/ReportBenthicColor.py <Laufordner> --baseline <Vorher-Bildordner> --audit-before <Vorher-JSON> --audit-after <Nachher-JSON> --output <Bericht>` prüft die Evidenz.

Die [Menü-/HUD-/FPV-Anforderungen](UI_UX_TARGET.md) folgen weiterhin erst nach Unterwasser- und Überwassergrafik.
