#include "include/transform2d.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string_view>
#include <vector>

namespace {

constexpr double kTolerance = 1e-9;

[[nodiscard]] bool nearlyEqual(double a, double b)
{
    return std::abs(a - b) <= kTolerance;
}

[[nodiscard]] bool nearlyEqual(apex3d::Vec2 a, apex3d::Vec2 b)
{
    return nearlyEqual(a.x, b.x) && nearlyEqual(a.y, b.y);
}

class TestRunner {
public:
    void check(std::string_view name, bool passed)
    {
        std::cout << "  [test] " << name << ": " << (passed ? "PASS" : "FAIL") << '\n';
        if (!passed) {
            failed_ = true;
        }
    }

    [[nodiscard]] bool passed() const { return !failed_; }

private:
    bool failed_{};
};

} // namespace

int main()
{
    using apex3d::Mat3;
    using apex3d::Vec2;

    std::cout << "Apex3D 2D transformation tests:\n";
    TestRunner tests;

    const Vec2 point{3.0, -2.0};
    tests.check("identity", nearlyEqual(Mat3::identity().transformPoint(point), point));
    tests.check("translation",
                nearlyEqual(Mat3::translation(5.0, 4.0).transformPoint(point), {8.0, 2.0}));
    tests.check("scaling",
                nearlyEqual(Mat3::scaling(2.0, -3.0).transformPoint(point), {6.0, 6.0}));
    tests.check("rotation 90 degrees",
                nearlyEqual(Mat3::rotationDegrees(90.0).transformPoint({2.0, 0.0}), {0.0, 2.0}));

    // Column-vector convention: the rightmost operation happens first.
    const Mat3 combined = Mat3::translation(10.0, 20.0) *
                          Mat3::rotationDegrees(90.0) *
                          Mat3::scaling(2.0, 2.0);
    tests.check("combined transform",
                nearlyEqual(combined.transformPoint({1.0, 0.0}), {10.0, 22.0}));

    const Mat3 pivotRotation = apex3d::aroundPivot(Mat3::rotationDegrees(90.0), {5.0, 5.0});
    tests.check("rotation around pivot",
                nearlyEqual(pivotRotation.transformPoint({7.0, 5.0}), {5.0, 7.0}));

    const std::vector<Vec2> triangle{{0.0, 0.0}, {2.0, 0.0}, {0.0, 2.0}};
    const auto moved = apex3d::transformPolygon(triangle, Mat3::translation(4.0, 6.0));
    tests.check("polygon vertex count", moved.size() == triangle.size());
    tests.check("polygon transformation",
                nearlyEqual(moved[0], {4.0, 6.0}) &&
                nearlyEqual(moved[1], {6.0, 6.0}) &&
                nearlyEqual(moved[2], {4.0, 8.0}));

    if (!tests.passed()) {
        std::cerr << "Transformation tests failed.\n";
        return EXIT_FAILURE;
    }
    std::cout << "  All transformation tests passed.\n";
    return EXIT_SUCCESS;
}
