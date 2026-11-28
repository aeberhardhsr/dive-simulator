"""Validate memory observations without treating complete sampling as leak freedom."""
import json
import math


def read_memory(path, production, requested_seconds):
    if not path.is_file():
        if production:
            raise ValueError('Production soak is missing private-memory observations')
        return None
    samples = [json.loads(line) for line in path.read_text(encoding='utf-8-sig').splitlines()]
    if not samples or any(not math.isfinite(row[key]) or row[key] <= 0 for row in samples
                          for key in ['wall_since_launch_seconds', 'private_bytes', 'working_set_bytes', 'virtual_bytes']):
        raise ValueError('Invalid host-memory observations')
    if any(b['wall_since_launch_seconds'] <= a['wall_since_launch_seconds'] for a,b in zip(samples,samples[1:])):
        raise ValueError('Non-monotonic host-memory observations')
    if any(not isinstance(row['production_verified'], bool) for row in samples):
        raise ValueError('Invalid production verification state')
    required = max(1, int(requested_seconds//30)-2)
    if production and sum(row['production_verified'] for row in samples) < required:
        raise ValueError(f'Incomplete verified memory sampling: need at least {required} records')
    # Preserve warmup, peaks and growth for the later route-phase review.
    return samples
