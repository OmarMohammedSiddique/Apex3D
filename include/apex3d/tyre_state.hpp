#pragma once
#include "driver_focus.hpp"
#include "generated/monza_tyres_2025.hpp"

namespace apex3d::cp3 {
struct TyreState {
    const TyreStint* stint;
    int raceLaps,priorLaps,completedLaps;
    bool changing;
};
inline TyreState tyreState(DriverFocus driver,double seconds) {
    const auto& info=driverInfo(driver);
    // The timing feed describes whole-lap stints. Switch the actual fitted set
    // at service completion, since the timing line is crossed inside this pit visit.
    const int set=seconds>=info.boxStart+info.service?2:1;
    const TyreStint* stint=nullptr;
    for(const auto& candidate:tyreStints)
        if(candidate.driver==static_cast<int>(info.focus)&&candidate.number==set)stint=&candidate;
    const int completed=driverClock(driver,seconds).completed;
    const int onSet=std::max(0,completed-(stint->firstLap-1));
    return {stint,onSet,stint->priorLaps,onSet+stint->priorLaps,
        seconds>=info.boxStart&&seconds<info.boxStart+info.service};
}
inline Color compoundColor(std::string_view compound) {
    if(compound=="SOFT")return {0.96F,0.17F,0.20F};
    if(compound=="MEDIUM")return {0.98F,0.80F,0.18F};
    if(compound=="INTERMEDIATE")return {0.20F,0.80F,0.32F};
    if(compound=="WET")return {0.22F,0.48F,0.96F};
    return {0.94F,0.95F,0.95F};
}
inline std::string tyreCompoundText(std::string_view compound) {
    if(compound=="SOFT")return "SOFT C5";
    if(compound=="MEDIUM")return "MEDIUM C4";
    if(compound=="HARD")return "HARD C3";
    return std::string(compound);
}
} // namespace apex3d::cp3
