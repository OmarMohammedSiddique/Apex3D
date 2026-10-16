#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
constexpr int kWidth = 1280;
constexpr int kHeight = 720;
constexpr const char* kOutputPath =
    "checkpoint1/circles/output/circles_ellipses_demo.ppm";

struct Color {
    std::uint8_t red;
    std::uint8_t green;
    std::uint8_t blue;
};
static_assert(sizeof(Color) == 3, "RGB24 requires three bytes per pixel");

constexpr Color kBackground{0x12, 0x14, 0x1A};
constexpr Color kGrid{0x20, 0x27, 0x33};
constexpr Color kFrame{0x33, 0x42, 0x57};
constexpr Color kCyan{0x3E, 0xDB, 0xF2};
constexpr Color kYellow{0xFF, 0xD1, 0x66};
constexpr Color kGreen{0x64, 0xD9, 0xA0};
constexpr Color kRed{0xEF, 0x44, 0x44};
constexpr Color kTyre{0xBE, 0xC6, 0xD1};
constexpr Color kRim{0x77, 0x92, 0xAA};
constexpr Color kWhite{0xF2, 0xF5, 0xFA};
constexpr Color kPurple{0xA8, 0x8B, 0xFA};

class Canvas {
public:
    Canvas() : pixels_(static_cast<std::size_t>(kWidth) * kHeight, kBackground) {}

    void setPixel(int x, int y, Color color) {
        if (x < 0 || x >= kWidth || y < 0 || y >= kHeight) {
            return;
        }
        pixels_[static_cast<std::size_t>(y) * kWidth +
                static_cast<std::size_t>(x)] = color;
    }

    void savePpm(const std::filesystem::path& path) const {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Could not open output file: " + path.string());
        }
        file << "P6\n" << kWidth << ' ' << kHeight << "\n255\n";
        file.write(reinterpret_cast<const char*>(pixels_.data()),
                   static_cast<std::streamsize>(pixels_.size() * sizeof(Color)));
        if (!file) {
            throw std::runtime_error("Could not write output file: " + path.string());
        }
    }

private:
    std::vector<Color> pixels_;
};

void drawLine(Canvas& canvas, int x0, int y0, int x1, int y1, Color color) {
    const int dx = (x1 >= x0) ? x1 - x0 : x0 - x1;
    const int dy = (y1 >= y0) ? y1 - y0 : y0 - y1;
    const int sx = (x0 < x1) ? 1 : -1;
    const int sy = (y0 < y1) ? 1 : -1;
    int error = dx - dy;
    while (true) {
        canvas.setPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const int twiceError = 2 * error;
        if (twiceError > -dy) {
            error -= dy;
            x0 += sx;
        }
        if (twiceError < dx) {
            error += dx;
            y0 += sy;
        }
    }
}

void drawRectangle(Canvas& canvas, int left, int top, int right, int bottom,
                   Color color) {
    drawLine(canvas, left, top, right, top, color);
    drawLine(canvas, right, top, right, bottom, color);
    drawLine(canvas, right, bottom, left, bottom, color);
    drawLine(canvas, left, bottom, left, top, color);
}

// Reflect one octant into all eight circle octants.
void plotCircle8(Canvas& canvas, int xc, int yc, int x, int y, Color color) {
    canvas.setPixel(xc + x, yc + y, color);
    canvas.setPixel(xc - x, yc + y, color);
    canvas.setPixel(xc + x, yc - y, color);
    canvas.setPixel(xc - x, yc - y, color);
    canvas.setPixel(xc + y, yc + x, color);
    canvas.setPixel(xc - y, yc + x, color);
    canvas.setPixel(xc + y, yc - x, color);
    canvas.setPixel(xc - y, yc - x, color);
}

void drawCircle(Canvas& canvas, int xc, int yc, int radius, Color color) {
    if (radius < 0) {
        throw std::invalid_argument("Circle radius must be nonnegative");
    }
    int x = 0;
    int y = radius;
    std::int64_t decision = 1LL - radius; // p0 = 1 - r
    while (x <= y) {
        plotCircle8(canvas, xc, yc, x, y, color);
        ++x;
        if (decision < 0) {
            decision += 2LL * x + 1;
        } else {
            --y;
            decision += 2LL * (x - y) + 1;
        }
    }
}

// A full step writes four reflections; upperHalf keeps the top two for arcs.
void plotEllipse4(Canvas& canvas, int xc, int yc, int x, int y,
                  Color color, bool upperHalf) {
    canvas.setPixel(xc + x, yc - y, color);
    canvas.setPixel(xc - x, yc - y, color);
    if (!upperHalf) {
        canvas.setPixel(xc + x, yc + y, color);
        canvas.setPixel(xc - x, yc + y, color);
    }
}

void drawEllipse(Canvas& canvas, int xc, int yc, int rx, int ry,
                 Color color, bool upperHalf = false) {
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
    std::int64_t dx = 0;            // 2 * ry^2 * x
    std::int64_t dy = 2 * rx2 * y;  // 2 * rx^2 * y

    // Four times the Region 1 midpoint value; no floating-point arithmetic.
    std::int64_t decision = 4 * ry2 - 4 * rx2 * y + rx2;
    while (dx < dy) { // Region 1: |dy/dx| < 1
        plotEllipse4(canvas, xc, yc, static_cast<int>(x),
                     static_cast<int>(y), color, upperHalf);
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

    // Four times F(x + 1/2, y - 1) at the region transition.
    const std::int64_t twiceXPlusOne = 2 * x + 1;
    const std::int64_t yMinusOne = y - 1;
    decision = ry2 * twiceXPlusOne * twiceXPlusOne +
               4 * rx2 * yMinusOne * yMinusOne - 4 * rx2 * ry2;
    while (y >= 0) { // Region 2: |dy/dx| >= 1
        plotEllipse4(canvas, xc, yc, static_cast<int>(x),
                     static_cast<int>(y), color, upperHalf);
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

void drawBackdrop(Canvas& canvas) {
    for (int x = 480; x < kWidth; x += 64) {
        drawLine(canvas, x, 0, x, kHeight - 1, kGrid);
    }
    for (int y = 64; y < kHeight; y += 64) {
        drawLine(canvas, 464, y, kWidth - 1, y, kGrid);
    }
    drawRectangle(canvas, 18, 18, 446, 701, kFrame);
    drawLine(canvas, 464, 18, 464, 701, kCyan);
    drawRectangle(canvas, 482, 18, 1261, 701, kFrame);
    drawLine(canvas, 40, 64, 424, 64, kCyan);
    drawLine(canvas, 504, 64, 1239, 64, kRed);
    drawLine(canvas, 40, 674, 424, 674, kFrame);
    drawLine(canvas, 504, 674, 1239, 674, kFrame);
}

void drawTestBench(Canvas& canvas) {
    drawCircle(canvas, 226, 176, 80, kCyan);
    drawCircle(canvas, 226, 176, 50, kYellow);
    drawCircle(canvas, 226, 176, 20, kGreen);
    drawLine(canvas, 138, 176, 314, 176, kFrame);
    drawLine(canvas, 226, 88, 226, 264, kFrame);

    drawRectangle(canvas, 42, 287, 408, 342, kFrame);
    drawCircle(canvas, 72, 314, 1, kWhite);
    drawCircle(canvas, 113, 314, 3, kYellow);
    drawEllipse(canvas, 175, 314, 5, 2, kGreen);

    drawEllipse(canvas, 225, 402, 160, 22, kPurple);
    drawLine(canvas, 65, 402, 385, 402, kFrame);
    drawEllipse(canvas, 225, 554, 28, 140, kCyan);
    drawLine(canvas, 225, 414, 225, 694, kFrame);
}

void drawMotorsportScene(Canvas& canvas) {
    // Tyre, compound stripe, wheel barrel, rim, hub, and central lock nut.
    drawCircle(canvas, 754, 409, 205, kTyre);
    drawCircle(canvas, 754, 409, 198, kFrame);
    drawCircle(canvas, 754, 409, 188, kRed);
    drawCircle(canvas, 754, 409, 184, kRed);
    drawCircle(canvas, 754, 409, 169, kTyre);
    drawCircle(canvas, 754, 409, 146, kRim);
    drawCircle(canvas, 754, 409, 134, kWhite);
    drawCircle(canvas, 754, 409, 72, kRim);
    drawCircle(canvas, 754, 409, 25, kYellow);
    drawCircle(canvas, 754, 409, 15, kWhite);

    drawLine(canvas, 754, 276, 754, 337, kRim);
    drawLine(canvas, 754, 481, 754, 542, kRim);
    drawLine(canvas, 621, 409, 682, 409, kRim);
    drawLine(canvas, 826, 409, 887, 409, kRim);
    drawLine(canvas, 660, 315, 703, 358, kRim);
    drawLine(canvas, 805, 460, 848, 503, kRim);
    drawLine(canvas, 660, 503, 703, 460, kRim);
    drawLine(canvas, 805, 358, 848, 315, kRim);

    // Helmet shell and visor arcs are upper halves of midpoint ellipses.
    drawEllipse(canvas, 1111, 276, 135, 83, kWhite, true);
    drawEllipse(canvas, 1111, 282, 123, 52, kCyan, true);
    drawEllipse(canvas, 1111, 291, 115, 35, kPurple, true);
    drawLine(canvas, 996, 291, 1226, 291, kRim);

    // Steering rim and telemetry display pod use complete ellipses.
    drawEllipse(canvas, 1111, 489, 139, 101, kWhite);
    drawEllipse(canvas, 1111, 489, 127, 89, kRim);
    drawEllipse(canvas, 1111, 489, 88, 49, kCyan);
    drawEllipse(canvas, 1111, 489, 76, 38, kFrame);
    drawLine(canvas, 972, 489, 1023, 489, kRed);
    drawLine(canvas, 1199, 489, 1250, 489, kRed);
    drawLine(canvas, 1070, 489, 1093, 489, kGreen);
    drawLine(canvas, 1101, 489, 1124, 489, kYellow);
    drawLine(canvas, 1132, 489, 1153, 489, kRed);
    drawCircle(canvas, 1010, 489, 9, kYellow);
    drawCircle(canvas, 1212, 489, 9, kGreen);
    drawLine(canvas, 973, 646, 1241, 646, kRed);
    drawLine(canvas, 1010, 631, 1204, 631, kFrame);
}

} // namespace

int main() {
    try {
        Canvas canvas;
        drawBackdrop(canvas);
        drawTestBench(canvas);
        drawMotorsportScene(canvas);
        canvas.savePpm(kOutputPath);

        std::cout << "Apex3D Checkpoint 1: midpoint rasterization\n"
                  << "  Canvas:  " << kWidth << " x " << kHeight << " RGB24\n"
                  << "  Output:  " << kOutputPath << '\n'
                  << "  Shapes:  concentric circles (r=20, 50, 80); "
                     "micro circles (r=1, 3);\n"
                  << "           micro ellipse (rx=5, ry=2); "
                     "wide (160, 22); tall (28, 140);\n"
                  << "           F1 tyre and wheel circles; steering, "
                     "telemetry, and visor ellipses\n"
                  << "  Status:  success\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Apex3D Checkpoint 1 failed: " << error.what() << '\n';
        return 1;
    }
}
