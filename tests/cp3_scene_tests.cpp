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
    const auto controls = cp3::evenlySpacedControls();
    const auto path = cp3::makePath();
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
    std::cout << "CP3 Monza path, lap pose, track mesh and CP2 car integration: PASS\n";
}
