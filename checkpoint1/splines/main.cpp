// Apex3D - Checkpoint 1: Catmull-Rom splines in Pure C++20.
// No graphics library is used. Every pixel is written to an RGB canvas and
// exported as a binary PPM image.

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

struct Color {
    std::uint8_t r{};
    std::uint8_t g{};
    std::uint8_t b{};
};

struct Point {
    double x{};
    double y{};
};

[[nodiscard]] Point operator+(Point a, Point b) { return {a.x + b.x, a.y + b.y}; }
[[nodiscard]] Point operator-(Point a, Point b) { return {a.x - b.x, a.y - b.y}; }
[[nodiscard]] Point operator*(Point p, double scalar) { return {p.x * scalar, p.y * scalar}; }

namespace Palette {
constexpr Color background{9, 12, 20};
constexpr Color panel{18, 23, 34};
constexpr Color grid{35, 42, 58};
constexpr Color raw{118, 128, 150};
constexpr Color handle{255, 145, 35};
constexpr Color control{255, 75, 95};
constexpr Color spline{65, 220, 255};
constexpr Color racingLine{55, 230, 125};
constexpr Color car{255, 215, 45};
constexpr Color carAccent{245, 75, 55};
constexpr Color white{235, 240, 248};
} // namespace Palette

class Canvas {
public:
    Canvas(int width, int height, Color background)
        : width_(width), height_(height),
          pixels_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), background)
    {
    }

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
        for (const Color color : pixels_) {
            const std::array<char, 3> bytes{
                static_cast<char>(color.r),
                static_cast<char>(color.g),
                static_cast<char>(color.b),
            };
            output.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        }
        return static_cast<bool>(output);
    }

    [[nodiscard]] int width() const { return width_; }
    [[nodiscard]] int height() const { return height_; }

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

void drawDisk(Canvas& canvas, Point centre, int radius, Color color)
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

// Uniform Catmull-Rom segment. The curve interpolates P1 at t=0 and P2 at t=1.
[[nodiscard]] Point catmullRom(Point p0, Point p1, Point p2, Point p3, double t)
{
    const double t2 = t * t;
    const double t3 = t2 * t;
    return (p1 * 2.0 +
            (p2 - p0) * t +
            (p0 * 2.0 - p1 * 5.0 + p2 * 4.0 - p3) * t2 +
            (p0 * -1.0 + p1 * 3.0 - p2 * 3.0 + p3) * t3) *
           0.5;
}

// Analytic derivative used to orient car poses along the path.
[[nodiscard]] Point catmullRomTangent(Point p0, Point p1, Point p2, Point p3, double t)
{
    const double t2 = t * t;
    return ((p2 - p0) +
            (p0 * 2.0 - p1 * 5.0 + p2 * 4.0 - p3) * (2.0 * t) +
            (p0 * -1.0 + p1 * 3.0 - p2 * 3.0 + p3) * (3.0 * t2)) *
           0.5;
}

[[nodiscard]] bool nearlyEqual(double a, double b, double tolerance = 1e-9)
{
    return std::abs(a - b) <= tolerance;
}

[[nodiscard]] bool nearlyEqual(Point a, Point b, double tolerance = 1e-9)
{
    return nearlyEqual(a.x, b.x, tolerance) && nearlyEqual(a.y, b.y, tolerance);
}

[[nodiscard]] bool runSelfChecks()
{
    bool allPassed = true;
    const auto report = [&](std::string_view name, bool passed) {
        std::cout << "  [self-check] " << name << ": " << (passed ? "PASS" : "FAIL") << '\n';
        allPassed = allPassed && passed;
    };

    const Point p0{-2.0, 1.0};
    const Point p1{0.0, 2.0};
    const Point p2{4.0, 5.0};
    const Point p3{8.0, 3.0};
    report("t=0 interpolates P1", nearlyEqual(catmullRom(p0, p1, p2, p3, 0.0), p1));
    report("t=1 interpolates P2", nearlyEqual(catmullRom(p0, p1, p2, p3, 1.0), p2));

    const Point same{7.0, -3.0};
    report("degenerate segment", nearlyEqual(catmullRom(same, same, same, same, 0.37), same));

    bool collinear = true;
    const Point l0{0.0, 0.0};
    const Point l1{1.0, 1.0};
    const Point l2{2.0, 2.0};
    const Point l3{3.0, 3.0};
    for (int step = 0; step <= 20; ++step) {
        const Point sample = catmullRom(l0, l1, l2, l3, step / 20.0);
        collinear = collinear && nearlyEqual(sample.x, sample.y);
    }
    report("collinear controls", collinear);

    Canvas tiny(2, 2, Palette::background);
    report("canvas bounds", tiny.setPixel(1, 1, Palette::white) &&
                                !tiny.setPixel(-1, 0, Palette::white) &&
                                !tiny.setPixel(2, 1, Palette::white));
    return allPassed;
}

struct RenderStats {
    int segments{};
    int samples{};
};

constexpr int kSamplesPerSegment = 120;

void drawSplineSegment(Canvas& canvas, Point p0, Point p1, Point p2, Point p3,
                       Color color, RenderStats& stats)
{
    Point previous = catmullRom(p0, p1, p2, p3, 0.0);
    for (int step = 1; step <= kSamplesPerSegment; ++step) {
        const double t = static_cast<double>(step) / kSamplesPerSegment;
        const Point current = catmullRom(p0, p1, p2, p3, t);
        drawLine(canvas,
                 static_cast<int>(std::lround(previous.x)),
                 static_cast<int>(std::lround(previous.y)),
                 static_cast<int>(std::lround(current.x)),
                 static_cast<int>(std::lround(current.y)), color);
        previous = current;
        ++stats.samples;
    }
    ++stats.segments;
}

void drawOpenSpline(Canvas& canvas, const std::vector<Point>& points,
                    Color curveColor, Color polygonColor, RenderStats& stats)
{
    if (points.size() < 2) {
        return;
    }
    for (std::size_t i = 1; i < points.size(); ++i) {
        drawLine(canvas,
                 static_cast<int>(std::lround(points[i - 1].x)),
                 static_cast<int>(std::lround(points[i - 1].y)),
                 static_cast<int>(std::lround(points[i].x)),
                 static_cast<int>(std::lround(points[i].y)), polygonColor);
    }
    for (std::size_t i = 0; i + 1 < points.size(); ++i) {
        const Point p0 = i == 0 ? points[i] : points[i - 1];
        const Point p1 = points[i];
        const Point p2 = points[i + 1];
        const Point p3 = i + 2 < points.size() ? points[i + 2] : points[i + 1];
        drawSplineSegment(canvas, p0, p1, p2, p3, curveColor, stats);
    }
    for (Point point : points) {
        drawDisk(canvas, point, 5, Palette::control);
    }
}

[[nodiscard]] Point closedPoint(const std::vector<Point>& points, int index)
{
    const int count = static_cast<int>(points.size());
    return points[static_cast<std::size_t>((index % count + count) % count)];
}

void drawClosedSpline(Canvas& canvas, const std::vector<Point>& points,
                      Color curveColor, Color polygonColor, RenderStats& stats)
{
    const int count = static_cast<int>(points.size());
    for (int i = 0; i < count; ++i) {
        const Point a = closedPoint(points, i);
        const Point b = closedPoint(points, i + 1);
        drawLine(canvas, static_cast<int>(std::lround(a.x)), static_cast<int>(std::lround(a.y)),
                 static_cast<int>(std::lround(b.x)), static_cast<int>(std::lround(b.y)),
                 polygonColor);
        drawSplineSegment(canvas, closedPoint(points, i - 1), a, b,
                          closedPoint(points, i + 2), curveColor, stats);
        drawDisk(canvas, a, 4, Palette::control);
    }
}

void drawGrid(Canvas& canvas, int x0, int y0, int x1, int y1, int spacing)
{
    canvas.fillRect(x0, y0, x1, y1, Palette::panel);
    for (int x = x0; x <= x1; x += spacing) {
        drawLine(canvas, x, y0, x, y1, Palette::grid);
    }
    for (int y = y0; y <= y1; y += spacing) {
        drawLine(canvas, x0, y, x1, y, Palette::grid);
    }
}

void drawCarPose(Canvas& canvas, Point position, Point tangent)
{
    const double length = std::hypot(tangent.x, tangent.y);
    if (length < 1e-9) {
        return;
    }
    const Point forward{tangent.x / length, tangent.y / length};
    const Point side{-forward.y, forward.x};
    const Point nose = position + forward * 12.0;
    const Point rearLeft = position - forward * 9.0 + side * 6.0;
    const Point rearRight = position - forward * 9.0 - side * 6.0;
    drawLine(canvas, static_cast<int>(std::lround(nose.x)), static_cast<int>(std::lround(nose.y)),
             static_cast<int>(std::lround(rearLeft.x)), static_cast<int>(std::lround(rearLeft.y)),
             Palette::car);
    drawLine(canvas, static_cast<int>(std::lround(rearLeft.x)), static_cast<int>(std::lround(rearLeft.y)),
             static_cast<int>(std::lround(rearRight.x)), static_cast<int>(std::lround(rearRight.y)),
             Palette::carAccent);
    drawLine(canvas, static_cast<int>(std::lround(rearRight.x)), static_cast<int>(std::lround(rearRight.y)),
             static_cast<int>(std::lround(nose.x)), static_cast<int>(std::lround(nose.y)),
             Palette::car);
    drawDisk(canvas, position, 2, Palette::white);
}

int main(int argc, char* argv[])
{
    std::cout << "Apex3D Catmull-Rom self-checks:\n";
    if (!runSelfChecks()) {
        std::cerr << "ERROR: spline self-check failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "  All self-checks passed.\n\n";

    constexpr int width = 1200;
    constexpr int height = 720;
    Canvas canvas(width, height, Palette::background);
    drawGrid(canvas, 20, 20, 385, 620, 50);
    drawGrid(canvas, 410, 20, 790, 620, 50);
    drawGrid(canvas, 815, 20, 1179, 620, 50);

    RenderStats stats;
    const std::vector<Point> openControls{
        {45, 500}, {100, 180}, {180, 455}, {260, 145}, {350, 470},
    };
    drawOpenSpline(canvas, openControls, Palette::spline, Palette::handle, stats);

    const std::vector<Point> monza{
        {515, 535}, {485, 380}, {460, 225}, {505, 100}, {625, 70},
        {735, 165}, {750, 315}, {700, 435}, {610, 475}, {560, 420},
        {590, 555},
    };
    drawClosedSpline(canvas, monza, Palette::racingLine, Palette::raw, stats);

    const std::vector<Point> motionPath{
        {845, 510}, {850, 300}, {905, 125}, {1030, 90}, {1135, 210},
        {1140, 430}, {1040, 540}, {930, 470},
    };
    drawClosedSpline(canvas, motionPath, Palette::spline, Palette::raw, stats);
    const int count = static_cast<int>(motionPath.size());
    for (int i = 0; i < count; ++i) {
        constexpr double t = 0.55;
        const Point position = catmullRom(closedPoint(motionPath, i - 1), closedPoint(motionPath, i),
                                          closedPoint(motionPath, i + 1), closedPoint(motionPath, i + 2), t);
        const Point tangent = catmullRomTangent(closedPoint(motionPath, i - 1), closedPoint(motionPath, i),
                                                closedPoint(motionPath, i + 1), closedPoint(motionPath, i + 2), t);
        drawCarPose(canvas, position, tangent);
    }

    // Bottom colour key: raw polyline, spline, control points, oriented poses.
    canvas.fillRect(20, 645, 1179, 700, Palette::panel);
    drawLine(canvas, 55, 672, 175, 672, Palette::raw);
    drawLine(canvas, 265, 672, 385, 672, Palette::spline);
    for (int x = 490; x <= 610; x += 30) {
        drawDisk(canvas, {static_cast<double>(x), 672}, 5, Palette::control);
    }
    drawCarPose(canvas, {820, 672}, {1, 0});
    drawCarPose(canvas, {900, 672}, {0.7, -0.7});
    drawCarPose(canvas, {980, 672}, {0, -1});

    const std::filesystem::path outputDirectory = argc > 1
        ? std::filesystem::path(argv[1])
        : std::filesystem::path(__FILE__).parent_path() / "output";
    std::filesystem::create_directories(outputDirectory);
    const std::filesystem::path outputPath = outputDirectory / "splines_demo.ppm";
    if (!canvas.writePpm(outputPath)) {
        std::cerr << "ERROR: failed to write " << outputPath << '\n';
        return EXIT_FAILURE;
    }

    std::cout << "Apex3D Checkpoint 1 - Catmull-Rom Splines\n"
              << "  Canvas             : " << canvas.width() << " x " << canvas.height() << " RGB\n"
              << "  Spline segments    : " << stats.segments << '\n'
              << "  Samples generated  : " << stats.samples << '\n'
              << "  Samples per segment: " << kSamplesPerSegment << '\n'
              << "  Animated poses     : " << count << '\n'
              << "  Output             : " << outputPath.string() << '\n';
    return EXIT_SUCCESS;
}
