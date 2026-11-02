#pragma once

// Pixel canvas and rasterisers for the Checkpoint 2 wheel animation.
//
// drawCircle and drawEllipse are the integer midpoint algorithms merged in
// checkpoint1/circles/main.cpp, kept identical so this animation builds on the
// same raster decisions. They are repeated here because that milestone is a
// single translation unit with its own main().

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../../../transformations/include/transform2d.hpp"

namespace apex3d::raster {

struct Color {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};

    friend bool operator==(const Color&, const Color&) = default;
};
static_assert(sizeof(Color) == 3, "RGB24 requires three bytes per pixel");

// Inclusive pixel bounds of every pixel with a given colour.
struct PixelBounds {
    int left{};
    int top{};
    int right{};
    int bottom{};
    bool empty{true};

    [[nodiscard]] int width() const { return empty ? 0 : right - left + 1; }
    [[nodiscard]] int height() const { return empty ? 0 : bottom - top + 1; }
};

class Canvas {
public:
    Canvas(int width, int height, Color background)
        : width_(width), height_(height),
          pixels_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height),
                  background)
    {
        if (width <= 0 || height <= 0) {
            throw std::invalid_argument("Canvas dimensions must be positive");
        }
    }

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }

    // Every rasteriser writes through here, so all shapes are clipped safely.
    void setPixel(int x, int y, Color color)
    {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) {
            return;
        }
        pixels_[index(x, y)] = color;
    }

    [[nodiscard]] Color pixelAt(int x, int y) const
    {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) {
            throw std::out_of_range("Pixel outside canvas");
        }
        return pixels_[index(x, y)];
    }

    void fillRect(int left, int top, int right, int bottom, Color color)
    {
        for (int y = std::max(0, top); y <= std::min(height_ - 1, bottom); ++y) {
            for (int x = std::max(0, left); x <= std::min(width_ - 1, right); ++x) {
                pixels_[index(x, y)] = color;
            }
        }
    }

    // Copies another canvas with its top-left corner at (left, top).
    void blit(const Canvas& source, int left, int top)
    {
        for (int y = 0; y < source.height(); ++y) {
            for (int x = 0; x < source.width(); ++x) {
                setPixel(left + x, top + y, source.pixelAt(x, y));
            }
        }
    }

    [[nodiscard]] std::size_t countPixels(Color color) const
    {
        return static_cast<std::size_t>(std::count(pixels_.begin(), pixels_.end(), color));
    }

    [[nodiscard]] PixelBounds boundsOf(Color color) const
    {
        PixelBounds bounds{width_, height_, -1, -1, true};
        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                if (pixels_[index(x, y)] == color) {
                    bounds.left = std::min(bounds.left, x);
                    bounds.top = std::min(bounds.top, y);
                    bounds.right = std::max(bounds.right, x);
                    bounds.bottom = std::max(bounds.bottom, y);
                    bounds.empty = false;
                }
            }
        }
        return bounds;
    }

    friend bool operator==(const Canvas&, const Canvas&) = default;

    void writePpm(const std::filesystem::path& path) const
    {
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Could not open output file: " + path.string());
        }
        file << "P6\n" << width_ << ' ' << height_ << "\n255\n";
        file.write(reinterpret_cast<const char*>(pixels_.data()),
                   static_cast<std::streamsize>(pixels_.size() * sizeof(Color)));
        if (!file) {
            throw std::runtime_error("Could not write output file: " + path.string());
        }
    }

private:
    [[nodiscard]] std::size_t index(int x, int y) const
    {
        return static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
               static_cast<std::size_t>(x);
    }

    int width_{};
    int height_{};
    std::vector<Color> pixels_;
};

[[nodiscard]] inline int toPixel(double value)
{
    return static_cast<int>(std::lround(value));
}

// Integer Bresenham line.
inline void drawLine(Canvas& canvas, int x0, int y0, int x1, int y1, Color color)
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

inline void drawLine(Canvas& canvas, Vec2 a, Vec2 b, Color color)
{
    drawLine(canvas, toPixel(a.x), toPixel(a.y), toPixel(b.x), toPixel(b.y), color);
}

// ---------------------------------------------------------------------------
// Midpoint circle (identical decisions to checkpoint1/circles).
// ---------------------------------------------------------------------------

inline void plotCircle8(Canvas& canvas, int xc, int yc, int x, int y, Color color)
{
    canvas.setPixel(xc + x, yc + y, color);
    canvas.setPixel(xc - x, yc + y, color);
    canvas.setPixel(xc + x, yc - y, color);
    canvas.setPixel(xc - x, yc - y, color);
    canvas.setPixel(xc + y, yc + x, color);
    canvas.setPixel(xc - y, yc + x, color);
    canvas.setPixel(xc + y, yc - x, color);
    canvas.setPixel(xc - y, yc - x, color);
}

// Visits every first-octant point chosen by the midpoint decision p0 = 1 - r.
template <typename Visit>
void midpointCircleOctant(int radius, Visit visit)
{
    if (radius < 0) {
        throw std::invalid_argument("Circle radius must be nonnegative");
    }
    int x = 0;
    int y = radius;
    std::int64_t decision = 1LL - radius;
    while (x <= y) {
        visit(x, y);
        ++x;
        if (decision < 0) {
            decision += 2LL * x + 1;
        } else {
            --y;
            decision += 2LL * (x - y) + 1;
        }
    }
}

inline void drawCircle(Canvas& canvas, int xc, int yc, int radius, Color color)
{
    midpointCircleOctant(radius, [&](int x, int y) {
        plotCircle8(canvas, xc, yc, x, y, color);
    });
}

// Solid disc: the same midpoint octant points, joined by horizontal spans.
inline void fillCircle(Canvas& canvas, int xc, int yc, int radius, Color color)
{
    midpointCircleOctant(radius, [&](int x, int y) {
        drawLine(canvas, xc - x, yc + y, xc + x, yc + y, color);
        drawLine(canvas, xc - x, yc - y, xc + x, yc - y, color);
        drawLine(canvas, xc - y, yc + x, xc + y, yc + x, color);
        drawLine(canvas, xc - y, yc - x, xc + y, yc - x, color);
    });
}

// ---------------------------------------------------------------------------
// Midpoint ellipse (identical decisions to checkpoint1/circles).
// ---------------------------------------------------------------------------

inline void plotEllipse4(Canvas& canvas, int xc, int yc, int x, int y, Color color,
                         bool upperHalf)
{
    canvas.setPixel(xc + x, yc - y, color);
    canvas.setPixel(xc - x, yc - y, color);
    if (!upperHalf) {
        canvas.setPixel(xc + x, yc + y, color);
        canvas.setPixel(xc - x, yc + y, color);
    }
}

inline void drawEllipse(Canvas& canvas, int xc, int yc, int rx, int ry, Color color,
                        bool upperHalf = false)
{
    if (rx < 0 || ry < 0) {
        throw std::invalid_argument("Ellipse radii must be nonnegative");
    }
    if (rx == 0 || ry == 0) {
        if (upperHalf && ry != 0) {
            drawLine(canvas, xc, yc - ry, xc, yc, color);
        } else if (rx == 0) {
            drawLine(canvas, xc, yc - ry, xc, yc + ry, color);
        } else {
            drawLine(canvas, xc - rx, yc, xc + rx, yc, color);
        }
        return;
    }

    const std::int64_t rx2 = static_cast<std::int64_t>(rx) * rx;
    const std::int64_t ry2 = static_cast<std::int64_t>(ry) * ry;
    std::int64_t x = 0;
    std::int64_t y = ry;
    std::int64_t dx = 0;           // 2 * ry^2 * x
    std::int64_t dy = 2 * rx2 * y; // 2 * rx^2 * y

    std::int64_t decision = 4 * ry2 - 4 * rx2 * y + rx2;
    while (dx < dy) { // Region 1: |dy/dx| < 1
        plotEllipse4(canvas, xc, yc, static_cast<int>(x), static_cast<int>(y), color,
                     upperHalf);
        ++x;
        dx += 2 * ry2;
        if (decision < 0) {
            decision += 4 * dx + 4 * ry2;
        } else {
            --y;
            dy -= 2 * rx2;
            decision += 4 * dx + 4 * ry2 - 4 * dy;
        }
    }

    const std::int64_t twiceXPlusOne = 2 * x + 1;
    const std::int64_t yMinusOne = y - 1;
    decision = ry2 * twiceXPlusOne * twiceXPlusOne + 4 * rx2 * yMinusOne * yMinusOne -
               4 * rx2 * ry2;
    while (y >= 0) { // Region 2: |dy/dx| >= 1
        plotEllipse4(canvas, xc, yc, static_cast<int>(x), static_cast<int>(y), color,
                     upperHalf);
        --y;
        dy -= 2 * rx2;
        if (decision > 0) {
            decision += 4 * rx2 - 4 * dy;
        } else {
            ++x;
            dx += 2 * ry2;
            decision += 4 * dx + 4 * rx2 - 4 * dy;
        }
    }
}

// ---------------------------------------------------------------------------
// Polygons (scanline fill at pixel centres, Bresenham outline).
// ---------------------------------------------------------------------------

inline void fillPolygon(Canvas& canvas, const std::vector<Vec2>& polygon, Color color)
{
    if (polygon.size() < 3) {
        return;
    }
    double minimumY = polygon.front().y;
    double maximumY = polygon.front().y;
    for (Vec2 point : polygon) {
        minimumY = std::min(minimumY, point.y);
        maximumY = std::max(maximumY, point.y);
    }
    for (int y = static_cast<int>(std::floor(minimumY));
         y <= static_cast<int>(std::ceil(maximumY)); ++y) {
        const double scanY = y + 0.5;
        std::vector<double> crossings;
        for (std::size_t i = 0; i < polygon.size(); ++i) {
            const Vec2 a = polygon[i];
            const Vec2 b = polygon[(i + 1) % polygon.size()];
            if (scanY < std::min(a.y, b.y) || scanY >= std::max(a.y, b.y)) {
                continue;
            }
            const double t = (scanY - a.y) / (b.y - a.y);
            crossings.push_back(a.x + t * (b.x - a.x));
        }
        std::sort(crossings.begin(), crossings.end());
        for (std::size_t i = 0; i + 1 < crossings.size(); i += 2) {
            const int x0 = static_cast<int>(std::ceil(crossings[i] - 0.5));
            const int x1 = static_cast<int>(std::floor(crossings[i + 1] - 0.5));
            for (int x = x0; x <= x1; ++x) {
                canvas.setPixel(x, y, color);
            }
        }
    }
}

inline void outlinePolygon(Canvas& canvas, const std::vector<Vec2>& polygon, Color color)
{
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        drawLine(canvas, polygon[i], polygon[(i + 1) % polygon.size()], color);
    }
}

// ---------------------------------------------------------------------------
// 3 x 5 bitmap glyphs for the frame HUD: digits, '.', '/' and space.
// ---------------------------------------------------------------------------

[[nodiscard]] inline const std::array<std::uint8_t, 5>* glyphRows(char character)
{
    // Each row stores three bits, most significant bit on the left.
    static constexpr std::array<std::array<std::uint8_t, 5>, 12> kGlyphs{{
        {7, 5, 5, 5, 7}, // 0
        {2, 6, 2, 2, 7}, // 1
        {7, 1, 7, 4, 7}, // 2
        {7, 1, 7, 1, 7}, // 3
        {5, 5, 7, 1, 1}, // 4
        {7, 4, 7, 1, 7}, // 5
        {7, 4, 7, 5, 7}, // 6
        {7, 1, 1, 1, 1}, // 7
        {7, 5, 7, 5, 7}, // 8
        {7, 5, 7, 1, 7}, // 9
        {0, 0, 0, 0, 2}, // .
        {1, 1, 2, 4, 4}, // /
    }};
    if (character >= '0' && character <= '9') {
        return &kGlyphs[static_cast<std::size_t>(character - '0')];
    }
    if (character == '.') {
        return &kGlyphs[10];
    }
    if (character == '/') {
        return &kGlyphs[11];
    }
    return nullptr;
}

// Returns the x coordinate just after the drawn text.
inline int drawText(Canvas& canvas, int left, int top, std::string_view text, int scale,
                    Color color)
{
    int x = left;
    for (char character : text) {
        if (const auto* rows = glyphRows(character)) {
            for (int row = 0; row < 5; ++row) {
                for (int column = 0; column < 3; ++column) {
                    if (((*rows)[static_cast<std::size_t>(row)] >> (2 - column)) & 1U) {
                        canvas.fillRect(x + column * scale, top + row * scale,
                                        x + (column + 1) * scale - 1,
                                        top + (row + 1) * scale - 1, color);
                    }
                }
            }
        }
        x += 4 * scale;
    }
    return x;
}

} // namespace apex3d::raster
