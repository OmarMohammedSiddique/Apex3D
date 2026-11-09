#pragma once
#include "cp3_hud.hpp"
#include "scene3d.hpp"
namespace apex3d::three {
inline cp3::HudAction overlayAction(double x,double y,bool visible=true) {
    if(!visible||x<0.45)return cp3::HudAction::None;
    return cp3::hudActionAt(0.17+(x-0.45)*1.5,y);
}
inline std::vector<cp3::Vertex> overlay(const cp3::Playback& p,const cp3::RaceSample& sample,
    double pathLength,double aspect,bool audio,CameraMode camera,bool visible,cp3::HudAction hover) {
    auto source=p.isRace()?cp3::makeRaceHudMesh(p,sample,pathLength,aspect,audio,hover):
        cp3::makeHudMesh(p.progress,p.speed,p.playing,p.scaleDemo,p.route,p.showWheels,p.travelled,aspect,p.looping,
            p.playing?cp3::wheelRpm(pathLength*p.speed/cp3::lapSeconds):0.0);
    std::vector<cp3::Vertex> result;
    for(std::size_t i=0;i<source.size();i+=3) {
        const bool lights=source[i].x<0.17F;
        if(lights&&p.started&&p.elapsed>3.0)continue;
        if(!lights&&!visible)continue;
        for(std::size_t j=0;j<3;++j) {
            auto v=source[i+j];
            if(lights){v.x=static_cast<float>((v.x+0.565)*0.75+(visible?-0.285:0.0));v.y=static_cast<float>((v.y-0.775)*0.75+0.80);}
            else v.x=static_cast<float>(0.45+(v.x-0.17)*(2.0/3.0));
            result.push_back(v);
        }
    }
    cp3::hudRectangle(result,-0.985,-0.985,visible?0.42:0.985,-0.825,{0.035F,0.075F,0.11F});
    cp3::hudText(result,-0.95,-0.85,0.0040,std::string("3D / ")+cameraName(camera)+"   C SWITCH CAMERA",{0.94F,0.97F,0.97F});
    cp3::hudText(result,-0.95,-0.925,0.0035,"DRAG ORBIT  SCROLL ZOOM  F RESET  H DASHBOARD",{0.62F,0.73F,0.76F});
    return result;
}
} // namespace apex3d::three
