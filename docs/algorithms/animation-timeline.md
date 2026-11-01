# Animation Timeline and Interpolation

**Checkpoint 2 — Apex3D**  
**Author:** Azhar

---

## What is an Animation Timeline?

An animation timeline is a **clock abstraction** that converts a discrete
frame index into a normalised progress value `t ∈ [0, 1]`.  The rest of the
animation system only needs to ask: *"what fraction of the total animation has
elapsed?"* — the timeline answers that question regardless of frame rate or
duration.

---

## Timeline Formula

```
elapsed(frame)  = frame / fps                                 [seconds]
raw_t(elapsed)  = elapsed / duration                          [unbounded]

t_clamped = clamp(raw_t, 0, 1)
t_looping = raw_t − floor(raw_t)                             = frac(raw_t)
```

**Total frames** = `ceil(fps × duration) + 1`

The `+1` ensures that both endpoints (`t = 0` and `t = 1`) each have an
exact discrete frame.

---

## Linear Interpolation

```
lerp(A, B, t) = A + t × (B − A)
```

- `t = 0` → result is `A`
- `t = 1` → result is `B`
- `t = 0.5` → midpoint

For a 2-D position `Vec2`, both components are interpolated independently.

**Properties:**
- Constant velocity: the distance per unit `t` is constant.
- Straight-line path: intermediate points lie on the line segment `AB`.

---

## Shortest-Arc Angle Interpolation

Angles require special treatment because they wrap around at 360°.  Naively
interpolating `A + t(B − A)` can go the long way around the circle.

**Algorithm:**

```
delta = B − A
delta = fmod(delta, 360)          -- reduce to (−360, +360)
if delta >  180:  delta -= 360    -- prefer the shorter CW arc
if delta <= −180: delta += 360    -- prefer the shorter CCW arc
result = A + t × delta            -- interpolate along the short arc
result = fmod(result, 360)        -- normalise to [0, 360)
```

**Example:**  350° → 10°  
- `delta = 10 − 350 = −340`  
- after wrapping: `delta = −340 + 360 = +20`  
- at `t = 0.5`: `350 + 0.5 × 20 = 360 ≡ 0°` ✓

---

## Playback Modes

| Mode | Behaviour | Use case |
|------|-----------|---------|
| **Clamp** | `t` freezes at 0 or 1 beyond bounds | One-shot animations |
| **Loop** | `t` wraps back to 0 after 1 | Repeating cycles |

---

## Car Placement with the Shared Transformation Matrix

At each frame the car's world position is computed from the timeline:

```
pos   = lerp(start, end, t)
angle = lerpAngle(startAngle, endAngle, t)
M     = Translation(pos.x, pos.y) × Rotation(angle − 90°)
```

The subtraction of 90° compensates for the fact that the car model is
defined with its nose pointing in the −Y direction (upward in canvas
coordinates) while a heading angle of 90° means "facing right".

This is the standard **TRS (Translation × Rotation × Scale)** composition.
Since we read the matrix from left to right, the rotation is applied first
(in model space), then the translation moves the result to world space.

---

## Complexity

| Operation | Time | Space |
|-----------|------|-------|
| `progressAtFrame(f)` | O(1) | O(1) |
| `lerp(A, B, t)` | O(1) | O(1) |
| `lerpAngle(a, b, t)` | O(1) | O(1) |
| Frame loop (N frames) | O(N × pixels) | O(pixels) per frame |

---

## References

- Shoemake, K. (1985). *Animating rotation with quaternion curves.* SIGGRAPH.
  (Quaternion SLERP; the angle lerp here is the 1-D analogue.)
- Foley, van Dam, Feiner, Hughes. *Computer Graphics: Principles and Practice.*
  Section on keyframe animation and spline interpolation.
