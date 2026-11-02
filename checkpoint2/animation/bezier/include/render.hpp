#pragma once

#include <string>
#include <vector>

#include "bezier_motion.hpp"
#include "../../wheels/include/raster.hpp"

namespace apex3d::bezier {

inline constexpr raster::Color background{10, 16, 27};
inline constexpr raster::Color grid{23, 33, 48};
inline constexpr raster::Color controlLine{83, 96, 118};
inline constexpr raster::Color anchorColor{255, 96, 105};
inline constexpr raster::Color pathColor{54, 214, 239};
inline constexpr raster::Color carColor{255, 210, 55};
inline constexpr raster::Color centreColor{255, 255, 255};
inline constexpr raster::Color traceColor{91, 120, 158};

inline void drawCircuit(raster::Canvas& canvas)
{
    using namespace raster;
    for (const CubicBezier& segment : circuit()) {
        drawLine(canvas, segment.p0, segment.p1, controlLine);
        drawLine(canvas, segment.p2, segment.p3, controlLine);
        Vec2 previous = evaluate(segment, 0.0);
        for (int step = 1; step <= 120; ++step) {
            const Vec2 current = evaluate(segment, step / 120.0);
            drawLine(canvas, previous, current, pathColor);
            previous = current;
        }
    }
    for (Vec2 anchor : anchors) {
        fillCircle(canvas, toPixel(anchor.x), toPixel(anchor.y), 3, anchorColor);
    }
}

inline void drawCar(raster::Canvas& canvas, const Pose& pose)
{
    using namespace raster;
    const std::vector<Vec2> body{{18, 0}, {7, -7}, {-12, -7}, {-16, -4},
                                 {-16, 4}, {-12, 7}, {7, 7}};
    const auto world = transformPolygon(body, pose.matrix);
    for (std::size_t index = 0; index < world.size(); ++index) {
        drawLine(canvas, world[index], world[(index + 1) % world.size()], carColor);
    }
    drawLine(canvas, pose.matrix.transformPoint({-13, -10}),
             pose.matrix.transformPoint({-13, 10}), carColor);
    drawLine(canvas, pose.matrix.transformPoint({10, -9}),
             pose.matrix.transformPoint({10, 9}), carColor);
    drawLine(canvas, pose.matrix.transformPoint({-7, 0}),
             pose.matrix.transformPoint({14, 0}), carColor);
    fillCircle(canvas, toPixel(pose.centre.x), toPixel(pose.centre.y), 2, centreColor);
}

[[nodiscard]] inline raster::Canvas render(int frame)
{
    using namespace raster;
    Canvas canvas(560, 360, background);
    for (int x = 0; x < canvas.width(); x += 40) {
        drawLine(canvas, x, 35, x, 330, grid);
    }
    for (int y = 50; y < 330; y += 40) {
        drawLine(canvas, 20, y, 540, y, grid);
    }
    drawCircuit(canvas);

    for (int previousFrame = 0; previousFrame < frame; ++previousFrame) {
        const Pose previous = framePose(previousFrame);
        fillCircle(canvas, toPixel(previous.centre.x), toPixel(previous.centre.y), 1,
                   traceColor);
    }

    const Pose pose = framePose(frame);
    drawCar(canvas, pose);
    drawText(canvas, 20, 14, "BEZIER " + std::to_string(frame), 2, carColor);
    canvas.fillRect(20, 340, 20 + 520 * frame / (frameCount - 1), 344, pathColor);
    return canvas;
}

} // namespace apex3d::bezier
