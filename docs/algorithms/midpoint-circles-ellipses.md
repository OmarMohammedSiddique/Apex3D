# Midpoint Circle and Ellipse Rasterization

This document provides a concise theoretical and architectural overview of the midpoint circle and ellipse algorithms implemented in Apex3D.

---

## 1. Implementation Location

The primary C++20 implementation resides in:
- Source: [`checkpoint1/circles/main.cpp`](../../checkpoint1/circles/main.cpp)
  - `drawCircle`: Bresenham / Midpoint circle algorithm with integer-only arithmetic.
  - `plotCircle8`: Eight-way symmetry coordinate reflection helper.
  - `drawEllipse`: Midpoint ellipse algorithm handling Region 1 and Region 2 transitions.
  - `plotEllipse4`: Four-way symmetry reflection helper with optional upper-half arc clipping.
- Demo documentation & visual test bench: [`checkpoint1/circles/README.md`](../../checkpoint1/circles/README.md)

---

## 2. Midpoint Circle Decision Parameter

A circle centered at $(x_c, y_c)$ with radius $r$ is defined implicitly by:

$$F_c(x, y) = (x - x_c)^2 + (y - y_c)^2 - r^2 = 0$$

Rasterization starts at $(0, r)$ in the first octant ($0 \le x \le y$). At each step, $x$ increments by $1$, and the algorithm evaluates the midpoint between candidate pixels **E** $(x+1, y)$ and **SE** $(x+1, y-1)$:

$$M = \left(x + 1,\, y - \tfrac{1}{2}\right)$$

Evaluating the implicit function at the initial midpoint $M_0 = (1, r - 1/2)$:

$$F_c\left(1, r - \tfrac{1}{2}\right) = 1^2 + \left(r - \tfrac{1}{2}\right)^2 - r^2 = \tfrac{5}{4} - r$$

Using the conventional integer decision parameter $p_0 = 1 - r$ (substituting $1$ for $5/4$ to eliminate fractions while preserving exact integer tie-breaking):
- **If $p < 0$:** Midpoint is inside the boundary; select **E** $(x+1, y)$ and update:
  $$p \leftarrow p + 2x + 3$$
- **If $p \ge 0$:** Midpoint is outside or on the boundary; select **SE** $(x+1, y-1)$ and update:
  $$p \leftarrow p + 2(x - y) + 5$$

All decisions and increments use pure integer additions and bit-shifts, requiring no floating-point arithmetic, square roots, or trigonometric evaluations.

---

## 3. Eight-Way Symmetry

A circle centered at $(0,0)$ is symmetric across the horizontal axis ($y = 0$), vertical axis ($x = 0$), and both diagonal axes ($y = x$ and $y = -x$).

Consequently, determining points along one octant ($0 \le x \le y$, spanning an angle of $45^\circ$) yields all eight points across the entire perimeter:

| Quadrant | Octant 1 Points | Octant 2 Points |
| :--- | :--- | :--- |
| **I** | $(x_c + x,\, y_c + y)$ | $(x_c + y,\, y_c + x)$ |
| **II** | $(x_c - x,\, y_c + y)$ | $(x_c - y,\, y_c + x)$ |
| **III** | $(x_c - x,\, y_c - y)$ | $(x_c - y,\, y_c - x)$ |
| **IV** | $(x_c + x,\, y_c - y)$ | $(x_c + y,\, y_c - x)$ |

Implemented via `plotCircle8`, this property reduces raster decision computations to approximately $\frac{1}{8}$ of the total circle perimeter.

---

## 4. The Two Midpoint Ellipse Regions

An axis-aligned ellipse centered at $(x_c, y_c)$ with semi-major/minor radii $r_x$ and $r_y$ is defined by:

$$F_e(x, y) = r_y^2 (x - x_c)^2 + r_x^2 (y - y_c)^2 - r_x^2 r_y^2 = 0$$

Implicit differentiation yields the curve's slope:

$$\frac{dy}{dx} = -\frac{2r_y^2 x}{2r_x^2 y} = -\frac{d_x}{d_y}$$

Because the magnitude of the slope changes from flat to steep along the quadrant, the quadrant is divided into two regions separated by the transition boundary where $\left|\frac{dy}{dx}\right| = 1 \iff 2r_y^2 x = 2r_x^2 y$ ($d_x = d_y$):

```
       (0, ry)
          +---__
          |     --_   Region 1: |dy/dx| < 1  (dx < dy, advance x)
          |        \
          |         \ Transition: dx >= dy
          |          |
          |          | Region 2: |dy/dx| >= 1 (dx >= dy, advance y)
          +----------+ (rx, 0)
       (xc, yc)
```

### Region 1 ($\left|\frac{dy}{dx}\right| < 1 \iff d_x < d_y$)
- **Orientation**: Flatter curve; $x$ advances by $+1$ on every iteration.
- **Midpoint Tested**: $(x+1, y - 1/2)$ between **E** and **SE**.
- **Initial Decision**: $p_{1,0} = r_y^2 - r_x^2 r_y + \frac{1}{4}r_x^2$. To maintain exact integer precision without fractions, the implementation evaluates $D_1 = 4p_{1,0} = 4r_y^2 - 4r_x^2 r_y + r_x^2$.
- **Iteration**: Continues while $d_x < d_y$.

### Region 2 ($\left|\frac{dy}{dx}\right| \ge 1 \iff d_x \ge d_y$)
- **Orientation**: Steeper curve; $y$ advances downward by $-1$ on every iteration.
- **Midpoint Tested**: $(x + 1/2, y - 1)$ between **S** and **SE**.
- **Initial Decision**: Evaluated at the final Region 1 position $(x, y)$:
  $$D_2 = 4p_{2,0} = r_y^2 (2x + 1)^2 + 4r_x^2 (y - 1)^2 - 4r_x^2 r_y^2$$
- **Iteration**: Continues until $y < 0$.

All radius powers and decision updates use `std::int64_t` to guard against 32-bit integer overflow.

---

## 5. Four-Way Symmetry

Unlike circles, ellipses lack diagonal symmetry ($x \neq y$ invariance) and are only symmetric across their two orthogonal axes ($x = 0$ and $y = 0$).

Evaluating coordinates in the primary quadrant generates four symmetric pixels:

$$(x_c \pm x,\, y_c \pm y)$$

The helper function `plotEllipse4` reflects these four points. It also includes an `upperHalf` boolean parameter to plot only the upper two points $(x_c \pm x, y_c - y)$, allowing the generation of arcs (e.g., driver helmet curves and visor profiles) without duplicating decision logic.

---

## 6. Project Relevance

In the context of Apex3D (Formula 1 telemetry and circuit visualizer):
- **Cockpit & HUD Instrumentation**: Renders circular and elliptical instrument gauges, speedometers, RPM dials, and telemetry meters.
- **Projective 2D Approximations**: Models 3D wheels, rims, and steering wheels viewed at oblique angles prior to full 3D mesh rendering.
- **Visual Overlays & Masks**: Provides procedural circular and elliptical viewport boundaries, minimap frames, radar displays, and stencil masks.
- **Pipeline Isolation**: Validates fundamental rasterization logic independently in a CPU-rendered framebuffer before integrating with OpenGL fragment shaders.

---

## 7. Related Links

- Comprehensive Checkpoint 1 Guide: [`checkpoint1/circles/README.md`](../../checkpoint1/circles/README.md)
- Checkpoint 1 Source Implementation: [`checkpoint1/circles/main.cpp`](../../checkpoint1/circles/main.cpp)
- High-level Architecture: [`docs/architecture.md`](../architecture.md)

