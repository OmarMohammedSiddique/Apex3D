#pragma once
#include "traffic_replay.hpp"
#include "scene3d.hpp"
#include <array>
#include <cmath>
#include <limits>
#include <vector>

// Track-surface telemetry heatmap: the proposal's "colour the track itself" feature.
//
// Pipeline (all offline-recorded data, no invented values):
//   1. Take the focused driver's recorded telemetry samples inside one lap's
//      official timing window.
//   2. Ask FieldMotion where that driver was along the circuit at each sample time
//      (the same timing-gate reconstruction that moves the car).
//   3. Store each measured value at that point of the centreline, then fill the
//      gaps between samples by linear interpolation along the track distance.
//   4. Turn values into colours with a piecewise-linear colour ramp, give every road
//      vertex its own colour, and let OpenGL interpolate colour across each triangle.
//
// Position accuracy follows the replay: lap and sector gates are exact, positions
// between them are reconstructed from the recorded speed, not GPS.
namespace apex3d::cp3 {
enum class HeatLayer { Off, Speed };
inline constexpr std::array<HeatLayer,2> heatLayers{HeatLayer::Speed,HeatLayer::Off};
inline const char* heatLayerName(HeatLayer layer) {
    switch(layer){case HeatLayer::Speed:return "SPEED";case HeatLayer::Off:return "OFF";}
    return "OFF";
}
inline HeatLayer nextHeatLayer(HeatLayer layer) {
    for(std::size_t i=0;i<heatLayers.size();++i)
        if(heatLayers[i]==layer)return heatLayers[(i+1)%heatLayers.size()];
    return HeatLayer::Speed;
}
// A fixed scale lets laps and drivers be compared by colour alone.
struct HeatScale { double low,high;const char* unit;const char* lowLabel;const char* highLabel; };
inline HeatScale heatScale(HeatLayer) { return {70,350,"KM/H","70","350"}; }

// Red = slow / braking, through amber and yellow, to green = fast / full throttle,
// matching the proposal's "green for throttle, red for braking" convention.
inline Color heatColor(double t) {
    constexpr std::array<Color,4> stops{{{0.86F,0.13F,0.11F},{0.96F,0.52F,0.10F},
                                         {0.96F,0.86F,0.18F},{0.16F,0.78F,0.30F}}};
    if(!std::isfinite(t))t=0;
    t=std::clamp(t,0.0,1.0)*(stops.size()-1);
    const auto i=std::min(static_cast<std::size_t>(t),stops.size()-2);
    const auto f=static_cast<float>(t-i);
    const auto& a=stops[i];const auto& b=stops[i+1];
    return {a.r+(b.r-a.r)*f,a.g+(b.g-a.g)*f,a.b+(b.b-a.b)*f};
}
// What is painted on the road: the ramp colour with a quarter of the asphalt mixed
// back in, so the surface still reads as tarmac. The legend uses the same colour.
inline Color roadHeatColor(double t) {
    constexpr Color asphalt{0.17F,0.195F,0.225F};constexpr float keep=0.75F;
    const auto c=heatColor(t);
    return {c.r*keep+asphalt.r*(1-keep),c.g*keep+asphalt.g*(1-keep),c.b*keep+asphalt.b*(1-keep)};
}
inline double heatFraction(double value,HeatLayer layer) {
    const auto scale=heatScale(layer);
    return std::clamp((value-scale.low)/(scale.high-scale.low),0.0,1.0);
}

struct LapWindow { double start{},end{};int lap{}; };
// Official timing window of a driver's lap (1-based). Lap 1 starts at lights-out.
inline LapWindow lapWindow(const DriverInfo& info,int lap) {
    if(info.totalLaps<=0)return {};
    lap=std::clamp(lap,1,info.totalLaps);
    double start=0;
    for(int i=0;i<lap-1;++i)start+=(*info.laps)[i].milliseconds()/1000.0;
    return {start,start+(*info.laps)[lap-1].milliseconds()/1000.0,lap};
}
// The lap shown for a driver at a race instant: the lap they are on, or their last lap.
inline int heatmapLap(DriverFocus focus,double seconds) {
    const auto& info=driverInfo(focus);
    if(info.totalLaps<=0)return 0;
    return std::clamp(driverClock(focus,seconds).lap,1,info.totalLaps);
}

inline double layerValue(const TelemetryPoint& point,HeatLayer) { return point.speedKmh; }

// One value per centreline sample (path index). NaN means "no recorded data".
inline std::vector<double> lapProfile(const std::vector<PathSample>& path,const FieldMotion& motion,
                                      DriverFocus focus,int lap,HeatLayer layer) {
    const double missing=std::numeric_limits<double>::quiet_NaN();
    std::vector<double> values(path.size(),missing);
    const auto& info=driverInfo(focus);
    if(layer==HeatLayer::Off||path.size()<2||info.totalLaps<=0||info.archive->dns)return values;
    const auto window=lapWindow(info,lap);
    const double total=path.back().distance;
    std::vector<double> sum(path.size(),0.0);std::vector<int> count(path.size(),0);
    for(const auto& point:recordedSeries(static_cast<int>(focus))) {
        const double t=point.raceMs/1000.0;
        if(t<window.start||t>=window.end)continue;
        const auto s=motion.sample(t);
        if(s.inPit||!s.visible)continue; // pit lane is a different road
        const double along=(s.progress-std::floor(s.progress))*total;
        const auto next=std::lower_bound(path.begin(),path.end(),along,
            [](const PathSample& p,double d){return p.distance<d;});
        auto index=static_cast<std::size_t>(std::min<std::ptrdiff_t>(next-path.begin(),path.size()-1));
        if(index>0&&along-path[index-1].distance<path[index].distance-along)--index;
        sum[index]+=layerValue(point,layer);++count[index];
    }
    // The last sample closes the loop at the first point; keep them as one location.
    const std::size_t loop=path.size()-1;
    if(count[loop]){sum[0]+=sum[loop];count[0]+=count[loop];count[loop]=0;}
    std::vector<std::size_t> known;
    for(std::size_t i=0;i<loop;++i)if(count[i]){values[i]=sum[i]/count[i];known.push_back(i);}
    if(known.empty())return values;
    // Linear interpolation by track distance between neighbouring measurements,
    // wrapping across the timing line.
    for(std::size_t k=0;k<known.size();++k) {
        const auto a=known[k],b=known[(k+1)%known.size()];
        const double da=path[a].distance;
        double span=path[b].distance-da;if(span<=0)span+=total;
        for(std::size_t i=(a+1)%loop;i!=b;i=(i+1)%loop) {
            double d=path[i].distance-da;if(d<0)d+=total;
            values[i]=values[a]+(values[b]-values[a])*(d/span);
        }
    }
    values[loop]=values[0];
    return values;
}
} // namespace apex3d::cp3

namespace apex3d::three {
// Coloured road overlay, 5 mm above the asphalt and inside the white edge lines.
inline Mesh heatmapMesh(const std::vector<cp3::PathSample>& path,const std::vector<double>& values,cp3::HeatLayer layer) {
    Mesh mesh;
    if(layer==cp3::HeatLayer::Off||values.size()!=path.size()||path.size()<2)return mesh;
    const double half=physical::trackWidth/2-0.15,height=physical::roadHeight+0.005;
    const V3 up{0,1,0};
    const auto edge=[&](const cp3::Pose& p,double offset){return world(p.position,height)+rotateY({0,0,offset},p.headingRadians);};
    for(std::size_t i=0;i+1<path.size();++i) {
        if(!std::isfinite(values[i])||!std::isfinite(values[i+1]))continue;
        const auto a=cp3::poseAtProgress(path,path[i].distance/path.back().distance),
                   b=cp3::poseAtProgress(path,path[i+1].distance/path.back().distance);
        // Each end of the strip gets its own colour; the GPU blends between them.
        const auto ca=cp3::roadHeatColor(cp3::heatFraction(values[i],layer)),
                   cb=cp3::roadHeatColor(cp3::heatFraction(values[i+1],layer));
        const auto al=edge(a,-half),ar=edge(a,half),bl=edge(b,-half),br=edge(b,half);
        vertex(mesh,al,up,ca);vertex(mesh,ar,up,ca);vertex(mesh,br,up,cb);
        vertex(mesh,al,up,ca);vertex(mesh,br,up,cb);vertex(mesh,bl,up,cb);
    }
    return mesh;
}
} // namespace apex3d::three
