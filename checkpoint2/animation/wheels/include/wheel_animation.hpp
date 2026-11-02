#pragma once

// Formula 1 wheel animation: wheels spin about their own centres while the
// car body carries them along the track (rolling without slipping).
//
// Every moving point is produced by a 3 x 3 homogeneous matrix from
// checkpoint2/transformations; every circle and ellipse is rasterised with the
// Checkpoint 1 midpoint algorithms in raster.hpp.

#include "raster.hpp"

#include <array>
#include <cmath>
#include <cstdio>
#include <numbers>
#include <stdexcept>
#include <string>
#include <vector>

namespace apex3d::wheels {

using raster::Canvas;
using raster::Color;

// ---------------------------------------------------------------------------
// Animation schedule
// ---------------------------------------------------------------------------

inline constexpr int kFrameWidth = 640;
inline constexpr int kFrameHeight = 360;

// Frames 00..16 cover one full revolution, so frame 16 is the 360-degree state.
inline constexpr int kFrameCount = 17;
inline constexpr double kDegreesPerFrame = 360.0 / (kFrameCount - 1);

// Frames used by the contact sheet: 0, 90, 180, 270 and 360 degrees.
inline constexpr std::array<int, 5> kKeyFrames{0, 4, 8, 12, 16};

inline constexpr double kGroundY = 290.0;
inline constexpr double kStartX = 100.0; // rear-wheel centre at frame 0
inline constexpr int kTyreRadius = 36;

namespace palette {
inline constexpr Color background{0x12, 0x14, 0x1A};
inline constexpr Color grid{0x20, 0x27, 0x33};
inline constexpr Color frame{0x33, 0x42, 0x57};
inline constexpr Color asphalt{0x26, 0x2A, 0x33};
inline constexpr Color cyan{0x3E, 0xDB, 0xF2};
inline constexpr Color yellow{0xFF, 0xD1, 0x66};
inline constexpr Color green{0x64, 0xD9, 0xA0};
inline constexpr Color red{0xEF, 0x44, 0x44};
inline constexpr Color tyre{0xBE, 0xC6, 0xD1};
inline constexpr Color rubber{0x1C, 0x1F, 0x26};
inline constexpr Color rim{0x77, 0x92, 0xAA};
inline constexpr Color rimFace{0x2A, 0x34, 0x42};
inline constexpr Color white{0xF2, 0xF5, 0xFA};
inline constexpr Color purple{0xA8, 0x8B, 0xFA};
inline constexpr Color carbon{0x2E, 0x33, 0x3F};
inline constexpr Color body{0xD9, 0x2B, 0x2B};
} // namespace palette

// Wheel angle (degrees, clockwise on screen) for a frame index.
[[nodiscard]] inline double frameAngleDegrees(int frameIndex)
{
    if (frameIndex < 0 || frameIndex >= kFrameCount) {
        throw std::out_of_range("Frame index outside the animation");
    }
    return frameIndex * kDegreesPerFrame;
}

// Rolling without slipping: distance travelled = radius x angle in radians.
[[nodiscard]] inline double rollingDistance(double wheelDegrees)
{
    return kTyreRadius * wheelDegrees * std::numbers::pi / 180.0;
}

// Places the car body. Body-local origin is the rear-wheel centre.
[[nodiscard]] inline Mat3 bodyTransformForAngle(double wheelDegrees)
{
    return Mat3::translation(kStartX + rollingDistance(wheelDegrees),
                             kGroundY - kTyreRadius);
}

// ---------------------------------------------------------------------------
// Wheel: circular tyre, compound band, rim, spokes, hub and marker.
// ---------------------------------------------------------------------------

struct WheelGeometry {
    Vec2 mount{};                // wheel centre in body-local coordinates
    int tyreRadius{kTyreRadius}; // outer tyre circle
    int bandRadius{28};          // red soft-compound band (two rings)
    int rimRadius{22};           // outer rim lip
    int barrelRadius{20};        // inner rim barrel; spokes end here
    int hubRadius{7};
    int nutRadius{3};
    int spokeCount{5};
    int markerOrbit{32};         // compound marker distance from centre
    int markerRadius{2};
};

struct WheelPose {
    Vec2 centre;
    Vec2 tyreReference; // a point on the tyre circle (radius measurement)
    Vec2 rimReference;  // a point on the rim circle
    Vec2 marker;        // compound marker centre
    std::vector<Vec2> spokeInner;
    std::vector<Vec2> spokeOuter;
};

// Unit direction of spoke k in the unrotated model; spoke 0 points up (-y).
[[nodiscard]] inline Vec2 spokeDirection(int spoke, int spokeCount)
{
    if (spoke == 0) {
        return {0.0, -1.0};
    }
    const double radians = 2.0 * std::numbers::pi * spoke / spokeCount;
    return {std::sin(radians), -std::cos(radians)};
}

// Spin about the mount first, then carry the wheel with the body:
//   M = Body * Translate(mount) * Rotate(angle) * Translate(-mount)
[[nodiscard]] inline Mat3 wheelTransform(const WheelGeometry& wheel, const Mat3& body,
                                         double wheelDegrees)
{
    return body * aroundPivot(Mat3::rotationDegrees(wheelDegrees), wheel.mount);
}

[[nodiscard]] inline WheelPose poseWheel(const WheelGeometry& wheel, const Mat3& body,
                                         double wheelDegrees)
{
    const Mat3 m = wheelTransform(wheel, body, wheelDegrees);
    const Vec2 c = wheel.mount;
    const auto at = [&](double dx, double dy) {
        return m.transformPoint({c.x + dx, c.y + dy});
    };

    WheelPose pose;
    pose.centre = m.transformPoint(c);
    pose.tyreReference = at(wheel.tyreRadius, 0.0);
    pose.rimReference = at(wheel.rimRadius, 0.0);
    pose.marker = at(0.0, -wheel.markerOrbit);
    for (int spoke = 0; spoke < wheel.spokeCount; ++spoke) {
        const Vec2 d = spokeDirection(spoke, wheel.spokeCount);
        pose.spokeInner.push_back(at(d.x * wheel.hubRadius, d.y * wheel.hubRadius));
        pose.spokeOuter.push_back(at(d.x * wheel.barrelRadius, d.y * wheel.barrelRadius));
    }
    return pose;
}

// Every transformed point of a wheel pose, for whole-wheel checks.
[[nodiscard]] inline std::vector<Vec2> allPoints(const WheelPose& pose)
{
    std::vector<Vec2> points{pose.centre, pose.tyreReference, pose.rimReference,
                             pose.marker};
    points.insert(points.end(), pose.spokeInner.begin(), pose.spokeInner.end());
    points.insert(points.end(), pose.spokeOuter.begin(), pose.spokeOuter.end());
    return points;
}

[[nodiscard]] inline double distance(Vec2 a, Vec2 b)
{
    return std::hypot(a.x - b.x, a.y - b.y);
}

// Radii measured from the transformed geometry, not copied from the model.
[[nodiscard]] inline double tyreRadiusOf(const WheelPose& pose)
{
    return distance(pose.centre, pose.tyreReference);
}

[[nodiscard]] inline double rimRadiusOf(const WheelPose& pose)
{
    return distance(pose.centre, pose.rimReference);
}

inline void drawWheel(Canvas& canvas, const WheelGeometry& wheel, const WheelPose& pose)
{
    using namespace palette;
    using raster::toPixel;

    const int cx = toPixel(pose.centre.x);
    const int cy = toPixel(pose.centre.y);
    const int tyre = toPixel(tyreRadiusOf(pose));
    const int rimR = toPixel(rimRadiusOf(pose));

    raster::fillCircle(canvas, cx, cy, tyre, rubber);
    raster::drawCircle(canvas, cx, cy, tyre, palette::tyre);
    raster::drawCircle(canvas, cx, cy, wheel.bandRadius, red);
    raster::drawCircle(canvas, cx, cy, wheel.bandRadius + 1, red);
    raster::fillCircle(canvas, cx, cy, rimR, rimFace);
    raster::drawCircle(canvas, cx, cy, rimR, rim);
    raster::drawCircle(canvas, cx, cy, wheel.barrelRadius, rim);

    // Spoke 0 is highlighted so the angle can be read in every frame.
    for (std::size_t spoke = pose.spokeInner.size(); spoke-- > 0;) {
        raster::drawLine(canvas, pose.spokeInner[spoke], pose.spokeOuter[spoke],
                         spoke == 0 ? yellow : rim);
    }

    raster::drawCircle(canvas, cx, cy, wheel.hubRadius, white);
    raster::fillCircle(canvas, cx, cy, wheel.nutRadius, yellow);
    raster::fillCircle(canvas, toPixel(pose.marker.x), toPixel(pose.marker.y),
                       wheel.markerRadius, yellow);
}

inline WheelPose renderWheel(Canvas& canvas, const WheelGeometry& wheel, const Mat3& body,
                             double wheelDegrees)
{
    const WheelPose pose = poseWheel(wheel, body, wheelDegrees);
    drawWheel(canvas, wheel, pose);
    return pose;
}

// ---------------------------------------------------------------------------
// Elliptical telemetry pod: fixed to the chassis (translates, never rotates),
// with a needle that sweeps the inner ellipse in step with the wheel.
// ---------------------------------------------------------------------------

struct TelemetryPod {
    Vec2 centre{110.0, -118.0}; // body-local
    int rx{58};
    int ry{26};
    int innerRx{48};
    int innerRy{18};
    Vec2 tetherTop{110.0, -92.0};
    Vec2 tetherBottom{100.0, -50.0};
};

struct PodPose {
    Vec2 centre;
    Vec2 xAxisTip;      // centre + (rx, 0) after transformation
    Vec2 yAxisTip;      // centre + (0, ry) after transformation
    Vec2 innerXAxisTip;
    Vec2 innerYAxisTip;
    Vec2 needleTip;     // on the inner ellipse at the wheel angle
    Vec2 tetherTop;
    Vec2 tetherBottom;
};

[[nodiscard]] inline PodPose posePod(const TelemetryPod& pod, const Mat3& body,
                                     double wheelDegrees)
{
    const Vec2 c = pod.centre;
    PodPose pose;
    pose.centre = body.transformPoint(c);
    pose.xAxisTip = body.transformPoint({c.x + pod.rx, c.y});
    pose.yAxisTip = body.transformPoint({c.x, c.y + pod.ry});
    pose.innerXAxisTip = body.transformPoint({c.x + pod.innerRx, c.y});
    pose.innerYAxisTip = body.transformPoint({c.x, c.y + pod.innerRy});
    // Unit "12 o'clock" vector rotated, stretched onto the ellipse, then placed.
    const Mat3 needle = body * Mat3::translation(c.x, c.y) *
                        Mat3::scaling(pod.innerRx, pod.innerRy) *
                        Mat3::rotationDegrees(wheelDegrees);
    pose.needleTip = needle.transformPoint({0.0, -1.0});
    pose.tetherTop = body.transformPoint(pod.tetherTop);
    pose.tetherBottom = body.transformPoint(pod.tetherBottom);
    return pose;
}

[[nodiscard]] inline double podRxOf(const PodPose& pose)
{
    return distance(pose.centre, pose.xAxisTip);
}

[[nodiscard]] inline double podRyOf(const PodPose& pose)
{
    return distance(pose.centre, pose.yAxisTip);
}

inline void drawPod(Canvas& canvas, const PodPose& pose, bool withTether = true)
{
    using namespace palette;
    using raster::toPixel;

    const int cx = toPixel(pose.centre.x);
    const int cy = toPixel(pose.centre.y);
    const int rx = toPixel(podRxOf(pose));
    const int ry = toPixel(podRyOf(pose));
    const int innerRx = toPixel(distance(pose.centre, pose.innerXAxisTip));
    const int innerRy = toPixel(distance(pose.centre, pose.innerYAxisTip));

    if (withTether) {
        raster::drawLine(canvas, pose.tetherTop, pose.tetherBottom, frame);
    }
    raster::drawEllipse(canvas, cx, cy, rx, ry, white);
    raster::drawEllipse(canvas, cx, cy, innerRx, innerRy, cyan);
    // Quarter ticks between the two ellipses.
    raster::drawLine(canvas, cx, cy - innerRy - 1, cx, cy - innerRy - 5, rim);
    raster::drawLine(canvas, cx, cy + innerRy + 1, cx, cy + innerRy + 5, rim);
    raster::drawLine(canvas, cx - innerRx - 1, cy, cx - innerRx - 5, cy, rim);
    raster::drawLine(canvas, cx + innerRx + 1, cy, cx + innerRx + 5, cy, rim);
    raster::drawLine(canvas, pose.centre, pose.needleTip, yellow);
    raster::fillCircle(canvas, cx, cy, 2, yellow);
}

inline PodPose renderPod(Canvas& canvas, const TelemetryPod& pod, const Mat3& body,
                         double wheelDegrees, bool withTether = true)
{
    const PodPose pose = posePod(pod, body, wheelDegrees);
    drawPod(canvas, pose, withTether);
    return pose;
}

// ---------------------------------------------------------------------------
// Car body (side view) and the full frame.
// ---------------------------------------------------------------------------

struct CarModel {
    WheelGeometry rearWheel{.mount = {0.0, 0.0}};
    WheelGeometry frontWheel{.mount = {220.0, 0.0}};
    TelemetryPod pod{};

    std::vector<Vec2> chassis{
        {-44, 14}, {-44, -10}, {-20, -16}, {30, -22}, {70, -40}, {92, -50},
        {104, -50}, {112, -34}, {150, -30}, {175, -24}, {260, -8}, {288, -2},
        {288, 6}, {240, 8}, {150, 14},
    };
    std::vector<Vec2> rearWingPillar{{-52, -50}, {-46, -50}, {-46, -12}, {-52, -12}};
    std::vector<Vec2> rearWing{{-68, -64}, {-28, -64}, {-28, -52}, {-68, -52}};
    std::vector<Vec2> frontWing{{246, 12}, {302, 12}, {302, 20}, {246, 20}};
    Vec2 helmet{130.0, -31.0};
    int helmetRx{13};
    int helmetRy{12};
};

struct CarPose {
    Mat3 body;
    double wheelDegrees{};
    WheelPose rear;
    WheelPose front;
    PodPose pod;
};

[[nodiscard]] inline CarPose poseCar(const CarModel& car, double wheelDegrees)
{
    CarPose pose;
    pose.body = bodyTransformForAngle(wheelDegrees);
    pose.wheelDegrees = wheelDegrees;
    pose.rear = poseWheel(car.rearWheel, pose.body, wheelDegrees);
    pose.front = poseWheel(car.frontWheel, pose.body, wheelDegrees);
    pose.pod = posePod(car.pod, pose.body, wheelDegrees);
    return pose;
}

inline void drawCarBody(Canvas& canvas, const CarModel& car, const Mat3& body)
{
    using namespace palette;
    const auto shape = [&](const std::vector<Vec2>& model, Color fill, Color edge) {
        const auto placed = transformPolygon(model, body);
        raster::fillPolygon(canvas, placed, fill);
        raster::outlinePolygon(canvas, placed, edge);
    };
    shape(car.rearWingPillar, carbon, tyre);
    shape(car.rearWing, carbon, tyre);
    shape(car.frontWing, carbon, tyre);
    shape(car.chassis, palette::body, white);

    // Helmet: upper half of a midpoint ellipse, with a visor and halo.
    const Vec2 helmet = body.transformPoint(car.helmet);
    const int hx = raster::toPixel(helmet.x);
    const int hy = raster::toPixel(helmet.y);
    raster::drawEllipse(canvas, hx, hy, car.helmetRx, car.helmetRy, yellow, true);
    raster::drawEllipse(canvas, hx + 3, hy - 3, car.helmetRx - 6, car.helmetRy - 8, cyan,
                        true);
    raster::drawLine(canvas, body.transformPoint({108, -44}),
                     body.transformPoint({160, -38}), tyre);
}

[[nodiscard]] inline std::string formatAngle(double degrees)
{
    std::array<char, 16> text{};
    std::snprintf(text.data(), text.size(), "%.1f", degrees);
    return text.data();
}

inline void drawBackdrop(Canvas& canvas, const CarModel& car)
{
    using namespace palette;
    const int ground = raster::toPixel(kGroundY);
    for (int x = 0; x < kFrameWidth; x += 40) {
        raster::drawLine(canvas, x, 0, x, ground - 1, grid);
    }
    for (int y = 40; y < ground; y += 40) {
        raster::drawLine(canvas, 0, y, kFrameWidth - 1, y, grid);
    }
    canvas.fillRect(0, ground, kFrameWidth - 1, kFrameHeight - 1, asphalt);
    for (int x = 0, block = 0; x < kFrameWidth; x += 16, ++block) {
        canvas.fillRect(x, ground, x + 15, ground + 5, block % 2 == 0 ? red : white);
    }

    // Start and end positions of both wheel centres across one revolution.
    const double revolution = rollingDistance(360.0);
    for (const double mountX : {car.rearWheel.mount.x, car.frontWheel.mount.x}) {
        for (const double x : {kStartX + mountX, kStartX + mountX + revolution}) {
            const int px = raster::toPixel(x);
            for (int y = ground - 2 * kTyreRadius - 30; y < ground; y += 6) {
                raster::drawLine(canvas, px, y, px, y + 2, frame);
            }
        }
    }

    // Dimension line: one revolution moves the car 2*pi*r pixels.
    const int left = raster::toPixel(kStartX);
    const int right = raster::toPixel(kStartX + revolution);
    const int y = ground + 26;
    raster::drawLine(canvas, left, y, right, y, cyan);
    raster::drawLine(canvas, left, y - 5, left, y + 5, cyan);
    raster::drawLine(canvas, right, y - 5, right, y + 5, cyan);
    const std::string label = formatAngle(revolution);
    const int labelWidth = static_cast<int>(label.size()) * 8;
    raster::drawText(canvas, (left + right - labelWidth) / 2, y + 10, label, 2, cyan);
}

inline void drawHud(Canvas& canvas, int frameIndex, double wheelDegrees)
{
    using namespace palette;
    const int end = raster::drawText(canvas, 20, 18, formatAngle(wheelDegrees), 4, white);
    raster::drawCircle(canvas, end + 2, 21, 3, white); // degree sign

    std::array<char, 8> counter{};
    std::snprintf(counter.data(), counter.size(), "%02d/%02d", frameIndex, kFrameCount - 1);
    raster::drawText(canvas, 20, 48, counter.data(), 2, rim);

    const int left = 420;
    const int right = 620;
    raster::drawLine(canvas, left, 22, right, 22, frame);
    raster::drawLine(canvas, left, 34, right, 34, frame);
    raster::drawLine(canvas, left, 22, left, 34, frame);
    raster::drawLine(canvas, right, 22, right, 34, frame);
    const int filled = left + 2 + (right - left - 4) * frameIndex / (kFrameCount - 1);
    canvas.fillRect(left + 2, 24, filled, 32, frameIndex == kFrameCount - 1 ? green : cyan);
}

inline CarPose drawCar(Canvas& canvas, const CarModel& car, double wheelDegrees)
{
    const CarPose pose = poseCar(car, wheelDegrees);
    drawCarBody(canvas, car, pose.body);
    drawWheel(canvas, car.rearWheel, pose.rear);
    drawWheel(canvas, car.frontWheel, pose.front);
    drawPod(canvas, pose.pod);
    return pose;
}

[[nodiscard]] inline Canvas renderFrame(const CarModel& car, int frameIndex)
{
    Canvas canvas(kFrameWidth, kFrameHeight, palette::background);
    const double degrees = frameAngleDegrees(frameIndex);
    drawBackdrop(canvas, car);
    drawCar(canvas, car, degrees);
    drawHud(canvas, frameIndex, degrees);
    return canvas;
}

// Last frame with the rear compound marker and hub traced over every frame:
// the hub follows a straight line, the marker a curtate cycloid.
[[nodiscard]] inline Canvas renderTraceTile(const CarModel& car)
{
    Canvas canvas = renderFrame(car, kFrameCount - 1);
    std::vector<WheelPose> rear;
    for (int frame = 0; frame < kFrameCount; ++frame) {
        rear.push_back(poseCar(car, frameAngleDegrees(frame)).rear);
    }
    for (std::size_t i = 0; i + 1 < rear.size(); ++i) {
        raster::drawLine(canvas, rear[i].marker, rear[i + 1].marker, palette::purple);
        raster::drawLine(canvas, rear[i].centre, rear[i + 1].centre, palette::frame);
    }
    for (const WheelPose& pose : rear) {
        raster::fillCircle(canvas, raster::toPixel(pose.marker.x),
                           raster::toPixel(pose.marker.y), 2, palette::yellow);
        raster::fillCircle(canvas, raster::toPixel(pose.centre.x),
                           raster::toPixel(pose.centre.y), 1, palette::white);
    }
    return canvas;
}

inline constexpr int kSheetGutter = 12;
inline constexpr int kSheetColumns = 3;
inline constexpr int kSheetRows = 2;

// 3 x 2 grid: 0, 90, 180 / 270, 360 degrees and the motion trace.
[[nodiscard]] inline Canvas renderContactSheet(const CarModel& car)
{
    Canvas sheet(kSheetColumns * kFrameWidth + (kSheetColumns + 1) * kSheetGutter,
                 kSheetRows * kFrameHeight + (kSheetRows + 1) * kSheetGutter,
                 palette::frame);
    for (std::size_t tile = 0; tile < kKeyFrames.size() + 1; ++tile) {
        const int column = static_cast<int>(tile) % kSheetColumns;
        const int row = static_cast<int>(tile) / kSheetColumns;
        const int left = kSheetGutter + column * (kFrameWidth + kSheetGutter);
        const int top = kSheetGutter + row * (kFrameHeight + kSheetGutter);
        sheet.blit(tile < kKeyFrames.size() ? renderFrame(car, kKeyFrames[tile])
                                            : renderTraceTile(car),
                   left, top);
    }
    return sheet;
}

} // namespace apex3d::wheels
