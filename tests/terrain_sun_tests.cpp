#include "../include/apex3d/hud3d.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>
#define CHECK(x) do{if(!(x))throw std::runtime_error("Check failed: " #x);}while(false)
namespace {
bool near(double a,double b,double e){return std::abs(a-b)<=e;}
}
int main(){using namespace apex3d;using namespace three;
try{
    const auto& ground=terrain();
    const double scale=physical::metresPerSceneUnit();
    const auto& controls=cp3::measuredMonzaControls;const auto& elevation=cp3::measuredMonzaElevation;
    // On the track the terrain is the recorded elevation, relative to the finish line.
    double worstTrack=0,worstCamber=0,low=1e9,high=-1e9;
    for(std::size_t i=0;i<controls.size();++i){
        const double x=controls[i].x*scale,z=-controls[i].y*scale,h=terrainHeight(x,z);
        worstTrack=std::max(worstTrack,std::abs(h-(elevation[i]-elevation[0])));
        low=std::min(low,h);high=std::max(high,h);
        // Across the 12 m road the surface stays nearly level (no invented banking).
        const auto& next=controls[(i+1)%controls.size()];
        const double dx=(next.x-controls[i].x)*scale,dz=-(next.y-controls[i].y)*scale,run=std::hypot(dx,dz);
        const double nx=-dz/run*6,nz=dx/run*6;
        worstCamber=std::max(worstCamber,std::abs(terrainHeight(x+nx,z+nz)-terrainHeight(x-nx,z-nz)));
    }
    CHECK(worstTrack<0.25);CHECK(worstCamber<0.10);
    CHECK(near(terrainHeight(controls[0].x*scale,-controls[0].y*scale),0.0,0.25));
    CHECK(high-low>11.5&&high-low<12.6); // the recorded 12.4 m range survives
    // Off the map the height is the lap mean; the map's border texels match it exactly.
    CHECK(terrainHeight(-40000,30000)==double(float(ground.mean))&&terrainHeight(40000,-30000)==double(float(ground.mean)));
    for(int c=0;c<ground.columns;++c)CHECK(ground.texel(c,0)==float(ground.mean)&&ground.texel(c,ground.rows-1)==float(ground.mean));
    for(int r=0;r<ground.rows;++r)CHECK(ground.texel(0,r)==float(ground.mean)&&ground.texel(ground.columns-1,r)==float(ground.mean));
    // Smooth, gently rolling ground: no step over 6 % between neighbouring texels.
    double steepest=0;
    for(int r=0;r+1<ground.rows;++r)for(int c=0;c+1<ground.columns;++c){
        steepest=std::max({steepest,std::abs(ground.texel(c+1,r)-ground.texel(c,r)),std::abs(ground.texel(c,r+1)-ground.texel(c,r))});
    }
    CHECK(steepest/ground.cell<0.06);
    // Bilinear lookup reproduces texels exactly and interpolates between them.
    CHECK(near(ground.height(ground.minX+5*ground.cell,ground.minZ+7*ground.cell),ground.texel(5,7),1e-6));
    CHECK(near(ground.height(ground.minX+5.5*ground.cell,ground.minZ+7*ground.cell),(ground.texel(5,7)+ground.texel(6,7))/2,1e-6));
    // The world mesh starts with the ground grid the shadow pass skips: all flat, facing up.
    const auto path=cp3::makePath();const cp3::RaceMotion race(path);
    const auto scene=makeWorld(path,&race);
    CHECK(static_cast<int>(scene.size())>groundVertexCount());
    for(int i=0;i<groundVertexCount();++i){const auto& v=scene[static_cast<std::size_t>(i)];CHECK(v.y==0.0F&&v.ny>0.999F);}
    // Cameras ride on the terrain: the chase target sits just above the car, wherever the car is.
    for(double t:{0.0,600.0,1500.0,2500.0}){
        const auto pose=race.sample(t).pose;
        CameraRig chase;chase.mode=CameraMode::Chase;chase.reset();
        const auto frame=chase.frame(pose,1.5,0,true);
        const auto car=world(pose.position,physical::roadHeight);
        CHECK(near(frame.target.y-terrainHeight(frame.target.x,frame.target.z),car.y+0.65,1e-6));
        CHECK(frame.eye.y>terrainHeight(frame.eye.x,frame.eye.z)+1.0);
    }

    // Compass: true north/east are horizontal, perpendicular and right-handed with up.
    const auto north=worldNorth(),east=worldEast();
    CHECK(near(north.y,0,1e-12)&&near(length(north),1,1e-9)&&near(dot(north,east),0,1e-9));
    CHECK(near(dot(cross(east,north),V3{0,1,0}),1,1e-9));
    // The main straight runs roughly north into Turn 1 (scene -x).
    CHECK(dot(north,V3{-1,0,0})>0.95);
    // Sun at the recorded race start (13:03:35 UTC, 7 Sep 2025, Monza), checked against
    // a hand calculation: declination 5.85 deg (14.8 deg of ecliptic longitude before the
    // 22 Sep 18:19 UTC equinox), equation of time +1.9 min, so the hour angle is 25.65 deg
    // and spherical trigonometry gives elevation 44.45 deg, azimuth 217.1 deg.
    const auto start=solarPosition(cp3::raceStartUnixSeconds,cp3::siteLatitude,cp3::siteLongitude);
    CHECK(near(start.elevationDegrees,44.45,0.3));CHECK(near(start.azimuthDegrees,217.1,0.5));
    // At the chequered flag (+74 min, hour angle 44.15 deg): 34.9 deg, 237.7 deg.
    const auto finish=solarPosition(cp3::raceStartUnixSeconds+74*60,cp3::siteLatitude,cp3::siteLongitude);
    CHECK(near(finish.elevationDegrees,34.9,0.3));CHECK(near(finish.azimuthDegrees,237.7,0.5));
    double previous=90;
    for(double t=0;t<=cp3::raceTotalSeconds+600;t+=60){
        const auto sun=sunDirection(t);
        CHECK(near(length(sun),1,1e-9));CHECK(sun.y>0.5&&sun.y<previous);previous=sun.y;
        CHECK(dot(sun,north)<0&&dot(sun,east)<0); // south-west quadrant
    }
    // Solar noon: the sun peaks at 90 - 45.62 + 5.85 = 50.23 deg, due south.
    double peak=0;
    for(double minutes=-180;minutes<=0;minutes+=1)peak=std::max(peak,solarPosition(cp3::raceStartUnixSeconds+minutes*60,cp3::siteLatitude,cp3::siteLongitude).elevationDegrees);
    CHECK(near(peak,50.23,0.2));
    // Late-race light: neutral at the start, then slightly warmer as the air mass grows.
    const auto first=sunlightTint(0);CHECK(near(first.x,1,1e-12)&&near(first.y,1,1e-12)&&near(first.z,1,1e-12));
    CHECK(near(airMass(90),1.0,0.001)&&near(airMass(30),1.995,0.01)); // Kasten-Young reference values
    const auto flag=sunlightTint(74*60);
    CHECK(flag.z<flag.y&&flag.y<flag.x&&flag.x<1.0&&flag.z>0.9); // bluer light is lost first; a subtle change
    std::cout<<"Terrain and sun: measured elevation, smooth ground, camera lift, solar position: PASS\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
