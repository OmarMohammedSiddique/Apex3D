#pragma once

#include "cp3_scene.hpp"

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
    double speed, bool playing, bool scaleDemo, RouteMode routeMode)
{
    constexpr Color panel{0.035F, 0.075F, 0.11F};
    constexpr Color panelAccent{0.055F, 0.16F, 0.20F};
    constexpr Color cyan{0.22F, 0.86F, 0.91F};
    constexpr Color orange{1.0F, 0.55F, 0.1F};
    constexpr Color white{0.94F, 0.97F, 0.97F};
    constexpr Color secondary{0.55F, 0.68F, 0.72F};
    const bool isBezier = (routeMode == RouteMode::Bezier);
    const Color routeColor = isBezier ? orange : cyan;
    std::vector<Vertex> mesh;
    hudRectangle(mesh, 0.17, 0.39, 0.95, 0.94, panel);
    hudRectangle(mesh, 0.17, 0.927, 0.95, 0.94, routeColor);
    hudText(mesh, 0.21, 0.88, 0.009, "APEX3D", white);
    hudText(mesh, 0.21, 0.78, 0.0044,
            isBezier ? "MONZA / BEZIER ROUTE" : "MONZA / CATMULL-ROM ROUTE",
            secondary);

    std::ostringstream lap;
    lap << "LAP " << std::setfill('0') << std::setw(2)
        << static_cast<int>(std::clamp(progress, 0.0, 1.0) * 100.0)
        << "%  " << std::fixed << std::setprecision(2) << speed << "X";
    hudText(mesh, 0.21, 0.70, 0.0062, lap.str(), white);
    hudRectangle(mesh, 0.21, 0.605, 0.91, 0.638, panelAccent);
    hudRectangle(mesh, 0.21, 0.605,
                 0.21 + 0.70 * std::clamp(progress, 0.0, 1.0), 0.638, routeColor);
    hudText(mesh, 0.21, 0.57, 0.0046,
            playing ? "PLAYING" : "PAUSED", routeColor);
    hudText(mesh, 0.21, 0.50, 0.0039, "SPACE PAUSE  R RESTART", secondary);
    hudText(mesh, 0.21, 0.46, 0.0039,
            scaleDemo ? "+/- SPEED  T SCALE ON" : "+/- SPEED  T SCALE OFF",
            secondary);
    hudText(mesh, 0.21, 0.42, 0.0039, "B ROUTE  P POINTS  ESC QUIT", secondary);
    return mesh;
}

} // namespace apex3d::cp3
