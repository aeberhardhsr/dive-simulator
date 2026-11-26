# Korrekte Quellfarben für Sand, Kies und Felsen

Die diffuse Farbe der gescannten Oberflächen wurde bei den 16-Bit-PNGs zu hell verarbeitet. Die sechs Farbkarten besitzen eingebettete Profile `sRGB IEC61966-2.1`; fünf davon sind in Unreal als `TSF_RGBA16` gespeichert. Zuvor hatten alle sechs `SourceColorSettings.EncodingOverride = None`, obwohl das Ausgabeattribut `SRGB` aktiviert war.

Unreals ImageCore behandelt 16-/32-Bit-Quellformate standardmässig als linear. Das Ausgabeattribut allein beschreibt deshalb nicht die notwendige Dekodierung dieser PNG-Quellen. Im lokalen UE-5.8-Quellcode ist dies in `Runtime/ImageCore/Public/ImageCore.h`, `ERawImageFormat::GetFormatNeedsGammaSpace`, und in `Developer/TextureCompressor/Private/TextureCompressorModule.cpp`, `LinearizeToWorkingColorSpace`, nachvollziehbar. Die gespeicherte Sand-Quellprobe im Bildzentrum beträgt beispielsweise RGB `(0.585946, 0.423835, 0.288594)`: Das sind weiterhin die kodierten Originalwerte, keine bereits linearisierten Farben.

`Tools/texture_color.py` setzt für diese Farbkarten jetzt explizit die Quellkodierung **sRGB** und für Normalen-/ARM-Karten **Linear**. Die normalen Content-Builder verwenden dieselbe Einstellung. Die Umrechnung geschieht beim Texturaufbau. Es gibt keine zusätzliche Gammafunktion im Material, keine neue Shader-Abfrage und keine pauschale Abdunklung der Beleuchtung. Original-PNGs, UVs, Geometrie, Verteilung und Materialgraphen bleiben erhalten. Die bisherige künstlerische Entsättigung und die nassen Beläge bleiben Teil des Materials.

## Nachweise

Der neue, lesende `DiveTextureAudit`-Commandlet prüft die tatsächlich erzeugten Windows-Texturformate, sRGB-Flags, Quellkodierungen, Abmessungen und Mipmaps. Alle 18 Karten besitzen 2.048 × 2.048 Pixel und zwölf Mipstufen. Die Farbkarten und ARM-Karten verwenden `PF_DXT1`, die Normalenkarten `PF_BC5`. Damit ist die ursprüngliche Vermutung eines ungeeigneten GPU-Formats ausgeschlossen. Die Änderung betrifft die Interpretation der Quelle vor der Kompression.

Ein erster Versuch mit `-nullrhi` lieferte keine Plattformdaten und schlug entsprechend fehl. Die gültigen Vorher-/Nachher-Prüfungen laufen mit D3D12 und `-AllowCommandletRendering`. Die ursprünglichen Dateien und die im Commandlet ausgelesenen Quellproben bleiben gleich. Der Ergebnisbericht prüft zusätzlich die Original-SHA-256-Werte sowie die eingebetteten ICC-Profile.

Die Sichtprüfung verwendet echte Unreal-Aufnahmen mit identischen Kameraangaben bei Mols/Lediwracks. Sand und Felsflächen wirken weniger ausgebleicht; feine Farbunterschiede treten stärker hervor. Der Seegrund ist weiterhin zu gleichförmig, und Pflanzen, Materialübergänge und Lichtverteilung erreichen die [Qualitätsreferenz](VISUAL_TARGET.md) noch nicht.

`Tools/TestBenthicColor.ps1` prüft sechs Fälle: Weesen, Murg, Lediwracks aus der Nähe, Lediwracks mit dem experimentellen Wasservolumen, Nacht mit Lampe und einen Uferblick. Das sind 15 frische Aufnahmen je vollständigem Lauf. Die Uferprüfung erfasst auch die gemeinsame Verwendung der Bodenmaterialien ausserhalb des Wassers; sie ersetzt die spätere Überarbeitung der Überwasserwelt nicht. `Tools/ReportBenthicColor.py` bindet Quell-/Asset-Prüfsummen, Plattformprüfung, ICC-Profile, Laufzeitprotokolle und Bilder zusammen. Diese kurzen Prüfungen sind keine neue Leistungs- oder Langzeitmessung.

## Reproduktion

1. `Tools/Build.ps1 -EditorSandbox` baut den aktuellen Editor-Code.
2. `Tools/Build.ps1 -EditorSandbox -Target Content -OnlyBenthicColor` aktualisiert nur die 18 Textur-Einstellungen.
3. `UnrealEditor-Cmd.exe <Sandbox-Projekt> -run=DiveTextureAudit -RequireSourceEncoding -Output=<JSON-Datei> -unattended -AllowCommandletRendering -d3d12 -nosplash` prüft die erzeugten Texturen. Hier kein `-nullrhi` verwenden.
4. `Tools/TestBenthicColor.ps1` prüft den nativen Stand; mit `-Packaged -PackageDirectory <Paketordner>` das gebaute Spiel.
5. `Tools/ReportBenthicColor.py <Laufordner> --baseline <Vorher-Bildordner> --audit-before <Vorher-JSON> --audit-after <Nachher-JSON> --output <Bericht>` prüft die Evidenz.

Die [Menü-/HUD-/FPV-Anforderungen](UI_UX_TARGET.md) folgen weiterhin erst nach Unterwasser- und Überwassergrafik.
