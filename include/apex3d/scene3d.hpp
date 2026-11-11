#pragma once
#include "math3d.hpp"
#include "cp3_race.hpp"
#include "generated/monza_stands_2025.hpp"
#include "generated/monza_entries_2025.hpp"
#include "terrain.hpp"
#include "sun_position.hpp"
#include <vector>

namespace apex3d::three {
using cp3::Color;
struct Vertex {
    float x,y,z,nx,ny,nz,r,g,b;
};
using Mesh=std::vector<Vertex>;
inline V3 world(Vec2 p,double height=0) { return {p.x*physical::metresPerSceneUnit(),height,-p.y*physical::metresPerSceneUnit()}; }
inline void vertex(Mesh& mesh,V3 p,V3 n,Color color) {
    mesh.push_back({static_cast<float>(p.x),static_cast<float>(p.y),static_cast<float>(p.z),
        static_cast<float>(n.x),static_cast<float>(n.y),static_cast<float>(n.z),color.r,color.g,color.b});
}
inline void triangle(Mesh& mesh,V3 a,V3 b,V3 c,Color color) {
    const auto normal=cross(b-a,c-a);
    if(length(normal)<1e-12)return;
    const auto n=unit(normal); vertex(mesh,a,n,color);vertex(mesh,b,n,color);vertex(mesh,c,n,color);
}
inline void quad(Mesh& mesh,V3 a,V3 b,V3 c,V3 d,Color color) {
    triangle(mesh,a,b,c,color);triangle(mesh,a,c,d,color);
}
inline void box(Mesh& m,V3 low,V3 high,Color c) {
    quad(m,{low.x,high.y,low.z},{low.x,high.y,high.z},{high.x,high.y,high.z},{high.x,high.y,low.z},c);
    quad(m,{low.x,low.y,high.z},{low.x,low.y,low.z},{high.x,low.y,low.z},{high.x,low.y,high.z},c);
    quad(m,{low.x,low.y,high.z},{high.x,low.y,high.z},{high.x,high.y,high.z},{low.x,high.y,high.z},c);
    quad(m,{high.x,low.y,low.z},{low.x,low.y,low.z},{low.x,high.y,low.z},{high.x,high.y,low.z},c);
    quad(m,{low.x,low.y,low.z},{low.x,low.y,high.z},{low.x,high.y,high.z},{low.x,high.y,low.z},c);
    quad(m,{high.x,low.y,high.z},{high.x,low.y,low.z},{high.x,high.y,low.z},{high.x,high.y,high.z},c);
}
inline void tube(Mesh& mesh,V3 a,V3 b,double radius,Color c,int sides=10) {
    const V3 axis=unit(b-a);
    const V3 u=unit(cross(axis,std::abs(axis.y)<0.9 ? V3{0,1,0}:V3{1,0,0}));
    const V3 v=cross(axis,u);
    for(int i=0;i<sides;++i) {
        const double p=2*std::numbers::pi*i/sides,q=2*std::numbers::pi*(i+1)/sides;
        const V3 n0=u*std::cos(p)+v*std::sin(p),n1=u*std::cos(q)+v*std::sin(q);
        const V3 a0=a+n0*radius,a1=a+n1*radius,b0=b+n0*radius,b1=b+n1*radius;
        vertex(mesh,a0,n0,c);vertex(mesh,b0,n0,c);vertex(mesh,b1,n1,c);
        vertex(mesh,a0,n0,c);vertex(mesh,b1,n1,c);vertex(mesh,a1,n1,c);
        vertex(mesh,a,axis*(-1),c);vertex(mesh,a1,axis*(-1),c);vertex(mesh,a0,axis*(-1),c);
        vertex(mesh,b,axis,c);vertex(mesh,b0,axis,c);vertex(mesh,b1,axis,c);
    }
}
inline void sphere(Mesh& mesh,V3 centre,V3 radii,Color c,int rings=8,int sides=12) {
    const auto point=[&](double latitude,double longitude) {
        return V3{radii.x*std::cos(latitude)*std::cos(longitude),radii.y*std::sin(latitude),radii.z*std::cos(latitude)*std::sin(longitude)}+centre;
    };
    for(int r=0;r<rings;++r)for(int s=0;s<sides;++s) {
        const double a=-std::numbers::pi/2+std::numbers::pi*r/rings,b=a+std::numbers::pi/rings;
        const double u=2*std::numbers::pi*s/sides,v=u+2*std::numbers::pi/sides;
        quad(mesh,point(a,u),point(b,u),point(b,v),point(a,v),c);
    }
}
inline void cone(Mesh& mesh,V3 centre,double radius,double height,Color c) {
    for(int i=0;i<10;++i) {
        const double a=2*std::numbers::pi*i/10,b=2*std::numbers::pi*(i+1)/10;
        triangle(mesh,centre+V3{radius*std::cos(b),0,radius*std::sin(b)},centre+V3{radius*std::cos(a),0,radius*std::sin(a)},centre+V3{0,height,0},c);
    }
}
inline V3 rotateY(V3 p,double yaw) {
    return {std::cos(yaw)*p.x+std::sin(yaw)*p.z,p.y,-std::sin(yaw)*p.x+std::cos(yaw)*p.z};
}
inline void place(Mesh& mesh,std::size_t begin,V3 centre,double yaw) {
    for(std::size_t i=begin;i<mesh.size();++i) {
        auto& v=mesh[i];const auto p=rotateY({v.x,v.y,v.z},yaw)+centre;
        const auto n=rotateY({v.nx,v.ny,v.nz},yaw);
        v.x=static_cast<float>(p.x);v.y=static_cast<float>(p.y);v.z=static_cast<float>(p.z);
        v.nx=static_cast<float>(n.x);v.ny=static_cast<float>(n.y);v.nz=static_cast<float>(n.z);
    }
}
// All geometry below is measured in metres; CP2 coordinates enter through world().
inline void roadStrip(Mesh& mesh,const cp3::Pose& a,const cp3::Pose& b,
                      double left,double right,double height,Color color) {
    const auto edge=[](const cp3::Pose& p,double offset,double h) {
        return world(p.position,h)+rotateY({0,0,offset},p.headingRadians);
    };
    if(left>right)std::swap(left,right);
    quad(mesh,edge(a,left,height),edge(a,right,height),edge(b,right,height),edge(b,left,height),color);
}
} // namespace apex3d::three
#include "world_detail.hpp"
#include "gravel_birds.hpp"
#include "tv_camera.hpp"
namespace apex3d::three {
inline void drsZones(Mesh& mesh,const std::vector<cp3::PathSample>& path){
    const double t7=cp3::turn7Fraction,t8=cp3::turn8Fraction,t1=cp3::turn1Fraction;
    const std::array<std::pair<double,double>,2> zones{{{t7+170/cp3::circuitMetres,t8-70/cp3::circuitMetres},{12/cp3::circuitMetres,t1-70/cp3::circuitMetres}}};
    for(const auto& zone:zones){
        // On the main straight the pit lane and apron lie on the +z side: mark only the grandstand side.
        const bool mainStraight=zone.first<.05;
        const auto sides=[&]{return mainStraight?std::vector<double>{-1.}:std::vector<double>{-1.,1.};}();
        // Bright cyan verge bands on both sides mark each DRS zone. They are a
        // visualization overlay; Monza's real verges are not painted cyan.
        constexpr Color cyan{.05F,.95F,.85F};
        for(double p=zone.first;p<zone.second;p+=6/cp3::circuitMetres){const auto a=cp3::poseAtProgress(path,p),b=cp3::poseAtProgress(path,std::min(zone.second,p+6/cp3::circuitMetres));
            for(double side:sides)roadStrip(mesh,a,b,side*7.4,side*8.6,.03,cyan);}
        // Every 40 m a chevron on the verge points the way, readable from the chase camera.
        for(double p=zone.first+20/cp3::circuitMetres;p<zone.second;p+=40/cp3::circuitMetres){const auto pose=cp3::poseAtProgress(path,p);
            for(double side:sides){const auto start=mesh.size();
                triangle(mesh,{1.6,.035,side*8.0},{-.8,.035,side*8.0-1.0},{-.8,.035,side*8.0+1.0},cyan);
                place(mesh,start,world(pose.position),pose.headingRadians);}}
        const auto pose=cp3::poseAtProgress(path,zone.first);const auto start=mesh.size();
        // Activation line across the track, as on the real circuit, plus a tall board each side.
        box(mesh,{-.15,.111,-6},{.15,.116,6},{.95F,.97F,.97F});
        for(double side:sides){const double z=side*11;
            box(mesh,{-.10,0,z-.10},{.10,3.2,z+.10},{.48F,.51F,.51F});
            box(mesh,{-.08,2.0,z-1.5},{.08,3.6,z+1.5},{.025F,.045F,.055F});
            box(mesh,{-.09,2.08,z-1.42},{-.085,3.52,z+1.42},cyan);
            surfaceText(mesh,{-.10,2.30,z-.95},{0,0,1},{0,1,0},.18,"DRS",{.02F,.05F,.06F});
            surfaceText(mesh,{.10,2.30,z+.95},{0,0,-1},{0,1,0},.18,"DRS",{1,1,1});}
        place(mesh,start,world(pose.position),pose.headingRadians);
    }
}
inline void parcFermeScene(Mesh& mesh,const std::vector<cp3::PathSample>& path){
    const auto start=mesh.size();
    box(mesh,{-14,-.015,7},{22,.01,65},{.32F,.35F,.36F});
    box(mesh,{20,-.015,45},{145,.01,70},{.32F,.35F,.36F});
    // Race control / weighing facilities and a podium bridge at the pit-wall end.
    box(mesh,{-30,0,24},{-13,11,40},{.80F,.79F,.73F});
    box(mesh,{-30.2,8.0,23.8},{-12.8,10.2,40.2},{.21F,.32F,.37F});
    box(mesh,{-31,11,23},{-12,11.5,41},{.66F,.67F,.63F});
    box(mesh,{-27,0,10},{-20,4.5,24},{.70F,.71F,.69F});
    box(mesh,{-29,4.5,5},{-18,5,25},{.40F,.43F,.44F});
    surfaceText(mesh,{-12.9,6.2,26},{0,0,1},{0,1,0},.14,"CONTROL",{.05F,.10F,.12F});
    for(int rank=1;rank<=3;++rank){const double along=rank==1?6:8,lateral=rank==1?18:rank==2?22:14;
        const Color c=rank==1?Color{.94F,.72F,.21F}:rank==2?Color{.72F,.77F,.79F}:Color{.74F,.43F,.25F};
        box(mesh,{along+3.7,0,lateral-.65},{along+3.9,1.35,lateral+.65},{.05F,.07F,.08F});
        surfaceText(mesh,{along+3.69,.23,lateral-.29},{0,0,1},{0,1,0},.12,std::to_string(rank),c);
        for(double side:{-1.,1.})box(mesh,{along-3,.015,lateral+side*1.45-.04},{along+3,.025,lateral+side*1.45+.04},c);
    }
    const auto origin=cp3::poseAtProgress(path,0);place(mesh,start,world(origin.position,physical::roadHeight),origin.headingRadians);
}
inline void garageBlock(Mesh& mesh,V3 p,Color team) {
    // Two representative 6 m bays per team, with a 15 m deep building.
    box(mesh,p+V3{-6,0,-21},p+V3{6,physical::garageHeight,-6},{0.51F,0.54F,0.55F});
    box(mesh,p+V3{-6.3,6,-21.3},p+V3{6.3,6.3,-5.7},{0.24F,0.28F,0.31F});
    box(mesh,p+V3{-6,-0.10,-6},p+V3{6,-0.001,5.5},{0.31F,0.34F,0.35F});
    for(int i=0;i<2;++i) {
        const double x=-5.5+i*6;
        box(mesh,p+V3{x,0,-5.98},p+V3{x+5,3.5,-5.90},{0.055F,0.075F,0.085F});
        box(mesh,p+V3{x,4.0,-5.97},p+V3{x+5,4.25,-5.88},team);
    }
    // Service box: 7 m long, 3 m wide, 10 cm paint.
    for(double side:{-1.0,1.0}) {
        box(mesh,p+V3{-3.5,0.025,side*1.5-0.05},p+V3{3.5,0.03,side*1.5+0.05},{0.74F,0.64F,0.35F});
        box(mesh,p+V3{side*3.5-0.05,0.025,-1.5},p+V3{side*3.5+0.05,0.03,1.5},{0.74F,0.64F,0.35F});
    }
}
inline void garage(Mesh& mesh,const cp3::RaceMotion& race) {
    std::vector<std::string_view> teams;
    for(const auto& entry:cp3::archivedEntries){
        if(std::find(teams.begin(),teams.end(),entry.team)!=teams.end())continue;
        teams.push_back(entry.team);double metres=0;int count=0;
        for(const auto& teammate:cp3::archivedEntries)if(std::string_view(teammate.team)==entry.team)
            for(const auto& pit:cp3::archivedPits)if(pit.driver==teammate.number){metres+=pit.boxMetres;++count;}
        if(!count)continue;
        const double progress=37+metres/count/cp3::circuitMetres;
        garageBlock(mesh,world(race.pitPose(progress,true,progress).position,physical::roadHeight),entry.color);
    }
    const auto mainPath=cp3::makePath();
    // Pit wall 3 m in from the lane centre, wherever the lane is fully separated from the track.
    const double wallFrom=physical::sceneUnits(10.5);
    for(int i=0;i<320;++i) {
        const double a=cp3::pitLaneStartProgress+(cp3::pitLaneEndProgress-cp3::pitLaneStartProgress)*i/320;
        const double b=cp3::pitLaneStartProgress+(cp3::pitLaneEndProgress-cp3::pitLaneStartProgress)*(i+1)/320;
        const double la=cp3::pitLaneOffset(a),lb=cp3::pitLaneOffset(b);
        if(std::abs(la)<wallFrom||std::abs(lb)<wallFrom)continue;
        const auto edge=[&](double p,double lateral){
            const auto pose=cp3::poseAtProgress(mainPath,p);const double l=lateral+physical::sceneUnits(3.0);
            return world({pose.position.x-std::sin(pose.headingRadians)*l,pose.position.y+std::cos(pose.headingRadians)*l},0.70);};
        tube(mesh,edge(a,la),edge(b,lb),0.20,{0.68F,0.69F,0.65F},4);
    }
}
inline cp3::Pose standPose(const std::vector<cp3::PathSample>& path,const StandSpec& stand) {
    const auto nearest=std::min_element(path.begin(),path.end(),[&](const auto& a,const auto& b){
        return std::hypot(a.point.x-stand.x,a.point.y-stand.y)<std::hypot(b.point.x-stand.x,b.point.y-stand.y);});
    return cp3::poseAtProgress(path,nearest->distance/path.back().distance);
}
inline V3 standFront(const std::vector<cp3::PathSample>& path,const StandSpec& stand) {
    const auto pose=standPose(path,stand);
    return world(pose.position)+rotateY({0,0,stand.side*stand.setback},pose.headingRadians);
}
inline void grandstands(Mesh& m,const std::vector<cp3::PathSample>& path) {
    for(const auto& stand:monzaStands) {
        const auto pose=standPose(path,stand);const auto front=standFront(path,stand);
        const auto begin=m.size();const double half=stand.length/2;
        const int seats=static_cast<int>(stand.length/0.6);
        for(int row=0;row<stand.rows;++row) {
            const double z=row*0.8,h=0.35+row*0.4;
            box(m,{-half,0,z},{half,h,z+0.8},{0.53F,0.56F,0.55F});
            for(int seat=0;seat<seats;++seat) {
                const double x=-half+seat*0.6;
                box(m,{x,h,z+0.1},{x+0.45,h+0.10,z+0.55},
                    (row+seat)%3?Color{0.75F,0.74F,0.66F}:Color{0.20F,0.32F,0.45F});
                // Low-poly seated spectators; deterministic and correctly sized.
                if((seat+row)%4==0) {
                    const Color shirt=(seat+row)%3?Color{0.78F,0.10F,0.12F}:Color{0.10F,0.20F,0.37F};
                    box(m,{x+0.06,h+0.15,z+0.22},{x+0.40,h+0.75,z+0.48},shirt);
                    box(m,{x+0.12,h+0.75,z+0.26},{x+0.34,h+1.0,z+0.46},{0.68F,0.47F,0.33F});
                }
            }
        }
        const double back=stand.rows*0.8,height=stand.rows*0.4+2.2;
        if(stand.covered) {
            box(m,{-half-0.5,height,-0.5},{half+0.5,height+0.20,back+0.5},{0.72F,0.73F,0.69F});
            for(double x:{-half+0.2,half-0.2})for(double z:{0.0,back})
                tube(m,{x,0,z},{x,height,z},0.1,{0.40F,0.44F,0.45F});
        }
        place(m,begin,front,pose.headingRadians+(stand.side<0?std::numbers::pi:0));
    }
}
} // namespace apex3d::three
#include "monza_landmarks.hpp"
#include "celebration.hpp"
namespace apex3d::three {
inline Mesh gantry(const std::vector<cp3::PathSample>& path,int lights,bool frame) {
    Mesh mesh;const auto pose=cp3::poseAtProgress(path,cp3::gridOffset+20.0/cp3::circuitMetres);
    constexpr Color steel{0.26F,0.29F,0.31F};
    if(frame) {
        for(double z:{-7.5,7.5})box(mesh,{-0.18,0,z-0.18},{0.18,6.5,z+0.18},steel);
        box(mesh,{-0.25,5.5,-7.7},{0.25,6.5,7.7},steel);
        box(mesh,{-0.32,5.45,-1.5},{-0.26,6.35,1.5},{0.06F,0.07F,0.08F});
    } else {
        for(int i=0;i<5;++i)for(double h:{5.70,6.10})
            sphere(mesh,{-0.34,h,-1.12+i*0.56},{0.05,0.15,0.15},
                i<lights?Color{1.0F,0.025F,0.02F}:Color{0.13F,0.035F,0.035F},6,12);
    }
    place(mesh,0,world(pose.position,physical::roadHeight),pose.headingRadians);return mesh;
}
// The ground comes first in the world mesh. It receives shadows but cannot cast
// one, so the shadow pass skips these vertices (avoids self-shadow acne).
inline int groundVertexCount(){const auto& t=terrain();return ((t.columns-1)*(t.rows-1)+4)*6;}
// Where Monza has kerbs: through corners, not along the straights. OSM doesn't map
// circuit kerbs, so they are placed from the measured line's curvature (smoothed
// over +/-25 m). Every corner tighter than a 400 m radius gets an inside kerb, from
// 10 m before to 10 m after; corners tighter than 150 m (the chicanes, Lesmos,
// Ascari and Parabolica) also get an outside kerb from mid-corner to 25 m past the
// exit. That picks out exactly Monza's 11 turns. Kerb lengths are representative.
// No kerb on the pit side where the measured pit lane leaves or rejoins the track.
struct KerbSides { bool left{}, right{}; }; // relative to the direction of travel
inline std::vector<KerbSides> kerbLayout(const std::vector<cp3::PathSample>& path) {
    const std::size_t n=path.size()-1; // the last sample closes the loop
    const double scale=physical::metresPerSceneUnit(),lap=path.back().distance*scale;
    std::vector<double> metres(n),heading(n),curvature(n),smooth(n,0.0);
    for(std::size_t i=0;i<n;++i){metres[i]=path[i].distance*scale;heading[i]=std::atan2(-path[i].normal.x,path[i].normal.y);}
    const auto wrapped=[&](double d){return d<0?d+lap:d>=lap?d-lap:d;};
    for(std::size_t i=0;i<n;++i){
        const std::size_t a=(i+n-1)%n,b=(i+1)%n;
        const double turn=std::remainder(heading[b]-heading[a],2*std::numbers::pi);
        curvature[i]=turn/wrapped(metres[b]-metres[a]); // + = turning left
    }
    for(std::size_t i=0;i<n;++i){double sum=0;int count=0;
        for(std::size_t j=0;j<n;++j){const double d=std::abs(metres[j]-metres[i]);if(std::min(d,lap-d)<=25){sum+=curvature[j];++count;}}
        smooth[i]=sum/count;}
    std::vector<KerbSides> sides(n+1);
    const auto mark=[&](double from,double to,bool left){
        for(std::size_t i=0;i<n;++i){const double d=wrapped(metres[i]-wrapped(from));
            if(d<=wrapped(to-from)){(left?sides[i].left:sides[i].right)=true;}}
    };
    for(const double limit:{1.0/400,1.0/150}){
        for(std::size_t start=0;start<n;++start){
            const std::size_t previous=(start+n-1)%n;
            if(std::abs(smooth[start])<=limit||std::abs(smooth[previous])>limit)continue;
            std::size_t end=start;double peak=0;
            while(std::abs(smooth[end%n])>limit&&end<start+n){if(std::abs(smooth[end%n])>std::abs(peak))peak=smooth[end%n];++end;}
            const double from=metres[start],to=metres[(end-1)%n];
            const bool left=peak>0;
            if(limit<1.0/200)mark(from-10,to+10,left);                       // inside, through the corner
            else mark(from+wrapped(to-from)/2,to+25,!left);                     // outside, on the exit
        }
    }
    for(std::size_t i=0;i<n;++i)if(cp3::onPitLane(path[i].distance/path.back().distance))sides[i].right=false;
    sides[n]=sides[0];
    return sides;
}
inline Mesh makeWorld(const std::vector<cp3::PathSample>& path,const cp3::RaceMotion* race) {
    Mesh mesh;
    const double scale=physical::metresPerSceneUnit();
    // The detailed ground covers the entire camera frustum; the sky pass extends it analytically to the horizon.
    // Over the terrain map it is one quad per height texel so it follows the
    // elevation exactly; outside, four large quads sit at the map's edge height.
    constexpr Color grass{0.24F,0.36F,0.23F};
    const auto& ground=terrain();
    for(int row=0;row+1<ground.rows;++row)for(int column=0;column+1<ground.columns;++column){
        const double x0=ground.minX+column*ground.cell,x1=x0+ground.cell,z0=ground.minZ+row*ground.cell,z1=z0+ground.cell;
        quad(mesh,{x0,0,z0},{x0,0,z1},{x1,0,z1},{x1,0,z0},grass);
    }
    constexpr double far=50000;
    const double gx0=ground.minX,gx1=ground.maxX(),gz0=ground.minZ,gz1=ground.maxZ();
    quad(mesh,{-far,0,-far},{-far,0,far},{gx0,0,far},{gx0,0,-far},grass);
    quad(mesh,{gx1,0,-far},{gx1,0,far},{far,0,far},{far,0,-far},grass);
    quad(mesh,{gx0,0,-far},{gx0,0,gz0},{gx1,0,gz0},{gx1,0,-far},grass);
    quad(mesh,{gx0,0,gz1},{gx0,0,far},{gx1,0,far},{gx1,0,gz1},grass);
    constexpr Color asphalt{0.17F,0.195F,0.225F},white{0.88F,0.91F,0.91F};
    const double half=physical::trackWidth/2;
    const auto kerbs=kerbLayout(path);
    for(std::size_t i=0;i+1<path.size();++i) {
        const auto a=cp3::poseAtProgress(path,path[i].distance/path.back().distance),
                   b=cp3::poseAtProgress(path,path[i+1].distance/path.back().distance);
        roadStrip(mesh,a,b,-half,half,physical::roadHeight,asphalt);
        // roadStrip offsets are + to the right of travel; cp3::offset is + to the left.
        for(double side:{-1.0,1.0}) {
            roadStrip(mesh,a,b,side*(half-0.15),side*half,physical::roadHeight+0.01,white);
            if(!(side<0?kerbs[i].left:kerbs[i].right))continue;
            const auto color=(static_cast<int>(path[i].distance*scale/2)%2)==0?Color{0.78F,0.12F,0.17F}:white;
            roadStrip(mesh,a,b,side*half,side*(half+physical::kerbWidth),physical::kerbHeight,color);
            const auto pa=world(cp3::offset(path[i],physical::sceneUnits(-side*(half+physical::kerbWidth)))),
                       pb=world(cp3::offset(path[i+1],physical::sceneUnits(-side*(half+physical::kerbWidth))));
            // Outer face, wound so its normal faces away from the track on either side.
            const V3 up{0,physical::kerbHeight,0};
            if(side>0)quad(mesh,pa,pb,pb+up,pa+up,color);else quad(mesh,pb,pa,pa+up,pb+up,color);
        }
    }
    // A 1 m deep chequered timing line, rather than a scene-sized marking.
    const auto start=cp3::poseAtProgress(path,0);
    const auto origin=world(start.position,physical::roadHeight+0.012);
    for(int row=0;row<2;++row)for(int col=0;col<24;++col) {
        const auto begin=mesh.size();
        quad(mesh,{row*0.5,0,-half+col*0.5},{row*0.5,0,-half+(col+1)*0.5},
             {(row+1)*0.5,0,-half+(col+1)*0.5},{(row+1)*0.5,0,-half+col*0.5},
             (row+col)%2?Color{0.04F,0.04F,0.04F}:white);
        place(mesh,begin,origin,start.headingRadians);
    }
    if(race) {
        for(int i=0;i<256;++i) {
            const double a=cp3::pitLaneStartProgress+(cp3::pitLaneEndProgress-cp3::pitLaneStartProgress)*i/256;
            const double b=cp3::pitLaneStartProgress+(cp3::pitLaneEndProgress-cp3::pitLaneStartProgress)*(i+1)/256;
            const auto pa=race->pitPose(a,false),pb=race->pitPose(b,false);
            roadStrip(mesh,pa,pb,-physical::pitWidth/2,physical::pitWidth/2,physical::roadHeight,asphalt);
            for(double side:{-1.0,1.0})roadStrip(mesh,pa,pb,side*1.90,side*2.0,physical::roadHeight+0.01,white);
        }
        garage(mesh,*race);grandstands(mesh,path);drsZones(mesh,path);parcFermeScene(mesh,path);
        trackside(mesh,path);podium(mesh,path);ovalBanking(mesh,path);tvTowers(mesh);
        auto structure=gantry(path,0,true);mesh.insert(mesh.end(),structure.begin(),structure.end());
    }
    if(race){
        mappedBuildings(mesh,path);
        for(const auto& tree:woodlandTrees(path))broadleafTree(mesh,tree);
    }
    // 0.75 m Armco, estimated 5 m beyond the kerb. Omit pit-entry region.
    for(std::size_t i=0;i+1<path.size();++i) {
        const double progress=path[i].distance/path.back().distance;
        if(progress<0.13||progress>0.94)continue;
        for(double side:{-1.0,1.0}) {
            const auto a=world(cp3::offset(path[i],physical::sceneUnits(side*12)),0.75),
                       b=world(cp3::offset(path[i+1],physical::sceneUnits(side*12)),0.75);
            tube(mesh,a,b,0.08,{0.58F,0.61F,0.59F},6);
            tube(mesh,{a.x,0,a.z},a,0.06,{0.43F,0.47F,0.45F},5);
        }
    }
    return mesh;
}
inline void tyre(Mesh& mesh,V3 centre,double spin,bool hard,double width=physical::frontTyreWidth,Color marker={-1,-1,-1}) {
    constexpr Color rubber{0.035F,0.040F,0.045F};
    tube(mesh,centre+V3{0,0,-width/2},centre+V3{0,0,width/2},physical::tyreRadius,rubber,32);
    for(double side:{-1.0,1.0}) {
        const double z=centre.z+side*(width/2+0.001);
        tube(mesh,{centre.x,centre.y,z-side*0.012},{centre.x,centre.y,z},physical::rimRadius,{0.30F,0.33F,0.35F},24);
        for(int i=0;i<6;++i) {
            const double angle=spin+i*std::numbers::pi/3;
            tube(mesh,{centre.x,centre.y,z+side*0.001},
                {centre.x+0.205*std::cos(angle),centre.y+0.205*std::sin(angle),z+side*0.001},0.003,{0.70F,0.73F,0.75F},6);
        }
        const double angle=spin+0.5;
        sphere(mesh,{centre.x+0.30*std::cos(angle),centre.y+0.30*std::sin(angle),z},
            {0.025,0.025,0.002},marker.r>=0?marker:hard?Color{0.95F,0.95F,0.93F}:Color{0.95F,0.77F,0.15F},4,8);
    }
}
inline double metricSteering(const std::vector<cp3::PathSample>& path,double progress) {
    const double step=2.0/(path.back().distance*physical::metresPerSceneUnit());
    const auto a=cp3::poseAtProgress(path,progress-step),b=cp3::poseAtProgress(path,progress+step);
    const double delta=std::atan2(std::sin(b.headingRadians-a.headingRadians),std::cos(b.headingRadians-a.headingRadians));
    return std::clamp(std::atan(physical::wheelbase*delta/4.0),-0.436332313,0.436332313);
}
inline double pitSteering(const cp3::RaceMotion& race,double progress,double bay=cp3::pitBoxProgress) {
    const double step=1.0/cp3::circuitMetres;
    const auto a=race.pitPose(progress-step,true,bay),b=race.pitPose(progress+step,true,bay);
    const double metres=length(world(b.position)-world(a.position));
    const double delta=std::atan2(std::sin(b.headingRadians-a.headingRadians),std::cos(b.headingRadians-a.headingRadians));
    return std::clamp(std::atan(physical::wheelbase*delta/std::max(metres,0.01)),-0.436332313,0.436332313);
}
inline Mesh makeCar(const cp3::Pose& pose,double turns,bool hard=false,double steer=0.0,Color bodyColor={0.10F,0.23F,0.52F},Color helmet={0.95F,0.74F,0.15F},Color tyreMarker={-1,-1,-1},int number=0,double drs=0,bool airflow=false) {
    Mesh mesh;const Color blue=bodyColor;constexpr Color carbon{0.055F,0.065F,0.075F};
    // Lofted monocoque/nose: real volume with separate top and side normals.
    struct Section { double x,w,h; };
    constexpr std::array<Section,6> sections{{{-0.060,0.012,0.017},{-0.032,0.016,0.030},
        {-0.005,0.014,0.027},{0.024,0.011,0.022},{0.055,0.006,0.017},{0.078,0.0035,0.013}}};
    for(std::size_t i=0;i+1<sections.size();++i) {
        const auto a=sections[i],b=sections[i+1];
        quad(mesh,{a.x,a.h,-a.w},{a.x,a.h,a.w},{b.x,b.h,b.w},{b.x,b.h,-b.w},blue);
        quad(mesh,{a.x,0.008,a.w},{b.x,0.008,b.w},{b.x,b.h,b.w},{a.x,a.h,a.w},blue);
        quad(mesh,{b.x,0.008,-b.w},{a.x,0.008,-a.w},{a.x,a.h,-a.w},{b.x,b.h,-b.w},blue);
    }
    box(mesh,{-0.057,0.006,-0.021},{0.048,0.010,0.021},carbon);
    box(mesh,{0.069,0.004,-0.030},{0.082,0.007,0.030},carbon);
    box(mesh,{0.076,0.007,-0.027},{0.084,0.009,0.027},blue);
    for(double side:{-1.0,1.0}) {
        const double z=side*0.030;
        box(mesh,{0.065,0.004,z-0.001},{0.085,0.017,z+0.001},blue);
        box(mesh,{-0.072,0.035,z-0.001},{-0.055,0.052,z+0.001},blue);
        for(double x:{-0.05,0.05}) {
            const V3 wheel{x,0.020,side*0.025};
            tube(mesh,{x-0.014,0.014,side*0.011},wheel,0.0012,carbon,6);
            tube(mesh,{x+0.013,0.014,side*0.010},wheel,0.0012,carbon,6);
        }
        box(mesh,{-0.066,0.012,side*0.006-0.001},{-0.063,0.044,side*0.006+0.001},carbon);
    }
    box(mesh,{-0.072,0.044,-0.029},{-0.056,0.048,0.029},carbon);
    const auto flapStart=mesh.size();
    box(mesh,{-0.070,0.052,-0.026},{-0.058,0.053,0.026},carbon);
    const auto flapEnd=mesh.size();
    sphere(mesh,{-0.008,0.026,0},{0.017,0.004,0.009},carbon,6,16);
    sphere(mesh,{-0.010,0.033,0},{0.0042,0.0089,0.00465},helmet,8,12);
    box(mesh,{-0.031,0.025,-0.006},{-0.022,0.042,0.006},blue);
    for(double side:{-1.0,1.0})
        tube(mesh,{-0.020,0.037,side*0.009},{0.010,0.035,side*0.008},0.0015,carbon,8);
    tube(mesh,{0.011,0.035,-0.008},{0.011,0.035,0.008},0.0015,carbon,8);
    tube(mesh,{0.011,0.035,0},{0.025,0.022,0},0.0014,carbon,8);
    // 5.65 m body length, 2.0 m wing span and ~0.95 m upper body height.
    // This is a representative envelope, not proprietary RB21 CAD.
    for(auto& v:mesh) {
        v.x*=36.0F;v.y*=18.0F;v.z*=static_cast<float>(physical::carWidth/0.062);
        const auto n=unit({v.nx/36.0,v.ny/18.0,v.nz/(physical::carWidth/0.062)});
        v.nx=static_cast<float>(n.x);v.ny=static_cast<float>(n.y);v.nz=static_cast<float>(n.z);
    }
    // DRS: the upper flap pivots from steep (about 29 degrees, high drag) to almost
    // flat, opening a slot above the main plane. Eased by the caller over 0.2 s.
    // Closed, the flap tip stays under the 1 m overall height of the car.
    const double angle=.03+(1-drs)*.47,hinge=-.070*36,height=.052*18,flapHeight=.75+drs*.12;
    for(auto i=flapStart;i<flapEnd;++i){auto& v=mesh[i];const double x=v.x-hinge,y=v.y-height;
        v.x=static_cast<float>(hinge+x*std::cos(angle)-y*std::sin(angle));
        v.y=static_cast<float>(flapHeight+x*std::sin(angle)+y*std::cos(angle));
        const double nx=v.nx,ny=v.ny;v.nx=static_cast<float>(nx*std::cos(angle)-ny*std::sin(angle));v.ny=static_cast<float>(nx*std::sin(angle)+ny*std::cos(angle));}
    if(number){
        const auto label=std::to_string(number);const double cell=.040;
        // Rear-facing race number for chase view, plus a number on the upper nose.
        box(mesh,{-2.62,.53,-.32},{-2.60,.84,.32},carbon);
        surfaceText(mesh,{-2.625,.55,-double(label.size()*6-1)*cell/2},{0,0,1},{0,1,0},cell,label,{1,1,1});
        surfaceText(mesh,{1.28,.364,-.19},{0,0,1},{-1,.097,0},.038,label,{1,1,1});
        for(double side:{-1.,1.}){
            const double z=side*.54;
            box(mesh,{-.55,.18,z-.003},{.48,.47,z+.003},{.035F,.045F,.055F});
            logoDecal(mesh,number,side>0?V3{-.51,.20,z+.004}:V3{.44,.20,z-.004},side>0?V3{1,0,0}:V3{-1,0,0},{0,1,0},.95,.25);
        }
        // A second logo on the engine cover remains readable from above.
        logoDecal(mesh,number,{-1.65,.59,.20},{1,0,0},{0,0,-1},.70,.40);
    }
    if(drs>.1){
        // Glowing flap edge and endplate markers make an open wing obvious from any camera.
        tube(mesh,{-2.52,flapHeight+.025,-.90},{-2.52,flapHeight+.025,.90},.018,{.10F,1.0F,.85F},6);
        for(double side:{-1.,1.})box(mesh,{-2.60,.80,side*.93-.01},{-2.22,flapHeight+.10,side*.93+.01},{.10F,1.0F,.85F});
    }
    if(airflow&&drs>.1)for(double side:{-1.,1.})for(int i=0;i<3;++i){const double y=.82+i*.10,z=side*(.50+i*.12);
        quad(mesh,{-2.6,y,z},{-2.6,y+.018,z},{-5.3-i*.45,y+.15,z+side*.15},{-5.3-i*.45,y+.14,z+side*.15},{.12F,.74F,.65F});}
    for(double x:{-physical::wheelbase/2,physical::wheelbase/2})for(double side:{-1.0,1.0}) {
        const double width=x>0?physical::frontTyreWidth:physical::rearTyreWidth;
        const auto tyreStart=mesh.size();
        tyre(mesh,{0,0,0},-std::fmod(turns,1.0)*2*std::numbers::pi,hard,width,tyreMarker);
        place(mesh,tyreStart,{x,physical::tyreRadius,side*(physical::carWidth/2-width/2-0.02)},x>0?steer:0);
    }
    place(mesh,0,world(pose.position,physical::roadHeight),pose.headingRadians);
    return mesh;
}
inline Mesh makeCrew(const cp3::Pose& pose,bool active,Color team={0.10F,0.22F,0.44F}) {
    Mesh mesh;if(!active)return mesh;
    // Crouched mechanics, 1.15 m high, spaced around the actual wheel mounts.
    for(double x:{-physical::wheelbase/2,physical::wheelbase/2})for(double side:{-1.0,1.0}) {
        const V3 p{x,0,side*1.65};
        tube(mesh,p+V3{-0.18,0.06,0},p+V3{-0.10,0.48,0},0.08,{0.07F,0.13F,0.26F},7);
        tube(mesh,p+V3{0.18,0.06,0},p+V3{0.10,0.48,0},0.08,{0.07F,0.13F,0.26F},7);
        box(mesh,p+V3{-0.22,0.42,-0.13},p+V3{0.22,0.80,0.13},team);
        sphere(mesh,p+V3{0,physical::crewHeight-0.16,0},{0.15,0.16,0.15},{0.86F,0.88F,0.86F},6,10);
        tube(mesh,p+V3{0,0.72,0},V3{x,physical::tyreRadius,side*1.02},0.065,{0.13F,0.24F,0.44F},6);
    }
    place(mesh,0,world(pose.position,physical::roadHeight),pose.headingRadians);return mesh;
}
inline Mesh makeAnchors(cp3::Route route) {
    Mesh mesh;const auto flat=cp3::makeControlMesh(route);
    for(std::size_t i=0;i<flat.size();i+=6)
        sphere(mesh,world({flat[i].x,flat[i].y},0.45),{0.35,0.35,0.35},{1.0F,0.65F,0.17F},5,8);
    return mesh;
}

enum class CameraMode { Overview, Chase, Cockpit, Tv };
inline const char* cameraName(CameraMode mode) {
    switch(mode){case CameraMode::Overview:return "OVERVIEW";case CameraMode::Chase:return "CHASE";case CameraMode::Cockpit:return "COCKPIT";case CameraMode::Tv:return "TV";}
    return "OVERVIEW";
}
struct CameraFrame { M4 viewProjection; V3 eye; V3 target; double shadowExtent{55.0}; double tanHalfFov{0.62}; };
struct CameraRig {
    CameraMode mode{CameraMode::Overview};
    double yaw{0.35},pitch{0.82},zoom{1.0};
    V3 smoothEye{},smoothTarget{},previousOrigin{};
    bool initialized{false};
    void reset() { yaw=mode==CameraMode::Overview?0.35:0.0;pitch=mode==CameraMode::Overview?0.82:0.48;zoom=1.0;initialized=false; }
    void cycle() {mode=static_cast<CameraMode>((static_cast<int>(mode)+1)%4);reset();}
    void orbit(double dx,double dy) {
        if(mode==CameraMode::Cockpit||mode==CameraMode::Tv){mode=CameraMode::Chase;reset();}
        yaw-=dx*0.006;pitch=std::clamp(pitch+dy*0.005,0.18,1.40);
    }
    void dolly(double steps) {
        const double minimum=mode==CameraMode::Overview?0.008:mode==CameraMode::Chase?0.18:mode==CameraMode::Tv?0.35:0.42;
        zoom=std::clamp(zoom*std::pow(0.88,steps),minimum,mode==CameraMode::Cockpit?1.9:mode==CameraMode::Tv?2.5:3.0);
    }
    CameraFrame frame(const cp3::Pose& pose,double aspect,double delta=0.0,bool snap=false) {
        V3 eye,target;double fov=48.0;
        if(mode==CameraMode::Overview) {
            const double distance=4.35*physical::metresPerSceneUnit()*zoom;
            const double focus=std::clamp((0.85-zoom)/0.60,0.0,1.0);
            target=mix({0.03*physical::metresPerSceneUnit(),0.0,0.0},world(pose.position),focus);
            eye=target+V3{distance*std::sin(yaw)*std::cos(pitch),distance*std::sin(pitch),distance*std::cos(yaw)*std::cos(pitch)};
        }else if(mode==CameraMode::Tv){
            // Fixed tower; a long lens keeps about 22 m of track in shot at the car (zoom scales it).
            static const double lapLength=cp3::makePath().back().distance;
            const auto& tower=tvCameraFor(pose.distance/lapLength);
            eye=tower.lens;target=world(pose.position,physical::roadHeight+0.55);
            const double range=std::max(length(target-eye),1.0);
            fov=std::clamp(2*std::atan(11.0*zoom/range)*180/std::numbers::pi,4.0,60.0);
        }else{
            const auto origin=world(pose.position,physical::roadHeight);
            const auto forward=rotateY({1,0,0},pose.headingRadians),right=rotateY({0,0,1},pose.headingRadians);
            if(mode==CameraMode::Cockpit) {
                eye=origin+forward*(-0.45)+V3{0,0.82,0};target=eye+forward*30+V3{0,-0.15,0};fov=std::clamp(70.0*zoom,40.0,95.0);
            }else{
                const auto behind=forward*(-std::cos(yaw))+right*std::sin(yaw);
                eye=origin+behind*(12.0*zoom*std::cos(pitch))+V3{0,12.0*zoom*std::sin(pitch)+0.65,0};
                target=origin+forward*2.0+V3{0,0.65,0};fov=55.0;
            }
        }
        if(mode==CameraMode::Chase && initialized && !snap && delta>0) {
            const auto movement=world(pose.position,physical::roadHeight)-previousOrigin;
            smoothEye=smoothEye+movement;smoothTarget=smoothTarget+movement;
            const double blend=1.0-std::exp(-delta*9.0);
            smoothEye=mix(smoothEye,eye,blend);smoothTarget=mix(smoothTarget,target,blend);
        }else{smoothEye=eye;smoothTarget=target;}
        previousOrigin=world(pose.position,physical::roadHeight);initialized=true;
        const bool overview=mode==CameraMode::Overview;
        // The renderer raises the scene by the terrain height; raise the camera with it.
        // Lifting eye and target separately keeps their ground clearance and lets the
        // cockpit view pitch with the road.
        const V3 eyeOut=onTerrain(smoothEye),targetOut=onTerrain(smoothTarget);
        // Depth precision falls with distance/near. A fixed 5 cm near plane left
        // too few depth steps far down the road to separate the heatmap strip
        // (5 mm above the asphalt), so it flickered when the chase camera pulled
        // back. Scaling the near plane with camera distance keeps the car unclipped.
        const double distance=length(smoothEye-smoothTarget);
        const double nearPlane=overview?std::max(.1,distance*.01):mode==CameraMode::Chase||mode==CameraMode::Tv?std::max(.05,distance*.02):0.05;
        return {perspective(fov*std::numbers::pi/180,aspect,nearPlane,15000.0)*lookAt(eyeOut,targetOut),
            eyeOut,targetOut,overview?3.0*physical::metresPerSceneUnit():55.0,std::tan(fov*std::numbers::pi/360)};
    }
};
} // namespace apex3d::three
