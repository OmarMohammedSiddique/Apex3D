#pragma once
#include "driver_focus.hpp"
#include "generated/monza_events_2025.hpp"
namespace apex3d::cp3 {
// Race time after which positions are the official classification (penalties applied).
inline constexpr double finalClassificationSeconds=4500;
inline int currentPosition(DriverFocus focus,double seconds){
    const auto& entry=*driverInfo(focus).archive;
    if(entry.dns)return 0;
    if(seconds>=finalClassificationSeconds)return entry.dnf?0:entry.rank;
    int result=entry.grid;
    for(const auto& event:recordedPositions)if(event.driver==entry.number&&event.raceMs<=seconds*1000+1e-6)result=event.position;
    return result;
}
inline std::string driverLabel(DriverFocus focus,double seconds){
    const auto& info=driverInfo(focus);std::string abbreviation=info.tag;abbreviation=abbreviation.substr(0,abbreviation.find(' '));
    if(info.archive->dns)return abbreviation+" DNS";
    if(info.archive->dnf&&seconds>=info.archive->retire)return abbreviation+" DNF";
    return abbreviation+" P"+std::to_string(currentPosition(focus,seconds));
}
inline double drsOpening(int driver,double seconds){
    if(!std::isfinite(seconds))return 0;
    for(const auto& span:recordedDrs)if(span.driver==driver&&seconds*1000>=span.startMs&&seconds*1000<span.endMs)
        return std::clamp(std::min((seconds-span.startMs/1000.0)/.20,(span.endMs/1000.0-seconds)/.12),0.0,1.0);
    return 0;
}
inline bool drsActive(int driver,double seconds){
    for(const auto& span:recordedDrs)if(span.driver==driver&&seconds*1000>=span.startMs&&seconds*1000<span.endMs)return true;
    return false;
}
} // namespace apex3d::cp3
