# Checkpoint 3 demonstration guide

Status: integration walkthrough prepared; lecturer rubric and final submission
requirements remain unconfirmed. This is a 2D OpenGL prototype.

## Before presenting

Run `cmake --build build --parallel`,
`ctest --test-dir build --output-on-failure`, and `checkpoint2/verify_all.sh`.
Launch `./build/Apex3D`. Check both routes for a complete lap and confirm that
the car follows the road, faces forward and crosses the start line cleanly.

## Suggested two-minute walkthrough

1. Show Monza playing. Explain that CP2 Catmull–Rom position and analytic
   tangent orient the polygon car. Distance sampling approximates uniform
   travel; the animation is not telemetry or a vehicle physics simulation.
2. Press `Space`. Point out that the car, progress and wheel inset freeze.
   Press `P` to display the route anchors. Explain how a 3 x 3 matrix combines
   translation, rotation and optional scaling of all car polygons.
3. Resume with `Space`, then use `+` / `-` and `T` to demonstrate playback
   speed and scaling. Disable scaling afterward. `R` resets and resumes.
4. Show the wheel inset. Explain the parent body transform and rotation about
   each wheel mount, using the same CP2 wheel-pose functions. Rolling uses
   distance = radius x angle. The side-view placement repeats each revolution;
   the top-down tyres are not represented as rotating circles.
5. Press `B`. Identify the separate CP2 Bézier demo circuit, its matching road,
   and its eight anchors (`P`). Explain cubic Bézier evaluation, derivatives
   and approximate arc-length lookup. Switching resets the lap and wheel
   travel but preserves pause/play; both routes take 18 seconds at 1x.
6. Toggle the wheel inset with `W`, switch back with `B`, and finish with the
   known limitations: top-down 2D, hand-authored geometry, no telemetry,
   perspective, lighting or 3D wheel model yet.

## Evidence capture

```bash
./build/Apex3D --screenshot-at 0.25 monza.ppm
./build/Apex3D --screenshot-at 0.25 bezier.ppm --bezier
```

The checked-in PNGs are real OpenGL frames:

- `docs/evidence/cp3-opengl-prototype.png`
- `docs/evidence/cp3-bezier-wheels.png`

For a short recording, follow steps 1–5 and speak to the visible controls.
A video recording is not included in this evidence pack.

## Group explanation preparation

Each member should explain the algorithm and source they actually contributed,
then show how the integration uses it. Use the responsibilities in the root
README as the starting point; do not imply everyone implemented the OpenGL
integration. Be prepared to show the transformation matrix order, tangent
heading, distance table, triangle rendering, and local wheel pivot.

## Final submission gate

Compare the official rubric with `docs/checkpoint3-progress.md` once available.
Complete any required evidence, teammate review and submission steps before
calling CP3 submitted. Do not present this walkthrough as final-project 3D
completion.
