# Apex3D architecture

## Race state and 3D rendering

```text
FIA timing CSV -> generated integer-ms data -> 53-lap race/sector boundaries
Real frame delta -> ready/start/pause/rate/seek/finish -> race clock
    -> current/last lap, sector/pit keyframes and unwrapped progress
    -> CP2 Catmull-Rom or Bezier position + tangent
    -> 3D pose: (path.x, road height, -path.y), yaw from tangent
    -> solid F1-style car, rotating tyres and optional service crew

Static sampled route -> raised track/kerbs, barriers and parkland
Pit route -> road ribbon, service box, garages and pit wall
Starting state -> 3D gantry lamps + compact screen-space lamp display

3D mesh normals + directional sunlight -> lit material shader
Static/dynamic meshes -> directional light-space depth map -> filtered shadows
Camera -> perspective x look-at matrix -> depth-tested scene viewport
Race dashboard and camera hints -> screen-space overlay, depth disabled
```

The historical replay clock and pit assumptions are independent of the renderer.
All sector and lap crossings retain their documented times. Intra-sector motion,
pit geometry and box-arrival splits remain interpolated; they are not a GPS,
continuous speed-telemetry or full-field simulation. See `monza-race-research.md`.

The app starts Ready and muted. Space or START begins the illustrative real-time
6.5-second five-light sequence. The race clock starts at lights-out and supports
0.25x–64x, pause, seeking and an exact 4,404.325-second finish. Seeking reconstructs
state directly without replaying old start cues. There is no Monza lap 54.

## Meshes and world coordinates

`scene3d.hpp` converts the existing 2D path into an X/Z ground plane with Y up.
It builds actual volume: track sides and raised kerb rings, guardrails, garages,
a pit wall, grandstands, trees and a start gantry. Road and tyre contact share
the same surface height. Raised kerb pieces that fold into the sampled road
are omitted; the original layered track surface fills tight corners.

The car is a procedural solid model with a lofted monocoque/nose, front and rear
wings, suspension rods, cylindrical tyres, six-spoke rims, a cockpit, helmet,
airbox and halo. Local geometry and normals are rotated into the path tangent.
Race wheel rotation uses reconstructed metres and nominal 2025 tyre radius,
independent of the exaggerated rendering scale. The old fixed-body inspector
remains available as a separate educational overlay. Procedural crew figures
are visible during stationary service; loaded tyre radius, slip and differential
wheel speed are not simulated.

The road's course elevation is intentionally planar. Raised surfaces and buildings
are model geometry, not a surveyed Monza elevation/architecture dataset. No new
external model, texture or rendering dependency is required.

## Perspective, depth, light and shadows

`math3d.hpp` provides column-major 4x4 matrices, perspective/orthographic projection,
look-at, vector operations and camera frames. Matrices are uploaded with no
transpose. `renderer3d.hpp` owns its shader programs, vertex buffers and depth
framebuffer and destroys them while the OpenGL context is still current.

The renderer draws a directional light-space depth pass, then a perspective lit
scene with depth testing. Vertex normals drive ambient/diffuse/specular lighting.
A 4096-square directional shadow map uses a 3x3 depth comparison filter and slope
bias; unavailable shadow buffers fall back to directional lighting. Four-sample
MSAA is requested. The dashboard is drawn last in the full-window viewport with
depth testing disabled, keeping its clocks and controls legible. Screenshot
modes use a hidden GLFW context and suppress macOS menu creation so evidence
capture does not take over the active preview.

## Cameras and interaction

- Overview: orbit the whole course; drag adjusts yaw/pitch and scroll changes distance.
- Chase: follow the car with a smoothed eye/target; drag orbits the car. Seeking and
  mode changes reset smoothing to avoid flying across the course.
- Cockpit: a driver-height perspective looking forward through the halo; scroll
  adjusts field of view. Dragging transitions to the external chase/orbit view.

`C` cycles cameras, `F` resets the current view and `H` hides/shows the dashboard.
The 3D viewport expands when the dashboard is hidden. The race controls and
clickable mute/playback buttons remain available in the narrower sidebar.
Mouse hit regions use the inverse of the overlay's horizontal transform. GLFW
cursor events provide coordinates for both hit testing and dragging, including
retina/targeted desktop input. Sticky key/mouse states preserve short presses.

## Separate Bezier teaching route

`B` switches to the Monza Bézier chain and resets playback. That route
also renders in 3D but retains its separate 18-second teaching lap, optional
looping, scaling and anchors. It has no historical race timings or pit visit.
Switching back to Monza restarts its lights and documented race schedule.

## Source map

| Path | Responsibility |
| --- | --- |
| `src/main.cpp` | GLFW input/context, race/audio transitions, view selection, capture and frame loop |
| `include/apex3d/math3d.hpp` | 3D vectors, projection and look-at matrices |
| `include/apex3d/scene3d.hpp` | World/car/crew/gantry meshes, normals and camera rig |
| `include/apex3d/renderer3d.hpp` | Lit/depth shaders, mesh buffers, shadow framebuffer and rendering passes |
| `include/apex3d/hud3d.hpp` | Sidebar transform, inverse hit testing, camera hints and compact start overlay |
| `include/apex3d/cp3_scene.hpp` | Original route sampling, 2D track/car foundations and shared model scale |
| `include/apex3d/cp3_race.hpp` | Historical race clocks, sector/pit keyframes and reconstructed pit route |
| `include/apex3d/cp3_playback.hpp` | Start/pause/rate/seek/finish and separate demo playback |
| `include/apex3d/cp3_hud.hpp` | Bitmap text, race dashboard, clocks, buttons and fixed-body wheel inspector |
| `include/apex3d/cp3_start_audio.hpp` | Optional muted-by-default CC0 start cues on macOS |
| `data/monza-2025-verstappen.csv` | Inspectable timing facts in integer milliseconds |
| `tools/generate_race_assets.py` | Reproducible embedding/check of timing and PCM inputs |
| `tests/scene3d_tests.cpp` | Projection, normals, volume, tyre contact, wheel motion, cameras and HUD mapping |
| `tests/cp3_scene_tests.cpp` | Existing path, transformations and teaching-mode integration |
| `tests/cp3_race_tests.cpp` | All laps/sectors, pit service/joins, lights, mute, seeking and finish |

Hosted CI, lecturer-rubric alignment, group review and submission remain separate
from the verified local implementation. The visualization uses procedural assets
and reconstructed motion rather than production simulator physics or telemetry.
