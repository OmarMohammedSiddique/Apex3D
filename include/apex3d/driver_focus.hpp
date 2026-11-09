#pragma once
#include "race_field.hpp"
#include "race_telemetry.hpp"
#include "generated/monza_entries_2025.hpp"

namespace apex3d::cp3 {
enum class DriverFocus {
    Verstappen=1,Norris=4,Bortoleto=5,Hadjar=6,Gasly=10,Antonelli=12,Alonso=14,
    Leclerc=16,Stroll=18,Tsunoda=22,Albon=23,Hulkenberg=27,Lawson=30,Ocon=31,
    Colapinto=43,Hamilton=44,Sainz=55,Russell=63,Piastri=81,Bearman=87
};
struct DriverInfo {
    DriverFocus focus;const char* name;const char* tag;Color color;
    const std::array<RaceLap,53>* laps;
    double finish,pitEntry,pitExit,boxStart,service;int pitLap;
    int totalLaps;const ArchivedEntry* archive;
};
inline constexpr auto raceDrivers=[] {
    std::array<DriverInfo,20> result{};
    for(std::size_t i=0;i<archivedEntries.size();++i) {
        const auto& entry=archivedEntries[i];
        result[i]={static_cast<DriverFocus>(entry.number),entry.name,entry.tag,
            entry.number==1?Color{0.22F,0.86F,0.91F}:entry.color,&entry.laps,
            entry.elapsed,-1,-1,-1,0,0,entry.totalLaps,&entry};
        for(const auto& pit:archivedPits)if(pit.driver==entry.number){
            result[i].pitEntry=pit.entry;result[i].pitExit=pit.exit;result[i].boxStart=pit.boxStart;
            result[i].service=pit.service;result[i].pitLap=pit.lap;break;
        }
    }
    return result;
}();
// Kept as a small adapter for the original podium regression checks.
inline constexpr std::array<DriverInfo,3> podiumDrivers{raceDrivers[0],raceDrivers[1],raceDrivers[2]};
inline const DriverInfo& driverInfo(DriverFocus focus) {
    for(const auto& info:raceDrivers)if(info.focus==focus)return info;
    return raceDrivers.front();
}
inline bool driverActive(DriverFocus focus,double seconds) {
    const auto& entry=*driverInfo(focus).archive;
    return !entry.dns&&(!entry.dnf||seconds<entry.retire);
}
inline int driverIndex(DriverFocus focus) {
    for(std::size_t i=0;i<raceDrivers.size();++i)if(raceDrivers[i].focus==focus)return static_cast<int>(i);
    return 0;
}
inline RaceClock driverClock(DriverFocus focus,double seconds) {
    const auto& info=driverInfo(focus);
    if(info.totalLaps==0)return {0,0,0,0,false};
    if(!std::isfinite(seconds))seconds=0;
    const double ms=std::clamp(seconds,0.0,info.archive->dnf?info.archive->retire:info.finish)*1000;
    int completed=0,start=0;
    for(int i=0;i<info.totalLaps;++i){const auto& lap=(*info.laps)[i];
        if(ms+1e-6<start+lap.milliseconds())break;
        start+=lap.milliseconds();++completed;}
    if(completed==info.totalLaps){const double last=(*info.laps)[info.totalLaps-1].milliseconds()/1000.0;
        return {info.archive->dnf?info.totalLaps+1:info.totalLaps,completed,
            info.archive->dnf?(ms-start)/1000:last,last,!info.archive->dnf};}
    return {completed+1,completed,(ms-start)/1000,
        completed?(*info.laps)[completed-1].milliseconds()/1000.0:0,false};
}
inline double nextDriverLap(DriverFocus focus,double seconds) {
    const auto& info=driverInfo(focus);const auto clock=driverClock(focus,seconds);
    if(clock.completed>=info.totalLaps)return seconds;
    int milliseconds=0;
    for(int i=0;i<std::min(clock.lap,info.totalLaps);++i)milliseconds+=(*info.laps)[i].milliseconds();
    return milliseconds/1000.0;
}
inline RaceSample focusedSample(DriverFocus focus,const RaceSample& ver,
                               const OpponentSample& nor,const OpponentSample& pia) {
    if(focus==DriverFocus::Verstappen)return ver;
    const auto& s=focus==DriverFocus::Norris?nor:pia;
    return {s.pose,s.progress,s.velocity,s.inPit,s.servicing,s.travelledMetres};
}
} // namespace apex3d::cp3
