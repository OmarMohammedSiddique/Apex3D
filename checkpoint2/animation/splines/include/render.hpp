#pragma once
#include "spline_motion.hpp"
#include "../../wheels/include/raster.hpp"

namespace apex3d::spline {
inline constexpr raster::Color background{12, 18, 29};
inline constexpr raster::Color cyan{60, 215, 240};
inline constexpr raster::Color yellow{255, 214, 65};

inline raster::Canvas render(int frame)
{
    using namespace raster;
    Canvas canvas(560, 360, background);
    for (int x = 0; x < 560; x += 40) drawLine(canvas, x, 35, x, 330, {24, 34, 48});
    for (int y = 50; y < 330; y += 40) drawLine(canvas, 20, y, 540, y, {24, 34, 48});
    for (int i = 0; i < segmentCount; ++i) {
        drawLine(canvas, closedPoint(i), closedPoint(i + 1), {105, 115, 135});
        Vec2 previous = position(i, 0);
        for (int step = 1; step <= 100; ++step) {
            const Vec2 current = position(i, step / 100.0);
            drawLine(canvas, previous, current, cyan);
            previous = current;
        }
        const Vec2 point = closedPoint(i);
        fillCircle(canvas, toPixel(point.x), toPixel(point.y), 3, {255, 100, 110});
    }
    // Local car faces +X. All vertices, including the nose, use the shared matrix.
    const Pose car = framePose(frame);
    const std::vector<Vec2> body{{16, 0}, {6, -7}, {-13, -7}, {-13, 7}, {6, 7}};
    const auto vertices = transformPolygon(body, car.matrix);
    for (std::size_t i = 0; i < vertices.size(); ++i)
        drawLine(canvas, vertices[i], vertices[(i + 1) % vertices.size()], yellow);
    drawLine(canvas, car.matrix.transformPoint({-6, 0}), car.matrix.transformPoint({12, 0}), yellow);
    fillCircle(canvas, toPixel(car.centre.x), toPixel(car.centre.y), 2, {255, 255, 255});
    drawText(canvas, 20, 14, std::to_string(frame), 2, yellow);
    canvas.fillRect(20, 340, 20 + 520 * frame / (frameCount - 1), 344, cyan);
    return canvas;
}
} // namespace apex3d::spline
