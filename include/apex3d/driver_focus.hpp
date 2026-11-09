#pragma once
#include "race_field.hpp"
#include "race_telemetry.hpp"

namespace apex3d::cp3 {
enum class DriverFocus { Verstappen=1, Norris=4, Piastri=81 };
struct DriverInfo {
    DriverFocus focus;const char* name;const char* tag;Color color;
    const std::array<RaceLap,53>* laps;
    double finish,pitEntry,pitExit,boxStart,service;int pitLap;
};
inline constexpr std::array<DriverInfo,3> podiumDrivers{{
    {DriverFocus::Verstappen,"MAX VERSTAPPEN","VER 1",{0.22F,0.86F,0.91F},&raceLaps,
        raceTotalSeconds,pitEntrySeconds,pitExitSeconds,pitBoxStartSeconds,pitServiceSeconds,37},
    {DriverFocus::Norris,"LANDO NORRIS","NOR 4",{1.0F,0.52F,0.10F},&norrisLaps,
        norrisSpec.finish,norrisSpec.entry,norrisSpec.exit,norrisSpec.boxStart,norrisSpec.service,46},
    {DriverFocus::Piastri,"OSCAR PIASTRI","PIA 81",{1.0F,0.72F,0.35F},&piastriLaps,
        piastriSpec.finish,piastriSpec.entry,piastriSpec.exit,piastriSpec.boxStart,piastriSpec.service,45}
}};
inline const DriverInfo& driverInfo(DriverFocus focus) {
    for(const auto& info:podiumDrivers)if(info.focus==focus)return info;
    return podiumDrivers.front();
}
inline RaceClock driverClock(DriverFocus focus,double seconds) {
    const auto& info=driverInfo(focus);
    const double ms=std::clamp(seconds,0.0,info.finish)*1000;
    int completed=0,start=0;
    for(const auto& lap:*info.laps){if(ms+1e-6<start+lap.milliseconds())break;
        start+=lap.milliseconds();++completed;}
    if(completed==53){const double last=info.laps->back().milliseconds()/1000.0;
        return {53,53,last,last,true};}
    return {completed+1,completed,(ms-start)/1000,
        completed?(*info.laps)[completed-1].milliseconds()/1000.0:0,false};
}
inline double nextDriverLap(DriverFocus focus,double seconds) {
    const auto clock=driverClock(focus,seconds);int milliseconds=0;
    for(int i=0;i<clock.lap;++i)milliseconds+=(*driverInfo(focus).laps)[i].milliseconds();
    return milliseconds/1000.0;
}
inline RaceSample focusedSample(DriverFocus focus,const RaceSample& ver,
                               const OpponentSample& nor,const OpponentSample& pia) {
    if(focus==DriverFocus::Verstappen)return ver;
    const auto& s=focus==DriverFocus::Norris?nor:pia;
    return {s.pose,s.progress,s.velocity,s.inPit,s.servicing,s.travelledMetres};
}
} // namespace apex3d::cp3
