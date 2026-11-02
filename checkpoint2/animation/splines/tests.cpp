#include <iostream>
#include "include/render.hpp"

namespace {
int failures = 0;
void check(bool condition, const char* name)
{
    if (!condition) { std::cerr << "FAIL: " << name << '\n'; ++failures; }
}
bool near(apex3d::Vec2 a, apex3d::Vec2 b, double tolerance = 1e-9)
{
    return std::hypot(a.x - b.x, a.y - b.y) < tolerance;
}
double angleDifference(double a, double b)
{
    return std::abs(std::remainder(a - b, 2 * std::numbers::pi));
}
}
int main()
{
    using namespace apex3d::spline;
    check(frameCount >= 24, "at least 24 frames");
    for (int i = 0; i < segmentCount; ++i) {
        check(near(position(i, 0), controls[static_cast<std::size_t>(i)]), "segment begins at P1");
        check(near(position(i, 1), controls[static_cast<std::size_t>((i + 1) % segmentCount)]), "segment ends at P2");
        check(near(tangent(i, 1), tangent(i + 1, 0)), "analytic tangents agree at joins");
        for (double t : {0.0, 0.125, 0.5, 0.875, 1.0}) {
            const auto d = tangent(i, t);
            check(std::isfinite(d.x) && std::isfinite(d.y) && std::hypot(d.x, d.y) > 1e-6,
                  "finite non-zero tangent");
            constexpr double h = 1e-5;
            const auto a = position(i, t - h);
            const auto b = position(i, t + h);
            check(near(d, {(b.x - a.x) / (2 * h), (b.y - a.y) / (2 * h)}, 1e-6),
                  "derivative agrees with central difference");
        }
        // Includes the final-to-first join and the atan2 branch cut.
        constexpr double epsilon = 1e-7;
        check(angleDifference(pose(i - epsilon).heading, pose(i + epsilon).heading) < 1e-5,
              "heading continuous across join modulo 2pi");
    }
    check(near(closedPoint(-1), controls.back()), "negative closed index");
    check(near(closedPoint(segmentCount), controls.front()), "positive closed index");
    check(near(closedPoint(-2 * segmentCount - 1), controls.back()), "multiple negative wraps");
    check(near(position(-1, 0.4), position(segmentCount - 1, 0.4)), "wrapped segment evaluation");
    check(near(pose(-0.25).centre, pose(segmentCount - 0.25).centre), "negative lap wrapping");
    check(near(pose(2 * segmentCount + 0.25).centre, pose(0.25).centre), "multiple lap wrapping");
    check(near(framePose(0).centre, {110, 285}), "first frame position");
    check(near(framePose(frameCount - 1).centre, {110, 285}), "final frame completes lap");
    for (int frame = 0; frame < frameCount; ++frame) {
        const auto car = framePose(frame);
        check(near(car.matrix.transformPoint({0, 0}), car.centre), "matrix places centre");
        const double length = std::hypot(car.derivative.x, car.derivative.y);
        check(near(car.matrix.transformPoint({1, 0}),
                   {car.centre.x + car.derivative.x / length, car.centre.y + car.derivative.y / length}),
              "matrix aligns car with analytic tangent");
        const auto canvas = render(frame);
        check(canvas.pixelAt(apex3d::raster::toPixel(car.centre.x),
                             apex3d::raster::toPixel(car.centre.y)) == apex3d::raster::Color{255, 255, 255},
              "rendered centre matches pose");
    }
    if (failures) return 1;
    std::cout << "All spline motion checks passed (8 segments, 33 poses).\n";
}
