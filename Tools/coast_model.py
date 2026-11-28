"""Sparse Dirichlet interpolation for explicitly unmeasured lake cells."""
import numpy as np
from scipy.sparse import coo_matrix
from scipy.sparse.linalg import cg


def solve(values, lake, surveyed, tolerance=1e-5, progress=None):
    """Keep surveys/land fixed; solve only inside the outline without bathymetry.

    The shoreline-side solver constraint cannot exceed the scenario water level.
    The actual outside land values are retained, including above-water terrain.
    This is a game terrain estimate, not a reconstruction of measured depths.
    """
    values = np.asarray(values, dtype=np.float32)
    lake, surveyed = np.asarray(lake, dtype=bool), np.asarray(surveyed, dtype=bool)
    if values.ndim != 2 or min(values.shape, default=0) < 3 or values.shape != lake.shape or values.shape != surveyed.shape or not np.isfinite(values).all():
        raise ValueError("Invalid coast constraints")
    if not np.isfinite(tolerance) or tolerance <= 0:
        raise ValueError("Invalid solver tolerance")
    unknown = lake & ~surveyed
    if unknown[0].any() or unknown[-1].any() or unknown[:, 0].any() or unknown[:, -1].any():
        raise ValueError("Unknown cells touch the model boundary")
    flat = np.flatnonzero(unknown)
    if len(flat) == 0:
        return values.copy(), dict(unknown_cells=0, iterations=0, residual_m=0.0)
    if not surveyed.any():
        raise ValueError("Lake has no surveyed bathymetry")
    identifiers = np.full(values.size, -1, dtype=np.int32)
    identifiers[flat] = np.arange(len(flat), dtype=np.int32)
    indices = np.arange(len(flat), dtype=np.int32)
    rows, columns, coefficients = [indices], [indices], [np.full(len(flat), 4.0)]
    right = np.zeros(len(flat), dtype=np.float64)
    boundary_count = 0
    for offset in (-values.shape[1], values.shape[1], -1, 1):
        neighbours = flat+offset
        other = identifiers[neighbours]
        free = other >= 0
        rows.append(indices[free]); columns.append(other[free]); coefficients.append(np.full(int(free.sum()), -1.0))
        fixed = ~free
        boundary_values = values.ravel()[neighbours[fixed]].astype(np.float64)
        boundary_count += int((boundary_values > 0).sum())
        right[fixed] += np.minimum(boundary_values, 0)
    matrix = coo_matrix((np.concatenate(coefficients), (np.concatenate(rows), np.concatenate(columns))),
                        shape=(len(flat), len(flat))).tocsr()
    del rows, columns, coefficients, identifiers
    iterations = 0

    def advance(x):
        nonlocal iterations
        iterations += 1
        if progress and iterations % 100 == 0:
            progress(iterations)

    result, status = cg(matrix, right, x0=np.minimum(values.ravel()[flat], 0).astype(np.float64),
                        rtol=1e-11, atol=1e-10, maxiter=10000, callback=advance)
    residual = float(np.max(np.abs(matrix@result-right)))*.25
    if status != 0 or residual > tolerance or not np.isfinite(result).all() or result.max() > tolerance:
        raise ValueError(f"Coast solver did not converge: status={status}, residual={residual}")
    output = values.copy(); output.ravel()[flat] = result.astype(np.float32)
    stored_residual = float(np.max(np.abs(matrix@output.ravel()[flat].astype(np.float64)-right)))*.25
    if stored_residual > tolerance:
        raise ValueError(f"Float32 coast residual exceeds tolerance: {stored_residual}")
    return output, dict(unknown_cells=len(flat), iterations=iterations, residual_m=residual,
                        stored_residual_m=stored_residual, shore_constraints_clipped_to_water=boundary_count,
                        estimated_depth_range_m=[float(-result.max()), float(-result.min())])
