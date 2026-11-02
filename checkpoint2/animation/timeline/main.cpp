// Apex3D - Checkpoint 2: Animation Timeline Demo
//
// Demonstrates the animation timeline by moving a Formula 1 car from the
// start grid (left side) to the finish line (right side) over 13 frames.
// Each frame is saved as a numbered PPM file: frame_000.ppm to frame_012.ppm.
//
// The shared apex3d::Mat3 transformation engine is used to position and
// orient the car on every frame.
//
// Build (from the timeline directory):
//   g++ -std=c++20 -Wall -Wextra -Wpedantic
//       -I../../transformations/include
//       main.cpp -o timeline_demo
//
// Run:
//   ./timeline_demo            (writes to ./output/)
//   ./timeline_demo /some/dir  (writes to specified directory)

#include "timeline.hpp"
#include "../../transformations/include/transform2d.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

// ─────────────────────────────────────────────────────────────────────────────
// Colour palette
// ─────────────────────────────────────────────────────────────────────────────
struct Color {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
};

namespace Palette {
constexpr Color background {9,  12,  20};
constexpr Color track      {62, 64,  73};
constexpr Color trackLine  {86, 88,  99};
constexpr Color finishLine {255, 215, 0};
constexpr Color body       {220, 28,  40};
constexpr Color tyre       {10,  10,  13};
constexpr Color wheelStripe{250, 210, 35};
constexpr Color cockpit    {18,  20,  25};
constexpr Color helmet     {255, 215, 35};
constexpr Color wing       {238, 240, 245};
constexpr Color outline    {250, 250, 252};
constexpr Color arrowCol   {100, 220, 255};
constexpr Color trailCol   {80,  100, 140};
constexpr Color label      {200, 204, 220};
} // namespace Palette

// ─────────────────────────────────────────────────────────────────────────────
// Canvas
// ─────────────────────────────────────────────────────────────────────────────
class Canvas {
public:
    Canvas(int w, int h, Color bg)
        : width_(w), height_(h),
          pixels_(static_cast<std::size_t>(w) * static_cast<std::size_t>(h), bg)
    {}

    void setPixel(int x, int y, Color c)
    {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) return;
        pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                static_cast<std::size_t>(x)] = c;
    }

    void fillRect(int x0, int y0, int x1, int y1, Color c)
    {
        for (int y = std::max(0, y0); y <= std::min(height_-1, y1); ++y)
            for (int x = std::max(0, x0); x <= std::min(width_-1, x1); ++x)
                setPixel(x, y, c);
    }

    [[nodiscard]] bool writePpm(const std::filesystem::path& path) const
    {
        std::ofstream out(path, std::ios::binary);
        if (!out) return false;
        out << "P6\n" << width_ << ' ' << height_ << "\n255\n";
        for (const Color& px : pixels_) {
            const std::array<char,3> bytes{
                static_cast<char>(px.r),
                static_cast<char>(px.g),
                static_cast<char>(px.b)};
            out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
        return static_cast<bool>(out);
    }

    [[nodiscard]] int width()  const noexcept { return width_; }
    [[nodiscard]] int height() const noexcept { return height_; }

private:
    int width_{};
    int height_{};
    std::vector<Color> pixels_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Drawing primitives
// ─────────────────────────────────────────────────────────────────────────────
void drawLine(Canvas& canvas, int x0, int y0, int x1, int y1, Color c)
{
    const int dx =  std::abs(x1 - x0);
    const int dy = -std::abs(y1 - y0);
    const int sx = x0 < x1 ? 1 : -1;
    const int sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        canvas.setPixel(x0, y0, c);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void drawDisk(Canvas& canvas, int cx, int cy, int radius, Color c)
{
    for (int dy = -radius; dy <= radius; ++dy)
        for (int dx = -radius; dx <= radius; ++dx)
            if (dx*dx + dy*dy <= radius*radius)
                canvas.setPixel(cx+dx, cy+dy, c);
}

void fillPolygon(Canvas& canvas, const std::vector<apex3d::Vec2>& poly, Color c)
{
    if (poly.size() < 3) return;
    double yMin = poly[0].y, yMax = poly[0].y;
    for (const auto& p : poly) { yMin = std::min(yMin, p.y); yMax = std::max(yMax, p.y); }
    for (int y = static_cast<int>(std::floor(yMin));
             y <= static_cast<int>(std::ceil(yMax)); ++y) {
        const double scanY = y + 0.5;
        std::vector<double> xs;
        for (std::size_t i = 0; i < poly.size(); ++i) {
            apex3d::Vec2 a = poly[i], b = poly[(i+1) % poly.size()];
            double lo = std::min(a.y, b.y), hi = std::max(a.y, b.y);
            if (scanY < lo || scanY >= hi || std::abs(a.y - b.y) < 1e-12) continue;
            double t2 = (scanY - a.y) / (b.y - a.y);
            xs.push_back(a.x + t2 * (b.x - a.x));
        }
        std::sort(xs.begin(), xs.end());
        for (std::size_t i = 0; i + 1 < xs.size(); i += 2) {
            int x0 = static_cast<int>(std::ceil(xs[i] - 0.5));
            int x1 = static_cast<int>(std::floor(xs[i+1] - 0.5));
            for (int x = x0; x <= x1; ++x) canvas.setPixel(x, y, c);
        }
    }
}

void outlinePolygon(Canvas& canvas, const std::vector<apex3d::Vec2>& poly, Color c)
{
    for (std::size_t i = 0; i < poly.size(); ++i) {
        apex3d::Vec2 a = poly[i], b = poly[(i+1) % poly.size()];
        drawLine(canvas,
            static_cast<int>(std::lround(a.x)), static_cast<int>(std::lround(a.y)),
            static_cast<int>(std::lround(b.x)), static_cast<int>(std::lround(b.y)), c);
    }
}

void drawShape(Canvas& canvas, const std::vector<apex3d::Vec2>& model,
               const apex3d::Mat3& mat, Color fill, Color outln)
{
    auto transformed = apex3d::transformPolygon(model, mat);
    fillPolygon(canvas, transformed, fill);
    outlinePolygon(canvas, transformed, outln);
}

[[nodiscard]] std::vector<apex3d::Vec2> rect(double x0, double y0, double x1, double y1)
{
    return {{x0,y0},{x1,y0},{x1,y1},{x0,y1}};
}

// ─────────────────────────────────────────────────────────────────────────────
// F1 car model
// ─────────────────────────────────────────────────────────────────────────────
void drawCar(Canvas& canvas, const apex3d::Mat3& mat, Color bodyColor)
{
    const std::vector<apex3d::Vec2> chassis{
        {-8,-72},{8,-72},{12,-35},{25,-18},{27,22},
        {18,52},{12,78},{-12,78},{-18,52},{-27,22},
        {-25,-18},{-12,-35}};
    const auto frontWing  = rect(-34,-78, 34,-69);
    const auto rearWing   = rect(-31, 72, 31, 82);
    const auto frontLeft  = rect(-38,-48,-24,-13);
    const auto frontRight = rect( 24,-48, 38,-13);
    const auto rearLeft   = rect(-40, 31,-24, 69);
    const auto rearRight  = rect( 24, 31, 40, 69);
    const auto cockpit    = rect(-10, -8, 10, 29);
    const auto helmetRect = rect( -6, -1,  6, 13);

    for (const auto& tyre : {frontLeft, frontRight, rearLeft, rearRight})
        drawShape(canvas, tyre, mat, Palette::tyre, Palette::outline);
    drawShape(canvas, frontWing,  mat, Palette::wing,    Palette::outline);
    drawShape(canvas, rearWing,   mat, Palette::wing,    Palette::outline);
    drawShape(canvas, chassis,    mat, bodyColor,         Palette::outline);
    drawShape(canvas, cockpit,    mat, Palette::cockpit,  Palette::outline);
    drawShape(canvas, helmetRect, mat, Palette::helmet,   Palette::helmet);
    for (const double wy : {-30.0, 51.0}) {
        drawShape(canvas, rect(-38,wy-1.5,-24,wy+1.5), mat,
                  Palette::wheelStripe, Palette::wheelStripe);
        drawShape(canvas, rect( 24,wy-1.5, 38,wy+1.5), mat,
                  Palette::wheelStripe, Palette::wheelStripe);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Heading arrow
// ─────────────────────────────────────────────────────────────────────────────
void drawHeadingArrow(Canvas& canvas, int cx, int cy,
                      double angleDeg, int length, Color c)
{
    namespace tl = apex3d::timeline;
    double rad = tl::toRadians(angleDeg - 90.0);
    int ex = cx + static_cast<int>(std::lround(length * std::cos(rad)));
    int ey = cy + static_cast<int>(std::lround(length * std::sin(rad)));
    drawLine(canvas, cx, cy, ex, ey, c);
    double lRad = rad + tl::toRadians(150.0);
    double rRad = rad - tl::toRadians(150.0);
    constexpr int kArr = 10;
    drawLine(canvas, ex, ey,
        ex + static_cast<int>(std::lround(kArr * std::cos(lRad))),
        ey + static_cast<int>(std::lround(kArr * std::sin(lRad))), c);
    drawLine(canvas, ex, ey,
        ex + static_cast<int>(std::lround(kArr * std::cos(rRad))),
        ey + static_cast<int>(std::lround(kArr * std::sin(rRad))), c);
}

// ─────────────────────────────────────────────────────────────────────────────
// Pixel font (5x7) for labels
// ─────────────────────────────────────────────────────────────────────────────
void drawChar(Canvas& canvas, int px, int py, char ch, Color c)
{
    static const std::array<std::array<std::uint8_t,7>,11> glyphs = {{
        {{0x0E,0x11,0x13,0x15,0x19,0x11,0x0E}}, // 0
        {{0x04,0x0C,0x04,0x04,0x04,0x04,0x0E}}, // 1
        {{0x0E,0x11,0x01,0x06,0x08,0x10,0x1F}}, // 2
        {{0x0E,0x11,0x01,0x06,0x01,0x11,0x0E}}, // 3
        {{0x02,0x06,0x0A,0x12,0x1F,0x02,0x02}}, // 4
        {{0x1F,0x10,0x1E,0x01,0x01,0x11,0x0E}}, // 5
        {{0x06,0x08,0x10,0x1E,0x11,0x11,0x0E}}, // 6
        {{0x1F,0x01,0x02,0x04,0x08,0x08,0x08}}, // 7
        {{0x0E,0x11,0x11,0x0E,0x11,0x11,0x0E}}, // 8
        {{0x0E,0x11,0x11,0x0F,0x01,0x02,0x0C}}, // 9
        {{0x00,0x00,0x00,0x00,0x00,0x00,0x00}}, // space
    }};
    int idx = 10;
    if (ch >= '0' && ch <= '9') idx = ch - '0';
    for (int row = 0; row < 7; ++row)
        for (int col = 0; col < 5; ++col)
            if (glyphs[static_cast<std::size_t>(idx)][static_cast<std::size_t>(row)]
                & (1 << (4-col)))
                canvas.setPixel(px + col, py + row, c);
}

void drawString(Canvas& canvas, int px, int py, const std::string& s, Color c)
{
    int x = px;
    for (char ch : s) { drawChar(canvas, x, py, ch, c); x += 6; }
}

// ─────────────────────────────────────────────────────────────────────────────
// Track background
// ─────────────────────────────────────────────────────────────────────────────
void drawTrackBackground(Canvas& canvas)
{
    const int w = canvas.width();
    const int h = canvas.height();
    canvas.fillRect(0, 0, w-1, h/4-1, {15, 20, 45});
    canvas.fillRect(0, h/4, w-1, h-1, Palette::track);
    for (int x = 0; x < w; x += 40)
        canvas.fillRect(x, h/2-2, x+20, h/2+2, Palette::trackLine);
    for (int row = 0; row < 6; ++row)
        for (int col = 0; col < 6; ++col) {
            Color sq = ((row+col)%2==0) ? Color{255,255,255} : Color{0,0,0};
            canvas.fillRect(40+col*8, h/4+row*8, 47+col*8, h/4+7+row*8, sq);
        }
    for (int row = 0; row < 6; ++row)
        for (int col = 0; col < 6; ++col) {
            Color sq = ((row+col)%2==0) ? Palette::finishLine : Color{0,0,0};
            canvas.fillRect(w-88+col*8, h/4+row*8, w-81+col*8, h/4+7+row*8, sq);
        }
}

void drawProgressBar(Canvas& canvas, double progress, int frameIdx, int totalFrames)
{
    const int w = canvas.width();
    constexpr int barY = 24, barH = 8;
    const int barX0 = 2, barX1 = w - 2;
    const int barW = barX1 - barX0;
    canvas.fillRect(barX0, barY, barX1, barY+barH, {30,35,55});
    int filled = static_cast<int>(progress * static_cast<double>(barW));
    canvas.fillRect(barX0, barY, barX0+filled, barY+barH, {220,28,40});
    drawString(canvas, barX0+4, barY+1,
               std::to_string(frameIdx)+"/"+std::to_string(totalFrames-1),
               Palette::label);
}

void drawTrail(Canvas& canvas, const std::vector<apex3d::timeline::Vec2>& trail)
{
    for (std::size_t i = 0; i + 1 < trail.size(); ++i)
        drawDisk(canvas,
            static_cast<int>(std::lround(trail[i].x)),
            static_cast<int>(std::lround(trail[i].y)),
            3, Palette::trailCol);
}

[[nodiscard]] std::string frameName(int index)
{
    std::ostringstream oss;
    oss << "frame_" << std::setfill('0') << std::setw(3) << index << ".ppm";
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────────
// main
// ─────────────────────────────────────────────────────────────────────────────
int main(int argc, char* argv[])
{
    namespace tl = apex3d::timeline;
    using apex3d::Mat3;

    constexpr double kFps      = 6.0;
    constexpr double kDuration = 2.0;

    const tl::Timeline timeline(kFps, kDuration, tl::PlaybackMode::Clamp);
    const int totalFrames = timeline.totalFrames();

    const tl::Vec2 startPos{120.0, 300.0};
    const tl::Vec2 endPos  {760.0, 300.0};
    constexpr double kStartAngle = 90.0;
    constexpr double kEndAngle   = 90.0;

    constexpr int kWidth  = 900;
    constexpr int kHeight = 540;

    const std::filesystem::path outputDir = argc > 1
        ? std::filesystem::path(argv[1])
        : std::filesystem::path(__FILE__).parent_path() / "output";
    std::filesystem::create_directories(outputDir);

    std::vector<tl::Vec2> trail;
    trail.reserve(static_cast<std::size_t>(totalFrames));

    std::cout << "Apex3D - Animation Timeline Demo\n"
              << "  FPS         : " << kFps        << "\n"
              << "  Duration    : " << kDuration    << " s\n"
              << "  Total frames: " << totalFrames  << "\n"
              << "  Output dir  : " << outputDir.string() << "\n\n";

    for (int frame = 0; frame < totalFrames; ++frame) {
        const double t       = timeline.progressAtFrame(frame);
        const double elapsed = timeline.elapsedAtFrame(frame);
        const tl::Vec2 pos   = tl::lerp(startPos, endPos, t);
        const double angle   = tl::lerpAngle(kStartAngle, kEndAngle, t);

        trail.push_back(pos);

        const Mat3 mat =
            Mat3::translation(pos.x, pos.y) *
            Mat3::rotationDegrees(angle - 90.0);

        Canvas canvas(kWidth, kHeight, Palette::background);
        drawTrackBackground(canvas);
        drawTrail(canvas, trail);
        drawCar(canvas, mat, Palette::body);
        drawHeadingArrow(canvas,
            static_cast<int>(std::lround(pos.x)),
            static_cast<int>(std::lround(pos.y)),
            angle, 55, Palette::arrowCol);
        drawProgressBar(canvas, t, frame, totalFrames);

        const std::filesystem::path outPath = outputDir / frameName(frame);
        if (!canvas.writePpm(outPath)) {
            std::cerr << "ERROR: failed to write " << outPath << '\n';
            return EXIT_FAILURE;
        }

        std::cout << "  frame " << std::setw(3) << frame
                  << "  t=" << std::fixed << std::setprecision(4) << t
                  << "  elapsed=" << std::setprecision(3) << elapsed << "s"
                  << "  pos=(" << std::setprecision(1)
                  << pos.x << "," << pos.y << ")"
                  << "  -> " << outPath.filename().string() << "\n";
    }

    std::cout << "\nAll " << totalFrames << " frames written to "
              << outputDir.string() << "\n";
    return EXIT_SUCCESS;
}
