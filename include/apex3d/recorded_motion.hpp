#pragma once
#include "generated/monza_telemetry_2025.hpp"
#include <algorithm>
#include <span>
#include <vector>

namespace apex3d::cp3 {
inline std::span<const TelemetryPoint> recordedSeries(int number) {
    if(number==4)return norrisTelemetry;
    if(number==81)return piastriTelemetry;
    return verstappenTelemetry;
}
// Integrate recorded speeds for the shape of motion between exact timing gates.
// Each sector is normalized to the simplified route's length; this is not GPS.
class RecordedDistance {
public:
    explicit RecordedDistance(int number=1):points_(recordedSeries(number)),metres_(points_.size(),0.0) {
        for(std::size_t i=1;i<points_.size();++i) {
            const auto& a=points_[i-1];const auto& b=points_[i];
            metres_[i]=metres_[i-1]+(a.speedKmh+b.speedKmh)/7.2*(b.raceMs-a.raceMs)/1000;
        }
    }
    double distance(double seconds)const {
        const auto i=segment(seconds);const auto& a=points_[i];const auto& b=points_[i+1];
        const double dt=std::clamp(seconds-a.raceMs/1000.0,0.0,(b.raceMs-a.raceMs)/1000.0);
        const double slope=(b.speedKmh-a.speedKmh)/3.6/((b.raceMs-a.raceMs)/1000.0);
        return metres_[i]+a.speedKmh/3.6*dt+0.5*slope*dt*dt;
    }
    double speed(double seconds)const {
        const auto i=segment(seconds);const auto& a=points_[i];const auto& b=points_[i+1];
        const double alpha=std::clamp((seconds*1000-a.raceMs)/(b.raceMs-a.raceMs),0.0,1.0);
        return (a.speedKmh+alpha*(b.speedKmh-a.speedKmh))/3.6;
    }
    double fraction(double time,double start,double end)const {
        const double span=distance(end)-distance(start);
        return span>1e-6?std::clamp((distance(time)-distance(start))/span,0.0,1.0):(time-start)/(end-start);
    }
    double rate(double time,double start,double end)const {
        const double span=distance(end)-distance(start);
        return span>1e-6?speed(time)/span:1.0/(end-start);
    }
private:
    std::span<const TelemetryPoint> points_;std::vector<double> metres_;
    std::size_t segment(double seconds)const {
        const auto next=std::upper_bound(points_.begin(),points_.end(),seconds*1000,
            [](double t,const TelemetryPoint& p){return t<p.raceMs;});
        if(next==points_.begin())return 0;
        return std::min(static_cast<std::size_t>(next-points_.begin()-1),points_.size()-2);
    }
};
} // namespace apex3d::cp3
