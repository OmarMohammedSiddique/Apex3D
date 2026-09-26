# Apex3D Architecture

## Current milestone

The Monza centerline milestone extends the basic OpenGL rendering pipeline:

1. GLFW creates a window and OpenGL context.
2. GLAD loads the OpenGL functions provided by the graphics driver.
3. C++ uploads an ordered set of two-dimensional track points to a vertex buffer.
4. A vertex shader corrects the coordinates for the current viewport aspect ratio.
5. `GL_LINE_LOOP` connects consecutive points and closes the circuit.
6. A second draw call displays the same vertices as red control points.
7. A fragment shader applies a uniform colour to each draw call.
8. OpenGL rasterizes the primitives and presents the completed frame.

The current coordinates are intentionally hand-authored. They isolate the graphics pipeline from telemetry parsing so the team can verify one subsystem at a time.

## Planned modules

- **Application:** startup, main loop, input routing, and shared state.
- **Telemetry:** CSV loading, validation, normalization, and interpolation.
- **Geometry:** raw centerline points, Bezier smoothing, and track ribbon generation.
- **Rendering:** shaders, buffers, meshes, grid, vehicle marker, and debug views.
- **Camera:** orbit, pan, zoom, view matrix, and perspective projection.
- **Playback:** time control, scrubbing, marker position, and live statistics.
- **Interface:** telemetry layers, legend, playback controls, and errors.

These modules will be separated gradually. The early milestones intentionally keep the rendering code together so every team member can trace the complete pipeline in one file.
