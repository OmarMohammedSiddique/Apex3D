#pragma once
#include "driver_focus.hpp"
#include "post_race.hpp"

namespace apex3d::cp3 {
// Sector and pit gates are measured; the intervening racing line is schematic.
class FieldMotion {
    struct Key { double time,progress; };
    const std::vector<PathSample>& path_;
    const RaceMotion& geometry_;
    const ArchivedEntry& entry_;
    RecordedDistance profile_;
    std::vector<Key> keys_;
    double start_{},pitStartExit_{-1};
public:
    FieldMotion(const std::vector<PathSample>& path,const RaceMotion& geometry,const ArchivedEntry& entry)
        :path_(path),geometry_(geometry),entry_(entry),profile_(entry.number) {
        if(entry.dns)return;
        const auto nearest=[&](Vec2 target){
            const auto it=std::min_element(path.begin(),path.end(),[&](const auto& a,const auto& b){
                return std::hypot(a.point.x-target.x,a.point.y-target.y)<std::hypot(b.point.x-target.x,b.point.y-target.y);});
            return it->distance/path.back().distance;
        };
        start_=gridOffset-(entry.grid-1)*8/circuitMetres;
        if(entry.pitStart) {
            for(const auto& p:recordedSeries(entry.number))if(p.raceMs>=0&&p.raceMs<20000&&p.speedKmh>=85){
                pitStartExit_=p.raceMs/1000.0;break;
            }
            if(pitStartExit_<0)pitStartExit_=10;
            start_=332/circuitMetres-(profile_.distance(pitStartExit_)-profile_.distance(0))/circuitMetres;
            keys_.push_back({pitStartExit_,332/circuitMetres});
        }
        keys_.push_back({0,start_});
        double t=0;
        for(int i=0;i<entry.totalLaps;++i){const auto& lap=entry.laps[i];
            keys_.push_back({t+lap.s1/1000.0,i+nearest({-1.33,-.12})});
            keys_.push_back({t+(lap.s1+lap.s2)/1000.0,i+nearest({-.02,-.14})});
            t+=lap.milliseconds()/1000.0;keys_.push_back({t,double(i+1)});
        }
        for(const auto& pit:archivedPits)if(pit.driver==entry.number){
            const double shift=pit.lap-37,box=pit.lap+pit.boxMetres/circuitMetres;
            keys_.push_back({pit.entry,pitEntryProgress+shift});
            keys_.push_back({pit.boxStart,box});keys_.push_back({pit.boxStart+pit.service,box});
            keys_.push_back({pit.exit,pitExitProgress+shift});
        }
        if(entry.dnf){
            keys_.push_back({entry.retireEntry,pitEntryProgress+(entry.totalLaps-37)});
            keys_.push_back({entry.retire,entry.totalLaps+entry.retireBox/circuitMetres});
        }
        std::sort(keys_.begin(),keys_.end(),[](auto a,auto b){return a.time<b.time;});
    }
    RaceSample sample(double seconds)const {
        if(!std::isfinite(seconds))seconds=0;
        if(entry_.dns)return {geometry_.pitPose(37+240/circuitMetres),0,0,false,false,0,false,false,true};
        if(!entry_.dnf&&seconds>=entry_.elapsed)return postRaceSample(path_,geometry_,entry_,seconds);
        const double end=entry_.dnf?entry_.retire:entry_.elapsed;
        const double t=std::clamp(seconds,0.0,end);
        double p=keys_.back().progress,velocity=0;
        const auto next=std::upper_bound(keys_.begin(),keys_.end(),t,[](double v,auto k){return v<k.time;});
        if(next!=keys_.end()&&next!=keys_.begin()){
            const auto& a=*(next-1);const auto& b=*next;
            p=a.progress+profile_.fraction(t,a.time,b.time)*(b.progress-a.progress);
            velocity=(b.progress-a.progress)*profile_.rate(t,a.time,b.time)*path_.back().distance;
        }
        bool inPit=false,service=false;double shift=0,box=pitBoxProgress,fade=1;
        if(entry_.pitStart&&t>=pitStartExit_)fade=std::clamp((t-pitStartExit_)/2,0.0,1.0);
        if(entry_.pitStart&&t<pitStartExit_){inPit=true;shift=-37;box=37;}
        for(const auto& pit:archivedPits)if(pit.driver==entry_.number){
            fade=std::min(fade,std::min(std::abs(t-pit.entry),std::abs(t-pit.exit))/2);
            if(t>=pit.entry&&t<pit.exit){inPit=true;shift=pit.lap-37;box=37+pit.boxMetres/circuitMetres;
                service=t>=pit.boxStart&&t<pit.boxStart+pit.service;}
        }
        if(entry_.dnf&&t>=entry_.retireEntry){inPit=true;shift=entry_.totalLaps-37;box=37+entry_.retireBox/circuitMetres;}
        auto pose=inPit?geometry_.pitPose(p-shift,!entry_.pitStart||t>=pitStartExit_,box):poseAtProgress(path_,p);
        if(!inPit){const double lane=physical::sceneUnits(entry_.grid%2?2.25:-2.25)*std::clamp(fade,0.0,1.0);
            pose.position.x-=std::sin(pose.headingRadians)*lane;pose.position.y+=std::cos(pose.headingRadians)*lane;}
        const bool retired=entry_.dnf&&seconds>=entry_.retire;
        return {pose,p,retired?0:velocity,inPit,service,std::max(0.0,(p-start_)*circuitMetres),!retired,retired,false};
    }
};
} // namespace apex3d::cp3
