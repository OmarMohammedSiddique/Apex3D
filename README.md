# Apex3D

Apex3D is an ICS 3204 Computer Graphics semester project that turns Formula 1
circuit geometry into a progressively richer visualisation. The current Pure
C++20 implementation demonstrates the underlying line, polygon, circle, curve,
transformation and animation algorithms directly. OpenGL integration belongs
to a later checkpoint.

## Current milestone: Checkpoint 2

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
| Omar Siddique | Project coordination, transformation engine, CP2 Bézier animation, integration, verification and evidence |
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
- CMake, GLFW, GLAD and OpenGL for the separate prototype foundation
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

Each milestone also has its own README with individual build and run commands:

- [Checkpoint 2 overview](checkpoint2/README.md)
- [Transformations](checkpoint2/transformations/README.md)
- [Timeline and interpolation](checkpoint2/animation/timeline/README.md)
- [Wheel animation](checkpoint2/animation/wheels/README.md)
- [Polygon-car animation](checkpoint2/animation/car/README.md)
- [Catmull–Rom motion](checkpoint2/animation/splines/README.md)
- [Bézier motion](checkpoint2/animation/bezier/README.md)

## OpenGL prototype

The separate prototype can be configured with CMake:

```bash
cmake -S . -B build
cmake --build build
./build/Apex3D
```

On Windows, the executable may be under a configuration directory such as
`build/Debug/Apex3D.exe`. Press `P` to show or hide raw control points and
`Escape` to close the application.

## Evidence and documentation

- [CP2 readiness checklist](docs/checkpoint2-readiness.md)
- [CP2 evidence PDF](docs/evidence/Apex3D_Checkpoint2_Evidence.pdf)
- [Architecture and rendering pipeline](docs/architecture.md)
- [Contribution workflow](CONTRIBUTING.md)
- [Algorithm notes](docs/algorithms/)

## Current limitations

- The CP2 demonstrations are 2D CPU-rendered evidence programs, not the final
  integrated OpenGL application.
- Circuit coordinates and cars are simplified rather than telemetry-derived.
- Raster edges are not anti-aliased.
- Bézier arc length is approximated numerically and Catmull–Rom sampling is not
  constant-speed.
- Camera control, perspective, lighting, telemetry heatmaps and interactive lap
  playback remain later-checkpoint work.

## Reference

The circuit work uses the official Formula 1 Monza map as a shape reference:
[Italy — Autodromo Nazionale Monza](https://www.formula1.com/en/information/italy-autodromo-nazionale-monza.FiJN1jnQlRLeHqOxIt13m).
