#pragma once
#include "generated/monza_safety.hpp"
#include <string>
#include <algorithm>
namespace apex3d::cp3 {
struct RaceFlag {std::string title,detail;int severity{};};
inline RaceFlag raceFlagAt(double seconds){
    std::array<std::string_view,30> sectors{};const FlagEvent* recent=nullptr;const FlagEvent* clear=nullptr;
    for(const auto& e:flagEvents)if(e.raceMs<=seconds*1000+1e-6){
        if(e.sector){sectors[e.sector]=e.flag=="CLEAR"?std::string_view{}:e.flag;if(e.flag=="CLEAR")clear=&e;}
        else if(e.flag!="GREEN")recent=&e;
    }
    RaceFlag result;std::string ids;
    for(int i=1;i<30;++i)if(!sectors[i].empty()){
        if(!ids.empty())ids+=", ";ids+=std::to_string(i);result.severity=std::max(result.severity,sectors[i]=="DOUBLE YELLOW"?3:2);}
    if(result.severity){result.title=result.severity==3?"DOUBLE YELLOW":"YELLOW FLAG";result.detail="MARSHAL SECTOR "+ids+" / NO OVERTAKING";return result;}
    // Race-control incident messages (severity 7) show for 8 s; safety flags above take priority.
    for(const auto& m:incidentMessages)if(seconds*1000>=m.raceMs&&seconds*1000<m.raceMs+8000)
        return {std::string(m.title),std::string(m.detail),7};
    if(recent&&(recent->flag=="CHEQUERED"||seconds-recent->raceMs/1000.0<=12)){
        return {std::string(recent->flag)+" FLAG",recent->driver?"CAR "+std::to_string(recent->driver)+(recent->flag=="BLACK AND WHITE"?" / TRACK LIMITS":" / LET FASTER CAR PASS"):"RACE FINISH",recent->flag=="BLUE"?4:recent->flag=="CHEQUERED"?6:5};}
    if(clear&&seconds-clear->raceMs/1000.0<=6)return {"TRACK CLEAR","LOCAL YELLOW FLAGS CLEARED",1};
    return {};
}
// Illustrative spin: heading turns through the recorded stop window. The timing
// feed does not record heading, so only the timing and location are factual.
inline double incidentYaw(int driver,double seconds){
    for(const auto& s:incidentSpins)if(s.driver==driver){
        const double t=std::clamp((seconds*1000-s.startMs)/double(s.endMs-s.startMs),0.0,1.0);
        if(t<=0||t>=1)return 0;
        const double eased=1-(1-t)*(1-t)*(1-t); // fast rotation that slows as the car stops
        return eased*s.turns*2*3.14159265358979;
    }
    return 0;
}
} // namespace apex3d::cp3
