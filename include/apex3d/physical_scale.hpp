#pragma once

#include "cp3_scene.hpp"
#include "generated/monza_2025.hpp"

namespace apex3d::physical {
// The CP2 curves retain their teaching coordinates. Only the 3D boundary
// converts them to metres; one canonical scale applies to every object.
inline double metresPerSceneUnit() {
    static const double scale = cp3::circuitMetres / cp3::makePath().back().distance;
    return scale;
}
inline double sceneUnits(double metres) { return metres / metresPerSceneUnit(); }
inline constexpr double trackWidth = 12.0; // published Monza range: 10-12 m
inline constexpr double roadHeight = 0.10;
inline constexpr double kerbWidth = 0.80;
inline constexpr double kerbHeight = 0.16;
inline constexpr double carWidth = 2.0;
inline constexpr double wheelbase = 3.6; // 2025 maximum, representative model
inline constexpr double tyreRadius = 0.360;
inline constexpr double frontTyreWidth = 0.305;
inline constexpr double rearTyreWidth = 0.405;
inline constexpr double rimRadius = 18.0 * 0.0254 / 2.0;
inline constexpr double pitOffset = 18.0; // reconstructed, not a survey
inline constexpr double pitWidth = 4.0;
inline constexpr double pitBayOffset = 3.5;
inline constexpr double garageHeight = 6.0;
inline constexpr double crewHeight = 1.15; // crouched service posture
} // namespace apex3d::physical
