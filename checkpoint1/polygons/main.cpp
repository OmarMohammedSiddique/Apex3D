// Apex3D - Checkpoint 1: Pure C++ polygon representation, scanline filling,
// and Sutherland-Hodgman clipping.
//
// No graphics libraries are used. Every pixel is written by the algorithms in
// this file into an in-memory RGB canvas, which is then saved as a binary PPM.
//
// Build and run from the repository root:
//   c++ -std=c++20 -Wall -Wextra -Wpedantic checkpoint1/polygons/main.cpp -o apex3d-polygons
//   ./apex3d-polygons

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Canvas
// ---------------------------------------------------------------------------

struct Color {
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

class Canvas {
public:
    Canvas(int width, int height, Color background)
        : width_(width), height_(height),
          pixels_(static_cast<std::size_t>(width) * static_cast<std::size_t>(height), background) {}

    int width() const { return width_; }
    int height() const { return height_; }

    // Writes one pixel. Coordinates outside the canvas are silently ignored,
    // so geometry may extend past the image edges without corrupting memory.
    void setPixel(int x, int y, Color color) {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) {
            return;
        }
        pixels_[static_cast<std::size_t>(y) * static_cast<std::size_t>(width_) +
                static_cast<std::size_t>(x)] = color;
    }

    // Saves the canvas as a binary (P6) PPM image.
    bool writePPM(const std::filesystem::path& path) const {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            return false;
        }
        file << "P6\n" << width_ << ' ' << height_ << "\n255\n";
        for (const Color& c : pixels_) {
            const char rgb[3] = {static_cast<char>(c.r), static_cast<char>(c.g), static_cast<char>(c.b)};
            file.write(rgb, 3);
        }
        return static_cast<bool>(file);
    }

private:
    int width_;
    int height_;
    std::vector<Color> pixels_;
};

// ---------------------------------------------------------------------------
// Polygon representation
// ---------------------------------------------------------------------------

// A vertex in screen space: x grows to the right, y grows downwards.
struct Point {
    double x;
    double y;
};

// A polygon is an ordered list of vertices. The edge list is implicit:
// vertex i connects to vertex (i + 1) % n, so the last vertex closes the shape.
using Polygon = std::vector<Point>;

// Axis-aligned rectangular clipping window.
struct ClipRect {
    double xMin;
    double yMin;
    double xMax;
    double yMax;
};

// ---------------------------------------------------------------------------
// Line rasterisation (Bresenham) used for polygon edges
// ---------------------------------------------------------------------------

int roundToPixel(double value) {
    return static_cast<int>(std::lround(value));
}

// Integer Bresenham line that works in all octants. The decision parameter
// `error` tracks the accumulated distance from the ideal line.
void drawLine(Canvas& canvas, Point from, Point to, Color color) {
    int x0 = roundToPixel(from.x);
    int y0 = roundToPixel(from.y);
    const int x1 = roundToPixel(to.x);
    const int y1 = roundToPixel(to.y);

    const int dx = std::abs(x1 - x0);
    const int dy = -std::abs(y1 - y0);
    const int stepX = x0 < x1 ? 1 : -1;
    const int stepY = y0 < y1 ? 1 : -1;
    int error = dx + dy;

    while (true) {
        canvas.setPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int doubledError = 2 * error;
        if (doubledError >= dy) {
            error += dy;
            x0 += stepX;
        }
        if (doubledError <= dx) {
            error += dx;
            y0 += stepY;
        }
    }
}

// Draws every edge of the polygon, including the closing edge.
void drawPolygonOutline(Canvas& canvas, const Polygon& polygon, Color color) {
    const std::size_t n = polygon.size();
    if (n < 2) {
        return;
    }
    for (std::size_t i = 0; i < n; ++i) {
        drawLine(canvas, polygon[i], polygon[(i + 1) % n], color);
    }
}

void drawRectOutline(Canvas& canvas, const ClipRect& rect, Color color) {
    const Polygon corners = {
        {rect.xMin, rect.yMin}, {rect.xMax, rect.yMin}, {rect.xMax, rect.yMax}, {rect.xMin, rect.yMax}};
    drawPolygonOutline(canvas, corners, color);
}

// ---------------------------------------------------------------------------
// Scanline polygon filling
// ---------------------------------------------------------------------------

// Fills a polygon with the even-odd rule. For every scanline the centre of the
// pixel row (y + 0.5) is intersected with each edge, the intersection x values
// are sorted, and the spans between successive pairs are filled.
//
// An edge contributes an intersection only when the scanline lies in the
// half-open interval [yLow, yHigh). This counts a shared vertex exactly once
// and skips horizontal edges, which never cross a scanline.
void scanlineFill(Canvas& canvas, const Polygon& polygon, Color color) {
    const std::size_t n = polygon.size();
    if (n < 3) {
        return;
    }

    double minY = polygon[0].y;
    double maxY = polygon[0].y;
    for (const Point& p : polygon) {
        minY = std::min(minY, p.y);
        maxY = std::max(maxY, p.y);
    }

    const int firstRow = std::max(0, static_cast<int>(std::floor(minY)));
    const int lastRow = std::min(canvas.height() - 1, static_cast<int>(std::ceil(maxY)));

    std::vector<double> intersections;
    for (int row = firstRow; row <= lastRow; ++row) {
        const double scanY = row + 0.5;
        intersections.clear();

        for (std::size_t i = 0; i < n; ++i) {
            const Point& a = polygon[i];
            const Point& b = polygon[(i + 1) % n];
            const bool crosses = (a.y <= scanY && b.y > scanY) || (b.y <= scanY && a.y > scanY);
            if (!crosses) {
                continue;
            }
            // Inverse slope: how far x moves for each unit step in y.
            const double inverseSlope = (b.x - a.x) / (b.y - a.y);
            intersections.push_back(a.x + (scanY - a.y) * inverseSlope);
        }

        std::sort(intersections.begin(), intersections.end());

        for (std::size_t i = 0; i + 1 < intersections.size(); i += 2) {
            // Fill pixels whose centres (x + 0.5) lie inside [left, right).
            const int startX = static_cast<int>(std::ceil(intersections[i] - 0.5));
            const int endX = static_cast<int>(std::ceil(intersections[i + 1] - 0.5)) - 1;
            for (int x = std::max(startX, 0); x <= std::min(endX, canvas.width() - 1); ++x) {
                canvas.setPixel(x, row, color);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Sutherland-Hodgman polygon clipping
// ---------------------------------------------------------------------------

enum class Boundary { Left, Right, Top, Bottom };

bool isInside(Point p, Boundary boundary, const ClipRect& rect) {
    switch (boundary) {
    case Boundary::Left:
        return p.x >= rect.xMin;
    case Boundary::Right:
        return p.x <= rect.xMax;
    case Boundary::Top:
        return p.y >= rect.yMin;
    case Boundary::Bottom:
        return p.y <= rect.yMax;
    }
    return false;
}

// Intersection of segment a-b with one infinite clipping line, found by
// solving the parametric line equation P(t) = a + t * (b - a) for t.
Point intersectBoundary(Point a, Point b, Boundary boundary, const ClipRect& rect) {
    switch (boundary) {
    case Boundary::Left:
    case Boundary::Right: {
        const double edgeX = boundary == Boundary::Left ? rect.xMin : rect.xMax;
        const double t = (edgeX - a.x) / (b.x - a.x);
        return {edgeX, a.y + t * (b.y - a.y)};
    }
    case Boundary::Top:
    case Boundary::Bottom: {
        const double edgeY = boundary == Boundary::Top ? rect.yMin : rect.yMax;
        const double t = (edgeY - a.y) / (b.y - a.y);
        return {a.x + t * (b.x - a.x), edgeY};
    }
    }
    return a;
}

// Clips the polygon against a single boundary. Each edge (previous -> current)
// falls into one of four cases:
//   inside  -> inside : output current
//   inside  -> outside: output the intersection
//   outside -> inside : output the intersection, then current
//   outside -> outside: output nothing
Polygon clipAgainstBoundary(const Polygon& input, Boundary boundary, const ClipRect& rect) {
    Polygon output;
    if (input.empty()) {
        return output;
    }

    Point previous = input.back();
    bool previousInside = isInside(previous, boundary, rect);

    for (const Point& current : input) {
        const bool currentInside = isInside(current, boundary, rect);
        if (currentInside) {
            if (!previousInside) {
                output.push_back(intersectBoundary(previous, current, boundary, rect));
            }
            output.push_back(current);
        } else if (previousInside) {
            output.push_back(intersectBoundary(previous, current, boundary, rect));
        }
        previous = current;
        previousInside = currentInside;
    }
    return output;
}

// Sutherland-Hodgman: the output of each boundary pass becomes the input of
// the next, so the polygon is clipped by one window edge at a time.
Polygon sutherlandHodgmanClip(const Polygon& subject, const ClipRect& rect) {
    Polygon result = subject;
    for (Boundary boundary : {Boundary::Left, Boundary::Right, Boundary::Top, Boundary::Bottom}) {
        result = clipAgainstBoundary(result, boundary, rect);
    }
    return result;
}

bool allVerticesInside(const Polygon& polygon, const ClipRect& rect) {
    constexpr double epsilon = 1e-9;
    return std::all_of(polygon.begin(), polygon.end(), [&](Point p) {
        return p.x >= rect.xMin - epsilon && p.x <= rect.xMax + epsilon &&
               p.y >= rect.yMin - epsilon && p.y <= rect.yMax + epsilon;
    });
}

// ---------------------------------------------------------------------------
// Scene: simplified Formula 1 car on a Monza track section
// ---------------------------------------------------------------------------

// Helpers for building axis-aligned quads and left/right mirrored parts.
Polygon makeRect(double x0, double y0, double x1, double y1) {
    return {{x0, y0}, {x1, y0}, {x1, y1}, {x0, y1}};
}

Polygon mirrorX(const Polygon& polygon, double axisX) {
    Polygon mirrored;
    mirrored.reserve(polygon.size());
    for (auto it = polygon.rbegin(); it != polygon.rend(); ++it) {
        mirrored.push_back({2.0 * axisX - it->x, it->y});
    }
    return mirrored;
}

void drawTrackSection(Canvas& canvas, double left, double right, double top, double bottom) {
    const Color grass{38, 110, 52};
    const Color asphalt{72, 72, 80};
    const Color kerbRed{200, 30, 35};
    const Color kerbWhite{235, 235, 235};
    const Color lineWhite{245, 245, 245};

    scanlineFill(canvas, makeRect(left, top, right, bottom), grass);

    const double trackLeft = left + 70.0;
    const double trackRight = right - 70.0;
    scanlineFill(canvas, makeRect(trackLeft, top, trackRight, bottom), asphalt);

    // Alternating red and white kerb blocks, each a small quad.
    const double kerbWidth = 14.0;
    const double blockLength = 24.0;
    int index = 0;
    for (double y = top; y < bottom; y += blockLength, ++index) {
        const double yEnd = std::min(y + blockLength, bottom);
        const Color kerb = index % 2 == 0 ? kerbRed : kerbWhite;
        scanlineFill(canvas, makeRect(trackLeft - kerbWidth, y, trackLeft, yEnd), kerb);
        scanlineFill(canvas, makeRect(trackRight, y, trackRight + kerbWidth, yEnd), kerb);
    }

    // Track limit lines drawn with the line rasteriser.
    drawLine(canvas, {trackLeft + 3.0, top}, {trackLeft + 3.0, bottom - 1.0}, lineWhite);
    drawLine(canvas, {trackRight - 4.0, top}, {trackRight - 4.0, bottom - 1.0}, lineWhite);
}

void drawFormulaOneCar(Canvas& canvas, double cx, double noseY) {
    const Color body{215, 25, 32};
    const Color carbon{28, 28, 32};
    const Color tyre{12, 12, 12};
    const Color tyreStripe{240, 200, 30};
    const Color cockpit{20, 20, 24};
    const Color helmet{250, 210, 40};
    const Color outline{255, 255, 255};

    const double y = noseY;

    // Front wing with endplates.
    const Polygon frontWing = {
        {cx - 62, y + 10}, {cx + 62, y + 10}, {cx + 66, y + 30}, {cx - 66, y + 30}};
    scanlineFill(canvas, frontWing, carbon);
    scanlineFill(canvas, makeRect(cx - 70, y + 4, cx - 62, y + 34), body);
    scanlineFill(canvas, mirrorX(makeRect(cx - 70, y + 4, cx - 62, y + 34), cx), body);

    // Wheels, left side then mirrored to the right.
    const Polygon frontWheel = makeRect(cx - 74, y + 55, cx - 44, y + 110);
    const Polygon rearWheel = makeRect(cx - 80, y + 245, cx - 42, y + 310);
    for (const Polygon& wheel : {frontWheel, rearWheel}) {
        scanlineFill(canvas, wheel, tyre);
        scanlineFill(canvas, mirrorX(wheel, cx), tyre);
    }
    // Yellow sidewall stripes mark the tyre compound.
    for (const Polygon& stripe : {makeRect(cx - 74, y + 80, cx - 44, y + 84),
                                  makeRect(cx - 80, y + 275, cx - 42, y + 279)}) {
        scanlineFill(canvas, stripe, tyreStripe);
        scanlineFill(canvas, mirrorX(stripe, cx), tyreStripe);
    }

    // Suspension arms connect the wheels to the chassis.
    drawLine(canvas, {cx - 44, y + 70}, {cx - 10, y + 95}, carbon);
    drawLine(canvas, {cx + 44, y + 70}, {cx + 10, y + 95}, carbon);
    drawLine(canvas, {cx - 42, y + 270}, {cx - 25, y + 255}, carbon);
    drawLine(canvas, {cx + 42, y + 270}, {cx + 25, y + 255}, carbon);

    // Main body: nose cone, sidepods and engine cover as one concave polygon.
    const Polygon chassis = {
        {cx - 7, y + 30},  {cx + 7, y + 30},  {cx + 14, y + 120}, {cx + 40, y + 150},
        {cx + 44, y + 220}, {cx + 26, y + 260}, {cx + 18, y + 315}, {cx - 18, y + 315},
        {cx - 26, y + 260}, {cx - 44, y + 220}, {cx - 40, y + 150}, {cx - 14, y + 120}};
    scanlineFill(canvas, chassis, body);
    drawPolygonOutline(canvas, chassis, outline);

    // Cockpit opening and driver helmet.
    const Polygon cockpitOpening = {
        {cx - 11, y + 130}, {cx + 11, y + 130}, {cx + 14, y + 185}, {cx - 14, y + 185}};
    scanlineFill(canvas, cockpitOpening, cockpit);
    scanlineFill(canvas, makeRect(cx - 7, y + 150, cx + 7, y + 168), helmet);

    // Rear wing.
    const Polygon rearWing = makeRect(cx - 52, y + 318, cx + 52, y + 340);
    scanlineFill(canvas, rearWing, carbon);
    drawPolygonOutline(canvas, rearWing, outline);
}

// ---------------------------------------------------------------------------
// Program
// ---------------------------------------------------------------------------

void printPolygon(const char* label, const Polygon& polygon) {
    std::cout << label << " (" << polygon.size() << " vertices):";
    for (const Point& p : polygon) {
        std::cout << " (" << p.x << ", " << p.y << ")";
    }
    std::cout << '\n';
}

int main(int argc, char* argv[]) {
    constexpr int width = 900;
    constexpr int height = 520;
    const Color background{24, 26, 32};
    Canvas canvas(width, height, background);

    // ---- Left panel: clipping demonstration -------------------------------
    const ClipRect clipWindow{110.0, 120.0, 380.0, 390.0};

    // A concave polygon that crosses every side of the clipping window.
    const Polygon original = {
        {60.0, 210.0}, {240.0, 60.0}, {300.0, 180.0}, {440.0, 150.0},
        {330.0, 270.0}, {450.0, 440.0}, {220.0, 330.0}, {130.0, 470.0}};

    const Polygon clipped = sutherlandHodgmanClip(original, clipWindow);

    const Color clippedFill{30, 120, 70};
    const Color clippedEdge{90, 255, 140};
    const Color originalEdge{255, 70, 70};
    const Color clipBoundary{255, 215, 0};

    scanlineFill(canvas, clipped, clippedFill);
    drawPolygonOutline(canvas, original, originalEdge);
    drawPolygonOutline(canvas, clipped, clippedEdge);
    drawRectOutline(canvas, clipWindow, clipBoundary);

    // Panel separator.
    drawLine(canvas, {500.0, 0.0}, {500.0, height - 1.0}, Color{90, 90, 100});

    // ---- Right panel: filled polygon F1 car on a track section ------------
    drawTrackSection(canvas, 520.0, 880.0, 20.0, 500.0);
    drawFormulaOneCar(canvas, 700.0, 80.0);

    // ---- Report -----------------------------------------------------------
    printPolygon("Original polygon", original);
    printPolygon("Clipped polygon ", clipped);
    std::cout << "Clipping window: x [" << clipWindow.xMin << ", " << clipWindow.xMax << "], y ["
              << clipWindow.yMin << ", " << clipWindow.yMax << "]\n";

    const bool inside = allVerticesInside(clipped, clipWindow);
    std::cout << "All clipped vertices inside clipping window: " << (inside ? "yes" : "NO") << '\n';

    // Default output: the output/ folder next to this source file.
    const std::filesystem::path outputPath =
        argc > 1 ? std::filesystem::path(argv[1])
                 : std::filesystem::path(__FILE__).parent_path() / "output" / "polygons_demo.ppm";
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }
    if (!canvas.writePPM(outputPath)) {
        std::cerr << "Failed to write " << outputPath.string() << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "Wrote " << outputPath.string() << " (" << width << "x" << height << ")\n";

    return inside ? EXIT_SUCCESS : EXIT_FAILURE;
}
