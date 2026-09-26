# Apex3D

Interactive C++ and OpenGL application that transforms offline Formula 1 telemetry into a 3D circuit visualization with animated lap playback, telemetry heatmaps, and camera controls.

## Current milestone

The repository currently contains the rendering-pipeline foundation: a GLFW window, an OpenGL 3.3 Core context, GLAD function loading, vertex data, and simple vertex and fragment shaders. Running it displays a coloured triangle and proves that the development environment is working.

## Requirements

- A C++20 compiler
- CMake 3.24 or newer
- Git
- A graphics driver with OpenGL 3.3 support

CMake downloads a pinned GLFW release during the first configuration. The generated GLAD loader is stored in the repository, so no manual installation of either library is required.

## Build and run

Clone the repository, then run these commands from its root:

```bash
cmake -S . -B build
cmake --build build
./build/Apex3D
```

On Windows, the executable may be located under a configuration directory such as `build/Debug/Apex3D.exe`.

Press `Escape` to close the application.

## Learning goals

Apex3D will demonstrate:

- OpenGL rendering pipeline stages
- Geometric primitives and indexed polygons
- Bezier curves and track smoothing
- 2D and 3D transformations
- Perspective projection and camera controls
- Colour interpolation for telemetry layers
- Time-based path animation
- Depth testing and basic lighting

## Documentation

- [Architecture and rendering pipeline](docs/architecture.md)
- [Contribution workflow](CONTRIBUTING.md)

## Project status

Apex3D is under active development as an ICS 3204 Computer Graphics semester project.
