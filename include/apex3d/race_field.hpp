#pragma once
#include "cp3_race.hpp"
#include "generated/monza_field_2025.hpp"

namespace apex3d::cp3 {
struct OpponentSpec {
    int number;
    const char* name;
    const std::array<RaceLap,53>* laps;
    double finish,entry,exit,service,boxStart;
    int entryLap;
    double gridBehindMetres;
};
inline constexpr OpponentSpec norrisSpec{4,"NORRIS",&norrisLaps,4423.532,3827.924,3855.422,5.9,3834.279,46,8};
inline constexpr OpponentSpec piastriSpec{81,"PIASTRI",&piastriLaps,4425.676,3748.148,3771.750,1.9,3754.639,45,24};
struct OpponentSample {Pose pose;double progress,travelledMetres;bool inPit,servicing,finished;double velocity{};};
class OpponentMotion {
public:
    OpponentMotion(const std::vector<PathSample>& path,const RaceMotion& geometry,OpponentSpec spec)
        :path_(path),geometry_(geometry),spec_(spec),profile_(spec.number) {
        keys_.push_back({0,gridOffset-spec.gridBehindMetres/circuitMetres});
        double time=0;
        for(std::size_t i=0;i<spec.laps->size();++i) {
            const auto& lap=(*spec.laps)[i];
            keys_.push_back({time+lap.s1/1000.0,i+sector1Fraction});
            keys_.push_back({time+(lap.s1+lap.s2)/1000.0,i+sector2Fraction});
            time+=lap.milliseconds()/1000.0;keys_.push_back({time,double(i+1)});
        }
        // Gate durations are archived; arrival is bracketed by stationary speed samples.
        // Box position is estimated by integrating speed after the timing line.
        boxStart_=spec.boxStart;boxEnd_=boxStart_+spec.service;
        const double shift=spec.entryLap-37;
        keys_.push_back({spec.entry,pitEntryProgress+shift});
        keys_.push_back({boxStart_,37.0+shift+34/circuitMetres});
        keys_.push_back({boxEnd_,37.0+shift+34/circuitMetres});
        keys_.push_back({spec.exit,pitExitProgress+shift});
        std::sort(keys_.begin(),keys_.end(),[](const Key& a,const Key& b){return a.time<b.time;});
    }
    OpponentSample sample(double seconds) const {
        const double t=std::clamp(seconds,0.0,spec_.finish);
        if(t>=spec_.finish){auto pose=poseAtProgress(path_,0);offset(pose,1);
            return {pose,53,(53-gridOffset)*circuitMetres+spec_.gridBehindMetres,false,false,true};}
        const auto next=std::upper_bound(keys_.begin(),keys_.end(),t,[](double v,const Key& k){return v<k.time;});
        const auto& a=*(next-1);const auto& b=*next;
        const double p=a.progress+profile_.fraction(t,a.time,b.time)*(b.progress-a.progress);
        const bool pit=t>=spec_.entry&&t<spec_.exit;
        auto pose=pit?geometry_.pitPose(p-(spec_.entryLap-37),true,37.0+34/circuitMetres):poseAtProgress(path_,p);
        // Small driver-specific offsets depict parallel cars without asserting measured racing lines.
        if(!pit){const double fade=t<spec_.entry?std::clamp((spec_.entry-t)/2,0.0,1.0):std::clamp((t-spec_.exit)/2,0.0,1.0);
            offset(pose,fade);}
        return {pose,p,(p-gridOffset+spec_.gridBehindMetres/circuitMetres)*circuitMetres,pit,t>=boxStart_&&t<boxEnd_,false,
            (b.progress-a.progress)*profile_.rate(t,a.time,b.time)*path_.back().distance};
    }
    const OpponentSpec& spec()const{return spec_;}
private:
    struct Key{double time,progress;};
    std::vector<PathSample> path_;const RaceMotion& geometry_;OpponentSpec spec_;
    std::vector<Key> keys_;double boxStart_{},boxEnd_{};RecordedDistance profile_;
    void offset(Pose& pose,double fade)const {
        const double lane=physical::sceneUnits(spec_.number==4?-2.25:2.25)*fade;
        pose.position.x-=std::sin(pose.headingRadians)*lane;pose.position.y+=std::cos(pose.headingRadians)*lane;
    }
};
} // namespace apex3d::cp3
