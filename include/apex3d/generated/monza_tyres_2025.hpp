#pragma once
#include <array>
#include <string_view>
namespace apex3d::cp3 {
struct TyreStint {int driver,number,firstLap,lastLap;std::string_view compound;int priorLaps;};
inline constexpr std::array<TyreStint,6> tyreStints{{
    {1, 1, 1, 37, "MEDIUM", 0},
    {1, 2, 38, 53, "HARD", 0},
    {4, 1, 1, 46, "MEDIUM", 0},
    {4, 2, 47, 53, "SOFT", 2},
    {81, 1, 1, 45, "MEDIUM", 0},
    {81, 2, 46, 53, "SOFT", 3},
}};
}
