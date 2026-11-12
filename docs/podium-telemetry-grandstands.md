# Podium cars, recorded telemetry and grandstands

This local extension uses the same **7 September 2025 Italian GP** as the original
replay. It does not silently switch seasons or extrapolate the other cars from
Verstappen's finishing gap.

## Three independently timed cars

| Driver | Number | Official finish | Gap | Pit in-lap | Lane time | Stationary time |
| --- | --- | --- | --- | --- | --- | --- |
| Max Verstappen | 1 | 1:13:24.325 | — | 37 | 24.545 s | 2.3 s |
| Lando Norris | 4 | 1:13:43.532 | +19.207 s | 46 | 27.498 s | 5.9 s |
| Oscar Piastri | 81 | 1:13:45.676 | +21.351 s | 45 | 23.602 s | 1.9 s |

Sources: [F1 official result](https://www.formula1.com/en/results/2025/races/1268/italy/race-result),
[FIA classification](https://www.fia.com/sites/default/files/2025_16_ita_f1_r0_timing_raceprovisionalclassification_v01.pdf),
[FIA sector analysis](https://www.fia.com/sites/default/files/2025_16_ita_f1_r0_timing_racesectoranalysis_v01.pdf),
and [OpenF1 pit archive, session 9912](https://api.openf1.org/v1/pit?session_key=9912).

The Norris and Piastri CSV files contain 53 independent sector triplets each.
All complete triplets from laps 2–53 were cross-checked against their respective
pages of the FIA sector report. Their first-lap sector 1 is derived from the
classification total minus all other known sectors, giving first laps of
87.430 s and 87.992 s. The sums exactly reproduce their official finishes.
The start-grid offsets represent Norris in P2 and Piastri in P4 (Leclerc's P3
car is outside the scope of this three-car view); grid spacing is schematic.

Both McLarens have orange bodywork and different helmet colors. Verstappen is the initial camera focus; clicking a podium row switches the
camera, dashboard and telemetry to that driver. Positions within each sector use the driver's own
archived speed integral, normalized to the simplified route between exact
sector crossings. This preserves acceleration and braking shape and every
recorded sector/lap crossing, but it is **not an exact racing line or GPS replay**.
The modeled ±2.25 m lateral lanes prevent visual overlap; they are not measured.
Pit entry/exit follow archived lane timings, with stationary intervals bracketed
from each car's zero-speed samples. Service-box positions are estimated from
integrated speed after crossing the timing line: roughly 75 m for Red Bull and
34 m for McLaren. Their modeled garage blocks are placed beside those boxes.
These estimates are not surveyed pit-box coordinates.

Replay still stops at Verstappen's exact 53-lap finish. The McLarens are shown
where their timelines put them at that instant. The driver list is now a live
timing tower during the race and the final classification afterwards; see
[Full field](full-field.md#live-timing-tower).
Norris can legitimately be ahead of Verstappen early in the race.

## Speed, gear and engine RPM: recorded channels

[OpenF1 documentation](https://openf1.org/docs/) defines `speed` in km/h,
`n_gear` as the selected gear (0 means neutral/no engaged gear), and `rpm` as
**engine** revolutions per minute. These are different from the wheel inspector's
computed rolling RPM. The archive provides roughly 3.7 samples per second.

The checked-in telemetry CSV files preserve 16,394 Verstappen samples,
16,464 Norris samples and 16,471 Piastri samples, without synthesizing gears
or RPM. The dashboard displays the selected driver's **last recorded observation** at
the current race time. It holds the observation while paused, does not multiply
values at faster playback, displays placeholders during the start-light sequence,
and shows a gap rather than reusing samples more than one second old.

The replay's UTC origin is **2025-09-07T13:03:35.207Z**, the median of the
OpenF1 lap-start timestamps for Verstappen's laps 2–53 minus their exact FIA
cumulative boundaries. Those source dates are documented as approximate.
The inferred origins span 0.371 s (about ±0.19 s around the median); **sub-second
synchronization is approximate**, even though the FIA lap totals are exact.
The first-lap `SESSION STARTED` timestamp was not treated as the exact lights-out
instant, as its elapsed duration does not reproduce the official first lap.

`data/monza-2025-provenance.json` records the exact endpoint URLs, raw-response
SHA-256 hashes, UTC alignment and source pit timing. The game runs fully offline.
`python3 tools/generate_race_assets.py --check` validates every embedded table
against its inspectable CSV input. Regeneration does not require network access.

## Grandstands: official 2025 map locations

The positions and sides are interpreted from the circuit operator's
[official 2025 grandstand map](https://www.monzanet.it/wp-content/uploads/2025/03/GP_F1_2025_LISTINO_PREZZI_02_25.pdf).
The scene now has **37 named/numbered stand sections**, including spectators:

- Main straight: 1/Centrale, 2, 3, 4/Laterale Sinistra, 5/Piscina,
  24/Vedano, 26A–C and 27–30.
- First chicane: 6A–C/Alta Velocità on the inside, 7/Biassono and
  8A–B/Prima Variante Esterna on the outside.
- Roggia/Lesmo/Serraglio: 9, 10, 10 BIS and 11.
- Ascari: 12, 15, 16, 17A–B on the outside and 18 at the exit.
- Parabolica: 21A–E, 22 and 23A–B, on the map's respective outer/inner sides.

`data/monza-2025-grandstands.csv` records each ID, name, nearest route anchor,
side, setback and modeled size. The exact geographic corners and architecture
cannot be recovered from this schematic map: **the corner areas and sides are
source-backed; setbacks, footprints, row counts and roof shapes are estimates**.
There is no claim of surveyed placement. General-admission grass banks and the
continuous inner-Parabolica terrace are not separate numbered grandstands in
this model. Tests check road clearance of the modeled stands. Trees are kept
away from the seating areas and road.

## Overview pointer and zoom

In overview, a fixed-size screen marker identifies only the selected driver.
Podium rows select the camera target and matching recorded telemetry; the other
two cars stay visible without name tags. Marker size remains readable at
whole-track scale without enlarging the physical car. See
[Driver focus and graphic telemetry](driver-focus.md).

Overview zoom now reaches 0.008 of its initial distance, over 50 times closer
than the former minimum of 0.42. Zoom transitions towards the selected driver so the
selected car stays in view. Chase also zooms further. `C` cycles cameras,
scrolling zooms, and `F` resets. A deterministic capture can use:

```bash
./build/Apex3D --camera overview --zoom 0.025 --screenshot-time 5 podium.ppm
```

Local verification covers all three cars' 53 crossings, exact result totals,
source-sample readout, pit intervals, launch acceleration, camera focusing,
marker projection, physical scale, normals and stand clearance. Native OpenGL
captures and an interactive desktop preview complement the automated checks.
