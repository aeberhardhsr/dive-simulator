"""Render the coast-data diagnostic (optional dependency: matplotlib)."""
import json
from pathlib import Path

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.colors import ListedColormap
from matplotlib.patches import Patch, Polygon
import numpy as np

ROOT = Path(__file__).resolve().parents[1]
DEST = ROOT/"Geodata/Processed/Coast"


def main():
    manifest = json.loads((DEST/"coast.json").read_text(encoding="utf-8"))
    outline = json.loads((ROOT/"Geodata/Raw/WaterBoundary/walensee_polygon.geojson").read_text(encoding="utf-8"))["geometry"]["coordinates"]
    west, south, east, north = manifest["bounds"]
    labels = np.zeros((north-south, east-west), "u1")
    # Fine, local Walenstadt diagnostic; all data use the same LV95 coordinates.
    w, s, e, n = 2740800, 1220300, 2741500, 1220950
    heights = np.full((n-s, e-w), np.nan, "f4")
    local_labels = np.zeros(heights.shape, "u1")
    for tile in manifest["tiles"]:
        size = tile["columns"]
        p = np.fromfile(DEST/tile["provenance_file"], "u1").reshape(size, size)
        z = np.fromfile(DEST/tile["file"], "<f4").reshape(size, size)
        tw, ts, tn = tile["west"], tile["south"], tile["south"]+size
        labels[north-tn:north-ts, tw-west:tw-west+size] = p
        left, bottom, right, top = max(tw, w), max(ts, s), min(tw+size, e), min(tn, n)
        if left >= right or bottom >= top:
            continue
        target, source = np.s_[n-top:n-bottom, left-w:right-w], np.s_[tn-top:tn-bottom, left-tw:right-tw]
        heights[target], local_labels[target] = z[source], p[source]
    # Full measured cells beyond the coastal output are displayed as well.
    bathy = json.loads((ROOT/"divingsim/Content/Diving/Geodata/manifest.json").read_text())
    for tile in bathy["tiles"]:
        tw, ts = int(tile["xllcorner"]), int(tile["yllcorner"])
        rows, cols = int(tile["nrows"]), int(tile["ncols"])
        tn = ts+rows
        left, bottom, right, top = max(tw, w), max(ts, s), min(tw+cols, e), min(tn, n)
        if left >= right or bottom >= top:
            continue
        raw = np.fromfile(ROOT/"divingsim/Content/Diving/Geodata"/tile["file"], "<i2").reshape(rows, cols)[tn-top:tn-bottom, left-tw:right-tw]
        target = np.s_[n-top:n-bottom, left-w:right-w]
        valid = raw != -32768
        heights[target][valid] = raw[valid].astype("f8")*.01
        local_labels[target][valid] = 1

    fig = plt.figure(figsize=(14, 10), layout="constrained", facecolor="#f7f8fa")
    grid = fig.add_gridspec(2, 2, height_ratios=[1, 2])
    whole = fig.add_subplot(grid[0, :])
    whole.set_facecolor("#e7e4dc")
    for index, ring in enumerate(outline):
        whole.add_patch(Polygon(np.array(ring)/1000, facecolor="#345c7b" if index == 0 else "#e7e4dc", edgecolor="#345c7b", linewidth=.5))
    overlay = np.ma.masked_where(labels[::4, ::4] != 3, labels[::4, ::4])
    whole.imshow(overlay, extent=(west/1000, east/1000, south/1000, north/1000), origin="upper", cmap=ListedColormap(["#ef922e"]), interpolation="nearest", zorder=2)
    whole.plot(np.array([w, e, e, w, w])/1000, np.array([s, s, n, n, s])/1000, color="#b80054", lw=1.4)
    whole.set(xlim=(2725.6, 2741.8), ylim=(1219.1, 1222.6), title="Gesamter Walensee · ergänzte Bereiche orange · Ausschnitt Walenstadt rechts", ylabel="LV95 Nord [km]")
    whole.legend(handles=[Patch(color="#345c7b", label="Seeumriss swissTLM3D"), Patch(color="#ef922e", label="Modellierte Lücken: 0,772 km²")], loc="lower left", fontsize=9)
    whole.ticklabel_format(useOffset=False)

    for index, title in enumerate(("Walenstadt · Originalmessung", "Walenstadt · separater Modellentwurf")):
        axis = fig.add_subplot(grid[1, index])
        axis.set_facecolor("#e7e4dc")
        if index == 0:
            missing = np.ma.masked_where(local_labels != 3, local_labels)
            axis.imshow(missing, extent=(w/1000, e/1000, s/1000, n/1000), cmap=ListedColormap(["#e7aaca"]), origin="upper", interpolation="nearest")
        included = (local_labels == 1) | ((local_labels == 3) & (index == 1))
        depth = np.ma.masked_where(~included, -heights)
        picture = axis.imshow(depth, extent=(w/1000, e/1000, s/1000, n/1000), origin="upper", cmap="Blues", vmin=0, vmax=25, interpolation="nearest")
        for ring in outline:
            line = np.array(ring)/1000
            axis.plot(line[:, 0], line[:, 1], color="#b80054", linewidth=.8)
        axis.plot(2741.1105, 1220.6195, marker="+", color="#202020", ms=10, mew=1.5)
        axis.set(xlim=(w/1000, e/1000), ylim=(s/1000, n/1000), title=title, xlabel="LV95 Ost [km]", ylabel="LV95 Nord [km]")
        axis.ticklabel_format(useOffset=False)
        if index == 0:
            axis.legend(handles=[Patch(color="#e7aaca", label="Unvermessen innerhalb Seeumriss")], loc="lower left", fontsize=8)
        fig.colorbar(picture, ax=axis, shrink=.7, label="Tiefe unter Szenariopegel 419 m [m]; Skala bei 25 m begrenzt")
    fig.suptitle("Uferdatenprüfung · noch nicht in der Spielversion eingebunden", fontsize=17, weight="bold")
    fig.supxlabel("Quellen: © swisstopo · swissBATHY3D / swissALTI3D / swissTLM3D   |   Ergänzte Tiefen sind Modellwerte, keine Vermessung.", fontsize=10)
    output = ROOT/"Documentation/Images/Coast_Model.png"
    fig.savefig(output, dpi=130)
    print(output)


if __name__ == "__main__":
    main()
