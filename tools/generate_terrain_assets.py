#!/usr/bin/env python3
"""Real ground heights around the circuit from Regione Lombardia's 5 m terrain model.

Source: "DTM 5x5 - Modello digitale del terreno (ed. 2015)", Regione Lombardia,
CC BY 4.0 (metadata r_lombar:dfc98d60-5f02-4a2b-8113-5ad24cc53a9c). It is a bare-earth
model (no trees or buildings), built from the regional topographic database's
contours, spot heights and break lines, with LiDAR along watercourses.

  --extract CLIP_TIF   samples the clipped DTM (a byte-exact cut of DTM5_RL.zip, see
                       data/monza-terrain.json) on the scene's terrain grid and writes
                       data/monza-terrain.json. The 3.1 GB download is not kept.
  build()              returns (header, source) for the generated terrain table,
                       called by generate_race_assets.py.

The grid is aligned with the scene axes (columns along +x, rows along +z, where
z = -y of the 2D scene units) and has one sample every 16 real metres, from 1.2 km
beyond the track's bounding box on every side. Each sample is the DTM bilinearly
interpolated at that point (scene units -> local frame -> longitude/latitude ->
UTM 32N, the inverse of generate_track_assets.to_scene).

The OpenF1 elevation the road uses has its own vertical origin. Along the lap the
DTM is 5.4 m below it (median; 1.0 m RMS once that offset is removed), so the
ground is shifted up by that offset to meet the road. The blend between road and
ground happens at runtime in include/apex3d/terrain.hpp.
"""
import argparse, hashlib, json, math, statistics, struct, sys
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools'))
import generate_track_assets as T  # noqa: E402

DATA = ROOT / 'data/monza-terrain.json'
CELL_M, MARGIN_M = 16.0, 1200.0
CLIP_SHA256 = '36b6fb3caabd2065ddad588b5f62f286789cb510c203e1043de188b83d0baf30'
SOURCE = {
    'title': 'DTM 5X5 - Modello digitale del terreno (ed. 2015), Regione Lombardia',
    'metadata_identifier': 'r_lombar:dfc98d60-5f02-4a2b-8113-5ad24cc53a9c',
    'metadata_page': 'https://www.geoportale.regione.lombardia.it/metadati?p_p_id=detailSheetMetadata_WAR_gptmetadataportlet'
                     '&p_p_lifecycle=0&p_p_state=normal&p_p_mode=view&_detailSheetMetadata_WAR_gptmetadataportlet_identifier='
                     'r_lombar%3Adfc98d60-5f02-4a2b-8113-5ad24cc53a9c',
    'licence': 'CC BY 4.0',
    'attribution': 'Regione Lombardia - DTM 5x5 m (ed. 2015), CC BY 4.0, via Geoportale della Lombardia',
    'download_url': 'https://www.cartografia.servizirl.it/download/DTM5_RL.zip',
    'download_bytes': 3085490651,
    'download_sha256': 'ae96d3dca7bd728683ad964e6d3bad49792ff36857f4bc532099479513a8e1fc',
    'download_last_modified': 'Thu, 27 Feb 2025 23:27:07 GMT',
    'nature': 'Bare-earth terrain model: ground in urban and rural areas, water level on lakes. Stated vertical '
              'accuracy 0.3 m (1:2000 urban mapping), 1 m (1:5000), 2 m (1:10000 rural and forest).',
    'clip': {'file': 'DTM5_RL_monza_clip_EPSG32632.tif', 'sha256': CLIP_SHA256,
             'method': 'Byte-exact copy of the 64x64 blocks in columns 185-202, rows 350-370 of DTM5_RL.ige (inside '
                       'DTM5_RL.zip), rewritten as an uncompressed float32 GeoTIFF: 1152 x 1344 cells of 5 m, '
                       'EPSG:32632, top-left corner (519201.8694, 5054901.2481). No resampling.'},
}


def _read_tiff(path):
    """Uncompressed little-endian float32 GeoTIFF, one strip per row (the clip's layout)."""
    raw = Path(path).read_bytes()
    assert raw[:4] == b'II*\0', 'expected a classic little-endian TIFF'
    sizes = {3: 2, 4: 4, 12: 8}
    tags, (offset,) = {}, struct.unpack_from('<I', raw, 4)
    (count,) = struct.unpack_from('<H', raw, offset)
    for k in range(count):
        tag, kind, n, value = struct.unpack_from('<HHI4s', raw, offset + 2 + 12 * k)
        if kind == 2:
            continue
        size = sizes[kind] * n
        data = value if size <= 4 else raw[struct.unpack('<I', value)[0]:][:size]
        tags[tag] = struct.unpack('<' + {3: 'H', 4: 'I', 12: 'd'}[kind] * n, data[:size])
    width, height = tags[256][0], tags[257][0]
    assert tags.get(259, (1,))[0] == 1 and tags[258][0] == 32 and tags[339][0] == 3
    assert len(tags[273]) == height, 'expected one strip per row'
    cells = [struct.unpack_from(f'<{width}f', raw, o) for o in tags[273]]
    scale, tie = tags[33550], tags[33922]
    return {'cells': cells, 'width': width, 'height': height, 'x0': tie[3] - tie[0] * scale[0],
            'y0': tie[4] + tie[1] * scale[1], 'dx': scale[0], 'dy': scale[1]}


def lonlat_to_utm32(lon, lat):
    """WGS 84 longitude/latitude to UTM zone 32N easting/northing (metres)."""
    a, f, k0 = 6378137.0, 1 / 298.257223563, 0.9996
    e2 = f * (2 - f)
    ep2 = e2 / (1 - e2)
    phi, lam = math.radians(lat), math.radians(lon - 9.0)
    n = a / math.sqrt(1 - e2 * math.sin(phi) ** 2)
    t, c, big_a = math.tan(phi) ** 2, ep2 * math.cos(phi) ** 2, math.cos(phi) * lam
    m = a * ((1 - e2 / 4 - 3 * e2 ** 2 / 64 - 5 * e2 ** 3 / 256) * phi
             - (3 * e2 / 8 + 3 * e2 ** 2 / 32 + 45 * e2 ** 3 / 1024) * math.sin(2 * phi)
             + (15 * e2 ** 2 / 256 + 45 * e2 ** 3 / 1024) * math.sin(4 * phi)
             - (35 * e2 ** 3 / 3072) * math.sin(6 * phi))
    east = k0 * n * (big_a + (1 - t + c) * big_a ** 3 / 6 + (5 - 18 * t + t * t + 72 * c - 58 * ep2) * big_a ** 5 / 120) + 500000
    north = k0 * (m + n * math.tan(phi) * (big_a ** 2 / 2 + (5 - t + 9 * c + 4 * c * c) * big_a ** 4 / 24
                                           + (61 - 58 * t + t * t + 600 * c - 330 * ep2) * big_a ** 6 / 720))
    return east, north


def scene_to_lonlat(sx, sy):
    """Inverse of generate_track_assets.to_scene (2D scene units to longitude/latitude)."""
    x, y = (sx - T.FINISH_SCENE[0]) * T.METRES_PER_UNIT, (sy - T.FINISH_SCENE[1]) * T.METRES_PER_UNIT
    c, s = math.cos(T._ANGLE), math.sin(T._ANGLE)
    gx, gy = c * x + s * y + T.GEO[0][0], -s * x + c * y + T.GEO[0][1]
    return gx / T.FRAME['m_per_deg_lon'] + T.FRAME['lon0'], gy / T.FRAME['m_per_deg_lat'] + T.FRAME['lat0']


def _bilinear(dtm, east, north):
    col = (east - dtm['x0']) / dtm['dx'] - 0.5
    row = (dtm['y0'] - north) / dtm['dy'] - 0.5
    c, r = math.floor(col), math.floor(row)
    assert 0 <= c < dtm['width'] - 1 and 0 <= r < dtm['height'] - 1, 'grid point outside the clip'
    fc, fr = col - c, row - r
    a, b = dtm['cells'][r][c], dtm['cells'][r][c + 1]
    d, e = dtm['cells'][r + 1][c], dtm['cells'][r + 1][c + 1]
    assert min(a, b, d, e) > -1e30, 'no-data cell inside the grid'
    return (a + (b - a) * fc) * (1 - fr) + (d + (e - d) * fc) * fr


def extract(path):
    assert hashlib.sha256(Path(path).read_bytes()).hexdigest() == CLIP_SHA256, 'unexpected clip file'
    dtm = _read_tiff(path)
    unit = CELL_M / T.METRES_PER_UNIT
    xs, zs = [p[0] for p in T.SCENE], [-p[1] for p in T.SCENE]
    margin = MARGIN_M / T.METRES_PER_UNIT
    min_x = math.floor((min(xs) - margin) / unit) * unit
    min_z = math.floor((min(zs) - margin) / unit) * unit
    columns = math.ceil((max(xs) + margin - min_x) / unit) + 1
    rows = math.ceil((max(zs) + margin - min_z) / unit) + 1
    heights = []
    for row in range(rows):
        line = []
        for column in range(columns):
            lon, lat = scene_to_lonlat(min_x + column * unit, -(min_z + row * unit))
            line.append(round(_bilinear(dtm, *lonlat_to_utm32(lon, lat)) * 100))
        heights.append(line)
    # Offset between the DTM and the OpenF1 elevation along the measured line.
    gaps = [_bilinear(dtm, *lonlat_to_utm32(float(r['lon']), float(r['lat']))) - float(r['elevation_m']) for r in T.ROWS]
    offset = statistics.median(gaps)
    rms = math.sqrt(sum((g - offset) ** 2 for g in gaps) / len(gaps))
    record = {
        'source': SOURCE,
        'grid': {
            'note': 'Scene-aligned grid in 2D scene units (generate_track_assets.to_scene): columns along +x, rows '
                    'along +z = -y. Each value is the DTM height in centimetres above sea level, bilinearly '
                    'interpolated from the 5 m cells at the grid point.',
            'origin_units': [round(min_x, 9), round(min_z, 9)], 'cell_units': unit, 'cell_m': CELL_M,
            'margin_m': MARGIN_M, 'columns': columns, 'rows': rows,
        },
        'track_offset': {
            'note': 'DTM minus OpenF1 elevation at the 1,152 measured-centreline points (5 m apart).',
            'median_m': round(offset, 3), 'rms_after_offset_m': round(rms, 3),
            'finish_line_dtm_m': round(gaps[0] + float(T.ROWS[0]['elevation_m']), 2),
        },
        'heights_cm': heights,
    }
    text = json.dumps({k: v for k, v in record.items() if k != 'heights_cm'}, indent=1)[:-2]
    text += ',\n "heights_cm": [\n' + ',\n'.join('  ' + json.dumps(line, separators=(',', ':')) for line in heights) + '\n ]\n}\n'
    DATA.write_text(text)


def build():
    data = json.loads(DATA.read_text())
    grid, heights = data['grid'], data['heights_cm']
    columns, rows = grid['columns'], grid['rows']
    assert len(heights) == rows and all(len(line) == columns for line in heights)
    zero = float(T.ROWS[0]['elevation_m']) + data['track_offset']['median_m']
    header = '#pragma once\n// Generated by tools/generate_terrain_assets.py from data/monza-terrain.json.\n'
    header += f'// {data["source"]["attribution"]}\n'
    header += '#include <array>\n#include <cstdint>\n\nnamespace apex3d::cp3 {\n'
    header += '// Scene-aligned DTM grid: columns along +x, rows along +z, in 2D scene units.\n'
    header += f'inline constexpr double surveyedTerrainOriginX = {grid["origin_units"][0]!r};\n'
    header += f'inline constexpr double surveyedTerrainOriginZ = {grid["origin_units"][1]!r};\n'
    header += f'inline constexpr double surveyedTerrainCell = {grid["cell_units"]!r};\n'
    header += f'inline constexpr int surveyedTerrainColumns = {columns}, surveyedTerrainRows = {rows};\n'
    header += '// DTM height (m above sea level) that sits level with the finish line: the finish line\'s\n'
    header += f'// OpenF1 elevation plus the median DTM offset along the lap ({data["track_offset"]["median_m"]} m).\n'
    header += f'inline constexpr double surveyedTerrainZeroMetres = {zero:.3f};\n'
    header += '// Row-major heights in centimetres above sea level (src/generated/monza_terrain_2025.cpp).\n'
    header += 'extern const std::array<std::int16_t, surveyedTerrainColumns * surveyedTerrainRows> surveyedTerrainCentimetres;\n'
    header += '}\n'
    source = '// Generated by tools/generate_terrain_assets.py from data/monza-terrain.json.\n'
    source += '#include "../../include/apex3d/generated/monza_terrain_2025.hpp"\n\nnamespace apex3d::cp3 {\n'
    source += 'const std::array<std::int16_t, surveyedTerrainColumns * surveyedTerrainRows> surveyedTerrainCentimetres{{\n'
    source += ''.join(','.join(str(v) for v in line) + ',\n' for line in heights)
    source += '}};\n}\n'
    return header, source


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--extract', metavar='CLIP_TIF')
    args = parser.parse_args()
    if args.extract:
        extract(args.extract)
    else:
        print(build()[0], end='')
