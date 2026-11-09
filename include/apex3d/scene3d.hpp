#pragma once
#include "math3d.hpp"
#include "cp3_race.hpp"
#include "generated/monza_stands_2025.hpp"
#include "generated/monza_entries_2025.hpp"
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
    for(int i=0;i<96;++i) {
        const double a=cp3::pitEntryProgress+0.021+(cp3::pitExitProgress-cp3::pitEntryProgress-0.045)*i/96;
        const double b=cp3::pitEntryProgress+0.021+(cp3::pitExitProgress-cp3::pitEntryProgress-0.045)*(i+1)/96;
        // Follow the fast lane, without following the service-bay excursion.
        auto pa=world(cp3::poseAtProgress(mainPath,a).position),
             pb=world(cp3::poseAtProgress(mainPath,b).position);
        pa.z-=physical::pitOffset-3.0;pb.z-=physical::pitOffset-3.0;
        pa.y=pb.y=0.70;tube(mesh,pa,pb,0.20,{0.68F,0.69F,0.65F},4);
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
inline Mesh makeWorld(const std::vector<cp3::PathSample>& path,const cp3::RaceMotion* race) {
    Mesh mesh;
    const double scale=physical::metresPerSceneUnit();
    box(mesh,{-2.02*scale,-2,-1.29*scale},{2.08*scale,0,1.30*scale},{0.24F,0.36F,0.23F});
    constexpr Color asphalt{0.17F,0.195F,0.225F},white{0.88F,0.91F,0.91F};
    const double half=physical::trackWidth/2;
    for(std::size_t i=0;i+1<path.size();++i) {
        const auto a=cp3::poseAtProgress(path,path[i].distance/path.back().distance),
                   b=cp3::poseAtProgress(path,path[i+1].distance/path.back().distance);
        roadStrip(mesh,a,b,-half,half,physical::roadHeight,asphalt);
        for(double side:{-1.0,1.0}) {
            const auto color=(static_cast<int>(path[i].distance*scale/2)%2)==0?Color{0.78F,0.12F,0.17F}:white;
            roadStrip(mesh,a,b,side*half,side*(half+physical::kerbWidth),physical::kerbHeight,color);
            roadStrip(mesh,a,b,side*(half-0.15),side*half,physical::roadHeight+0.01,white);
            const auto pa=world(cp3::offset(path[i],physical::sceneUnits(side*(half+physical::kerbWidth)))),
                       pb=world(cp3::offset(path[i+1],physical::sceneUnits(side*(half+physical::kerbWidth))));
            quad(mesh,pa,pb,pb+V3{0,physical::kerbHeight,0},pa+V3{0,physical::kerbHeight,0},color);
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
            const double a=cp3::pitEntryProgress+(cp3::pitExitProgress-cp3::pitEntryProgress)*i/256;
            const double b=cp3::pitEntryProgress+(cp3::pitExitProgress-cp3::pitEntryProgress)*(i+1)/256;
            const auto pa=race->pitPose(a,false),pb=race->pitPose(b,false);
            roadStrip(mesh,pa,pb,-physical::pitWidth/2,physical::pitWidth/2,physical::roadHeight,asphalt);
            for(double side:{-1.0,1.0})roadStrip(mesh,pa,pb,side*1.90,side*2.0,physical::roadHeight+0.01,white);
        }
        garage(mesh,*race);grandstands(mesh,path);
        auto structure=gantry(path,0,true);mesh.insert(mesh.end(),structure.begin(),structure.end());
    }
    // Trees: 10-18 m high with 0.4 m trunks, clear of road and pit facilities.
    for(int i=0;i<400;++i) {
        const double x=(-1.94+3.92*std::fmod(i*0.61803398875,1.0))*scale;
        const double z=(-1.20+2.39*std::fmod(i*0.41421356237+0.2,1.0))*scale;
        double nearest=1e9;
        for(std::size_t j=0;j+1<path.size();++j){
            const auto a=world(path[j].point),b=world(path[j+1].point),d=b-a;
            const double t=std::clamp(dot(V3{x,0,z}-a,d)/dot(d,d),0.0,1.0);
            nearest=std::min(nearest,length(V3{x,0,z}-(a+d*t)));
        }
        if(nearest<55)continue;
        const double h=10+8*std::fmod(i*0.73,1.0);
        tube(mesh,{x,0,z},{x,h*0.50,z},0.20,{0.31F,0.23F,0.15F},6);
        cone(mesh,{x,h*0.27,z},3.0,h*0.65,{0.16F,0.30F,0.18F});
        cone(mesh,{x,h*0.52,z},2.1,h*0.48,{0.20F,0.37F,0.21F});
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
inline Mesh makeCar(const cp3::Pose& pose,double turns,bool hard=false,double steer=0.0,Color bodyColor={0.10F,0.23F,0.52F},Color helmet={0.95F,0.74F,0.15F},Color tyreMarker={-1,-1,-1}) {
    Mesh mesh;const Color blue=bodyColor;constexpr Color carbon{0.055F,0.065F,0.075F},gold{0.95F,0.74F,0.15F};
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
    box(mesh,{-0.070,0.049,-0.028},{-0.056,0.052,0.028},blue);
    box(mesh,{-0.070,0.052,-0.026},{-0.058,0.053,0.026},gold);
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

enum class CameraMode { Overview, Chase, Cockpit };
inline const char* cameraName(CameraMode mode) {
    switch(mode){case CameraMode::Overview:return "OVERVIEW";case CameraMode::Chase:return "CHASE";case CameraMode::Cockpit:return "COCKPIT";}
    return "OVERVIEW";
}
struct CameraFrame { M4 viewProjection; V3 eye; V3 target; double shadowExtent{55.0}; };
struct CameraRig {
    CameraMode mode{CameraMode::Overview};
    double yaw{0.35},pitch{0.82},zoom{1.0};
    V3 smoothEye{},smoothTarget{},previousOrigin{};
    bool initialized{false};
    void reset() { yaw=mode==CameraMode::Overview?0.35:0.0;pitch=mode==CameraMode::Overview?0.82:0.48;zoom=1.0;initialized=false; }
    void cycle() {mode=static_cast<CameraMode>((static_cast<int>(mode)+1)%3);reset();}
    void orbit(double dx,double dy) {
        if(mode==CameraMode::Cockpit){mode=CameraMode::Chase;reset();}
        yaw-=dx*0.006;pitch=std::clamp(pitch+dy*0.005,0.18,1.40);
    }
    void dolly(double steps) {
        const double minimum=mode==CameraMode::Overview?0.008:mode==CameraMode::Chase?0.18:0.42;
        zoom=std::clamp(zoom*std::pow(0.88,steps),minimum,mode==CameraMode::Cockpit?1.9:3.0);
    }
    CameraFrame frame(const cp3::Pose& pose,double aspect,double delta=0.0,bool snap=false) {
        V3 eye,target;double fov=48.0;
        if(mode==CameraMode::Overview) {
            const double distance=4.35*physical::metresPerSceneUnit()*zoom;
            const double focus=std::clamp((0.85-zoom)/0.60,0.0,1.0);
            target=mix({0.03*physical::metresPerSceneUnit(),0.0,0.0},world(pose.position),focus);
            eye=target+V3{distance*std::sin(yaw)*std::cos(pitch),distance*std::sin(pitch),distance*std::cos(yaw)*std::cos(pitch)};
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
        return {perspective(fov*std::numbers::pi/180,aspect,overview?1.0:0.05,15000.0)*lookAt(smoothEye,smoothTarget),
            smoothEye,smoothTarget,overview?3.0*physical::metresPerSceneUnit():55.0};
    }
};
} // namespace apex3d::three
