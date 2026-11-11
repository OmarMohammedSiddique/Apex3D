#!/usr/bin/env python3
"""Turn the measured Monza centreline into the scene's track header.

Input: ``data/monza-2025-centreline.csv`` and its provenance file, both made by
``tools/build_track_centreline.py`` from recorded OpenF1 car positions.
Output: ``include/apex3d/generated/monza_track_2025.hpp``.

The scene frame
---------------
The scene keeps its 2D "scene units" (the CP2 curves work in them) and
``physical::metresPerSceneUnit()`` converts to metres. To keep cameras, the HUD
map and the pit lane code working, the measured track is placed so that:

* the finish line sits where the old hand-drawn one was, at (1.16, -0.87);
* the main straight runs in the -x direction, as before;
* one scene unit is ``METRES_PER_UNIT`` metres of real ground.

This is a rotation, a shift and one uniform scale: no bending. Every map
feature (OSM buildings, woods, gravel) goes through the same ``to_scene``
function, so it stays where it really is relative to the road.

The runtime then stretches everything uniformly by 5,793 / 5,758 (about 0.6%)
so the lap length matches the official circuit length used by the timing code.
The measured line is the line the cars drove, which is shorter than the painted
centreline because it cuts across each chicane.

Legacy estimates
----------------
A few things were placed by hand against the old 62-anchor drawing: turn
markers for DRS boards and the old sector guesses. ``legacy_to_distance``
carries an old hand-drawn point to the same place on the measured track by
matching the 14 corner landmarks recorded in ``data/monza-scenery.json``.
"""
import bisect
import csv
import datetime as dt
import json
import math
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OFFICIAL_LAP_M = 5793.0
METRES_PER_UNIT = 675.0  # keeps the lap close to the old 8.6 scene units
CONTROL_SPACING_M = 10.0
FINISH_SCENE = (1.16, -0.87)

# The original hand-authored drawing. Kept only to migrate positions that were
# estimated against it; it no longer draws anything.
LEGACY_CONTROLS = [
    (1.16, -0.87), (0.82, -0.87), (0.45, -0.87), (0.08, -0.87), (-0.25, -0.87), (-0.46, -0.87),
    (-0.49, -0.84), (-0.49, -0.78), (-0.52, -0.75), (-0.58, -0.76), (-0.67, -0.81),
    (-0.80, -0.84), (-0.94, -0.83), (-1.07, -0.77), (-1.18, -0.67), (-1.25, -0.54), (-1.30, -0.38),
    (-1.33, 0.00), (-1.35, 0.18), (-1.39, 0.23), (-1.46, 0.24), (-1.49, 0.28), (-1.50, 0.35),
    (-1.54, 0.49), (-1.58, 0.63), (-1.58, 0.70), (-1.55, 0.76), (-1.49, 0.80), (-1.39, 0.83),
    (-1.25, 0.85), (-1.10, 0.88), (-0.95, 0.90), (-0.87, 0.88), (-0.81, 0.81),
    (-0.72, 0.68), (-0.61, 0.51), (-0.51, 0.35), (-0.39, 0.20), (-0.25, 0.06), (-0.10, -0.08),
    (0.05, -0.21), (0.18, -0.32), (0.26, -0.38), (0.33, -0.39), (0.40, -0.36), (0.48, -0.35),
    (0.56, -0.37), (0.63, -0.42), (0.70, -0.44), (0.91, -0.45), (1.14, -0.46), (1.36, -0.46),
    (1.51, -0.47), (1.59, -0.50), (1.64, -0.56), (1.66, -0.63), (1.65, -0.69), (1.61, -0.75),
    (1.54, -0.80), (1.43, -0.84), (1.30, -0.86), (1.17, -0.87),
]


def load():
    rows = list(csv.DictReader((ROOT / 'data/monza-2025-centreline.csv').open(newline='')))
    provenance = json.loads((ROOT / 'data/monza-2025-centreline-provenance.json').read_text())
    frame = provenance['osm_fit']['local_frame']
    return rows, provenance, frame


ROWS, PROVENANCE, FRAME = load()


def geo_metres(lon, lat):
    """Longitude/latitude to local east/north metres (same frame as the scenery tools)."""
    return ((lon - FRAME['lon0']) * FRAME['m_per_deg_lon'], (lat - FRAME['lat0']) * FRAME['m_per_deg_lat'])


GEO = [geo_metres(float(r['lon']), float(r['lat'])) for r in ROWS]
# Heading of the first 300 m of the main straight sets the rotation.
_dx, _dy = GEO[60][0] - GEO[0][0], GEO[60][1] - GEO[0][1]
_ANGLE = math.pi - math.atan2(_dy, _dx)  # rotate that heading onto -x


def to_scene(lon, lat):
    x, y = geo_metres(lon, lat)
    x, y = x - GEO[0][0], y - GEO[0][1]
    c, s = math.cos(_ANGLE), math.sin(_ANGLE)
    return (FINISH_SCENE[0] + (c * x - s * y) / METRES_PER_UNIT,
            FINISH_SCENE[1] + (s * x + c * y) / METRES_PER_UNIT)


SCENE = [to_scene(float(r['lon']), float(r['lat'])) for r in ROWS]
ELEVATION = [float(r['elevation_m']) for r in ROWS]
MEASURED_LAP_M = PROVENANCE['lap_length_m']
STRETCH = OFFICIAL_LAP_M / MEASURED_LAP_M


def closed_cumulative(points):
    out = [0.0]
    for a, b in zip(points, points[1:] + points[:1]):
        out.append(out[-1] + math.dist(a, b))
    return out


SCENE_CUM = closed_cumulative(SCENE)  # scene units, closed


def point_at(fraction):
    """Point on the measured line at a fraction of the lap."""
    target = (fraction % 1.0) * SCENE_CUM[-1]
    k = bisect.bisect_right(SCENE_CUM, target) - 1
    a, b = SCENE[k], SCENE[(k + 1) % len(SCENE)]
    t = (target - SCENE_CUM[k]) / (SCENE_CUM[k + 1] - SCENE_CUM[k])
    return (a[0] + t * (b[0] - a[0]), a[1] + t * (b[1] - a[1]))


def nearest_fraction(points, cum, p):
    best = None
    for k in range(len(points)):
        a, b = points[k], points[(k + 1) % len(points)]
        dx, dy = b[0] - a[0], b[1] - a[1]
        t = max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / (dx * dx + dy * dy)))
        d = math.hypot(p[0] - a[0] - t * dx, p[1] - a[1] - t * dy)
        if best is None or d < best[0]:
            best = (d, (cum[k] + t * (cum[k + 1] - cum[k])) / cum[-1])
    return best[1]


LEGACY_CUM = closed_cumulative(LEGACY_CONTROLS)


def landmark_pairs():
    """(old fraction, new fraction) for each recorded corner landmark, finish line fixed at 0."""
    anchors = json.loads((ROOT / 'data/monza-scenery.json').read_text())['anchors']
    pairs = [(0.0, 0.0)]
    for lon, lat, ox, oy in anchors[1:]:  # anchor 0 is the OSM way start, not the timing line
        old = nearest_fraction(LEGACY_CONTROLS, LEGACY_CUM, (ox, oy))
        new = nearest_fraction(SCENE, SCENE_CUM, to_scene(lon, lat))
        pairs.append((old, new))
    pairs.sort()
    assert all(a[1] < b[1] for a, b in zip(pairs, pairs[1:])), 'landmarks out of order'
    return pairs + [(1.0, 1.0)]


PAIRS = landmark_pairs()


def legacy_to_fraction(point):
    """Carry a point estimated on the old drawing to the measured track (lap fraction)."""
    old = nearest_fraction(LEGACY_CONTROLS, LEGACY_CUM, point)
    k = bisect.bisect_right([a for a, _ in PAIRS], old) - 1
    (a0, b0), (a1, b1) = PAIRS[k], PAIRS[min(k + 1, len(PAIRS) - 1)]
    return b0 + (old - a0) * (b1 - b0) / (a1 - a0) if a1 > a0 else b0


def controls():
    """Evenly spaced control points every CONTROL_SPACING_M real metres."""
    count = round(MEASURED_LAP_M / CONTROL_SPACING_M)
    return [point_at(i / count) for i in range(count)], \
           [ELEVATION[round(i / count * len(ELEVATION)) % len(ELEVATION)] for i in range(count)]


def header():
    points, heights = controls()
    sectors = PROVENANCE['sector_lines']
    landmarks = {
        # Old hand-placed turn markers (DRS boards) carried to the measured track.
        'turn1': legacy_to_fraction((-0.49, -0.84)),
        'turn7': legacy_to_fraction((-0.95, 0.90)),
        'turn8': legacy_to_fraction((0.33, -0.39)),
    }
    base = min(heights)
    text = '#pragma once\n// Generated by tools/generate_track_assets.py from data/monza-2025-centreline.csv.\n'
    text += '// Measured from recorded OpenF1 car positions (session 9912); see docs/real-track-geometry.md.\n'
    text += '#include <array>\n\nnamespace apex3d::cp3 {\nstruct TrackPoint {double x, y;};\n'
    text += f'inline constexpr double measuredLapMetres = {MEASURED_LAP_M};\n'
    text += f'inline constexpr double trackControlSpacingMetres = {CONTROL_SPACING_M};\n'
    text += f'inline constexpr std::array<TrackPoint, {len(points)}> measuredMonzaControls{{{{\n'
    text += ''.join(f'    {{{x:.6f}, {y:.6f}}},\n' for x, y in points) + '}};\n'
    text += '// Height above the lowest point of the lap (m), one per control point.\n'
    text += '// Rendered as terrain (include/apex3d/terrain.hpp).\n'
    text += f'inline constexpr std::array<double, {len(heights)}> measuredMonzaElevation{{{{\n'
    for i in range(0, len(heights), 12):
        text += '    ' + ', '.join(f'{h - base:.2f}' for h in heights[i:i + 12]) + ',\n'
    text += '}};\n'
    text += '// Lap fractions (0 = finish line). Sectors are measured; turn markers are carried estimates.\n'
    text += f'inline constexpr double sector1Fraction = {sectors["sector1_end_m"] / MEASURED_LAP_M:.6f};\n'
    text += f'inline constexpr double sector2Fraction = {sectors["sector2_end_m"] / MEASURED_LAP_M:.6f};\n'
    for name, value in landmarks.items():
        text += f'inline constexpr double {name}Fraction = {value:.6f};\n'
    # Inputs for the solar position (include/apex3d/sun_position.hpp).
    origin = dt.datetime.fromisoformat(
        json.loads((ROOT / 'data/monza-2025-provenance.json').read_text())['race_origin_utc'])
    text += '// Sun inputs: finish-line coordinates, the recorded race-clock origin (race time 0,\n'
    text += '// OpenF1 session 9912) and true north in scene axes (the scene is a rotation of east/north).\n'
    text += f'inline constexpr double siteLatitude = {float(ROWS[0]["lat"]):.6f};\n'
    text += f'inline constexpr double siteLongitude = {float(ROWS[0]["lon"]):.6f};\n'
    text += f'inline constexpr double raceStartUnixSeconds = {origin.timestamp():.3f}; // {origin.isoformat()}\n'
    text += f'inline constexpr double sceneNorthX = {-math.sin(_ANGLE):.6f};\n'
    text += f'inline constexpr double sceneNorthY = {math.cos(_ANGLE):.6f};\n'
    text += '} // namespace apex3d::cp3\n'
    return text


def migrate_stands():
    """One-off: move grandstand anchors from the old drawing to the measured track."""
    path = ROOT / 'data/monza-2025-grandstands.csv'
    rows = list(csv.DictReader(path.open(newline='')))
    for row in rows:
        x, y = point_at(legacy_to_fraction((float(row['anchor_x']), float(row['anchor_y']))))
        row['anchor_x'], row['anchor_y'] = f'{x:.4f}', f'{y:.4f}'
    with path.open('w', newline='') as out:
        writer = csv.DictWriter(out, fieldnames=list(rows[0]), lineterminator='\n')
        writer.writeheader()
        writer.writerows(rows)


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument('--check', action='store_true')
    parser.add_argument('--migrate-stands', action='store_true')
    args = parser.parse_args()
    target = ROOT / 'include/apex3d/generated/monza_track_2025.hpp'
    if args.migrate_stands:
        migrate_stands()
    elif args.check:
        assert target.read_text() == header(), 'monza_track_2025.hpp is out of date'
    else:
        target.write_text(header())
        print('pairs', [(round(a, 3), round(b, 3)) for a, b in PAIRS])
