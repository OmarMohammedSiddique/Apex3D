# Apex3D Architecture

## Current OpenGL prototype

The Checkpoint 3 application connects earlier C++ graphics work to a live
OpenGL renderer:

```text
Hand-authored Monza points
    -> even-distance resampling
    -> CP2 Catmull-Rom interpolation
    -> sampled lap distances and track surface
    -> static OpenGL triangle buffer

Elapsed time -> lap progress -> path position and tangent
    -> CP2 Mat3 translation, rotation and optional scaling
    -> CP2 polygon-car model -> triangulated car mesh
    -> dynamic OpenGL triangle buffer

Playback state -> in-window progress and controls -> dynamic HUD buffer
```

GLFW creates the window and handles keys. GLAD loads OpenGL functions. A vertex
shader corrects the scene for the window's aspect ratio and passes per-vertex
colour to a fragment shader. The track mesh is uploaded once; the car and HUD
meshes are updated as the lap progresses. The car is drawn after the track, and
the HUD is drawn last in clip coordinates.

## Source map

| Path | Responsibility |
| --- | --- |
| `src/main.cpp` | Window, shaders, buffers, playback controls and frame loop |
| `include/apex3d/cp3_scene.hpp` | Monza points, path resampling, CP2 spline reuse, track mesh, CP2 car placement and triangulation |
| `include/apex3d/cp3_hud.hpp` | Small CPU-built text and progress display drawn with OpenGL triangles |
| `tests/cp3_scene_tests.cpp` | Closure, monotonic lap distance, car orientation and mesh checks |
| `checkpoint2/transformations/include/transform2d.hpp` | Shared 3 x 3 matrix and polygon transforms |
| `checkpoint2/animation/splines/include/spline_motion.hpp` | Catmull-Rom position and analytic tangent |
| `checkpoint2/animation/car/include/car_animation.hpp` | Formula 1 car polygon components and shared placement function |

The original Bresenham and other CP1/CP2 demonstrations remain available in
their checkpoint directories. The current OpenGL path uses triangles for the
visible track surface and car.

## Scope

This is a top-down 2D prototype inside an OpenGL window. The car and road are
simplified graphical components. Future work includes a 3D camera, elevation,
lighting, detailed car and scenery, telemetry input, wheel animation in the
live scene, and a selectable Bézier route. The lecturer's final requirements
will determine which of those become assessed features.
