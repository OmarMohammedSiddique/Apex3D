#include "../include/apex3d/cp3_hud.hpp"
#include "../include/apex3d/cp3_playback.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <stdexcept>

#define CHECK(expression) \
    do { \
        if (!(expression)) { \
            throw std::runtime_error("Check failed: " #expression); \
        } \
    } while (false)

int main()
{
    using namespace apex3d;
    cp3::Playback playback;
    playback.route = cp3::Route::BezierDemo;
    playback.playing = true;
    CHECK(!playback.looping && !playback.showWheels);
    CHECK(!playback.advance(17.9, 10.0));
    CHECK(playback.advance(0.2, 10.0));
    CHECK(playback.progress == 1.0 && !playback.playing);
    CHECK(std::abs(playback.travelled - 10.0) < 1e-10);
    CHECK(!playback.advance(10.0, 10.0));
    CHECK(playback.progress == 1.0);
    playback.togglePlaying();
    CHECK(playback.playing && playback.progress == 0.0 && playback.travelled == 0.0);
    playback.togglePlaying();
    CHECK(!playback.advance(5.0, 10.0));
    CHECK(playback.progress == 0.0);
    playback.restart();
    playback.looping = true;
    CHECK(playback.advance(19.0, 10.0));
    CHECK(playback.playing && std::abs(playback.progress - 1.0 / 18.0) < 1e-10);
    CHECK(playback.travelled > 10.0);
    playback.looping = false;
    CHECK(playback.advance(18.0, 10.0));
    CHECK(!playback.playing && playback.progress == 1.0);
    playback.restart();
    playback.speed = 4.0;
    CHECK(playback.advance(4.5, 10.0));
    CHECK(playback.progress == 1.0 && !playback.playing);

    // No-slip rotation must follow travel and tyre radius at every speed.
    CHECK(std::abs(cp3::wheelTurnsForDistance(cp3::wheelCircumference) - 1.0) < 1e-10);
    CHECK(std::abs(cp3::wheelTurnsForDistance(cp3::wheelRadius * std::numbers::pi) - 0.5) < 1e-10);
    CHECK(std::abs(cp3::wheelRpm(cp3::wheelCircumference) - 60.0) < 1e-10);
    CHECK(std::abs(cp3::wheelRpm(2.0 * cp3::wheelCircumference) - 120.0) < 1e-10);
    CHECK(cp3::wheelRpm(0.0) == 0.0);
    cp3::Playback rolling;
    rolling.route = cp3::Route::BezierDemo;
    rolling.playing = true;
    rolling.advance(1.0, 10.0);
    const double oneSecondTurns = cp3::wheelTurnsForDistance(rolling.travelled);
    rolling.playing = false;
    rolling.advance(1.0, 10.0);
    CHECK(cp3::wheelTurnsForDistance(rolling.travelled) == oneSecondTurns);
    rolling.restart();
    rolling.speed = 2.0;
    rolling.advance(1.0, 10.0);
    CHECK(std::abs(cp3::wheelTurnsForDistance(rolling.travelled) - 2.0 * oneSecondTurns) < 1e-10);

    // Crossing a wheel revolution must be visually continuous: no body reset.
    const auto beforeTurn = cp3::makeHudMesh(0.5, 1.0, true, false,
        cp3::Route::Monza, true, cp3::wheelCircumference * 0.999999);
    const auto afterTurn = cp3::makeHudMesh(0.5, 1.0, true, false,
        cp3::Route::Monza, true, cp3::wheelCircumference * 1.000001);
    CHECK(beforeTurn.size() == afterTurn.size());
    for (std::size_t i = 0; i < beforeTurn.size(); ++i) {
        CHECK(std::abs(beforeTurn[i].x - afterTurn[i].x) < 0.001F);
        CHECK(std::abs(beforeTurn[i].y - afterTurn[i].y) < 0.001F);
    }


    // ── Catmull-Rom path (existing checks) ───────────────────────────────────
    const auto controls = cp3::evenlySpacedControls();
    const auto path = cp3::makePath();
    CHECK(cp3::monzaControls.size() == 62);
    for (std::size_t i = 0; i < cp3::monzaControls.size(); ++i) {
        const Vec2 a = cp3::monzaControls[i];
        const Vec2 b = cp3::monzaControls[(i + 1) % cp3::monzaControls.size()];
        CHECK(std::hypot(b.x - a.x, b.y - a.y) < 0.5);
    }
    CHECK(controls.size() > cp3::monzaControls.size());
    CHECK(path.size() == controls.size() * cp3::samplesPerSegment + 1);
    CHECK(path.back().distance > 7.0);
    CHECK(std::hypot(path.front().point.x - path.back().point.x,
                     path.front().point.y - path.back().point.y) < 1e-8);
    for (std::size_t i = 1; i < path.size(); ++i) {
        CHECK(path[i].distance > path[i - 1].distance);
        CHECK(std::abs(std::hypot(path[i].normal.x, path[i].normal.y) - 1.0) < 1e-8);
    }
    const auto start   = cp3::poseAtProgress(path, 0.0);
    const auto finish  = cp3::poseAtProgress(path, 1.0);
    const auto quarter = cp3::poseAtProgress(path, 0.25);
    CHECK(std::hypot(start.position.x - finish.position.x,
                     start.position.y - finish.position.y) < 1e-8);
    CHECK(quarter.distance > 0.0 && quarter.distance < path.back().distance);
    CHECK(std::hypot(quarter.position.x - start.position.x,
                     quarter.position.y - start.position.y) > 0.1);
    bool rejectedNonFinite = false;
    try {
        static_cast<void>(cp3::poseAtProgress(path,
            std::numeric_limits<double>::quiet_NaN()));
    } catch (const std::invalid_argument&) {
        rejectedNonFinite = true;
    }
    CHECK(rejectedNonFinite);

    const auto track   = cp3::makeTrackMesh(path);
    const auto model   = car::carModel();
    const auto vehicle = cp3::makeCarMesh(model, quarter);
    const auto transform = cp3::carTransform(quarter);
    const Vec2 centre  = transform.transformPoint(car::kCarCentre);
    const Vec2 nose    = transform.transformPoint({0.0, -70.0});
    const Vec2 forward{std::cos(quarter.headingRadians),
                       std::sin(quarter.headingRadians)};
    CHECK(std::hypot(centre.x - quarter.position.x,
                     centre.y - quarter.position.y) < 1e-8);
    CHECK((nose.x - centre.x) * forward.x +
          (nose.y - centre.y) * forward.y > 0.0);
    CHECK(!track.empty()   && track.size()   % 3 == 0);
    CHECK(!vehicle.empty() && vehicle.size() % 3 == 0);
    for (const auto& vertex : vehicle) {
        CHECK(std::isfinite(vertex.x) && std::isfinite(vertex.y));
    }
    const auto bezierPath = cp3::makeDemoBezierPath();
    CHECK(bezierPath.size() == bezier::arcLengthTable().samples().size());
    for (std::size_t i = 1; i < bezierPath.size(); ++i) {
        CHECK(bezierPath[i].distance > bezierPath[i - 1].distance);
    }
    for (double progress : {0.0, 0.25, 0.5, 0.75, 1.0}) {
        const auto pose = cp3::poseAtProgress(bezierPath, progress);
        const auto expected = cp3::bezierScenePoint(bezier::pointAt(
            bezier::arcLengthTable().parameterForProgress(progress)));
        CHECK(std::hypot(pose.position.x - expected.x, pose.position.y - expected.y) < 0.001);
        const double epsilon = 0.00001;
        const auto next = cp3::poseAtProgress(bezierPath, progress + epsilon);
        const double dx = next.position.x - pose.position.x;
        const double dy = next.position.y - pose.position.y;
        CHECK(dx * std::cos(pose.headingRadians) + dy * std::sin(pose.headingRadians) > 0.0);
    }
    CHECK(!cp3::makeTrackMesh(bezierPath).empty());
    CHECK(cp3::makeControlMesh(cp3::Route::BezierDemo).size() == cp3::monzaControls.size() * 6);
    // Wheel hierarchy: body motion carries the centre, local spin moves the
    // marker without changing the radius. The HUD uses this shared CP2 pose.
    const wheels::WheelGeometry wheel;
    const auto body = Mat3::translation(4.0, 7.0) * Mat3::scaling(0.001, -0.001);
    const auto wheel0 = wheels::poseWheel(wheel, body, 0.0);
    const auto wheel90 = wheels::poseWheel(wheel, body, 90.0);
    CHECK(wheels::distance(wheel0.centre, wheel90.centre) < 1e-10);
    CHECK(std::abs(wheels::tyreRadiusOf(wheel90) - 0.036) < 1e-10);
    CHECK(wheels::distance(wheel0.marker, wheel90.marker) > 0.01);
    for (const auto route : {cp3::Route::Monza, cp3::Route::BezierDemo}) {
        const auto withWheels = cp3::makeHudMesh(0.25, 1.0, false, false, route, true, 1.0);
        const auto withoutWheels = cp3::makeHudMesh(0.25, 1.0, false, false, route, false);
        CHECK(withWheels.size() > withoutWheels.size());
        CHECK(withWheels.size() % 3 == 0);
        for (const auto& vertex : withWheels) {
            CHECK(std::isfinite(vertex.x) && std::isfinite(vertex.y));
            CHECK(vertex.x >= -1.0F && vertex.x <= 1.0F);
            CHECK(vertex.y >= -1.0F && vertex.y <= 1.0F);
        }
    }
    std::cout << "CP3 Monza path, lap pose, track mesh and CP2 car integration: PASS\n";

    // ── CP2 Bezier route (issue #18) ─────────────────────────────────────────

    // 1. Circuit has exactly 62 segments (one per Monza anchor).
    const auto& circuit = cp3::monzaCircuit();
    CHECK(circuit.size() == cp3::monzaControls.size());

    // 2. C0: p3 of segment i == p0 of segment i+1 around the whole loop.
    for (std::size_t i = 0; i < circuit.size(); ++i) {
        const bezier::CubicBezier& seg  = circuit[i];
        const bezier::CubicBezier& next = circuit[(i + 1) % circuit.size()];
        CHECK(std::hypot(seg.p3.x - next.p0.x, seg.p3.y - next.p0.y) < 1e-10);
    }

    // 3. C1: derivative at end of segment i == derivative at start of i+1.
    for (std::size_t i = 0; i < circuit.size(); ++i) {
        const bezier::CubicBezier& seg  = circuit[i];
        const bezier::CubicBezier& next = circuit[(i + 1) % circuit.size()];
        const Vec2 dEnd   = bezier::derivative(seg,  1.0);
        const Vec2 dStart = bezier::derivative(next, 0.0);
        CHECK(std::hypot(dEnd.x - dStart.x, dEnd.y - dStart.y) < 1e-8);
    }

    // 4. Arc-length table is monotone and covers the full Monza circuit.
    const auto& arcTable = cp3::monzaArcTable();
    CHECK(arcTable.size() > 1);
    CHECK(arcTable.back().cumulativeDistance > 7.0);
    for (std::size_t i = 1; i < arcTable.size(); ++i) {
        CHECK(arcTable[i].cumulativeDistance >= arcTable[i - 1].cumulativeDistance);
        // Continuous u must also be strictly non-decreasing.
        CHECK(arcTable[i].u >= arcTable[i - 1].u);
    }
    // u must be continuous across every segment join: the last entry of
    // segment i has u == i+1 == the first entry of segment i+1.
    {
        const int nSegs = static_cast<int>(cp3::monzaCircuit().size());
        constexpr int stepsPerSeg = 120; // must match makeMonzaArcTable
        for (int i = 0; i < nSegs - 1; ++i) {
            // Entry index of the last step of segment i
            const std::size_t lastOfSeg =
                static_cast<std::size_t>(i * stepsPerSeg + stepsPerSeg);
            const std::size_t firstOfNext =
                static_cast<std::size_t>((i + 1) * stepsPerSeg + 1);
            // The u at the end of seg i should equal i+1.0 (== start of seg i+1).
            CHECK(std::abs(arcTable[lastOfSeg].u - static_cast<double>(i + 1)) < 1e-12);
            CHECK(std::abs(arcTable[firstOfNext].u -
                           (static_cast<double>(i + 1) +
                            1.0 / stepsPerSeg)) < 1e-12);
        }
    }

    // 5. makeBezierPath: closed loop, monotone distances, unit normals.
    const auto bzPath = cp3::makeBezierPath();
    CHECK(bzPath.size() >= 2);
    CHECK(bzPath.back().distance > 7.0);
    CHECK(std::hypot(bzPath.front().point.x - bzPath.back().point.x,
                     bzPath.front().point.y - bzPath.back().point.y) < 1e-8);
    for (std::size_t i = 1; i + 1 < bzPath.size(); ++i) {
        CHECK(bzPath[i].distance >= bzPath[i - 1].distance);
    }
    for (const auto& s : bzPath) {
        CHECK(std::abs(std::hypot(s.normal.x, s.normal.y) - 1.0) < 1e-8);
    }

    // 6. poseAtProgress: progress 0 and 1 land at same position.
    const auto bzStart   = cp3::poseAtProgress(bzPath, 0.0);
    const auto bzFinish  = cp3::poseAtProgress(bzPath, 1.0);
    const auto bzQuarter = cp3::poseAtProgress(bzPath, 0.25);
    CHECK(std::hypot(bzStart.position.x  - bzFinish.position.x,
                     bzStart.position.y  - bzFinish.position.y) < 1e-8);
    CHECK(std::hypot(bzQuarter.position.x - bzStart.position.x,
                     bzQuarter.position.y - bzStart.position.y) > 0.1);

    // 7. All 40 evenly-spaced lap poses along the Bezier route are finite.
    for (int k = 0; k < 40; ++k) {
        const double prog = static_cast<double>(k) / 40.0;
        const auto pose = cp3::poseAtProgress(bzPath, prog);
        CHECK(std::isfinite(pose.position.x) && std::isfinite(pose.position.y));
        CHECK(std::isfinite(pose.headingRadians));
    }

    // 8. makePathForMode dispatches to the correct path.
    const auto modePathBZ = cp3::makePathForMode(cp3::RouteMode::Bezier);
    const auto modePathCR = cp3::makePathForMode(cp3::RouteMode::CatmullRom);
    CHECK(modePathBZ.size() == bzPath.size());
    CHECK(modePathCR.size() == path.size());

    // 9. Track mesh built from the Bezier path is valid.
    const auto bzTrack = cp3::makeTrackMesh(bzPath);
    CHECK(!bzTrack.empty() && bzTrack.size() % 3 == 0);

    // 10. Bezier route stays within the Monza bounding box (no shortcuts).
    for (const auto& s : bzPath) {
        CHECK(s.point.x > -2.0 && s.point.x < 2.0);
        CHECK(s.point.y > -1.2 && s.point.y < 1.2);
    }

    // 11. Regression: no sudden position jumps across segment boundaries.
    //
    // Densely sample bezierSampleAtProgress() around every segment join
    // (progress values that map to the arc-length fraction at the join ± a
    // tiny epsilon).  Two consecutive samples must never be farther apart
    // than a threshold that rules out the wrap-around discontinuity that the
    // old per-segment t interpolation produced.
    //
    // The Monza circuit is ~9 units long with 62 segments (~0.145 u each).
    // A genuine smooth step at 1/10 000th of a lap covers at most ~0.0009 u,
    // which at peak curvature is well under 0.01 world units.  We use 0.05 as
    // the jump threshold — generous enough to tolerate arc-length variation but
    // tight enough to catch the old boundary teleport (~0.1–0.3 units).
    {
        const auto& arcTableRef = cp3::monzaArcTable();
        const double totalLen   = arcTableRef.back().cumulativeDistance;
        constexpr int nSegs     = 62; // monzaControls.size()
        constexpr int stepsPerSeg = 120;
        constexpr double jumpTolerance = 0.05;
        constexpr double epsilon = 1e-5; // tiny progress nudge near the join

        for (int joinIdx = 1; joinIdx < nSegs; ++joinIdx) {
            // Arc-length distance at the join between seg (joinIdx-1) and joinIdx.
            // That is the last table entry of the previous segment.
            const std::size_t lastIdx =
                static_cast<std::size_t>(joinIdx * stepsPerSeg);
            const double joinDist = arcTableRef[lastIdx].cumulativeDistance;
            const double progressAtJoin = joinDist / totalLen;

            // Sample just before and just after the join.
            const cp3::PathSample before =
                cp3::bezierSampleAtProgress(progressAtJoin - epsilon);
            const cp3::PathSample after  =
                cp3::bezierSampleAtProgress(progressAtJoin + epsilon);

            const double jump = std::hypot(after.point.x - before.point.x,
                                           after.point.y - before.point.y);
            CHECK(jump < jumpTolerance);
        }
    }

    std::cout << "CP3 Bezier route closure, C1 continuity, lap poses and"
                 " track mesh: PASS\n";
    return 0;
}
