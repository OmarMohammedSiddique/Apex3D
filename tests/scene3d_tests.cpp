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
    CHECK(highest>0.15F&&lowest<=0); // raised geometry above the flat terrain
    const cp3::Pose straight{{0,0},0,0};
    const auto car0=makeCar(straight,0),car90=makeCar(straight,0.25);
    finiteMesh(car0);finiteMesh(car90);CHECK(car0.size()==car90.size());
    float tyreBottom=1;for(const auto& v:car0)tyreBottom=std::min(tyreBottom,v.y);
    CHECK(near(tyreBottom,physical::roadHeight));
    bool spokeMoved=false;
    for(std::size_t i=0;i<car0.size();++i){
        if(i<90)CHECK(near(car0[i].x,car90[i].x)&&near(car0[i].y,car90[i].y)&&near(car0[i].z,car90[i].z));
        if(!near(car0[i].x,car90[i].x)||!near(car0[i].y,car90[i].y))spokeMoved=true;
    }
    CHECK(spokeMoved);
    // Metre-space relationships prevent independently exaggerated objects.
    double lapMetres=0;
    for(std::size_t i=1;i<path.size();++i)lapMetres+=length(world(path[i].point)-world(path[i-1].point));
    CHECK(std::abs(lapMetres-cp3::circuitMetres)<0.001);
    const auto bounds=[](const Mesh& mesh){
        std::array<double,6> b{1e9,1e9,1e9,-1e9,-1e9,-1e9};
        for(const auto& v:mesh){b[0]=std::min(b[0],double(v.x));b[1]=std::min(b[1],double(v.y));b[2]=std::min(b[2],double(v.z));
            b[3]=std::max(b[3],double(v.x));b[4]=std::max(b[4],double(v.y));b[5]=std::max(b[5],double(v.z));}return b;
    };
    const auto cb=bounds(car0);
    CHECK(std::abs(cb[3]-cb[0]-5.652)<0.001);
    CHECK(std::abs(cb[5]-cb[2]-physical::carWidth)<0.001);
    CHECK(cb[4]-physical::roadHeight<1.0);
    Mesh road;roadStrip(road,straight,{{10/physical::metresPerSceneUnit(),0},0,0},-6,6,physical::roadHeight,{1,1,1});
    const auto rb=bounds(road);CHECK(near(rb[5]-rb[2],12));
    for(const auto& v:road)CHECK(v.ny>0.99);
    CHECK(near((rb[5]-rb[2])/(cb[5]-cb[2]),6));
    for(double width:{physical::frontTyreWidth,physical::rearTyreWidth}) {
        Mesh t;tyre(t,{0,physical::tyreRadius,0},0,false,width);const auto tb=bounds(t);
        CHECK(near(tb[4]-tb[1],0.720));CHECK(near(tb[3]-tb[0],0.720));
        CHECK(std::abs(tb[5]-tb[2]-width)<0.012);
    }
    const auto crewBounds=bounds(makeCrew(straight,true));
    CHECK(std::abs(crewBounds[4]-physical::roadHeight-physical::crewHeight)<0.001);
    const auto box=race.pitPose(cp3::pitBoxProgress),trackPose=cp3::poseAtProgress(path,cp3::pitBoxProgress);
    CHECK(std::abs(length(world(box.position)-world(trackPose.position))-(std::abs(cp3::pitLaneOffset(cp3::pitBoxProgress))*physical::metresPerSceneUnit()+physical::pitBayOffset))<0.001);
    // The measured main straight is straight to within a fraction of a degree.
    CHECK(std::abs(metricSteering(path,0.02))<0.002);
    const auto steered=makeCar(straight,0,false,0.25);
    finiteMesh(steered);CHECK(steered.size()==car0.size());
    bool frontMoved=false;
    for(std::size_t i=0;i<car0.size();++i){
        // Steering transforms only the front tyres; rear axle and body stay fixed.
        if(car0[i].x<0)CHECK(near(car0[i].x,steered[i].x)&&near(car0[i].z,steered[i].z));
        if(!near(car0[i].x,steered[i].x)||!near(car0[i].z,steered[i].z))frontMoved=true;
    }
    CHECK(frontMoved);
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
        CHECK(frame.eye.y>terrainHeight(frame.eye.x,frame.eye.z)+0.5);
        for(auto value:frame.viewProjection.values)CHECK(std::isfinite(value));
    }
    CameraRig following;following.mode=CameraMode::Chase;following.reset();
    const auto f0=following.frame(straight,1.3,0,true);
    const cp3::Pose translated{{100/physical::metresPerSceneUnit(),0},0,0};
    const auto f1=following.frame(translated,1.3,1.0/60);
    CHECK(near(f1.eye.x-f0.eye.x,100));CHECK(near(f1.target.x-f0.target.x,100));
    CameraRig camera;camera.orbit(500,-5000);CHECK(camera.pitch>=0.18);camera.dolly(10000);CHECK(camera.zoom>=0.008&&camera.zoom<0.01);
    const auto zoomFocus=camera.frame(race.sample(120).pose,1.3,0,true);
    CHECK(length(zoomFocus.target-onTerrain(world(race.sample(120).pose.position)))<0.01);
    // Regression: the chase eye sits metres above the 100 km ground quad. With a 5 cm near plane the
    // ground won the depth test over the 10 cm-higher road. Keep road and ground separable in a
    // 24-bit depth buffer down the straight, without the near plane cutting into the car.
    double tightestSteps=1e9;
    for(double zoom:{0.18,0.5,1.0,2.0,3.0})for(double pitch:{0.18,0.48,1.40}){
        CameraRig chase;chase.mode=CameraMode::Chase;chase.reset();chase.zoom=zoom;chase.pitch=pitch;
        const auto frame=chase.frame(straight,1.3,0,true);
        CHECK(frame.nearPlane>=chaseNearPlane&&frame.nearPlane<0.25*length(frame.target-frame.eye));
        const auto depth=[&](V3 p){const auto c=frame.viewProjection.transform(p);CHECK(c[3]>0);return 0.5*(c[2]/c[3]+1.0);};
        for(double ahead:{50.0,150.0,250.0}){const V3 ground{frame.target.x+ahead,0,frame.target.z};
            const double steps=(depth(ground)-depth(ground+V3{0,physical::roadHeight,0}))*16777216.0;
            tightestSteps=std::min(tightestSteps,steps);}
    }
    std::cout<<"Tightest chase road/ground depth separation: "<<tightestSteps<<" depth-buffer steps\n";
    CHECK(tightestSteps>=1.0);
    CameraRig cockpit;cockpit.mode=CameraMode::Cockpit;cockpit.reset();
    const auto wide=cockpit.frame(straight,1.3,0,true).viewProjection;
    cockpit.dolly(3);
    const auto zoomed=cockpit.frame(straight,1.3,0,true).viewProjection;
    CHECK(zoomed.values!=wide.values);
    CHECK(monzaStands.size()==37);
    for(const auto& stand:monzaStands) {
        const auto front=standFront(path,stand);const auto pose=standPose(path,stand);
        CHECK(std::abs(length(front-world(pose.position))-stand.setback)<0.01);
        CHECK(stand.setback>=16&&stand.length>0&&stand.rows>0);
        // Independent footprint-to-road distance: no stand occupies the racing surface.
        for(int x=0;x<9;++x)for(int z=0;z<3;++z) {
            const auto point=front+rotateY({-stand.length/2+stand.length*x/8,0,stand.rows*0.8*z/2},
                pose.headingRadians+(stand.side<0?std::numbers::pi:0));
            double nearest=1e9;
            for(std::size_t j=0;j+1<path.size();++j){
                const auto a=world(path[j].point),b=world(path[j+1].point),d=b-a;
                const double t=std::clamp(dot(point-a,d)/dot(d,d),0.0,1.0);
                nearest=std::min(nearest,length(point-(a+d*t)));
            }
            CHECK(nearest>physical::trackWidth/2+physical::kerbWidth);
        }
    }
    CameraRig map;map.mode=CameraMode::Overview;map.reset();
    const auto mapFrame=map.frame(straight,1.3,0,true);
    const auto marker=markerPosition(mapFrame,straight,0.715);
    CHECK(marker.onScreen&&marker.x<0.43);
    std::vector<cp3::Vertex> markers;overviewMarker(markers,mapFrame,straight,0.715,1.8,"VER 1",{1,1,1},true);
    CHECK(!markers.empty());
    for(const auto& v:markers)CHECK(std::abs(v.x)<=1&&std::abs(v.y)<=1);
    CHECK(overlayAction(0.45+(0.3-0.17)*2/3,-0.3)==cp3::HudAction::Play);
    CHECK(overlayAction(0.45+(0.7-0.17)*2/3,-0.3)==cp3::HudAction::Mute);
    CHECK(overlayAction(0.1,-0.3)==cp3::HudAction::None);
    CHECK(overlayAction(0.8,-0.3,false)==cp3::HudAction::None);
    CHECK(overlayAction(0.70,-0.83)==cp3::HudAction::FocusVerstappen);
    CHECK(overlayAction(0.70,-0.885)==cp3::HudAction::FocusNorris);
    CHECK(overlayAction(0.70,-0.94)==cp3::HudAction::FocusPiastri);
    CHECK(overlayAction(0.70,-0.862)==cp3::HudAction::None);
    CHECK(overlayAction(0.70,-0.94,false)==cp3::HudAction::None);
    CHECK(overlayAction(0.96,-0.94)==cp3::HudAction::None);
    const auto minGauge=gaugeLevels({true,0,0,0,0}),maxGauge=gaugeLevels({true,400,8,15000,0});
    CHECK(minGauge.speed==0&&minGauge.rpm==0&&minGauge.gear==0);
    CHECK(maxGauge.speed==1&&maxGauge.rpm==1&&maxGauge.gear==8);
    const auto midGauge=gaugeLevels({true,200,4,7500,0});CHECK(midGauge.speed==0.5&&midGauge.rpm==0.5&&midGauge.gear==4);
    const auto capped=gaugeLevels({true,450,9,18000,0});CHECK(capped.speed==1&&capped.rpm==1&&capped.gear==8);
    cp3::Playback playback;
    for(bool visible:{true,false})for(double t:{0.0,cp3::pitBoxStartSeconds+1,cp3::raceTotalSeconds}){
        playback.seek(t);playback.showWheels=true;
        const auto mesh=overlay(playback,race.sample(t),path.back().distance,16.0/9.0,true,CameraMode::Overview,visible,cp3::HudAction::None);
        for(const auto& v:mesh){CHECK(std::isfinite(v.x)&&std::isfinite(v.y));CHECK(v.x>=-1&&v.x<=1&&v.y>=-1&&v.y<=1);}
    }
    const cp3::OpponentMotion norris(path,race,cp3::norrisSpec),piastri(path,race,cp3::piastriSpec);
    for(const auto& driver:cp3::podiumDrivers)for(double t:{0.0,120.0,driver.boxStart+0.5,cp3::raceTotalSeconds}) {
        playback.seek(t);const auto sample=cp3::focusedSample(driver.focus,race.sample(t),norris.sample(t),piastri.sample(t));
        CameraRig selected;selected.mode=CameraMode::Chase;selected.reset();
        const auto frame=selected.frame(sample.pose,1.3,0,true);
        CHECK(length(frame.target-onTerrain(world(sample.pose.position)))<3.0);
        for(double aspect:{0.8,16.0/9.0,3.2}) {
            const auto mesh=overlay(playback,sample,path.back().distance,aspect,true,CameraMode::Chase,true,cp3::HudAction::None,driver.focus);
            for(const auto& v:mesh){CHECK(std::isfinite(v.x)&&std::isfinite(v.y));CHECK(std::abs(v.x)<=1&&std::abs(v.y)<=1);}
        }
    }
    std::cout<<"3D: projection, depth geometry, normals, wheel spin, cameras and HUD hit testing: PASS\n";
}
