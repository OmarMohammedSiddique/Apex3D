#!/usr/bin/env python3
"""Measured track width from Regione Lombardia's topographic database (DBGT).

The DBGT for the comune of Monza (ISTAT 108033, CC BY 4.0) maps the circuit's paved
surface as "attrezzatura sportiva" polygons of type '08' (pista). Four of them cover
the Grand Prix lap.

  --extract DBGT_GPKG   reads those polygons from the official GeoPackage (inside the
                        downloaded MONZA.zip) and writes data/monza-track-surface.json,
                        keeping the survey coordinates unchanged (EPSG:7791).
  edge_profile()        left/right edge distances from the measured line at every
                        centreline point, used by generate_track_assets.header().

How the edges are measured (all distances in metres, from the measured line):
  1. At each 5 m centreline point, cast rays perpendicular to the line, left and
     right, to the nearest polygon boundary (at most 60 m away).
  2. Points outside the polygons (the line passes under the old banking, and at Lesmo
     the survey and the timing-derived line differ by a few metres) have no edges.
     Each such gap is widened, on both sides, while the measured width next to it is
     below 85% of the lap's median width: there the line is cutting across the
     surveyed edge and the perpendicular measurement is unreliable.
  3. Pit lane: on the right, where the pit lane is fully separated from the track
     (|offset| >= 10.5 m, as for the pit wall), the edge stops 0.5 m short of the pit
     wall line (3 m in from the lane centre). Beyond it the survey continues into the
     pit lane, which is modelled separately.
  4. Run-off: the "pista" polygon also includes asphalt run-off. A side edge that jumps
     outwards by more than 3 m from one point to the next starts a run-off section. It
     ends at the first point back within 1.5 m of the edge before the jump, or where the
     edge, back within 3 m of it, stops narrowing (less than 0.1 m per 5 m).
  5. Missing points and run-off sections are filled by linear interpolation between
     the nearest measured points, then each edge gets a 5-point (20 m) moving average,
     or 9 points (40 m) along the pit wall, where the surveyed boundary keeps
     switching between the wall and the pit lane behind it.
  6. The cars follow the timing-derived line, which sits up to a metre or two from
     the survey in places. Neither edge is placed closer than 1.5 m to that line, so a
     car on it stays on the asphalt.
"""
import argparse, hashlib, json, math, sqlite3, struct
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / 'data/monza-track-surface.json'
LAP_POLYGONS = (2, 110, 113, 148)
RAY_MAX_M = 60.0
PIT_WALL_FROM_M, PIT_WALL_INSET_M, PIT_EDGE_GAP_M = 10.5, 3.0, 0.5
RUNOFF_JUMP_M, RUNOFF_RETURN_M, RUNOFF_FLAT_M, RUNOFF_FLAT_WITHIN_M = 3.0, 1.5, 0.1, 3.0
SMOOTH_POINTS, PIT_WALL_SMOOTH_POINTS, MIN_EDGE_M, GAP_WIDTH_FRACTION = 5, 9, 1.5, 0.85


# ---- GeoPackage (no GDAL): GP header + WKB polygons ----
def _wkb(b, off):
    order = '<' if b[off] == 1 else '>'
    (kind,) = struct.unpack_from(order + 'I', b, off + 1)
    off += 5
    base = (kind & 0xFFFF) % 1000
    dims = 2 + ((kind & 0xFFFF) // 1000 in (1, 3) or bool(kind & 0x80000000)) + ((kind & 0xFFFF) // 1000 in (2, 3) or bool(kind & 0x40000000))
    if base == 3:
        (rings,) = struct.unpack_from(order + 'I', b, off); off += 4
        out = []
        for _ in range(rings):
            (n,) = struct.unpack_from(order + 'I', b, off); off += 4
            ring = []
            for _ in range(n):
                c = struct.unpack_from(order + 'd' * dims, b, off); off += 8 * dims
                ring.append((c[0], c[1]))
            out.append(ring)
        return [out], off
    if base == 6:
        (parts,) = struct.unpack_from(order + 'I', b, off); off += 4
        polygons = []
        for _ in range(parts):
            p, off = _wkb(b, off)
            polygons += p
        return polygons, off
    raise ValueError(f'unsupported WKB type {kind}')


def _gpkg_polygons(blob):
    assert blob[:2] == b'GP'
    envelope = {0: 0, 1: 32, 2: 48, 3: 48, 4: 64}[(blob[3] >> 1) & 7]
    return _wkb(blob, 8 + envelope)[0]


def extract(gpkg):
    con = sqlite3.connect(f'file:{gpkg}?mode=ro', uri=True)
    polygons = []
    for oid, shape, ty, level, start in con.execute(
            "select OBJECTID, Shape, ATTR_SP_TY, ATTR_SP_LV, DATA_INI from ATTR_SP where ATTR_SP_TY='08' order by OBJECTID"):
        if oid in LAP_POLYGONS:
            polygons.append({'objectid': oid, 'attr_sp_ty': ty, 'attr_sp_lv': level, 'data_ini': start,
                             'rings': [[[round(x, 3), round(y, 3)] for x, y in ring] for p in _gpkg_polygons(shape) for ring in p]})
    assert [p['objectid'] for p in polygons] == list(LAP_POLYGONS)
    record = {
        'source': 'Regione Lombardia, Database Geo-Topografico (DBGT), comune di Monza (ISTAT 108033)',
        'download': 'https://www.geoportale.regione.lombardia.it/rlregis_download/service/package?dbId=514&cod=108033',
        'metadata': 'geoportale.regione.lombardia.it metadata sheet r_lombar:0092b7ba-c697-433b-b5f2-9f9d9b8694e3',
        'licence': 'CC BY 4.0',
        'attribution': 'Contains data from Regione Lombardia, Database Geo-Topografico (DBGT), licensed under CC BY 4.0.',
        'retrieved_utc': '2026-11-12',
        'zip_sha256': '71a7fa1149f00c436ca5cb9d4ae0824d1a3f8f9937a0225ad9ca4e7d4a146171',
        'gpkg_sha256': hashlib.sha256(Path(gpkg).read_bytes()).hexdigest(),
        'layer': "ATTR_SP (attrezzatura sportiva, class 020204), ATTR_SP_TY = '08' (pista); code list: Allegato 1.3 v2.0, pp. 162-163",
        'crs': 'EPSG:7791 (RDN2008 / UTM zone 32N), metres; survey scale 1:2000, surveyed 2015',
        'polygons': polygons,
    }
    DATA.write_text(json.dumps(record, separators=(',', ':')) + '\n')


# ---- UTM zone 32N (GRS80) to longitude/latitude, Krueger series ----
def utm32_to_lonlat(e, n):
    a, f = 6378137.0, 1 / 298.257222101
    m = f / (2 - f)
    big_a = a / (1 + m) * (1 + m * m / 4 + m ** 4 / 64)
    beta = (m / 2 - 2 * m * m / 3 + 37 * m ** 3 / 96, m * m / 48 + m ** 3 / 15, 17 * m ** 3 / 480)
    delta = (2 * m - 2 * m * m / 3 - 2 * m ** 3, 7 * m * m / 3 - 8 * m ** 3 / 5, 56 * m ** 3 / 15)
    xi, eta = n / (0.9996 * big_a), (e - 500000.0) / (0.9996 * big_a)
    xp = xi - sum(b * math.sin(2 * j * xi) * math.cosh(2 * j * eta) for j, b in enumerate(beta, 1))
    ep = eta - sum(b * math.cos(2 * j * xi) * math.sinh(2 * j * eta) for j, b in enumerate(beta, 1))
    chi = math.asin(math.sin(xp) / math.cosh(ep))
    lat = chi + sum(d * math.sin(2 * j * chi) for j, d in enumerate(delta, 1))
    return math.degrees(math.radians(9.0) + math.atan2(math.sinh(ep), math.cos(xp))), math.degrees(lat)


def _interpolate(values, missing):
    n = len(values)
    known = [i for i in range(n) if not missing[i]]
    out = list(values)
    for i in range(n):
        if not missing[i]:
            continue
        before = max((k for k in known if k < i), default=None)
        after = min((k for k in known if k > i), default=None)
        if before is None: before = known[-1] - n
        if after is None: after = known[0] + n
        t = (i - before) / (after - before)
        out[i] = values[before % n] * (1 - t) + values[after % n] * t
    return out


def _runoff(edge, missing):
    """Flag run-off sections on one side (see the module notes)."""
    n = len(edge)
    flagged = [False] * n
    i = 0
    while i < n:
        prev, here = (i - 1) % n, i
        if not missing[prev] and not missing[here] and edge[here] - edge[prev] > RUNOFF_JUMP_M:
            base, k = edge[prev], here
            while k < i + n // 4:
                j, nxt = k % n, (k + 1) % n
                flat = not missing[nxt] and 0 <= edge[j] - edge[nxt] < RUNOFF_FLAT_M and edge[j] <= base + RUNOFF_FLAT_WITHIN_M
                if not missing[j] and (edge[j] <= base + RUNOFF_RETURN_M or flat):
                    break
                flagged[j] = True
                k += 1
            i = max(i + 1, k)
        else:
            i += 1
    return flagged


def edge_profile():
    """(left, right, raw) edge distances in metres at every measured-centreline point."""
    import generate_track_assets as T
    record = json.loads(DATA.read_text())
    edges = []
    for polygon in record['polygons']:
        for ring in polygon['rings']:
            pts = [T.geo_metres(*utm32_to_lonlat(x, y)) for x, y in ring]
            edges += list(zip(pts, pts[1:]))

    def ray(o, d):
        best = None
        for (ax, ay), (bx, by) in edges:
            ex, ey = bx - ax, by - ay
            den = d[0] * ey - d[1] * ex
            if abs(den) < 1e-12:
                continue
            t = ((ax - o[0]) * ey - (ay - o[1]) * ex) / den
            u = ((ax - o[0]) * d[1] - (ay - o[1]) * d[0]) / den
            if 0 <= u <= 1 and 0 < t < RAY_MAX_M and (best is None or t < best):
                best = t
        return best

    def inside(p):
        c = False
        for (ax, ay), (bx, by) in edges:
            if (ay > p[1]) != (by > p[1]) and p[0] < ax + (p[1] - ay) * (bx - ax) / (by - ay):
                c = not c
        return c

    first, step, lateral, _ = T.pit_lane_profile()
    lap = T.MEASURED_LAP_M

    def pit_offset(along):
        rel = along / lap
        rel -= round(rel)
        k = (rel - first) / step
        if k < 0 or k > len(lateral) - 1:
            return 0.0
        i = min(int(k), len(lateral) - 2)
        return -(lateral[i] * (1 - (k - i)) + lateral[i + 1] * (k - i)) * T.METRES_PER_UNIT  # metres, + = right

    g, n = T.GEO, len(T.GEO)
    left, right, missing_l, missing_r, raw, walled = [], [], [], [], [], []
    for i in range(n):
        a, b = g[(i - 1) % n], g[(i + 1) % n]
        h = math.atan2(b[1] - a[1], b[0] - a[0])
        o = g[i]
        ok = inside(o)
        l = ray(o, (-math.sin(h), math.cos(h))) if ok else None
        r = ray(o, (math.sin(h), -math.cos(h))) if ok else None
        raw.append((l, r))
        pit = pit_offset(float(T.ROWS[i]['distance_m']))
        if r is not None and pit >= PIT_WALL_FROM_M:
            r = min(r, pit - PIT_WALL_INSET_M - PIT_EDGE_GAP_M)
        walled.append(pit >= PIT_WALL_FROM_M)
        left.append(l or 0.0); right.append(r or 0.0)
        missing_l.append(l is None); missing_r.append(r is None)
    # Widen the gaps where the line leaves the surveyed surface (note 2).
    widths = sorted(left[i] + right[i] for i in range(n) if not (missing_l[i] or missing_r[i]))
    floor = GAP_WIDTH_FRACTION * widths[len(widths) // 2]
    gap = [missing_l[i] or missing_r[i] for i in range(n)]
    grown = list(gap)
    for i in range(n):
        if not gap[i]:
            continue
        for step in (-1, 1):
            k = (i + step) % n
            while not gap[k] and left[k] + right[k] < floor:
                grown[k] = True
                k = (k + step) % n
    missing_l = [a or b for a, b in zip(missing_l, grown)]
    missing_r = [a or b for a, b in zip(missing_r, grown)]
    for edge, missing, wall in ((left, missing_l, [False] * n), (right, missing_r, walled)):
        flagged = _runoff(edge, missing)
        filled = _interpolate(edge, [m or f for m, f in zip(missing, flagged)])
        def mean(i, points):
            half = points // 2
            return sum(filled[(i + k) % n] for k in range(-half, half + 1)) / points
        edge[:] = [max(MIN_EDGE_M, mean(i, PIT_WALL_SMOOTH_POINTS if wall[i] else SMOOTH_POINTS)) for i in range(n)]
    return left, right, raw


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--extract', metavar='DBGT_GPKG')
    args = parser.parse_args()
    if args.extract:
        extract(args.extract)
    else:
        import sys
        sys.path.insert(0, str(Path(__file__).resolve().parent))
        left, right, _ = edge_profile()
        print(f'{len(left)} points; left {min(left):.2f}-{max(left):.2f} m, right {min(right):.2f}-{max(right):.2f} m')
