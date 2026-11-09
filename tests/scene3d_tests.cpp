#include "../include/apex3d/hud3d.hpp"
#include <iostream>
#include <stdexcept>
#define CHECK(x) do {if(!(x))throw std::runtime_error("Check failed: " #x);}while(false)
int main(){
    using namespace apex3d;
    using namespace apex3d::three;
    const auto near=[](double a,double b){return std::abs(a-b)<1e-5;};
    CHECK(near(length(unit({1,2,3})),1.0));
    CHECK(near(dot(cross({1,0,0},{0,1,0}),{0,0,1}),1.0));
    const auto identity=M4::identity();const auto p=identity.transform({2,3,4});
    CHECK(p[0]==2&&p[1]==3&&p[2]==4&&p[3]==1);
    const auto view=lookAt({0,0,5},{0,0,0});const auto origin=view.transform({0,0,0});
    CHECK(near(origin[2],-5));
    const auto projection=perspective(std::numbers::pi/2,1,0.1,10);
    const auto nearClip=projection.transform({0,0,-0.1}),farClip=projection.transform({0,0,-10});
    CHECK(near(nearClip[2]/nearClip[3],-1));CHECK(near(farClip[2]/farClip[3],1));
    bool rejected=false;try{static_cast<void>(perspective(1,0,0.1,10));}catch(const std::invalid_argument&){rejected=true;}CHECK(rejected);
    const auto path=cp3::makePath();const cp3::RaceMotion race(path);
    const auto finiteMesh=[](const Mesh& mesh){
        CHECK(!mesh.empty()&&mesh.size()%3==0);
        for(const auto& v:mesh){
            CHECK(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z));
            CHECK(std::isfinite(v.nx)&&std::isfinite(v.ny)&&std::isfinite(v.nz));
            CHECK(std::abs(std::sqrt(v.nx*v.nx+v.ny*v.ny+v.nz*v.nz)-1)<1e-4);
        }
    };
    const auto scene=makeWorld(path,&race);finiteMesh(scene);
    float highest=-1,lowest=1;
    for(const auto& v:scene){highest=std::max(highest,v.y);lowest=std::min(lowest,v.y);}
    CHECK(highest>0.15F&&lowest<0); // genuine vertical volume, not a tilted 2D plane
    const cp3::Pose straight{{0,0},0,0};
    const auto car0=makeCar(straight,0),car90=makeCar(straight,0.25);
    finiteMesh(car0);finiteMesh(car90);CHECK(car0.size()==car90.size());
    float tyreBottom=1;for(const auto& v:car0)tyreBottom=std::min(tyreBottom,v.y);
    CHECK(near(tyreBottom,0.016));
    bool spokeMoved=false;
    for(std::size_t i=0;i<car0.size();++i){
        if(i<90)CHECK(near(car0[i].x,car90[i].x)&&near(car0[i].y,car90[i].y)&&near(car0[i].z,car90[i].z));
        if(!near(car0[i].x,car90[i].x)||!near(car0[i].y,car90[i].y))spokeMoved=true;
    }
    CHECK(spokeMoved);
    Mesh round; sphere(round,{0,0,0},{1,1,1},{1,1,1});
    for(const auto& v:round)CHECK(v.x*v.nx+v.y*v.ny+v.z*v.nz>0.5); // outward sphere normals
    finiteMesh(makeCrew(straight,true));CHECK(makeCrew(straight,false).empty());
    finiteMesh(gantry(path,5,false));finiteMesh(gantry(path,0,true));
    for(auto mode:{CameraMode::Overview,CameraMode::Chase,CameraMode::Cockpit})for(double t:{0.0,87.159,cp3::pitBoxStartSeconds+1,cp3::raceTotalSeconds}){
        CameraRig camera;camera.mode=mode;camera.reset();
        const auto sample=race.sample(t);
        const auto frame=camera.frame(sample.pose,1.3,0,true);
        const auto target=frame.viewProjection.transform(frame.target);
        CHECK(target[3]>0&&near(target[0]/target[3],0)&&near(target[1]/target[3],0));
        CHECK(frame.eye.y>0);
        for(auto value:frame.viewProjection.values)CHECK(std::isfinite(value));
    }
    CameraRig camera;camera.orbit(500,-5000);CHECK(camera.pitch>=0.18);camera.dolly(10000);CHECK(camera.zoom>=0.42);
    CameraRig cockpit;cockpit.mode=CameraMode::Cockpit;cockpit.reset();
    const auto wide=cockpit.frame(straight,1.3,0,true).viewProjection;
    cockpit.dolly(3);
    const auto zoomed=cockpit.frame(straight,1.3,0,true).viewProjection;
    CHECK(zoomed.values!=wide.values);
    CHECK(overlayAction(0.45+(0.3-0.17)*2/3,-0.3)==cp3::HudAction::Play);
    CHECK(overlayAction(0.45+(0.7-0.17)*2/3,-0.3)==cp3::HudAction::Mute);
    CHECK(overlayAction(0.1,-0.3)==cp3::HudAction::None);
    CHECK(overlayAction(0.8,-0.3,false)==cp3::HudAction::None);
    cp3::Playback playback;
    for(bool visible:{true,false})for(double t:{0.0,cp3::pitBoxStartSeconds+1,cp3::raceTotalSeconds}){
        playback.seek(t);playback.showWheels=true;
        const auto mesh=overlay(playback,race.sample(t),path.back().distance,16.0/9.0,true,CameraMode::Overview,visible,cp3::HudAction::None);
        for(const auto& v:mesh){CHECK(std::isfinite(v.x)&&std::isfinite(v.y));CHECK(v.x>=-1&&v.x<=1&&v.y>=-1&&v.y<=1);}
    }
    std::cout<<"3D: projection, depth geometry, normals, wheel spin, cameras and HUD hit testing: PASS\n";
}
