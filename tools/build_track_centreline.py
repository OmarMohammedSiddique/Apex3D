#!/usr/bin/env python3
"""Build Monza's centreline from recorded OpenF1 car positions (session 9912).

What this does, step by step:

1. Load the recorded x/y/z car positions (``/v1/location``, about 3.7 samples
   per second, decimetre units) and lap start times (``/v1/laps``) for five
   cars that ran the whole race: VER, NOR, PIA, LEC and RUS.
2. Keep only clean green-flag laps: no lap 1 (standing start), no pit-in or
   pit-out laps, and only laps within 3% of that driver's median lap time.
3. Use one clean Verstappen lap as a first guess of the track shape.
4. Project every kept sample onto the current guess, giving a distance along
   the lap and a sideways offset. Average the offsets in 5 m bins, smooth them
   and move the guess sideways by that average. Repeat. The result is the
   average line the cars drove, not a painted centreline.
5. Put distance 0 where the timing loop is: the average place where cars
   were when OpenF1 says a new lap started.
6. Fit the result onto OpenStreetMap's mapped Grand Prix raceway ways with a
   rotation, scale and shift (no bending). This gives every point a real
   longitude/latitude and reports how far the two sources disagree.

The cached downloads live in the ignored ``tmp/openf1/`` and ``tmp/monza-world/``
folders. Their SHA-256 hashes must match ``data/monza-2025-full-field-provenance.json``
and ``data/monza-scenery.json`` respectively, so the inputs are the same bytes the
rest of the project already uses (except the new location downloads, whose
hashes are recorded in the output provenance file).

Outputs: ``data/monza-2025-centreline.csv`` and
``data/monza-2025-centreline-provenance.json``.
Run ``python3 tools/build_track_centreline.py`` to rebuild (downloads missing
files), or add ``--offline`` to use only the caches.
"""
import argparse
import bisect
import csv
import hashlib
import json
import math
import statistics
import time
import urllib.request
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / 'tmp' / 'openf1'
OSM_FEATURES = ROOT / 'tmp' / 'monza-world' / 'features.json'
OSM_RAW = ROOT / 'tmp' / 'monza-world' / 'osm.xml'
SESSION = 9912
DRIVERS = {1: 'VER', 4: 'NOR', 81: 'PIA', 16: 'LEC', 63: 'RUS'}
REFERENCE_DRIVER, REFERENCE_LAP = 1, 10
BIN_M = 5.0
# OSM ways that make up the 2025 Grand Prix layout (same list as the scenery fit).
GP_WAYS = [19842206, 179968220, 179968234, 179968229, 179968230, 179968239,
           179968245, 179968257, 179968252, 179968251, 179968263, 179968262,
           179968264, 179968242, 179968228, 179968254, 179968226, 179968249,
           1443867792, 1443867793]
# Local metres per degree at Monza; identical to the scenery script so the
# longitude/latitude frame matches data/monza-scenery.json exactly.
LON0, LAT0, M_PER_DEG_LON, M_PER_DEG_LAT = 9.285, 45.622, 77800.0, 111200.0


def url_for(kind, driver):
    return f'https://api.openf1.org/v1/{kind}?session_key={SESSION}&driver_number={driver}'


def fetch(kind, driver, offline):
    path = CACHE / f'{kind}-{driver}.json'
    if not path.exists():
        if offline:
            raise SystemExit(f'missing cache {path}; run without --offline')
        CACHE.mkdir(parents=True, exist_ok=True)
        request = urllib.request.Request(url_for(kind, driver),
                                         headers={'User-Agent': 'Apex3D-coursework-data-fetch'})
        with urllib.request.urlopen(request, timeout=180) as response:
            path.write_bytes(response.read())
        time.sleep(0.5)  # OpenF1 allows 3 requests per second
    data = path.read_bytes()
    return json.loads(data), hashlib.sha256(data).hexdigest()


def when(text):
    return datetime.fromisoformat(text).timestamp()


# ---------- small 2D helpers (no numpy, so the tool runs anywhere) ----------

def cumulative(points):
    out = [0.0]
    for a, b in zip(points, points[1:] + points[:1]):
        out.append(out[-1] + math.dist(a[:2], b[:2]))
    return out  # closed loop: last value is the full lap length


def resample(points, spacing):
    """Evenly spaced points around a closed loop (any extra coordinates are interpolated too)."""
    cum = cumulative(points)
    count = round(cum[-1] / spacing)
    out = []
    for i in range(count):
        target = cum[-1] * i / count
        k = bisect.bisect_right(cum, target) - 1
        a, b = points[k], points[(k + 1) % len(points)]
        t = (target - cum[k]) / (cum[k + 1] - cum[k])
        out.append(tuple(p + t * (q - p) for p, q in zip(a, b)))
    return out


def smooth_circular(values, sigma_bins):
    n = len(values)
    reach = int(3 * sigma_bins)
    weights = [math.exp(-0.5 * (k / sigma_bins) ** 2) for k in range(-reach, reach + 1)]
    total = sum(weights)
    return [sum(w * values[(i + k) % n] for w, k in zip(weights, range(-reach, reach + 1))) / total
            for i in range(n)]


def normals(points):
    n = len(points)
    out = []
    for i in range(n):
        ax, ay = points[i - 1][:2]
        bx, by = points[(i + 1) % n][:2]
        length = math.hypot(bx - ax, by - ay)
        out.append((-(by - ay) / length, (bx - ax) / length))
    return out


def project_segment(p, a, b):
    dx, dy = b[0] - a[0], b[1] - a[1]
    t = max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / (dx * dx + dy * dy)))
    qx, qy = a[0] + t * dx, a[1] + t * dy
    return t, math.hypot(p[0] - qx, p[1] - qy), (p[0] - qx) * -dy + (p[1] - qy) * dx


def project_lap(samples, line):
    """Walk along one lap, projecting each sample near the previous match."""
    n = len(line)
    index = None
    out = []
    for p in samples:
        window = range(n) if index is None else range(index - 8, index + 40)
        best = None
        for k in window:
            k %= n
            t, d, side = project_segment(p, line[k], line[(k + 1) % n])
            if best is None or d < best[1]:
                best = (k + t, d, side, k)
        if best[1] > 40.0:  # far from the circuit: pit lane or data glitch
            index = None
            continue
        index = best[3]
        seg = math.dist(line[best[3]][:2], line[(best[3] + 1) % n][:2])
        out.append((best[0], math.copysign(best[1], best[2]) / 1.0, p[2], seg))
    return out


# ------------------------------- data loading -------------------------------

def clean_laps(laps, positions):
    """Return [(lap_number, start, end, samples)] for clean racing laps."""
    by_number = {lap['lap_number']: lap for lap in laps}
    durations = [lap['lap_duration'] for lap in laps if lap['lap_duration']]
    median = statistics.median(durations)
    times = [p[0] for p in positions]
    kept = []
    for number, lap in sorted(by_number.items()):
        nxt = by_number.get(number + 1)
        if number == 1 or not lap['lap_duration'] or not lap['date_start']:
            continue
        if lap['is_pit_out_lap'] or (nxt and nxt['is_pit_out_lap']):
            continue
        if abs(lap['lap_duration'] - median) > 0.03 * median:
            continue
        start = when(lap['date_start'])
        end = start + lap['lap_duration']
        lo, hi = bisect.bisect_left(times, start), bisect.bisect_right(times, end)
        samples = [p[1:] for p in positions[lo:hi]]
        if len(samples) > 200:
            kept.append((number, start, end, samples))
    return kept


def position_at(positions, moment):
    times = [p[0] for p in positions]
    k = bisect.bisect_left(times, moment)
    (t0, *a), (t1, *b) = positions[k - 1], positions[k]
    alpha = (moment - t0) / (t1 - t0)
    return tuple(p + alpha * (q - p) for p, q in zip(a, b))


def load(offline):
    hashes, cars = {}, {}
    for driver in DRIVERS:
        location, hashes[f'location-{driver}'] = fetch('location', driver, offline)
        laps, hashes[f'laps-{driver}'] = fetch('laps', driver, offline)
        positions = sorted((when(r['date']), r['x'] / 10.0, r['y'] / 10.0, r['z'] / 10.0)
                           for r in location if (r['x'], r['y']) != (0, 0))
        cars[driver] = (laps, positions)
    recorded = json.loads((ROOT / 'data/monza-2025-full-field-provenance.json').read_text())['sources']
    for key, value in hashes.items():
        if key in recorded:
            assert recorded[key]['sha256'] == value, f'{key} differs from the recorded download'
    return cars, hashes


# ------------------------------ centreline fit ------------------------------

def build_line(cars):
    laps, positions = cars[REFERENCE_DRIVER]
    reference = next(l for l in clean_laps(laps, positions) if l[0] == REFERENCE_LAP)
    line = resample([s for s in reference[3]], BIN_M)
    all_laps = [(d, lap) for d, (laps, pos) in cars.items() for lap in clean_laps(laps, pos)]
    for _ in range(4):
        n = len(line)
        offset_sum, z_sum, count = [0.0] * n, [0.0] * n, [0] * n
        for _, lap in all_laps:
            for s, side, z, _ in project_lap(lap[3], line):
                k = int(round(s)) % n
                offset_sum[k] += side
                z_sum[k] += z
                count[k] += 1
        assert min(count) > 0, 'a 5 m bin had no samples'
        offsets = smooth_circular([o / c for o, c in zip(offset_sum, count)], 2.0)
        heights = smooth_circular([z / c for z, c in zip(z_sum, count)], 4.0)
        norms = normals(line)
        moved = [(x + o * nx, y + o * ny, h)
                 for (x, y, *_), o, (nx, ny), h in zip(line, offsets, norms, heights)]
        moved = [(x, y, h) for (x, y, _), h in zip(smooth_xy(moved), heights)]
        line = resample(moved, BIN_M)
    return line, all_laps, count


def smooth_xy(points):
    xs = smooth_circular([p[0] for p in points], 1.0)
    ys = smooth_circular([p[1] for p in points], 1.0)
    return [(x, y, p[2]) for x, y, p in zip(xs, ys, points)]


def sector_lines(line, cars):
    """Mean distance from the finish line at which OpenF1 sector 1 and 2 ended."""
    cum = cumulative(line)
    found = {1: [], 2: []}
    for laps, positions in cars.values():
        for lap in laps:
            if lap['lap_number'] < 2 or not lap['date_start'] or lap['is_pit_out_lap']:
                continue
            if not lap['duration_sector_1'] or not lap['duration_sector_2']:
                continue
            start = when(lap['date_start'])
            for sector, seconds in ((1, lap['duration_sector_1']),
                                    (2, lap['duration_sector_1'] + lap['duration_sector_2'])):
                p = position_at(positions, start + seconds)
                k = min(range(len(line)), key=lambda i: math.dist(p[:2], line[i][:2]))
                found[sector].append(cum[k])
    return {f'sector{s}_end_m': round(statistics.median(v), 1) for s, v in found.items()} | \
        {'laps_used': len(found[1]), 'method': 'median distance of cars at OpenF1 lap start + sector durations'}


def residuals(line, all_laps):
    """Sideways spread of individual samples around the final line (metres)."""
    values = [abs(side) for _, lap in all_laps for _, side, _, _ in project_lap(lap[3], line)]
    values.sort()
    return values[len(values) // 2], values[int(0.95 * len(values))]


def seam(line, cars):
    """Mean along-track position of cars at OpenF1 lap starts (laps 2-53)."""
    n = len(line)
    angles = []
    for laps, positions in cars.values():
        for lap in laps:
            if lap['lap_number'] < 2 or not lap['date_start'] or lap['is_pit_out_lap']:
                continue
            p = position_at(positions, when(lap['date_start']))
            best = min(range(n), key=lambda k: math.dist(p[:2], line[k][:2]))
            angles.append(2 * math.pi * best / n)
    mean = math.atan2(sum(map(math.sin, angles)), sum(map(math.cos, angles)))
    return round((mean % (2 * math.pi)) / (2 * math.pi) * n) % n, len(angles)


# ------------------------------ georeferencing ------------------------------

def osm_track():
    raw = hashlib.sha256(OSM_RAW.read_bytes()).hexdigest()
    scenery = json.loads((ROOT / 'data/monza-scenery.json').read_text())
    assert raw == scenery['raw_sha256'], 'OSM download differs from the scenery source'
    features = json.loads(OSM_FEATURES.read_text())
    ways = [f for f in features if f['kind'] == 'raceway' and int(f['id']) in GP_WAYS]
    assert len(ways) == len(GP_WAYS)
    points = []
    for way in ways:
        geo = [((lon - LON0) * M_PER_DEG_LON, (lat - LAT0) * M_PER_DEG_LAT) for lon, lat in way['points']]
        for a, b in zip(geo, geo[1:]):  # densify to ~2 m so every metre of road is represented
            steps = max(1, int(math.dist(a, b) / 2.0))
            points += [(a[0] + (b[0] - a[0]) * i / steps, a[1] + (b[1] - a[1]) * i / steps) for i in range(steps)]
    return points, raw


def similarity(src, dst, reflect):
    """Least-squares rotation + uniform scale + shift mapping src onto dst."""
    n = len(src)
    sx = sum(p[0] for p in src) / n; sy = sum(p[1] for p in src) / n
    dx = sum(p[0] for p in dst) / n; dy = sum(p[1] for p in dst) / n
    a = b = var = 0.0
    sign = -1.0 if reflect else 1.0
    for (px, py), (qx, qy) in zip(src, dst):
        px, py, qx, qy = px - sx, sign * (py - sy), qx - dx, qy - dy
        a += px * qx + py * qy
        b += px * qy - py * qx
        var += px * px + py * py
    scale = math.hypot(a, b) / var
    angle = math.atan2(b, a)
    return {'scale': scale, 'angle': angle, 'reflect': reflect,
            'src': (sx, sy), 'dst': (dx, dy)}


def apply(t, p):
    x, y = p[0] - t['src'][0], p[1] - t['src'][1]
    if t['reflect']:
        y = -y
    c, s = math.cos(t['angle']) * t['scale'], math.sin(t['angle']) * t['scale']
    return (t['dst'][0] + c * x - s * y, t['dst'][1] + s * x + c * y)


def nearest(points, p, grid):
    cell = (int(p[0] // 50), int(p[1] // 50))
    best = None
    for gx in range(cell[0] - 1, cell[0] + 2):
        for gy in range(cell[1] - 1, cell[1] + 2):
            for q in grid.get((gx, gy), ()):
                d = math.dist(p, q)
                if best is None or d < best[0]:
                    best = (d, q)
    return best


def fit_to_osm(line):
    osm, raw = osm_track()
    grid = {}
    for q in osm:
        grid.setdefault((int(q[0] // 50), int(q[1] // 50)), []).append(q)
    xy = [p[:2] for p in line]
    best = None
    for reflect in (False, True):
        for start_deg in range(0, 360, 15):  # coarse start, then iterative closest point
            ox = sum(p[0] for p in osm) / len(osm); oy = sum(p[1] for p in osm) / len(osm)
            lx = sum(p[0] for p in xy) / len(xy); ly = sum(p[1] for p in xy) / len(xy)
            t = {'scale': 1.0, 'angle': math.radians(start_deg), 'reflect': reflect,
                 'src': (lx, ly), 'dst': (ox, oy)}
            for _ in range(30):
                pairs = [(p, nearest(osm, apply(t, p), grid)) for p in xy]
                pairs = [(p, m[1]) for p, m in pairs if m and m[0] < 60]
                if len(pairs) < 0.8 * len(xy):
                    break
                t = similarity([p for p, _ in pairs], [q for _, q in pairs], reflect)
            gaps = [nearest(osm, apply(t, p), grid) for p in xy]
            if any(g is None for g in gaps):
                continue
            gaps = sorted(g[0] for g in gaps)
            score = sum(gaps) / len(gaps)
            if best is None or score < best[0]:
                best = (score, t, gaps)
    score, t, gaps = best
    return t, {'mean_m': round(score, 2), 'median_m': round(gaps[len(gaps) // 2], 2),
               'p95_m': round(gaps[int(0.95 * len(gaps))], 2), 'max_m': round(gaps[-1], 2)}, raw


# ---------------------------------- main ------------------------------------

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--offline', action='store_true')
    args = parser.parse_args()
    cars, hashes = load(args.offline)
    line, all_laps, counts = build_line(cars)
    start, crossings = seam(line, cars)
    line = line[start:] + line[:start]
    median_spread, p95_spread = residuals(line, all_laps)
    sectors = sector_lines(line, cars)
    transform, osm_gap, osm_raw = fit_to_osm(line)
    length = cumulative(line)[-1]
    rows = []
    for (x, y, z), s in zip(line, cumulative(line)):
        ex, ny = apply(transform, (x, y))
        rows.append({'distance_m': f'{s:.1f}', 'openf1_x_m': f'{x:.3f}', 'openf1_y_m': f'{y:.3f}',
                     'elevation_m': f'{z:.2f}',
                     'lon': f'{LON0 + ex / M_PER_DEG_LON:.9f}', 'lat': f'{LAT0 + ny / M_PER_DEG_LAT:.9f}'})
    with (ROOT / 'data/monza-2025-centreline.csv').open('w', newline='') as out:
        writer = csv.DictWriter(out, fieldnames=list(rows[0]))
        writer.writeheader()
        writer.writerows(rows)
    lap_counts = {DRIVERS[d]: sum(1 for dd, _ in all_laps if dd == d) for d in DRIVERS}
    provenance = {
        'session_key': SESSION,
        'method': 'Average driven line of clean green-flag laps from OpenF1 /v1/location, '
                  'iteratively re-projected in 5 m bins, then fitted to OpenStreetMap GP raceway ways '
                  'with a similarity transform (rotation, uniform scale, shift). See tools/build_track_centreline.py.',
        'drivers': lap_counts,
        'reference_lap': {'driver': DRIVERS[REFERENCE_DRIVER], 'lap': REFERENCE_LAP},
        'points': len(rows), 'spacing_m': BIN_M,
        'lap_length_m': round(length, 1),
        'official_lap_length_m': 5793.0,
        'start_finish': {'method': 'mean along-track position of cars at OpenF1 lap start times (laps 2+)',
                         'crossings_used': crossings},
        'sector_lines': sectors,
        'elevation': {'datum': 'OpenF1 z, arbitrary origin', 'min_m': round(min(p[2] for p in line), 2),
                      'max_m': round(max(p[2] for p in line), 2), 'smoothing_sigma_m': 4 * BIN_M},
        'sample_spread_from_line_m': {'median': round(median_spread, 2), 'p95': round(p95_spread, 2)},
        'osm_fit': {'ways': GP_WAYS, 'raw_sha256': osm_raw, 'scale': round(transform['scale'], 5),
                    'rotation_deg': round(math.degrees(transform['angle']), 3),
                    'reflected': transform['reflect'], 'distance_to_osm_centreline': osm_gap,
                    'local_frame': {'lon0': LON0, 'lat0': LAT0, 'm_per_deg_lon': M_PER_DEG_LON,
                                    'm_per_deg_lat': M_PER_DEG_LAT}},
        'sources': {k: {'url': url_for(*k.split('-')), 'sha256': v} for k, v in sorted(hashes.items())},
        'licence': 'OpenF1 data (unofficial, non-commercial); OSM ODbL 1.0 for the georeference.',
    }
    (ROOT / 'data/monza-2025-centreline-provenance.json').write_text(json.dumps(provenance, indent=1) + '\n')
    print(json.dumps({k: provenance[k] for k in ('drivers', 'points', 'lap_length_m', 'elevation', 'sector_lines',
                                                 'sample_spread_from_line_m')}, indent=1))
    print('OSM fit', provenance['osm_fit']['scale'], provenance['osm_fit']['rotation_deg'],
          provenance['osm_fit']['reflected'], osm_gap)


if __name__ == '__main__':
    main()
