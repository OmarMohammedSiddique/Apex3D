#pragma once
#include "cp3_race.hpp"
#include "generated/monza_entries_2025.hpp"
namespace apex3d::cp3 {
inline constexpr double cooldownDuration=180,parkingDuration=38;
inline double parkingRelease(const ArchivedEntry& entry){
    std::array<const ArchivedEntry*,18> finishers{};int n=0;
    for(const auto& e:archivedEntries)if(!e.dns&&!e.dnf)finishers[n++]=&e;
    std::sort(finishers.begin(),finishers.end(),[](auto a,auto b){return a->elapsed<b->elapsed;});
    double release=0;
    for(const auto* e:finishers){release=std::max(e->elapsed+cooldownDuration,release+14);if(e->number==entry.number)return release;}
    return entry.elapsed+cooldownDuration;
}
// Post-race seconds at which the last car is parked; the track invasion and
// podium celebration start then and run for celebrationDuration.
inline const double celebrationStart=[] {
    double last=raceTotalSeconds;
    for(const auto& entry:archivedEntries)if(!entry.dns&&!entry.dnf)last=std::max(last,parkingRelease(entry)+parkingDuration);
    return last-raceTotalSeconds+4;
}();
inline constexpr double celebrationDuration=75;
inline const double postRaceDuration=celebrationStart+celebrationDuration;
inline Pose parcFermePose(const std::vector<PathSample>& path,int rank){
    int arrival=0;
    if(rank>3)for(const auto& e:archivedEntries)if(e.rank>=4&&e.rank<=18&&(e.rank-4)%3==(rank-4)%3&&e.elapsed<archivedEntries[rank-1].elapsed)++arrival;
    const double along=rank<=3?6+(rank==1?0:2):100-7*arrival;
    auto pose=poseAtProgress(path,along/circuitMetres);
    const double lateral=rank<=3?18+(rank==1?0:rank==2?4:-4):55+3.5*((rank-4)%3);
    pose.position.y+=physical::sceneUnits(lateral);return pose;
}
inline RaceSample postRaceSample(const std::vector<PathSample>& path,const RaceMotion& geometry,const ArchivedEntry& entry,double seconds){
    const double dt=std::max(0.0,seconds-entry.elapsed),entryFraction=pitEntryProgress-36;
    const double gridStart=gridOffset-(entry.grid-1)*8/circuitMetres;
    const double inLapDuration=parkingRelease(entry)-entry.elapsed;
    if(dt<inLapDuration){
        // An illustrative in-lap, separate from recorded race timing.
        const double q=dt/inLapDuration,p=entry.totalLaps+entryFraction*q;
        auto pose=poseAtProgress(path,p);
        // Match the finishing lane, then merge smoothly into the pit entry.
        const double lane=physical::sceneUnits(entry.grid%2?2.25:-2.25)*std::pow(1-q,4);
        pose.position.x-=std::sin(pose.headingRadians)*lane;pose.position.y+=std::cos(pose.headingRadians)*lane;
        return {pose,p,entryFraction/inLapDuration*path.back().distance,false,false,
            (p-gridStart)*circuitMetres,true,false,false,true,false};
    }
    const double release=parkingRelease(entry);
    if(seconds<release)return {geometry.pitPose(pitEntryProgress,false),entry.totalLaps+entryFraction,0,true,false,(entry.totalLaps+entryFraction-gridStart)*circuitMetres,true,false,false,true,false};
    const double q=std::clamp((seconds-release)/parkingDuration,0.0,1.0);
    const auto target=parcFermePose(path,entry.rank);
    const double gateProgress=37-12/circuitMetres;const auto gate=geometry.pitPose(gateProgress,false);
    auto staging=poseAtProgress(path,-12/circuitMetres);staging.position.y=target.position.y;
    const auto distance=[](const Pose& a,const Pose& b){return std::hypot(a.position.x-b.position.x,a.position.y-b.position.y)*physical::metresPerSceneUnit();};
    const double laneMetres=(gateProgress-pitEntryProgress)*circuitMetres;
    Pose pose;double velocity=0,extra=0;
    if(q<.55){const double p=pitEntryProgress+(gateProgress-pitEntryProgress)*q/.55;pose=geometry.pitPose(p,false);
        velocity=laneMetres/(.55*parkingDuration)/physical::metresPerSceneUnit();extra=laneMetres*q/.55;}
    else {
        const bool turning=q<.75;const auto from=turning?gate:staging,to=turning?staging:target;
        const double duration=(turning?.20:.25)*parkingDuration,v=std::clamp((q-(turning?.55:.75))/(turning?.20:.25),0.0,1.0),u=v*v*(3-2*v);
        pose=from;pose.position={from.position.x+(to.position.x-from.position.x)*u,from.position.y+(to.position.y-from.position.y)*u};
        const double direction=std::atan2(to.position.y-from.position.y,to.position.x-from.position.x);
        pose.headingRadians=from.headingRadians+std::atan2(std::sin(direction-from.headingRadians),std::cos(direction-from.headingRadians))*std::clamp(v/.15,0.0,1.0);
        pose.headingRadians+=std::atan2(std::sin(to.headingRadians-pose.headingRadians),std::cos(to.headingRadians-pose.headingRadians))*std::clamp((v-.8)/.2,0.0,1.0);
        velocity=distance(from,to)/physical::metresPerSceneUnit()*6*v*(1-v)/duration;
        extra=laneMetres+(turning?0:distance(gate,staging))+distance(from,to)*u;
    }
    return {pose,entry.totalLaps+entryFraction+q*.02,velocity,true,false,(entry.totalLaps+entryFraction-gridStart)*circuitMetres+extra,true,false,false,true,q>=1};
}
} // namespace apex3d::cp3
