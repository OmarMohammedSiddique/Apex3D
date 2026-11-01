# Checkpoint 1: Midpoint circles and ellipses

This standalone C++20 program rasterizes circle and ellipse outlines into a
1280 × 720 RGB24 canvas. The left partition is a shape test bench; the right
partition sketches Formula 1 equipment. Running the program from the repository
root writes `checkpoint1/circles/output/circles_ellipses_demo.ppm` as a binary
P6 PPM. Pixel coordinates have their origin at the top left, with positive
`y` pointing down. `Canvas::setPixel` clips every plotted point to the canvas.

## Contents

- [Implicit curves and midpoint tests](#implicit-curves-and-midpoint-tests)
- [Midpoint circle and eight-way symmetry](#midpoint-circle)
- [Midpoint ellipse and four-way symmetry](#midpoint-ellipse)
- [Scene and colour legend](#scene-and-colour-legend)
- [Relevance to Apex3D](#relevance-to-apex3d)
- [Build, run, convert, and prepare a commit](#build-run-convert-and-prepare-a-commit)
- [Known limitations](#known-limitations)

## Implicit curves and midpoint tests

A circle centered at $(x_c,y_c)$ with radius $r$ is described by

$$F_c(X,Y)=(X-x_c)^2+(Y-y_c)^2-r^2=0.$$

For an ellipse with horizontal radius $r_x$ and vertical radius $r_y$,

$$F_e(X,Y)=r_y^2(X-x_c)^2+r_x^2(Y-y_c)^2-r_x^2r_y^2=0.$$

In the derivations below, $x$ and $y$ are nonnegative offsets from the center.
A negative implicit value lies inside the curve; a positive value lies
outside. A midpoint test selects the closer of two neighboring raster pixels.
The implementation evaluates only integer decisions and integer increments;
it uses no square roots or trigonometric functions.

## Midpoint circle

Start at $(x,y)=(0,r)$ in the first octant and move toward $x=y$. At each
step choose **E** = $(x+1,y)$ or **SE** = $(x+1,y-1)$ by testing the midpoint
$(x+1,y-\tfrac12)$. Its first exact implicit value is

$$F_c(1,r-\tfrac12)=\tfrac54-r.$$

The conventional integer midpoint decision replaces the constant $5/4$ by
$1$, giving **$p_0=1-r$**. This quarter-unit bias removes fractions while
preserving the raster choices under the algorithm's tie convention. If
$p<0$, the midpoint is inside and E is chosen; otherwise SE is chosen.
Using the *old* offsets in each update:

| Choice | New point | New decision |
| --- | --- | --- |
| E, when $p<0$ | $(x+1,y)$ | $p+2x+3$ |
| SE, when $p\ge0$ | $(x+1,y-1)$ | $p+2(x-y)+5$ |

These follow by subtracting the implicit circle values at consecutive
midpoints. In `main.cpp`, `x` is incremented before updating `p`, so the same
formulas appear as `2*x+1` and, after decrementing `y`, `2*(x-y)+1`.
The loop includes $x=y$ and handles $r=0$ as a single center pixel.

### Eight-way symmetry

A circle is unchanged by sign changes and by exchanging $x$ and $y$.
`plotCircle8` therefore plots $(x_c\pm x,y_c\pm y)$ and
$(x_c\pm y,y_c\pm x)$ for each first-octant point. Repeated pixels on the
axes and diagonals are harmless. Only approximately one eighth of the
outline needs decision steps.

## Midpoint ellipse

Implicit differentiation in the first quadrant gives

$$\frac{dy}{dx}=-\frac{r_y^2x}{r_x^2y}.$$

The tangent reaches absolute slope $1$ when
$r_y^2x=r_x^2y$. Define $d_x=2r_y^2x$ and $d_y=2r_x^2y$.
**Region 1** has $d_x<d_y$, or $|dy/dx|<1$, and advances $x$ on every
step. **Region 2** has $d_x\ge d_y$, or $|dy/dx|\ge1$, and advances $y$
on every step. The transition criterion is explicitly
**$2r_y^2x\ge2r_x^2y$**. For screen coordinates, the first-quadrant
construction is reflected vertically during plotting.

### Region 1 decision

Start at $(0,r_y)$. Test $F_e(x+1,y-\tfrac12)$ to choose E or SE. Initially,

$$p_{1,0}=r_y^2-r_x^2r_y+\tfrac14r_x^2.$$

The code stores $D_1=4p_1$, so its initial value is
$4r_y^2-4r_x^2r_y+r_x^2$. This represents the quarter term exactly with
integers. After stepping, let $d_x'$ and $d_y'$ denote the derivative terms
at the **new** point:

| Decision | Step | Scaled decision update |
| --- | --- | --- |
| $D_1<0$ | E: $x\leftarrow x+1$ | $D_1\leftarrow D_1+4d_x'+4r_y^2$ |
| $D_1\ge0$ | SE: $x\leftarrow x+1$, $y\leftarrow y-1$ | $D_1\leftarrow D_1+4d_x'+4r_y^2-4d_y'$ |

These increments are the difference between successive values of
$4F_e(x+1,y-\tfrac12)$. Region 1 stops when $d_x\ge d_y$.

### Region 2 decision

At the transition point $(x,y)$, test the midpoint between **S** and
**SE**, namely $(x+\tfrac12,y-1)$:

$$p_{2,0}=r_y^2(x+\tfrac12)^2+r_x^2(y-1)^2-r_x^2r_y^2.$$

The integer initial decision is

$$D_2=4p_{2,0}=r_y^2(2x+1)^2+4r_x^2(y-1)^2-4r_x^2r_y^2.$$

If $D_2>0$, the midpoint lies outside and the algorithm chooses S;
otherwise it chooses SE. Again using derivative terms at the **new** point:

| Decision | Step | Scaled decision update |
| --- | --- | --- |
| $D_2>0$ | S: $y\leftarrow y-1$ | $D_2\leftarrow D_2+4r_x^2-4d_y'$ |
| $D_2\le0$ | SE: $x\leftarrow x+1$, $y\leftarrow y-1$ | $D_2\leftarrow D_2+4d_x'+4r_x^2-4d_y'$ |

The loop plots through $y=0$. Squared radii, derivative terms, and scaled
decisions use `std::int64_t` to avoid 32-bit overflow for the demo's radii.
Zero radii reduce to straight lines; negative radii are rejected.

### Four-way symmetry

An axis-aligned ellipse is symmetric about its two axes. Each computed
first-quadrant offset generates $(x_c\pm x,y_c\pm y)$. `plotEllipse4`
explicitly emits these four pixels for complete ellipses. Its upper-half
option emits the upper two, making the helmet and visor arcs without changing
the midpoint decisions.

## Scene and colour legend

The left test bench has concentric circles of radii 20, 50, and 80;
micro circles of radii 1 and 3; a $5\times2$ micro ellipse; a $160\times22$
wide ellipse; and a $28\times140$ tall ellipse. The right scene builds a
Formula 1 tyre, soft-compound stripe, rim, hub, and central lock nut from
circles. Ellipses draw the steering wheel rim, telemetry display pod, and
helmet visor arcs. Straight guide and spoke lines use integer line drawing.

| Name | Hex | Visual target |
| --- | --- | --- |
| Background | `#12141A` | Entire dark canvas |
| Grid | `#202733` | Motorsport-panel grid |
| Frame | `#334257` | Panel borders and guides |
| Cyan | `#3EDBF2` | Circle test, tall ellipse, display, visor |
| Yellow | `#FFD166` | Circle test, lock nut, steering control |
| Green | `#64D9A0` | Small circle, micro ellipse, telemetry indicator |
| Red | `#EF4444` | Soft tyre compound stripe and steering accents |
| Tyre silver | `#BEC6D1` | Tyre perimeter and wheel barrel |
| Rim blue | `#7792AA` | Wheel rim, spokes, steering details |
| White | `#F2F5FA` | Micro circle, hub, steering rim, helmet |
| Purple | `#A88BFA` | Wide ellipse and inner visor arc |

## Relevance to Apex3D

The same pixel-level construction can supply circular HUD overlays and
telemetry dials on top of the circuit view. Ellipses give a simple projective
approximation of 3D discs such as wheels viewed obliquely. Symmetric outlines
can also define viewport masks or clipping boundaries before a fill or stencil
pass. This checkpoint isolates raster decisions from the OpenGL application so
their geometry is easy to inspect.

## Build, run, convert, and prepare a commit

Run these Bash commands from the repository root. The first two are the exact
standalone compile and execution commands; compilation requires a C++20
compiler and no third-party libraries.

```bash
c++ -std=c++20 -Wall -Wextra -Wpedantic checkpoint1/circles/main.cpp -o /tmp/apex3d-circles
/tmp/apex3d-circles
```

The `/tmp/apex3d-circles` run command above is for Bash. On this Windows
machine, Cygwin maps `/tmp` to `C:\cygwin64\tmp`. From **PowerShell** in the
repository root, run the compiled program and open the PNG with:

```powershell
& 'C:\cygwin64\tmp\apex3d-circles.exe'
py -c "from PIL import Image; Image.open('checkpoint1/circles/output/circles_ellipses_demo.ppm').save('checkpoint1/circles/output/circles_ellipses_demo.png')"
Invoke-Item '.\checkpoint1\circles\output\circles_ellipses_demo.png'
```

Convert the generated PPM to a lossless PNG with **either** ffmpeg or
ImageMagick. PNG encoding is lossless; the input is already 8-bit RGB.

```bash
ffmpeg -y -i checkpoint1/circles/output/circles_ellipses_demo.ppm -frames:v 1 -pix_fmt rgb24 checkpoint1/circles/output/circles_ellipses_demo.png
# Alternatively:
magick checkpoint1/circles/output/circles_ellipses_demo.ppm -define png:color-type=2 checkpoint1/circles/output/circles_ellipses_demo.png
```

## Known limitations

- The outlines are aliased one-pixel wireframes. There is no sub-pixel
  filtering or antialiasing, so shallow curves show stair steps.
- The demo has no scanline fill. A filled tyre, visor, or viewport mask would
  need an additional fill or stencil stage.
- Coordinates are clipped at `setPixel`; very large input radii are not a
  practical workload, and intermediate 64-bit arithmetic is not checked for
  overflow beyond the intended canvas-scale shapes.
