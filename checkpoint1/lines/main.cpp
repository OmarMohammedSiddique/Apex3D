// Apex3D - Checkpoint 1: Pure C++ line algorithms.
//
// Rasterizes Monza circuit geometry into an in-memory RGB canvas using
// hand-written DDA, Bresenham and Cohen-Sutherland routines, then saves the
// result as a binary PPM image. No graphics library is used: every pixel is
// written through Canvas::setPixel.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <numbers>
#include <string_view>
#include <vector>

namespace {

struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

// Colour key used throughout the demonstration (see README.md).
constexpr Color kBackground {12, 14, 24};
constexpr Color kWindowFill {24, 28, 46};
constexpr Color kDdaColor {255, 150, 30};       // orange
constexpr Color kBresenhamColor {40, 200, 255}; // cyan
constexpr Color kWindowColor {235, 235, 235};   // white
constexpr Color kOriginalColor {90, 94, 120};   // dim grey
constexpr Color kClippedColor {255, 40, 200};   // magenta
constexpr Color kTextColor {200, 204, 220};

class Canvas {
public:
    Canvas(int width, int height, Color background)
        : width_(width), height_(height),
          pixels_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), background)
    {
    }

    // Writes one pixel. Coordinates outside the canvas are ignored so that no
    // algorithm can write past the end of the pixel buffer.
    bool setPixel(int x, int y, Color color)
    {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) {
            return false;
        }
        pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                static_cast<std::size_t>(x)] = color;
        return true;
    }

    void fillRect(int x0, int y0, int x1, int y1, Color color)
    {
        for (int y = y0; y <= y1; ++y) {
            for (int x = x0; x <= x1; ++x) {
                setPixel(x, y, color);
            }
        }
    }

    // Binary PPM (P6): a short text header followed by raw RGB bytes.
    bool writePpm(const std::filesystem::path& path) const
    {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }
        file << "P6\n" << width_ << ' ' << height_ << "\n255\n";
        for (const Color& pixel : pixels_) {
            const std::array<char, 3> rgb {static_cast<char>(pixel.r), static_cast<char>(pixel.g),
                                           static_cast<char>(pixel.b)};
            file.write(rgb.data(), rgb.size());
        }
        return static_cast<bool>(file);
    }

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }

private:
    int width_;
    int height_;
    std::vector<Color> pixels_;
};

struct Stats {
    int ddaLines = 0;
    int ddaPixels = 0;
    int bresenhamLines = 0;
    int bresenhamPixels = 0;
    int clipAccepted = 0; // trivially inside
    int clipClipped = 0;  // partly inside, shortened
    int clipRejected = 0; // nothing inside
};

Stats gStats;

// ---------------------------------------------------------------------------
// DDA (Digital Differential Analyzer)
//
// Steps along the longer axis one pixel at a time. On every step the other
// coordinate advances by a constant fractional increment (the slope), and the
// floating-point position is rounded to the nearest pixel.
// ---------------------------------------------------------------------------
void drawLineDda(Canvas& canvas, int x0, int y0, int x1, int y1, Color color)
{
    const int dx = x1 - x0;
    const int dy = y1 - y0;
    const int steps = std::max(std::abs(dx), std::abs(dy));

    ++gStats.ddaLines;
    if (steps == 0) {
        gStats.ddaPixels += canvas.setPixel(x0, y0, color) ? 1 : 0;
        return;
    }

    const double xIncrement = static_cast<double>(dx) / steps;
    const double yIncrement = static_cast<double>(dy) / steps;
    double x = x0;
    double y = y0;

    for (int i = 0; i <= steps; ++i) {
        const int px = static_cast<int>(std::lround(x));
        const int py = static_cast<int>(std::lround(y));
        gStats.ddaPixels += canvas.setPixel(px, py, color) ? 1 : 0;
        x += xIncrement;
        y += yIncrement;
    }
}

// ---------------------------------------------------------------------------
// Bresenham (integer, all octants)
//
// err tracks, in integers only, how far the ideal line is from the current
// pixel. Doubling err and comparing it with dy and dx decides whether the next
// pixel moves in x, in y, or diagonally. sx and sy make it work in every
// direction, and the same test handles both shallow and steep slopes.
// ---------------------------------------------------------------------------
void drawLineBresenham(Canvas& canvas, int x0, int y0, int x1, int y1, Color color)
{
    const int dx = std::abs(x1 - x0);
    const int dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;

    ++gStats.bresenhamLines;
    while (true) {
        gStats.bresenhamPixels += canvas.setPixel(x0, y0, color) ? 1 : 0;
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int e2 = 2 * err;
        if (e2 >= dy) { // the error allows a step in x
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) { // the error allows a step in y
            err += dx;
            y0 += sy;
        }
    }
}

// ---------------------------------------------------------------------------
// Cohen-Sutherland line clipping
//
// Each endpoint receives a 4-bit outcode describing where it lies relative to
// the clipping window. Screen coordinates are used, so y grows downward and
// TOP means y < yMin.
// ---------------------------------------------------------------------------
struct ClipWindow {
    double xMin;
    double yMin;
    double xMax;
    double yMax;
};

constexpr int kInside = 0b0000;
constexpr int kLeft = 0b0001;
constexpr int kRight = 0b0010;
constexpr int kBottom = 0b0100;
constexpr int kTop = 0b1000;

int computeOutcode(double x, double y, const ClipWindow& window)
{
    int code = kInside;
    if (x < window.xMin) {
        code |= kLeft;
    } else if (x > window.xMax) {
        code |= kRight;
    }
    if (y < window.yMin) {
        code |= kTop;
    } else if (y > window.yMax) {
        code |= kBottom;
    }
    return code;
}

enum class ClipResult { Accepted, Clipped, Rejected };

// Shortens the segment in place to the part inside the window.
ClipResult clipCohenSutherland(double& x0, double& y0, double& x1, double& y1,
                               const ClipWindow& window)
{
    int code0 = computeOutcode(x0, y0, window);
    int code1 = computeOutcode(x1, y1, window);
    bool changed = false;

    while (true) {
        if ((code0 | code1) == 0) {
            // Both endpoints inside: trivially accept.
            return changed ? ClipResult::Clipped : ClipResult::Accepted;
        }
        if ((code0 & code1) != 0) {
            // Both endpoints share an outside region: trivially reject.
            return ClipResult::Rejected;
        }

        // Pick an endpoint that is outside and move it to the window edge it
        // violates, using the line equation to find the intersection.
        const int outside = code0 != 0 ? code0 : code1;
        double x = 0.0;
        double y = 0.0;
        if ((outside & kTop) != 0) {
            x = x0 + (x1 - x0) * (window.yMin - y0) / (y1 - y0);
            y = window.yMin;
        } else if ((outside & kBottom) != 0) {
            x = x0 + (x1 - x0) * (window.yMax - y0) / (y1 - y0);
            y = window.yMax;
        } else if ((outside & kRight) != 0) {
            y = y0 + (y1 - y0) * (window.xMax - x0) / (x1 - x0);
            x = window.xMax;
        } else {
            y = y0 + (y1 - y0) * (window.xMin - x0) / (x1 - x0);
            x = window.xMin;
        }

        if (outside == code0) {
            x0 = x;
            y0 = y;
            code0 = computeOutcode(x0, y0, window);
        } else {
            x1 = x;
            y1 = y;
            code1 = computeOutcode(x1, y1, window);
        }
        changed = true;
    }
}

// Draws the full segment in its original colour, then clips it and draws the
// surviving part over it in the clipped colour.
void drawClippedSegment(Canvas& canvas, int x0, int y0, int x1, int y1, Color original,
                        const ClipWindow& window)
{
    drawLineBresenham(canvas, x0, y0, x1, y1, original);

    double cx0 = x0;
    double cy0 = y0;
    double cx1 = x1;
    double cy1 = y1;
    switch (clipCohenSutherland(cx0, cy0, cx1, cy1, window)) {
    case ClipResult::Rejected:
        ++gStats.clipRejected;
        return;
    case ClipResult::Accepted:
        ++gStats.clipAccepted;
        break;
    case ClipResult::Clipped:
        ++gStats.clipClipped;
        break;
    }
    drawLineBresenham(canvas, static_cast<int>(std::lround(cx0)), static_cast<int>(std::lround(cy0)),
                      static_cast<int>(std::lround(cx1)), static_cast<int>(std::lround(cy1)),
                      kClippedColor);
}

// ---------------------------------------------------------------------------
// Minimal 3x5 bitmap font so the image can carry its own labels.
// ---------------------------------------------------------------------------
std::array<std::uint8_t, 5> glyph(char c)
{
    switch (c) {
    case 'A': return {0b010, 0b101, 0b111, 0b101, 0b101};
    case 'B': return {0b110, 0b101, 0b110, 0b101, 0b110};
    case 'C': return {0b011, 0b100, 0b100, 0b100, 0b011};
    case 'D': return {0b110, 0b101, 0b101, 0b101, 0b110};
    case 'E': return {0b111, 0b100, 0b110, 0b100, 0b111};
    case 'G': return {0b011, 0b100, 0b101, 0b101, 0b011};
    case 'H': return {0b101, 0b101, 0b111, 0b101, 0b101};
    case 'I': return {0b111, 0b010, 0b010, 0b010, 0b111};
    case 'L': return {0b100, 0b100, 0b100, 0b100, 0b111};
    case 'M': return {0b101, 0b111, 0b111, 0b101, 0b101};
    case 'N': return {0b110, 0b101, 0b101, 0b101, 0b101};
    case 'O': return {0b010, 0b101, 0b101, 0b101, 0b010};
    case 'P': return {0b110, 0b101, 0b110, 0b100, 0b100};
    case 'R': return {0b110, 0b101, 0b110, 0b101, 0b101};
    case 'S': return {0b011, 0b100, 0b010, 0b001, 0b110};
    case 'T': return {0b111, 0b010, 0b010, 0b010, 0b010};
    case 'U': return {0b101, 0b101, 0b101, 0b101, 0b111};
    case 'W': return {0b101, 0b101, 0b111, 0b111, 0b101};
    case 'Z': return {0b111, 0b001, 0b010, 0b100, 0b111};
    case '-': return {0b000, 0b000, 0b111, 0b000, 0b000};
    default: return {0, 0, 0, 0, 0};
    }
}

void drawText(Canvas& canvas, int x, int y, std::string_view text, int scale, Color color)
{
    for (const char c : text) {
        const auto rows = glyph(c);
        for (int row = 0; row < 5; ++row) {
            for (int col = 0; col < 3; ++col) {
                if ((rows[static_cast<std::size_t>(row)] >> (2 - col)) & 1) {
                    canvas.fillRect(x + col * scale, y + row * scale, x + (col + 1) * scale - 1,
                                    y + (row + 1) * scale - 1, color);
                }
            }
        }
        x += 4 * scale;
    }
}

// ---------------------------------------------------------------------------
// Scene data
// ---------------------------------------------------------------------------
struct TrackPoint {
    double x;
    double y;
};

// Same hand-authored Monza centerline as src/main.cpp (normalised coordinates).
constexpr TrackPoint kMonzaCenterline[] = {
    { 1.16, -0.87}, { 0.82, -0.87}, { 0.45, -0.87}, { 0.08, -0.87}, {-0.25, -0.87},
    {-0.46, -0.87}, {-0.49, -0.84}, {-0.49, -0.78}, {-0.52, -0.75}, {-0.58, -0.76},
    {-0.67, -0.81}, {-0.80, -0.84}, {-0.94, -0.83}, {-1.07, -0.77}, {-1.18, -0.67},
    {-1.25, -0.54}, {-1.30, -0.38}, {-1.33,  0.00}, {-1.35,  0.18}, {-1.39,  0.23},
    {-1.46,  0.24}, {-1.49,  0.28}, {-1.50,  0.35}, {-1.54,  0.49}, {-1.58,  0.63},
    {-1.58,  0.70}, {-1.55,  0.76}, {-1.49,  0.80}, {-1.39,  0.83}, {-1.25,  0.85},
    {-1.10,  0.88}, {-0.95,  0.90}, {-0.87,  0.88}, {-0.81,  0.81}, {-0.72,  0.68},
    {-0.61,  0.51}, {-0.51,  0.35}, {-0.39,  0.20}, {-0.25,  0.06}, {-0.10, -0.08},
    { 0.05, -0.21}, { 0.18, -0.32}, { 0.26, -0.38}, { 0.33, -0.39}, { 0.40, -0.36},
    { 0.48, -0.35}, { 0.56, -0.37}, { 0.63, -0.42}, { 0.70, -0.44}, { 0.91, -0.45},
    { 1.14, -0.46}, { 1.36, -0.46}, { 1.51, -0.47}, { 1.59, -0.50}, { 1.64, -0.56},
    { 1.66, -0.63}, { 1.65, -0.69}, { 1.61, -0.75}, { 1.54, -0.80}, { 1.43, -0.84},
    { 1.30, -0.86}, { 1.17, -0.87},
};

// Maps normalised circuit coordinates (y up) to canvas pixels (y down).
struct Pixel {
    int x;
    int y;
};

Pixel toCanvas(TrackPoint point)
{
    constexpr double kScale = 190.0;
    constexpr double kOriginX = 330.0;
    constexpr double kOriginY = 250.0;
    return {static_cast<int>(std::lround(kOriginX + point.x * kScale)),
            static_cast<int>(std::lround(kOriginY - point.y * kScale))};
}

void drawStarburst(Canvas& canvas, int cx, int cy, int radius, bool useDda)
{
    // 24 rays at 15 degree steps cover all eight octants, including the exact
    // horizontal, vertical and 45 degree cases.
    constexpr int kRays = 24;
    for (int i = 0; i < kRays; ++i) {
        const double angle = 2.0 * std::numbers::pi * i / kRays;
        const int x1 = cx + static_cast<int>(std::lround(radius * std::cos(angle)));
        const int y1 = cy - static_cast<int>(std::lround(radius * std::sin(angle)));
        if (useDda) {
            drawLineDda(canvas, cx, cy, x1, y1, kDdaColor);
        } else {
            drawLineBresenham(canvas, cx, cy, x1, y1, kBresenhamColor);
        }
    }
}

void drawLegendEntry(Canvas& canvas, int x, int y, Color color, std::string_view label)
{
    canvas.fillRect(x, y, x + 23, y + 9, color);
    drawText(canvas, x + 34, y, label, 2, kTextColor);
}

std::filesystem::path outputDirectory()
{
    // Prefer the output/ folder next to this source file so the image lands in
    // checkpoint1/lines/output/ when run from the repository root.
    const std::filesystem::path sourceDir = std::filesystem::path(__FILE__).parent_path();
    if (!sourceDir.empty() && std::filesystem::is_directory(sourceDir)) {
        return sourceDir / "output";
    }
    return "output";
}

} // namespace

int main(int argc, char** argv)
{
    Canvas canvas(1000, 620, kBackground);

    // Sector camera window over the Lesmo-to-Ascari descent.
    const ClipWindow window {170.0, 120.0, 420.0, 360.0};
    canvas.fillRect(170, 120, 420, 360, kWindowFill);
    drawLineBresenham(canvas, 170, 120, 420, 120, kWindowColor);
    drawLineBresenham(canvas, 420, 120, 420, 360, kWindowColor);
    drawLineBresenham(canvas, 420, 360, 170, 360, kWindowColor);
    drawLineBresenham(canvas, 170, 360, 170, 120, kWindowColor);

    // 1. Test segments crossing the window in different ways (Cohen-Sutherland).
    struct Segment {
        int x0, y0, x1, y1;
    };
    constexpr Segment kTestSegments[] = {
        {120, 200, 500, 260}, // crosses left and right edges
        {300, 70, 330, 385},  // steep, crosses top and bottom
        {100, 250, 250, 50},  // enters through the left, leaves through the top
        {190, 345, 300, 270}, // fully inside: trivial accept
        {450, 60, 600, 140},  // fully right/top: trivial reject
        {120, 160, 200, 60},  // outside near the corner: rejected after clipping
    };
    for (const Segment& s : kTestSegments) {
        drawClippedSegment(canvas, s.x0, s.y0, s.x1, s.y1, kOriginalColor, window);
    }

    // 2. Monza circuit with Bresenham, each segment also clipped to the window.
    constexpr std::size_t kPointCount = std::size(kMonzaCenterline);
    for (std::size_t i = 0; i < kPointCount; ++i) {
        const Pixel a = toCanvas(kMonzaCenterline[i]);
        const Pixel b = toCanvas(kMonzaCenterline[(i + 1) % kPointCount]);
        drawClippedSegment(canvas, a.x, a.y, b.x, b.y, kBresenhamColor, window);
    }

    // 3. Pit lane and start/finish line with DDA.
    const Pixel pitEntry = toCanvas({1.30, -0.78});
    const Pixel pitExit = toCanvas({-0.30, -0.78});
    const Pixel pitIn = toCanvas({1.45, -0.84});
    const Pixel pitOut = toCanvas({-0.45, -0.85});
    drawLineDda(canvas, pitIn.x, pitIn.y, pitEntry.x, pitEntry.y, kDdaColor);
    drawLineDda(canvas, pitEntry.x, pitEntry.y, pitExit.x, pitExit.y, kDdaColor);
    drawLineDda(canvas, pitExit.x, pitExit.y, pitOut.x, pitOut.y, kDdaColor);
    const Pixel startTop = toCanvas({0.60, -0.81});
    const Pixel startBottom = toCanvas({0.60, -0.93});
    drawLineDda(canvas, startTop.x, startTop.y, startBottom.x, startBottom.y, kDdaColor);

    // 4. Direction and slope comparison: same 24 rays drawn by each algorithm.
    drawText(canvas, 700, 50, "DDA", 3, kDdaColor);
    drawStarburst(canvas, 740, 160, 75, true);
    drawText(canvas, 845, 50, "BRESENHAM", 3, kBresenhamColor);
    drawStarburst(canvas, 905, 160, 75, false);

    // 5. Titles and colour legend.
    drawText(canvas, 30, 20, "MONZA - COHEN-SUTHERLAND", 3, kTextColor);
    drawText(canvas, 174, 366, "CLIP WINDOW", 2, kWindowColor);
    drawLegendEntry(canvas, 30, 470, kDdaColor, "DDA");
    drawLegendEntry(canvas, 30, 495, kBresenhamColor, "BRESENHAM");
    drawLegendEntry(canvas, 30, 520, kWindowColor, "CLIP WINDOW");
    drawLegendEntry(canvas, 30, 545, kOriginalColor, "ORIGINAL SEGMENT");
    drawLegendEntry(canvas, 30, 570, kClippedColor, "CLIPPED RESULT");

    const std::filesystem::path outDir = argc > 1 ? std::filesystem::path(argv[1]) : outputDirectory();
    std::filesystem::create_directories(outDir);
    const std::filesystem::path outFile = outDir / "lines_demo.ppm";
    if (!canvas.writePpm(outFile)) {
        std::cerr << "Failed to write " << outFile << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "Apex3D Checkpoint 1 - Pure C++ line algorithms\n"
              << "  Canvas            : " << canvas.width() << " x " << canvas.height() << " RGB\n"
              << "  DDA               : " << gStats.ddaLines << " lines, " << gStats.ddaPixels
              << " pixels (orange)\n"
              << "  Bresenham         : " << gStats.bresenhamLines << " lines, "
              << gStats.bresenhamPixels << " pixels (cyan; also used for grey, white, magenta)\n"
              << "  Cohen-Sutherland  : " << gStats.clipAccepted << " accepted, " << gStats.clipClipped
              << " clipped, " << gStats.clipRejected << " rejected (magenta = clipped result)\n"
              << "  Output            : " << outFile.string() << '\n';
    return EXIT_SUCCESS;
}
