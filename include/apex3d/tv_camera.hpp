#pragma once
// Included from scene3d.hpp after world_detail.hpp.
// Trackside TV cameras, like the broadcast coverage: fixed camera towers around
// the lap. The TV view cuts to the tower ahead of the selected car and zooms to
// keep the car framed as it approaches and passes.
//
// Towers stand every 200 m, 13.4 m from the centreline (behind the catch fence,
// in front of every grandstand) with the lens 7 m up. They go on the outside of
// corners, and on the left on straights, away from the pit lane and garages.
// The real broadcast camera plan isn't public, so tower positions are representative.
#include <algorithm>
#include <vector>
namespace apex3d::three {
struct TvCamera { V3 lens; V3 base; double progress; };
inline constexpr double tvTowerSpacing=200.0,tvTowerLateral=13.4,tvLensHeight=7.0;
inline std::vector<TvCamera> makeTvCameras(const std::vector<cp3::PathSample>& path){
    std::vector<TvCamera> cameras;
    const double lap=cp3::circuitMetres;
    for(double nominal=40;nominal<lap-tvTowerSpacing/2;nominal+=tvTowerSpacing){
        // Slide a tower along the track if it would stand on the old banking or its bridges.
        for(double shift:{0.0,40.0,-40.0,80.0,-80.0}){
            const double progress=(nominal+shift)/lap;
            const auto here=cp3::poseAtProgress(path,progress);
            const auto before=cp3::poseAtProgress(path,progress-60/lap),after=cp3::poseAtProgress(path,progress+60/lap);
            const double turn=std::remainder(after.headingRadians-before.headingRadians,2*std::numbers::pi); // + = left
            // rotateY lateral: + is right of travel. Outside of a left turn is the right.
            double side=std::abs(turn)>0.2?(turn>0?1.0:-1.0):-1.0;
            if(cp3::onPitLane(progress))side=-1.0;
            const auto base=world(here.position)+rotateY({0,0,side*tvTowerLateral},here.headingRadians);
            if(ovalDistance(base)<30)continue;
            // The lens sits on an arm 1.1 m out over the trackside edge of the platform, so the
            // tower itself never enters the shot.
            const auto lens=base+V3{0,tvLensHeight,0}+rotateY({0,0,-side*1.1},here.headingRadians);
            cameras.push_back({lens,base,progress-std::floor(progress)});
            break;
        }
    }
    std::sort(cameras.begin(),cameras.end(),[](const auto& a,const auto& b){return a.progress<b.progress;});
    return cameras;
}
inline const std::vector<TvCamera>& tvCameras(){static const auto cameras=makeTvCameras(cp3::makePath());return cameras;}
// The director's cut list, worked out once: for each 1/2000 of the lap, the nearest
// tower to a point 40 m ahead of the car (so the car is seen approaching) whose line
// of sight to the car clears every tree trunk by 4.5 m. Shots shorter than 45 m are
// merged into the previous one so the view doesn't flick between towers.
inline constexpr int tvScheduleSteps=2000;
inline const std::vector<int>& tvSchedule(){
    static const auto schedule=[]{
        const auto path=cp3::makePath();const auto trees=woodlandTrees(path);const auto& cameras=tvCameras();
        std::vector<int> pick(tvScheduleSteps,0);
        std::vector<int> order(cameras.size());
        for(int k=0;k<tvScheduleSteps;++k){
            const double progress=(k+0.5)/tvScheduleSteps,target=progress+40/cp3::circuitMetres;
            const auto car=world(cp3::poseAtProgress(path,progress).position);
            for(std::size_t i=0;i<order.size();++i)order[i]=static_cast<int>(i);
            const auto gap=[&](int i){double d=std::abs(cameras[static_cast<std::size_t>(i)].progress-(target-std::floor(target)));return std::min(d,1-d);};
            std::sort(order.begin(),order.end(),[&](int a,int b){return gap(a)<gap(b);});
            pick[static_cast<std::size_t>(k)]=order.front();
            for(int i:order){
                const auto& camera=cameras[static_cast<std::size_t>(i)];
                const V3 a{camera.base.x,0,camera.base.z},b{car.x,0,car.z};
                if(length(b-a)>400)break;
                bool clear=true;for(const auto& tree:trees)if(distanceToSegment({tree.p.x,0,tree.p.z},a,b)<4.5){clear=false;break;}
                if(clear){pick[static_cast<std::size_t>(k)]=i;break;}
            }
        }
        constexpr int shortest=static_cast<int>(45.0/5793.0*tvScheduleSteps)+1;
        for(int k=1;k<tvScheduleSteps;){
            int end=k;while(end<tvScheduleSteps&&pick[static_cast<std::size_t>(end)]==pick[static_cast<std::size_t>(k)])++end;
            if(end-k<shortest)for(int j=k;j<end;++j)pick[static_cast<std::size_t>(j)]=pick[static_cast<std::size_t>(k-1)];
            k=end;
        }
        return pick;
    }();
    return schedule;
}
inline const TvCamera& tvCameraFor(double progress){
    const double wrapped=progress-std::floor(progress);
    const int k=std::clamp(static_cast<int>(wrapped*tvScheduleSteps),0,tvScheduleSteps-1);
    return tvCameras()[static_cast<std::size_t>(tvSchedule()[static_cast<std::size_t>(k)])];
}
// A simple scaffold tower with a platform, operator box and lens.
inline void tvTowers(Mesh& mesh){
    constexpr Color steel{.55F,.57F,.57F},deck{.30F,.32F,.33F},body{.12F,.13F,.14F};
    for(const auto& camera:tvCameras()){
        const auto b=camera.base;
        for(double x:{-.6,.6})for(double z:{-.6,.6})tube(mesh,b+V3{x,0,z},b+V3{x,tvLensHeight-.6,z},.05,steel,4);
        for(double h=1.5;h<tvLensHeight-.6;h+=1.5){
            tube(mesh,b+V3{-.6,h,-.6},b+V3{.6,h,.6},.03,steel,3);tube(mesh,b+V3{-.6,h,.6},b+V3{.6,h,-.6},.03,steel,3);
        }
        box(mesh,b+V3{-.9,tvLensHeight-.6,-.9},b+V3{.9,tvLensHeight-.5,.9},deck);
        // Camera body and arm sit behind the lens (seen from the track), outside the view.
        const auto lens=camera.lens,back=lens+(V3{b.x,lens.y,b.z}-lens)*0.55;
        tube(mesh,V3{b.x,tvLensHeight-.5+b.y,b.z},back+V3{0,-.35,0},.04,steel,4);
        box(mesh,back+V3{-.25,-.45,-.25},back+V3{.25,-.05,.25},body);
    }
}
} // namespace apex3d::three
