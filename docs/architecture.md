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

CP2 Bezier circuit -> pixel-to-scene conversion -> sampled demo road

Active route + elapsed time -> lap progress -> path position and tangent
    -> CP2 Mat3 translation, rotation and optional scaling
    -> CP2 polygon-car model -> triangulated car mesh
    -> dynamic OpenGL triangle buffer

Accumulated route distance -> CP2 body and wheel poses -> side-view inset
Playback state -> in-window progress and controls -> dynamic HUD buffer
```

GLFW creates the window and handles keys. GLAD loads OpenGL functions. A vertex
shader fits the scene into the left area with an aspect correction and offset,
and passes per-vertex colour to a fragment shader. The track mesh is uploaded
once per route switch; the car and HUD meshes are updated as the lap progresses. The car is drawn after the track, and
the HUD is drawn last in clip coordinates on the right. `B` switches the route, road
and anchor buffer together and resets progress and accumulated wheel travel.
The existing CP2 Bézier evaluator, derivative and arc-length table are reused;
its original pixel coordinates are scaled and Y-flipped into scene units.
The live wheel inset reuses CP2 body translation and local wheel transforms,
with a repeating one-revolution side-view placement.

## Source map

| Path | Responsibility |
| --- | --- |
| `src/main.cpp` | Window, shaders, buffers, playback controls and frame loop |
| `include/apex3d/cp3_scene.hpp` | Monza points, path resampling, CP2 spline reuse, track mesh, CP2 car placement and triangulation |
| `include/apex3d/cp3_hud.hpp` | Text, progress and live CP2 side-view wheel hierarchy drawn with OpenGL triangles |
| `tests/cp3_scene_tests.cpp` | Closure, monotonic lap distance, car orientation and mesh checks |
| `checkpoint2/transformations/include/transform2d.hpp` | Shared 3 x 3 matrix and polygon transforms |
| `checkpoint2/animation/splines/include/spline_motion.hpp` | Catmull-Rom position and analytic tangent |
| `checkpoint2/animation/bezier/include/bezier_motion.hpp` | Bézier circuit, polynomial evaluation, derivatives and arc-length samples |
| `checkpoint2/animation/wheels/include/wheel_animation.hpp` | Rolling distance, parent body transform and wheel-pivot poses |
| `checkpoint2/animation/car/include/car_animation.hpp` | Formula 1 car polygon components and shared placement function |

The original Bresenham and other CP1/CP2 demonstrations remain available in
their checkpoint directories. The current OpenGL path uses triangles for the
visible track surface and car.

## Scope

This is a top-down 2D prototype inside an OpenGL window. The car and road are
simplified graphical components. Future work includes a 3D camera, elevation,
lighting, detailed car and scenery, telemetry input, 3D wheel geometry, and
additional routes. The lecturer's final requirements will determine which of
those become assessed features.
