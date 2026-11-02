// Apex3D - Checkpoint 2: polygon Formula 1 car animation.
//
// Builds on two merged milestones:
//   * checkpoint1/polygons      - polygon representation and scanline filling
//   * checkpoint2/transformations - the shared apex3d::Mat3 engine
//
// Every pose of the car is described by ONE 3 x 3 homogeneous matrix. All
// thirteen polygon components (chassis, wings, tyres, wheel stripes, cockpit,
// helmet) are pushed through that same matrix by placeCar(), so the car always
// moves as one rigid (or uniformly deformed) object.
//
// No OpenGL or third-party library is used. Frames are written as binary PPM
// and the contact sheet as both PPM and PNG.
#pragma once

#include "../../../transformations/include/transform2d.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <vector>

namespace apex3d::car {

// ---------------------------------------------------------------------------
// Canvas and colours
// ---------------------------------------------------------------------------

struct Color {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};

    friend constexpr bool operator==(Color, Color) = default;
};

namespace palette {
constexpr Color background{9, 12, 20};
constexpr Color track{62, 64, 73};
constexpr Color laneMark{150, 152, 160};
constexpr Color trail{120, 125, 140};
constexpr Color ghost{105, 110, 128};
constexpr Color body{220, 28, 40};
constexpr Color tyre{10, 10, 13};
constexpr Color wheelStripe{250, 210, 35};
constexpr Color cockpit{18, 20, 25};
constexpr Color helmet{255, 215, 35};
constexpr Color wing{238, 240, 245};
constexpr Color outline{250, 250, 252};
constexpr Color pivot{255, 75, 95};
// Phase colours match the panel headers of checkpoint2/transformations.
constexpr Color translation{55, 220, 255};
constexpr Color rotation{255, 175, 45};
constexpr Color scaling{65, 225, 125};
constexpr Color combined{210, 95, 255};
} // namespace palette

class Canvas {
public:
    Canvas(int width, int height, Color fill)
        : width_(width), height_(height),
          pixels_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), fill)
    {
    }

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }

    void setPixel(int x, int y, Color color)
    {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) {
            return; // clipping to the canvas is done per pixel
        }
        pixels_[index(x, y)] = color;
    }

    [[nodiscard]] Color pixel(int x, int y) const { return pixels_[index(x, y)]; }

    void fillRect(int x0, int y0, int x1, int y1, Color color)
    {
        for (int y = std::max(0, y0); y <= std::min(height_ - 1, y1); ++y) {
            for (int x = std::max(0, x0); x <= std::min(width_ - 1, x1); ++x) {
                pixels_[index(x, y)] = color;
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
            const std::array<char, 3> bytes{static_cast<char>(color.r),
                                            static_cast<char>(color.g),
                                            static_cast<char>(color.b)};
            output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
        return static_cast<bool>(output);
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

// ---------------------------------------------------------------------------
// Rasterisation (same algorithms as checkpoint1/polygons)
// ---------------------------------------------------------------------------

// Integer Bresenham line for all octants.
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

inline void outlinePolygon(Canvas& canvas, const std::vector<Vec2>& polygon, Color color)
{
    for (std::size_t i = 0; i < polygon.size(); ++i) {
        const Vec2 a = polygon[i];
        const Vec2 b = polygon[(i + 1) % polygon.size()];
        drawLine(canvas, toPixel(a.x), toPixel(a.y), toPixel(b.x), toPixel(b.y), color);
    }
}

// Even-odd scanline fill. Each pixel row is sampled at its centre (y + 0.5);
// an edge counts only when the scanline lies in [yLow, yHigh), so shared
// vertices are counted once and horizontal edges are skipped.
inline void scanlineFill(Canvas& canvas, const std::vector<Vec2>& polygon, Color color)
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

    std::vector<double> crossings;
    for (int y = static_cast<int>(std::floor(minimumY));
         y <= static_cast<int>(std::ceil(maximumY)); ++y) {
        const double scanY = y + 0.5;
        crossings.clear();
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

inline void fillDisk(Canvas& canvas, Vec2 centre, int radius, Color color)
{
    const int cx = toPixel(centre.x);
    const int cy = toPixel(centre.y);
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                canvas.setPixel(cx + x, cy + y, color);
            }
        }
    }
}

// 3 x 5 bitmap digits, used to stamp frame numbers onto contact-sheet tiles.
inline void drawDigits(Canvas& canvas, int x, int y, int value, int scale, Color color)
{
    static constexpr std::array<std::array<std::uint8_t, 5>, 10> glyphs{{
        {7, 5, 5, 5, 7}, {2, 6, 2, 2, 7}, {7, 1, 7, 4, 7}, {7, 1, 7, 1, 7},
        {5, 5, 7, 1, 1}, {7, 4, 7, 1, 7}, {7, 4, 7, 5, 7}, {7, 1, 1, 1, 1},
        {7, 5, 7, 5, 7}, {7, 5, 7, 1, 7},
    }};
    const std::array<int, 2> digits{(value / 10) % 10, value % 10};
    for (std::size_t d = 0; d < digits.size(); ++d) {
        const auto& glyph = glyphs[static_cast<std::size_t>(digits[d])];
        for (int row = 0; row < 5; ++row) {
            for (int column = 0; column < 3; ++column) {
                if ((glyph[static_cast<std::size_t>(row)] >> (2 - column)) & 1U) {
                    const int px = x + (static_cast<int>(d) * 4 + column) * scale;
                    const int py = y + row * scale;
                    canvas.fillRect(px, py, px + scale - 1, py + scale - 1, color);
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Car model (local coordinates, nose towards -y, same shape as the
// checkpoint2/transformations demo)
// ---------------------------------------------------------------------------

struct Component {
    std::string_view name;
    std::vector<Vec2> polygon; // model-space vertices
    Color fill;
    Color edge;
};

[[nodiscard]] inline std::vector<Vec2> rectangle(double x0, double y0, double x1, double y1)
{
    return {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
}

// The centre of the car's bounding box (x in [-40, 40], y in [-78, 82]).
// Rotation and scaling are applied about this point.
inline constexpr Vec2 kCarCentre{0.0, 2.0};

// Components in draw order (back to front).
[[nodiscard]] inline std::vector<Component> carModel()
{
    using namespace palette;
    return {
        {"tyre front-left", rectangle(-38, -48, -24, -13), tyre, outline},
        {"tyre front-right", rectangle(24, -48, 38, -13), tyre, outline},
        {"tyre rear-left", rectangle(-40, 31, -24, 69), tyre, outline},
        {"tyre rear-right", rectangle(24, 31, 40, 69), tyre, outline},
        {"front wing", rectangle(-34, -78, 34, -69), wing, outline},
        {"rear wing", rectangle(-31, 72, 31, 82), wing, outline},
        {"chassis",
         {{-8, -72}, {8, -72}, {12, -35}, {25, -18}, {27, 22}, {18, 52},
          {12, 78}, {-12, 78}, {-18, 52}, {-27, 22}, {-25, -18}, {-12, -35}},
         body, outline},
        {"cockpit", rectangle(-10, -8, 10, 29), cockpit, outline},
        {"helmet", rectangle(-6, -1, 6, 13), helmet, helmet},
        {"stripe front-left", rectangle(-38, -31.5, -24, -28.5), wheelStripe, wheelStripe},
        {"stripe front-right", rectangle(24, -31.5, 38, -28.5), wheelStripe, wheelStripe},
        {"stripe rear-left", rectangle(-40, 48.5, -24, 51.5), wheelStripe, wheelStripe},
        {"stripe rear-right", rectangle(24, 48.5, 40, 51.5), wheelStripe, wheelStripe},
    };
}

// ---------------------------------------------------------------------------
// Pose -> matrix
// ---------------------------------------------------------------------------

struct Pose {
    Vec2 position;           // where the car centre is placed on screen
    double rotationDegrees;  // 90 = nose pointing right (screen y points down)
    double scaleX;           // across the car (model x)
    double scaleY;           // along the car (model y)
};

// M = T(position) * R(theta) * S(sx, sy) * T(-centre)
//
// Read right to left: move the car centre to the origin, scale and rotate
// about it, then place it at `position`. Because T(-centre) runs first, the
// centre is the fixed pivot of both rotation and scaling.
[[nodiscard]] inline Mat3 poseMatrix(const Pose& pose)
{
    return Mat3::translation(pose.position.x, pose.position.y) *
           Mat3::rotationDegrees(pose.rotationDegrees) *
           Mat3::scaling(pose.scaleX, pose.scaleY) *
           Mat3::translation(-kCarCentre.x, -kCarCentre.y);
}

// A component after placement. The matrix is stored so tests can confirm that
// every component of one frame received exactly the same transformation.
struct PlacedComponent {
    std::string_view name;
    Mat3 matrix;
    std::vector<Vec2> polygon; // screen-space vertices
    Color fill;
    Color edge;
};

[[nodiscard]] inline std::vector<PlacedComponent> placeCar(
    const std::vector<Component>& model, const Mat3& matrix)
{
    std::vector<PlacedComponent> placed;
    placed.reserve(model.size());
    for (const Component& component : model) {
        placed.push_back({component.name, matrix,
                          transformPolygon(component.polygon, matrix),
                          component.fill, component.edge});
    }
    return placed;
}

// ---------------------------------------------------------------------------
// Animation schedule: keyframes with linear interpolation
// ---------------------------------------------------------------------------

enum class Phase { Translation, Rotation, Scaling, Combined };

struct Keyframe {
    int frame;
    Pose pose;
};

inline constexpr int kFrameCount = 25; // frames 00 ... 24
inline constexpr int kLastFrame = kFrameCount - 1;
inline constexpr int kCanvasWidth = 640;
inline constexpr int kCanvasHeight = 420;

//  00-06 translation  : slide right 220 px, heading fixed
//  06-12 rotation     : one full turn (360 deg, 60 deg per frame) about the centre
//  12-18 scaling      : 1.6x wider and 0.6x shorter at frame 15, then back
//  18-24 combined     : translate (+180, +80) while turning +60 deg
inline constexpr std::array<Keyframe, 6> kKeyframes{{
    {0, {{110.0, 210.0}, 90.0, 1.0, 1.0}},
    {6, {{330.0, 210.0}, 90.0, 1.0, 1.0}},
    {12, {{330.0, 210.0}, 450.0, 1.0, 1.0}},
    {15, {{330.0, 210.0}, 450.0, 1.6, 0.6}},
    {18, {{330.0, 210.0}, 450.0, 1.0, 1.0}},
    {24, {{510.0, 290.0}, 510.0, 1.0, 1.0}},
}};

[[nodiscard]] constexpr double lerp(double a, double b, double t)
{
    return a + (b - a) * t;
}

[[nodiscard]] constexpr Pose lerp(const Pose& a, const Pose& b, double t)
{
    return {{lerp(a.position.x, b.position.x, t), lerp(a.position.y, b.position.y, t)},
            lerp(a.rotationDegrees, b.rotationDegrees, t),
            lerp(a.scaleX, b.scaleX, t),
            lerp(a.scaleY, b.scaleY, t)};
}

// Pose at an integer frame. Frames outside [0, kLastFrame] are clamped.
[[nodiscard]] constexpr Pose poseAtFrame(int frame)
{
    frame = std::clamp(frame, 0, kLastFrame);
    for (std::size_t i = 0; i + 1 < kKeyframes.size(); ++i) {
        const Keyframe& a = kKeyframes[i];
        const Keyframe& b = kKeyframes[i + 1];
        if (frame <= b.frame) {
            const double t = static_cast<double>(frame - a.frame) /
                             static_cast<double>(b.frame - a.frame);
            return lerp(a.pose, b.pose, t);
        }
    }
    return kKeyframes.back().pose;
}

[[nodiscard]] constexpr Phase phaseAtFrame(int frame)
{
    if (frame < 6) {
        return Phase::Translation;
    }
    if (frame < 12) {
        return Phase::Rotation;
    }
    if (frame < 18) {
        return Phase::Scaling;
    }
    return Phase::Combined;
}

[[nodiscard]] constexpr int phaseStartFrame(Phase phase)
{
    switch (phase) {
    case Phase::Translation: return 0;
    case Phase::Rotation: return 6;
    case Phase::Scaling: return 12;
    case Phase::Combined: return 18;
    }
    return 0;
}

[[nodiscard]] constexpr Color phaseColor(Phase phase)
{
    switch (phase) {
    case Phase::Translation: return palette::translation;
    case Phase::Rotation: return palette::rotation;
    case Phase::Scaling: return palette::scaling;
    case Phase::Combined: return palette::combined;
    }
    return palette::outline;
}

[[nodiscard]] constexpr std::string_view phaseName(Phase phase)
{
    switch (phase) {
    case Phase::Translation: return "translation";
    case Phase::Rotation: return "rotation about centre";
    case Phase::Scaling: return "non-uniform scaling";
    case Phase::Combined: return "translation + rotation";
    }
    return "";
}

// ---------------------------------------------------------------------------
// Frame rendering
// ---------------------------------------------------------------------------

inline void drawCar(Canvas& canvas, const std::vector<PlacedComponent>& car)
{
    for (const PlacedComponent& component : car) {
        scanlineFill(canvas, component.polygon, component.fill);
        outlinePolygon(canvas, component.polygon, component.edge);
    }
}

inline void drawCarGhost(Canvas& canvas, const std::vector<PlacedComponent>& car)
{
    for (const PlacedComponent& component : car) {
        outlinePolygon(canvas, component.polygon, palette::ghost);
    }
}

[[nodiscard]] inline Canvas renderFrame(int frame)
{
    Canvas canvas(kCanvasWidth, kCanvasHeight, palette::background);
    const Phase phase = phaseAtFrame(frame);

    // Header band in the colour of the current phase.
    canvas.fillRect(0, 0, kCanvasWidth - 1, 17, phaseColor(phase));

    // Track surface with dashed lane markings.
    canvas.fillRect(0, 28, kCanvasWidth - 1, 395, palette::track);
    for (int x = 8; x < kCanvasWidth; x += 32) {
        canvas.fillRect(x, 32, x + 15, 34, palette::laneMark);
        canvas.fillRect(x, 389, x + 15, 391, palette::laneMark);
    }

    const std::vector<Component> model = carModel();

    // Ghost outline of the pose at the start of the current phase, so each
    // frame shows the transformation relative to where the phase began.
    drawCarGhost(canvas, placeCar(model, poseMatrix(poseAtFrame(phaseStartFrame(phase)))));

    // Trail of the car centre for every frame up to this one.
    for (int previous = 0; previous <= frame; ++previous) {
        fillDisk(canvas, poseAtFrame(previous).position, 2, palette::trail);
    }

    const Pose pose = poseAtFrame(frame);
    drawCar(canvas, placeCar(model, poseMatrix(pose)));

    // Pivot marker: the car centre, which rotation and scaling keep fixed.
    fillDisk(canvas, pose.position, 3, palette::pivot);

    // Progress bar coloured by phase.
    for (int x = 0; x < kCanvasWidth; ++x) {
        const int barFrame = x * kLastFrame / (kCanvasWidth - 1);
        const bool done = x * kLastFrame <= frame * (kCanvasWidth - 1);
        const Color color = done ? phaseColor(phaseAtFrame(barFrame)) : palette::track;
        canvas.fillRect(x, 404, x, 413, color);
    }
    drawDigits(canvas, kCanvasWidth - 40, 4, frame, 2, palette::background);
    return canvas;
}

// ---------------------------------------------------------------------------
// Contact sheet
// ---------------------------------------------------------------------------

inline constexpr std::array<int, 9> kContactFrames{0, 3, 6, 8, 10, 13, 15, 21, 24};

// Box-filter 2:1 downsample of one frame, copied into the sheet at (left, top).
inline void blitHalf(Canvas& sheet, const Canvas& frame, int left, int top)
{
    for (int y = 0; y + 1 < frame.height(); y += 2) {
        for (int x = 0; x + 1 < frame.width(); x += 2) {
            const std::array<Color, 4> samples{frame.pixel(x, y), frame.pixel(x + 1, y),
                                               frame.pixel(x, y + 1), frame.pixel(x + 1, y + 1)};
            int r = 0;
            int g = 0;
            int b = 0;
            for (Color c : samples) {
                r += c.r;
                g += c.g;
                b += c.b;
            }
            sheet.setPixel(left + x / 2, top + y / 2,
                           {static_cast<std::uint8_t>((r + 2) / 4),
                            static_cast<std::uint8_t>((g + 2) / 4),
                            static_cast<std::uint8_t>((b + 2) / 4)});
        }
    }
}

[[nodiscard]] inline Canvas renderContactSheet()
{
    constexpr int columns = 3;
    constexpr int tileWidth = kCanvasWidth / 2;
    constexpr int tileHeight = kCanvasHeight / 2;
    constexpr int gap = 12;
    constexpr int labelHeight = 18;
    constexpr int rows = static_cast<int>(kContactFrames.size()) / columns;
    constexpr int width = columns * tileWidth + (columns + 1) * gap;
    constexpr int height = rows * (tileHeight + labelHeight) + (rows + 1) * gap;

    Canvas sheet(width, height, palette::background);
    for (std::size_t i = 0; i < kContactFrames.size(); ++i) {
        const int frame = kContactFrames[i];
        const int column = static_cast<int>(i) % columns;
        const int row = static_cast<int>(i) / columns;
        const int left = gap + column * (tileWidth + gap);
        const int top = gap + row * (tileHeight + labelHeight + gap);
        const Color border = phaseColor(phaseAtFrame(frame));

        sheet.fillRect(left - 3, top - 3, left + tileWidth + 2,
                       top + tileHeight + labelHeight + 2, border);
        sheet.fillRect(left, top + tileHeight, left + tileWidth - 1,
                       top + tileHeight + labelHeight - 1, palette::background);
        blitHalf(sheet, renderFrame(frame), left, top);
        drawDigits(sheet, left + 4, top + tileHeight + 3, frame, 2, border);
    }
    return sheet;
}

// ---------------------------------------------------------------------------
// Minimal PNG writer (8-bit RGB, zlib "stored" blocks, no compression library)
// ---------------------------------------------------------------------------

namespace png {

[[nodiscard]] inline std::uint32_t crc32(const std::uint8_t* data, std::size_t length,
                                         std::uint32_t crc = 0xFFFFFFFFU)
{
    for (std::size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            crc = (crc & 1U) != 0U ? (0xEDB88320U ^ (crc >> 1)) : (crc >> 1);
        }
    }
    return crc;
}

inline void putU32(std::vector<std::uint8_t>& out, std::uint32_t value)
{
    for (int shift = 24; shift >= 0; shift -= 8) {
        out.push_back(static_cast<std::uint8_t>((value >> shift) & 0xFFU));
    }
}

inline void putChunk(std::vector<std::uint8_t>& out, std::string_view type,
                     const std::vector<std::uint8_t>& data)
{
    putU32(out, static_cast<std::uint32_t>(data.size()));
    const std::size_t typeStart = out.size();
    out.insert(out.end(), type.begin(), type.end());
    out.insert(out.end(), data.begin(), data.end());
    putU32(out, crc32(out.data() + typeStart, out.size() - typeStart) ^ 0xFFFFFFFFU);
}

[[nodiscard]] inline std::vector<std::uint8_t> encode(const Canvas& canvas)
{
    const auto width = static_cast<std::uint32_t>(canvas.width());
    const auto height = static_cast<std::uint32_t>(canvas.height());

    // Each scanline: filter byte 0 (None) followed by RGB triples.
    std::vector<std::uint8_t> raw;
    for (int y = 0; y < canvas.height(); ++y) {
        raw.push_back(0);
        for (int x = 0; x < canvas.width(); ++x) {
            const Color c = canvas.pixel(x, y);
            raw.insert(raw.end(), {c.r, c.g, c.b});
        }
    }

    // zlib stream made of uncompressed deflate blocks (max 65535 bytes each).
    std::vector<std::uint8_t> zlib{0x78, 0x01};
    std::uint32_t adlerA = 1;
    std::uint32_t adlerB = 0;
    for (std::size_t offset = 0; offset < raw.size();) {
        const std::size_t length = std::min<std::size_t>(65535, raw.size() - offset);
        const bool last = offset + length == raw.size();
        const auto len16 = static_cast<std::uint16_t>(length);
        const auto nlen16 = static_cast<std::uint16_t>(~len16);
        zlib.insert(zlib.end(), {static_cast<std::uint8_t>(last ? 1 : 0),
                                 static_cast<std::uint8_t>(len16 & 0xFFU),
                                 static_cast<std::uint8_t>(len16 >> 8),
                                 static_cast<std::uint8_t>(nlen16 & 0xFFU),
                                 static_cast<std::uint8_t>(nlen16 >> 8)});
        for (std::size_t i = offset; i < offset + length; ++i) {
            zlib.push_back(raw[i]);
            adlerA = (adlerA + raw[i]) % 65521U;
            adlerB = (adlerB + adlerA) % 65521U;
        }
        offset += length;
    }
    putU32(zlib, (adlerB << 16) | adlerA);

    std::vector<std::uint8_t> out{137, 80, 78, 71, 13, 10, 26, 10};
    std::vector<std::uint8_t> header;
    putU32(header, width);
    putU32(header, height);
    header.insert(header.end(), {8, 2, 0, 0, 0}); // 8-bit, RGB, deflate, no filter, no interlace
    putChunk(out, "IHDR", header);
    putChunk(out, "IDAT", zlib);
    putChunk(out, "IEND", {});
    return out;
}

[[nodiscard]] inline bool write(const std::filesystem::path& path, const Canvas& canvas)
{
    const std::vector<std::uint8_t> bytes = encode(canvas);
    std::ofstream output(path, std::ios::binary);
    if (!output) {
        return false;
    }
    output.write(reinterpret_cast<const char*>(bytes.data()),
                 static_cast<std::streamsize>(bytes.size()));
    return static_cast<bool>(output);
}

} // namespace png

} // namespace apex3d::car
