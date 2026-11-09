# Checkpoint 3 progress

Current local milestone: an interactive 3D OpenGL visualization with a documented Monza 2025
race replay and the Monza Bézier teaching circuit.

| Item | Status | Evidence |
| --- | --- | --- |
| 3D rendering | Implemented locally | Perspective matrices, depth testing, solid car/scene meshes, normals, directional lighting and filtered sun shadows |
| 3D environment | Implemented locally | Raised track/kerbs, garages, pit wall, gantry, grandstands and trees |
| Cameras | Implemented locally | Overview/orbit, chase, cockpit, drag/zoom/reset and full-width view |
| CP2 curves and transformations | Integrated | Catmull-Rom / Bezier paths, tangent heading and shared Mat3 |
| Monza full race | Implemented locally | 53 FIA lap/sector triplets; exact 4,404.325-second finish |
| Timing dashboard | Implemented locally | Lap counter, race/current/last lap clocks, race progress |
| Pit lane and stop | Reconstructed | 24.545-second visit and 2.3-second stationary service; geometry/arrival interpolated |
| Starting lights | Implemented locally | Five double-red columns, lights-out starts race clock |
| Starting-light cues | Implemented locally | Embedded CC0 samples, muted initially, clickable mute and M; macOS output |
| Playback controls | Implemented locally | Click/Space start-pause-resume-replay, restart, 0.25x–64x, real-time reset, seek, next lap, pit seek |
| Wheel hierarchy | Integrated | Fixed body, metre-based distance / nominal 2025 tyre radius, computed rolling RPM and pause/service freeze |
| Bezier teaching route | Preserved | B switches route; demo scaling, anchors and optional looping remain available |
| Regression checks | Passing locally | CTest scene/race/3D checks, data/header verification, strict CP2 tests |
| Visual evidence and walkthrough | Prepared | OpenGL stage captures and `checkpoint3-demo-guide.md` |
| Cross-platform CI | Configured | Linux/macOS builds, CTest, data consistency and CP2 checks; new hosted run not claimed |
| Final rubric / group review / submission | Pending | Lecturer requirements and human acceptance still needed |

The detailed source trail and every reconstruction assumption are in
`monza-race-research.md`. Data-backed timings must not be described as a GPS,
continuous telemetry, vehicle-physics or full-field race simulation.

The course elevation remains planar; no surveyed elevation or continuous GPS
telemetry is provided. The car, scenery and pit crew are procedural low-poly
models. Lecturer-specific acceptance and any telemetry requirement remain
separate from local rendering and timing verification.

## Integration with the latest CP3 contributions

The 3D renderer retains the Monza Bézier route from #20, including its continuous arc-length parameter and regression checks. `B` selects this route as a teaching demo; historical race timing remains on the Catmull–Rom route. The original CP2 demo geometry remains available as `makeDemoBezierPath()` for regression checks. The wheel hierarchy and tests from #21 remain available, and the 3D front tyres reuse its curvature-based steering while rolling follows the historical race distance. The overview, chase and cockpit cameras replace the old 2D close-up. See [Bézier algorithm](algorithms/bezier-route.md).
