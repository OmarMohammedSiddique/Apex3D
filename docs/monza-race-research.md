# Monza 2025 race replay: sources and reconstruction

Researched and implemented locally on 8–9 November 2026. The selected replay is
Max Verstappen's winning 2025 Italian Grand Prix, not a fictional race made
by repeating a qualifying lap. The current renderer is a schematic 3D teaching visualization.

## Which record and why

F1 and the FIA described the 7 September 2025 race as the fastest world
championship Grand Prix, with 53 laps completed in **1:13:24.325**, at an
average of 250.706 km/h. This gives one documented driver, race, tyre stop and
finish to reproduce coherently.

The separate records must not be mixed:

| Measure | Driver / session | Time |
| --- | --- | --- |
| Selected full race | Max Verstappen, 2025 Italian GP | 1:13:24.325 |
| Fastest lap of that race | Lando Norris, lap 53 | 1:20.901 |
| Verstappen's best race lap | Lap 52 | 1:21.003 |
| Verstappen's qualifying pole lap | 2025 Italian GP qualifying | 1:18.792 |

The 2026 F1 event preview still names Norris's 2025 race-lap record. The
2026 Italian GP result was also checked; it does not replace the selected
2025 historical replay. The product identifies its season explicitly rather
than silently changing its dataset when new records are set.

Sources:

- [F1 official 2025 result](https://www.formula1.com/en/results/2025/races/1268/italy/race-result)
- [F1 fastest-race report](https://www.formula1.com/en/latest/article/facts-and-stats-verstappen-wins-fastest-race-in-f1-history-at-monza.7mMpbX22KFk06YjdTKTFLU)
- [FIA race report, including the 2.3-second tyre change](https://www.fia.com/news/f1-verstappen-wins-f1s-fastest-race-monza-ahead-norris-and-piastri)
- [F1 qualifying video and pole record](https://www.youtube.com/watch?v=wgZdpFRpZCs)
- [F1 2026 Italian GP preview](https://www.formula1.com/en/latest/article/need-to-know-the-most-important-facts-stats-and-trivia-ahead-of-the-2026-italian-grand-prix.3R2slU8oSHdeOc5lidJDGy)
- [F1 official 2026 result](https://www.formula1.com/en/results/2026/races/1293/italy/race-result)

## Timing data and exact checks

`data/monza-2025-verstappen.csv` records the 53 sector triplets as integer
milliseconds. `tools/generate_race_assets.py` generates the C++ header, and
`--check` verifies it agrees with the CSV. No network access is needed to run
the app or regenerate its embedded data.

The primary input is car 1 on page 1 of the FIA Race Sector Analysis, checked
against the Race Lap Analysis. The first lap's complete 87.159 seconds comes
from the Race History Chart. Its missing first-sector value is derived as
87.159 - 29.088 - 28.517 = 29.554 seconds. All other lap totals are the sum of
the three published sector values. All 53 sum to exactly 4,404,325 ms.

- [FIA Race Sector Analysis](https://www.fia.com/sites/default/files/2025_16_ita_f1_r0_timing_racesectoranalysis_v01.pdf)
- [FIA Race Lap Analysis](https://www.fia.com/sites/default/files/2025_16_ita_f1_r0_timing_racelapanalysis_v01.pdf)
- [FIA Race History Chart](https://www.fia.com/sites/default/files/2025_16_ita_f1_r0_timing_racehistorychart_v01.pdf)

At 1x the replay clock runs at real-time pace after lights-out, excluding
pauses. Playback rate scales the race clock and motion together; it does not
change any recorded lap duration. At exactly 87.159 seconds the display reads
lap 2 of 53, race 00:01:27.159, last lap 01:27.159. At 4,404.325 seconds it
stops at lap 53 of 53 and never advances to lap 54.

## Geometry and movement: reconstructed

The existing hand-authored Monza/Catmull-Rom centreline is retained. Sector
boundaries are placed schematically from the FIA map, and travel within each
sector is linearly interpolated in distance. There is no GPS trace, continuous
speed telemetry, race traffic, braking model, overtaking recreation, or full
field of opponents. Exact sector times do not imply exact corner-by-corner
motion. The procedural 3D car's blue body is a simplified visual identifier, not a licensed
Red Bull model or livery.

The grid is offset 309 m from the control line, consistent with the circuit's
5,793 m lap and 306.720 km race distance (53 full laps minus 309 m). This
avoids treating the first lap as a full flying lap. The control line and grid
are distinct positions even though the prototype only draws the control line.

The pit-lane schematic follows the inside of the main straight with smooth
entry and exit joins and a marked service box. Circuit reference distances
5,707 m pit-in / 332 m pit-out and 418 m controlled length come from Monza's
2021 FIA WEC briefing; they are geometric references, not a claim to surveyed
2025 F1 gate coordinates. The pit line and garage spacing are exaggerated for
legibility on the current map. The HUD explicitly labels interpolated movement.

- [2025 FIA circuit map and pit-lane drawing](https://www.fia.com/system/files/decision-document/2025_italian_grand_prix_-_event_notes_-_circuit_map_pit_lane_emergency_exit_map_quarantine_zone_and_red_zones.pdf)
- [Monza circuit information](https://www.monzanet.it/en/circuit/)
- [2021 FIA WEC Monza briefing, circuit reference distances](https://fiawec.alkamelsystems.com/Results_NoticeBoard/09_2021/03_Monza/006_Doc%206%20-%20Briefing%20Notes.pdf)

## Wheel rotation

Race wheel speed uses a nominal 720 mm 2025 slick tyre diameter (0.360 m
radius) and converts circuit progress to metres over the 5,793 m circuit.
Rotation follows distance / circumference; RPM follows reconstructed linear
speed / circumference times 60. Rendering size is deliberately exaggerated
and is kept separate from the physical tyre radius. Loaded rolling radius,
slip and left/right wheel differentials are not modelled. These are computed
rolling RPM, not measured telemetry. The Bezier demo retains its model-unit
wheel calculation. At high playback rates the spokes can visually alias.

- [FIA 2025 tyre technical targets, nominal dry sizes](https://legal.fia.com/web/appeloffre.nsf/0F43EC70A0FACADFC1258991006043A9/%24FILE/ITT%20F1%20TYRES%202025-2027%20%28including%20F2%20and%20F3%29_UPDATE%2014.04.2023.pdf?openelement=)

## Pit visit: documented duration, estimated arrival

The FIA pit summary reports one visit for Verstappen, marked **lap 37**, at
15:54:52, lasting **24.545 seconds**. The lap report marks lap 37 as the in-lap
(1:27.432); lap 38 is 1:42.349. The FIA narrative calls the tyre change a
lap-38 event and reports a **2.3-second stationary stop**, Medium to Hard.
These descriptions are consistent with entering before the timing-line
crossing and servicing after it. The 24.545 seconds are the whole pit-lane
visit, not 24.545 seconds standing still.

The reconstruction uses these explicit assumptions:

- Entry is placed at race 3,077.249 s, aligned with the whole-second pit
  report using the classification-derived race start, approximately
  15:03:34.751. The source's whole-second precision limits that alignment.
- The measured control-line crossing remains exactly 3,081.730 s.
- Arrival at the box is estimated as 10 s after entry: 3,087.249 s.
- The car stays still for the documented 2.3 s, until 3,089.549 s.
- Exit is 3,101.794 s, exactly 24.545 s after the modelled entry.
- It reaches the lap-38 sector-1 line at its published timing and resumes
  the remaining sector/lap schedule. Seeking reconstructs this state directly.

Entry-to-box and box-to-exit splits are not independently measured. The
archived live timing endpoint returned 401 and direct PDF downloads returned
403; the readable FIA reports were inspected through the research browser.
No access restrictions were bypassed, and no missing telemetry was invented.

- [FIA Pit Stop Summary](https://www.fia.com/sites/default/files/2025_16_ita_f1_r0_timing_racepitstopsummary_v01.pdf)
- [FIA classification and time-of-day table](https://api.fia.com/events/fia-formula-one-world-championship/season-2025/italian-grand-prix/race-qualification)

## Starting lights and optional sound

The application presents five double red lamps which illuminate one column
per second. All lamps extinguish together after a fixed 6.5-second illustrative
sequence. The race timer starts at lights-out, not on the first red lamp. This
is not a frame-accurate recreation of the 2025 broadcast's start interval.
There is no green start light.

The short red-light tick and lights-out confirmation are Kenney Interface
Sounds samples, CC0. They are application cues, not recordings of F1 gantry
hardware or broadcast audio. `assets/audio/README.md` records provenance,
conversion and the included license. They play only if the user unmutes via
the clickable button or `M`; every fresh launch is muted. No engine tone or
background loop has been restored. Native output is currently macOS-only;
other platforms remain fully usable silently. Screenshot captures never
initialize audio.

- [Kenney original pack and CC0 license declaration](https://kenney.nl/assets/interface-sounds)
- [WAV mirror used](https://github.com/Calinou/kenney-interface-sounds)

## Local verification

- Release build and both CTest suites pass.
- Every lap/sector crossing, timer formatting, full-race finish, replay,
  pause, seeking and 64x playback are checked deterministically.
- A complete half-second sweep checks finite, continuous, forward motion.
- Pit entry/exit joins, stationary service, duration and RPM freeze are checked.
- Embedded data and audio headers match their CSV/WAV inputs.
- Invalid timing, speed and conflicting CLI options are rejected before GLFW.
- Real OpenGL frames cover starting lights, lap 2, the service box and finish.
- A live 64x run completed all 53 laps and visibly stopped at 1:13:24.325
  with zero wheel RPM. Clickable start/mute controls and native event-based
  pointer mapping were checked in the desktop app.

Hosted CI, final lecturer-rubric alignment, group review and formal submission
remain separate from these local checks.

## 3D rendering update

The race timing, sector interpolation, pit assumptions and muted start cues
remain the same. The new renderer places this planar course in a 3D world with
raised road/kerb surfaces, pit garages/wall, grandstands, trees and a start
gantry. The F1-style car has solid body sections, front/rear wings, suspension,
a cockpit/halo and rotating cylindrical tyres. Procedural crew figures appear
only during the stationary service interval. These are illustrative assets,
not surveyed circuit architecture or a licensed detailed vehicle model.

Three perspective cameras, depth testing, directional lighting and a filtered
directional shadow map are implemented. A graphics regression suite checks
projection/clipping, surface normals, volume, tyre contact, wheel motion,
camera framing and remapped HUD controls. Real OpenGL views are stored in
`docs/evidence/apex3d-3d-views.png`.
