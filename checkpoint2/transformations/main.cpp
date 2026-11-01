#include "include/transform2d.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

struct Color {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
};

namespace Palette {
constexpr Color background{9, 12, 20};
constexpr Color panel{19, 24, 35};
constexpr Color track{62, 64, 73};
constexpr Color grid{86, 88, 99};
constexpr Color body{220, 28, 40};
constexpr Color bodyGhost{95, 100, 116};
constexpr Color tyre{10, 10, 13};
constexpr Color wheelStripe{250, 210, 35};
constexpr Color cockpit{18, 20, 25};
constexpr Color helmet{255, 215, 35};
constexpr Color wing{238, 240, 245};
constexpr Color outline{250, 250, 252};
constexpr Color translated{55, 220, 255};
constexpr Color rotated{255, 175, 45};
constexpr Color scaled{65, 225, 125};
constexpr Color combined{210, 95, 255};
constexpr Color pivot{255, 75, 95};
} // namespace Palette

class Canvas {
public:
    Canvas(int width, int height, Color background)
        : width_(width), height_(height),
          pixels_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), background)
    {
    }

    void setPixel(int x, int y, Color color)
    {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) {
            return;
        }
        pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                static_cast<std::size_t>(x)] = color;
    }

    void fillRect(int x0, int y0, int x1, int y1, Color color)
    {
        for (int y = std::max(0, y0); y <= std::min(height_ - 1, y1); ++y) {
            for (int x = std::max(0, x0); x <= std::min(width_ - 1, x1); ++x) {
                setPixel(x, y, color);
            }
        }
    }

    [[nodiscard]] bool writePpm(const std::filesystem::path& path) const
    {
        std::ofstream output(path, std::ios::binary);
        if (!output) {
            return false;
        }
        output << "P6\n" << width_ << ' ' << height_ << "\n255\n";
        for (Color color : pixels_) {
            const std::array<char, 3> bytes{
                static_cast<char>(color.r),
                static_cast<char>(color.g),
                static_cast<char>(color.b),
            };
            output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
        return static_cast<bool>(output);
    }

private:
    int width_{};
    int height_{};
    std::vector<Color> pixels_;
};

void drawLine(Canvas& canvas, int x0, int y0, int x1, int y1, Color color)
{
    const int dx = std::abs(x1 - x0);
    const int dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    for (;;) {
        canvas.setPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int doubled = 2 * error;
        if (doubled >= dy) {
            error += dy;
            x0 += sx;
        }
        if (doubled <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

void drawDisk(Canvas& canvas, apex3d::Vec2 centre, int radius, Color color)
{
    const int cx = static_cast<int>(std::lround(centre.x));
    const int cy = static_cast<int>(std::lround(centre.y));
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                canvas.setPixel(cx + x, cy + y, color);
            }
        }
    }
}

void fillPolygon(Canvas& canvas, const std::vector<apex3d::Vec2>& polygon, Color color)
{
    if (polygon.size() < 3) {
        return;
    }
    double minimumY = polygon.front().y;
    double maximumY = polygon.front().y;
    for (apex3d::Vec2 point : polygon) {
        minimumY = std::min(minimumY, point.y);
        maximumY = std::max(maximumY, point.y);
    }

    for (int y = static_cast<int>(std::floor(minimumY));
         y <= static_cast<int>(std::ceil(maximumY)); ++y) {
        const double scanY = y + 0.5;
        std::vector<double> intersections;
        for (std::size_t i = 0; i < polygon.size(); ++i) {
            const apex3d::Vec2 a = polygon[i];
            const apex3d::Vec2 b = polygon[(i + 1) % polygon.size()];
            const double low = std::min(a.y, b.y);
            const double high = std::max(a.y, b.y);
            if (scanY < low || scanY >= high || std::abs(a.y - b.y) < 1e-12) {
                continue;
            }
            const double t = (scanY - a.y) / (b.y - a.y);
            intersections.push_back(a.x + t * (b.x - a.x));
        }
        std::sort(intersections.begin(), intersections.end());
        for (std::size_t i = 0; i + 1 < intersections.size(); i += 2) {
            const int x0 = static_cast<int>(std::ceil(intersections[i] - 0.5));
            const int x1 = static_cast<int>(std::floor(intersections[i + 1] - 0.5));
            for (int x = x0; x <= x1; ++x) {
                canvas.setPixel(x, y, color);
            }
        }
    }
}

void outlinePolygon(Canvas& canvas, const std::vector<apex3d::Vec2>& polygon, Color color)
{
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const apex3d::Vec2 a = polygon[i];
        const apex3d::Vec2 b = polygon[(i + 1) % polygon.size()];
        drawLine(canvas, static_cast<int>(std::lround(a.x)), static_cast<int>(std::lround(a.y)),
                 static_cast<int>(std::lround(b.x)), static_cast<int>(std::lround(b.y)), color);
    }
}

void drawShape(Canvas& canvas, const std::vector<apex3d::Vec2>& model,
               const apex3d::Mat3& transformation, Color fill, Color outline)
{
    const auto transformed = apex3d::transformPolygon(model, transformation);
    fillPolygon(canvas, transformed, fill);
    outlinePolygon(canvas, transformed, outline);
}

[[nodiscard]] std::vector<apex3d::Vec2> rectangle(double x0, double y0, double x1, double y1)
{
    return {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
}

void drawCar(Canvas& canvas, const apex3d::Mat3& transformation, Color bodyColor)
{
    const std::vector<apex3d::Vec2> chassis{
        {-8, -72}, {8, -72}, {12, -35}, {25, -18}, {27, 22},
        {18, 52}, {12, 78}, {-12, 78}, {-18, 52}, {-27, 22},
        {-25, -18}, {-12, -35},
    };
    const auto frontWing = rectangle(-34, -78, 34, -69);
    const auto rearWing = rectangle(-31, 72, 31, 82);
    const auto frontLeft = rectangle(-38, -48, -24, -13);
    const auto frontRight = rectangle(24, -48, 38, -13);
    const auto rearLeft = rectangle(-40, 31, -24, 69);
    const auto rearRight = rectangle(24, 31, 40, 69);
    const auto cockpit = rectangle(-10, -8, 10, 29);
    const auto helmet = rectangle(-6, -1, 6, 13);

    for (const auto& tyre : {frontLeft, frontRight, rearLeft, rearRight}) {
        drawShape(canvas, tyre, transformation, Palette::tyre, Palette::outline);
    }
    drawShape(canvas, frontWing, transformation, Palette::wing, Palette::outline);
    drawShape(canvas, rearWing, transformation, Palette::wing, Palette::outline);
    drawShape(canvas, chassis, transformation, bodyColor, Palette::outline);
    drawShape(canvas, cockpit, transformation, Palette::cockpit, Palette::outline);
    drawShape(canvas, helmet, transformation, Palette::helmet, Palette::helmet);

    for (const double y : {-30.0, 51.0}) {
        const auto stripeLeft = rectangle(-38, y - 1.5, -24, y + 1.5);
        const auto stripeRight = rectangle(24, y - 1.5, 38, y + 1.5);
        drawShape(canvas, stripeLeft, transformation, Palette::wheelStripe, Palette::wheelStripe);
        drawShape(canvas, stripeRight, transformation, Palette::wheelStripe, Palette::wheelStripe);
    }
}

void drawPanel(Canvas& canvas, int left, int right, Color header)
{
    canvas.fillRect(left, 45, right, 650, Palette::panel);
    canvas.fillRect(left, 45, right, 60, header);
    canvas.fillRect(left + 12, 75, right - 12, 630, Palette::track);
    const int centreX = (left + right) / 2;
    drawLine(canvas, centreX, 75, centreX, 630, Palette::grid);
    for (int y = 110; y < 630; y += 65) {
        drawLine(canvas, left + 12, y, right - 12, y, Palette::grid);
    }
}

int main(int argc, char* argv[])
{
    using apex3d::Mat3;
    using apex3d::Vec2;

    constexpr int width = 1280;
    constexpr int height = 720;
    Canvas canvas(width, height, Palette::background);

    constexpr std::array<int, 6> edges{20, 264, 508, 752, 996, 1260};
    const std::array<Color, 5> headers{
        Palette::outline, Palette::translated, Palette::rotated,
        Palette::scaled, Palette::combined,
    };
    for (std::size_t i = 0; i < headers.size(); ++i) {
        drawPanel(canvas, edges[i], edges[i + 1] - 8, headers[i]);
    }

    const std::array<Vec2, 5> centres{
        Vec2{138, 355}, Vec2{382, 355}, Vec2{626, 355},
        Vec2{870, 355}, Vec2{1114, 355},
    };

    // Original model.
    drawCar(canvas, Mat3::translation(centres[0].x, centres[0].y), Palette::body);

    // Translation: grey source pose and cyan translated result.
    drawCar(canvas, Mat3::translation(centres[1].x, centres[1].y), Palette::bodyGhost);
    drawCar(canvas, Mat3::translation(centres[1].x + 38, centres[1].y - 75), Palette::translated);
    drawLine(canvas, static_cast<int>(centres[1].x), static_cast<int>(centres[1].y),
             static_cast<int>(centres[1].x + 38), static_cast<int>(centres[1].y - 75),
             Palette::translated);

    // Rotation around the car centre.
    const Mat3 rotate = Mat3::translation(centres[2].x, centres[2].y) *
                        Mat3::rotationDegrees(42.0);
    drawCar(canvas, rotate, Palette::rotated);
    drawDisk(canvas, centres[2], 5, Palette::pivot);

    // Non-uniform scaling around the local origin before placement.
    const Mat3 scale = Mat3::translation(centres[3].x, centres[3].y) *
                       Mat3::scaling(1.45, 0.72);
    drawCar(canvas, scale, Palette::scaled);

    // Combined transform: scale, then rotate, then translate.
    const Mat3 combined = Mat3::translation(centres[4].x + 5, centres[4].y - 15) *
                          Mat3::rotationDegrees(-28.0) *
                          Mat3::scaling(0.82, 1.18);
    drawCar(canvas, combined, Palette::combined);

    // Bottom visual key in the same left-to-right order as the panels.
    for (std::size_t i = 0; i < headers.size(); ++i) {
        canvas.fillRect(edges[i] + 25, 675, edges[i + 1] - 35, 696, headers[i]);
    }

    const std::filesystem::path outputDirectory = argc > 1
        ? std::filesystem::path(argv[1])
        : std::filesystem::path(__FILE__).parent_path() / "output";
    std::filesystem::create_directories(outputDirectory);
    const std::filesystem::path outputPath = outputDirectory / "transformations_demo.ppm";
    if (!canvas.writePpm(outputPath)) {
        std::cerr << "ERROR: failed to write " << outputPath << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "Apex3D Checkpoint 2 - 2D Transformations\n"
              << "  Canvas         : " << width << " x " << height << " RGB\n"
              << "  Demonstrations : original, translation, rotation, scaling, combined\n"
              << "  Matrix model   : 3 x 3 homogeneous coordinates\n"
              << "  Output         : " << outputPath.string() << '\n';
    return EXIT_SUCCESS;
}
