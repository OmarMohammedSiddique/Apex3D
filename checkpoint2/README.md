# Checkpoint 2 - Transformation and Animation

Checkpoint 2 builds on the Pure C++ graphical components from Checkpoint 1.
The current foundation implements 2D homogeneous transformations and applies
them to the polygon-based Formula 1 car. Timed animation and curve-following
motion are the next milestones.

## Current progress

- 3 x 3 homogeneous transformation matrices
- Translation, rotation and non-uniform scaling
- Rotation around a selected pivot
- Matrix composition
- Polygon transformation
- Formula 1 transformation evidence image
- Deterministic transformation tests

See [`transformations/README.md`](transformations/README.md) for build commands,
mathematics, output interpretation and limitations.

## Planned animation milestone

The next Pure C++ program will evaluate a car pose over time, move it along a
Bezier or Catmull-Rom path, orient it from the path tangent and export numbered
PPM frames. OpenGL integration remains a Checkpoint 3 task.
