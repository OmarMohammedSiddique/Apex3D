#pragma once
#include "driver_focus.hpp"
#include "generated/monza_tyres_2025.hpp"

namespace apex3d::cp3 {
struct TyreState {
    const TyreStint* stint;
    int raceLaps,priorLaps,completedLaps;
    bool changing;
    bool available{true};
};
inline TyreState tyreState(DriverFocus driver,double seconds) {
    const auto& info=driverInfo(driver);
    static constexpr TyreStint missing{0,0,0,0,"UNAVAILABLE",0};
    if(info.archive->dns)return {&missing,0,0,0,false,false};
    int set=1;bool changing=false;
    for(const auto& pit:archivedPits)if(pit.driver==static_cast<int>(info.focus)){
        if(seconds>=pit.boxStart+pit.service)++set;
        if(seconds>=pit.boxStart&&seconds<pit.boxStart+pit.service)changing=true;
    }
    const TyreStint* stint=&missing;
    for(const auto& candidate:tyreStints)
        if(candidate.driver==static_cast<int>(info.focus)&&candidate.number==set)stint=&candidate;
    const int completed=driverClock(driver,seconds).completed;
    const int onSet=std::max(0,completed-(stint->firstLap-1));
    return {stint,onSet,stint->priorLaps,onSet+stint->priorLaps,
        changing,stint!=&missing};
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
