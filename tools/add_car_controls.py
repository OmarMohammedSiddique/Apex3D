#!/usr/bin/env python3
"""Add recorded throttle and brake columns to the full-field telemetry CSVs.

The existing CSVs were cut from OpenF1 `car_data` responses whose URLs and
SHA-256 hashes are kept in data/monza-2025-full-field-provenance.json. This tool:

1. downloads each driver's response (or reuses a cached copy in tmp/openf1/),
2. refuses to continue unless its SHA-256 matches the recorded hash,
3. finds the contiguous run of raw samples that the CSV already contains and
   checks race_ms, speed, gear and RPM are identical for every row,
4. rewrites the CSV with two extra columns: throttle_pct (0-100) and
   brake (0 or 100 in OpenF1: an on/off signal, not brake pressure).

OpenF1 occasionally reports 104 in throttle and/or brake on otherwise normal
full-throttle samples. These are glitch codes, kept unchanged here so the CSV
matches the source; the app treats 104 as "no valid reading".

Existing values never change, so all earlier timing and alignment stays valid.
Run with --check to verify the CSVs without downloading or writing anything.
Uses only the Python standard library.
"""
from __future__ import annotations

import csv
import datetime as dt
import hashlib
import json
import sys
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CACHE = ROOT / 'tmp' / 'openf1'
COLUMNS = ['race_ms', 'speed_kmh', 'gear', 'engine_rpm', 'throttle_pct', 'brake']


def load_entries():
    provenance = json.loads((ROOT / 'data/monza-2025-full-field-provenance.json').read_text())
    entries = json.loads((ROOT / 'data/monza-2025-full-field.json').read_text())
    origin = dt.datetime.fromisoformat(provenance['race_origin_utc'])
    return provenance['sources'], entries, origin


def fetch(url: str, path: Path) -> bytes:
    if path.exists():
        return path.read_bytes()
    request = urllib.request.Request(url, headers={'User-Agent': 'Apex3D-coursework-data-fetch'})
    with urllib.request.urlopen(request, timeout=180) as response:
        body = response.read()
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(body)
    time.sleep(1.0)  # be polite to the public API
    return body


def csv_path(slug: str) -> Path:
    return ROOT / f'data/monza-2025-{slug}-telemetry.csv'


def check() -> None:
    _, entries, _ = load_entries()
    for entry in entries:
        with csv_path(entry['slug']).open() as handle:
            reader = csv.DictReader(handle)
            assert reader.fieldnames == COLUMNS, f"{entry['slug']}: columns {reader.fieldnames}"
            rows = list(reader)
        assert len(rows) == entry['telemetry_rows'], entry['slug']
        for row in rows:
            throttle, brake = int(row['throttle_pct']), int(row['brake'])
            assert 0 <= throttle <= 100 or throttle == 104, (entry['slug'], row)
            assert brake in (0, 100, 104), (entry['slug'], row)
    print('Throttle and brake columns verified for', len(entries), 'entries.')


def build() -> None:
    sources, entries, origin = load_entries()
    for entry in entries:
        path = csv_path(entry['slug'])
        with path.open() as handle:
            reader = csv.DictReader(handle)
            existing = list(reader)
        if not existing:  # DNS entry: header only, no recorded race
            path.write_text(','.join(COLUMNS) + '\n')
            continue
        source = sources[f"car-{entry['number']}"]
        body = fetch(source['url'], CACHE / f"car-{entry['number']}.json")
        digest = hashlib.sha256(body).hexdigest()
        if digest != source['sha256']:
            sys.exit(f"{entry['slug']}: OpenF1 response hash {digest} does not match recorded {source['sha256']}")
        raw = json.loads(body)

        def race_ms(sample):
            return round((dt.datetime.fromisoformat(sample['date']) - origin).total_seconds() * 1000)

        first = int(existing[0]['race_ms'])
        start = next(i for i, sample in enumerate(raw) if race_ms(sample) == first)
        window = raw[start:start + len(existing)]
        assert len(window) == len(existing), entry['slug']
        with path.open('w', newline='') as handle:
            writer = csv.writer(handle, lineterminator='\n')
            writer.writerow(COLUMNS)
            for sample, row in zip(window, existing):
                same = (race_ms(sample), sample['speed'], sample['n_gear'], sample['rpm']) == \
                    (int(row['race_ms']), int(row['speed_kmh']), int(row['gear']), int(row['engine_rpm']))
                if not same:
                    sys.exit(f"{entry['slug']}: raw sample {sample} differs from CSV row {row}")
                writer.writerow([row['race_ms'], row['speed_kmh'], row['gear'], row['engine_rpm'],
                                 sample['throttle'], sample['brake']])
        print(f"{entry['slug']}: {len(existing)} rows, hash verified")


if __name__ == '__main__':
    check() if '--check' in sys.argv else build()
