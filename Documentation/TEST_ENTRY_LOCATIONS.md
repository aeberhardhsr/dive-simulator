# Zuordnung der Testeinstiege

Bei der Talus-Prüfung am 27. November 2026 wurde eine falsche Ortsbezeichnung in den bisherigen Grafikberichten entdeckt: **Index 2 ist beim direkten Wasserstart Walenstadt/Ost, beim Uferstart Mols/Lediwracks.** Ein identischer Index bedeutet hier nicht denselben Ort. Frühere Bezeichnungen wie „Ledi-Vergleich“ und der interne Fallname `ledi-wide` belegen daher für sich allein keine Prüfung bei den Lediwracks.

Die Tabellen folgen den unveränderten Projekt-Geodaten. Koordinaten sind Ost/Nord in EPSG:2056, in Metern:

| Index | Direkter Wasserstart (`manifest.json`) | Ost | Nord |
| --- | --- | ---: | ---: |
| 0 | Weesen / West | 2726086,5 | 1221674,5 |
| 1 | Murg / Sued | 2734890,5 | 1219849,5 |
| 2 | Walenstadt / Ost | 2741110,5 | 1220619,5 |

| Index | Uferstart (`shoreline.json`) | Ost | Nord |
| --- | --- | ---: | ---: |
| 0 | Weesen / Strandbad | 2727727,817 | 1222304,048 |
| 1 | Murg / West | 2734303,312 | 1219512,316 |
| 2 | Mols / Lediwracks | 2740697,067 | 1219937,022 |

`Dive::Config::StartOnShore` ist standardmässig falsch. Im normalen, nicht automatisierten Spiel wird der Uferstart vor dem Laden der Benutzereinstellungen aktiviert. Smoke-Tests laden diese Benutzereinstellungen nicht. `ADivePawn::StartDive` wählt die Einstiegsliste anhand von `PendingConfig.StartOnShore`. Das Szenario `shore` setzt den Uferstart ausdrücklich; die aktuellen `rocks`- und `stream`-Fälle tun dies nicht. Die Leistungsprüfung setzt ihn nur für `perf-shore`.

Für die [aktuelle Talus-Prüfung](TALUS_RESULTS.json) gilt deshalb:

- `close-before`, `close-after`, `ledi-wide`, `raster`: direkter Wasserstart, Index 2, Walenstadt/Ost.
- `weesen`, `sediment`: direkter Wasserstart, Index 0, Weesen/West.
- `murg`, `stream`: direkter Wasserstart, Index 1, Murg/Süd.
- `shore`: Uferstart, Index 0, Weesen/Strandbad.
- Die zusätzliche Unterwasser-Leistungsmessung mit Index 2 stammt aus Walenstadt/Ost, nicht von den Lediwracks.

Die historischen Fallnamen und Rohprotokolle bleiben unverändert, damit die Nachweise nachvollziehbar bleiben. Der aktuelle Bericht ergänzt zu jedem Fall den Startmodus, den tatsächlichen Eintrag aus den Geodaten und dessen Koordinaten. Beide Quelldateien sind per Prüfsumme gebunden. Die neun bestandenen Fälle sind kein vollständiger Durchlauf aller drei Uferzugänge; die normale Menüführung und weitere Ufer-Spielabläufe bleiben gesondert zu prüfen.

Quellen im Projekt: [Unterwassereinstiege](../divingsim/Content/Diving/Geodata/manifest.json), [Uferzugänge](../divingsim/Content/Diving/Geodata/shoreline.json), [Startlogik](../divingsim/Source/DiveSim/DivePawn.cpp), [Konfigurationsstandard](../divingsim/Source/DiveSim/Simulation/DiveModel.h).
