#include "../include/apex3d/cp3_hud.hpp"

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

    const auto start = cp3::poseAtProgress(path, 0.0);
    const auto finish = cp3::poseAtProgress(path, 1.0);
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

    const auto track = cp3::makeTrackMesh(path);
    const auto model = car::carModel();
    const auto vehicle = cp3::makeCarMesh(model, quarter);
    const auto transform = cp3::carTransform(quarter);
    const Vec2 centre = transform.transformPoint(car::kCarCentre);
    const Vec2 nose = transform.transformPoint({0.0, -70.0});
    const Vec2 forward{std::cos(quarter.headingRadians),
                       std::sin(quarter.headingRadians)};
    CHECK(std::hypot(centre.x - quarter.position.x,
                     centre.y - quarter.position.y) < 1e-8);
    CHECK((nose.x - centre.x) * forward.x +
          (nose.y - centre.y) * forward.y > 0.0);
    CHECK(!track.empty() && track.size() % 3 == 0);
    CHECK(!vehicle.empty() && vehicle.size() % 3 == 0);
    for (const auto& vertex : vehicle) {
        CHECK(std::isfinite(vertex.x) && std::isfinite(vertex.y));
    }
    const auto bezierPath = cp3::makeBezierPath();
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
    CHECK(cp3::makeControlMesh(cp3::Route::BezierDemo).size() == bezier::anchors.size() * 6);
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
}
