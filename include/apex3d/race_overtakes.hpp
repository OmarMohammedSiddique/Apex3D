#pragma once
// Position-change call-outs from the recorded OpenF1 position feed (session 9912).
//
// The feed also moves cars when someone pits, when the non-starter is dropped to
// the back, when penalties are applied after the flag, and it sometimes flickers
// between two cars for a second or two. A one-place swap is called out only when:
//   - it happens after the start and before either car has finished;
//   - neither car is the non-starter, or within 30 s of its own pit entry or exit;
//   - the car that lost the place was ahead 10 s earlier, and the car that gained it
//     stays ahead for the next 10 s (so flickers are ignored).
// The wording is neutral ("UP TO P3 / AHEAD OF PIA") because the feed does not say
// how the place changed: some are passes, some are places handed back.
#include "race_identity.hpp"
#include "race_flags.hpp"
#include <algorithm>
#include <array>
#include <map>
#include <string>
#include <vector>
namespace apex3d::cp3 {
struct PositionChange { int raceMs,driver,passed,position; };
inline constexpr double positionChangePitMargin=30,positionChangeHoldSeconds=10,positionChangeShowSeconds=6;
// Recorded position of a car at a race time in milliseconds (grid position before its first event).
inline int feedPosition(int driver,double raceMs){
    int result=99;
    for(const auto& e:recordedPositions)if(e.driver==driver&&e.raceMs<=raceMs)result=e.position;
    return result;
}
inline const std::vector<PositionChange>& positionChanges(){
    static const auto changes=[]{
        std::vector<const PositionEvent*> events;
        for(const auto& e:recordedPositions)events.push_back(&e);
        std::stable_sort(events.begin(),events.end(),[](auto a,auto b){return a->raceMs<b->raceMs;});
        const auto entry=[](int driver)->const ArchivedEntry*{for(const auto& e:archivedEntries)if(e.number==driver)return &e;return nullptr;};
        const auto nearPit=[](int driver,double seconds){
            for(const auto& pit:archivedPits)if(pit.driver==driver&&seconds>=pit.entry-positionChangePitMargin&&seconds<=pit.exit+positionChangePitMargin)return true;
            return false;
        };
        std::map<int,int> position;std::vector<PositionChange> result;
        for(std::size_t i=0;i<events.size();){
            const int t=events[i]->raceMs;const auto before=position;
            std::size_t end=i;while(end<events.size()&&events[end]->raceMs==t){position[events[end]->driver]=events[end]->position;++end;}
            for(std::size_t k=i;k<end&&t>0;++k){
                const int d=events[k]->driver,now=events[k]->position;
                const auto was=before.find(d);if(was==before.end()||was->second-now!=1)continue;
                for(const auto& [x,p]:position){
                    const auto xWas=before.find(x);
                    if(x==d||xWas==before.end()||xWas->second!=now||p!=was->second)continue;
                    const auto* a=entry(d);const auto* b=entry(x);
                    if(!a||!b||a->dns||b->dns)continue;
                    if(t>=a->elapsed*1000||t>=b->elapsed*1000)continue;
                    if(nearPit(d,t/1000.0)||nearPit(x,t/1000.0))continue;
                    const double hold=positionChangeHoldSeconds*1000;
                    if(!(feedPosition(x,t-hold)<feedPosition(d,t-hold)))continue;
                    bool held=true;
                    for(double s=t;s<=t+hold&&held;s+=250)held=feedPosition(d,s)<feedPosition(x,s);
                    if(held)result.push_back({t,d,x,now});
                }
            }
            i=end;
        }
        return result;
    }();
    return changes;
}
inline std::string driverCode(int driver){
    for(const auto& e:archivedEntries)if(e.number==driver){std::string tag=e.tag;return tag.substr(0,tag.find(' '));}
    return std::to_string(driver);
}
struct PositionCallout { bool active{false}; std::string title,detail; };
// The latest call-out still on screen at a race time (a newer one replaces an older one).
inline PositionCallout positionCalloutAt(double seconds){
    const PositionChange* latest=nullptr;
    for(const auto& c:positionChanges())if(seconds*1000>=c.raceMs&&seconds*1000<c.raceMs+positionChangeShowSeconds*1000)latest=&c;
    if(!latest)return {};
    const auto lap=driverClock(static_cast<DriverFocus>(latest->driver),latest->raceMs/1000.0).lap;
    return {true,"POSITION CHANGE","LAP "+std::to_string(lap)+" / "+driverCode(latest->driver)+" UP TO P"+std::to_string(latest->position)+" / AHEAD OF "+driverCode(latest->passed)};
}
// The banner over the scene: flags and race-control messages first, then position changes
// (severity 8).
inline RaceFlag raceBannerAt(double seconds){
    if(auto flag=raceFlagAt(seconds);flag.severity)return flag;
    if(const auto change=positionCalloutAt(seconds);change.active)return {change.title,change.detail,8};
    return {};
}
} // namespace apex3d::cp3
