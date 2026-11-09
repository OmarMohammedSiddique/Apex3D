#pragma once
#include "math3d.hpp"
#include "cp3_race.hpp"
#include <vector>

namespace apex3d::three {
using cp3::Color;
struct Vertex {
    float x,y,z,nx,ny,nz,r,g,b;
};
using Mesh=std::vector<Vertex>;
inline V3 world(Vec2 p,double height=0) { return {p.x,height,-p.y}; }
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
inline void lift(Mesh& mesh,const std::vector<cp3::Vertex>& flat,bool pit=false) {
    for(const auto& v:flat) {
        double h=pit?0.016:0.010;
        if(pit) {
            if(v.r>0.4F)h=0.0158;
            if(v.r>0.6F)h=0.0165;
        }else{
            if(v.r<0.05F)h=0.004;
            else if(v.r>0.8F)h=0.012;
            else if(v.r>0.7F)h=0.014;
            else if(v.r<0.25F)h=0.016;
            else h=0.0163;
            if((v.r>0.95F)||(v.r<0.03F&&v.g<0.035F))h=0.0165;
        }
        vertex(mesh,{v.x,h,-v.y},{0,1,0},{v.r,v.g,v.b});
    }
}
inline void garage(Mesh& mesh,const cp3::RaceMotion& race) {
    const auto p=world(race.pitPose(cp3::pitBoxProgress).position);
    box(mesh,p+V3{-0.18,0,-0.14},p+V3{0.18,0.105,-0.060},{0.51F,0.54F,0.55F});
    box(mesh,p+V3{-0.187,0.105,-0.147},p+V3{0.187,0.116,-0.048},{0.24F,0.28F,0.31F});
    box(mesh,p+V3{-0.18,0.074,-0.059},p+V3{0.18,0.090,-0.057},{0.09F,0.21F,0.43F});
    for(int i=0;i<4;++i) {
        const double x=-0.164+i*0.085;
        box(mesh,p+V3{x,0.019,-0.059},p+V3{x+0.067,0.067,-0.056},{0.055F,0.075F,0.085F});
        box(mesh,p+V3{x,0.092,-0.059},p+V3{x+0.067,0.096,-0.055},{0.87F,0.72F,0.24F});
    }
    // A working apron gives the crew room between the box and the pit wall.
    box(mesh,p+V3{-0.14,0.001,-0.059},p+V3{0.14,0.0155,0.063},{0.31F,0.34F,0.35F});
    // The low track-side wall follows the straight part of the refined lane.
    for(int i=0;i<24;++i) {
        const double a=cp3::pitEntryProgress+0.022+(cp3::pitExitProgress-cp3::pitEntryProgress-0.047)*i/24;
        const double b=cp3::pitEntryProgress+0.022+(cp3::pitExitProgress-cp3::pitEntryProgress-0.047)*(i+1)/24;
        auto pa=world(race.pitPose(a).position),pb=world(race.pitPose(b).position);
        pa.z+=0.067;pb.z+=0.067;pa.y=pb.y=0.043;
        tube(mesh,pa,pb,0.004,{0.68F,0.69F,0.65F},4);
    }
}
inline void grandstand(Mesh& m) {
    for(int row=0;row<5;++row) {
        const double z=1.02+row*0.043,h=0.018+row*0.020;
        box(m,{-0.46,0,z},{0.35,h,z+0.043},{0.53F,0.56F,0.55F});
        for(int seat=0;seat<22;++seat) {
            const double x=-0.44+seat*0.035;
            box(m,{x,h,z+0.01},{x+0.024,h+0.012,z+0.03},row%2?Color{0.18F,0.31F,0.43F}:Color{0.76F,0.75F,0.65F});
        }
    }
    box(m,{-0.50,0.19,0.98},{0.39,0.20,1.26},{0.72F,0.73F,0.69F});
    for(double x:{-0.47,0.36})for(double z:{1.0,1.23})tube(m,{x,0,z},{x,0.19,z},0.006,{0.40F,0.44F,0.45F});
}
inline Mesh gantry(const std::vector<cp3::PathSample>& path,int lights,bool frame) {
    Mesh mesh;const auto pose=cp3::poseAtProgress(path,cp3::gridOffset+0.018);
    constexpr Color steel{0.26F,0.29F,0.31F};
    if(frame) {
        for(double z:{-0.105,0.105})box(mesh,{-0.008,0,z-0.005},{0.008,0.132,z+0.005},steel);
        box(mesh,{-0.012,0.113,-0.114},{0.012,0.139,0.114},steel);
    } else {
        for(int i=0;i<5;++i)for(double h:{0.119,0.132})
            sphere(mesh,{-0.014,h,-0.041+i*0.0205},{0.004,0.005,0.005},
                i<lights?Color{1.0F,0.025F,0.02F}:Color{0.13F,0.035F,0.035F},4,8);
    }
    place(mesh,0,world(pose.position,0.018),pose.headingRadians);return mesh;
}
inline Mesh makeWorld(const std::vector<cp3::PathSample>& path,const cp3::RaceMotion* race) {
    Mesh mesh;
    box(mesh,{-2.02,-0.095,-1.29},{2.08,-0.005,1.30},{0.20F,0.27F,0.22F});
    box(mesh,{-2.02,-0.005,-1.29},{2.08,0,1.30},{0.24F,0.36F,0.23F});
    lift(mesh,cp3::makeTrackMesh(path));
    // Actual vertical road edges and raised kerb surfaces give the track depth.
    for(std::size_t i=0;i+1<path.size();++i)for(double side:{-0.083,0.083}) {
        const auto a=cp3::offset(path[i],side),b=cp3::offset(path[i+1],side);
        quad(mesh,world(a,0.0),world(b,0.0),world(b,0.012),world(a,0.012),{0.26F,0.28F,0.27F});
    }
    const auto coveredByRoad=[&](Vec2 point) {
        for(std::size_t j=0;j+1<path.size();++j) {
            const auto a=path[j].point,b=path[j+1].point;
            const double dx=b.x-a.x,dy=b.y-a.y,span=dx*dx+dy*dy;
            const double t=span>1e-12?std::clamp(((point.x-a.x)*dx+(point.y-a.y)*dy)/span,0.0,1.0):0.0;
            if(std::hypot(point.x-a.x-t*dx,point.y-a.y-t*dy)<0.068)return true;
        }
        return false;
    };
    // Raise only the narrow kerb rings; a raised full disk would cover the road.
    for(std::size_t i=0;i+1<path.size();++i) {
        const Color color=(i/6)%2==0?Color{0.78F,0.12F,0.17F}:Color{0.88F,0.91F,0.91F};
        for(double side:{-1.0,1.0}) {
            const auto a=cp3::offset(path[i],side*0.069),b=cp3::offset(path[i+1],side*0.069);
            const auto c=cp3::offset(path[i+1],side*0.083),d=cp3::offset(path[i],side*0.083);
            if(coveredByRoad(c)||coveredByRoad(d)||coveredByRoad({(a.x+b.x+c.x+d.x)/4,(a.y+b.y+c.y+d.y)/4}))continue;
            if(side>0)quad(mesh,world(a,0.019),world(b,0.019),world(c,0.019),world(d,0.019),color);
            else quad(mesh,world(a,0.019),world(d,0.019),world(c,0.019),world(b,0.019),color);
        }
    }
    if(race) {
        lift(mesh,race->pitMesh(false),true);garage(mesh,*race);grandstand(mesh);
        auto structure=gantry(path,0,true);mesh.insert(mesh.end(),structure.begin(),structure.end());
    }
    // Deterministic parkland planting, kept clear of both road and pit apron.
    for(int i=0;i<130;++i) {
        const double x=-1.94+3.92*std::fmod(i*0.61803398875,1.0);
        const double z=-1.20+2.39*std::fmod(i*0.41421356237+0.2,1.0);
        double nearest=10;
        for(const auto& p:path)nearest=std::min(nearest,std::hypot(x-p.point.x,z+p.point.y));
        if(nearest<0.18 || (x>-0.6&&z>0.95) || (x>0.55&&z>0.49))continue;
        const double h=0.105+0.06*std::fmod(i*0.73,1.0);
        tube(mesh,{x,0,z},{x,h*0.50,z},0.008,{0.31F,0.23F,0.15F},6);
        cone(mesh,{x,h*0.27,z},0.046,h*0.80,{0.16F,0.30F,0.18F});
        cone(mesh,{x,h*0.52,z},0.034,h*0.67,{0.20F,0.37F,0.21F});
    }
    // Armco away from the pit merge; no barrier crosses the racing surface.
    for(std::size_t i=0;i+4<path.size();i+=4) {
        const double progress=path[i].distance/path.back().distance;
        if(progress<0.13||progress>0.94)continue;
        for(double side:{-0.123,0.123}) {
            const auto a=world(cp3::offset(path[i],side),0.043),b=world(cp3::offset(path[i+4],side),0.043);
            tube(mesh,a,b,0.0028,{0.58F,0.61F,0.59F},6);
            if(i%12==0)tube(mesh,{a.x,0,a.z},a,0.003,{0.43F,0.47F,0.45F},5);
        }
    }
    return mesh;
}
inline void tyre(Mesh& mesh,V3 centre,double spin,bool hard) {
    constexpr Color rubber{0.035F,0.040F,0.045F};
    tube(mesh,centre+V3{0,0,-0.005},centre+V3{0,0,0.005},0.012,rubber,24);
    for(double side:{-1.0,1.0}) {
        const double z=centre.z+side*0.0052;
        tube(mesh,{centre.x,centre.y,z-side*0.0003},{centre.x,centre.y,z},0.0074,{0.30F,0.33F,0.35F},20);
        for(int i=0;i<6;++i) {
            const double angle=spin+i*std::numbers::pi/3;
            tube(mesh,{centre.x,centre.y,z+side*0.0001},
                {centre.x+0.0065*std::cos(angle),centre.y+0.0065*std::sin(angle),z+side*0.0001},0.0008,{0.70F,0.73F,0.75F},5);
        }
        const double angle=spin+0.5;
        sphere(mesh,{centre.x+0.010*std::cos(angle),centre.y+0.010*std::sin(angle),z},
            {0.0012,0.0012,0.0005},hard?Color{0.95F,0.95F,0.93F}:Color{0.95F,0.77F,0.15F},4,6);
    }
}
inline Mesh makeCar(const cp3::Pose& pose,double turns,bool hard=false,double steer=0.0) {
    Mesh mesh;constexpr Color blue{0.10F,0.23F,0.52F},carbon{0.055F,0.065F,0.075F},gold{0.95F,0.74F,0.15F};
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
        for(double x:{-0.044,0.045}) {
            const V3 wheel{x,0.012,side*0.025};
            const auto tyreStart=mesh.size();
            tyre(mesh,{0,0,0},std::fmod(turns,1.0)*2*std::numbers::pi,hard);
            place(mesh,tyreStart,wheel,x>0?steer:0.0);
            tube(mesh,{x-0.014,0.014,side*0.011},wheel,0.0012,carbon,6);
            tube(mesh,{x+0.013,0.014,side*0.010},wheel,0.0012,carbon,6);
        }
        box(mesh,{-0.066,0.012,side*0.006-0.001},{-0.063,0.044,side*0.006+0.001},carbon);
    }
    box(mesh,{-0.072,0.044,-0.029},{-0.056,0.048,0.029},carbon);
    box(mesh,{-0.070,0.049,-0.028},{-0.056,0.052,0.028},blue);
    box(mesh,{-0.070,0.052,-0.026},{-0.058,0.053,0.026},gold);
    sphere(mesh,{-0.008,0.026,0},{0.017,0.004,0.009},carbon,6,16);
    sphere(mesh,{-0.010,0.033,0},{0.007,0.007,0.006},gold,8,12);
    box(mesh,{-0.031,0.025,-0.006},{-0.022,0.042,0.006},blue);
    for(double side:{-1.0,1.0})
        tube(mesh,{-0.020,0.037,side*0.009},{0.010,0.035,side*0.008},0.0015,carbon,8);
    tube(mesh,{0.011,0.035,-0.008},{0.011,0.035,0.008},0.0015,carbon,8);
    tube(mesh,{0.011,0.035,0},{0.025,0.022,0},0.0014,carbon,8);
    place(mesh,0,world(pose.position,0.016),pose.headingRadians);
    return mesh;
}
inline Mesh makeCrew(const cp3::Pose& pose,bool active) {
    Mesh mesh;if(!active)return mesh;
    for(double x:{-0.044,0.045})for(double side:{-1.0,1.0}) {
        const V3 p{x,0.0,side*0.047};
        tube(mesh,p+V3{-0.006,0,0},p+V3{-0.003,0.017,0},0.003,{0.07F,0.13F,0.26F},7);
        tube(mesh,p+V3{0.006,0,0},p+V3{0.003,0.017,0},0.003,{0.07F,0.13F,0.26F},7);
        box(mesh,p+V3{-0.007,0.014,-0.004},p+V3{0.007,0.028,0.004},{0.10F,0.22F,0.44F});
        sphere(mesh,p+V3{0,0.034,0},{0.006,0.006,0.006},{0.86F,0.88F,0.86F},6,10);
        tube(mesh,p+V3{0,0.023,0},V3{x,0.012,side*0.030},0.0025,{0.13F,0.24F,0.44F},6);
    }
    place(mesh,0,world(pose.position,0.016),pose.headingRadians);return mesh;
}
inline Mesh makeAnchors(cp3::Route route) {
    Mesh mesh;const auto flat=cp3::makeControlMesh(route);
    for(std::size_t i=0;i<flat.size();i+=6)
        sphere(mesh,{flat[i].x,0.035,-flat[i].y},{0.009,0.009,0.009},{1.0F,0.65F,0.17F},5,8);
    return mesh;
}

enum class CameraMode { Overview, Chase, Cockpit };
inline const char* cameraName(CameraMode mode) {
    switch(mode){case CameraMode::Overview:return "OVERVIEW";case CameraMode::Chase:return "CHASE";case CameraMode::Cockpit:return "COCKPIT";}
    return "OVERVIEW";
}
struct CameraFrame { M4 viewProjection; V3 eye; V3 target; };
struct CameraRig {
    CameraMode mode{CameraMode::Overview};
    double yaw{0.35},pitch{0.82},zoom{1.0};
    V3 smoothEye{},smoothTarget{};
    bool initialized{false};
    void reset() { yaw=mode==CameraMode::Overview?0.35:0.0;pitch=mode==CameraMode::Overview?0.82:0.48;zoom=1.0;initialized=false; }
    void cycle() {mode=static_cast<CameraMode>((static_cast<int>(mode)+1)%3);reset();}
    void orbit(double dx,double dy) {
        if(mode==CameraMode::Cockpit){mode=CameraMode::Chase;reset();}
        yaw-=dx*0.006;pitch=std::clamp(pitch+dy*0.005,0.18,1.40);
    }
    void dolly(double steps) {zoom=std::clamp(zoom*std::pow(0.88,steps),0.42,1.9);}
    CameraFrame frame(const cp3::Pose& pose,double aspect,double delta=0.0,bool snap=false) {
        V3 eye,target;double fov=48.0;
        if(mode==CameraMode::Overview) {
            const double distance=4.35*zoom;
            target={0.03,0.0,0.0};
            eye=target+V3{distance*std::sin(yaw)*std::cos(pitch),distance*std::sin(pitch),distance*std::cos(yaw)*std::cos(pitch)};
        }else{
            const auto origin=world(pose.position,0.016);
            const auto forward=rotateY({1,0,0},pose.headingRadians),right=rotateY({0,0,1},pose.headingRadians);
            if(mode==CameraMode::Cockpit) {
                eye=origin+forward*(-0.006)+V3{0,0.043,0};target=eye+forward*0.5+V3{0,-0.006,0};fov=std::clamp(70.0*zoom,40.0,95.0);
            }else{
                const auto behind=forward*(-std::cos(yaw))+right*std::sin(yaw);
                eye=origin+behind*(0.39*zoom*std::cos(pitch))+V3{0,0.39*zoom*std::sin(pitch)+0.022,0};
                target=origin+forward*0.065+V3{0,0.023,0};fov=55.0;
            }
        }
        if(mode==CameraMode::Chase && initialized && !snap && delta>0) {
            const double blend=1.0-std::exp(-delta*9.0);
            smoothEye=mix(smoothEye,eye,blend);smoothTarget=mix(smoothTarget,target,blend);
        }else{smoothEye=eye;smoothTarget=target;}
        initialized=true;
        return {perspective(fov*std::numbers::pi/180,aspect,0.004,30.0)*lookAt(smoothEye,smoothTarget),smoothEye,smoothTarget};
    }
};
} // namespace apex3d::three
