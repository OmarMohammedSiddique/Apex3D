#include "include/render.hpp"

#include <cmath>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <string_view>

namespace {

constexpr double tolerance = 1e-8;
int failures = 0;

[[nodiscard]] bool near(double a, double b, double epsilon = tolerance)
{
    return std::abs(a - b) <= epsilon;
}

[[nodiscard]] bool near(apex3d::Vec2 a, apex3d::Vec2 b, double epsilon = tolerance)
{
    return std::hypot(a.x - b.x, a.y - b.y) <= epsilon;
}

void check(std::string_view name, bool condition)
{
    std::cout << "  [test] " << name << ": " << (condition ? "PASS" : "FAIL") << '\n';
    if (!condition) {
        ++failures;
    }
}

} // namespace

int main()
{
    using namespace apex3d::bezier;

    std::cout << "Apex3D Bezier motion tests:\n";
    const auto& route = circuit();

    bool endpointInterpolation = true;
    bool derivativeEndpoints = true;
    bool joinPositionContinuity = true;
    bool joinTangentContinuity = true;
    for (int index = 0; index < segmentCount; ++index) {
        const CubicBezier& curve = route[static_cast<std::size_t>(index)];
        const CubicBezier& next = route[static_cast<std::size_t>((index + 1) % segmentCount)];
        endpointInterpolation = endpointInterpolation && near(evaluate(curve, 0.0), curve.p0) &&
            near(evaluate(curve, 1.0), curve.p3);
        derivativeEndpoints = derivativeEndpoints &&
            near(derivative(curve, 0.0), (curve.p1 - curve.p0) * 3.0) &&
            near(derivative(curve, 1.0), (curve.p3 - curve.p2) * 3.0);
        joinPositionContinuity = joinPositionContinuity && near(curve.p3, next.p0);
        joinTangentContinuity = joinTangentContinuity &&
            near(derivative(curve, 1.0), derivative(next, 0.0));
    }
    check("cubic endpoints interpolate P0 and P3", endpointInterpolation);
    check("analytic derivative matches endpoint formulas", derivativeEndpoints);
    check("closed chain has C0 continuity", joinPositionContinuity);
    check("closed chain has C1 continuity", joinTangentContinuity);

    const auto& table = arcLengthTable();
    bool increasing = table.totalLength() > 0.0;
    for (std::size_t index = 1; index < table.samples().size(); ++index) {
        increasing = increasing &&
            table.samples()[index].cumulativeDistance >=
                table.samples()[index - 1].cumulativeDistance;
    }
    check("arc-length lookup is monotonic", increasing);
    check("arc-length lookup covers the entire circuit",
          near(table.samples().back().u, static_cast<double>(segmentCount)) &&
              near(table.samples().back().cumulativeDistance, table.totalLength()));

    const Pose first = framePose(0);
    const Pose last = framePose(frameCount - 1);
    check("final frame closes the lap", near(first.centre, last.centre));
    check("final heading closes continuously", near(first.tangent, last.tangent));

    double minimumStep = std::numeric_limits<double>::max();
    double maximumStep = 0.0;
    for (int frame = 1; frame < frameCount; ++frame) {
        const double step = distance(framePose(frame - 1).centre, framePose(frame).centre);
        minimumStep = std::min(minimumStep, step);
        maximumStep = std::max(maximumStep, step);
    }
    check("arc-length animation keeps frame travel approximately uniform",
          minimumStep > 0.0 && maximumStep / minimumStep < 1.12);

    bool matricesPlaceAndOrient = true;
    bool finitePoses = true;
    bool renderedCentres = true;
    for (int frame = 0; frame < frameCount; ++frame) {
        const Pose pose = framePose(frame);
        const double tangentLength = std::hypot(pose.tangent.x, pose.tangent.y);
        const apex3d::Vec2 forward = pose.matrix.transformPoint({1.0, 0.0});
        matricesPlaceAndOrient = matricesPlaceAndOrient &&
            near(pose.matrix.transformPoint({0.0, 0.0}), pose.centre) &&
            near(forward,
                 {pose.centre.x + pose.tangent.x / tangentLength,
                  pose.centre.y + pose.tangent.y / tangentLength});
        finitePoses = finitePoses && std::isfinite(pose.centre.x) &&
            std::isfinite(pose.centre.y) && std::isfinite(pose.headingRadians);
        const auto canvas = render(frame);
        renderedCentres = renderedCentres &&
            canvas.pixelAt(apex3d::raster::toPixel(pose.centre.x),
                           apex3d::raster::toPixel(pose.centre.y)) == centreColor;
    }
    check("shared Mat3 places and orients every car pose", matricesPlaceAndOrient);
    check("every animation pose is finite", finitePoses);
    check("rendered centre pixel agrees with every pose", renderedCentres);

    bool rejectsNonFinite = false;
    try {
        static_cast<void>(poseAtProgress(std::numeric_limits<double>::quiet_NaN()));
    } catch (const std::invalid_argument&) {
        rejectsNonFinite = true;
    }
    check("non-finite animation progress is rejected", rejectsNonFinite);

    bool rejectsBadFrame = false;
    try {
        static_cast<void>(framePose(frameCount));
    } catch (const std::out_of_range&) {
        rejectsBadFrame = true;
    }
    check("out-of-range frame index is rejected", rejectsBadFrame);

    if (failures != 0) {
        std::cerr << failures << " Bezier motion checks failed.\n";
        return EXIT_FAILURE;
    }
    std::cout << "  All Bezier motion checks passed (" << segmentCount
              << " segments, " << frameCount << " poses).\n";
    return EXIT_SUCCESS;
}
