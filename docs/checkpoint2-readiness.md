# Apex3D Checkpoint 2 Readiness

**Status:** Technically complete and ready to package for submission.

Checkpoint 2 applies transformations and animation in Pure C++ to simplified
Formula 1 components developed during Checkpoint 1. OpenGL integration is
intentionally reserved for Checkpoint 3.

## Technical checklist

| Requirement | Status | Repository evidence |
| --- | --- | --- |
| 2D transformation engine | Complete | `checkpoint2/transformations/` implements translation, rotation, scaling, pivot rotation, composition and polygon transformation with 3 × 3 homogeneous matrices. |
| Animation timing and interpolation | Complete | `checkpoint2/animation/timeline/` implements clamped and looping progress, linear position interpolation and shortest-arc angle interpolation. |
| Hierarchical object animation | Complete | `checkpoint2/animation/wheels/` applies parent car transforms and local wheel rotation, including rolling without slipping. |
| Project-related polygon animation | Complete | `checkpoint2/animation/car/` animates a simplified Formula 1 car through translation, rotation, scaling and combined keyframes. |
| Spline path animation | Complete | `checkpoint2/animation/splines/` moves and orients a car around a closed Catmull–Rom circuit. |
| Bézier path animation | Complete | `checkpoint2/animation/bezier/` moves and orients a car along chained cubic Bézier curves using approximate arc-length sampling. |
| Pure C++ implementation | Complete | Every CP2 demonstration uses C++20 and handwritten CPU rasterisation without a graphics library. |
| Automated verification | Complete | `checkpoint2/verify_all.sh` builds all modules with strict warnings and runs all deterministic tests and demonstrations. |
| Graphical evidence | Complete | Each module contains generated PPM/PNG output; the combined submission evidence is in `output/pdf/Apex3D_Checkpoint2_Evidence.pdf`. |
| Repository documentation | Complete | The root README and `checkpoint2/README.md` describe responsibilities, techniques, build instructions, progress and limitations. |

## Group contribution map

| Member | Checkpoint contribution |
| --- | --- |
| Omar Siddique | Coordination, transformation engine, Bézier animation, integration, verification and evidence |
| Hetvi Barot | Polygon foundations and keyframed polygon-car animation |
| Azhar Ahmed | Line foundations and CP2 timeline verification, cleanup and documentation |
| Nyariki | Circle/ellipse foundations and hierarchical wheel animation |
| Casper Muhereza | Bézier foundations and initial timeline/interpolation implementation |
| Shadrack Mungai | Catmull–Rom spline path and tangent-oriented lap animation |

## Final submission actions

- [x] Merge all six CP2 technical workstreams.
- [x] Run the combined strict build and test script.
- [x] Document algorithms, commands, responsibilities and limitations.
- [x] Assemble a simple PDF evidence pack from the generated graphical output.
- [x] Merge the final documentation and evidence pull request.
- [x] Create the `cp2-submission` Git tag after that merge.
- [ ] Submit the repository and evidence through the lecturer's CP2 submission
  channel when it is provided or opened.

## Honest scope and limitations

- The CP2 outputs are 2D CPU-rendered demonstrations, not the final integrated
  OpenGL prototype.
- Catmull–Rom sampling is parameter-based rather than constant-speed.
- Bézier distance is estimated from a finite lookup table.
- Raster edges are not anti-aliased.
- Camera controls, lighting, telemetry and full interactive lap playback remain
  future-checkpoint work.

