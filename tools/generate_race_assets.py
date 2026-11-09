#!/usr/bin/env python3
"""Rebuild checked-in race and CC0 cue headers from inspectable local inputs."""
import argparse
import csv
import json
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


def field_header():
    text = '#pragma once\n#include "monza_2025.hpp"\nnamespace apex3d::cp3 {\n'
    for name,total in [('norris',4423532),('piastri',4425676)]:
        rows=list(csv.DictReader((ROOT/f'data/monza-2025-{name}.csv').open()))
        assert [int(r['lap']) for r in rows]==list(range(1,54))
        values=[[int(r[k]) for k in ('sector1_ms','sector2_ms','sector3_ms')] for r in rows]
        assert sum(map(sum,values))==total
        assert all(sum(v)==int(r['lap_ms']) for v,r in zip(values,rows))
        text+=f'inline constexpr std::array<RaceLap,53> {name}Laps{{{{\n'
        text+=''.join('    {'+', '.join(map(str,r))+'},\n' for r in values)+'}};\n'
    return text+'}\n'


def telemetry_header():
    text='#pragma once\n#include <array>\nnamespace apex3d::cp3 {\n'
    text+='// Archived OpenF1 session 9912; raw samples, no synthesized gears/RPM.\n'
    text+='struct TelemetryPoint {int raceMs,speedKmh,gear,engineRpm;};\n'
    for name,finish in [('verstappen',4404325),('norris',4423532),('piastri',4425676)]:
        rows=list(csv.DictReader((ROOT/f'data/monza-2025-{name}-telemetry.csv').open()))
        data=[[int(r[k]) for k in ('race_ms','speed_kmh','gear','engine_rpm')] for r in rows]
        assert all(a[0]<b[0] for a,b in zip(data,data[1:]))
        assert all(0<=r[1]<=400 and 0<=r[2]<=8 and 0<=r[3]<=16000 for r in data)
        assert data[0][0]<0 and data[-1][0]>finish
        text+=f'inline constexpr std::array<TelemetryPoint,{len(data)}> {name}Telemetry{{{{\n'
        text+=''.join('    {'+', '.join(map(str,r))+'},\n' for r in data)+'}};\n'
    return text+'}\n'


def stands_header():
    rows=list(csv.DictReader((ROOT/'data/monza-2025-grandstands.csv').open()))
    assert len({r['id'] for r in rows})==len(rows)==37
    text='#pragma once\n#include <array>\n#include <string_view>\nnamespace apex3d::three {\n'
    text+='struct StandSpec {std::string_view id,name;double x,y,side,setback,length;int rows;bool covered;};\n'
    text+=f'inline constexpr std::array<StandSpec,{len(rows)}> monzaStands{{{{\n'
    for r in rows:
        vals=[r[k] for k in ('anchor_x','anchor_y','side','setback_m','length_m','rows')]
        text+='    {"'+r['id']+'", "'+r['name']+'", '+', '.join(vals)+', '+('true' if r['covered']=='1' else 'false')+'},\n'
    return text+'}};\n}\n'


def tyre_header():
    rows=list(csv.DictReader((ROOT/'data/monza-2025-tyre-stints.csv').open()))
    assert len(rows)==6
    assert {(int(r['driver_number']),int(r['stint_number'])) for r in rows}=={(d,s) for d in (1,4,81) for s in (1,2)}
    text='#pragma once\n#include <array>\n#include <string_view>\nnamespace apex3d::cp3 {\n'
    text+='struct TyreStint {int driver,number,firstLap,lastLap;std::string_view compound;int priorLaps;};\n'
    text+='inline constexpr std::array<TyreStint,6> tyreStints{{\n'
    for r in rows:
        assert r['compound'] in ('SOFT','MEDIUM','HARD','INTERMEDIATE','WET')
        nums=[int(r[k])for k in ('driver_number','stint_number','lap_start','lap_end')]
        assert nums[0] in (1,4,81) and 1<=nums[2]<=nums[3]<=53 and int(r['tyre_age_at_start'])>=0
        text+='    {'+', '.join(map(str,nums))+', "'+r['compound']+'", '+r['tyre_age_at_start']+'},\n'
    return text+'}};\n}\n'


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
    for filename, content in [('monza_2025.hpp', race_header()), ('start_cues.hpp', cue_header()), ('monza_field_2025.hpp', field_header()), ('monza_telemetry_2025.hpp', telemetry_header()), ('monza_stands_2025.hpp', stands_header()), ('monza_tyres_2025.hpp', tyre_header())]:
        output = ROOT / 'include/apex3d/generated' / filename
        if options.check:
            if not output.exists() or output.read_text() != content:
                raise SystemExit(f'{filename} is stale; run python3 tools/generate_race_assets.py')
        else:
            output.write_text(content)
    print('Race data and start-cue headers verified.' if options.check else 'Race asset headers generated.')


if __name__ == '__main__':
    main()
