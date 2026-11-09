#pragma once

#include <array>

namespace apex3d::cp3 {
// FIA 2025 Italian GP Race Sector Analysis, car 1. See docs/monza-race-research.md.
// Generated from data/monza-2025-verstappen.csv; times are integer milliseconds.
struct RaceLap { int s1; int s2; int s3; constexpr int milliseconds() const { return s1+s2+s3; } };
inline constexpr std::array<RaceLap, 53> raceLaps{{
    {29554, 29088, 28517},
    {28457, 28843, 27559},
    {27212, 28713, 27587},
    {27375, 28455, 27432},
    {27520, 28427, 27641},
    {27434, 28496, 27646},
    {27458, 28306, 27546},
    {27411, 28318, 27410},
    {27292, 28355, 27454},
    {27369, 28370, 27349},
    {27378, 28300, 27388},
    {27338, 28354, 27322},
    {27350, 28463, 27441},
    {27446, 28319, 27353},
    {27386, 28238, 27453},
    {27400, 28202, 27399},
    {27317, 28234, 27339},
    {27301, 28154, 27272},
    {27357, 28190, 27291},
    {27370, 28167, 27320},
    {27413, 28215, 27291},
    {27271, 28138, 27324},
    {27317, 28188, 27277},
    {27335, 28171, 27311},
    {27336, 28216, 27418},
    {27390, 28218, 27329},
    {27447, 28229, 27339},
    {27457, 28265, 27309},
    {27382, 28222, 27207},
    {27447, 28294, 27187},
    {27307, 28210, 27424},
    {27403, 28196, 27291},
    {27385, 28257, 27301},
    {27318, 28250, 27227},
    {27355, 28119, 27177},
    {27404, 28118, 27177},
    {27321, 28233, 31878},
    {47531, 27885, 26933},
    {26948, 27641, 26813},
    {27069, 27706, 26850},
    {27001, 27602, 26755},
    {26898, 27654, 26893},
    {27013, 27520, 26787},
    {26934, 27623, 26732},
    {27033, 27590, 26730},
    {26974, 27565, 26786},
    {27153, 27536, 26647},
    {27088, 27533, 26695},
    {26925, 27674, 26729},
    {26934, 27544, 26819},
    {27306, 27690, 26753},
    {26927, 27512, 26564},
    {26987, 27527, 26586},
}};
inline constexpr int raceTotalMilliseconds = 4404325;
inline constexpr double raceTotalSeconds = raceTotalMilliseconds / 1000.0;
inline constexpr double circuitMetres = 5793.0;
inline constexpr double gridOffset = 309.0 / circuitMetres;
inline constexpr double pitLaneSeconds = 24.545;
inline constexpr double pitServiceSeconds = 2.3;
inline constexpr double startSequenceSeconds = 6.5;

constexpr auto raceBoundariesMs = [] {
    std::array<int, 54> result{};
    for (std::size_t i = 0; i < raceLaps.size(); ++i) result[i+1] = result[i] + raceLaps[i].milliseconds();
    return result;
}();
static_assert(raceBoundariesMs.back() == raceTotalMilliseconds);
} // namespace apex3d::cp3
