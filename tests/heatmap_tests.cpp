#include "../include/apex3d/replay_controls.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#define CHECK(x) do{if(!(x))throw std::runtime_error("Check failed: " #x);}while(false)
namespace {
bool same(apex3d::cp3::Color a,apex3d::cp3::Color b){return std::abs(a.r-b.r)<1e-6F&&std::abs(a.g-b.g)<1e-6F&&std::abs(a.b-b.b)<1e-6F;}
bool near(double a,double b,double e=1e-6){return std::abs(a-b)<=e;}
}
int main(){using namespace apex3d;using namespace cp3;using namespace three;
try{
    // Colour ramp: red at the slow end, green at the fast end, continuous in between.
    CHECK(same(heatColor(0),{0.86F,0.13F,0.11F}));CHECK(same(heatColor(1),{0.16F,0.78F,0.30F}));
    CHECK(same(heatColor(-5),heatColor(0)));CHECK(same(heatColor(9),heatColor(1)));CHECK(same(heatColor(NAN),heatColor(0)));
    for(int i=0;i<1000;++i){const auto a=heatColor(i/1000.0),b=heatColor((i+1)/1000.0);
        CHECK(std::abs(a.r-b.r)<0.01F&&std::abs(a.g-b.g)<0.01F&&std::abs(a.b-b.b)<0.01F);}
    CHECK(near(heatFraction(70,HeatLayer::Speed),0));CHECK(near(heatFraction(350,HeatLayer::Speed),1));
    CHECK(near(heatFraction(210,HeatLayer::Speed),0.5));CHECK(near(heatFraction(500,HeatLayer::Speed),1));
    // V cycles speed -> throttle -> brake -> pedals -> off -> speed.
    CHECK(nextHeatLayer(HeatLayer::Speed)==HeatLayer::Throttle&&nextHeatLayer(HeatLayer::Throttle)==HeatLayer::Brake);
    CHECK(nextHeatLayer(HeatLayer::Brake)==HeatLayer::Pedals&&nextHeatLayer(HeatLayer::Pedals)==HeatLayer::Off);
    CHECK(nextHeatLayer(HeatLayer::Off)==HeatLayer::Speed);
    // Braking is drawn red (inverted scale); full throttle green.
    CHECK(near(heatFraction(100,HeatLayer::Brake),0)&&near(heatFraction(0,HeatLayer::Brake),1));
    CHECK(near(heatFraction(100,HeatLayer::Throttle),1)&&near(heatFraction(0,HeatLayer::Throttle),0));
    // Pedal samples: brake on -> red end; coasting amber; full throttle green. 104 glitch skipped.
    CHECK(near(layerValue({0,300,8,11000,0,100},HeatLayer::Pedals),0));
    CHECK(near(layerValue({0,300,8,11000,100,0},HeatLayer::Pedals),100));
    CHECK(near(layerValue({0,300,8,11000,0,0},HeatLayer::Pedals),30));
    CHECK(!std::isfinite(layerValue({0,342,8,11993,104,104},HeatLayer::Throttle)));
    CHECK(!std::isfinite(layerValue({0,314,8,11094,99,104},HeatLayer::Brake)));
    CHECK(near(layerValue({0,314,8,11094,99,104},HeatLayer::Speed),314));
    // Live readings expose throttle and brake, and refuse the glitch code.
    bool sawBrake=false,sawThrottle=false;
    for(double t=60;t<160;t+=0.25){const auto d=telemetryAt(t,1);if(!d.available)continue;
        CHECK(d.throttlePct>=0&&d.throttlePct<=100);sawBrake|=d.braking;sawThrottle|=d.throttlePct>=99;}
    CHECK(sawBrake&&sawThrottle);

    // Official lap windows.
    const auto& ver=driverInfo(DriverFocus::Verstappen);
    CHECK(near(lapWindow(ver,1).start,0)&&near(lapWindow(ver,1).end,87.159));
    CHECK(near(lapWindow(ver,53).end,raceTotalSeconds,1e-6));CHECK(lapWindow(ver,99).lap==53);
    CHECK(heatmapLap(DriverFocus::Verstappen,0)==1&&heatmapLap(DriverFocus::Verstappen,87.2)==2);
    CHECK(heatmapLap(DriverFocus::Verstappen,raceTotalSeconds+100)==53);
    CHECK(heatmapLap(DriverFocus::Hulkenberg,100)==0);

    const auto path=makePath();const RaceMotion race(path);const TrafficReplay traffic(path,race);
    const auto profile=[&](DriverFocus f,int lap,HeatLayer layer=HeatLayer::Speed){
        return lapProfile(path,traffic.motion(static_cast<std::size_t>(driverIndex(f))),f,lap,layer);};

    // Verstappen lap 6: every point of the circuit has a value, within the lap's recorded range.
    const auto values=profile(DriverFocus::Verstappen,6);
    const auto window=lapWindow(ver,6);double low=1e9,high=-1e9;
    for(const auto& p:recordedSeries(1))if(p.raceMs/1000.0>=window.start&&p.raceMs/1000.0<window.end){low=std::min(low,double(p.speedKmh));high=std::max(high,double(p.speedKmh));}
    CHECK(values.size()==path.size());
    double vLow=1e9,vHigh=-1e9;
    for(double v:values){CHECK(std::isfinite(v));CHECK(v>=low-1e-9&&v<=high+1e-9);vLow=std::min(vLow,v);vHigh=std::max(vHigh,v);}
    CHECK(near(values.front(),values.back()));
    CHECK(vLow<130);CHECK(vHigh>320); // Monza: slow chicanes, very fast straights

    // Every lap of every classified driver, including pit laps, covers the whole circuit.
    for(const auto& d:raceDrivers){if(d.archive->dns)continue;
        for(int lap=1;lap<=d.totalLaps;++lap)for(double v:profile(d.focus,lap))CHECK(std::isfinite(v));}

    // Pedal layers: throttle in range; braking happens on part of the lap, not all of it.
    for(auto layer:{HeatLayer::Throttle,HeatLayer::Brake,HeatLayer::Pedals}){
        const auto lap=profile(DriverFocus::Verstappen,6,layer);int braking=0;
        for(double v:lap){CHECK(std::isfinite(v));CHECK(v>=0&&v<=100);if(layer==HeatLayer::Brake&&v>50)++braking;}
        if(layer==HeatLayer::Brake){CHECK(braking>0);CHECK(braking<static_cast<int>(lap.size())/4);}
    }
    for(const auto& d:raceDrivers){if(d.archive->dns)continue;
        for(int lap=1;lap<=d.totalLaps;lap+=13)for(double v:profile(d.focus,lap,HeatLayer::Pedals))CHECK(std::isfinite(v));}

    // No recorded race, or layer switched off: nothing is painted.
    for(double v:profile(DriverFocus::Hulkenberg,1))CHECK(!std::isfinite(v));
    for(double v:profile(DriverFocus::Verstappen,6,HeatLayer::Off))CHECK(!std::isfinite(v));
    CHECK(heatmapMesh(path,profile(DriverFocus::Hulkenberg,1),HeatLayer::Speed).empty());
    CHECK(heatmapMesh(path,values,HeatLayer::Off).empty());

    // Road overlay: one coloured strip per centreline segment, just above the asphalt.
    const auto mesh=heatmapMesh(path,values,HeatLayer::Speed);
    CHECK(mesh.size()==(path.size()-1)*6);
    for(const auto& v:mesh){CHECK(near(v.y,physical::roadHeight+0.005,1e-5));CHECK(near(v.ny,1));
        CHECK(v.r>=0&&v.r<=1&&v.g>=0&&v.g<=1&&v.b>=0&&v.b<=1);}

    // Speed slider: nine doubling notches matching the +/- range.
    for(int i=0;i<speedNotches;++i){const double s=speedForFraction(i/8.0);CHECK(near(s,std::ldexp(1.0,i-2)));CHECK(near(speedFraction(s),i/8.0));}
    CHECK(near(speedForFraction(-1),0.25)&&near(speedForFraction(2),64));
    CHECK(speedText(0.25)=="0.25X"&&speedText(1)=="1X"&&speedText(64)=="64X");

    // Hit-testing uses the same layout as drawing.
    for(bool visible:{true,false}){const auto l=controlBarLayout(visible);
        CHECK(controlBarAt((l.playLeft+l.playRight)/2,l.lineY,visible).hit==BarHit::Play);
        const auto start=controlBarAt(l.lineLeft,l.lineY,visible),end=controlBarAt(l.lineRight,l.lineY,visible);
        CHECK(start.hit==BarHit::Timeline&&near(start.fraction,0)&&end.hit==BarHit::Timeline&&near(end.fraction,1));
        const auto mid=controlBarAt((l.lineLeft+l.lineRight)/2,l.lineY,visible);CHECK(near(mid.fraction,0.5));
        CHECK(controlBarAt(l.speedRight,l.lineY,visible).hit==BarHit::Speed);
        CHECK(near(controlBarAt(l.speedRight,l.lineY,visible).fraction,1));
        CHECK(controlBarAt(0,0,visible).hit==BarHit::None);CHECK(controlBarAt(l.left+0.01,l.top+0.01,visible).hit==BarHit::None);
        CHECK(l.right<=(visible?0.45:1.0)); // never under the dashboard
    }
    std::cout<<"Heatmap and replay control tests passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
