#include "../include/apex3d/cp3_wheels.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <numbers>
#include <stdexcept>

#define CHECK(expression) \
    do { \
        if (!(expression)) { \
            throw std::runtime_error("Check failed: " #expression); \
        } \
    } while (false)

namespace {

using namespace apex3d;

double length(Vec2 v)
{
    return std::hypot(v.x, v.y);
}

Vec2 minus(Vec2 a, Vec2 b)
{
    return {a.x - b.x, a.y - b.y};
}

double cross(Vec2 a, Vec2 b)
{
    return a.x * b.y - a.y * b.x;
}

// Direction the wheel rolls in world space: its local -y (the car's nose).
Vec2 rollingDirection(const Mat3& matrix, Vec2 mount)
{
    return minus(matrix.transformPoint({mount.x, mount.y - 1.0}),
                 matrix.transformPoint(mount));
}

void checkMounts(const cp3::WheelMounts& mounts)
{
    int steering = 0;
    for (const auto& mount : mounts) {
        CHECK(mount.halfWidth > 0.0 && mount.halfLength > mount.halfWidth);
        steering += mount.steers ? 1 : 0;
        // Front wheels are ahead of the car centre (nose is -y).
        CHECK(mount.steers == (mount.centre.y < car::kCarCentre.y));
    }
    CHECK(steering == 2);
    CHECK(std::abs(mounts[0].centre.x - -31.0) < 1e-12);
    CHECK(std::abs(mounts[0].centre.y - -30.5) < 1e-12);
    CHECK(cp3::wheelbase(mounts) > 70.0);
}

// Every wheel stays on its CP2 mount however the car is moved, turned,
// scaled or steered.
void checkAttachment(const std::vector<cp3::PathSample>& path,
                     const cp3::WheelMounts& mounts)
{
    for (const double progress : {0.0, 0.13, 0.25, 0.5, 0.71, 0.97}) {
        for (const double scale : {1.0, 1.22, 0.78}) {
            const auto pose = cp3::poseAtProgress(path, progress);
            const auto state = cp3::wheelStateAt(path, mounts, progress);
            const Mat3 body = cp3::carTransform(pose, scale);
            for (const auto& mount : mounts) {
                const Mat3 wheel = cp3::wheelMatrix(mount, body, state.steerRadians);
                const Vec2 expected = body.transformPoint(mount.centre);
                CHECK(length(minus(wheel.transformPoint(mount.centre), expected)) < 1e-12);
                const Vec2 carForward{std::cos(pose.headingRadians),
                                      std::sin(pose.headingRadians)};
                const Vec2 rolling = rollingDirection(wheel, mount.centre);
                const double yaw = std::atan2(cross(carForward, rolling),
                                              carForward.x * rolling.x +
                                              carForward.y * rolling.y);
                const double expectedYaw = mount.steers ? state.steerRadians : 0.0;
                CHECK(std::abs(yaw - expectedYaw) < 1e-9);
                // Steering is a rotation: the tyre keeps its size.
                CHECK(std::abs(length(rolling) - cp3::carBaseScale * scale) < 1e-12);
            }

            const auto mesh = cp3::makeWheelMesh(mounts, body, state);
            CHECK(!mesh.empty() && mesh.size() % 3 == 0);
            for (const auto& vertex : mesh) {
                CHECK(std::isfinite(vertex.x) && std::isfinite(vertex.y));
                double nearest = std::numeric_limits<double>::max();
                for (const auto& mount : mounts) {
                    const Vec2 centre = body.transformPoint(mount.centre);
                    const double reach = std::hypot(mount.halfWidth, mount.halfLength) *
                                         cp3::carBaseScale * scale;
                    nearest = std::min(nearest,
                        length(minus({vertex.x, vertex.y}, centre)) - reach);
                }
                CHECK(nearest < 1e-6);
            }
        }
    }
}

void checkSteering(const std::vector<cp3::PathSample>& path,
                   const cp3::WheelMounts& mounts)
{
    // The start/finish straight needs almost no lock. (Progress 0 itself sits
    // where the Parabolica exit joins the straight, so sample just after it.)
    for (const double progress : {0.02, 0.05, 0.1}) {
        CHECK(std::abs(cp3::steeringRadians(path, mounts, progress)) <
              1.0 * std::numbers::pi / 180.0);
    }
    double largest = 0.0;
    int agreeing = 0;
    int turning = 0;
    for (int step = 0; step < 400; ++step) {
        const double progress = step / 400.0;
        const double steer = cp3::steeringRadians(path, mounts, progress);
        CHECK(std::abs(steer) <= cp3::maxSteerRadians + 1e-12);
        largest = std::max(largest, std::abs(steer));
        if (std::abs(steer) < 5.0 * std::numbers::pi / 180.0) {
            continue;
        }
        // The front wheels point towards the side the circuit bends to.
        const auto here = cp3::poseAtProgress(path, progress);
        const auto ahead = cp3::poseAtProgress(path, progress + 0.04 / path.back().distance);
        const double bend = cp3::wrapRadians(ahead.headingRadians - here.headingRadians);
        ++turning;
        agreeing += (bend > 0.0) == (steer > 0.0) ? 1 : 0;
    }
    CHECK(largest > 12.0 * std::numbers::pi / 180.0);
    CHECK(turning > 20);
    CHECK(agreeing >= turning * 9 / 10);
}

void checkRolling(const std::vector<cp3::PathSample>& path,
                  const cp3::WheelMounts& mounts)
{
    for (const auto& mount : mounts) {
        const double turns = cp3::revolutionsPerLap(path, mount);
        const double radiusWorld = mount.halfLength * cp3::carBaseScale;
        const double ideal = path.back().distance / (2.0 * std::numbers::pi * radiusWorld);
        CHECK(turns == std::round(turns) && turns > 50.0);
        CHECK(std::abs(turns - ideal) <= 0.5);

        // Spin follows distance travelled (rolling without slipping).
        const auto quarter = cp3::poseAtProgress(path, 0.25);
        const double spin = cp3::spinRadians(path, mount, 0.25);
        CHECK(std::abs(spin - quarter.distance / radiusWorld) / spin < 0.01);
        // The tread pattern lines up across the start/finish seam.
        const double end = cp3::spinRadians(path, mount, 1.0 - 1e-9);
        CHECK(std::abs(cp3::wrapRadians(end - cp3::spinRadians(path, mount, 0.0))) < 1e-5);
        CHECK(cp3::spinRadians(path, mount, 0.5) > cp3::spinRadians(path, mount, 0.4));
    }

    // Rolling forward carries the top tread bars towards the nose (-y), and
    // only bars on the upper half of the tyre are drawn.
    const auto& front = mounts[0];
    const auto now = cp3::visibleTreadBars(front, 0.3);
    const auto later = cp3::visibleTreadBars(front, 0.35);
    CHECK(!now.empty() && now.size() == later.size());
    CHECK(now.front().highlighted && later.front().highlighted);
    CHECK(later.front().centreY < now.front().centreY);
    for (const auto& bar : now) {
        CHECK(std::abs(bar.centreY - front.centre.y) <= front.halfLength);
        CHECK(bar.halfThickness > 0.0);
    }
    bool hidden = true;
    for (const auto& bar : cp3::visibleTreadBars(front, std::numbers::pi)) {
        hidden = hidden && !bar.highlighted;
    }
    CHECK(hidden);

    bool rejected = false;
    try {
        static_cast<void>(cp3::spinRadians(path, front,
            std::numeric_limits<double>::infinity()));
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    CHECK(rejected);
}

void checkLiveCar(const std::vector<cp3::PathSample>& path,
                  const std::vector<car::Component>& model,
                  const cp3::WheelMounts& mounts)
{
    const auto pose = cp3::poseAtProgress(path, 0.25);
    const auto state = cp3::wheelStateAt(path, mounts, 0.25);
    const auto live = cp3::makeLiveCarMesh(model, mounts, pose, state);
    const auto wheels = cp3::makeWheelMesh(mounts, cp3::carTransform(pose), state);
    CHECK(live.size() > wheels.size() && live.size() % 3 == 0);
    // Wheels are drawn first so the bodywork sits on top of them.
    for (std::size_t i = 0; i < wheels.size(); ++i) {
        CHECK(live[i].x == wheels[i].x && live[i].y == wheels[i].y);
    }
}

} // namespace

int main()
{
    const auto path = cp3::makePath();
    const auto model = car::carModel();
    const auto mounts = cp3::wheelMountsFrom(model);
    checkMounts(mounts);
    checkAttachment(path, mounts);
    checkSteering(path, mounts);
    checkRolling(path, mounts);
    checkLiveCar(path, model, mounts);
    std::cout << "CP3 wheel hierarchy, steering, rolling tread and live car mesh: PASS\n";
}
