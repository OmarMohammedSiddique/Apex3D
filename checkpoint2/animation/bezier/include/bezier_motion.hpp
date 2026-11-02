#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <stdexcept>
#include <vector>

#include "../../../transformations/include/transform2d.hpp"

namespace apex3d::bezier {

[[nodiscard]] inline Vec2 operator+(Vec2 a, Vec2 b)
{
    return {a.x + b.x, a.y + b.y};
}

[[nodiscard]] inline Vec2 operator-(Vec2 a, Vec2 b)
{
    return {a.x - b.x, a.y - b.y};
}

[[nodiscard]] inline Vec2 operator*(Vec2 point, double scalar)
{
    return {point.x * scalar, point.y * scalar};
}

[[nodiscard]] inline Vec2 operator/(Vec2 point, double scalar)
{
    return {point.x / scalar, point.y / scalar};
}

struct CubicBezier {
    Vec2 p0;
    Vec2 p1;
    Vec2 p2;
    Vec2 p3;
};

[[nodiscard]] inline Vec2 evaluate(const CubicBezier& curve, double t)
{
    const double oneMinusT = 1.0 - t;
    const double b0 = oneMinusT * oneMinusT * oneMinusT;
    const double b1 = 3.0 * oneMinusT * oneMinusT * t;
    const double b2 = 3.0 * oneMinusT * t * t;
    const double b3 = t * t * t;
    return curve.p0 * b0 + curve.p1 * b1 + curve.p2 * b2 + curve.p3 * b3;
}

[[nodiscard]] inline Vec2 derivative(const CubicBezier& curve, double t)
{
    const double oneMinusT = 1.0 - t;
    return (curve.p1 - curve.p0) * (3.0 * oneMinusT * oneMinusT) +
           (curve.p2 - curve.p1) * (6.0 * oneMinusT * t) +
           (curve.p3 - curve.p2) * (3.0 * t * t);
}

inline constexpr std::array<Vec2, 8> anchors{{
    {95.0, 275.0}, {105.0, 160.0}, {175.0, 80.0}, {330.0, 65.0},
    {490.0, 125.0}, {515.0, 245.0}, {400.0, 325.0}, {245.0, 300.0}
}};

inline constexpr int segmentCount = static_cast<int>(anchors.size());
inline constexpr int frameCount = 25;
inline constexpr int arcSamplesPerSegment = 160;

[[nodiscard]] inline Vec2 closedAnchor(int index)
{
    const int wrapped = (index % segmentCount + segmentCount) % segmentCount;
    return anchors[static_cast<std::size_t>(wrapped)];
}

// Converts the closed anchor loop into a C1-continuous chain of cubic Bezier
// segments. Each handle is one sixth of the neighbouring-anchor chord.
[[nodiscard]] inline std::array<CubicBezier, segmentCount> makeCircuit()
{
    std::array<CubicBezier, segmentCount> circuit{};
    for (int index = 0; index < segmentCount; ++index) {
        const Vec2 previous = closedAnchor(index - 1);
        const Vec2 start = closedAnchor(index);
        const Vec2 end = closedAnchor(index + 1);
        const Vec2 after = closedAnchor(index + 2);
        circuit[static_cast<std::size_t>(index)] = {
            start,
            start + (end - previous) / 6.0,
            end - (after - start) / 6.0,
            end
        };
    }
    return circuit;
}

[[nodiscard]] inline const std::array<CubicBezier, segmentCount>& circuit()
{
    static const std::array<CubicBezier, segmentCount> value = makeCircuit();
    return value;
}

struct CurveParameter {
    int segment{};
    double t{};
};

[[nodiscard]] inline CurveParameter parameterAt(double u)
{
    if (!std::isfinite(u)) {
        throw std::invalid_argument("Bezier parameter must be finite");
    }
    const double maximum = static_cast<double>(segmentCount);
    const double clamped = std::clamp(u, 0.0, maximum);
    if (clamped >= maximum) {
        return {segmentCount - 1, 1.0};
    }
    const int segment = static_cast<int>(std::floor(clamped));
    return {segment, clamped - static_cast<double>(segment)};
}

[[nodiscard]] inline Vec2 pointAt(double u)
{
    const CurveParameter parameter = parameterAt(u);
    return evaluate(circuit()[static_cast<std::size_t>(parameter.segment)], parameter.t);
}

[[nodiscard]] inline Vec2 tangentAt(double u)
{
    const CurveParameter parameter = parameterAt(u);
    return derivative(circuit()[static_cast<std::size_t>(parameter.segment)], parameter.t);
}

[[nodiscard]] inline double distance(Vec2 a, Vec2 b)
{
    return std::hypot(a.x - b.x, a.y - b.y);
}

struct ArcSample {
    double cumulativeDistance{};
    double u{};
};

class ArcLengthTable {
public:
    ArcLengthTable()
    {
        samples_.reserve(static_cast<std::size_t>(segmentCount * arcSamplesPerSegment + 1));
        samples_.push_back({0.0, 0.0});
        Vec2 previous = pointAt(0.0);
        double cumulative = 0.0;
        for (int segment = 0; segment < segmentCount; ++segment) {
            for (int step = 1; step <= arcSamplesPerSegment; ++step) {
                const double t = static_cast<double>(step) / arcSamplesPerSegment;
                const double u = static_cast<double>(segment) + t;
                const Vec2 current = pointAt(u);
                cumulative += distance(previous, current);
                samples_.push_back({cumulative, u});
                previous = current;
            }
        }
        totalLength_ = cumulative;
    }

    [[nodiscard]] double totalLength() const { return totalLength_; }
    [[nodiscard]] const std::vector<ArcSample>& samples() const { return samples_; }

    [[nodiscard]] double parameterForProgress(double progress) const
    {
        if (!std::isfinite(progress)) {
            throw std::invalid_argument("Bezier progress must be finite");
        }
        const double clamped = std::clamp(progress, 0.0, 1.0);
        const double target = clamped * totalLength_;
        const auto upper = std::lower_bound(
            samples_.begin(), samples_.end(), target,
            [](const ArcSample& sample, double value) {
                return sample.cumulativeDistance < value;
            });
        if (upper == samples_.begin()) {
            return upper->u;
        }
        if (upper == samples_.end()) {
            return samples_.back().u;
        }
        const ArcSample& after = *upper;
        const ArcSample& before = *(upper - 1);
        const double span = after.cumulativeDistance - before.cumulativeDistance;
        const double alpha = span == 0.0 ? 0.0 :
            (target - before.cumulativeDistance) / span;
        return before.u + alpha * (after.u - before.u);
    }

private:
    std::vector<ArcSample> samples_;
    double totalLength_{};
};

[[nodiscard]] inline const ArcLengthTable& arcLengthTable()
{
    static const ArcLengthTable table;
    return table;
}

struct Pose {
    Vec2 centre;
    Vec2 tangent;
    double headingRadians{};
    double progress{};
    double travelledDistance{};
    Mat3 matrix;
};

[[nodiscard]] inline Pose poseAtProgress(double progress)
{
    if (!std::isfinite(progress)) {
        throw std::invalid_argument("Bezier progress must be finite");
    }
    const double clamped = std::clamp(progress, 0.0, 1.0);
    const double u = arcLengthTable().parameterForProgress(clamped);
    const Vec2 centre = pointAt(u);
    const Vec2 tangent = tangentAt(u);
    if (std::hypot(tangent.x, tangent.y) <= 1e-9) {
        throw std::runtime_error("Bezier tangent is degenerate");
    }
    const double heading = std::atan2(tangent.y, tangent.x);
    return {
        centre,
        tangent,
        heading,
        clamped,
        clamped * arcLengthTable().totalLength(),
        Mat3::translation(centre.x, centre.y) *
            Mat3::rotationDegrees(heading * 180.0 / std::numbers::pi)
    };
}

[[nodiscard]] inline Pose framePose(int frame)
{
    if (frame < 0 || frame >= frameCount) {
        throw std::out_of_range("Bezier frame index is outside the animation");
    }
    return poseAtProgress(static_cast<double>(frame) / (frameCount - 1));
}

} // namespace apex3d::bezier
