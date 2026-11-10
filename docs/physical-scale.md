# Physical scale of the 3D scene

The 3D renderer uses **one world unit per metre**. The Monza centreline is
measured from recorded car positions ([measured track geometry](real-track-geometry.md))
and kept in 2D scene units; `physical_scale.hpp` calibrates its sampled length to
5,793 m before converting positions into the X/Z plane. Every 3D object, camera,
light frustum and shadow calculation then uses metres. Multiplying every old
object by the same factor would have preserved their exaggerated proportions;
the car, road, pit facilities and scenery have instead been dimensioned separately.

## Sourced dimensions and representative choices

| Element | Rendered dimension | Basis |
| --- | --- | --- |
| Monza centreline | 5,793 m per lap | [Circuit operator](https://www.monzanet.it/en/circuit/) |
| Racing surface | 12 m wide | Operator publishes 10–12 m; uniform 12 m chosen because no surveyed width profile is supplied |
| Car body/wing envelope | 2 m wide, 3.6 m wheelbase | Representative 2025 envelope using FIA Article 3.4 limits; these are not measured RB21 dimensions |
| Slick tyres | 720 mm diameter; 305 mm front / 405 mm rear nominal width | FIA 2025–27 tyre technical targets |
| Rims | 18 inch diameter | FIA tyre specification |
| Procedural car length | 5.652 m | Representative modelling choice; not a claimed regulated maximum or exact RB21 length |

The car width/wheelbase references are [FIA 2025 Technical Regulations, Article 3.4](https://www.fia.com/sites/default/files/fia_2025_formula_1_technical_regulations_-_issue_01_-_2024-12-11_1.pdf).
The tyre references are [FIA 2025–27 tyre targets, nominal dimensions](https://legal.fia.com/web/appeloffre.nsf/0F43EC70A0FACADFC1258991006043A9/$FILE/ITT%20F1%20TYRES%202025-2027%20(including%20F2%20and%20F3)_UPDATE%2014.04.2023.pdf?openelement=).
These are 2025 dimensions, matching the race being replayed.

## Estimated scenery and pit dimensions

These are plausible dimensions for this reconstruction, **not measured Monza assets**:

| Element | Modelling choice |
| --- | --- |
| Road surface height above ground | 0.10 m; a rendering datum, not circuit elevation |
| Kerbs | 0.80 m wide, top 0.16 m above ground |
| Pit fast lane | 4 m wide, centre about 18 m from the main centreline |
| Service bay | Centre another 3.5 m towards the garages; 7 × 3 m painted box |
| Garage block | Six 6 m bays in each simplified team block, 15 m building depth, 6 m height |
| Armco | 0.75 m high, about 5 m beyond kerbs |
| Grandstand | 60 m seating length, 0.6 m seat spacing, 7 m canopy height |
| Starting gantry | 15 m clear span, 6.5 m height |
| Trees | 10–18 m height, 0.4 m diameter trunks |
| Mechanics | 1.15 m in a crouched service posture |

The fast lane stays straight past the service apron. The car follows a separate
smooth lateral move into its bay. Pit timing is unchanged. Wheel rotation and
RPM follow the distance along that reconstructed path, including lateral travel,
and freeze during service. The visible tyre radius matches the rolling radius.

## Viewing and accuracy limits

Chase is the default camera because a 2 m wide car is very small in a view of a
5.793 km circuit. `C` switches to cockpit and whole-track overview; `F` resets the
camera and scrolling zooms it. Camera position, near/far planes and local shadow
coverage are all expressed in metres. The chase camera follows translation
immediately, with smoothing applied to the viewing offset, so high replay rates
do not leave the car behind. The optional wheel inspector is a labelled diagram;
it is not another object in the physical scene. Bézier scaling with `T` remains
an explicit teaching demonstration and intentionally changes the model's size.

This provides consistent physical proportions, not a surveyed digital twin.
The route's shape comes from averaged OpenF1 car positions and matches the
OSM-mapped circuit within a few metres. Its length is calibrated to 5,793 m (a 0.6%
uniform stretch of the driven line), its elevation is still planar, and its width
is a uniform 12 m rather than a surveyed profile.
Scenery and proprietary car details remain simplified procedural geometry.
The 53 historical lap/sector timings and exact finish time are preserved.

Regression checks measure the rendered lap length, road-to-car width ratio,
car envelope, tyre size and ground contact, crew height, pit separation, shadow
geometry normals, camera following and full-race distance continuity.
