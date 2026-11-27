# Klarere Spiegelungen an der Wasserunterseite

Der experimentelle Wasserpfad zeigt jetzt deutlich schärfere Spiegelungen von Felsen und Seegrund. Zwei Änderungen wirken zusammen: Die Materialrauheit sinkt von 0,08 auf 0,02, und eine gezielte Hardware-Raytracing-Diagnose berechnet die Beleuchtung an den getroffenen Oberflächen. Die Wellen und ihre Normalen bleiben erhalten. Der Wert 0,02 ist eine gestalterische Abstimmung, keine Messung des Walensees.

Der Modus bleibt ein Grafikprototyp. Das normale Spielpaket und seine Leistungswerte ändern sich durch diese native Prüfung nicht. Die gesamte [Qualitätsreferenz](VISUAL_TARGET.md), vollständige Wasserbrechung und die Dämpfung entlang reflektierter Wasserwege bleiben offen.

## Warum die bisherige Spiegelung unscharf und unvollständig war

Die Kontrollreihe `artifacts/water-reflection-paths` trennt den unscharfen Beleuchtungs-Cache, die Reflexion der vordersten transparenten Fläche und Hardware-Raytracing. Der schärfere Reflexionspfad war bereits aktiv; die Kamerakonfiguration allein war also nicht die Ursache. Der zusätzliche Versuch mit Rauheit 0,005 unter `artifacts/water-reflection-roughness` machte die Konturen deutlich schärfer. Die endgültige Abstimmung 0,02 erhält eine kleine verbleibende Mikrorauheit.

Hardware-Raytracing mit Beleuchtung aus dem Surface Cache erfasste weiterhin nicht alle Boden- und Materialdetails. `r.Lumen.HardwareRayTracing.LightingMode=2` berechnet die Beleuchtung für Reflexionen am Trefferpunkt; die globale indirekte Beleuchtung behält dabei ihren Cache. Dieser Weg benötigt unterstützte Raytracing-Hardware. Die Software-Kontrolle bleibt im Vergleich erhalten und zeigt weiterhin unvollständige Bodenabdeckung.

Vorherige Software-Reflexion mit Rauheit 0,08:

![Bisherige unscharfe Spiegelung](Images/WaterReflections_Before.png)

Hardware-Raytracing mit Rauheit 0,02, aber weiterhin gecachter Beleuchtung:

![Kontrolle mit Beleuchtungs-Cache](Images/WaterReflections_Cache.png)

Korrigierter Aufbau mit Beleuchtung am Reflexionstreffer:

![Schärfere Spiegelung von Felsen und Seegrund](Images/WaterReflections_After.png)

## Falsche Treffer auf der eigenen Wasserfläche

Die erste grössere Serie `artifacts/water-reflections-20261127-004742` bestand sieben Laufzeitprüfungen mit 19 Aufnahmen, war visuell aber nicht akzeptabel: Der Blick nach oben zeigte eine rechteckige Grenze. Das Abschalten der bildschirmbasierten Reflexionsstrahlen unter `artifacts/water-reflection-screen-control` entfernte diese Grenze, liess jedoch grosse Teile der Spiegelung fehlen. Das blosse Abschalten wurde deshalb nicht als Lösung übernommen.

Der installierte `ProceduralMeshComponent` erzeugt seine Raytracing-Geometrie aus dem ursprünglichen Positionspuffer, ohne die Wellenverschiebung des Materials. Die sichtbare gewellte Fläche und ihre flache Raytracing-Geometrie stimmten nicht überein. Reflektierte Strahlen konnten dadurch die eigene flache Wasseroberseite treffen. Die bildschirmbasierten Treffer verdeckten diesen Fehler teilweise; ihr Übergang zum Raytracing wurde als Rechteck sichtbar.

Im getrennten Wasserpfad ist das Wassermesh nun für sekundäre Raytracing-Strahlen unsichtbar. Die primär sichtbare Oberfläche mit beiden Materialseiten, Wellen und Fresnel-Gewichtung bleibt bestehen. Die Kontrollen `water-reflection-self-surface-control` und `water-reflection-self-surface-hybrid` zeigen den Boden wieder vollständig und beseitigen die beobachtete rechteckige Grenze. Die übliche Kombination aus Bildschirmstrahlen und Hardware-Raytracing bleibt aktiv.

Verworfener Zwischenstand mit rechteckiger Grenze:

![Rechteckige Grenze im Sichtfenster](Images/WaterReflections_SeamBefore.png)

Gleiche Kamera nach Ausschluss der falschen Raytracing-Hilfsfläche:

![Sichtfenster ohne die zuvor sichtbare rechteckige Grenze](Images/WaterReflections_SeamAfter.png)

Das ist eine **Näherung mit einer Wassergrenzfläche**: Weitere Spiegelungen der Wasserfläche innerhalb einer Reflexion werden nicht berechnet. Eine künftig auch im Raytracing korrekt deformierte Wassergeometrie wäre für solche mehrfachen Wechsel erforderlich. Die Änderung ist keine vollständige Lösung des gesamten optischen Lichtwegs.

Die Ursache wurde im installierten Engine-Quelltext nachvollzogen: `ProceduralMeshComponent.cpp` erstellt den Dreiecks-Beschleunigungsbaum aus `PositionVertexBuffer` mit `bAllowUpdate=false`; `LumenReflectionTracing.cpp` beschreibt die Bildschirmtreffer vor dem Rückfall auf andere Verfahren; `LumenHardwareRayTracingCommon.cpp` definiert den verwendeten Beleuchtungsmodus 2. Engine-Dateien wurden nicht verändert.

## Reproduktion und verbleibende Integration

`Tools/Build.ps1 -EditorSandbox` baut die Korrektur am Wassermesh. Der Materialgenerator `Tools/Build.ps1 -EditorSandbox -Target Content -OnlyWater` erhält die Rauheit 0,02 bei späteren Neuimporten. `Tools/TestWaterReflections.ps1` erfasst Software- und Hardware-Cache-Kontrollen, den verbesserten Reflexionspfad, Nanite-Ersatzgeometrie, Sturm und Nacht mit/ohne Lampe. Die Wasserprüfung kontrolliert zusätzlich, dass die flache Raytracing-Hilfsfläche ausgeschlossen bleibt (`ray_surface=0`).

Die korrigierte Serie `artifacts/water-reflections-20261127-005814` besteht alle sieben Fälle mit 19 Aufnahmen. Der [Prüfbericht](WATER_REFLECTIONS_RESULTS.json) bindet Quellen, Modul, Materialien, Bildkameras, die verworfene Vorserie und die gezielten Strahlen-Kontrollen. Die Vergleichsposen stimmen überein. Ohne Sonne, Mond und Lampe bleibt das 99,9-%-Perzentil der maskierten Nachtansicht bei null; mit Lampe sind 254.550 Bildpunkte deutlich beleuchtet. Die bekannten Editor-Plugin-Startfehler werden separat erfasst. Diese Ergebnisse bestätigen die geprüften Darstellungswege, nicht die gesamte visuelle Zielqualität.

Berichtserzeugung für diesen Stand: `Tools/ReportWaterReflections.py artifacts/water-reflections-20261127-005814 --path-study artifacts/water-reflection-paths --roughness-study artifacts/water-reflection-roughness --prior-report artifacts/water-reflections-20261127-004742/Results.json`. Die fünf veröffentlichten Bilder werden ebenfalls per Hash geprüft. Die ersten sechs Erkundungsläufe besitzen archivierte Protokolle und 18 Bilder; nur die beiden vollständigen Serien besitzen zu Beginn erfasste Eingabemanifeste.

Einzelvergleich: `Tools/SmokeTest.ps1 -EditorSandbox -Scenario rocks -WaterView -WaterStart 1 -WaterEnd 1 -WaterInterface -BoundedWater -HardwareLumen -WaterReflection hit -Hour 12 -Date 2026-06-21 -MaxFPS 30`. `WaterReflection` akzeptiert ausserdem `cache` für die unscharfe Kontrolle und `front` für die Reflexion der vordersten Fläche. Diese Schalter sind Diagnosewerkzeuge, keine neuen Menüoptionen.

Vor der Übernahme in das normale Spiel fehlen Paketprüfung und repräsentative Leistungsmessungen, insbesondere für die Qualitätsstufe Hoch. Die Diagnose erzwingt die Reflexionsfreigaben; die normalen Engine-Qualitätsstufen dürfen daher nicht als bereits geprüft gelten. Menü, HUD und FPV folgen weiterhin erst nach den beiden Grafikphasen.
