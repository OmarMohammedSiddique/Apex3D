#pragma once

#include "cp3_scene.hpp"
#include "../../checkpoint2/animation/wheels/include/wheel_animation.hpp"

#include <algorithm>
#include <array>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace apex3d::cp3 {

[[nodiscard]] inline std::array<unsigned char, 7> glyph(char letter)
{
    switch (letter) {
    case 'A': return {14, 17, 17, 31, 17, 17, 17};
    case 'B': return {30, 17, 17, 30, 17, 17, 30};
    case 'C': return {15, 16, 16, 16, 16, 16, 15};
    case 'D': return {30, 17, 17, 17, 17, 17, 30};
    case 'E': return {31, 16, 16, 30, 16, 16, 31};
    case 'F': return {31, 16, 16, 30, 16, 16, 16};
    case 'G': return {15, 16, 16, 19, 17, 17, 15};
    case 'H': return {17, 17, 17, 31, 17, 17, 17};
    case 'I': return {31, 4, 4, 4, 4, 4, 31};
    case 'J': return {7, 2, 2, 2, 18, 18, 12};
    case 'K': return {17, 18, 20, 24, 20, 18, 17};
    case 'L': return {16, 16, 16, 16, 16, 16, 31};
    case 'M': return {17, 27, 21, 21, 17, 17, 17};
    case 'N': return {17, 25, 21, 19, 17, 17, 17};
    case 'O': return {14, 17, 17, 17, 17, 17, 14};
    case 'P': return {30, 17, 17, 30, 16, 16, 16};
    case 'Q': return {14, 17, 17, 17, 21, 18, 13};
    case 'R': return {30, 17, 17, 30, 20, 18, 17};
    case 'S': return {15, 16, 16, 14, 1, 1, 30};
    case 'T': return {31, 4, 4, 4, 4, 4, 4};
    case 'U': return {17, 17, 17, 17, 17, 17, 14};
    case 'V': return {17, 17, 17, 17, 17, 10, 4};
    case 'W': return {17, 17, 17, 21, 21, 21, 10};
    case 'X': return {17, 17, 10, 4, 10, 17, 17};
    case 'Y': return {17, 17, 10, 4, 4, 4, 4};
    case 'Z': return {31, 1, 2, 4, 8, 16, 31};
    case '0': return {14, 17, 19, 21, 25, 17, 14};
    case '1': return {4, 12, 4, 4, 4, 4, 14};
    case '2': return {14, 17, 1, 2, 4, 8, 31};
    case '3': return {30, 1, 1, 14, 1, 1, 30};
    case '4': return {2, 6, 10, 18, 31, 2, 2};
    case '5': return {31, 16, 16, 30, 1, 1, 30};
    case '6': return {14, 16, 16, 30, 17, 17, 14};
    case '7': return {31, 1, 2, 4, 8, 8, 8};
    case '8': return {14, 17, 17, 14, 17, 17, 14};
    case '9': return {14, 17, 17, 15, 1, 1, 14};
    case '/': return {1, 1, 2, 4, 8, 16, 16};
    case '+': return {0, 4, 4, 31, 4, 4, 0};
    case '-': return {0, 0, 0, 31, 0, 0, 0};
    case '.': return {0, 0, 0, 0, 0, 12, 12};
    case '%': return {17, 2, 4, 4, 8, 16, 17};
    default: return {0, 0, 0, 0, 0, 0, 0};
    }
}

inline void hudText(std::vector<Vertex>& mesh, double x, double top,
                    double pixel, const std::string& text, Color color)
{
    for (char letter : text) {
        const auto rows = glyph(letter);
        for (int row = 0; row < 7; ++row) {
            for (int column = 0; column < 5; ++column) {
                if ((rows[static_cast<std::size_t>(row)] & (1 << (4 - column))) == 0) {
                    continue;
                }
                const double left = x + column * pixel;
                const double high = top - row * pixel;
                quad(mesh, {left, high}, {left + pixel, high},
                     {left + pixel, high - pixel}, {left, high - pixel}, color);
            }
        }
        x += 6 * pixel;
    }
}

inline void hudRectangle(std::vector<Vertex>& mesh, double left, double bottom,
                         double right, double top, Color color)
{
    quad(mesh, {left, bottom}, {right, bottom},
         {right, top}, {left, top}, color);
}

[[nodiscard]] inline std::vector<Vertex> makeHudMesh(double progress,
    double speed, bool playing, bool scaleDemo,
    Route route = Route::Monza, bool showWheels = true, double travelled = 0.0,
    double aspect = 16.0 / 9.0)
{
    constexpr Color panel{0.035F, 0.075F, 0.11F};
    constexpr Color panelAccent{0.055F, 0.16F, 0.20F};
    constexpr Color cyan{0.22F, 0.86F, 0.91F};
    constexpr Color white{0.94F, 0.97F, 0.97F};
    constexpr Color secondary{0.55F, 0.68F, 0.72F};
    std::vector<Vertex> mesh;
    hudRectangle(mesh, 0.17, 0.35, 0.95, 0.94, panel);
    hudRectangle(mesh, 0.17, 0.927, 0.95, 0.94, cyan);
    hudText(mesh, 0.21, 0.88, 0.009, "APEX3D", white);
    hudText(mesh, 0.21, 0.78, 0.0044, routeName(route), secondary);

    std::ostringstream lap;
    lap << "LAP " << std::setfill('0') << std::setw(2)
        << static_cast<int>(std::clamp(progress, 0.0, 1.0) * 100.0)
        << "%  " << std::fixed << std::setprecision(2) << speed << "X";
    hudText(mesh, 0.21, 0.70, 0.0062, lap.str(), white);
    hudRectangle(mesh, 0.21, 0.605, 0.91, 0.638, panelAccent);
    hudRectangle(mesh, 0.21, 0.605,
                 0.21 + 0.70 * std::clamp(progress, 0.0, 1.0), 0.638, cyan);
    hudText(mesh, 0.21, 0.57, 0.0046,
            playing ? "PLAYING" : "PAUSED", cyan);
    hudText(mesh, 0.21, 0.50, 0.0039, "SPACE PAUSE  R RESTART", secondary);
    hudText(mesh, 0.21, 0.46, 0.0039,
            scaleDemo ? "+/- SPEED  T SCALE ON" : "+/- SPEED  T SCALE OFF", secondary);
    hudText(mesh, 0.21, 0.42, 0.0039, "P POINTS  B ROUTE  ESC QUIT", secondary);
    hudText(mesh, 0.21, 0.38, 0.0039,
            showWheels ? "W WHEEL VIEW ON" : "W WHEEL VIEW OFF", secondary);
    if (showWheels) {
        hudRectangle(mesh, 0.17, 0.02, 0.95, 0.33, panel);
        hudText(mesh, 0.21, 0.315, 0.0042, "LIVE WHEEL HIERARCHY", white);
        hudText(mesh, 0.21, 0.274, 0.0033, "BODY TRANSLATION + LOCAL ROTATION", secondary);
        // One revolution repeats in this inset; spin and parent displacement
        // both come from travelled distance, including across lap boundaries.
        const double degrees = std::fmod(travelled / 0.00070 /
            wheels::kTyreRadius * 180.0 / std::numbers::pi, 360.0);
        // Cap the inset aspect correction on very wide windows so the car
        // stays inside its panel; ordinary 16:9 wheels remain circular.
        const double wheelAspect = std::clamp(aspect, 0.5, 1.8);
        constexpr double scale = 0.0008;
        const Mat3 view = Mat3::translation(0.28, 0.104 + 290.0 * scale * wheelAspect) *
            Mat3::scaling(scale, -scale * wheelAspect);
        const Mat3 body = view * wheels::bodyTransformForAngle(degrees);
        const wheels::CarModel model;
        triangulate(mesh, transformPolygon(model.chassis, body), {0.85F, 0.17F, 0.17F});
        triangulate(mesh, transformPolygon(model.rearWing, body), secondary);
        triangulate(mesh, transformPolygon(model.frontWing, body), secondary);
        hudRectangle(mesh, 0.21, 0.100, 0.91, 0.104, secondary);
        const auto wheelDisk = [&](Vec2 centre, double radius, Color fill) {
            const std::size_t begin = mesh.size();
            disk(mesh, centre, radius, fill);
            for (std::size_t i = begin; i < mesh.size(); ++i) {
                mesh[i].y = static_cast<float>(centre.y + (mesh[i].y - centre.y) * wheelAspect);
            }
        };
        for (const auto& wheel : {model.rearWheel, model.frontWheel}) {
            const auto pose = wheels::poseWheel(wheel, body, degrees);
            wheelDisk(pose.centre, wheel.tyreRadius * scale, {0.012F, 0.018F, 0.025F});
            wheelDisk(pose.centre, wheel.rimRadius * scale, secondary);
            wheelDisk(pose.centre, 0.0056, white);
            wheelDisk(pose.marker, 0.0032, {1.0F, 0.8F, 0.25F});
            for (std::size_t i = 0; i < pose.spokeOuter.size(); ++i) {
                const Vec2 a = pose.spokeInner[i];
                const Vec2 b = pose.spokeOuter[i];
                const Vec2 normal{-(b.y - a.y) * 0.10, (b.x - a.x) * 0.10};
                quad(mesh, {a.x + normal.x, a.y + normal.y},
                     {b.x + normal.x, b.y + normal.y},
                     {b.x - normal.x, b.y - normal.y},
                     {a.x - normal.x, a.y - normal.y}, cyan);
            }
        }
        hudText(mesh, 0.21, 0.07, 0.0033, "DISTANCE = RADIUS X ANGLE", secondary);
    }
    return mesh;
}

} // namespace apex3d::cp3
