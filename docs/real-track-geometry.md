# Measured Monza track geometry

The track used to be 62 points drawn by hand from the Formula 1 circuit map. It
had the right corners in the right order, but the shape was distorted. Even the
best rotate-and-scale fit left its corners about 130 m from the real ones on
average, and up to 270 m. The track is now measured from where the cars actually
drove during the 2025 race.

![The whole measured lap from above](evidence/apex3d-measured-track.png)

![Verstappen lap 6 speed heatmap on the measured track](evidence/apex3d-measured-track-close.png)

## Where the shape comes from

OpenF1's `/v1/location` endpoint records each car's x, y and z position
about 3.7 times a second, in decimetres, for the same race (session 9912) the
timing and telemetry already come from. `tools/build_track_centreline.py`:

1. Loads positions and lap start times for five cars that ran the whole race
   without incident: Verstappen, Norris, Piastri, Leclerc and Russell.
2. Keeps only clean racing laps: not lap 1 (standing start), no pit-in or
   pit-out laps, and only laps within 3% of that driver's median. That leaves
   **249 laps** and about 75,000 position samples.
3. Takes one clean Verstappen lap (lap 10) as a first guess.
4. **Iterative averaging.** Each sample is projected onto the current guess.
   That gives its distance along the lap and how far it sits to the left or
   right. Offsets are averaged in 5 m bins and smoothed with a Gaussian. The
   guess is moved sideways by that average and resampled every 5 m. Four passes
   settle the line.
5. **Finish line.** Distance 0 is the average place cars were when OpenF1 says
   a new lap started (1,000+ crossings).
6. **Sector lines.** The same trick at "lap start + sector 1 time" and "lap
   start + sector 1 + 2 times" gives the real sector boundaries: **1,894.5 m**
   and **3,704.0 m** along the lap (median of 255 laps). They replace the old
   positions, which were estimated by eye from the FIA map.
7. **Georeferencing.** The line is fitted to OpenStreetMap's mapped Grand Prix
   raceway (the same OSM download the scenery uses), allowing only rotation,
   one uniform scale and a shift. Iterative closest point finds the best fit.

Output: `data/monza-2025-centreline.csv` (distance, OpenF1 x/y, elevation,
longitude, latitude every 5 m) and `data/monza-2025-centreline-provenance.json`
(method, laps used, every download URL and SHA-256 hash, fit quality).
Each laps download is byte-identical to the one already recorded in
`data/monza-2025-full-field-provenance.json`.

## How well it matches

| Check | Result |
| --- | --- |
| Fit scale to OSM (1.0 = same size) | 0.9992 |
| Distance from the OSM-mapped centreline | mean 2.2 m, 95% within 4.4 m, max 8.4 m |
| Measured lap length | 5,758.4 m (official centreline length 5,793 m) |
| Elevation range in the data | 12.4 m |

Two independent sources agreeing within a few metres on a 12 m wide road is
strong evidence that the shape is right.

**Driven line vs painted centreline.** The cars' line is 35 m (0.6%) shorter
than the official length because it cuts each chicane. The scene keeps the
official 5,793 m so existing lap distances, pit timing and wheel rotation are
unchanged. The renderer therefore stretches the measured shape, and everything
placed with it, uniformly by 0.6%. That is about 7 m across the 1.2 km width
of the circuit.

**OpenF1 positions are already smoothed.** Individual samples sit within
0.04 m (median) of the averaged line, which is far tighter than raw GPS. The
timing feed appears to snap cars onto one reference line. So this is the
timing system's track line, not each driver's own racing line.

## How it gets into the scene

`tools/generate_track_assets.py` (run by `tools/generate_race_assets.py`, and
checked in CI) turns the CSV into `include/apex3d/generated/monza_track_2025.hpp`:

- **576 control points, one every 10 m**, replace the 62 hand-drawn anchors.
  The CP2 Catmull-Rom spline and the CP3 Bézier chain both use them, so those
  teaching algorithms are unchanged. Only their input is now measured.
- The scene keeps its 2D scene units. The finish line stays at (1.16, -0.87)
  and the main straight still runs along -x, so cameras, the HUD map and the
  pit lane code work as before. One scene unit is 675 m of ground before the
  0.6% stretch.
- **Buildings, woods and gravel** now use the same `to_scene` transform from
  their original OSM longitude/latitude. Before, a thin-plate spline bent them
  to fit the distorted drawing. They now sit exactly where the map puts them
  relative to the road.
- **Grandstands and turn markers** (DRS boards at Turns 1, 7 and 8) were
  estimated against the old drawing. They were carried to the same place on
  the measured track using the 14 recorded corner landmarks as a
  distance-along-the-lap mapping (`legacy_to_fraction`). The grandstand CSV
  was migrated once with `python3 tools/generate_track_assets.py --migrate-stands`.
- **Elevation** is generated (`measuredMonzaElevation`) but not drawn yet.
  The track is still flat in 3D.

## What is still estimated

- Track width is still a uniform 12 m. Kerbs and Armco follow the line at
  fixed offsets.
- The pit lane is still a reconstructed offset from the main straight. Its
  entry (86 m before the line) and exit (332 m after) are unchanged estimates.
- Between timing gates, cars are still placed by recorded speed rather than by
  their own recorded positions.
- Grandstand positions are carried estimates, not surveyed.

## Rebuild and verify

```bash
python3 tools/build_track_centreline.py            # downloads missing OpenF1 files into tmp/openf1/
python3 tools/build_track_centreline.py --offline  # cached files only
python3 tools/generate_race_assets.py              # regenerate headers
python3 tools/generate_race_assets.py --check
ctest --test-dir build --output-on-failure
./build/Apex3D --camera overview --zoom 1.6 --screenshot-time 500 whole.ppm
```

The OSM georeference needs the scenery download in `tmp/monza-world/` (its hash
must match `data/monza-scenery.json`). Generating headers needs only the
checked-in CSV and JSON files.

## Sources

- [OpenF1 API: location, laps](https://openf1.org/docs/) (unofficial, non-commercial use)
- [OpenStreetMap contributors](https://www.openstreetmap.org/copyright), ODbL 1.0
- [Autodromo Nazionale Monza: circuit length](https://www.monzanet.it/en/circuit/)
