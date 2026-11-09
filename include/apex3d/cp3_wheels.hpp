#pragma once

// Checkpoint 3: the CP2 wheel hierarchy inside the live OpenGL car.
//
// Each tyre is a child of the car body. Its matrix is built by the CP2 wheel
// milestone's own hierarchy function:
//
//   M_wheel = M_car * T(mount) * R(steer) * T(-mount)
//
// so a wheel can turn about its own centre while still riding with the car.
// The prototype is viewed from above, where a spinning wheel's rotation axis
// lies in the screen plane. Spin is therefore shown the way a real tyre looks
// from above: tread bars roll over the top of the tyre, from the rear edge to
// the front, at the rate set by rolling without slipping (angle = distance /
// radius). The front pair also steers by an angle taken from the curvature of
// the Monza path.

#include "cp3_scene.hpp"
#include "../../checkpoint2/animation/wheels/include/wheel_animation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numbers>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace apex3d::cp3 {

inline constexpr std::size_t wheelCount = 4;
inline constexpr int treadBarCount = 6;
inline constexpr double maxSteerRadians = 25.0 * std::numbers::pi / 180.0;
// Half the spacing, in world units, of the two path samples used to measure
// how quickly the circuit is turning under the car.
inline constexpr double steerLookDistance = 0.03;

struct WheelMount {
    std::string_view name;
    Vec2 centre;       // car-model coordinates
    double halfWidth;  // across the car (model x)
    double halfLength; // along the car (model y); the tyre radius seen from above
    bool steers;
};

using WheelMounts = std::array<WheelMount, wheelCount>;

[[nodiscard]] inline bool isWheelComponent(std::string_view name)
{
    return name.starts_with("tyre ") || name.starts_with("stripe ");
}

// The mounts are measured from the CP2 polygon-car tyres, so the wheels stay
// exactly where the car model already draws them.
[[nodiscard]] inline WheelMounts wheelMountsFrom(const std::vector<car::Component>& model)
{
    WheelMounts mounts{};
    std::size_t found = 0;
    for (const auto& component : model) {
        if (!component.name.starts_with("tyre ")) {
            continue;
        }
        if (found == wheelCount || component.polygon.empty()) {
            throw std::invalid_argument("The car model must have four tyre polygons");
        }
        double left = std::numeric_limits<double>::max();
        double right = std::numeric_limits<double>::lowest();
        double top = std::numeric_limits<double>::max();
        double bottom = std::numeric_limits<double>::lowest();
        for (Vec2 point : component.polygon) {
            left = std::min(left, point.x);
            right = std::max(right, point.x);
            top = std::min(top, point.y);
            bottom = std::max(bottom, point.y);
        }
        mounts[found++] = {component.name,
                           {(left + right) * 0.5, (top + bottom) * 0.5},
                           (right - left) * 0.5, (bottom - top) * 0.5,
                           component.name.find("front") != std::string_view::npos};
    }
    if (found != wheelCount) {
        throw std::invalid_argument("The car model must have four tyre polygons");
    }
    return mounts;
}

[[nodiscard]] inline double wrapRadians(double angle)
{
    return std::atan2(std::sin(angle), std::cos(angle));
}

// Front-to-rear axle distance in model units.
[[nodiscard]] inline double wheelbase(const WheelMounts& mounts)
{
    double front = 0.0;
    double rear = 0.0;
    int frontCount = 0;
    int rearCount = 0;
    for (const auto& mount : mounts) {
        if (mount.steers) {
            front += mount.centre.y;
            ++frontCount;
        } else {
            rear += mount.centre.y;
            ++rearCount;
        }
    }
    if (frontCount == 0 || rearCount == 0) {
        throw std::invalid_argument("A car needs front and rear wheels");
    }
    return rear / rearCount - front / frontCount;
}

// Bicycle-model steering: atan(wheelbase * curvature), limited to a
// believable lock. Positive is a left (anticlockwise) turn, which matches the
// sign of the CP2 rotation matrix in the y-up OpenGL world.
[[nodiscard]] inline double steeringRadians(const std::vector<PathSample>& path,
                                            const WheelMounts& mounts,
                                            double progress)
{
    const double lapLength = path.back().distance;
    const double step = steerLookDistance / lapLength;
    const Pose before = poseAtProgress(path, progress - step);
    const Pose after = poseAtProgress(path, progress + step);
    const double curvature =
        wrapRadians(after.headingRadians - before.headingRadians) /
        (2.0 * steerLookDistance);
    const double wheelbaseWorld = wheelbase(mounts) * carBaseScale;
    return std::clamp(std::atan(wheelbaseWorld * curvature),
                      -maxSteerRadians, maxSteerRadians);
}

// Whole revolutions per lap for a tyre, so the tread pattern lines up again
// when the lap wraps at the start/finish line.
[[nodiscard]] inline double revolutionsPerLap(const std::vector<PathSample>& path,
                                              const WheelMount& mount)
{
    const double radiusWorld = mount.halfLength * carBaseScale;
    const double turns = path.back().distance / (2.0 * std::numbers::pi * radiusWorld);
    return std::max(1.0, std::round(turns));
}

// Rolling without slipping: spin = distance / radius, rounded to whole turns
// per lap. Grows from 0 at the start line to 2*pi*revolutions at the finish.
[[nodiscard]] inline double spinRadians(const std::vector<PathSample>& path,
                                        const WheelMount& mount, double progress)
{
    if (!std::isfinite(progress)) {
        throw std::invalid_argument("Wheel spin requires a finite lap progress");
    }
    const double wrapped = progress - std::floor(progress);
    return 2.0 * std::numbers::pi * revolutionsPerLap(path, mount) * wrapped;
}

struct WheelState {
    double steerRadians{};
    std::array<double, wheelCount> spinRadians{};
};

[[nodiscard]] inline WheelState wheelStateAt(const std::vector<PathSample>& path,
                                             const WheelMounts& mounts,
                                             double progress)
{
    WheelState state;
    state.steerRadians = steeringRadians(path, mounts, progress);
    for (std::size_t i = 0; i < wheelCount; ++i) {
        state.spinRadians[i] = spinRadians(path, mounts[i], progress);
    }
    return state;
}

// The CP2 wheel milestone's hierarchy, reused unchanged: the body matrix
// carries a rotation about the wheel's own mount point.
[[nodiscard]] inline Mat3 wheelMatrix(const WheelMount& mount, const Mat3& carMatrix,
                                      double steerRadians)
{
    wheels::WheelGeometry geometry;
    geometry.mount = mount.centre;
    const double yaw = mount.steers ? steerRadians * 180.0 / std::numbers::pi : 0.0;
    return wheels::wheelTransform(geometry, carMatrix, yaw);
}

struct TreadBar {
    double centreY;       // model y of the bar's middle
    double halfThickness; // shrinks as the bar turns away from the viewer
    bool highlighted;     // bar 0 is yellow so the wheel angle can be read
};

// Bars on the upper half of the tyre, projected onto the top-down view. A bar
// at angle phi from top-dead-centre sits at -r*sin(phi) along the car (nose is
// -y), so increasing spin carries each bar forward over the top of the tyre.
[[nodiscard]] inline std::vector<TreadBar> visibleTreadBars(const WheelMount& mount,
                                                            double spin)
{
    std::vector<TreadBar> bars;
    for (int bar = 0; bar < treadBarCount; ++bar) {
        const double phi = spin + 2.0 * std::numbers::pi * bar / treadBarCount;
        const double facing = std::cos(phi);
        if (facing <= 0.08) {
            continue;
        }
        bars.push_back({mount.centre.y - mount.halfLength * std::sin(phi),
                        0.09 * mount.halfLength * facing, bar == 0});
    }
    return bars;
}

[[nodiscard]] inline std::vector<Vec2> tyreOutline(const WheelMount& mount)
{
    return car::rectangle(mount.centre.x - mount.halfWidth,
                          mount.centre.y - mount.halfLength,
                          mount.centre.x + mount.halfWidth,
                          mount.centre.y + mount.halfLength);
}

[[nodiscard]] inline std::vector<Vertex> makeWheelMesh(const WheelMounts& mounts,
                                                       const Mat3& carMatrix,
                                                       const WheelState& state)
{
    constexpr Color tyre{0.04F, 0.04F, 0.05F};
    constexpr Color tread{0.46F, 0.48F, 0.52F};
    constexpr Color marker{0.98F, 0.82F, 0.14F};
    std::vector<Vertex> mesh;
    mesh.reserve(wheelCount * (2 + treadBarCount * 2) * 3);
    for (std::size_t i = 0; i < wheelCount; ++i) {
        const WheelMount& mount = mounts[i];
        const Mat3 matrix = wheelMatrix(mount, carMatrix, state.steerRadians);
        triangulate(mesh, transformPolygon(tyreOutline(mount), matrix), tyre);
        const double inset = mount.halfWidth * 0.18;
        const double front = mount.centre.y - mount.halfLength;
        const double rear = mount.centre.y + mount.halfLength;
        for (const TreadBar& bar : visibleTreadBars(mount, state.spinRadians[i])) {
            const double y0 = std::max(front, bar.centreY - bar.halfThickness);
            const double y1 = std::min(rear, bar.centreY + bar.halfThickness);
            if (y1 - y0 <= 1e-9) {
                continue;
            }
            const auto strip = car::rectangle(mount.centre.x - mount.halfWidth + inset, y0,
                                              mount.centre.x + mount.halfWidth - inset, y1);
            triangulate(mesh, transformPolygon(strip, matrix),
                        bar.highlighted ? marker : tread);
        }
    }
    return mesh;
}

// The live car: wheels first (they sit under the bodywork), then every CP2
// body component through the same car matrix. The CP2 static tyre and stripe
// polygons are replaced by the animated wheels.
[[nodiscard]] inline std::vector<Vertex> makeLiveCarMesh(
    const std::vector<car::Component>& model, const WheelMounts& mounts,
    const Pose& pose, const WheelState& state, double scaleMultiplier = 1.0)
{
    const Mat3 carMatrix = carTransform(pose, scaleMultiplier);
    std::vector<Vertex> mesh = makeWheelMesh(mounts, carMatrix, state);
    for (const auto& component : model) {
        if (!isWheelComponent(component.name)) {
            triangulate(mesh, transformPolygon(component.polygon, carMatrix),
                        colorFromCar(component.fill));
        }
    }
    return mesh;
}

} // namespace apex3d::cp3
