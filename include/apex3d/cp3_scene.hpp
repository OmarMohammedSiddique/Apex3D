#pragma once

#include "../../checkpoint2/animation/car/include/car_animation.hpp"
#include "../../checkpoint2/animation/bezier/include/bezier_motion.hpp"
#include "../../checkpoint2/animation/splines/include/spline_motion.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace apex3d::cp3 {

// Selectable lap route modes.
enum class RouteMode {
    CatmullRom, // CP2 Catmull-Rom spline through the Monza centreline (default)
    Bezier      // CP2 cubic Bézier chain through the same Monza centreline
};

struct Color {
    float r;
    float g;
    float b;
};

struct Vertex {
    float x;
    float y;
    float r;
    float g;
    float b;
};

// The original hand-authored Monza centreline remains the reference shape.
// CP2's Catmull-Rom algorithm turns these points into a continuous lap.
inline constexpr auto monzaControls = std::to_array<Vec2>({
    { 1.16, -0.87}, { 0.82, -0.87}, { 0.45, -0.87},
    { 0.08, -0.87}, {-0.25, -0.87}, {-0.46, -0.87},
    {-0.49, -0.84}, {-0.49, -0.78}, {-0.52, -0.75},
    {-0.58, -0.76}, {-0.67, -0.81},
    {-0.80, -0.84}, {-0.94, -0.83}, {-1.07, -0.77},
    {-1.18, -0.67}, {-1.25, -0.54}, {-1.30, -0.38},
    {-1.33,  0.00}, {-1.35,  0.18}, {-1.39,  0.23},
    {-1.46,  0.24}, {-1.49,  0.28}, {-1.50,  0.35},
    {-1.54,  0.49}, {-1.58,  0.63}, {-1.58,  0.70},
    {-1.55,  0.76}, {-1.49,  0.80}, {-1.39,  0.83},
    {-1.25,  0.85}, {-1.10,  0.88}, {-0.95,  0.90},
    {-0.87,  0.88}, {-0.81,  0.81},
    {-0.72,  0.68}, {-0.61,  0.51}, {-0.51,  0.35},
    {-0.39,  0.20}, {-0.25,  0.06}, {-0.10, -0.08},
    { 0.05, -0.21}, { 0.18, -0.32}, { 0.26, -0.38},
    { 0.33, -0.39}, { 0.40, -0.36}, { 0.48, -0.35},
    { 0.56, -0.37}, { 0.63, -0.42}, { 0.70, -0.44},
    { 0.91, -0.45}, { 1.14, -0.46}, { 1.36, -0.46},
    { 1.51, -0.47}, { 1.59, -0.50}, { 1.64, -0.56},
    { 1.66, -0.63}, { 1.65, -0.69}, { 1.61, -0.75},
    { 1.54, -0.80}, { 1.43, -0.84}, { 1.30, -0.86},
    { 1.17, -0.87},
});

inline constexpr int samplesPerSegment = 3;

[[nodiscard]] inline std::vector<Vec2> evenlySpacedControls()
{
    std::vector<double> cumulative{0.0};
    for (std::size_t i = 0; i < monzaControls.size(); ++i) {
        const Vec2 a = monzaControls[i];
        const Vec2 b = monzaControls[(i + 1) % monzaControls.size()];
        cumulative.push_back(cumulative.back() + std::hypot(b.x - a.x, b.y - a.y));
    }
    const int count = static_cast<int>(std::ceil(cumulative.back() / 0.05));
    std::vector<Vec2> controls;
    controls.reserve(static_cast<std::size_t>(count));
    for (int i = 0; i < count; ++i) {
        const double target = cumulative.back() * i / count;
        const auto upper = std::upper_bound(cumulative.begin(), cumulative.end(), target);
        const std::size_t segment = static_cast<std::size_t>(upper - cumulative.begin() - 1);
        const double alpha = (target - cumulative[segment]) /
            (cumulative[segment + 1] - cumulative[segment]);
        const Vec2 a = monzaControls[segment];
        const Vec2 b = monzaControls[(segment + 1) % monzaControls.size()];
        controls.push_back({a.x + alpha * (b.x - a.x),
                            a.y + alpha * (b.y - a.y)});
    }
    return controls;
}

[[nodiscard]] inline Vec2 control(const std::vector<Vec2>& controls, int index)
{
    const int count = static_cast<int>(controls.size());
    return controls[static_cast<std::size_t>((index % count + count) % count)];
}

[[nodiscard]] inline Vec2 pathPoint(const std::vector<Vec2>& controls, double u)
{
    const int segment = static_cast<int>(std::floor(u));
    const double t = u - segment;
    return spline::catmullRom(
        control(controls, segment - 1), control(controls, segment),
        control(controls, segment + 1), control(controls, segment + 2), t);
}

[[nodiscard]] inline Vec2 pathTangent(const std::vector<Vec2>& controls, double u)
{
    const int segment = static_cast<int>(std::floor(u));
    const double t = u - segment;
    return spline::catmullRomTangent(
        control(controls, segment - 1), control(controls, segment),
        control(controls, segment + 1), control(controls, segment + 2), t);
}

[[nodiscard]] inline Vec2 stableTangent(const std::vector<Vec2>& controls, double u)
{
    Vec2 tangent = pathTangent(controls, u);
    if (std::hypot(tangent.x, tangent.y) <= 1e-10) {
        const Vec2 before = pathPoint(controls, u - 0.001);
        const Vec2 after = pathPoint(controls, u + 0.001);
        tangent = {after.x - before.x, after.y - before.y};
    }
    return tangent;
}

struct PathSample {
    Vec2 point;
    Vec2 normal;
    double distance;
    double u;
};

[[nodiscard]] inline std::vector<PathSample> makePath()
{
    const std::vector<Vec2> controls = evenlySpacedControls();
    const int segmentCount = static_cast<int>(controls.size());
    const int steps = segmentCount * samplesPerSegment;
    std::vector<PathSample> path;
    path.reserve(static_cast<std::size_t>(steps + 1));
    double distance = 0.0;
    for (int step = 0; step <= steps; ++step) {
        const double u = static_cast<double>(step) / samplesPerSegment;
        const Vec2 point = pathPoint(controls, u);
        const Vec2 tangent = stableTangent(controls, u);
        const double length = std::hypot(tangent.x, tangent.y);
        if (length <= 1e-10) {
            throw std::runtime_error("Monza spline contains a zero tangent at u=" +
                                     std::to_string(u));
        }
        if (!path.empty()) {
            distance += std::hypot(point.x - path.back().point.x,
                                   point.y - path.back().point.y);
        }
        path.push_back({point, {-tangent.y / length, tangent.x / length},
                        distance, u});
    }
    return path;
}

struct Pose {
    Vec2 position;
    double headingRadians;
    double distance;
};

[[nodiscard]] inline Pose poseAtProgress(
    const std::vector<PathSample>& path, double progress)
{
    if (path.size() < 2 || !std::isfinite(progress)) {
        throw std::invalid_argument("A finite progress and a complete path are required");
    }
    const double wrapped = progress - std::floor(progress);
    const double target = wrapped * path.back().distance;
    const auto upper = std::lower_bound(
        path.begin(), path.end(), target,
        [](const PathSample& sample, double value) {
            return sample.distance < value;
        });
    if (upper == path.begin()) {
        return {path.front().point,
                std::atan2(-path.front().normal.x, path.front().normal.y), 0.0};
    }
    const PathSample& after = *upper;
    const PathSample& before = *(upper - 1);
    const double span = after.distance - before.distance;
    const double alpha = span > 0.0 ? (target - before.distance) / span : 0.0;
    const Vec2 point{before.point.x + alpha * (after.point.x - before.point.x),
                     before.point.y + alpha * (after.point.y - before.point.y)};
    const Vec2 normal{before.normal.x + alpha * (after.normal.x - before.normal.x),
                      before.normal.y + alpha * (after.normal.y - before.normal.y)};
    return {point, std::atan2(-normal.x, normal.y), target};
}

inline void triangle(std::vector<Vertex>& mesh, Vec2 a, Vec2 b, Vec2 c, Color color)
{
    mesh.push_back({static_cast<float>(a.x), static_cast<float>(a.y),
                    color.r, color.g, color.b});
    mesh.push_back({static_cast<float>(b.x), static_cast<float>(b.y),
                    color.r, color.g, color.b});
    mesh.push_back({static_cast<float>(c.x), static_cast<float>(c.y),
                    color.r, color.g, color.b});
}

inline void quad(std::vector<Vertex>& mesh, Vec2 a, Vec2 b, Vec2 c, Vec2 d,
                 Color color)
{
    triangle(mesh, a, b, c, color);
    triangle(mesh, a, c, d, color);
}

[[nodiscard]] inline Vec2 offset(const PathSample& sample, double distance)
{
    return {sample.point.x + sample.normal.x * distance,
            sample.point.y + sample.normal.y * distance};
}

inline void ribbonSegment(std::vector<Vertex>& mesh, const PathSample& a,
                          const PathSample& b, double inner, double outer,
                          Color color)
{
    quad(mesh, offset(a, inner), offset(b, inner),
         offset(b, outer), offset(a, outer), color);
}

inline void disk(std::vector<Vertex>& mesh, Vec2 centre, double radius, Color color)
{
    constexpr int sides = 10;
    for (int side = 0; side < sides; ++side) {
        const double first = 2.0 * std::numbers::pi * side / sides;
        const double second = 2.0 * std::numbers::pi * (side + 1) / sides;
        triangle(mesh, centre,
                 {centre.x + radius * std::cos(first),
                  centre.y + radius * std::sin(first)},
                 {centre.x + radius * std::cos(second),
                  centre.y + radius * std::sin(second)}, color);
    }
}

[[nodiscard]] inline std::vector<Vertex> makeTrackMesh(
    const std::vector<PathSample>& path)
{
    if (path.size() < 2) {
        throw std::invalid_argument("Track mesh requires a path");
    }
    constexpr Color shadow{0.025F, 0.038F, 0.049F};
    constexpr Color asphalt{0.17F, 0.195F, 0.225F};
    constexpr Color kerbRed{0.78F, 0.12F, 0.17F};
    constexpr Color kerbWhite{0.88F, 0.91F, 0.91F};
    constexpr Color lane{0.32F, 0.38F, 0.42F};
    std::vector<Vertex> mesh;
    mesh.reserve(path.size() * 3 * 30);
    // Render each full layer before the next. Circular stamps form a clean
    // union around tight chicanes where a normal-offset ribbon would fold.
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        disk(mesh, path[i].point, 0.093, shadow);
    }
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        disk(mesh, path[i].point, 0.083, kerbWhite);
    }
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        if ((i / 6) % 2 == 0) {
            ribbonSegment(mesh, path[i], path[i + 1], -0.083, -0.071, kerbRed);
            ribbonSegment(mesh, path[i], path[i + 1], 0.071, 0.083, kerbRed);
        }
    }
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        disk(mesh, path[i].point, 0.068, asphalt);
    }
    for (std::size_t i = 0; i + 1 < path.size(); ++i) {
        const PathSample& a = path[i];
        const PathSample& b = path[i + 1];
        if (i % 17 < 8) {
            ribbonSegment(mesh, a, b, -0.0015, 0.0015, lane);
        }
    }
    // Chequered start/finish line across the straight.
    const PathSample& start = path.front();
    const Vec2 tangent{start.normal.y, -start.normal.x};
    const double length = std::hypot(tangent.x, tangent.y);
    const Vec2 along{tangent.x / length * 0.012, tangent.y / length * 0.012};
    constexpr Color finishWhite{0.97F, 0.97F, 0.93F};
    constexpr Color finishBlack{0.025F, 0.03F, 0.035F};
    for (int row = 0; row < 2; ++row) {
        for (int column = 0; column < 8; ++column) {
            const double left = -0.064 + column * 0.016;
            const Vec2 p = offset(start, left);
            const Vec2 q = offset(start, left + 0.016);
            const Vec2 rowOffset{along.x * row, along.y * row};
            const Vec2 nextOffset{along.x * (row + 1), along.y * (row + 1)};
            quad(mesh, {p.x + rowOffset.x, p.y + rowOffset.y},
                 {q.x + rowOffset.x, q.y + rowOffset.y},
                 {q.x + nextOffset.x, q.y + nextOffset.y},
                 {p.x + nextOffset.x, p.y + nextOffset.y},
                 (row + column) % 2 == 0 ? finishWhite : finishBlack);
        }
    }
    return mesh;
}

[[nodiscard]] inline Color colorFromCar(car::Color color)
{
    return {color.r / 255.0F, color.g / 255.0F, color.b / 255.0F};
}

[[nodiscard]] inline double signedArea(const std::vector<Vec2>& polygon)
{
    double area = 0.0;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const Vec2 a = polygon[i];
        const Vec2 b = polygon[(i + 1) % polygon.size()];
        area += a.x * b.y - b.x * a.y;
    }
    return area * 0.5;
}

[[nodiscard]] inline bool insideTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c,
                                         double direction)
{
    const auto cross = [](Vec2 first, Vec2 second, Vec2 third) {
        return (second.x - first.x) * (third.y - first.y) -
               (second.y - first.y) * (third.x - first.x);
    };
    return direction * cross(a, b, p) >= -1e-10 &&
           direction * cross(b, c, p) >= -1e-10 &&
           direction * cross(c, a, p) >= -1e-10;
}

inline void triangulate(std::vector<Vertex>& mesh,
                        const std::vector<Vec2>& polygon, Color fill)
{
    if (polygon.size() < 3) {
        return;
    }
    const double direction = signedArea(polygon) >= 0.0 ? 1.0 : -1.0;
    std::vector<std::size_t> remaining;
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        remaining.push_back(i);
    }
    while (remaining.size() > 3) {
        bool foundEar = false;
        for (std::size_t i = 0; i < remaining.size(); ++i) {
            const std::size_t previous = remaining[(i + remaining.size() - 1) % remaining.size()];
            const std::size_t current = remaining[i];
            const std::size_t next = remaining[(i + 1) % remaining.size()];
            const Vec2 a = polygon[previous];
            const Vec2 b = polygon[current];
            const Vec2 c = polygon[next];
            const double cross = (b.x - a.x) * (c.y - a.y) -
                                 (b.y - a.y) * (c.x - a.x);
            if (direction * cross <= 1e-12) {
                continue;
            }
            bool containsVertex = false;
            for (std::size_t index : remaining) {
                if (index != previous && index != current && index != next &&
                    insideTriangle(polygon[index], a, b, c, direction)) {
                    containsVertex = true;
                    break;
                }
            }
            if (!containsVertex) {
                triangle(mesh, a, b, c, fill);
                remaining.erase(remaining.begin() + static_cast<std::ptrdiff_t>(i));
                foundEar = true;
                break;
            }
        }
        if (!foundEar) {
            throw std::runtime_error("Car polygon cannot be triangulated");
        }
    }
    triangle(mesh, polygon[remaining[0]], polygon[remaining[1]],
             polygon[remaining[2]], fill);
}

[[nodiscard]] inline Mat3 carTransform(const Pose& pose,
                                       double scaleMultiplier = 1.0)
{
    constexpr double baseScale = 0.00070;
    const double scale = baseScale * scaleMultiplier;
    // The CP2 model points its nose along local -Y. A quarter turn aligns it
    // with the direction of the circuit tangent.
    return Mat3::translation(pose.position.x, pose.position.y) *
        Mat3::rotationDegrees(pose.headingRadians * 180.0 / std::numbers::pi + 90.0) *
        Mat3::scaling(scale, scale) *
        Mat3::translation(-car::kCarCentre.x, -car::kCarCentre.y);
}

[[nodiscard]] inline std::vector<Vertex> makeCarMesh(
    const std::vector<car::Component>& model, const Pose& pose,
    double scaleMultiplier = 1.0)
{
    const auto placed = car::placeCar(model, carTransform(pose, scaleMultiplier));
    std::vector<Vertex> mesh;
    mesh.reserve(240);
    for (const auto& component : placed) {
        triangulate(mesh, component.polygon, colorFromCar(component.fill));
    }
    return mesh;
}

[[nodiscard]] inline std::vector<Vertex> makeControlMesh()
{
    constexpr Color cyan{0.2F, 0.81F, 0.9F};
    std::vector<Vertex> mesh;
    mesh.reserve(monzaControls.size() * 6);
    for (Vec2 point : monzaControls) {
        quad(mesh, {point.x - 0.006, point.y - 0.006},
             {point.x + 0.006, point.y - 0.006},
             {point.x + 0.006, point.y + 0.006},
             {point.x - 0.006, point.y + 0.006}, cyan);
    }
    return mesh;
}

// ─────────────────────────────────────────────────────────────────────────────
// CP2 Bézier route  (issue #18)
//
// The same 62 Monza centreline anchors are turned into a closed C1-continuous
// chain of cubic Bézier segments using exactly the same handle formula that
// CP2's bezier_motion.hpp uses for its generic circuit:
//
//   h_start = anchor[i] + (anchor[i+1] − anchor[i−1]) / 6
//   h_end   = anchor[i+1] − (anchor[i+2] − anchor[i]) / 6
//
// Every segment interpolates its two anchor endpoints, and the derivative at
// the join between segment i and segment i+1 is the same vector from both
// sides, giving C1 continuity around the whole closed loop.
// ─────────────────────────────────────────────────────────────────────────────

// Wrap-safe accessor for the monzaControls anchor array.
[[nodiscard]] inline Vec2 monzaAnchor(int index)
{
    const int n = static_cast<int>(monzaControls.size());
    return monzaControls[static_cast<std::size_t>((index % n + n) % n)];
}

// Build the full closed Bézier chain from the Monza anchor array.
[[nodiscard]] inline std::vector<bezier::CubicBezier> makeMonzaCircuit()
{
    const int n = static_cast<int>(monzaControls.size());
    std::vector<bezier::CubicBezier> chain;
    chain.reserve(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        const Vec2 prev  = monzaAnchor(i - 1);
        const Vec2 start = monzaAnchor(i);
        const Vec2 end   = monzaAnchor(i + 1);
        const Vec2 after = monzaAnchor(i + 2);
        const Vec2 h1{start.x + (end.x - prev.x) / 6.0,
                      start.y + (end.y - prev.y) / 6.0};
        const Vec2 h2{end.x   - (after.x - start.x) / 6.0,
                      end.y   - (after.y - start.y) / 6.0};
        chain.push_back({start, h1, h2, end});
    }
    return chain;
}

[[nodiscard]] inline const std::vector<bezier::CubicBezier>& monzaCircuit()
{
    static const std::vector<bezier::CubicBezier> value = makeMonzaCircuit();
    return value;
}

// Arc-length table for the Monza Bézier circuit so the car travels at
// approximately constant speed regardless of control-point density.
struct BezierArcSample {
    double cumulativeDistance{};
    double u{}; // continuous parameter: segment index + local t in [0, N)
};

[[nodiscard]] inline std::vector<BezierArcSample> makeMonzaArcTable()
{
    constexpr int stepsPerSegment = 120;
    const auto& chain = monzaCircuit();
    const int n = static_cast<int>(chain.size());
    std::vector<BezierArcSample> table;
    table.reserve(static_cast<std::size_t>(n * stepsPerSegment + 1));
    table.push_back({0.0, 0.0});
    double cumulative = 0.0;
    Vec2 prev = chain[0].p0;
    for (int seg = 0; seg < n; ++seg) {
        const bezier::CubicBezier& curve = chain[static_cast<std::size_t>(seg)];
        for (int step = 1; step <= stepsPerSegment; ++step) {
            const double tVal = static_cast<double>(step) / stepsPerSegment;
            const Vec2 cur = bezier::evaluate(curve, tVal);
            cumulative += std::hypot(cur.x - prev.x, cur.y - prev.y);
            // u is the continuous parameter: an integer segment offset plus
            // the local t so that it increases monotonically across the whole
            // circuit with no discontinuities at segment joins.
            table.push_back({cumulative, static_cast<double>(seg) + tVal});
            prev = cur;
        }
    }
    return table;
}

[[nodiscard]] inline const std::vector<BezierArcSample>& monzaArcTable()
{
    static const std::vector<BezierArcSample> value = makeMonzaArcTable();
    return value;
}

// Sample the Monza Bézier circuit at normalised lap progress in [0,1].
// Returns a PathSample so the rest of the scene is route-agnostic.
//
// Key design: the arc-length table stores a *continuous* parameter u in
// [0, N) where N = number of segments.  Interpolating u across a segment
// boundary is safe because u increases monotonically with no wrap-around
// discontinuity.  After interpolation, u is split with std::modf into an
// integer segment index and a local t in [0, 1].  This eliminates the
// position jump that occurred in the old code when before and after came
// from different segments and their per-segment t values were naively mixed.
[[nodiscard]] inline PathSample bezierSampleAtProgress(double progress)
{
    const auto& table = monzaArcTable();
    const double totalLength = table.back().cumulativeDistance;
    const double wrapped = progress - std::floor(progress);
    const double target  = wrapped * totalLength;

    const auto upper = std::lower_bound(
        table.begin(), table.end(), target,
        [](const BezierArcSample& s, double v) {
            return s.cumulativeDistance < v;
        });

    double u = 0.0;
    if (upper == table.begin()) {
        u = table.front().u;
    } else if (upper == table.end()) {
        u = table.back().u;
    } else {
        const BezierArcSample& after  = *upper;
        const BezierArcSample& before = *(upper - 1);
        const double span  = after.cumulativeDistance - before.cumulativeDistance;
        const double alpha = span > 0.0 ? (target - before.cumulativeDistance) / span : 0.0;
        // Interpolate the *continuous* parameter — safe across segment joins.
        u = before.u + alpha * (after.u - before.u);
    }

    // Split continuous u into segment index + local t.
    const int n = static_cast<int>(monzaCircuit().size());
    double tFrac = 0.0;
    double segFloor = 0.0;
    tFrac = std::modf(u, &segFloor);
    int seg = static_cast<int>(segFloor);
    // Clamp to the last valid segment/t when u lands exactly on the circuit end.
    if (seg >= n) { seg = n - 1; tFrac = 1.0; }
    if (seg < 0)  { seg = 0;     tFrac = 0.0; }

    const auto& chain = monzaCircuit();
    const bezier::CubicBezier& curve = chain[static_cast<std::size_t>(seg)];
    const Vec2 point   = bezier::evaluate(curve, tFrac);
    const Vec2 tangent = bezier::derivative(curve, tFrac);
    const double len   = std::hypot(tangent.x, tangent.y);
    if (len <= 1e-10) {
        throw std::runtime_error("Monza Bezier tangent is degenerate");
    }
    const Vec2 normal{-tangent.y / len, tangent.x / len};
    return {point, normal, target, u};
}

// Build a dense PathSample vector for the Bézier route.
[[nodiscard]] inline std::vector<PathSample> makeBezierPath()
{
    const int n     = static_cast<int>(monzaControls.size());
    const int steps = n * samplesPerSegment;
    std::vector<PathSample> path;
    path.reserve(static_cast<std::size_t>(steps + 1));

    for (int step = 0; step <= steps; ++step) {
        const double progress = static_cast<double>(step) / steps;
        PathSample s = bezierSampleAtProgress(progress);
        if (!path.empty()) {
            const double dx = s.point.x - path.back().point.x;
            const double dy = s.point.y - path.back().point.y;
            s.distance = path.back().distance + std::hypot(dx, dy);
        } else {
            s.distance = 0.0;
        }
        path.push_back(s);
    }
    // Close the loop so poseAtProgress wraps correctly.
    path.back().point  = path.front().point;
    path.back().normal = path.front().normal;
    return path;
}

// Unified factory: returns the path for the active route mode.
[[nodiscard]] inline std::vector<PathSample> makePathForMode(RouteMode mode)
{
    return mode == RouteMode::Bezier ? makeBezierPath() : makePath();
}

// Orange control-point overlay for the Bézier anchors (distinct from
// the cyan Catmull-Rom dots so the user can tell them apart at a glance).
[[nodiscard]] inline std::vector<Vertex> makeBezierAnchorMesh()
{
    constexpr Color orange{1.0F, 0.55F, 0.1F};
    std::vector<Vertex> mesh;
    mesh.reserve(monzaControls.size() * 6);
    for (Vec2 p : monzaControls) {
        quad(mesh, {p.x - 0.007, p.y - 0.007},
             {p.x + 0.007, p.y - 0.007},
             {p.x + 0.007, p.y + 0.007},
             {p.x - 0.007, p.y + 0.007}, orange);
    }
    return mesh;
}

} // namespace apex3d::cp3
