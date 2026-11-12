#pragma once
// Live timing tower: the running order and gap to the leader at the current race time.
//
// Order comes from the recorded OpenF1 position feed (the same source as the "VER P1"
// label), so the tower and the telemetry heading always agree. Position numbers are the
// feed's own, so they skip the non-starter's grid slot until the feed drops it, 16 s in. Gaps come from the
// recorded sector times, the way official timing measures them: every car's gap is
// taken at the last timing line it crossed (the end of sector 1, 2 or 3), against
// the first car to cross that line on that lap. A car the leader has already passed
// that line again before it arrives shows "+1 LAP". Gaps therefore update three
// times a lap, not continuously. Pit-lane times are the estimated timing lines.
// After the race the panel returns to the official classification, including
// penalties.
#include "race_identity.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace apex3d::cp3 {
inline constexpr int timingPointsPerLap=3,timingPointsMax=53*timingPointsPerLap;
// Race-clock milliseconds at which each driver crossed each timing line, in lap order.
struct TimingLines { std::array<std::array<int,timingPointsMax>,20> crossed{}; std::array<int,20> count{}; };
inline const TimingLines& timingLines(){
    static const TimingLines lines=[]{
        TimingLines result;
        for(std::size_t d=0;d<raceDrivers.size();++d){
            const auto& info=raceDrivers[d];int clock=0,n=0;
            for(int lap=0;lap<info.totalLaps;++lap){const auto& times=(*info.laps)[static_cast<std::size_t>(lap)];
                for(int sector:{times.s1,times.s2,times.s3}){clock+=sector;result.crossed[d][static_cast<std::size_t>(n++)]=clock;}}
            result.count[d]=n;
        }
        return result;
    }();
    return lines;
}
// Number of timing lines a driver has crossed by the given race time.
inline int timingPointsCrossed(std::size_t driver,double milliseconds){
    const auto& lines=timingLines();const auto& times=lines.crossed[driver];
    return static_cast<int>(std::upper_bound(times.begin(),times.begin()+lines.count[driver],milliseconds+1e-6)-times.begin());
}
struct TimingGap { bool timed{false}; int lapsDown{0}; double seconds{0}; };
inline TimingGap timingGap(std::size_t driver,double seconds){
    const auto& lines=timingLines();
    const double ms=seconds*1000;
    const int n=timingPointsCrossed(driver,ms);
    if(n==0)return {};
    const int at=lines.crossed[driver][static_cast<std::size_t>(n-1)];
    int first=at,leaderPoints=n;
    for(std::size_t other=0;other<raceDrivers.size();++other){
        if(lines.count[other]>=n)first=std::min(first,lines.crossed[other][static_cast<std::size_t>(n-1)]);
        leaderPoints=std::max(leaderPoints,timingPointsCrossed(other,at));
    }
    return {true,(leaderPoints-n)/timingPointsPerLap,(at-first)/1000.0};
}
inline bool liveTiming(double seconds){return seconds<finalClassificationSeconds;}
struct TowerRow { DriverFocus focus; std::string position; std::string gap; bool pit{false}; };
inline std::string signedSeconds(double seconds){
    std::ostringstream text;text<<std::fixed<<std::setprecision(3)<<"+"<<seconds<<"S";return text.str();
}
inline std::array<TowerRow,20> classificationRows(){
    std::array<TowerRow,20> rows{};
    for(std::size_t i=0;i<raceDrivers.size();++i){
        const auto& info=raceDrivers[i];const auto& entry=*info.archive;
        rows[i]={info.focus,entry.dnf||entry.dns?std::string("NC"):std::to_string(i+1),
            entry.dns?"DNS":entry.dnf?"DNF":entry.totalLaps<53?"+1 LAP":i==0?"WINNER":signedSeconds(entry.classified-raceTotalSeconds)};
    }
    return rows;
}
// Display order for the side panel at a race time: the live running order during the
// race, the official classification afterwards (and for an infinite time).
inline std::array<TowerRow,20> timingTower(double seconds){
    if(!std::isfinite(seconds)||!liveTiming(seconds))return classificationRows();
    seconds=std::max(seconds,0.0);
    std::array<std::size_t,20> order{};
    for(std::size_t i=0;i<order.size();++i)order[i]=i;
    // Running cars by recorded position, then retirements (latest first), then non-starters.
    const auto rank=[&](std::size_t i){
        const auto& info=raceDrivers[i];const auto& entry=*info.archive;
        if(entry.dns)return 1e9;
        if(!driverActive(info.focus,seconds))return 1e6-entry.retire;
        return double(currentPosition(info.focus,seconds));
    };
    std::stable_sort(order.begin(),order.end(),[&](std::size_t a,std::size_t b){return rank(a)<rank(b);});
    std::array<TowerRow,20> rows{};
    for(std::size_t row=0;row<order.size();++row){
        const std::size_t d=order[row];const auto& info=raceDrivers[d];const auto& entry=*info.archive;
        // Recorded positions, not row numbers: for the first 16 s the feed still holds the
        // non-starter's grid slot, so P12 is briefly skipped, exactly as in the label.
        TowerRow result{info.focus,std::to_string(currentPosition(info.focus,seconds)),"",false};
        if(entry.dns){result.position="NC";result.gap="DNS";}
        else if(!driverActive(info.focus,seconds)){result.position="NC";result.gap="OUT";}
        else if(info.pitEntry>=0&&seconds>=info.pitEntry&&seconds<info.pitExit){result.gap="PIT";result.pit=true;}
        else if(row==0)result.gap=seconds>0?"LEADER":"";
        else if(const auto gap=timingGap(d,seconds);gap.timed)
            result.gap=gap.lapsDown>0?"+"+std::to_string(gap.lapsDown)+(gap.lapsDown==1?" LAP":" LAPS"):signedSeconds(gap.seconds);
        rows[row]=result;
    }
    return rows;
}
inline int towerRow(DriverFocus focus,double seconds){
    const auto rows=timingTower(seconds);
    for(std::size_t i=0;i<rows.size();++i)if(rows[i].focus==focus)return static_cast<int>(i);
    return 0;
}
} // namespace apex3d::cp3
