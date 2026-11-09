# Track heatmap and timeline

This delivers two promises from the original proposal: **colouring the track surface
with telemetry** and **scrubbing through the replay with a timeline**. The
throttle and brake layers are not built yet (see "Still to do").

![Verstappen lap 6 speed heatmap, overview camera](evidence/apex3d-track-heatmap.png)

## What you see

- The 12 m road is painted by the selected driver's recorded **speed** on the lap
  they are currently driving. Red means slow or braking, and the colour moves through
  amber and yellow to green for the fastest sections.
- The key in the top-left corner shows the colour scale: a fixed **70–350 km/h**, so
  laps and drivers can be compared by colour alone.
- **V** turns the heatmap off or on. The track repaints when you select another
  driver or when the driver starts a new lap.
- The bar under the scene has a **play/pause button**, the **lap time**, a **race
  timeline** and a **speed slider**. Drag the timeline to scrub through the race.
  Small ticks mark each of the selected driver's laps (taller every 10 laps) and an
  amber triangle marks their pit stop. The speed slider has nine notches, doubling
  each time from 0.25x to 64x (the same range as `+`/`-`).

## How it works (graphics pipeline)

Code: `include/apex3d/telemetry_heatmap.hpp`, `include/apex3d/replay_controls.hpp`.

1. **Choose the data.** Take the driver's recorded telemetry samples that fall
   inside one lap's official timing window. Lap 1 starts at lights-out.
2. **Place each sample on the track.** For each sample time, ask the replay's
   `FieldMotion` where the driver was along the centreline. This is the same
   timing-gate reconstruction that moves the car. Samples taken in the pit lane are
   skipped because the pit lane is a different road.
3. **Fill the gaps.** Samples are about 20 m apart at racing speed. Every centreline
   point between two samples gets a value by **linear interpolation along the track
   distance**, wrapping across the start/finish line.
4. **Map values to colours.** The value becomes a 0–1 fraction of the 70–350 km/h
   scale. A **piecewise-linear colour ramp** with four stops (red → amber → yellow →
   green) turns that fraction into RGB. A quarter of the asphalt colour is mixed back in
   so the surface still reads as tarmac.
5. **Build the mesh.** One thin strip per centreline segment sits 5 mm above the
   asphalt. Each end of the strip gets **its own vertex colour**, and OpenGL
   interpolates colour across each triangle (Gouraud-style). That gives smooth
   gradients instead of blocks. The legend bar is drawn the same way.
6. **Draw it.** The overlay uses the normal lit shader, so sun shading and car shadows
   fall on it. A small **polygon offset** pulls it towards the camera so the asphalt
   underneath never shows through (no z-fighting). It is rebuilt only when the
   driver, lap or layer changes, not every frame.

The timeline maps race time linearly onto the bar. Clicking or dragging converts the
mouse position back to a race time and seeks there. Drawing and click detection use
the same layout function, so the clickable area matches what is drawn.

**Smooth scrubbing.** The replay caches car spacing in 50 ms steps, and filling that
cache far ahead can take a few seconds. The app now fills it a little each frame in
the background. The lighter part of the timeline shows how much is ready. Jumps inside
it are instant, and the results are identical because frames are computed in the same
order as a normal seek.

## Accuracy limits

- Speed values are the **archived OpenF1 car-data readings**, not estimates.
- **Where** each reading lands on the track follows the replay reconstruction: lap and
  sector crossings are exact, and positions between them are shaped by recorded speed,
  not GPS. So a red patch is at the right corner, but its exact start metre is
  approximate.
- Between samples the colour is interpolated, so very short braking spikes can be
  smoothed.
- Lap 1 includes the standing start, so the grid area shows red.
- Hülkenberg (DNS) has no recorded lap, so his heatmap is empty.

## Still to do

- **Throttle and brake layers.** The project's telemetry CSVs hold only
  `race_ms,speed_kmh,gear,engine_rpm`. Throttle and brake must first be added from
  OpenF1 `car_data` (session 9912) through `tools/generate_race_assets.py`, with
  provenance and hashes like the other inputs. The proposal's legend also lists a live
  **throttle %**, which depends on the same data.

## Verify

```bash
ctest --test-dir build -R heatmap --output-on-failure
./build/Apex3D --camera overview --zoom 0.6 --screenshot-time 500 heatmap.ppm
./build/Apex3D --heatmap off --screenshot-time 500 no_heatmap.ppm
```
