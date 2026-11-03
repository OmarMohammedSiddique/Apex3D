# Contributing to Apex3D

Apex3D is a group computer graphics project. Every contributor should make regular, understandable commits through their own GitHub account.

## Workflow

1. Pull the latest `main` branch.
2. Create a short-lived branch such as `feature/track-mesh` or `fix/shader-error`.
3. Keep each commit focused and use a descriptive Gitmoji-prefixed message.
4. Build and test the project locally.
5. Push the branch and open a pull request.
6. Ask at least one teammate to review the change before merging.

Do not commit directly to `main` once the initial project setup is complete.

## Commit examples

- `✨ feat: render the OpenGL track surface`
- `🐛 fix: close the track mesh seam`
- `📝 docs: explain spline motion`
- `✅ test: cover invalid lap progress`

Do not add AI attribution, `Co-Authored-By` trailers, Claude attribution,
Codex attribution, or automated-signature lines to any commit message.
Commits must list only the human Git author configured on the contributor's
machine.

## Before opening a pull request

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

Confirm that the application starts and that the terminal contains no shader or OpenGL errors.
