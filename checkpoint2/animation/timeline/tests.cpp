// Apex3D – Checkpoint 2: Animation Timeline Tests
//
// Deterministic unit tests covering:
//   1. Frame zero  – progress == 0 and elapsed == 0
//   2. Halfway position
//   3. Final frame – progress == 1 and elapsed == duration
//   4. Progress clamping (beyond end)
//   5. Loop wraparound
//   6. Angle interpolation across 360 degrees
//   7. Invalid FPS handling
//   8. Invalid duration handling
//
// Build:
//   g++ -std=c++20 -Wall -Wextra -Wpedantic tests.cpp -o timeline_tests
//
// Run:
//   ./timeline_tests

#include "timeline.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {

constexpr double kEps = 1e-9;

bool near(double a, double b) { return std::abs(a - b) <= kEps; }
bool near(apex3d::timeline::Vec2 a, apex3d::timeline::Vec2 b)
{
    return near(a.x, b.x) && near(a.y, b.y);
}

class TestRunner {
public:
    void check(std::string_view name, bool passed)
    {
        std::cout << "  [" << (passed ? "PASS" : "FAIL") << "] " << name << '\n';
        if (!passed) failed_ = true;
    }
    [[nodiscard]] bool allPassed() const { return !failed_; }
private:
    bool failed_{};
};

} // namespace

int main()
{
    using namespace apex3d::timeline;
    TestRunner r;

    std::cout << "Apex3D Animation Timeline Tests\n";

    // ── 1. Frame zero ─────────────────────────────────────────────────────
    {
        Timeline tl(10.0, 2.0);
        r.check("frame_zero: progress == 0",
                near(tl.progressAtFrame(0), 0.0));
        r.check("frame_zero: elapsed == 0",
                near(tl.elapsedAtFrame(0), 0.0));
    }

    // ── 2. Halfway position ───────────────────────────────────────────────
    {
        // At t=0.5, lerp between (0,0) and (100,200) gives (50,100).
        const Vec2 a{0.0, 0.0}, b{100.0, 200.0};
        const Vec2 mid = lerp(a, b, 0.5);
        r.check("lerp_halfway: x==50", near(mid.x, 50.0));
        r.check("lerp_halfway: y==100", near(mid.y, 100.0));
        // Also verify via Vec2 overload
        r.check("lerp_halfway: vec2 near",
                near(mid, Vec2{50.0, 100.0}));
    }

    // ── 3. Final frame ────────────────────────────────────────────────────
    {
        Timeline tl(10.0, 2.0);
        const int last = tl.totalFrames() - 1;
        r.check("final_frame: progress == 1",
                near(tl.progressAtFrame(last), 1.0));
        r.check("final_frame: elapsed == duration",
                near(tl.elapsedAtFrame(last), tl.duration()));
    }

    // ── 4. Progress clamping ──────────────────────────────────────────────
    {
        // Clamp mode: progress never exceeds 1.
        Timeline tl(10.0, 1.0, PlaybackMode::Clamp);
        r.check("clamp_beyond_end",
                near(tl.progressAt(5.0), 1.0));
        r.check("clamp_before_start",
                near(tl.progressAt(-1.0), 0.0));
    }

    // ── 5. Loop wraparound ────────────────────────────────────────────────
    {
        // Loop mode: elapsed == 1 * duration should return to 0.
        Timeline tl(10.0, 1.0, PlaybackMode::Loop);
        r.check("loop_full_cycle_returns_zero",
                near(tl.progressAt(1.0), 0.0));
        r.check("loop_half_cycle_at_t1.5",
                near(tl.progressAt(1.5), 0.5));
        r.check("loop_at_2_durations",
                near(tl.progressAt(2.0), 0.0));
    }

    // ── 6. Angle interpolation across 360 degrees ─────────────────────────
    {
        // Shortest arc from 350° to 10° should go +20° not -340°.
        const double mid350_10 = lerpAngle(350.0, 10.0, 0.5);
        r.check("angle_wrap_350_to_10_at_0.5 == 0",
                near(mid350_10, 0.0));

        // From 0° to 180°, halfway should be 90°.
        r.check("angle_0_to_180_at_0.5 == 90",
                near(lerpAngle(0.0, 180.0, 0.5), 90.0));

        // From 90° to 270° shortest arc is +180°, halfway = 180°.
        r.check("angle_90_to_270_at_0.5 == 180",
                near(lerpAngle(90.0, 270.0, 0.5), 180.0));

        // At t=0 result should equal start.
        r.check("angle_at_t0_equals_start",
                near(lerpAngle(45.0, 200.0, 0.0), 45.0));

        // At t=1 result should equal destination.
        r.check("angle_at_t1_equals_end",
                near(lerpAngle(45.0, 200.0, 1.0), 200.0));

        // Going across 0: 270° to 90°, shortest = +180°, halfway = 0°.
        r.check("angle_270_to_90_at_0.5 == 0",
                near(lerpAngle(270.0, 90.0, 0.5), 0.0));
    }

    // ── 7. Invalid FPS ────────────────────────────────────────────────────
    {
        bool caught = false;
        try { Timeline tl(0.0, 1.0); }
        catch (const std::invalid_argument&) { caught = true; }
        r.check("invalid_fps_zero_throws", caught);

        caught = false;
        try { Timeline tl(-5.0, 1.0); }
        catch (const std::invalid_argument&) { caught = true; }
        r.check("invalid_fps_negative_throws", caught);
    }

    // ── 8. Invalid duration ───────────────────────────────────────────────
    {
        bool caught = false;
        try { Timeline tl(30.0, 0.0); }
        catch (const std::invalid_argument&) { caught = true; }
        r.check("invalid_duration_zero_throws", caught);

        caught = false;
        try { Timeline tl(30.0, -1.0); }
        catch (const std::invalid_argument&) { caught = true; }
        r.check("invalid_duration_negative_throws", caught);
    }

    // ── Extra: totalFrames count ──────────────────────────────────────────
    {
        // 6 fps × 2 s = 12 intervals → 13 frames
        Timeline tl(6.0, 2.0);
        r.check("total_frames_6fps_2s == 13",
                tl.totalFrames() == 13);
    }

    if (!r.allPassed()) {
        std::cerr << "\nSome tests FAILED.\n";
        return EXIT_FAILURE;
    }
    std::cout << "\nAll tests PASSED.\n";
    return EXIT_SUCCESS;
}
