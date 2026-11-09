#pragma once
#include "hud3d.hpp"
#include "telemetry_heatmap.hpp"
#include <cmath>
#include <sstream>

// Bottom playback bar (play/pause, draggable race timeline, speed slider, lap time)
// and the heatmap colour legend. Drawing and mouse hit-testing share one layout,
// so what you see is exactly what you can click.
namespace apex3d::three {
struct ControlBarLayout {
    double left,right,bottom,top;
    double playLeft,playRight;
    double lineLeft,lineRight;
    double speedLeft,speedRight;
    double lineY;
};
inline ControlBarLayout controlBarLayout(bool dashboardVisible) {
    const double right=dashboardVisible?0.42:0.985;
    return {-0.985,right,-0.815,-0.700,-0.970,-0.920,-0.895,right-0.345,right-0.300,right-0.030,-0.778};
}
enum class BarHit { None, Play, Timeline, Speed };
struct BarTarget { BarHit hit{BarHit::None};double fraction{}; };
inline BarTarget controlBarAt(double x,double y,bool dashboardVisible) {
    const auto l=controlBarLayout(dashboardVisible);
    if(y<l.bottom||y>l.top||x<l.left||x>l.right)return {};
    const auto along=[&](double a,double b){return std::clamp((x-a)/(b-a),0.0,1.0);};
    if(x>=l.playLeft&&x<=l.playRight)return {BarHit::Play,0};
    // Generous margins either side make the ends easy to grab.
    if(x>=l.lineLeft-0.012&&x<=l.lineRight+0.012)return {BarHit::Timeline,along(l.lineLeft,l.lineRight)};
    if(x>=l.speedLeft-0.012&&x<=l.speedRight+0.012)return {BarHit::Speed,along(l.speedLeft,l.speedRight)};
    return {};
}
// Nine notches, doubling each time: 0.25x ... 64x (the existing +/- range).
inline constexpr int speedNotches=9;
inline double speedForFraction(double fraction) {
    const int notch=static_cast<int>(std::lround(std::clamp(fraction,0.0,1.0)*(speedNotches-1)));
    return std::ldexp(1.0,notch-2);
}
inline double speedFraction(double speed) {
    return std::clamp((std::log2(std::max(speed,1e-6))+2)/(speedNotches-1),0.0,1.0);
}
inline std::string speedText(double speed) {
    std::ostringstream text;
    if(speed<1)text<<speed;else text<<static_cast<int>(std::lround(speed));
    return text.str()+"X";
}
inline void controlBar(std::vector<cp3::Vertex>& mesh,const cp3::Playback& p,cp3::DriverFocus focus,
                       bool dashboardVisible,double bufferedFraction) {
    const auto l=controlBarLayout(dashboardVisible);
    constexpr cp3::Color panel{0.035F,0.075F,0.11F},white{0.94F,0.97F,0.97F},muted{0.55F,0.68F,0.72F},
        rail{0.12F,0.19F,0.23F},buffered{0.22F,0.31F,0.35F},amber{0.98F,0.74F,0.25F};
    const auto& info=cp3::driverInfo(focus);
    cp3::hudRectangle(mesh,l.left,l.bottom,l.right,l.top,panel);
    // Play / pause button.
    cp3::hudRectangle(mesh,l.playLeft,l.lineY-0.032,l.playRight,l.lineY+0.032,{0.075F,0.14F,0.18F});
    const double cx=(l.playLeft+l.playRight)/2;
    if(p.playing&&p.started)for(double side:{-1.0,1.0})
        cp3::hudRectangle(mesh,cx+side*0.009-0.004,l.lineY-0.018,cx+side*0.009+0.004,l.lineY+0.018,white);
    else cp3::triangle(mesh,{cx-0.010,l.lineY-0.020},{cx+0.013,l.lineY},{cx-0.010,l.lineY+0.020},white);

    // Timeline: whole race from lights-out to the winner's finish.
    const auto at=[&](double seconds){return l.lineLeft+(l.lineRight-l.lineLeft)*std::clamp(seconds/cp3::raceTotalSeconds,0.0,1.0);};
    cp3::hudRectangle(mesh,l.lineLeft,l.lineY-0.006,l.lineRight,l.lineY+0.006,rail);
    cp3::hudRectangle(mesh,l.lineLeft,l.lineY-0.006,l.lineLeft+(l.lineRight-l.lineLeft)*std::clamp(bufferedFraction,0.0,1.0),l.lineY+0.006,buffered);
    cp3::hudRectangle(mesh,l.lineLeft,l.lineY-0.006,at(p.elapsed),l.lineY+0.006,info.color);
    double lapStart=0;
    for(int lap=1;lap<info.totalLaps;++lap) {
        lapStart+=(*info.laps)[lap-1].milliseconds()/1000.0;
        const double x=at(lapStart),depth=lap%10==0?0.020:0.011;
        cp3::hudRectangle(mesh,x-0.0012,l.lineY-0.006-depth,x+0.0012,l.lineY-0.007,lap%10==0?white:muted);
    }
    if(info.pitEntry>=0) {
        const double x=at(info.pitEntry);
        cp3::triangle(mesh,{x-0.007,l.lineY+0.026},{x+0.007,l.lineY+0.026},{x,l.lineY+0.010},amber);
    }
    const double handle=at(p.elapsed);
    cp3::hudRectangle(mesh,handle-0.005,l.lineY-0.022,handle+0.005,l.lineY+0.022,white);
    const auto clock=cp3::driverClock(focus,p.simulationSeconds());
    std::string label=cp3::driverLabel(focus,p.simulationSeconds());
    if(info.totalLaps>0)label+="  LAP "+std::to_string(std::min(clock.lap,info.totalLaps))+"/"+std::to_string(info.totalLaps)+
        "  LAP TIME "+cp3::clockText(clock.lapElapsed)+"  RACE "+cp3::clockText(p.elapsed,true);
    else label+="  RACE "+cp3::clockText(p.elapsed,true);
    cp3::hudText(mesh,l.lineLeft,l.top-0.012,0.0020,label,white);
    cp3::hudText(mesh,l.lineRight-0.165,l.top-0.012,0.0017,"DRAG TO SCRUB",muted);

    // Speed slider: discrete doubling notches.
    cp3::hudRectangle(mesh,l.speedLeft,l.lineY-0.005,l.speedRight,l.lineY+0.005,rail);
    const double knob=l.speedLeft+(l.speedRight-l.speedLeft)*speedFraction(p.speed);
    cp3::hudRectangle(mesh,l.speedLeft,l.lineY-0.005,knob,l.lineY+0.005,info.color);
    for(int i=0;i<speedNotches;++i){const double x=l.speedLeft+(l.speedRight-l.speedLeft)*i/(speedNotches-1.0);
        cp3::hudRectangle(mesh,x-0.0012,l.lineY-0.017,x+0.0012,l.lineY-0.007,i==2?white:muted);}
    cp3::hudRectangle(mesh,knob-0.005,l.lineY-0.020,knob+0.005,l.lineY+0.020,white);
    cp3::hudText(mesh,l.speedLeft,l.top-0.012,0.0020,"SPEED "+speedText(p.speed),white);
}
// Colour key for the track heatmap. The bar uses the same ramp as the road,
// with per-vertex colours blended by the GPU in exactly the same way.
inline void heatLegend(std::vector<cp3::Vertex>& mesh,cp3::HeatLayer layer,cp3::DriverFocus focus,int lap,bool hasData) {
    constexpr cp3::Color panel{0.035F,0.075F,0.11F},white{0.94F,0.97F,0.97F},muted{0.55F,0.68F,0.72F};
    const double left=-0.975,right=-0.530; // fits "GABRIEL BORTOLETO / LAP 53 OF 53"
    if(layer==cp3::HeatLayer::Off) {
        cp3::hudRectangle(mesh,left,0.690,right,0.745,panel);
        cp3::hudText(mesh,left+0.020,0.728,0.0020,"TRACK HEATMAP OFF  V SHOW",muted);
        return;
    }
    const auto& info=cp3::driverInfo(focus);
    cp3::hudRectangle(mesh,left,0.495,right,0.745,panel);
    cp3::hudText(mesh,left+0.020,0.728,0.0022,std::string("TRACK HEATMAP / ")+cp3::heatLayerName(layer),white);
    cp3::hudText(mesh,left+0.020,0.690,0.0020,hasData?std::string(info.name)+" / LAP "+std::to_string(lap)+" OF "+std::to_string(info.totalLaps):
        std::string(info.name)+" / NO RECORDED LAP",info.color);
    const double barLeft=left+0.020,barRight=right-0.020,bottom=0.605,top=0.640;
    constexpr int segments=12;
    for(int i=0;i<segments;++i) {
        const double a=static_cast<double>(i)/segments,b=static_cast<double>(i+1)/segments;
        const auto ca=cp3::roadHeatColor(a),cb=cp3::roadHeatColor(b);
        const double xa=barLeft+(barRight-barLeft)*a,xb=barLeft+(barRight-barLeft)*b;
        const auto v=[&](double x,double y,cp3::Color c){mesh.push_back({static_cast<float>(x),static_cast<float>(y),c.r,c.g,c.b});};
        v(xa,bottom,ca);v(xb,bottom,cb);v(xb,top,cb);v(xa,bottom,ca);v(xb,top,cb);v(xa,top,ca);
    }
    const auto scale=cp3::heatScale(layer);
    cp3::hudText(mesh,barLeft,0.588,0.0019,scale.lowLabel,white);
    const std::string high=std::string(scale.highLabel)+" "+scale.unit;
    cp3::hudText(mesh,barRight-high.size()*6*0.0019,0.588,0.0019,high,white);
    cp3::hudText(mesh,barLeft,0.558,0.0017,"SLOW / BRAKING",muted);
    cp3::hudText(mesh,barRight-4*6*0.0017,0.558,0.0017,"FAST",muted);
    cp3::hudText(mesh,barLeft,0.522,0.0017,"V CHANGE LAYER",muted);
}
} // namespace apex3d::three
