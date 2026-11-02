#pragma once
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include "../../../transformations/include/transform2d.hpp"

namespace apex3d::spline {
[[nodiscard]] inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
[[nodiscard]] inline Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
[[nodiscard]] inline Vec2 operator*(Vec2 p, double scalar) { return {p.x * scalar, p.y * scalar}; }

// Uniform Catmull-Rom segment. The curve interpolates P1 at t=0 and P2 at t=1.
[[nodiscard]] inline Vec2 catmullRom(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, double t)
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    return (p1 * 2.0 +
            (p2 - p0) * t +
            (p0 * 2.0 - p1 * 5.0 + p2 * 4.0 - p3) * t2 +
            (p0 * -1.0 + p1 * 3.0 - p2 * 3.0 + p3) * t3) *
           0.5;
}

// Analytic derivative used to orient car poses along the path.
[[nodiscard]] inline Vec2 catmullRomTangent(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3, double t)
{
    const double t2 = t * t;
    return ((p2 - p0) +
            (p0 * 2.0 - p1 * 5.0 + p2 * 4.0 - p3) * (2.0 * t) +
            (p0 * -1.0 + p1 * 3.0 - p2 * 3.0 + p3) * (3.0 * t2)) *
           0.5;
}


// Circuit adapted from the merged Checkpoint 1 motion path, in screen pixels.
inline constexpr std::array<Vec2, 8> controls{{
    {110, 285}, {115, 170}, {180, 75}, {320, 60},
    {440, 125}, {450, 255}, {330, 305}, {220, 265}
}};
inline constexpr int frameCount = 33;
inline constexpr int segmentCount = static_cast<int>(controls.size());

[[nodiscard]] inline Vec2 closedPoint(int index)
{
    return controls[static_cast<std::size_t>((index % segmentCount + segmentCount) % segmentCount)];
}
[[nodiscard]] inline Vec2 position(int segment, double t)
{
    segment = (segment % segmentCount + segmentCount) % segmentCount;
    return catmullRom(closedPoint(segment - 1), closedPoint(segment),
                      closedPoint(segment + 1), closedPoint(segment + 2), t);
}
[[nodiscard]] inline Vec2 tangent(int segment, double t)
{
    segment = (segment % segmentCount + segmentCount) % segmentCount;
    return catmullRomTangent(closedPoint(segment - 1), closedPoint(segment),
                             closedPoint(segment + 1), closedPoint(segment + 2), t);
}
struct Pose {
    Vec2 centre;
    Vec2 derivative;
    double heading; // radians, atan2; positive clockwise in screen coordinates
    Mat3 matrix;
};
// Parameter u measures segments, and may continue through multiple laps.
[[nodiscard]] inline Pose pose(double u)
{
    if (!std::isfinite(u)) throw std::invalid_argument("Non-finite lap parameter");
    u = std::fmod(u, static_cast<double>(segmentCount));
    if (u < 0.0) u += segmentCount;
    const int segment = static_cast<int>(std::floor(u));
    const double t = u - segment;
    const Vec2 centre = position(segment, t);
    const Vec2 derivative = tangent(segment, t);
    const double heading = std::atan2(derivative.y, derivative.x);
    return {centre, derivative, heading,
            Mat3::translation(centre.x, centre.y) *
            Mat3::rotationDegrees(heading * 180.0 / std::numbers::pi)};
}
[[nodiscard]] inline Pose framePose(int frame)
{
    if (frame < 0 || frame >= frameCount) throw std::out_of_range("Frame index");
    return pose(static_cast<double>(segmentCount) * frame / (frameCount - 1));
}
} // namespace apex3d::spline
