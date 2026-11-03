# Checkpoint 3 Progress

Checkpoint 3 asks the group to integrate previous components into an OpenGL
prototype. The current branch contains the first working integration milestone.

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
| Wheel hierarchy in the live scene | Pending | The CP2 wheel demonstration remains separate. |
| Bézier path as a selectable live route | Pending | The CP2 Bézier demonstration remains separate. |
| Group explanation and final submission evidence | Pending | Prepare after the lecturer publishes the combined checkpoint specifics. |

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
