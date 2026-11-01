# Animation Timeline and Interpolation

**Author:** Azhar  
**Branch:** `feature/cp2-animation-timeline-azhar`  
**Module:** Apex3D Checkpoint 2 — Transformation and Animation

---

## Overview

This module implements a reusable, header-only animation timeline
(`timeline.hpp`) for Pure C++ 2D animation, together with a Formula 1 car
demonstration that drives 13 numbered PPM frames using the shared
`apex3d::Mat3` transformation engine.

---

## Files

| File | Purpose |
|------|---------|
| `timeline.hpp` | Core header: `Timeline`, `lerp`, `lerpAngle`, `PlaybackMode` |
| `main.cpp` | Demo: 13 PPM frames of F1 car moving start → finish |
| `contact_sheet.cpp` | Composites 5 representative frames into a PNG |
| `tests.cpp` | 23 deterministic unit tests |
| `build.bat` | Windows batch build and run script |
| `output/frame_NNN.ppm` | 13 numbered animation frames |
| `output/contact_sheet.png` | PNG contact sheet (first, 1/4, mid, 3/4, final) |

---

## Mathematics

### 1. Animation Timeline

The timeline maps a discrete **frame index** to a continuous normalised
progress value `t ∈ [0, 1]`:

```
elapsed(f)   = f / fps
progress(f)  = clamp(elapsed(f) / duration, 0, 1)   [Clamp mode]
progress(f)  = frac(elapsed(f) / duration)           [Loop mode]
```

The total number of frames is `ceil(fps × duration) + 1` so that both
`t = 0` (frame 0) and `t = 1` (last frame) are exactly representable.

### 2. Linear Interpolation (lerp)

```
lerp(A, B, t) = A + t × (B − A)
```

At `t = 0` the result is `A`; at `t = 1` the result is `B`.  The formula
gives a straight-line path through 2D space.

### 3. Angle Interpolation (shortest arc)

Naively interpolating `A + t(B − A)` for angles can rotate the long way
around the circle (e.g. 350° → 10° would travel 340° instead of 20°).
The shortest-arc algorithm:

```
delta = B − A
delta = fmod(delta, 360)
if delta > 180:  delta -= 360
if delta <= -180: delta += 360
result = A + t × delta
result = fmod(result, 360)   (normalise to [0, 360))
```

This guarantees the interpolation always uses the arc of length ≤ 180°.

### 4. F1 Car Placement

Each frame computes:

```
M = Translation(pos.x, pos.y) × Rotation(heading − 90°)
```

The model is defined with its nose pointing in the −Y direction (up in
screen coordinates), so subtracting 90° aligns it to a rightward heading
of 90°.

---

## Project Relevance

| Concept | Connection |
|---------|-----------|
| Timeline | Controls when each transformation is applied |
| `lerp` | Produces smooth straight-line car motion |
| `lerpAngle` | Ensures the car turns through the shorter arc |
| `apex3d::Mat3` | Shared engine — same matrix used by all members |
| PPM frames | Feed into contact sheets and future OpenGL playback |

---

## Build Commands

**Windows (batch script — recommended):**

```bat
cd checkpoint2\animation\timeline
build.bat
```

**Manual (from `checkpoint2/animation/timeline/`):**

```sh
# Tests
g++ -std=c++20 -Wall -Wextra -Wpedantic \
    tests.cpp -o timeline_tests

# Animation demo
g++ -std=c++20 -Wall -Wextra -Wpedantic \
    -I../../transformations/include \
    main.cpp -o timeline_demo

# Contact sheet
g++ -std=c++20 -Wall -Wextra -Wpedantic \
    contact_sheet.cpp -o contact_sheet
```

---

## Run Commands

```sh
./timeline_tests          # must print "All tests PASSED."
./timeline_demo           # writes frame_000.ppm .. frame_012.ppm to ./output/
./contact_sheet           # reads ./output/frame_*.ppm, writes contact_sheet.png
```

---

## Output

- **13 PPM frames** (`frame_000.ppm` … `frame_012.ppm`), each 900 × 540 px.
- **`contact_sheet.png`** — 4508 × 556 px composite showing frames 0, 3, 6, 9, 12.

---

## Test Coverage

| Test | Scenario |
|------|---------|
| `frame_zero: progress == 0` | Frame 0 has `t = 0` |
| `frame_zero: elapsed == 0` | Frame 0 has 0 s elapsed |
| `lerp_halfway: x==50` | Midpoint x coordinate |
| `lerp_halfway: y==100` | Midpoint y coordinate |
| `lerp_halfway: vec2 near` | Vec2 overload check |
| `final_frame: progress == 1` | Last frame has `t = 1` |
| `final_frame: elapsed == duration` | Last frame at full duration |
| `clamp_beyond_end` | Progress never exceeds 1 |
| `clamp_before_start` | Progress never goes below 0 |
| `loop_full_cycle_returns_zero` | One full loop → `t = 0` |
| `loop_half_cycle_at_t1.5` | Mid-second loop → `t = 0.5` |
| `loop_at_2_durations` | Two loops → `t = 0` |
| `angle_wrap_350_to_10_at_0.5 == 0` | Shortest arc across 0°/360° |
| `angle_0_to_180_at_0.5 == 90` | Simple 180° arc midpoint |
| `angle_90_to_270_at_0.5 == 180` | CW arc midpoint |
| `angle_at_t0_equals_start` | Boundary: `t = 0` |
| `angle_at_t1_equals_end` | Boundary: `t = 1` |
| `angle_270_to_90_at_0.5 == 0` | Short CCW arc midpoint |
| `invalid_fps_zero_throws` | Zero FPS rejected |
| `invalid_fps_negative_throws` | Negative FPS rejected |
| `invalid_duration_zero_throws` | Zero duration rejected |
| `invalid_duration_negative_throws` | Negative duration rejected |
| `total_frames_6fps_2s == 13` | Frame count formula |

---

## Known Limitations

- The contact sheet uses an uncompressed PNG encoder; files are larger than
  deflate-compressed PNG.
- The demo generates exactly `ceil(fps × duration) + 1` frames; a future
  version could accept an arbitrary frame count.
- Angle wrapping for very large `t` values (many loops) may accumulate small
  floating-point errors.
