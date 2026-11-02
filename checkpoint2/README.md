# Checkpoint 2 — Transformation and Animation

Checkpoint 2 builds on the Pure C++ graphical components from Checkpoint 1.
It demonstrates transformations and animation directly in C++20 before the
project moves to OpenGL integration.

## Implemented milestones

| Milestone | Main techniques | Evidence |
| --- | --- | --- |
| `transformations/` | Homogeneous matrices, translation, rotation, scaling, pivot rotation and composition | Five-panel transformation comparison |
| `animation/timeline/` | Frame clock, clamped/looping progress, position interpolation and shortest-arc angle interpolation | 13-frame motion sequence |
| `animation/wheels/` | Hierarchical transforms, local wheel pivots and rolling without slipping | 17 wheel-animation frames |
| `animation/car/` | Keyframes, whole-model translation, rotation, non-uniform scaling and combined transforms | 25 polygon-car frames |
| `animation/splines/` | Closed Catmull–Rom interpolation, analytic tangent and tangent-oriented car pose | 33-frame circuit lap |
| `animation/bezier/` | Chained cubic Bézier curves, analytic derivative and arc-length lookup | 25-frame approximately constant-speed lap |

Every milestone contains source code, deterministic tests, build instructions,
known limitations and generated graphical evidence.

## Complete verification

Run from the repository root:

```bash
checkpoint2/verify_all.sh
```

The script builds and runs every test and demonstration with strict C++20
warning flags. Generated verification output is written to a temporary
directory and removed automatically.

## Shared transformation model

The CP2 modules use `transformations/include/transform2d.hpp`, which provides:

- `Vec2`
- 3 × 3 `Mat3` homogeneous matrices
- translation, rotation and scaling constructors
- matrix composition
- fixed-pivot transformation
- polygon transformation

Animation modules convert a frame or path parameter into a pose and then apply
one shared matrix to the relevant object components.

## Evidence

The final evidence pack is available at
[`../output/pdf/Apex3D_Checkpoint2_Evidence.pdf`](../output/pdf/Apex3D_Checkpoint2_Evidence.pdf).

## Scope boundary

Checkpoint 2 remains Pure C++ and CPU rasterised. OpenGL integration, camera
control, perspective, lighting and the combined interactive prototype belong
to the later course checkpoints.
