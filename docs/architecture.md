# Apex3D Architecture

## Current milestone

The first milestone proves the basic OpenGL rendering pipeline:

1. GLFW creates a window and OpenGL context.
2. GLAD loads the OpenGL functions provided by the graphics driver.
3. C++ uploads vertex positions and colours to a vertex buffer.
4. A vertex shader positions the vertices and passes their colours forward.
5. A fragment shader calculates the final colour of each visible pixel.
6. OpenGL rasterizes the triangle and presents the completed frame.

## Planned modules

- **Application:** startup, main loop, input routing, and shared state.
- **Telemetry:** CSV loading, validation, normalization, and interpolation.
- **Geometry:** raw centerline points, Bezier smoothing, and track ribbon generation.
- **Rendering:** shaders, buffers, meshes, grid, vehicle marker, and debug views.
- **Camera:** orbit, pan, zoom, view matrix, and perspective projection.
- **Playback:** time control, scrubbing, marker position, and live statistics.
- **Interface:** telemetry layers, legend, playback controls, and errors.

These modules will be separated gradually. The first milestone intentionally keeps the rendering code together so every team member can trace the complete pipeline in one file.
