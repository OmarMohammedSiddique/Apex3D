# Polygon Formula 1 car animation

**Checkpoint 2 — Apex3D**
**Author:** Hetvi

The implementation is in `checkpoint2/animation/car/`. It animates the polygon
Formula 1 car with the shared `apex3d::Mat3` engine in
`checkpoint2/transformations/include/transform2d.hpp`.

## Technique

Each frame has one pose (position `p`, angle `θ`, scale `sx, sy`) and one
matrix:

```text
M = T(p) · R(θ) · S(sx, sy) · T(-c)
```

`c` is the car centre in model space. Because `T(-c)` acts first, rotation and
scaling happen about the car centre, and `M · c = p`. All 13 polygon
components (chassis, front and rear wings, four tyres, four wheel stripes,
cockpit, helmet) go through this same `M`. The car therefore moves as one
object, and every polygon keeps its vertex count.

Poses come from keyframes with linear interpolation over 25 frames:

1. Translation of the complete car.
2. One full rotation about the car centre.
3. Non-uniform scaling to (1.6, 0.6) and back.
4. Combined translation and rotation.

Transformed polygons are filled with the Checkpoint 1 even-odd scanline
algorithm and outlined with Bresenham lines. Each frame is a binary PPM, and
nine representative frames form a PNG contact sheet.

## Verification

`checkpoint2/animation/car/tests.cpp` has 11 deterministic tests. They check
the first, middle and final poses against hand-computed vertices, the fixed
centre pivot, equivalence with `aroundPivot`, unchanged vertex counts, and that
every component in every frame receives the identical matrix.

See `checkpoint2/animation/car/README.md` for the full derivation, build
commands, frame legend and limitations.
