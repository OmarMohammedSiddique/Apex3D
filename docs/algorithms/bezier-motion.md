# Bézier motion with arc-length sampling

## Purpose

Checkpoint 2 extends the Checkpoint 1 cubic Bézier implementation from static
curve rendering into a reusable animation path. The Formula 1 car must move
along the curve and rotate to match its tangent without relying on OpenGL or an
animation library.

## Curve and derivative

The position is evaluated with the cubic Bernstein basis:

```text
B(t) = (1−t)³P0 + 3(1−t)²tP1 + 3(1−t)t²P2 + t³P3
```

The analytic derivative supplies the heading:

```text
B′(t) = 3(1−t)²(P1−P0)
      + 6(1−t)t(P2−P1)
      + 3t²(P3−P2)
```

The circuit contains eight cubic segments. Adjacent segments share endpoints
and matching first derivatives, giving C1 continuity around the complete loop.

## Approximate arc-length parameterisation

Uniform parameter steps can make the car accelerate on straight sections and
slow down around tightly curved regions. Apex3D builds a lookup table using 160
samples per segment:

```text
sample[i] = (cumulativeDistance, globalCurveParameter)
```

For progress `p`, binary search locates `p × totalLength` in this table and
linearly interpolates the surrounding curve parameters. The result is an
efficient approximation of constant-distance animation.

Construction costs `O(N)` for `N` lookup samples. Each frame lookup costs
`O(log N)`, while curve evaluation and matrix construction are `O(1)`.

## Pose transformation

The curve point becomes the car centre. The derivative angle rotates the local
car model before translation:

```text
θ = atan2(B′y, B′x)
M = T(Bx, By) × R(θ)
```

Because the local car faces positive X, no corrective model-space rotation is
needed. Every body and wing vertex is transformed by the same `Mat3`.

## Evidence and limitations

The demo renders 25 PPM frames and a 12-panel contact sheet. Deterministic tests
cover curve endpoints, continuity, lap closure, travel uniformity, matrix
placement and invalid inputs. The lookup table remains an approximation, the
track is simplified, and the CPU raster output is not anti-aliased.
