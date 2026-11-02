// Apex3D - Checkpoint 2: deterministic tests for the polygon car animation.
//
// Build and run from the repository root:
//   c++ -std=c++20 -Wall -Wextra -Wpedantic checkpoint2/animation/car/tests.cpp -o apex3d-car-tests
//   ./apex3d-car-tests

#include "include/car_animation.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {

namespace car = apex3d::car;
using apex3d::Mat3;
using apex3d::Vec2;

constexpr double kTolerance = 1e-9;

[[nodiscard]] bool near(double a, double b) { return std::abs(a - b) <= kTolerance; }
[[nodiscard]] bool near(Vec2 a, Vec2 b) { return near(a.x, b.x) && near(a.y, b.y); }

[[nodiscard]] bool near(const Mat3& a, const Mat3& b)
{
    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            if (!near(a.at(row, column), b.at(row, column))) {
                return false;
            }
        }
    }
    return true;
}

[[nodiscard]] bool near(const car::Pose& a, const car::Pose& b)
{
    return near(a.position, b.position) && near(a.rotationDegrees, b.rotationDegrees) &&
           near(a.scaleX, b.scaleX) && near(a.scaleY, b.scaleY);
}

[[nodiscard]] double distance(Vec2 a, Vec2 b) { return std::hypot(a.x - b.x, a.y - b.y); }

// Screen position of model vertex `vertex` of the component called `name`.
[[nodiscard]] Vec2 placedVertex(int frame, std::string_view name, std::size_t vertex)
{
    for (const auto& component :
         car::placeCar(car::carModel(), car::poseMatrix(car::poseAtFrame(frame)))) {
        if (component.name == name) {
            return component.polygon.at(vertex);
        }
    }
    return {-1.0e9, -1.0e9};
}

int failures = 0;

void check(bool condition, std::string_view message)
{
    if (!condition) {
        ++failures;
        std::cout << "    FAIL: " << message << '\n';
    }
}

// ---------------------------------------------------------------------------

void testFirstPose()
{
    check(near(car::poseAtFrame(0), car::Pose{{110.0, 210.0}, 90.0, 1.0, 1.0}),
          "frame 0 pose is the first keyframe");

    // Chassis nose (-8, -72): minus centre -> (-8, -74); rotate 90 -> (74, -8);
    // translate by (110, 210) -> (184, 202).
    check(near(placedVertex(0, "chassis", 0), {184.0, 202.0}), "frame 0 chassis nose vertex");
    // Front wing corner (-34, -78) -> (-34, -80) -> (80, -34) -> (190, 176).
    check(near(placedVertex(0, "front wing", 0), {190.0, 176.0}), "frame 0 front wing vertex");
}

void testMiddlePose()
{
    constexpr int middle = car::kLastFrame / 2;
    check(middle == 12, "middle frame is 12");
    check(near(car::poseAtFrame(middle), car::Pose{{330.0, 210.0}, 450.0, 1.0, 1.0}),
          "middle pose is the end of the full rotation");
    // 450 degrees is one full turn plus 90, so the shape matches frame 6.
    check(near(placedVertex(middle, "chassis", 0), {404.0, 202.0}), "middle chassis nose vertex");
    check(near(placedVertex(middle, "chassis", 0), placedVertex(6, "chassis", 0)),
          "a 360-degree turn returns the car to its starting orientation");
    check(near(car::poseAtFrame(3).position, {220.0, 210.0}), "frame 3 is halfway along the slide");
}

void testFinalPose()
{
    check(near(car::poseAtFrame(car::kLastFrame), car::Pose{{510.0, 290.0}, 510.0, 1.0, 1.0}),
          "final pose is the last keyframe");

    // 510 degrees = 150 degrees: cos = -sqrt(3)/2, sin = 1/2.
    // (-8, -74) -> (4 sqrt(3) + 37, -4 + 37 sqrt(3)), then + (510, 290).
    const double root3 = std::sqrt(3.0);
    check(near(placedVertex(car::kLastFrame, "chassis", 0),
               {510.0 + 4.0 * root3 + 37.0, 290.0 - 4.0 + 37.0 * root3}),
          "final chassis nose vertex");
}

void testFrameClamping()
{
    check(near(car::poseAtFrame(-5), car::poseAtFrame(0)), "negative frames clamp to frame 0");
    check(near(car::poseAtFrame(99), car::poseAtFrame(car::kLastFrame)),
          "frames past the end clamp to the final frame");
    check(car::kFrameCount >= 16, "at least 16 frames");
    for (const car::Keyframe& key : car::kKeyframes) {
        check(near(car::poseAtFrame(key.frame), key.pose), "interpolation hits every keyframe exactly");
    }
}

void testTranslationPhase()
{
    const auto model = car::carModel();
    const auto start = car::placeCar(model, car::poseMatrix(car::poseAtFrame(0)));
    for (int frame = 1; frame <= 6; ++frame) {
        const car::Pose pose = car::poseAtFrame(frame);
        check(near(pose.rotationDegrees, 90.0) && near(pose.scaleX, 1.0) && near(pose.scaleY, 1.0),
              "translation phase keeps rotation and scale fixed");
        const Vec2 delta{pose.position.x - 110.0, pose.position.y - 210.0};
        const auto moved = car::placeCar(model, car::poseMatrix(pose));
        for (std::size_t c = 0; c < model.size(); ++c) {
            for (std::size_t v = 0; v < model[c].polygon.size(); ++v) {
                check(near(moved[c].polygon[v],
                           {start[c].polygon[v].x + delta.x, start[c].polygon[v].y + delta.y}),
                      "translation moves every vertex of every component by the same offset");
            }
        }
    }
}

void testCentrePivotRotation()
{
    const auto model = car::carModel();
    for (int frame = 6; frame <= 12; ++frame) {
        const car::Pose pose = car::poseAtFrame(frame);
        const Mat3 matrix = car::poseMatrix(pose);

        check(near(matrix.transformPoint(car::kCarCentre), {330.0, 210.0}),
              "rotation keeps the car centre fixed");

        // Same matrix written with the shared aroundPivot() helper:
        // T(p) R T(-c) = T(p - c) * [T(c) R T(-c)].
        const Mat3 viaPivot =
            Mat3::translation(pose.position.x - car::kCarCentre.x,
                              pose.position.y - car::kCarCentre.y) *
            apex3d::aroundPivot(Mat3::rotationDegrees(pose.rotationDegrees), car::kCarCentre);
        check(near(matrix, viaPivot), "pose matrix equals the aroundPivot formulation");

        // Rotation is rigid: every vertex keeps its distance from the pivot.
        for (const auto& component : model) {
            for (Vec2 vertex : component.polygon) {
                check(near(distance(matrix.transformPoint(vertex), pose.position),
                           distance(vertex, car::kCarCentre)),
                      "vertex distance from the pivot is preserved");
            }
        }
    }
    check(near(car::poseAtFrame(7).rotationDegrees - car::poseAtFrame(6).rotationDegrees, 60.0),
          "rotation advances 60 degrees per frame");
}

void testNonUniformScaling()
{
    constexpr int peak = 15;
    const car::Pose pose = car::poseAtFrame(peak);
    check(near(pose.scaleX, 1.6) && near(pose.scaleY, 0.6), "peak scale is (1.6, 0.6)");
    check(near(car::poseMatrix(pose).transformPoint(car::kCarCentre), pose.position),
          "scaling keeps the car centre fixed");
    // Front wing is 68 wide (model x) and 9 deep (model y).
    const Vec2 a = placedVertex(peak, "front wing", 0);
    const Vec2 b = placedVertex(peak, "front wing", 1);
    const Vec2 c = placedVertex(peak, "front wing", 2);
    check(near(distance(a, b), 68.0 * 1.6), "width scaled by 1.6");
    check(near(distance(b, c), 9.0 * 0.6), "length scaled by 0.6");
}

void testVertexCountsUnchanged()
{
    const auto model = car::carModel();
    check(model.size() == 13, "car has 13 polygon components");
    for (int frame = 0; frame < car::kFrameCount; ++frame) {
        const auto placed = car::placeCar(model, car::poseMatrix(car::poseAtFrame(frame)));
        check(placed.size() == model.size(), "no component is lost or added");
        for (std::size_t c = 0; c < model.size(); ++c) {
            check(placed[c].polygon.size() == model[c].polygon.size(),
                  "component vertex count is unchanged");
        }
    }
    check(model[6].name == "chassis" && model[6].polygon.size() == 12, "chassis has 12 vertices");
}

void testEveryComponentSharesTheMatrix()
{
    const auto model = car::carModel();

    // The brief's six component groups are all present.
    std::set<std::string> groups;
    for (const auto& component : model) {
        const std::string name(component.name);
        groups.insert(name.substr(0, name.find(' ')));
    }
    for (const char* group : {"chassis", "front", "rear", "tyre", "stripe", "cockpit", "helmet"}) {
        check(groups.contains(group), "model contains every required component");
    }

    for (int frame = 0; frame < car::kFrameCount; ++frame) {
        const Mat3 expected = car::poseMatrix(car::poseAtFrame(frame));
        const auto placed = car::placeCar(model, expected);
        for (std::size_t c = 0; c < model.size(); ++c) {
            check(near(placed[c].matrix, expected), "component received the frame matrix");
            for (std::size_t v = 0; v < model[c].polygon.size(); ++v) {
                check(near(placed[c].polygon[v], expected.transformPoint(model[c].polygon[v])),
                      "component vertex equals frame matrix * model vertex");
            }
        }
    }
}

void testRenderedFrame()
{
    const car::Canvas frame = car::renderFrame(0);
    check(frame.width() == car::kCanvasWidth && frame.height() == car::kCanvasHeight,
          "frame has the expected size");
    // Pivot marker sits on the car centre (110, 210).
    check(frame.pixel(110, 210) == car::palette::pivot, "pivot marker drawn at the car centre");
    // Model (0, -50) -> (0, -52) -> (52, 0) -> (162, 210): inside the chassis only.
    check(frame.pixel(162, 210) == car::palette::body, "chassis is filled with the body colour");
    check(frame.pixel(400, 100) == car::palette::track, "track is visible away from the car");
}

void testPngSignature()
{
    const car::Canvas tiny(2, 1, car::palette::body);
    const auto bytes = car::png::encode(tiny);
    const std::vector<std::uint8_t> signature{137, 80, 78, 71, 13, 10, 26, 10};
    check(std::equal(signature.begin(), signature.end(), bytes.begin()), "PNG signature");
    // Every PNG ends with the same empty IEND chunk and CRC AE 42 60 82.
    const std::vector<std::uint8_t> iend{0, 0, 0, 0, 'I', 'E', 'N', 'D', 0xAE, 0x42, 0x60, 0x82};
    check(bytes.size() > iend.size() &&
              std::equal(iend.begin(), iend.end(), bytes.end() - static_cast<std::ptrdiff_t>(iend.size())),
          "PNG ends with a valid IEND chunk");
}

} // namespace

int main()
{
    const std::vector<std::pair<std::string_view, std::function<void()>>> tests{
        {"first pose", testFirstPose},
        {"middle pose", testMiddlePose},
        {"final pose", testFinalPose},
        {"frame clamping and keyframes", testFrameClamping},
        {"translation of the complete car", testTranslationPhase},
        {"centre-pivot rotation", testCentrePivotRotation},
        {"non-uniform scaling", testNonUniformScaling},
        {"polygon vertex counts unchanged", testVertexCountsUnchanged},
        {"every component receives the same matrix", testEveryComponentSharesTheMatrix},
        {"rendered frame pixels", testRenderedFrame},
        {"PNG encoder", testPngSignature},
    };

    std::cout << "Apex3D Checkpoint 2 - polygon car animation tests\n";
    for (const auto& [name, test] : tests) {
        const int before = failures;
        test();
        std::cout << (failures == before ? "  PASS  " : "  FAIL  ") << name << '\n';
    }

    if (failures != 0) {
        std::cout << failures << " check(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "All " << tests.size() << " tests passed\n";
    return EXIT_SUCCESS;
}
