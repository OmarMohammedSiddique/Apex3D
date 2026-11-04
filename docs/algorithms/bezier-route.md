# CP3 Monza Bézier Route

## Overview

Issue #18 integrates the CP2 cubic Bézier technique as a second selectable lap
mode in the OpenGL prototype. Press **B** at runtime to toggle between the
existing Catmull-Rom route (cyan HUD) and the new Bézier route (orange HUD).

## Algorithm

### 1 — Closed C1 Bézier chain from Monza anchors

The 62 hand-placed Monza centreline anchors (the same array used by the
Catmull-Rom route) are converted into a closed chain of cubic Bézier segments.
Each segment i uses the same handle formula that CP2 `bezier_motion.hpp` uses
for its generic circuit:

```
h_start = anchor[i]   + (anchor[i+1] - anchor[i-1]) / 6
h_end   = anchor[i+1] - (anchor[i+2] - anchor[i]  ) / 6
```

This guarantees **C1 continuity** at every join: the outgoing tangent of
segment i equals the incoming tangent of segment i+1, so the car heading
changes smoothly with no kinks.

### 2 — Arc-length parameterisation

Each segment is sampled at 120 sub-steps to build a cumulative arc-length
table. Given a normalised lap progress `p` in [0, 1], the driver finds the
bracketing table entries via binary search and linearly interpolates the
Bézier parameter `t`. This ensures the car travels at approximately constant
speed regardless of how densely the anchors are placed.

### 3 — PathSample extraction

For each sample, the tangent vector is obtained from the analytic Bézier
derivative, normalised, and rotated 90° to give the track-normal used by the
existing ribbon and kerb renderer. The resulting `std::vector<PathSample>` is
identical in structure to the Catmull-Rom path, so `makeTrackMesh`,
`poseAtProgress`, and all car-placement code are route-agnostic.

### 4 — Route switching

`makePathForMode(RouteMode)` returns either path. In `main.cpp` the render
loop detects when `playback.routeMode` changes, calls `makePathForMode`, and
updates the track mesh in the same frame. The car immediately follows the new
path with no discontinuity in progress.

## Closure and continuity guarantees

| Property | Check | Test |
| --- | --- | --- |
| Closed loop | `circuit[i].p3 == circuit[(i+1)%n].p0` | cp3_scene test 2 |
| C1 continuity | `d/dt curve_i(1) == d/dt curve_{i+1}(0)` | cp3_scene test 3 |
| Monotone arc table | `arcTable[i].dist >= arcTable[i-1].dist` | cp3_scene test 4 |
| Closed path | `path.front.point == path.back.point` | cp3_scene test 5 |
| Finite poses | all `position` and `headingRadians` are finite | cp3_scene test 7 |
| No shortcuts | all points in `[-2,2] x [-1.2,1.2]` | cp3_scene test 10 |

## Visual evidence

`docs/evidence/cp3-bezier-route.png` — real OpenGL screenshot of the Bézier
lap at 25% progress, captured from `Apex3D --bezier --screenshot-at 0.25`.
