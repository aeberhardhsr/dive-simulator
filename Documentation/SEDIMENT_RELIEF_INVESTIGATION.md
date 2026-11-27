# Versuch mit einer zusätzlichen Sedimentschicht

## Entscheidung

**Nicht in die Spielvorschau übernommen.** Eine räumliche, bis zu 30 cm hohe Schicht wurde im nativen Unreal-Spiel geprüft. In drei identischen Kamerablicken bei Weesen erscheinen leichte Bodenwellen; die grossen offenen Flächen wirken weiterhin gleichmässig. Die gewünschte Verzahnung zwischen Felsen, Geröll, Bewuchs und Sediment entsteht damit noch nicht. Gleichzeitig verändert die neue Höhenabfrage die Platzierung und Neigung der Felsen. Das ist für diesen gestalterischen Zweck zu wenig gezielt.

Die aktuelle Spielvorschau bleibt `Builds/WindowsRockPose/Start-WaterPreview.cmd`. Die Quelländerungen des Versuchs sind als [anwendbarer Patch](Experiments/sediment-relief.patch) archiviert und aus dem aktiven Spielcode entfernt. Es wurden keine Inhaltsassets verändert und kein neues Spielpaket erstellt.

## Aufbau und technische Erkenntnisse

Die Original-Bathymetrie und die Tiefenkarte blieben unverändert abrufbar. Ein separates Höhenmodell ergänzte an den gemeinsamen Ein-Meter-Geländevertices positive Ablagerungen bis 0,30 m. Weltfeste, verformte Rauschfelder bestimmten deren Verteilung. Nahe der Wasserlinie, auf steilen Wänden, in grosser Tiefe und an Messdatenlücken blieb der Zusatz aus. Sichtbare Dreiecke, Kollision, Schwimmhöhe, Pflanzen, Kies und bodennahe Fische verwendeten dieselben interpolierten Oberflächen.

Der erste Lauf scheiterte an der vorhandenen Küstenprüfung. Vier direkte Nachbarn eines Messpunkts reichen an einer diagonal angrenzenden Datenlücke nicht aus: Ein bereits vorbereiteter Küstenrand kann dort noch dieselben Vertices verwenden. Die zusätzliche Prüfung aller acht Nachbarn hielt solche Übergänge unverändert. Danach bestanden die 496 Kontaktproben der vorbereiteten Küstenmeshes.

`artifacts/sediment-relief-20261127-043705` enthält den abgeschlossenen Vergleich mit zwei Fällen und sechs Originalaufnahmen. Kamera, Uhrzeit und Wasserprofil sind gleich; die Felslayouts sind wegen der zusätzlichen Höhen-/Hangabfrage ausdrücklich nicht gleich. Die Bodenkontaktprüfung trifft 796 von 796 Proben ohne Zusatzschicht und 790 von 790 mit Zusatzschicht. Die erfasste maximale Ablagerung beträgt 29,4724 cm. Die Populations-, Kies-, Felskontakt- und Fischbewegungsprüfungen bestehen. Die unabhängigen C++-Tests für Begrenzungen und stetige Übergänge bestehen ebenfalls.

Die Prüfung umfasst nur den nativen Lauf bei Weesen. Sie ist keine Paket-, Leistungs-, Langzeit- oder visuelle Gesamtfreigabe. Rechenkosten der zusätzlichen Höhenabfragen wurden nicht systematisch gemessen. Der [Versuchsbericht](SEDIMENT_RELIEF_EXPERIMENT.json) hält Kamerapositionen, Prüfmarker, Quell-/Assetprüfsummen sowie die Originalaufnahmen fest.

## Echter Spielvergleich

Bisherige Oberfläche:

![Bisheriger Seegrund in der niedrigen Vergleichsansicht](Images/SedimentReliefExperiment_Before.png)

Zusätzliche Sedimentschicht, identische Kamera:

![Leichte zusätzliche Bodenwellen im verworfenen Versuch](Images/SedimentReliefExperiment_After.png)

## Konsequenz für die weitere Gestaltung

Eine flächige Höhenanhebung allein ist keine ausreichende Lösung. Die nächste geometrische Gestaltung soll Ablagerungen und Geröll an vorhandene Felsfüsse anbinden, deren Formen dabei erhalten und die angrenzenden freien Bodenflächen bewusst einbeziehen. Neue Oberflächen benötigen weiterhin gemeinsame Kontakt- und Höhenabfragen; Messdatenlücken, gemischte Küstendreiecke und bodennahe Lebewesen müssen mitgeprüft werden. Der Versuch wird nicht als erreichte Verbesserung der Referenzqualität gewertet.

Die Wiederherstellung des geprüften Quell- und Paketstands ist in [SEDIMENT_RELIEF_RESTORATION.json](SEDIMENT_RELIEF_RESTORATION.json) dokumentiert.
