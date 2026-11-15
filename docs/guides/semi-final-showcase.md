# Semi-final technical showcase

Guide for recording the ICS 3204 semi-final technical showcase video. It follows
the five sections the brief asks for, gives each member a speaking part based on
their own commits, and lists the exact key presses for every live demonstration.

Tracking issue: #19.

## Recording checklist

- Every member has their camera on and is visible for the whole recording.
- The shared screen stays visible during every demonstration. Use a layout that
  shows the screen large with the camera tiles beside it.
- Build the app before recording (see [Demo run sheet](#demo-run-sheet)) so no
  one waits on a compile.
- Close notifications. Set the display to 1080p or higher so HUD text is legible.
- Upload to YouTube as **Unlisted**, then check the link in a private window.
- Submit the YouTube link and https://github.com/OmarMohammedSiddique/Apex3D.

## Running order

Target length: about 16 minutes.

| # | Section | Speaker | Time |
| --- | --- | --- | --- |
| 0 | Introduction and the app running | Omar | 0:45 |
| 1 | Proposed project vs actual project | Casper | 1:30 |
| 2 | Group responsibilities (each member, own work) | All six | 6:00 |
| 3 | Project components, live | Omar, Hetvi | 3:00 |
| 4 | Graphics techniques and algorithms, live | Azhar, Nyariki, Shadrack, Omar | 4:00 |
| 5 | Recommendations and lessons learnt | Casper, Omar | 1:00 |

Speakers in sections 3 to 5 are suggestions. Swap freely, as long as every member
speaks in section 2 about their own work.

---

## 0. Introduction (Omar)

Show the app already open in chase view, paused a few laps in.

> Apex3D is an interactive 3D visualisation of the 2025 Italian Grand Prix at Monza.
> It rebuilds the circuit from surveyed geometry, replays all 20 cars from the
> recorded timing and telemetry, and paints that telemetry straight onto the track
> surface. It is written in C++20 with OpenGL 3.3, and it uses the course's
> algorithms throughout, from Bresenham lines in Checkpoint 1 to shadow mapping in
> the 3D app.

---

## 1. Proposed project vs actual project (Casper)

Slide: the proposal's scope next to what was built.

**What we proposed.** An interactive 3D F1 track and telemetry visualiser: a 3D
track ribbon built from polygons, a simple wedge or pyramid car marker following
the racing line, telemetry colour mapped onto the track, orbit/pan/zoom cameras,
play/pause, a speed slider and a scrubbable timeline. Out of scope: detailed car
models, physics, realistic environments and live telemetry.

| Area | Proposed | Actual | Status |
| --- | --- | --- | --- |
| Track | Spline centreline extruded into a polygon ribbon | Centreline from recorded OpenF1 positions; road edges from Regione Lombardia's survey (median 10 m wide); kerbs, barriers, pit lane | Kept, made real |
| Track smoothing | Bézier splines | Catmull-Rom spline through the measured points (passes through every point); Bézier kept as a separate teaching route on `B` | Updated |
| Car | Wedge or pyramid marker | Volume-modelled F1 car with halo, wings, suspension, rotating 720 mm tyres, team logos and numbers | Expanded |
| Cars on track | One marker, one lap | All 20 entries over the full 53-lap race, from FIA sector times and recorded speed | Expanded |
| Telemetry overlay | Colour interpolation, green throttle / red braking | Track heatmap with speed, throttle, brake and pedals layers (`V`), with a colour key | Kept |
| Cameras | Orbit, pan, zoom | Overview orbit and zoom, chase, cockpit and 29 trackside TV towers; no free pan | Updated |
| Playback | Play/pause, speed slider, lap time, timeline | All of these, plus seek by lap, pit-stop jump and 0.25x to 64x | Kept |
| Data legend | Speed, gear, throttle | Speed dial, gear, RPM bar, live timing tower, weather, flags, tyre inspector | Expanded |
| Depth | Z-buffer algorithm | GPU depth buffer with depth testing, plus a light-space depth map for shadows | Kept, on the GPU |
| Data | Static offline CSV | Offline, hash-checked OpenF1 and FIA data, generated into C++ headers so the app runs without a network | Kept |
| Environment | Out of scope | Sky and clouds, surveyed terrain, sun position for the race time, trees, buildings, grandstands, shadows | Added |
| Lap comparison | Implied ("where a driver lost time") | Not implemented | Removed for now |
| Rain effects | Out of scope | Not added; the race was dry | Still out |
| Live streaming | Out of scope | Still out | Still out |

**Why it changed.**
- Checkpoints 1 and 2 asked for 2D algorithms first, so each member built one
  algorithm family. Those became the foundation of the 3D app rather than
  being thrown away (the race route still runs on the Checkpoint 2 Catmull-Rom code).
- Real open data (OpenF1, FIA timing, Lombardy's survey) was available, so the
  track, timing and telemetry are measured rather than approximated.
- Once the 3D renderer had lighting and depth, scenery that was out of scope became
  cheap to add and made the scale of the circuit readable.

**Roles also shifted** from the proposal; section 2 explains what each person
actually built.

| Member | Proposed role | Actual work |
| --- | --- | --- |
| Omar | Interface and integration | Integration, transformation engine, 3D renderer, data pipeline, heatmap, HUD |
| Azhar | 3D track geometry, matrix transformations | Line algorithms and clipping; timeline verification and documentation |
| Hetvi | Camera controls, path animation | Polygon fill; polygon car animation; live wheels and a follow camera in OpenGL |
| Nyariki | Telemetry pipeline | Midpoint circles and ellipses; hierarchical wheel animation |
| Shadrack | Colour interpolation | Closed Catmull-Rom spline path with tangent-oriented lap motion |
| Casper | UI overlays, legend, documentation | Bézier curves; first timeline/interpolation; Bézier route in the OpenGL scene |

---

## 2. Group responsibilities

Each member covers four points: responsibilities, techniques, what they
personally built, and a challenge with how they solved it. Show your own code and
output on screen while you speak.

The challenges below are drawn from commits and docs. **Each member should check
their part and replace anything that does not match their own experience.**

### Azhar Ahmed: line drawing and clipping, timeline verification

- **Responsibilities:** Checkpoint 1 line algorithms; Checkpoint 2 timeline
  verification, cleanup and documentation.
- **Techniques:** DDA, Bresenham, Cohen-Sutherland clipping; keyframe interpolation.
- **Built:** `checkpoint1/lines/main.cpp`: `drawLineDda` (line 111),
  `drawLineBresenham` (line 145) and `drawClippedSegment` (line 261), with the
  PNG evidence in `checkpoint1/lines/output/lines_demo.png`. Documentation of
  the timeline in `docs/algorithms/animation-timeline.md`.
- **Challenge to talk about:** making Bresenham work in all eight octants with
  integers only (swap the axes for steep lines and step in the direction of the
  sign), and handling segments that are partly inside the clip window. For the
  timeline, angle interpolation had to take the short way round at 360 degrees
  (-340 degrees becomes +20 degrees).
- **Show:** `lines_demo.png`, then the Bresenham function.

### Hetvi Barot: polygons and the animated car

- **Responsibilities:** Checkpoint 1 polygon representation and filling; Checkpoint
  2 keyframed polygon car; bringing the wheels into the live OpenGL car.
- **Techniques:** scanline polygon fill; composite transformation
  `M = T(p) R(theta) S(sx, sy) T(-c)`; keyframe animation; follow camera.
- **Built:** `scanlineFill` in `checkpoint1/polygons/main.cpp` (line 159); the 13
  polygon car in `checkpoint2/animation/car/`; steering and rolling CP2 wheels on
  the OpenGL car with a close-up camera that follows it (merged in #21).
- **Challenge to talk about:** rotating and scaling about the car's centre instead
  of the origin, solved by translating to the origin first with `T(-c)`. Later,
  merging main's new Bézier route into the live wheel branch without breaking
  either feature.
- **Show:** `checkpoint2/animation/car/output/car_contact_sheet.png`, then press
  `W` in the app for the wheel inspector.

### Stephen Nyariki: circles, ellipses and wheel hierarchy

- **Responsibilities:** Checkpoint 1 midpoint circles and ellipses; Checkpoint 2
  hierarchical wheel animation.
- **Techniques:** midpoint circle and two-region midpoint ellipse algorithms with
  eight-way and four-way symmetry; hierarchical (parent/child) transformations.
- **Built:** `drawCircle` and `drawEllipse` in `checkpoint1/circles/main.cpp`
  (lines 127 and 157) with tests; `checkpoint2/animation/wheels/`, where each
  wheel's spin is applied in its local frame and then carried by the car body.
- **Challenge to talk about:** the ellipse's switch from Region 1 to Region 2 where
  the slope reaches -1 (`2 ry^2 x = 2 rx^2 y`); getting it wrong leaves gaps or
  doubled pixels. For the wheels: spinning about the wheel's own centre, not the
  car's.
- **Show:** `checkpoint1/circles/output/circles_ellipses_demo.png` and the wheel
  contact sheet; in the app, the rotating tyres in the inspector.

### Shadrack Mungai: the spline racing line

- **Responsibilities:** Checkpoint 2 closed spline path and lap animation.
- **Techniques:** closed (periodic) Catmull-Rom spline; tangent from the spline
  derivative; heading from `atan2` of the tangent.
- **Built:** `checkpoint2/animation/splines/` with lap frames and tests. The 3D app
  still drives every car on this algorithm: `pathTangent` and `stableTangent` in
  `include/apex3d/cp3_scene.hpp` (lines 86 to 108) call the same Catmull-Rom code.
- **Challenge to talk about:** closing the loop so the last segment joins the first
  smoothly (control points wrap around), and keeping the heading stable where the
  tangent is almost zero (fallback to a finite difference).
- **Show:** `checkpoint2/animation/splines/output/spline_contact_sheet.png`, then
  the car following the racing line in chase view.

### Casper Muhereza: Bézier curves and the timeline

- **Responsibilities:** Checkpoint 1 Bézier foundations; first Checkpoint 2
  timeline/interpolation; the Bézier lap route in the OpenGL scene.
- **Techniques:** cubic Bézier curves, C1 continuity across joins, arc-length
  parameterisation, linear interpolation and easing.
- **Built:** `checkpoint1/bezier/`, `checkpoint2/animation/timeline/timeline.hpp`,
  and the closed Bézier route rendered in 3D (`B` in the app, #20).
- **Challenge to talk about:** the car jumped at segment joins because progress was
  reset per segment; fixed by interpolating one continuous `u` across segment
  boundaries in `bezierSampleAtProgress`. C1 continuity was enforced by making each
  join's incoming and outgoing control points collinear and equal in length.
- **Show:** press `B`, `P` (anchors) and `T` (scaling) in the app.

### Omar Siddique: integration, 3D renderer and data

- **Responsibilities:** project coordination and integration; Checkpoint 2
  transformation engine and Bézier animation; the Checkpoint 3 OpenGL application,
  data pipeline, verification and evidence.
- **Techniques:** homogeneous 2D and 3D matrices, perspective and look-at
  projection, Phong-style lighting, shadow mapping with filtering, vertex
  displacement from a height map, colour interpolation for the heatmap, camera
  smoothing, screen-space projection for HUD markers.
- **Built:** `checkpoint2/transformations/include/transform2d.hpp`;
  `include/apex3d/math3d.hpp`, `renderer3d.hpp`, `scene3d.hpp`, `terrain.hpp`,
  `telemetry_heatmap.hpp`, `tv_camera.hpp` and the Python tools that turn the
  OpenF1, FIA and survey data into checked C++ headers.
- **Challenges to talk about:**
  1. *Scale.* Early on the car was drawn far larger than the road. Moving the whole
     world to 1 unit = 1 metre fixed the car, road, tyres and cameras together.
  2. *Shadow acne and peter-panning.* Solved with a slope-scaled bias and a 3x3
     percentage-closer filter on a 4096 square shadow map.
  3. *Terrain.* Lifting every vertex by the survey on the GPU meant the CPU cameras
     had to sample the same height map with the same bilinear lookup, or the camera
     would sink into hills.
  4. *Honest data.* Keeping a clear line between recorded data and reconstruction,
     documented in `docs/monza-race-research.md`.
- **Show:** the techniques in section 4.

---

## 3. Project components (Omar, Hetvi)

How they connect:

```text
Recorded data (FIA timing, OpenF1 positions/telemetry, survey)
   -> generated C++ headers
   -> Race clock (playback, seek, rate)
        -> Race motion (Catmull-Rom path + timing keyframes) -> 20 cars
        -> Telemetry -> heatmap colours on the track, HUD panels
   -> World meshes (track, kerbs, pit, scenery) lifted by the terrain map
   -> Cameras (overview, chase, cockpit, TV)
   -> Renderer: shadow pass -> lit scene -> sky -> HUD overlay
```

| Component | Purpose | Code | Demo |
| --- | --- | --- | --- |
| Track ribbon, kerbs, barriers | The circuit itself, at measured width | `scene3d.hpp`, `track_edges.hpp` | Overview, zoom into the first chicane |
| Terrain | Measured ground around the circuit | `terrain.hpp`, `generated/monza_terrain_2025.hpp` | Overview from a low angle |
| F1 cars (20) | Each entry at its recorded timing | `scene3d.hpp`, `race_field.hpp` | Chase view, click drivers in the tower |
| Wheels and tyres | Rolling and steering wheels; compound and age | `cp3_wheels.hpp`, `tyre_state.hpp` | `W` |
| Pit lane and pit stop | 24.5 s visit and 2.3 s tyre change | `cp3_race.hpp` | `J` |
| Start gantry and lights | Five-light start | `hud3d.hpp`, `scene3d.hpp` | `R` |
| Scenery | Trees, grandstands, buildings, sky, sun | `world_detail.hpp`, `sky_shader.hpp`, `sun_position.hpp` | Overview |
| Heatmap overlay | Telemetry painted on the road | `telemetry_heatmap.hpp` | `V` |
| HUD | Clocks, telemetry dial, timing tower, weather, flags | `cp3_hud.hpp`, `timing_tower.hpp` | Always visible; `H` hides it |
| Cameras | Ways to view the race | `scene3d.hpp`, `tv_camera.hpp` | `C` |
| Bézier teaching route | The Checkpoint 2 curve, in 3D | `cp3_scene.hpp` | `B` |

---

## 4. Graphics techniques and algorithms

For each: what it is, how it is implemented, which component uses it, what it
achieves, and the demonstration.

### Checkpoint foundations (Azhar, Nyariki, Hetvi, Casper)

Briefly recap, since section 2 covered the detail: DDA and Bresenham lines,
Cohen-Sutherland clipping, midpoint circles and ellipses, scanline fill, Bézier
and Catmull-Rom curves. Show the CP1 output PNGs side by side. Point out that the
2D algorithms ran on our own CPU rasteriser, with no graphics library; the 3D app moves rasterisation
to the GPU but reuses the curve, transformation and animation code.

### 2D homogeneous transformations (Omar)

- **What:** translation, rotation and scaling as 3x3 matrices on homogeneous
  coordinates, composed by matrix multiplication.
- **How:** `checkpoint2/transformations/include/transform2d.hpp`; the rightmost
  matrix applies first, so `T(p) R S T(-c)` rotates and scales about the centre `c`.
- **Used by:** the polygon car, wheels, the HUD sidebar (including the inverse
  transform for mouse hit testing in `hud3d.hpp`).
- **Achieves:** one engine for every 2D movement, and clickable buttons that still
  work when the sidebar is resized.
- **Demo:** `checkpoint2/transformations/output/transformations_demo.png`; click a
  driver in the timing tower.

### 3D viewing pipeline: look-at and perspective (Omar)

- **What:** model, view and projection matrices.
- **How:** `math3d.hpp`: `lookAt` (line 64) builds the camera basis from forward,
  side and up vectors; `perspective` (line 44) builds the frustum from field of
  view, aspect and near/far planes. Matrices are column-major and uploaded without
  transposing.
- **Used by:** every camera; also projecting car positions to the screen to place
  overview markers.
- **Achieves:** real depth and foreshortening.
- **Demo:** `C` through the cameras; scroll in cockpit view to change field of view.

### Path animation and tangent heading (Shadrack, Omar)

- **What:** moving a car along a parametric curve and turning it to face along it.
- **How:** the car's progress comes from the timing keyframes; Catmull-Rom gives
  the position and its derivative gives the tangent; yaw is `atan2` of the tangent
  (`cp3_scene.hpp`, lines 86 to 129).
- **Used by:** all 20 cars and the cameras that follow them.
- **Achieves:** smooth motion that still crosses every timing line at the recorded
  time.
- **Demo:** chase view at 4x; press `N` to jump to a lap boundary and read the lap
  time.

### Hierarchical transformations (Nyariki, Hetvi)

- **What:** child parts move in their parent's frame.
- **How:** each tyre spins about its own axle (local rotation), steers about a
  vertical pivot, then the car body transform places both in the world. Spin angle
  is distance travelled divided by the 0.36 m tyre radius.
- **Used by:** wheels and tyres.
- **Demo:** `W` for the inspector; pause and it stops; `J` and it stops in the box.

### Depth buffer (z-buffer) (Omar)

- **What:** keep the nearest surface per pixel.
- **How:** OpenGL depth testing in the scene pass; the HUD is drawn last with depth
  testing off so it always sits on top.
- **Demo:** orbit so a grandstand passes in front of the track.

### Lighting and shading (Omar)

- **What:** ambient, diffuse (Lambert) and specular light from the sun.
- **How:** per-vertex normals from cross products of triangle edges (`scene3d.hpp`,
  line 23); the fragment shader in `renderer3d.hpp` (around line 68) combines
  ambient, `max(dot(n, sun), 0)` diffuse and a specular highlight. The sun
  direction is the real solar position at Monza for the race time
  (`sun_position.hpp`).
- **Demo:** orbit the car; the sunlit side and the highlight move with the view.

### Shadow mapping with PCF (Omar)

- **What:** two-pass shadows.
- **How:** pass 1 renders depth from the sun with an orthographic light matrix into
  a 4096 square depth texture. Pass 2 projects each fragment into light space and
  compares depths, with a slope-scaled bias against acne and a 3x3
  percentage-closer filter for soft edges (`renderer3d.hpp`, lines 55 to 66).
- **Demo:** zoom in on a car and on the trees; show the soft shadow edges.

### Colour interpolation: telemetry heatmap (Omar, from the proposal)

- **What:** map a value to a colour by piecewise linear interpolation.
- **How:** `heatColor` in `telemetry_heatmap.hpp` (line 51) normalises the value to
  0 to 1 and blends between four stops (red, orange, yellow, green);
  `roadHeatColor` mixes 25% asphalt back in so the road still reads as road. The
  colours are written into the track mesh vertices and the GPU interpolates
  between them across each triangle (Gouraud-style).
- **Demo:** `V` through speed, throttle, brake and pedals; click another driver and
  the track repaints.

### Height-map vertex displacement and bilinear sampling (Omar)

- **What:** lift flat geometry onto measured terrain.
- **How:** the 5 m survey grid is uploaded as a texture; the vertex shader samples
  it bilinearly and adds the height, and recomputes the normal from central
  differences of the height (`renderer3d.hpp`, lines 22 to 41). The CPU uses the
  same lookup (`terrain.hpp`) so cameras sit on the same ground.
- **Demo:** low overview along the back straight.

### Camera smoothing and broadcast cuts (Omar)

- **What:** exponential smoothing of the chase camera; precomputed TV camera cuts
  with a line-of-sight test against trees.
- **How:** each frame the chase eye moves a fraction `1 - exp(-9 dt)` of the way to
  its target (`scene3d.hpp`, line 563), so the motion is smooth at any frame rate.
  TV cuts pick the nearest tower to a point 40 m ahead of the car whose view clears
  the trees (`tv_camera.hpp`).
- **Demo:** `C` to TV view at 4x.

---

## 5. Recommendations and lessons learnt (Casper, Omar)

**Future plans**
- Lap comparison: two drivers or two laps side by side with a time delta painted
  on the track (the proposal's "where time was lost").
- Free camera pan.
- Texture mapping for the road, grass and team liveries (currently vertex colours).
- Instanced rendering for trees and spectators to raise the frame rate.
- A Windows audio backend.

**Removed or deferred:** rain effects and live telemetry stay out of scope.

**Recommendations**
- Fix units and scale on day one.
- Keep recorded data and reconstruction clearly separated and documented.
- Reuse checkpoint code instead of rewriting it; it kept the 3D app honest to the
  course algorithms.

**Lessons learnt**
- Small pull requests with tests made it safe to integrate six people's work.
- Deterministic screenshot modes made visual changes easy to verify.
- Proposals change once real data and real constraints appear; documenting why is
  part of the work.

---

## Demo run sheet

Build and check once before recording, from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

```bash
cmake --build build --parallel
```

```bash
ctest --test-dir build --output-on-failure
```

```bash
./build/Apex3D
```

| Step | Keys / action | What to point out |
| --- | --- | --- |
| 1 | Launch, click **START** or `Space` | Five-light start; race clock starts at lights-out |
| 2 | `+` twice (4x) | Cars follow the spline; lap counter |
| 3 | `C` repeatedly | Overview, chase, cockpit, TV: look-at and perspective |
| 4 | Drag and scroll in overview | Orbit and zoom; depth testing as stands pass in front |
| 5 | `V` repeatedly | Heatmap layers and colour key |
| 6 | Click a driver in the timing tower | Track repaints; camera follows; inverse hit testing |
| 7 | `W` | Wheel hierarchy and tyre inspector |
| 8 | `J`, then `Space` if paused | Pit lane, 2.3 s stop, crew |
| 9 | `N` | Seek to lap boundary; lap time check |
| 10 | Drag the timeline | Scrubbing through the race |
| 11 | Zoom in on a car | Lighting, specular highlight, PCF shadows |
| 12 | `B`, then `P` and `T` | Bézier route, anchors, scaling |
| 13 | `B`, then `E` | Back to Monza; parked field after the race |
| 14 | `H` | Full-width scene without the HUD |

Checkpoint output images to have open in a second window:

- `checkpoint1/lines/output/lines_demo.png`
- `checkpoint1/circles/output/circles_ellipses_demo.png`
- `checkpoint1/bezier/output/bezier_demo.png`
- `checkpoint1/splines/output/splines_demo.png`
- `checkpoint2/transformations/output/transformations_demo.png`
- `checkpoint2/animation/car/output/car_contact_sheet.png`
- `checkpoint2/animation/wheels/output/wheel_contact_sheet.png`
- `checkpoint2/animation/splines/output/spline_contact_sheet.png`
- `checkpoint2/animation/bezier/output/bezier_contact_sheet.png`
