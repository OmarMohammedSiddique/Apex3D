# Apex3D

Apex3D is an ICS 3204 Computer Graphics semester project that turns Formula 1
circuit geometry into a progressively richer visualisation. Checkpoints 1 and 2
demonstrate the underlying graphics algorithms in C++20. The interactive OpenGL
application now renders a 3D circuit, car and scenery around a documented race replay.

## Podium replay and driver focus

The replay includes Norris and Piastri with independent 53-lap timing,
readable overview pointers, deeper zoom, a recorded speed/gear/engine-RPM panel,
and 37 grandstand sections placed by the official 2025 map's corner areas and
sides. Click a podium row to follow any driver; only the selected car shows a pointer.
The telemetry panel has a speed dial, gear tile and segmented RPM bar. Scenery footprints and racing lines
are reconstructed. Sources and accuracy limits are in
[Podium, telemetry and grandstands](docs/podium-telemetry-grandstands.md). The wheel inspector also shows
[tyre compound, set number and completed laps](docs/tyre-inspector.md), including
prior use of reused sets.

![Driver focus and recorded telemetry](docs/evidence/apex3d-driver-focus.png)

## Current milestone: 3D Monza race visualization

The 3D OpenGL application follows **Max Verstappen's 53-lap 2025 Italian Grand Prix**
using the FIA lap and sector timings. F1 reported this as its fastest race:
**1:13:24.325**. The first lap takes **1:27.159**, not a repeated qualifying
record. A race timer, current/last lap times, lap counter and progress bar
follow the same clock. The replay ends at lap 53 with the clock fixed at the
official result.

A five-light start precedes the race clock. The drawn pit lane includes the
single documented **24.545-second visit**, with a **2.3-second stationary tyre
change** from Medium to Hard. The car body stays fixed in the optional wheel
inspector while local tyre rotation follows reconstructed travel and the nominal
2025 tyre radius.

The scene has a volume-modelled F1-style car with a cockpit, halo, suspension,
front/rear wings and rotating tyres. It includes a raised track and kerbs,
3D pit garages, a pit wall, starting gantry, grandstands, parkland trees,
directional lighting and filtered sun shadows. A small procedural pit crew
appears during the documented tyre-change interval.

Choose an overview, chase or cockpit perspective. Drag the scene to orbit,
scroll to zoom and press `F` to reset. `H` hides the dashboard. The chase view
smooths camera movement and resets on seeks; it does not change race timing.

This is a **timing-based, schematic reconstruction**. Lap and sector crossings
match the reports; positions within sectors, pit geometry and box-arrival
splits are interpolated. See [research and assumptions](docs/monza-race-research.md)
for primary sources and the distinction between qualifying, race-lap and
full-race records. The Monza Bézier route remains a separate teaching demo.

The 3D world now uses **1 unit = 1 metre**, with a 5,793 m centreline, a 12 m racing surface, and a representative 2025 F1 car with 720 mm tyres. Chase view opens by default. Dimensions, sources and estimated scenery sizes are documented in [Physical scale](docs/physical-scale.md).





### Build and run

Run these commands **inside the Apex3D project folder**, where CMakeLists.txt
is located. Requires CMake 3.24+, a C++20 compiler and OpenGL 3.3.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/Apex3D
```

The app opens ready and muted. Click **START** or press `Space` to begin the
lights. Click **UNMUTE/MUTE** or press `M` for optional short start cues. There
is no engine or background sound. Native audio currently supports macOS;
other platforms run silently with a NO AUDIO button. Screenshot modes remain
silent. The small CC0 samples and timing data are embedded, so launching the
executable does not require a network connection or a particular working folder.

On Windows the executable may be under `build/Release/Apex3D.exe`.

| Control | Action |
| --- | --- |
| `Space` / playback button | Start, pause, resume or replay after the finish |
| `R` | Restart with the light sequence |
| `+` / `-` | Double/halve playback rate, between 0.25x and 64x |
| `1` | Restore real-time playback |
| Left / Right | Seek backward/forward 10 race seconds |
| `N` | Seek to the next lap boundary |
| `J` | Seek to five seconds before the pit visit |
| `W` | Show or hide the fixed-body wheel inspector |
| `M` / sound button | Unmute/mute start cues; muted at every fresh launch |
| `P` | Show or hide 3D route anchors |
| `C` | Cycle overview, chase and cockpit cameras |
| Drag in the scene | Orbit the overview/chase camera |
| Scroll in the scene | Zoom the overview/chase camera |
| `F` | Reset the current camera |
| `H` | Hide/show the dashboard |
| `B` | Switch Monza race / Monza Bézier demonstration |
| `L` | Toggle looping in the Bezier demonstration only |
| `T` | Toggle scaling in the Bezier demonstration only |
| `Esc` | Quit |

At 1x the full race lasts 73 minutes 24.325 seconds after lights-out, excluding
pauses. At 64x it takes about 69 seconds. Start lights always run at real-time
pace; the illustrative sequence takes 6.5 seconds and is excluded from the
race timer. Seeking jumps directly to a race state without replaying old cues.

### Capture and inspect

```bash
./build/Apex3D --screenshot-start 5 five_lights.ppm
./build/Apex3D --screenshot-time 87.159 lap_two.ppm
./build/Apex3D --screenshot-time 3088.249 tyre_change.ppm --camera chase --wheels
./build/Apex3D --screenshot-time 4404.325 finish.ppm
./build/Apex3D --race-time 3072.249 --wheels
./build/Apex3D --screenshot-time 120 cockpit.ppm --camera cockpit
./build/Apex3D --bezier --screenshot-at 0.25 bezier.ppm
```

`--race-time` opens paused at the requested race second. `--screenshot-at`
retains the old 0..1 lap-progress capture option (first lap for Monza).
`--camera overview|chase|cockpit` sets the initial perspective.
`--speed 0.25..64` sets the initial rate; `--wheels` opens the inspector.

To verify or regenerate embedded data after editing the CSV/WAV inputs:

```bash
python3 tools/generate_race_assets.py --check
python3 tools/generate_race_assets.py
```

## Checkpoint 2 foundation

Checkpoint 2 builds on the graphical components from Checkpoint 1 by applying
2D transformations and frame-based animation to a simplified Formula 1 car.
The repository now contains six independently runnable CP2 milestones:

- 3 × 3 homogeneous transformations: translation, rotation, scaling, pivot
  rotation, composition and polygon transformation
- animation timeline, linear position interpolation and shortest-arc heading
  interpolation
- hierarchical wheel rotation and rolling-without-slipping motion
- keyframed polygon-car translation, rotation, scaling and combined motion
- tangent-oriented Catmull–Rom circuit animation
- tangent-oriented cubic Bézier animation with approximate arc-length sampling

All CP2 work is implemented in C++20 without OpenGL or another graphics
library. The programs rasterise their own PPM evidence images on the CPU.

## Group members and responsibilities

| Member | Responsibilities demonstrated in the repository |
| --- | --- |
| Omar Siddique | Project coordination, transformation engine, CP2 Bézier animation, CP3 OpenGL integration, verification and evidence |
| Hetvi Barot | Polygon representation and filling; keyframed polygon Formula 1 car animation |
| Azhar Ahmed | Line algorithms; CP2 timeline verification, cleanup and documentation |
| Nyariki | Midpoint circles and ellipses; hierarchical wheel animation |
| Casper Muhereza | Checkpoint 1 Bézier foundations and the initial CP2 timeline/interpolation implementation |
| Shadrack Mungai | Closed Catmull–Rom spline path and tangent-oriented lap animation |

## Technologies and tools

- C++20 standard library
- Handwritten CPU rasterisation and graphics algorithms
- 3 × 3 homogeneous transformation matrices
- PPM and PNG evidence outputs
- CMake, GLFW, GLAD and OpenGL 3.3 for the integrated prototype
- Git and GitHub pull-request workflow

## Verify Checkpoint 2

On macOS or Linux, run the complete strict verification from the repository
root:

```bash
checkpoint2/verify_all.sh
```

The script compiles every CP2 test and demonstration with:

```text
-std=c++20 -Wall -Wextra -Wpedantic
```

It runs the transformation, timeline, wheel, polygon-car, spline and Bézier
test suites and regenerates disposable evidence in a temporary directory.

GitHub Actions runs the release build, CP3 scene checks, and complete CP2
verification on Linux and macOS. OpenGL screenshots and keyboard interaction
are checked locally; CI does not launch the graphical window.

Each milestone also has its own README with individual build and run commands:

- [Checkpoint 2 overview](checkpoint2/README.md)
- [Transformations](checkpoint2/transformations/README.md)
- [Timeline and interpolation](checkpoint2/animation/timeline/README.md)
- [Wheel animation](checkpoint2/animation/wheels/README.md)
- [Polygon-car animation](checkpoint2/animation/car/README.md)
- [Catmull–Rom motion](checkpoint2/animation/splines/README.md)
- [Bézier motion](checkpoint2/animation/bezier/README.md)

## Evidence and documentation

- [Monza race research and assumptions](docs/monza-race-research.md)
- [CP3 progress and remaining work](docs/checkpoint3-progress.md)
- [CP3 demo and explanation guide](docs/checkpoint3-demo-guide.md)
- [CP3 OpenGL screenshot](docs/evidence/cp3-opengl-prototype.png)
- [CP2 readiness checklist](docs/checkpoint2-readiness.md)
- [CP2 evidence PDF](output/pdf/Apex3D_Checkpoint2_Evidence.pdf)
- [Architecture and rendering pipeline](docs/architecture.md)
- [Contribution workflow](CONTRIBUTING.md)
- [Algorithm notes](docs/algorithms/)

## Current limitations

- The track is a flat-height 3D reconstruction with raised surfaces and
  buildings. No surveyed Monza elevation model is supplied. Cars, scenery and
  crew are procedural low-poly models rather than detailed licensed assets.
- Geometry is simplified and hand-authored. The replay uses one driver's
  timing results; it does not reproduce the full field or GPS/speed telemetry.
- Intra-sector speed, pit geometry and service-box arrival are reconstructed.
  Exact lap times do not imply exact corner-by-corner motion.
- Race wheel RPM uses reconstructed travel in metres and the nominal 720 mm
  2025 tyre diameter, not measured wheel telemetry. Spokes may visually alias at high playback rates.
- Native starting-cue audio currently supports macOS only and is muted by default.
- Lecturer-rubric alignment, hosted CI, group review and submission remain
  separate from verified local builds and tests.

## Reference

The circuit work uses the official Formula 1 Monza map as a shape reference:
[Italy — Autodromo Nazionale Monza](https://www.formula1.com/en/information/italy-autodromo-nazionale-monza.FiJN1jnQlRLeHqOxIt13m).
