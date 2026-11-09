# Checkpoint 3 demonstration guide

Status: local 3D OpenGL race visualization; final lecturer rubric and formal
submission remain unconfirmed. Read `monza-race-research.md` before presenting.

## Before presenting

```bash
python3 tools/generate_race_assets.py --check
cmake --build build --parallel
ctest --test-dir build --output-on-failure
checkpoint2/verify_all.sh
./build/Apex3D
```

## Suggested walkthrough

1. Identify the selected race: Max Verstappen, Monza 2025, 53 laps in
   1:13:24.325. Explain that this is a full race timing replay, not 53 repeated
   qualifying laps. Lando Norris set that race's fastest lap, a separate record.
2. Show the 3D overview, then press `C` for chase and cockpit views. Drag to
   orbit, scroll to zoom, `F` to reset, and `H` for a full-width scene. Explain
   perspective projection, depth testing, normals, directional lighting and
   the shadow map.
3. Show the app ready at zero, muted. Optionally click UNMUTE, then START.
   Five red light columns illuminate and extinguish together. The race timer
   starts at lights-out. The 6.5-second pre-start is an illustrative sequence.
4. Show current lap, race time, lap time and last lap. At the first crossing,
   the last lap is 1:27.159. Pause, resume, change speed and press `1` to restore
   real time. `N` seeks to the next lap boundary, and arrows seek 10 seconds.
5. Press `J` to seek just before pit entry; resume if paused. Explain that the
   FIA labels the in-lap 37 while the tyre service happens across lap 38. The
   3D car joins the asphalt pit lane, stops at the marked box with crew figures
   for 2.3 seconds, then exits after a total pit visit of 24.545 seconds. Pit-box arrival and the
   drawn lane are reconstructed, not GPS or independently measured splits.
6. Press `W` to show the fixed-body wheel inspector. Rotation uses reconstructed
   travel in metres / nominal 2025 tyre radius. It stops on pause, at the service
   box and at the race finish. High wheel speeds or fast playback can alias spokes.
7. Use 64x to inspect the entire race, or capture the finish with the command
   below. The race stops at lap 53 of 53 and 1:13:24.325. Replay restarts lights.
8. Press `B` for the Monza Bézier teaching circuit. Show tangent heading,
   shared transformations, anchor points, optional scaling and demo looping.

## Evidence capture

```bash
./build/Apex3D --screenshot-start 5 lights.ppm
./build/Apex3D --screenshot-time 87.159 first_lap.ppm
./build/Apex3D --screenshot-time 3088.249 service.ppm --camera chase --wheels
./build/Apex3D --screenshot-time 4404.325 finish.ppm
./build/Apex3D --bezier --screenshot-at 0.25 bezier.ppm
```

Captures are deterministic, hidden and silent. Earlier CP3/race screenshots
show the previous 2D renderer. Current 3D evidence is stored separately as
`docs/evidence/apex3d-3d-views.png`. No video recording is included.

Each group member should explain their actual contribution, using the root
README responsibilities. Be ready to explain matrix order, tangent heading,
arc-length sampling, local wheel pivots, sector keyframes, and the limits of
interpolating motion from timing records.

Compare the lecturer's rubric with `checkpoint3-progress.md` before claiming
submission readiness or completed final-project 3D scope.
