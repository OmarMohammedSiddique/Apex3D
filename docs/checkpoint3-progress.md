# Checkpoint 3 Progress

Checkpoint 3 asks the group to integrate previous components into an OpenGL
prototype. The current branch contains the extended integration milestone.

| Item | Status | Evidence |
| --- | --- | --- |
| OpenGL application window and shaders | Complete | `src/main.cpp` creates a GLFW window and OpenGL 3.3 context. |
| Filled Monza surface | Complete | `include/apex3d/cp3_scene.hpp` builds a layered triangle mesh from the original Monza points. |
| CP2 curve integration | Complete | The scene calls the CP2 Catmull-Rom position and tangent functions. |
| CP2 transformation integration | Complete | The car's pose uses the shared 3 x 3 matrix for translation, rotation and optional scaling. |
| CP2 polygon-car integration | Complete | The CP2 car components are triangulated and rendered in OpenGL. |
| Live lap animation | Complete | Distance-based path lookup updates the car every frame, approximately evenly along the circuit. |
| Interaction and feedback | Complete | Play/pause, restart, speed, scale demonstration, points toggle and an in-window lap display. |
| Source and visual verification | Complete | `ctest --test-dir build --output-on-failure` and `docs/evidence/cp3-opengl-prototype.png`. |
| Wheel hierarchy in the live scene | Complete | Live side-view inset uses CP2 body placement and wheel poses, driven by accumulated travel; `W` toggles visibility. |
| Bézier path as a selectable live route | Complete | `B` switches to the original CP2 Bézier demo circuit with a matching road, tangent-oriented car and anchor display. |
| Group explanation and evidence | Prepared | `docs/checkpoint3-demo-guide.md` and two real OpenGL screenshots; final rubric alignment and submission remain pending. |
| Automated regression checks | Configured | GitHub Actions builds Release and runs CP3/CP2 checks on Linux and macOS; hosted results are pending until pushed. |

## Run the prototype

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/Apex3D
```

The app is currently a 2D top-down prototype, not the final 3D visualisation.
Its path and car are simplified and hand-authored; telemetry, perspective,
lighting and detailed scene assets are still future work.

## Verification for this integration

Local macOS build and expanded CTest pass. Real OpenGL frames were captured
on Apple M4 / OpenGL 4.1. Live interaction confirmed pause, route switching
while paused, anchor display, wheel inset visibility, restart and speed change.
The Bézier route uses its own original CP2 circuit rather than claiming to
reproduce Monza. The track and HUD have separate horizontal areas.

The wheel inset is a repeating side-view teaching demonstration. Its parent
translation and local rotation are derived from the same travelled distance;
its placement repeats every revolution. It freezes when playback is paused
and resets on restart or route switch. The top-down car remains unchanged.

The lecturer's latest rubric has not yet been supplied. No final submission,
assessment compliance, or completed 3D scene is claimed.
