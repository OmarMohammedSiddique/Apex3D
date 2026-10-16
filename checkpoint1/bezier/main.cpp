// Apex3D - Checkpoint 1: Cubic Bezier Curves in Pure C++20
//
// Demonstrates cubic Bezier mathematics applied to Formula 1 circuit geometry
// (Monza-inspired racing line smoothing). No graphics library is used. Every
// pixel is written through Canvas::setPixel and the finished canvas is saved
// as a binary PPM image.
//
// Mathematical foundation:
//   B(t) = (1-t)^3*P0 + 3(1-t)^2*t*P1 + 3(1-t)*t^2*P2 + t^3*P3
//   where t in [0,1] and P0...P3 are the four control points.
//
// Build and run from the repository root:
//   c++ -std=c++20 -Wall -Wextra -Wpedantic checkpoint1/bezier/main.cpp
//       -o /tmp/apex3d-bezier
//   /tmp/apex3d-bezier

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>
#include <vector>

// ============================================================================
// Colour definitions
// ============================================================================

struct Color {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
};

// Palette used throughout the image (see README colour legend).
namespace Palette {
    constexpr Color kBackground   { 10,  12,  22};  // near-black dark blue
    constexpr Color kGridLine     { 28,  32,  48};  // subtle grid
    constexpr Color kCurve        { 80, 220, 255};  // cyan   - Bezier curve
    constexpr Color kPolygon      {255, 140,  30};  // orange - control polygon
    constexpr Color kCtrlPoint    {255,  60,  90};  // red    - control points
    constexpr Color kRacingLine   { 50, 230, 120};  // green  - Monza racing line
    constexpr Color kIsolated     {200, 160, 255};  // violet - isolated test curve
    constexpr Color kIsolPoly     {255, 200,  80};  // yellow - isolated polygon
    constexpr Color kIsolCtrl     {255,  80, 140};  // pink   - isolated ctrl pts
    constexpr Color kSeparator    { 55,  60,  80};  // panel divider
    constexpr Color kLabel        {190, 195, 210};  // text / legend
    constexpr Color kLabelDim     {100, 105, 120};  // dim text
    constexpr Color kWhite        {255, 255, 255};
} // namespace Palette

// ============================================================================
// 2-D point (floating-point world coordinates)
// ============================================================================

struct Point2D {
    double x{};
    double y{};
};

// ============================================================================
// RGB Canvas
// ============================================================================

class Canvas {
public:
    Canvas(int width, int height, Color background)
        : width_(width), height_(height),
          pixels_(static_cast<std::size_t>(width) *
                  static_cast<std::size_t>(height),
                  background)
    {}

    /// Writes one pixel. Coordinates outside [0,width) x [0,height) are
    /// silently ignored so no rasteriser can corrupt memory.
    void setPixel(int x, int y, Color color)
    {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) { return; }
        pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                static_cast<std::size_t>(x)] = color;
    }

    /// Fills a solid rectangle (clipped to canvas bounds).
    void fillRect(int x0, int y0, int x1, int y1, Color color)
    {
        for (int y = std::max(0, y0); y <= std::min(height_ - 1, y1); ++y)
            for (int x = std::max(0, x0); x <= std::min(width_ - 1, x1); ++x)
                setPixel(x, y, color);
    }

    /// Writes a binary (P6) PPM file. Returns true on success.
    bool writePpm(const std::filesystem::path& path) const
    {
        std::ofstream file(path, std::ios::binary);
        if (!file) { return false; }
        file << "P6\n" << width_ << ' ' << height_ << "\n255\n";
        for (const Color& c : pixels_) {
            const std::array<char, 3> rgb{
                static_cast<char>(c.r),
                static_cast<char>(c.g),
                static_cast<char>(c.b)};
            file.write(rgb.data(), static_cast<std::streamsize>(rgb.size()));
        }
        return static_cast<bool>(file);
    }

    [[nodiscard]] int width()  const { return width_;  }
    [[nodiscard]] int height() const { return height_; }

private:
    int width_;
    int height_;
    std::vector<Color> pixels_;
};

// ============================================================================
// Line rasteriser (Bresenham, all octants)
//
// Joins pairs of sampled Bezier points so the curve appears continuous even
// when neighbouring samples are more than one pixel apart. The algorithm
// keeps an integer error accumulator to decide whether to step diagonally,
// horizontally, or vertically - no floating-point arithmetic, no division.
// ============================================================================

static void drawLine(Canvas& canvas, int x0, int y0, int x1, int y1, Color color)
{
    const int dx  =  std::abs(x1 - x0);
    const int dy  = -std::abs(y1 - y0);
    const int sx  = (x0 < x1) ? 1 : -1;
    const int sy  = (y0 < y1) ? 1 : -1;
    int err = dx + dy;

    for (;;) {
        canvas.setPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) { break; }
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

/// Draws a filled circle of radius r (used for control-point markers).
static void drawDot(Canvas& canvas, int cx, int cy, int r, Color color)
{
    for (int dy = -r; dy <= r; ++dy)
        for (int dx = -r; dx <= r; ++dx)
            if (dx * dx + dy * dy <= r * r)
                canvas.setPixel(cx + dx, cy + dy, color);
}

// ============================================================================
// Minimal 5x7 bitmap font (ASCII 32 to 90)
//
// Each character is 5 columns x 7 rows packed into 5 bytes (one per column,
// LSB = top row).
// ============================================================================

static constexpr int kGlyphW = 5;
static constexpr int kGlyphH = 7;

struct Glyph { std::array<std::uint8_t, 5> cols{}; };

// Column-major 5x7 glyphs for ASCII 0x20 (' ') through 0x5A ('Z').
static constexpr std::array<Glyph, 59> kFont = {{
    /* 0x20 ' '  */ {{{0x00,0x00,0x00,0x00,0x00}}},
    /* 0x21 '!'  */ {{{0x00,0x00,0x5F,0x00,0x00}}},
    /* 0x22 '"'  */ {{{0x00,0x07,0x00,0x07,0x00}}},
    /* 0x23 '#'  */ {{{0x14,0x7F,0x14,0x7F,0x14}}},
    /* 0x24 '$'  */ {{{0x24,0x2A,0x7F,0x2A,0x12}}},
    /* 0x25 '%'  */ {{{0x23,0x13,0x08,0x64,0x62}}},
    /* 0x26 '&'  */ {{{0x36,0x49,0x55,0x22,0x50}}},
    /* 0x27 '\'' */ {{{0x00,0x05,0x03,0x00,0x00}}},
    /* 0x28 '('  */ {{{0x00,0x1C,0x22,0x41,0x00}}},
    /* 0x29 ')'  */ {{{0x00,0x41,0x22,0x1C,0x00}}},
    /* 0x2A '*'  */ {{{0x14,0x08,0x3E,0x08,0x14}}},
    /* 0x2B '+'  */ {{{0x08,0x08,0x3E,0x08,0x08}}},
    /* 0x2C ','  */ {{{0x00,0x50,0x30,0x00,0x00}}},
    /* 0x2D '-'  */ {{{0x08,0x08,0x08,0x08,0x08}}},
    /* 0x2E '.'  */ {{{0x00,0x60,0x60,0x00,0x00}}},
    /* 0x2F '/'  */ {{{0x20,0x10,0x08,0x04,0x02}}},
    /* 0x30 '0'  */ {{{0x3E,0x51,0x49,0x45,0x3E}}},
    /* 0x31 '1'  */ {{{0x00,0x42,0x7F,0x40,0x00}}},
    /* 0x32 '2'  */ {{{0x42,0x61,0x51,0x49,0x46}}},
    /* 0x33 '3'  */ {{{0x21,0x41,0x45,0x4B,0x31}}},
    /* 0x34 '4'  */ {{{0x18,0x14,0x12,0x7F,0x10}}},
    /* 0x35 '5'  */ {{{0x27,0x45,0x45,0x45,0x39}}},
    /* 0x36 '6'  */ {{{0x3C,0x4A,0x49,0x49,0x30}}},
    /* 0x37 '7'  */ {{{0x01,0x71,0x09,0x05,0x03}}},
    /* 0x38 '8'  */ {{{0x36,0x49,0x49,0x49,0x36}}},
    /* 0x39 '9'  */ {{{0x06,0x49,0x49,0x2A,0x1E}}},
    /* 0x3A ':'  */ {{{0x00,0x36,0x36,0x00,0x00}}},
    /* 0x3B ';'  */ {{{0x00,0x56,0x36,0x00,0x00}}},
    /* 0x3C '<'  */ {{{0x08,0x14,0x22,0x41,0x00}}},
    /* 0x3D '='  */ {{{0x14,0x14,0x14,0x14,0x14}}},
    /* 0x3E '>'  */ {{{0x00,0x41,0x22,0x14,0x08}}},
    /* 0x3F '?'  */ {{{0x02,0x01,0x51,0x09,0x06}}},
    /* 0x40 '@'  */ {{{0x32,0x49,0x79,0x41,0x3E}}},
    /* 0x41 'A'  */ {{{0x7E,0x11,0x11,0x11,0x7E}}},
    /* 0x42 'B'  */ {{{0x7F,0x49,0x49,0x49,0x36}}},
    /* 0x43 'C'  */ {{{0x3E,0x41,0x41,0x41,0x22}}},
    /* 0x44 'D'  */ {{{0x7F,0x41,0x41,0x22,0x1C}}},
    /* 0x45 'E'  */ {{{0x7F,0x49,0x49,0x49,0x41}}},
    /* 0x46 'F'  */ {{{0x7F,0x09,0x09,0x09,0x01}}},
    /* 0x47 'G'  */ {{{0x3E,0x41,0x49,0x49,0x7A}}},
    /* 0x48 'H'  */ {{{0x7F,0x08,0x08,0x08,0x7F}}},
    /* 0x49 'I'  */ {{{0x00,0x41,0x7F,0x41,0x00}}},
    /* 0x4A 'J'  */ {{{0x20,0x40,0x41,0x3F,0x01}}},
    /* 0x4B 'K'  */ {{{0x7F,0x08,0x14,0x22,0x41}}},
    /* 0x4C 'L'  */ {{{0x7F,0x40,0x40,0x40,0x40}}},
    /* 0x4D 'M'  */ {{{0x7F,0x02,0x04,0x02,0x7F}}},
    /* 0x4E 'N'  */ {{{0x7F,0x04,0x08,0x10,0x7F}}},
    /* 0x4F 'O'  */ {{{0x3E,0x41,0x41,0x41,0x3E}}},
    /* 0x50 'P'  */ {{{0x7F,0x09,0x09,0x09,0x06}}},
    /* 0x51 'Q'  */ {{{0x3E,0x41,0x51,0x21,0x5E}}},
    /* 0x52 'R'  */ {{{0x7F,0x09,0x19,0x29,0x46}}},
    /* 0x53 'S'  */ {{{0x46,0x49,0x49,0x49,0x31}}},
    /* 0x54 'T'  */ {{{0x01,0x01,0x7F,0x01,0x01}}},
    /* 0x55 'U'  */ {{{0x3F,0x40,0x40,0x40,0x3F}}},
    /* 0x56 'V'  */ {{{0x1F,0x20,0x40,0x20,0x1F}}},
    /* 0x57 'W'  */ {{{0x3F,0x40,0x38,0x40,0x3F}}},
    /* 0x58 'X'  */ {{{0x63,0x14,0x08,0x14,0x63}}},
    /* 0x59 'Y'  */ {{{0x07,0x08,0x70,0x08,0x07}}},
    /* 0x5A 'Z'  */ {{{0x61,0x51,0x49,0x45,0x43}}},
}};

/// Draws a single ASCII character at pixel (x,y) with given scale and colour.
static void drawChar(Canvas& canvas, int x, int y, char ch, int scale, Color color)
{
    const auto idx = static_cast<unsigned char>(ch);
    if (idx < 0x20 || idx > 0x5A) { return; }
    const Glyph& g = kFont[idx - 0x20];
    for (int col = 0; col < kGlyphW; ++col) {
        for (int row = 0; row < kGlyphH; ++row) {
            if ((g.cols[static_cast<std::size_t>(col)] & (1u << row)) != 0u) {
                canvas.fillRect(
                    x + col * scale,
                    y + row * scale,
                    x + col * scale + scale - 1,
                    y + row * scale + scale - 1,
                    color);
            }
        }
    }
}

/// Draws a string (upper-case ASCII) starting at (x,y).
static void drawText(Canvas& canvas, int x, int y, std::string_view text,
                     int scale, Color color)
{
    int cx = x;
    for (char ch : text) {
        drawChar(canvas, cx, y, ch, scale, color);
        cx += (kGlyphW + 1) * scale;
    }
}

/// Returns the pixel width of a string at the given scale.
[[nodiscard]] static int textWidth(std::string_view text, int scale)
{
    return static_cast<int>(text.size()) * (kGlyphW + 1) * scale - scale;
}

// ============================================================================
// Cubic Bezier evaluation
//
// B(t) = (1-t)^3*P0 + 3(1-t)^2*t*P1 + 3(1-t)*t^2*P2 + t^3*P3
//
// The four Bernstein basis polynomials of degree 3:
//   b0(t) = (1-t)^3        b1(t) = 3(1-t)^2*t
//   b2(t) = 3(1-t)*t^2     b3(t) = t^3
//
// They sum to 1 for all t, so B(t) is always a convex combination of P0..P3.
// Consequence: B(0)=P0 exactly and B(1)=P3 exactly; the curve lies inside
// the convex hull of the four control points.
// ============================================================================

/// Evaluates one scalar component of the cubic Bezier at parameter t in [0,1].
[[nodiscard]] static double bezierEval(double p0, double p1,
                                       double p2, double p3,
                                       double t) noexcept
{
    const double u  = 1.0 - t;
    const double u2 = u  * u;
    const double u3 = u2 * u;
    const double t2 = t  * t;
    const double t3 = t2 * t;
    return u3 * p0 + 3.0 * u2 * t * p1 + 3.0 * u * t2 * p2 + t3 * p3;
}

/// Evaluates the 2-D cubic Bezier point at parameter t.
[[nodiscard]] static Point2D cubicBezier(Point2D p0, Point2D p1,
                                          Point2D p2, Point2D p3,
                                          double t) noexcept
{
    return { bezierEval(p0.x, p1.x, p2.x, p3.x, t),
             bezierEval(p0.y, p1.y, p2.y, p3.y, t) };
}

// ============================================================================
// High-level Bezier drawing helpers
// ============================================================================

/// Number of t-samples used to rasterise one cubic Bezier segment.
/// 500 steps gives sub-pixel resolution for curves up to ~500 px long.
static constexpr int kBezierSteps = 500;

/// Statistics collected during rendering.
struct Stats {
    int curvesDrawn   = 0;
    int totalPixels   = 0;
    int samplesPerSeg = kBezierSteps;
};

/// Renders one cubic Bezier curve by sampling t uniformly and joining
/// consecutive sample points with Bresenham line segments.
static int drawBezier(Canvas& canvas,
                      Point2D p0, Point2D p1, Point2D p2, Point2D p3,
                      Color color)
{
    int pixels = 0;
    Point2D prev = cubicBezier(p0, p1, p2, p3, 0.0);

    for (int step = 1; step <= kBezierSteps; ++step) {
        const double t     = static_cast<double>(step) /
                             static_cast<double>(kBezierSteps);
        const Point2D curr = cubicBezier(p0, p1, p2, p3, t);

        const int x0 = static_cast<int>(std::round(prev.x));
        const int y0 = static_cast<int>(std::round(prev.y));
        const int x1 = static_cast<int>(std::round(curr.x));
        const int y1 = static_cast<int>(std::round(curr.y));

        if (x0 != x1 || y0 != y1) {
            drawLine(canvas, x0, y0, x1, y1, color);
            pixels += std::max(std::abs(x1 - x0), std::abs(y1 - y0)) + 1;
        }
        prev = curr;
    }
    return pixels;
}

/// Draws the control polygon (P0-P1, P1-P2, P2-P3).
static void drawControlPolygon(Canvas& canvas,
                               Point2D p0, Point2D p1, Point2D p2, Point2D p3,
                               Color color)
{
    auto ipx = [](Point2D p){ return static_cast<int>(std::round(p.x)); };
    auto ipy = [](Point2D p){ return static_cast<int>(std::round(p.y)); };
    drawLine(canvas, ipx(p0), ipy(p0), ipx(p1), ipy(p1), color);
    drawLine(canvas, ipx(p1), ipy(p1), ipx(p2), ipy(p2), color);
    drawLine(canvas, ipx(p2), ipy(p2), ipx(p3), ipy(p3), color);
}

/// Draws a filled circle marker at each control point.
static void drawControlPoints(Canvas& canvas,
                              Point2D p0, Point2D p1, Point2D p2, Point2D p3,
                              Color color, int radius = 5)
{
    for (const Point2D& p : {p0, p1, p2, p3}) {
        drawDot(canvas,
                static_cast<int>(std::round(p.x)),
                static_cast<int>(std::round(p.y)),
                radius, color);
    }
}

/// Maps a normalised coordinate (-1..+1) into pixel space for a panel.
[[nodiscard]] static Point2D toPixel(double nx, double ny,
                                     int panelX, int panelY,
                                     int panelW, int panelH)
{
    return { panelX + (nx + 1.0) * 0.5 * panelW,
             panelY + (1.0 - (ny + 1.0) * 0.5) * panelH };
}

// ============================================================================
// Panel 1: Isolated cubic Bezier test curve
//
// Shows a single S-shaped curve together with its control polygon and four
// labelled control point markers. The asymmetric placement of P1 and P2 makes
// the influence of the tangent handles visually obvious.
// ============================================================================

static void drawIsolatedPanel(Canvas& canvas, Stats& stats,
                              int px, int py, int pw, int ph)
{
    // Grid lines for visual reference.
    for (int gx = px; gx <= px + pw; gx += pw / 8)
        for (int y = py; y < py + ph; ++y)
            canvas.setPixel(gx, y, Palette::kGridLine);
    for (int gy = py; gy <= py + ph; gy += ph / 6)
        for (int x = px; x < px + pw; ++x)
            canvas.setPixel(x, gy, Palette::kGridLine);

    // Control points in normalised space.
    const Point2D p0 = toPixel(-0.75, -0.55, px, py, pw, ph);
    const Point2D p1 = toPixel(-0.30,  0.70, px, py, pw, ph);
    const Point2D p2 = toPixel( 0.40, -0.65, px, py, pw, ph);
    const Point2D p3 = toPixel( 0.75,  0.50, px, py, pw, ph);

    // Control polygon behind the curve.
    drawControlPolygon(canvas, p0, p1, p2, p3, Palette::kIsolPoly);
    // Smooth curve.
    stats.totalPixels += drawBezier(canvas, p0, p1, p2, p3, Palette::kIsolated);
    ++stats.curvesDrawn;
    // Markers on top.
    drawControlPoints(canvas, p0, p1, p2, p3, Palette::kIsolCtrl);

    // Labels.
    auto lx = [](Point2D p, int off){ return static_cast<int>(std::round(p.x)) + off; };
    auto ly = [](Point2D p, int off){ return static_cast<int>(std::round(p.y)) + off; };
    drawText(canvas, lx(p0,-20), ly(p0, 8), "P0", 2, Palette::kLabel);
    drawText(canvas, lx(p1,  8), ly(p1,-4), "P1", 2, Palette::kLabel);
    drawText(canvas, lx(p2,  8), ly(p2, 8), "P2", 2, Palette::kLabel);
    drawText(canvas, lx(p3,  8), ly(p3,-4), "P3", 2, Palette::kLabel);

    const std::string_view title = "ISOLATED BEZIER TEST CURVE";
    drawText(canvas,
             px + (pw - textWidth(title, 2)) / 2,
             py + 10, title, 2, Palette::kLabel);
}

// ============================================================================
// Panel 2: Monza racing line - chained Bezier segments
//
// Seven cubic Bezier segments approximate a full Monza lap in counter-clockwise
// order: SF straight -> Variante del Rettifilo -> Curva Grande ->
// Variante della Roggia -> Lesmo 1+2 -> Ascari chicane -> Parabolica -> SF.
//
// C1 continuity: the last control point of segment i (P3_i), the first of
// segment i+1 (P0_{i+1}), and its handle (P1_{i+1}) are collinear, giving a
// smooth tangent at every join so the racing line has no kinks.
// ============================================================================

static void drawMonzaPanel(Canvas& canvas, Stats& stats,
                           int px, int py, int pw, int ph)
{
    for (int gx = px; gx <= px + pw; gx += pw / 10)
        for (int y = py; y < py + ph; ++y)
            canvas.setPixel(gx, y, Palette::kGridLine);
    for (int gy = py; gy <= py + ph; gy += ph / 8)
        for (int x = px; x < px + pw; ++x)
            canvas.setPixel(x, gy, Palette::kGridLine);

    auto tp = [&](double nx, double ny) -> Point2D {
        return toPixel(nx, ny, px, py, pw, ph);
    };

    struct Seg { Point2D p0, p1, p2, p3; };

    // Seven chained cubic Bezier segments. P1 and P2 are tangent handles.
    const std::array<Seg, 7> segs = {{
        // Seg 0: SF straight approach to Variante del Rettifilo
        { tp(-0.60,-0.75), tp(-0.60,-0.10), tp(-0.78, 0.30), tp(-0.78, 0.62) },
        // Seg 1: Variante del Rettifilo chicane (tight right-left)
        { tp(-0.78, 0.62), tp(-0.78, 0.82), tp(-0.30, 0.82), tp( 0.00, 0.65) },
        // Seg 2: Curva Grande (long left sweep)
        { tp( 0.00, 0.65), tp( 0.40, 0.45), tp( 0.80, 0.20), tp( 0.78,-0.05) },
        // Seg 3: Variante della Roggia chicane + Lesmo approach
        { tp( 0.78,-0.05), tp( 0.76,-0.35), tp( 0.50,-0.50), tp( 0.30,-0.45) },
        // Seg 4: Lesmo 1 and 2 (two right-handers)
        { tp( 0.30,-0.45), tp( 0.08,-0.40), tp(-0.10,-0.55), tp(-0.05,-0.72) },
        // Seg 5: Ascari chicane (quick left-right-left)
        { tp(-0.05,-0.72), tp( 0.00,-0.88), tp( 0.42,-0.88), tp( 0.50,-0.72) },
        // Seg 6: Parabolica (right-hander) back to SF
        { tp( 0.50,-0.72), tp( 0.58,-0.55), tp( 0.20,-0.75), tp(-0.60,-0.75) },
    }};

    // Draw polygons first (behind curves).
    for (const Seg& s : segs)
        drawControlPolygon(canvas, s.p0, s.p1, s.p2, s.p3, Palette::kPolygon);
    // Draw smooth curves.
    for (const Seg& s : segs) {
        stats.totalPixels +=
            drawBezier(canvas, s.p0, s.p1, s.p2, s.p3, Palette::kRacingLine);
        ++stats.curvesDrawn;
    }
    // Join-node markers.
    for (const Seg& s : segs)
        drawDot(canvas,
                static_cast<int>(std::round(s.p0.x)),
                static_cast<int>(std::round(s.p0.y)),
                4, Palette::kCtrlPoint);
    // Inner handle markers (smaller).
    for (const Seg& s : segs) {
        drawDot(canvas, static_cast<int>(std::round(s.p1.x)),
                static_cast<int>(std::round(s.p1.y)), 3, Palette::kPolygon);
        drawDot(canvas, static_cast<int>(std::round(s.p2.x)),
                static_cast<int>(std::round(s.p2.y)), 3, Palette::kPolygon);
    }

    drawText(canvas, px + 12, py + 12, "MONZA RACING LINE", 2, Palette::kLabel);
    drawText(canvas, px + 12, py + 30, "7 BEZIER SEGMENTS", 2, Palette::kLabelDim);

    // Landmark labels.
    const Point2D sfDot  = tp(-0.60,-0.75);
    const Point2D vrDot  = tp(-0.78, 0.62);
    const Point2D parDot = tp( 0.50,-0.72);
    drawText(canvas,
             static_cast<int>(std::round(sfDot.x))  - 24,
             static_cast<int>(std::round(sfDot.y))  + 8, "SF",  2, Palette::kWhite);
    drawText(canvas,
             static_cast<int>(std::round(vrDot.x))  - 38,
             static_cast<int>(std::round(vrDot.y))  -16, "VR",  2, Palette::kWhite);
    drawText(canvas,
             static_cast<int>(std::round(parDot.x)) +  8,
             static_cast<int>(std::round(parDot.y)) -16, "PAR", 2, Palette::kWhite);
}

// ============================================================================
// Panel 3: Comparison - raw control polygon vs smooth Bezier curve
// ============================================================================

static void drawComparisonPanel(Canvas& canvas, Stats& stats,
                                int px, int py, int pw, int ph)
{
    for (int gx = px; gx <= px + pw; gx += pw / 10)
        for (int y = py; y < py + ph; ++y)
            canvas.setPixel(gx, y, Palette::kGridLine);
    for (int gy = py; gy <= py + ph; gy += ph / 6)
        for (int x = px; x < px + pw; ++x)
            canvas.setPixel(x, gy, Palette::kGridLine);

    const int third = pw / 3;
    const int ox0 = px;
    const int ox1 = px + third;
    const int ox2 = px + 2 * third;

    auto tp = [&](double nx, double ny, int ox) -> Point2D {
        return toPixel(nx, ny, ox, py + 30, third, ph - 40);
    };

    struct CurveSpec { Point2D p0, p1, p2, p3; };
    const std::array<CurveSpec, 3> curves = {{
        { tp(-0.7,-0.6,ox0), tp(-0.6, 0.8,ox0),
          tp( 0.6,-0.8,ox0), tp( 0.7, 0.6,ox0) },
        { tp(-0.8, 0.4,ox1), tp( 0.0, 0.8,ox1),
          tp( 0.8, 0.0,ox1), tp( 0.6,-0.7,ox1) },
        { tp(-0.6, 0.5,ox2), tp( 0.6, 0.5,ox2),
          tp( 0.6,-0.5,ox2), tp(-0.6,-0.5,ox2) },
    }};
    const std::array<std::string_view, 3> subtitles = {
        "CHICANE", "SWEEPER", "HAIRPIN"
    };

    for (std::size_t i = 0; i < curves.size(); ++i) {
        const CurveSpec& c = curves[i];
        const int subPx = (i == 0) ? ox0 : (i == 1) ? ox1 : ox2;
        drawControlPolygon(canvas, c.p0, c.p1, c.p2, c.p3, Palette::kPolygon);
        stats.totalPixels +=
            drawBezier(canvas, c.p0, c.p1, c.p2, c.p3, Palette::kCurve);
        ++stats.curvesDrawn;
        drawControlPoints(canvas, c.p0, c.p1, c.p2, c.p3,
                          Palette::kCtrlPoint, 4);
        drawText(canvas,
                 subPx + (third - textWidth(subtitles[i], 2)) / 2,
                 py + ph - 20, subtitles[i], 2, Palette::kLabelDim);
    }
    for (int y = py; y < py + ph; ++y) {
        canvas.setPixel(ox1, y, Palette::kSeparator);
        canvas.setPixel(ox2, y, Palette::kSeparator);
    }
    const std::string_view heading = "POLYGON VS SMOOTH CURVE";
    drawText(canvas,
             px + (pw - textWidth(heading, 2)) / 2,
             py + 10, heading, 2, Palette::kLabel);
}

// ============================================================================
// Colour legend strip
// ============================================================================

static void drawLegend(Canvas& canvas, int x, int y)
{
    struct Entry { Color color; std::string_view label; };
    const std::array<Entry, 5> entries = {{
        { Palette::kIsolated,   "BEZIER CURVE (ISOLATED)"   },
        { Palette::kCurve,      "BEZIER CURVE (COMPARISON)" },
        { Palette::kRacingLine, "MONZA RACING LINE"         },
        { Palette::kPolygon,    "CONTROL POLYGON"           },
        { Palette::kCtrlPoint,  "CONTROL POINTS"            },
    }};
    drawText(canvas, x, y, "COLOUR LEGEND", 2, Palette::kLabel);
    int ey = y + 20;
    for (const Entry& e : entries) {
        canvas.fillRect(x, ey + 2, x + 14, ey + 10, e.color);
        drawText(canvas, x + 20, ey, e.label, 1, Palette::kLabel);
        ey += 16;
    }
}

// ============================================================================
// main
// ============================================================================

int main(int argc, char* argv[])
{
    constexpr int kWidth  = 1200;
    constexpr int kHeight =  700;
    Canvas canvas(kWidth, kHeight, Palette::kBackground);

    constexpr int kPanelW = kWidth  / 3;
    constexpr int kPanelH = kHeight - 80;

    // Separators.
    for (int x = 0; x < kWidth; ++x) {
        canvas.setPixel(x, kPanelH,     Palette::kSeparator);
        canvas.setPixel(x, kPanelH + 1, Palette::kSeparator);
    }
    for (int y = 0; y < kPanelH; ++y) {
        canvas.setPixel(kPanelW,         y, Palette::kSeparator);
        canvas.setPixel(kPanelW + 1,     y, Palette::kSeparator);
        canvas.setPixel(2 * kPanelW,     y, Palette::kSeparator);
        canvas.setPixel(2 * kPanelW + 1, y, Palette::kSeparator);
    }

    Stats stats;
    drawIsolatedPanel  (canvas, stats,  4,               4, kPanelW - 8, kPanelH - 8);
    drawMonzaPanel     (canvas, stats,  kPanelW     + 4, 4, kPanelW - 8, kPanelH - 8);
    drawComparisonPanel(canvas, stats, 2 * kPanelW + 4,  4, kPanelW - 8, kPanelH - 8);
    drawLegend(canvas, 16, kPanelH + 8);

    // Title and equation.
    const std::string_view mainTitle =
        "APEX3D - CUBIC BEZIER CURVES - MONZA F1";
    drawText(canvas,
             kWidth - textWidth(mainTitle, 2) - 10,
             kPanelH + 8, mainTitle, 2, Palette::kLabel);

    const std::string_view eqLine =
        "B(T)=(1-T)^3 P0+3(1-T)^2 T P1+3(1-T) T^2 P2+T^3 P3";
    drawText(canvas,
             kWidth - textWidth(eqLine, 1) - 10,
             kPanelH + 32, eqLine, 1, Palette::kLabelDim);

    // Output path.
    const std::filesystem::path outDir =
        (argc > 1)
            ? std::filesystem::path(argv[1])
            : std::filesystem::path(__FILE__).parent_path() / "output";

    std::filesystem::create_directories(outDir);
    const std::filesystem::path outFile = outDir / "bezier_demo.ppm";

    if (!canvas.writePpm(outFile)) {
        std::cerr << "ERROR: failed to write " << outFile << '\n';
        return EXIT_FAILURE;
    }

    std::cout
        << "Apex3D Checkpoint 1 - Cubic Bezier Curves\n"
        << "  Canvas             : "
        << canvas.width() << " x " << canvas.height() << " RGB\n"
        << "  Curves drawn       : " << stats.curvesDrawn   << '\n'
        << "  Samples per curve  : " << stats.samplesPerSeg
        << "  (t steps 0 to 1)\n"
        << "  Total Bezier pixels: " << stats.totalPixels    << '\n'
        << "  Output             : " << outFile.string()     << '\n';

    return EXIT_SUCCESS;
}

