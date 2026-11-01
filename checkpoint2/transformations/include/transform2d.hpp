#pragma once

#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace apex3d {

struct Vec2 {
    double x{};
    double y{};
};

class Mat3 {
public:
    constexpr Mat3()
        : values_{1.0, 0.0, 0.0,
                  0.0, 1.0, 0.0,
                  0.0, 0.0, 1.0}
    {
    }

    explicit constexpr Mat3(std::array<double, 9> values)
        : values_(values)
    {
    }

    [[nodiscard]] static constexpr Mat3 identity() { return Mat3{}; }

    [[nodiscard]] static constexpr Mat3 translation(double tx, double ty)
    {
        return Mat3{{1.0, 0.0, tx,
                     0.0, 1.0, ty,
                     0.0, 0.0, 1.0}};
    }

    [[nodiscard]] static constexpr Mat3 scaling(double sx, double sy)
    {
        return Mat3{{sx,  0.0, 0.0,
                     0.0, sy,  0.0,
                     0.0, 0.0, 1.0}};
    }

    [[nodiscard]] static Mat3 rotationDegrees(double degrees)
    {
        const double radians = degrees * std::numbers::pi / 180.0;
        const double cosine = std::cos(radians);
        const double sine = std::sin(radians);
        return Mat3{{cosine, -sine, 0.0,
                     sine,   cosine, 0.0,
                     0.0,    0.0,    1.0}};
    }

    [[nodiscard]] constexpr double at(int row, int column) const
    {
        return values_[static_cast<std::size_t>(row * 3 + column)];
    }

    [[nodiscard]] constexpr Mat3 operator*(const Mat3& other) const
    {
        std::array<double, 9> result{};
        for (int row = 0; row < 3; ++row) {
            for (int column = 0; column < 3; ++column) {
                for (int index = 0; index < 3; ++index) {
                    result[static_cast<std::size_t>(row * 3 + column)] +=
                        at(row, index) * other.at(index, column);
                }
            }
        }
        return Mat3{result};
    }

    [[nodiscard]] constexpr Vec2 transformPoint(Vec2 point) const
    {
        const double x = at(0, 0) * point.x + at(0, 1) * point.y + at(0, 2);
        const double y = at(1, 0) * point.x + at(1, 1) * point.y + at(1, 2);
        const double w = at(2, 0) * point.x + at(2, 1) * point.y + at(2, 2);
        return w == 0.0 ? Vec2{x, y} : Vec2{x / w, y / w};
    }

private:
    std::array<double, 9> values_;
};

// Applies a transformation around a fixed point rather than the origin.
[[nodiscard]] inline Mat3 aroundPivot(const Mat3& transformation, Vec2 pivot)
{
    return Mat3::translation(pivot.x, pivot.y) * transformation *
           Mat3::translation(-pivot.x, -pivot.y);
}

[[nodiscard]] inline std::vector<Vec2> transformPolygon(
    const std::vector<Vec2>& polygon, const Mat3& transformation)
{
    std::vector<Vec2> transformed;
    transformed.reserve(polygon.size());
    for (Vec2 point : polygon) {
        transformed.push_back(transformation.transformPoint(point));
    }
    return transformed;
}

} // namespace apex3d
