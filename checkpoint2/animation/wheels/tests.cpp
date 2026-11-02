#include "include/wheel_animation.hpp"

#include <cmath>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

using apex3d::Mat3;
using apex3d::Vec2;
using apex3d::raster::Canvas;
namespace wheels = apex3d::wheels;
namespace palette = apex3d::wheels::palette;

constexpr double kTolerance = 1e-9;

[[nodiscard]] bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) <= kTolerance;
}

[[nodiscard]] bool nearlyEqual(Vec2 a, Vec2 b)
{
    return nearlyEqual(a.x, b.x) && nearlyEqual(a.y, b.y);
}

[[nodiscard]] Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }

[[nodiscard]] bool samePixel(Vec2 a, Vec2 b)
{
    using apex3d::raster::toPixel;
    return toPixel(a.x) == toPixel(b.x) && toPixel(a.y) == toPixel(b.y);
}

// Test wheel mounted at a body-local point that is not the origin, so a
// pivot error (rotating about the origin) would move the centre.
const wheels::WheelGeometry kWheel{.mount = {150.0, 120.0}};

[[nodiscard]] wheels::WheelPose poseAt(double degrees, const Mat3& body = Mat3::identity())
{
    return wheels::poseWheel(kWheel, body, degrees);
}

[[nodiscard]] Canvas wheelCanvas(double degrees, const Mat3& body = Mat3::identity())
{
    Canvas canvas(400, 300, palette::background);
    wheels::renderWheel(canvas, kWheel, body, degrees);
    return canvas;
}

[[nodiscard]] Canvas podCanvas(const Mat3& body, double degrees)
{
    // Wide enough that a full revolution of travel never clips the pod.
    Canvas canvas(640, 300, palette::background);
    wheels::renderPod(canvas, wheels::TelemetryPod{.centre = {150.0, 120.0}}, body,
                      degrees, false);
    return canvas;
}

class TestRunner {
public:
    void check(std::string_view name, const std::function<bool()>& test)
    {
        bool passed = false;
        try {
            passed = test();
        } catch (const std::exception& error) {
            std::cout << "    exception: " << error.what() << '\n';
        }
        std::cout << "  [test] " << name << ": " << (passed ? "PASS" : "FAIL") << '\n';
        failed_ = failed_ || !passed;
    }

    [[nodiscard]] bool passed() const { return !failed_; }

private:
    bool failed_{};
};

} // namespace

int main()
{
    std::cout << "Apex3D wheel animation tests:\n";
    TestRunner tests;
    const Vec2 c = kWheel.mount;
    const double orbit = kWheel.markerOrbit;
    const double barrel = kWheel.barrelRadius;
    const double tyre = kWheel.tyreRadius;

    tests.check("frame schedule covers 0, 90, 180, 270 and 360 degrees", [] {
        const std::array<double, 5> expected{0.0, 90.0, 180.0, 270.0, 360.0};
        for (std::size_t i = 0; i < expected.size(); ++i) {
            if (!nearlyEqual(wheels::frameAngleDegrees(wheels::kKeyFrames[i]), expected[i])) {
                return false;
            }
        }
        return wheels::kFrameCount >= 16;
    });

    tests.check("0-degree rotation leaves every wheel point unchanged", [&] {
        const auto pose = poseAt(0.0);
        if (!nearlyEqual(pose.centre, c) ||
            !nearlyEqual(pose.marker, {c.x, c.y - orbit}) ||
            !nearlyEqual(pose.spokeOuter[0], {c.x, c.y - barrel}) ||
            !nearlyEqual(pose.tyreReference, {c.x + tyre, c.y})) {
            return false;
        }
        // Rotation matrix is the identity at 0 degrees.
        const Mat3 m = wheels::wheelTransform(kWheel, Mat3::identity(), 0.0);
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                if (!nearlyEqual(m.at(row, column), row == column ? 1.0 : 0.0)) {
                    return false;
                }
            }
        }
        const Canvas canvas = wheelCanvas(0.0);
        return canvas.pixelAt(static_cast<int>(c.x), static_cast<int>(c.y - orbit)) ==
               palette::yellow;
    });

    tests.check("90-degree rotation moves the marker from top to right", [&] {
        const auto pose = poseAt(90.0);
        return nearlyEqual(pose.marker, {c.x + orbit, c.y}) &&
               nearlyEqual(pose.spokeOuter[0], {c.x + barrel, c.y}) &&
               nearlyEqual(pose.tyreReference, {c.x, c.y + tyre}) &&
               wheelCanvas(90.0).pixelAt(static_cast<int>(c.x + orbit),
                                         static_cast<int>(c.y)) == palette::yellow;
    });

    tests.check("180-degree rotation moves the marker from top to bottom", [&] {
        const auto pose = poseAt(180.0);
        return nearlyEqual(pose.marker, {c.x, c.y + orbit}) &&
               nearlyEqual(pose.spokeOuter[0], {c.x, c.y + barrel}) &&
               nearlyEqual(pose.tyreReference, {c.x - tyre, c.y}) &&
               wheelCanvas(180.0).pixelAt(static_cast<int>(c.x),
                                          static_cast<int>(c.y + orbit)) == palette::yellow;
    });

    tests.check("270-degree rotation moves the marker from top to left", [&] {
        const auto pose = poseAt(270.0);
        return nearlyEqual(pose.marker, {c.x - orbit, c.y}) &&
               nearlyEqual(pose.spokeOuter[0], {c.x - barrel, c.y});
    });

    tests.check("360-degree rotation returns to the initial coordinates", [&] {
        const auto start = allPoints(poseAt(0.0));
        const auto end = allPoints(poseAt(360.0));
        if (start.size() != end.size()) {
            return false;
        }
        for (std::size_t i = 0; i < start.size(); ++i) {
            if (!nearlyEqual(start[i], end[i]) || !samePixel(start[i], end[i])) {
                return false;
            }
        }
        return wheelCanvas(0.0) == wheelCanvas(360.0);
    });

    tests.check("360-degree frame matches frame 0 apart from translation", [] {
        const wheels::CarModel car;
        const auto first = wheels::poseCar(car, wheels::frameAngleDegrees(0));
        const auto last = wheels::poseCar(car, wheels::frameAngleDegrees(wheels::kFrameCount - 1));
        const Vec2 shift{wheels::rollingDistance(360.0), 0.0};
        for (const auto& [a, b] : {std::pair{first.rear, last.rear},
                                   std::pair{first.front, last.front}}) {
            const auto from = allPoints(a);
            const auto to = allPoints(b);
            for (std::size_t i = 0; i < from.size(); ++i) {
                if (!nearlyEqual(from[i] + shift, to[i])) {
                    return false;
                }
            }
        }
        return true;
    });

    tests.check("pivot stays fixed at the wheel centre for every angle", [&] {
        for (double degrees = -720.0; degrees <= 720.0; degrees += 7.5) {
            if (!nearlyEqual(poseAt(degrees).centre, c)) {
                return false;
            }
        }
        // Inside the animation, the hub only ever moves horizontally with the body.
        const wheels::CarModel car;
        for (int frame = 0; frame < wheels::kFrameCount; ++frame) {
            const auto pose = wheels::poseCar(car, wheels::frameAngleDegrees(frame));
            if (!nearlyEqual(pose.rear.centre, pose.body.transformPoint(car.rearWheel.mount)) ||
                !nearlyEqual(pose.front.centre, pose.body.transformPoint(car.frontWheel.mount)) ||
                !nearlyEqual(pose.rear.centre.y, wheels::kGroundY - wheels::kTyreRadius)) {
                return false;
            }
        }
        return true;
    });

    tests.check("pivot pixel keeps the hub colour while the wheel spins", [&] {
        for (int frame = 0; frame < wheels::kFrameCount; ++frame) {
            const Canvas canvas = wheelCanvas(wheels::frameAngleDegrees(frame));
            if (canvas.pixelAt(static_cast<int>(c.x), static_cast<int>(c.y)) !=
                palette::yellow) {
                return false;
            }
        }
        return true;
    });

    tests.check("translation moves every wheel point by the same vector", [&] {
        for (const double degrees : {0.0, 37.0, 90.0, 213.5, 360.0}) {
            for (const Vec2 t : {Vec2{15.0, -7.0}, Vec2{-42.25, 3.5}, Vec2{226.19, 0.0}}) {
                const auto still = allPoints(poseAt(degrees));
                const auto moved = allPoints(poseAt(degrees, Mat3::translation(t.x, t.y)));
                for (std::size_t i = 0; i < still.size(); ++i) {
                    if (!nearlyEqual(still[i] + t, moved[i])) {
                        return false;
                    }
                }
            }
        }
        return true;
    });

    tests.check("translation shifts the whole rendered wheel", [&] {
        constexpr int tx = 15;
        constexpr int ty = -7;
        const Canvas still = wheelCanvas(37.0);
        const Canvas moved = wheelCanvas(37.0, Mat3::translation(tx, ty));
        if (still.countPixels(palette::background) != moved.countPixels(palette::background)) {
            return false;
        }
        for (int y = 0; y < still.height(); ++y) {
            for (int x = 0; x < still.width(); ++x) {
                if (still.pixelAt(x, y) == palette::background) {
                    continue;
                }
                if (moved.pixelAt(x + tx, y + ty) != still.pixelAt(x, y)) {
                    return false;
                }
            }
        }
        return true;
    });

    tests.check("car body, both wheels and telemetry pod share each frame step", [] {
        const wheels::CarModel car;
        for (int frame = 1; frame < wheels::kFrameCount; ++frame) {
            const auto before = wheels::poseCar(car, wheels::frameAngleDegrees(frame - 1));
            const auto after = wheels::poseCar(car, wheels::frameAngleDegrees(frame));
            const Vec2 step{wheels::rollingDistance(wheels::kDegreesPerFrame), 0.0};
            if (!nearlyEqual(before.rear.centre + step, after.rear.centre) ||
                !nearlyEqual(before.front.centre + step, after.front.centre) ||
                !nearlyEqual(before.pod.centre + step, after.pod.centre) ||
                !nearlyEqual(before.body.transformPoint({0, 0}) + step,
                             after.body.transformPoint({0, 0}))) {
                return false;
            }
        }
        return true;
    });

    tests.check("circle radii stay unchanged in every frame", [] {
        const wheels::CarModel car;
        for (int frame = 0; frame < wheels::kFrameCount; ++frame) {
            const auto pose = wheels::poseCar(car, wheels::frameAngleDegrees(frame));
            for (const auto& [geometry, wheel] : {std::pair{car.rearWheel, pose.rear},
                                                  std::pair{car.frontWheel, pose.front}}) {
                if (!nearlyEqual(tyreRadiusOf(wheel), geometry.tyreRadius) ||
                    !nearlyEqual(rimRadiusOf(wheel), geometry.rimRadius) ||
                    !nearlyEqual(wheels::distance(wheel.centre, wheel.marker),
                                 geometry.markerOrbit)) {
                    return false;
                }
            }
        }
        return true;
    });

    tests.check("rendered tyre and rim circles keep their diameters", [&] {
        for (const double degrees : {0.0, 22.5, 90.0, 157.5, 270.0, 360.0}) {
            for (const Vec2 t : {Vec2{0.0, 0.0}, Vec2{14.137, 0.0}, Vec2{-31.4, 9.6}}) {
                const Canvas canvas = wheelCanvas(degrees, Mat3::translation(t.x, t.y));
                const auto tyreBox = canvas.boundsOf(palette::tyre);
                const auto rimBox = canvas.boundsOf(palette::rim);
                if (tyreBox.width() != 2 * kWheel.tyreRadius + 1 ||
                    tyreBox.height() != 2 * kWheel.tyreRadius + 1 ||
                    rimBox.width() != 2 * kWheel.rimRadius + 1 ||
                    rimBox.height() != 2 * kWheel.rimRadius + 1) {
                    return false;
                }
            }
        }
        return true;
    });

    tests.check("ellipse radii stay unchanged when the car translates", [] {
        const wheels::TelemetryPod pod{.centre = {150.0, 120.0}};
        for (int frame = 0; frame < wheels::kFrameCount; ++frame) {
            const double degrees = wheels::frameAngleDegrees(frame);
            const Mat3 body = Mat3::translation(wheels::rollingDistance(degrees), 3.25);
            const auto pose = wheels::posePod(pod, body, degrees);
            if (!nearlyEqual(wheels::podRxOf(pose), pod.rx) ||
                !nearlyEqual(wheels::podRyOf(pose), pod.ry)) {
                return false;
            }
            const auto outer = podCanvas(body, degrees).boundsOf(palette::white);
            const auto inner = podCanvas(body, degrees).boundsOf(palette::cyan);
            if (outer.width() != 2 * pod.rx + 1 || outer.height() != 2 * pod.ry + 1 ||
                inner.width() != 2 * pod.innerRx + 1 || inner.height() != 2 * pod.innerRy + 1) {
                return false;
            }
        }
        return true;
    });

    tests.check("telemetry needle stays on the inner ellipse", [] {
        const wheels::TelemetryPod pod{};
        for (double degrees = 0.0; degrees <= 360.0; degrees += 22.5) {
            const auto pose = wheels::posePod(pod, Mat3::translation(40.0, 25.0), degrees);
            const double u = (pose.needleTip.x - pose.centre.x) / pod.innerRx;
            const double v = (pose.needleTip.y - pose.centre.y) / pod.innerRy;
            if (!nearlyEqual(u * u + v * v, 1.0)) {
                return false;
            }
        }
        return true;
    });

    tests.check("rolling distance equals radius times angle", [] {
        return nearlyEqual(wheels::rollingDistance(360.0),
                           2.0 * std::numbers::pi * wheels::kTyreRadius) &&
               nearlyEqual(wheels::rollingDistance(90.0),
                           std::numbers::pi * wheels::kTyreRadius / 2.0);
    });

    if (!tests.passed()) {
        std::cerr << "Wheel animation tests failed.\n";
        return EXIT_FAILURE;
    }
    std::cout << "  All wheel animation tests passed.\n";
    return EXIT_SUCCESS;
}
