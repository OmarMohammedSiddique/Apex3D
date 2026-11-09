#!/usr/bin/env python3
"""Rebuild checked-in race and CC0 cue headers from inspectable local inputs."""
import argparse
import csv
from pathlib import Path
import struct
import wave

ROOT = Path(__file__).resolve().parents[1]


def race_header():
    with (ROOT / 'data/monza-2025-verstappen.csv').open(newline='') as source:
        rows = list(csv.DictReader(source))
    assert [int(r['lap']) for r in rows] == list(range(1, 54))
    values = [[int(r[k]) for k in ('sector1_ms', 'sector2_ms', 'sector3_ms')] for r in rows]
    assert all(sum(v) == int(r['lap_ms']) for v, r in zip(values, rows))
    assert sum(map(sum, values)) == 4404325
    header = '''#pragma once

#include <array>

namespace apex3d::cp3 {
// FIA 2025 Italian GP Race Sector Analysis, car 1. See docs/monza-race-research.md.
// Generated from data/monza-2025-verstappen.csv; times are integer milliseconds.
struct RaceLap { int s1; int s2; int s3; constexpr int milliseconds() const { return s1+s2+s3; } };
inline constexpr std::array<RaceLap, 53> raceLaps{{
'''
    header += ''.join('    {' + ', '.join(map(str, row)) + '},\n' for row in values)
    header += '''}};
inline constexpr int raceTotalMilliseconds = 4404325;
inline constexpr double raceTotalSeconds = raceTotalMilliseconds / 1000.0;
inline constexpr double circuitMetres = 5793.0;
inline constexpr double gridOffset = 309.0 / circuitMetres;
inline constexpr double pitLaneSeconds = 24.545;
inline constexpr double pitServiceSeconds = 2.3;
inline constexpr double startSequenceSeconds = 6.5;

constexpr auto raceBoundariesMs = [] {
    std::array<int, 54> result{};
    for (std::size_t i = 0; i < raceLaps.size(); ++i) result[i+1] = result[i] + raceLaps[i].milliseconds();
    return result;
}();
static_assert(raceBoundariesMs.back() == raceTotalMilliseconds);
} // namespace apex3d::cp3
'''
    return header


def cue_header():
    header = '#pragma once\n#include <array>\n#include <cstdint>\nnamespace apex3d::audio {\n// Kenney Interface Sounds, CC0. Converted to 24kHz mono PCM. See assets/audio/.\n'
    for name, filename in [('redCue', 'start-light.wav'), ('goCue', 'lights-out.wav')]:
        with wave.open(str(ROOT / 'assets/audio' / filename)) as source:
            assert (source.getnchannels(), source.getsampwidth(), source.getframerate()) == (1, 2, 24000)
            values = struct.unpack('<' + 'h' * source.getnframes(), source.readframes(source.getnframes()))
        header += f'inline constexpr std::array<std::int16_t, {len(values)}> {name}{{{{\n'
        header += '\n'.join('    ' + ', '.join(map(str, values[i:i+24])) + ',' for i in range(0, len(values), 24))
        header += '\n}};\n'
    return header + '}\n'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true', help='verify generated headers without writing')
    options = parser.parse_args()
    for filename, content in [('monza_2025.hpp', race_header()), ('start_cues.hpp', cue_header())]:
        output = ROOT / 'include/apex3d/generated' / filename
        if options.check:
            if not output.exists() or output.read_text() != content:
                raise SystemExit(f'{filename} is stale; run python3 tools/generate_race_assets.py')
        else:
            output.write_text(content)
    print('Race data and start-cue headers verified.' if options.check else 'Race asset headers generated.')


if __name__ == '__main__':
    main()
