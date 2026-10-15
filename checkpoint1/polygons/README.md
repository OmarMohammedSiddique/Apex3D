# Checkpoint 1: Polygon Filling and Clipping (Pure C++)

This is a standalone C++20 program. It draws polygons into an RGB pixel buffer in memory and saves the result as a PPM image. It does not use OpenGL, GLFW, SFML, SDL, OpenCV or any drawing library. Every pixel comes from the algorithms in [`main.cpp`](main.cpp).

> The output image is [`output/polygons_demo.ppm`](output/polygons_demo.ppm). GitHub and most Markdown viewers cannot display PPM files, so open it `output/polygons_demo.ppm` in GIMP, IrfanView, Photoshop, macOS Preview or VS Code with a PPM extension.

## What the program does

1. Creates a 900 × 520 RGB `Canvas` in memory.
2. Writes pixels with `Canvas::setPixel`, which ignores any coordinate outside the canvas.
3. Draws polygon edges with a hand-written Bresenham line rasteriser (`drawLine`).
4. Fills polygons with an even-odd scanline algorithm (`scanlineFill`).
5. Clips a concave polygon against an axis-aligned rectangle using Sutherland–Hodgman (`sutherlandHodgmanClip`).
6. Draws a simplified top-down Formula 1 car on a section of Monza track, built entirely from filled polygons.
7. Saves the canvas to `output/polygons_demo.ppm`.
8. Prints the vertices and vertex counts of the original and clipped polygons, then checks that every clipped vertex lies inside the clipping window.

## Polygon representation

```cpp
struct Point { double x; double y; };   // screen space, y grows downwards
using Polygon = std::vector<Point>;     // vertices in drawing order
```

A polygon is an ordered list of vertices. Its edges are not stored separately: edge *i* runs from `polygon[i]` to `polygon[(i + 1) % n]`, so the last vertex always connects back to the first. Coordinates are stored as `double` because clipping creates new vertices at non-integer positions. They are rounded to whole pixels only when lines are rasterised.

The clipping window is a `ClipRect { xMin, yMin, xMax, yMax }`.

## Scanline filling

`scanlineFill` processes one pixel row at a time, from the polygon's lowest y to its highest y:

1. **Sample at the row centre.** Row `y` is tested at `scanY = y + 0.5`. Sampling at the centre means a scanline never passes exactly through an integer vertex.
2. **Find edge intersections.** An edge from `a` to `b` crosses the scanline only if `scanY` lies in the half-open range `[min(a.y, b.y), max(a.y, b.y))`. When it does, the crossing point is
   `x = a.x + (scanY − a.y) · (b.x − a.x) / (b.y − a.y)`,
   where `(b.x − a.x) / (b.y − a.y)` is the inverse slope: how far x moves for each step in y. The half-open range means a vertex shared by two edges is counted only once. Horizontal edges are skipped automatically, since they never cross a scanline.
3. **Sort the intersections** from left to right.
4. **Fill the spans in pairs** (even-odd rule). The program fills from the 1st crossing to the 2nd, from the 3rd to the 4th, and so on. A pixel is filled when its centre `x + 0.5` lies in `[left, right)`. Spans are trimmed to the canvas width.

This method handles concave polygons, such as the clipped star and the car chassis, as well as convex ones.

## Sutherland–Hodgman clipping

The window is treated as four infinite lines: left, right, top and bottom. The polygon is clipped against **one boundary at a time**, and the output of each pass becomes the input of the next:

```
subject → clip(Left) → clip(Right) → clip(Top) → clip(Bottom) → result
```

Within one pass (`clipAgainstBoundary`), the program walks every edge `previous → current` and applies four cases:

| previous | current | output                          |
|----------|---------|---------------------------------|
| inside   | inside  | `current`                       |
| inside   | outside | intersection                    |
| outside  | inside  | intersection, then `current`    |
| outside  | outside | nothing                         |

`intersectBoundary` finds where the edge crosses the boundary by solving the parametric form `P(t) = a + t(b − a)`. For a vertical boundary `x = X`, the parameter is `t = (X − a.x) / (b.x − a.x)`. For a horizontal boundary `y = Y`, it is `t = (Y − a.y) / (b.y − a.y)`.

Sample terminal output:

```
Original polygon (8 vertices): (60, 210) (240, 60) (300, 180) (440, 150) (330, 270) (450, 440) (220, 330) (130, 470)
Clipped polygon  (13 vertices): (110, 390) (110, 168.333) (168, 120) (270, 120) (300, 180) (380, 162.857) (380, 215.455) (330, 270) (380, 340.833) (380, 390) (345.455, 390) (220, 330) (181.429, 390)
Clipping window: x [110, 380], y [120, 390]
All clipped vertices inside clipping window: yes
```

The original polygon crosses all four window edges, so clipping produces more vertices (8 → 13) even though the area shrinks.

## Connection to Apex3D

Apex3D will render the Monza circuit and a Formula 1 car. The right-hand panel uses the same polygon pipeline on that subject matter:

- **Track section:** grass verges, an asphalt strip and alternating red and white kerb blocks. Each is a filled polygon, and the white track-limit lines are Bresenham lines.
- **Car (top-down):** the chassis is a single 12-vertex concave polygon covering the nose cone, sidepods and engine cover. The front and rear wings, endplates, tyres, compound stripes, cockpit and helmet are separate filled polygons. Right-side parts are produced by mirroring the left-side polygons about the car's centreline (`mirrorX`).

The clipping panel shows the operation a renderer needs to keep geometry inside a viewport. In the 3D application, the same idea extends to clipping triangles against the view frustum.

## Build and run

Run these commands from the repository root:

```bash
c++ -std=c++20 -Wall -Wextra -Wpedantic checkpoint1/polygons/main.cpp -o /tmp/apex3d-polygons
/tmp/apex3d-polygons
```

On Windows with MinGW-w64 (GCC 10 or newer):

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic checkpoint1/polygons/main.cpp -o apex3d-polygons.exe
./apex3d-polygons.exe
```

By default, the image is written to `checkpoint1/polygons/output/polygons_demo.ppm`. That path is the `output/` folder next to `main.cpp`, based on the path given to the compiler. To write somewhere else, pass a path as the first argument, for example `./apex3d-polygons out.ppm`. The program exits with a non-zero status if the file cannot be written or a clipped vertex falls outside the window.

## Colour legend

| Colour | RGB | Meaning |
|---|---|---|
| Red outline | (255, 70, 70) | Original, unclipped polygon |
| Yellow outline | (255, 215, 0) | Clipping window boundary |
| Bright green outline | (90, 255, 140) | Clipped polygon edges |
| Dark green fill | (30, 120, 70) | Clipped polygon, scanline-filled |
| Red fill with white outline | (215, 25, 32) | F1 car chassis (filled concave polygon) |
| Black / dark grey fills | (12–28, …) | Tyres, wings, cockpit |
| Yellow fills | (250, 210, 40), (240, 200, 30) | Helmet and tyre compound stripes |
| Grey fill | (72, 72, 80) | Track asphalt |
| Red / white blocks | (200, 30, 35) / (235, 235, 235) | Kerbs |
| Green fill | (38, 110, 52) | Grass verge |

In the clipping panel, the clipped polygon's edges that lie on the window boundary are covered by the yellow window outline. Its interior edges appear in bright green where they follow the original polygon.

## Known limitations

- **Concave clipping artefacts:** Sutherland–Hodgman always returns one polygon. If a concave polygon is clipped into several separate pieces, they are joined by zero-area edges along the window boundary instead of being split. The demo polygon does not trigger this case.
- **Rectangular window only:** the clipping window must be axis-aligned. Clipping against an arbitrary polygon would need Weiler–Atherton or a general convex-window version.
- **Even-odd fill rule only:** self-intersecting polygons get holes where they overlap. The non-zero winding rule is not implemented.
- **No anti-aliasing:** edges are 1-pixel Bresenham lines and fills use hard pixel-centre sampling, so diagonal edges look stepped.
- **Simple scanline implementation:** each row tests every edge rather than keeping a sorted edge table and active edge list. This is fine for these small polygons but slower for large meshes.
- **No text rendering:** the image has no on-image labels. Use the legend above.
- **PPM output only:** the image is uncompressed and needs a PPM-capable viewer.
- **Not connected to the OpenGL app:** this demo is a separate Checkpoint 1 program and is not part of the CMake build.
