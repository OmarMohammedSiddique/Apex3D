// Apex3D – Checkpoint 2: Animation Timeline and Interpolation
//
// Reusable, header-only animation timeline for Pure C++ 2D animation.
// Provides:
//   - Timeline: frame counter, FPS, elapsed time, total duration, normalised
//     progress [0, 1] with clamped and looping playback.
//   - lerp: linear interpolation between two 2D positions.
//   - lerpAngle: shortest-arc angle interpolation (degrees).
//   - PlaybackMode: enum selecting CLAMP or LOOP behaviour.
//
// No external dependencies. Compatible with C++20.

#pragma once

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>

namespace apex3d::timeline {

// ─────────────────────────────────────────────────────────────────────────────
// Vec2 – a simple 2-D point/vector used throughout this module.
// ─────────────────────────────────────────────────────────────────────────────
struct Vec2 {
    double x{};
    double y{};
};

// ─────────────────────────────────────────────────────────────────────────────
// PlaybackMode – controls what happens when progress exceeds [0, 1].
// ─────────────────────────────────────────────────────────────────────────────
enum class PlaybackMode {
    Clamp,  ///< Progress is clamped to [0, 1].  Animation freezes at end.
    Loop,   ///< Progress wraps modulo 1.  Animation repeats from the start.
};

// ─────────────────────────────────────────────────────────────────────────────
// Timeline – the core animation clock.
// ─────────────────────────────────────────────────────────────────────────────
class Timeline {
public:
    /// @param fps            Frames per second.  Must be > 0.
    /// @param durationSeconds Total animation duration. Must be > 0.
    /// @param mode           Clamped or looping playback.
    Timeline(double fps, double durationSeconds,
             PlaybackMode mode = PlaybackMode::Clamp)
        : fps_(fps),
          duration_(durationSeconds),
          mode_(mode)
    {
        if (fps <= 0.0) {
            throw std::invalid_argument(
                "Timeline: fps must be positive, got " + std::to_string(fps));
        }
        if (durationSeconds <= 0.0) {
            throw std::invalid_argument(
                "Timeline: duration must be positive, got " +
                std::to_string(durationSeconds));
        }
    }

    [[nodiscard]] double fps()      const noexcept { return fps_; }
    [[nodiscard]] double duration() const noexcept { return duration_; }
    [[nodiscard]] PlaybackMode mode() const noexcept { return mode_; }

    /// Total number of discrete frames ( ceil(fps * duration) + 1 ).
    [[nodiscard]] int totalFrames() const noexcept
    {
        return static_cast<int>(std::ceil(fps_ * duration_)) + 1;
    }

    /// Elapsed time in seconds for a given frame index.
    [[nodiscard]] double elapsedAtFrame(int frame) const noexcept
    {
        return static_cast<double>(frame) / fps_;
    }

    /// Normalised progress in [0,1] for a given elapsed time.
    [[nodiscard]] double progressAt(double elapsed) const noexcept
    {
        double raw = elapsed / duration_;
        if (mode_ == PlaybackMode::Clamp) {
            return clamp01(raw);
        }
        double wrapped = raw - std::floor(raw);
        return wrapped < 0.0 ? wrapped + 1.0 : wrapped;
    }

    /// Normalised progress in [0,1] for a given frame index.
    [[nodiscard]] double progressAtFrame(int frame) const noexcept
    {
        return progressAt(elapsedAtFrame(frame));
    }

    [[nodiscard]] static double clamp(double v, double lo, double hi) noexcept
    {
        return v < lo ? lo : (v > hi ? hi : v);
    }
    [[nodiscard]] static double clamp01(double v) noexcept
    {
        return clamp(v, 0.0, 1.0);
    }

private:
    double       fps_;
    double       duration_;
    PlaybackMode mode_;
};

// ─────────────────────────────────────────────────────────────────────────────
// lerp – linear interpolation between two 2D positions.
//   result = a + t * (b - a)
// ─────────────────────────────────────────────────────────────────────────────
[[nodiscard]] inline Vec2 lerp(Vec2 a, Vec2 b, double t) noexcept
{
    return {a.x + t * (b.x - a.x),
            a.y + t * (b.y - a.y)};
}

// ─────────────────────────────────────────────────────────────────────────────
// lerpAngle – shortest-arc interpolation between two angles (degrees).
// Delta is wrapped into (-180, +180] to always take the shorter arc.
// ─────────────────────────────────────────────────────────────────────────────
[[nodiscard]] inline double lerpAngle(double aDeg, double bDeg, double t) noexcept
{
    double delta = bDeg - aDeg;
    delta = std::fmod(delta, 360.0);
    if (delta > 180.0)  { delta -= 360.0; }
    else if (delta <= -180.0) { delta += 360.0; }
    double result = aDeg + t * delta;
    result = std::fmod(result, 360.0);
    if (result < 0.0) { result += 360.0; }
    return result;
}

[[nodiscard]] inline double toRadians(double degrees) noexcept
{
    return degrees * std::numbers::pi / 180.0;
}

[[nodiscard]] inline double toDegrees(double radians) noexcept
{
    return radians * 180.0 / std::numbers::pi;
}

} // namespace apex3d::timeline
