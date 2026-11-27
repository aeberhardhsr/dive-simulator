"""Signed horizontal distance to a surveyed lake boundary, in metres.

Positive inside water; negative on land, including island interiors. Rows run
north to south and columns west to east. This is a fixed horizontal outline,
not a water-level, terrain-height, wave, or flooding model.
"""
import math

import numpy as np
import shapely
from shapely.geometry import box
from shapely.strtree import STRtree


def layout(lake, spacing=1.0, limit=32.0):
    if not lake.is_valid or lake.is_empty or lake.geom_type != 'Polygon':
        raise ValueError('Expected a valid nonempty lake Polygon with its islands')
    if not math.isfinite(spacing) or spacing <= 0 or not math.isfinite(limit) or limit <= spacing:
        raise ValueError('Invalid footprint spacing or distance limit')
    west, south, east, north = lake.bounds
    padding = limit + 2*spacing
    west = math.floor((west-padding)/spacing)*spacing
    north = math.ceil((north+padding)/spacing)*spacing
    columns = math.ceil((east+padding-west)/spacing)
    rows = math.ceil((north-south+padding)/spacing)
    if max(rows, columns) > 16384:
        raise ValueError('Footprint exceeds the intended 2D texture dimensions')
    return dict(west=west, north=north, spacing_m=spacing, columns=columns, rows=rows,
                distance_limit_m=limit)


def generate(lake, grid, target, tile_size=256, progress=None):
    """Compute exact centre distances, truncated to the requested narrow band.

    Spatial queries retain all segments within `limit` of each tile. Missing
    segments therefore cannot alter its truncated distances. Tiles entirely
    farther away are constant, avoiding one geometric query per lake pixel.
    `target` may be a little-endian float16 memmap to bound memory use.
    """
    rows, cols = grid['rows'], grid['columns']
    if target.shape != (rows, cols) or tile_size < 1:
        raise ValueError('Invalid footprint output shape or tile size')
    spacing, limit = grid['spacing_m'], grid['distance_limit_m']
    west, north = grid['west'], grid['north']
    parts = []
    for ring in [lake.exterior, *lake.interiors]:
        coordinates = shapely.get_coordinates(ring)
        parts.append(np.stack([coordinates[:-1], coordinates[1:]], axis=1))
    segments = shapely.linestrings(np.concatenate(parts))
    tree = STRtree(segments)
    shapely.prepare(lake)
    tiles = math.ceil(rows/tile_size)*math.ceil(cols/tile_size)
    count = 0
    for row in range(0, rows, tile_size):
        end_row = min(rows, row+tile_size)
        for col in range(0, cols, tile_size):
            end_col = min(cols, col+tile_size)
            x0, x1 = west+col*spacing, west+end_col*spacing
            y0, y1 = north-end_row*spacing, north-row*spacing
            nearby = tree.query(box(x0-limit, y0-limit, x1+limit, y1+limit))
            output = target[row:end_row, col:end_col]
            if not len(nearby):
                output[:] = limit if shapely.contains_xy(lake, (x0+x1)*.5, (y0+y1)*.5) else -limit
            else:
                x, y = np.meshgrid(west+(np.arange(col, end_col)+.5)*spacing,
                                   north-(np.arange(row, end_row)+.5)*spacing)
                distance = shapely.distance(shapely.points(x, y), shapely.multilinestrings(segments[nearby]))
                distance = np.minimum(distance, limit)
                output[:] = np.where(shapely.contains_xy(lake, x, y), distance, -distance)
            count += 1
            if progress:
                progress(count, tiles)


def sample(grid, data, east, north):
    """Reference bilinear texture sampling at LV95 coordinates.

    Outside texture coverage returns dry land, never a clamped edge sample.
    The half-texel convention must also be respected by the future GPU upload.
    """
    east, north = np.broadcast_arrays(np.asarray(east, dtype=float), np.asarray(north, dtype=float))
    if not np.isfinite(east).all() or not np.isfinite(north).all():
        raise ValueError('Nonfinite footprint coordinates')
    x = (east-grid['west'])/grid['spacing_m']-.5
    y = (grid['north']-north)/grid['spacing_m']-.5
    valid = (x >= -.5) & (y >= -.5) & (x < grid['columns']-.5) & (y < grid['rows']-.5)
    x = np.clip(x, 0, grid['columns']-1)
    y = np.clip(y, 0, grid['rows']-1)
    col, row = np.floor(x).astype(int), np.floor(y).astype(int)
    right, bottom = np.minimum(col+1, grid['columns']-1), np.minimum(row+1, grid['rows']-1)
    tx, ty = x-col, y-row
    value = ((1-tx)*data[row, col].astype(float)+tx*data[row, right].astype(float))*(1-ty)
    value += ((1-tx)*data[bottom, col].astype(float)+tx*data[bottom, right].astype(float))*ty
    return np.where(valid, value, -grid['distance_limit_m'])
