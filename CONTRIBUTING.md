# Contributing to Apex3D

Apex3D is a group computer graphics project. Every contributor should make regular, understandable commits through their own GitHub account.

## Workflow

1. Pull the latest `main` branch.
2. Create a short-lived branch such as `feature/track-mesh` or `fix/shader-error`.
3. Keep each commit focused and give it a descriptive message.
4. Build and test the project locally.
5. Push the branch and open a pull request.
6. Ask at least one teammate to review the change before merging.

Do not commit directly to `main` once the initial project setup is complete.

## Commit examples

- `feat: render raw track centerline`
- `feat: parse telemetry csv data`
- `fix: close track mesh seam`
- `docs: explain bezier smoothing`
- `test: cover invalid telemetry ranges`

## Before opening a pull request

```bash
cmake -S . -B build
cmake --build build
```

Confirm that the application starts and that the terminal contains no shader or OpenGL errors.
