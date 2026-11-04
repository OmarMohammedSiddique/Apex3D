#include "../include/apex3d/cp3_scene.hpp"

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

    std::cout << "CP3 Bezier route closure, C1 continuity, lap poses and"
                 " track mesh: PASS\n";
    return 0;
}
