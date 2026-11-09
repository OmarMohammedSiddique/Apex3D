#pragma once
#include "cp3_race.hpp"
#include "generated/monza_entries_2025.hpp"
namespace apex3d::cp3 {
inline constexpr double cooldownDuration=180,parkingDuration=38;
inline constexpr double postRaceDuration=[] {
    double last=raceTotalSeconds;
    for(const auto& entry:archivedEntries)if(!entry.dns&&!entry.dnf)last=std::max(last,entry.elapsed);
    return last-raceTotalSeconds+cooldownDuration+parkingDuration+.001;
}();
inline Pose parcFermePose(const std::vector<PathSample>& path,int rank){
    const double along=rank<=3?6+(rank==1?0:2):62+7*((rank-4)/3);
    auto pose=poseAtProgress(path,along/circuitMetres);
    const double lateral=rank<=3?18+(rank==1?0:rank==2?4:-4):55+3.5*((rank-4)%3);
    pose.position.y+=physical::sceneUnits(lateral);return pose;
}
inline RaceSample postRaceSample(const std::vector<PathSample>& path,const RaceMotion& geometry,const ArchivedEntry& entry,double seconds){
    const double dt=std::max(0.0,seconds-entry.elapsed),entryFraction=pitEntryProgress-36;
    const double gridStart=gridOffset-(entry.grid-1)*8/circuitMetres;
    if(dt<cooldownDuration){
        // An illustrative in-lap, separate from recorded race timing.
        const double q=dt/cooldownDuration,p=entry.totalLaps+entryFraction*q;
        auto pose=poseAtProgress(path,p);
        // Match the finishing lane, then merge smoothly into the pit entry.
        const double lane=physical::sceneUnits(entry.grid%2?2.25:-2.25)*std::pow(1-q,4);
        pose.position.x-=std::sin(pose.headingRadians)*lane;pose.position.y+=std::cos(pose.headingRadians)*lane;
        return {pose,p,entryFraction/cooldownDuration*path.back().distance,false,false,
            (p-gridStart)*circuitMetres,true,false,false,true,false};
    }
    const double q=std::clamp((dt-cooldownDuration)/parkingDuration,0.0,1.0);
    const auto target=parcFermePose(path,entry.rank),gate=geometry.pitPose(37,false);
    Pose pose;double velocity=0,extra=0;
    const double pitMetres=(37.0-pitEntryProgress)*circuitMetres;
    if(q<.48){const double p=pitEntryProgress+(37.0-pitEntryProgress)*(q/.48);
        pose=geometry.pitPose(p,false);velocity=pitMetres/(.48*parkingDuration)/physical::metresPerSceneUnit();extra=pitMetres*q/.48;}
    else {
        auto from=gate,to=target;double v=(q-.48)/.52,duration=.52*parkingDuration;
        auto via=poseAtProgress(path,5/circuitMetres);via.position.y+=physical::sceneUnits(55);
        const auto distance=[](const Pose& a,const Pose& b){return std::hypot(a.position.x-b.position.x,a.position.y-b.position.y)*physical::metresPerSceneUnit();};
        extra=pitMetres;
        if(entry.rank>3){duration/=2;if(v<.5){to=via;v*=2;}else{from=via;v=(v-.5)*2;extra+=distance(gate,via);}}
        const double s=v*v*(3-2*v);
        pose=from;pose.position={from.position.x*(1-s)+to.position.x*s,from.position.y*(1-s)+to.position.y*s};
        const double direction=std::atan2(to.position.y-from.position.y,to.position.x-from.position.x);
        const double entryBlend=std::clamp(v/.15,0.0,1.0),exitBlend=std::clamp((v-.80)/.20,0.0,1.0);
        pose.headingRadians=from.headingRadians+std::atan2(std::sin(direction-from.headingRadians),std::cos(direction-from.headingRadians))*entryBlend;
        pose.headingRadians+=std::atan2(std::sin(to.headingRadians-pose.headingRadians),std::cos(to.headingRadians-pose.headingRadians))*exitBlend;
        velocity=distance(from,to)/physical::metresPerSceneUnit()*6*v*(1-v)/duration;extra+=distance(from,to)*s;
    }
    const double travel=(entry.totalLaps+entryFraction-gridStart)*circuitMetres+extra;
    return {pose,entry.totalLaps+entryFraction+q*.02,velocity,true,false,travel,true,false,false,true,q>=1};
}
} // namespace apex3d::cp3
