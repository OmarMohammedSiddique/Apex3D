# Peer-reviewed presentation

Run sheet for the in-person ICS 3204 project presentation. This one is live in the
room and marked mostly by classmates, so it differs from the
[semi-final showcase video](semi-final-showcase.md): shorter, more visual, plainer
language, and the app does most of the talking.

## What we know

- In person, at the usual venue. Sessions on Tuesday 17 November (from 8:45) and
  Thursday 19 November (from 12:30, STM B F2 05).
- 70% of the mark comes from peers: the score other groups give us plus how well
  we review them. 30% comes from the lecturer.
- Because our own feedback is marked, every member needs to review the other
  groups properly (see [Reviewing other groups](#reviewing-other-groups)).

## What we don't know yet

Ask at the start of the session if nobody has said:

1. How long each group has (this sheet assumes 10 minutes plus questions, with a
   5 minute and a 15 minute version below).
2. Whether every member must speak.
3. Which day we present, and whether the review form is on paper or online.

## The night before

- [ ] Pull `main`, build in Release, run the tests (commands below). Do it at home,
      not in the room.
- [ ] Launch `./build/Apex3D` once and click through the demo below.
- [ ] Charge the laptop. Pack the charger and a USB-C to HDMI adaptor.
- [ ] Open these in Preview, ready to show as a contact sheet:
      `checkpoint1/lines/output/lines_demo.png`,
      `checkpoint1/circles/output/circles_ellipses_demo.png`,
      `checkpoint1/bezier/output/bezier_demo.png`,
      `checkpoint2/transformations/output/transformations_demo.png`,
      `checkpoint2/animation/car/output/car_contact_sheet.png`,
      `checkpoint2/animation/splines/output/spline_contact_sheet.png`.
- [ ] Turn on Do Not Disturb. Close chats and email.
- [ ] Have the semi-final YouTube video and `docs/evidence/` open in a tab as the
      fallback if the projector or the app misbehaves.

```bash
git pull --ff-only
```

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel
```

```bash
ctest --test-dir build --output-on-failure
```

```bash
./build/Apex3D
```

The app runs entirely offline: all the race data is compiled in, so the venue Wi-Fi
does not matter.

## In the room

- Plug in and mirror the display before your slot starts. Check the HUD text is
  readable from the back row; if not, press `H` during the demo to hide it and
  describe instead.
- Start the app, press `Space` to start the race, set 4x with `+` twice, switch to
  TV view with `C`, and leave it running while the previous group finishes
  questions. The room sees a live race before you say a word.

## Running order (10 minutes)

| # | Part | Time | On screen |
| --- | --- | --- | --- |
| 1 | Hook | 0:30 | App running in TV view |
| 2 | What it is | 1:00 | Overview camera |
| 3 | How we got here: CP1, CP2, CP3 | 2:00 | Checkpoint images, then the app |
| 4 | Live demo | 5:00 | The app |
| 5 | Planned vs built, and what's next | 1:00 | The app paused |
| 6 | Close and questions | 0:30 | Overview |

**5 minute version:** parts 1, 2, demo steps 1 to 5, close.
**15 minute version:** add the hierarchical wheels (`W`), pit stop (`J`),
Bézier route (`B`) and let each member explain their own algorithm in part 3.

### 1. Hook (30 s)

> What you're watching is the 2025 Italian Grand Prix at Monza, all 20 cars, replayed
> from the real timing data. The track, the cars and the scenery are all built by
> code we wrote: no game engine and no downloaded 3D models. The only images we
> load are the team logos.

### 2. What it is (1 min)

Press `C` to the overview. Drag to orbit.

> Apex3D is a 3D race visualiser. The track shape comes from surveyed road edges,
> the hills come from Lombardy's terrain survey, and the cars move using recorded
> sector times and speed. We also paint the telemetry, like speed and braking,
> straight onto the road, so you can see where drivers brake and accelerate.

### 3. How we got here (2 min)

This maps directly onto the checkpoints the lecturer set, so say it out loud.
Peers marking against the brief will listen for it.

> **Checkpoint 1** was pure C++ with no graphics library. Each of us implemented one
> family of algorithms on our own pixel buffer: DDA and Bresenham lines, clipping,
> midpoint circles and ellipses, scanline polygon fill, Bézier and Catmull-Rom
> curves.

Show the CP1 images.

> **Checkpoint 2** added transformations and animation, still pure C++: matrix
> translation, rotation and scaling, a polygon car, wheels that spin in their own
> frame, and a car following a closed spline.

Show the CP2 contact sheets.

> **Checkpoint 3** moved to OpenGL. We didn't throw the old code away: the cars
> you're watching still follow the same Catmull-Rom spline code from Checkpoint 2,
> and the sidebar still uses the same 2D matrix engine.

### 4. Live demo (5 min)

| Step | Keys | Say |
| --- | --- | --- |
| 1 | `C` through the cameras | "Overview, chase, cockpit, TV. Each is a look-at and perspective matrix." |
| 2 | Click a driver in the timing tower | "The camera follows them. The buttons work through an inverse transform, the same 2D matrices from CP2." |
| 3 | `V` through the layers | "Speed, throttle, brake, pedals. Each value is turned into a colour by interpolation and written onto the track." |
| 4 | Click a different driver | "The road repaints with their data. You can see exactly where each driver brakes." |
| 5 | Zoom in on a car in chase view | "Lighting from the real sun position at race time, and shadows from a shadow map." |
| 6 | `W` | "Each wheel spins about its own axle and steers, then the car carries it: a hierarchical transform." |
| 7 | `N`, then drag the timeline | "Jump to the next lap boundary and read the lap time from the recorded timing. Then scrub anywhere in the race." |
| 8 | `H` | "And without the HUD, just the scene." |

Keep your hands off the keyboard while you explain something; let the room watch it.

### 5. Planned vs built, and what's next (1 min)

> We proposed a simple track ribbon with one wedge-shaped car. We ended up with the
> measured circuit, the full field and real scenery, because the open data was
> there. One thing we planned and haven't built is lap comparison: two drivers side
> by side with the time gap painted on the track. That, textures and a free camera
> are what we're working on for the final submission.

### 6. Close (30 s)

> That's Apex3D. The code and the documentation are on GitHub. We'd like your
> honest feedback, especially on what was hard to follow.

Saying "what was hard to follow" invites the specific feedback that helps us
improve before the final, and it signals confidence.

## Likely questions

| Question | Answer |
| --- | --- |
| Is this real data or made up? | Real: FIA timing, OpenF1 positions and telemetry, Regione Lombardia's survey. Where we had to reconstruct something, like the exact path between timing points, it's documented in `docs/monza-race-research.md`. |
| Did you use Unity, Three.js or a game engine? | No. C++20 with OpenGL 3.3 and GLFW for the window. We wrote the matrices, the meshes, the shaders and the camera code ourselves. |
| Was CP1 and CP2 really pure C++? | Yes. Those ran on our own CPU rasteriser writing pixels into a buffer, saved as PNG. |
| How do the cars know where to be? | The race clock gives each car's progress from its recorded sector times; the Catmull-Rom spline turns progress into a position, and its tangent gives the direction the car faces. |
| Why are the cars so detailed if the proposal said a wedge? | Once the scale was right (1 unit = 1 metre) a wedge looked wrong next to a 10 m road. The car is built from simple shapes in code, not a downloaded model. |
| What was the hardest part? | Pick one you can explain: the scale problem, shadow acne (fixed with a bias and a filter), or keeping the cameras on top of the hills by sampling the same height map as the GPU. |
| What doesn't work yet? | Lap comparison isn't built. Road and grass use vertex colours, not textures. No free pan camera. Be upfront: peers mark honesty well. |
| How did six people work on one codebase? | Each of us owned an algorithm family, worked on branches and merged through pull requests with tests. |

If you don't know an answer, say so and offer to show the code afterwards.

## If something goes wrong

- **App won't start or crashes:** play the semi-final video from the part that
  matches, or walk through `docs/evidence/` screenshots. Don't debug in front of
  the room.
- **Projector shows the wrong resolution:** press `H` to hide the HUD and narrate.
- **Running long:** skip demo steps 6 and 7 and go to part 5.

## Reviewing other groups

Our critiques are marked, and the stated aim is a well-rounded critique. For each
group, note at least one point under each heading, tied to something specific you
saw:

1. **Brief coverage:** did they show CP1 algorithms in pure C++, CP2
   transformations and animation, and CP3 OpenGL integration?
2. **Technique:** which graphics techniques were used, and were they explained
   correctly? Name them (for example "the rotation spun about the origin, not the
   object's centre").
3. **Working prototype:** did it run live? Was there interaction, animation?
4. **Presentation:** clear structure, timing, could the room see the screen?
5. **Strength:** the single best thing, stated specifically.
6. **Suggestion:** one concrete, achievable improvement before the final
   (week 13).

Avoid one-word scores with no reason ("good", "nice"), and avoid harshness
without a suggestion. Score on what was shown, not on how polished the slides
were.
