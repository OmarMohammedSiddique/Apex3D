#pragma once
#include "generated/monza_safety.hpp"
namespace apex3d::three {
inline bool inGravelPatch(double x,double y,const cp3::GravelPatch& f){
    bool inside=false;
    for(int i=0,j=f.count-1;i<f.count;j=i++){
        const auto a=cp3::gravelPoints[f.first+i],b=cp3::gravelPoints[f.first+j];
        if((a.y>y)!=(b.y>y)&&x<(b.x-a.x)*(y-a.y)/(b.y-a.y)+a.x)inside=!inside;}
    return inside;
}
inline bool inGravel(double x,double y){for(const auto& patch:cp3::gravelPatches)if(inGravelPatch(x,y,patch))return true;return false;}
inline void gravelTraps(Mesh& mesh,const std::vector<cp3::PathSample>& path){
    const double scale=physical::metresPerSceneUnit(),cell=1.4;
    // Raster cells clip concave mapped polygons without covering the track or escape lanes.
    for(const auto& patch:cp3::gravelPatches){double minx=10,miny=10,maxx=-10,maxy=-10;
        for(int i=0;i<patch.count;++i){auto p=cp3::gravelPoints[patch.first+i];minx=std::min(minx,p.x);maxx=std::max(maxx,p.x);miny=std::min(miny,p.y);maxy=std::max(maxy,p.y);}
        for(double x=minx*scale;x<maxx*scale;x+=cell)for(double y=miny*scale;y<maxy*scale;y+=cell){
            if(!inGravelPatch((x+cell/2)/scale,(y+cell/2)/scale,patch))continue;
            const V3 p{x+cell/2,0,-y-cell/2};if(roadDistance(p,path)<physical::trackWidth/2+physical::kerbWidth+1)continue;
            const float grain=static_cast<float>(std::sin(x*13.1+y*7.7)*.025);
            quad(mesh,{x,.025,-y},{x+cell,.025,-y},{x+cell,.025,-y-cell},{x,.025,-y-cell},{.65F+grain,.56F+grain,.40F+grain});
            for(int k=0;k<3;++k){const double px=x+.25+k*.4,pz=-y-.25-(k%2)*.7;
                triangle(mesh,{px,.027,pz},{px+.11,.027,pz+.05},{px+.04,.04,pz+.15},{.75F,.67F,.51F});}
        }
    }
}
// Four small flocks of four (crow/pigeon size, ~1 m wingspan) circle at 16-27 m
// over the track, so they pass through the cockpit, chase and overview cameras. Decorative:
// not a record of wildlife during the race.
inline Mesh birds(const std::vector<cp3::PathSample>& path,double seconds){
    Mesh mesh;const Color feather{.10F,.11F,.12F};
    constexpr std::array<double,4> sites{.04,.30,.69,.86};
    for(std::size_t f=0;f<sites.size();++f){
        const auto pose=cp3::poseAtProgress(path,sites[f]);
        const auto centre=world(pose.position)+rotateY({0,0,(f%2?1.:-1.)*6.0},pose.headingRadians);
        const double radius=26+6.0*f,orbit=seconds*(.10+.015*f)*(f%2?1:-1)+f*1.3;
        for(int b=0;b<4;++b){
            // Loose cluster: each bird trails slightly and bobs on its own phase.
            const double lag=orbit-b*.05*(f%2?1:-1),bob=std::sin(seconds*.9+b*1.7+f)*1.4;
            const V3 p=centre+V3{radius*std::cos(lag)+std::sin(b*2.1)*3,16+2.5*f+bob+(b%3)*1.1,radius*std::sin(lag)+std::cos(b*1.3)*3};
            const double heading=-lag+(f%2?-1:1)*std::numbers::pi/2;
            const double flap=.55*std::sin(seconds*(7.5+b*.4)+b*2.3+f);
            const auto begin=mesh.size();
            sphere(mesh,{0,0,0},{.22,.07,.07},feather,3,6);
            sphere(mesh,{.20,.02,0},{.06,.05,.05},feather,3,5);
            triangle(mesh,{-.18,0,0},{-.32,0,-.07},{-.32,0,.07},feather);
            for(double side:{-1.,1.}){const V3 tip{-.10,flap*.5,side*.52};
                triangle(mesh,{.10,0,side*.03},{-.10,0,side*.06},tip,feather);
                triangle(mesh,{.10,0,side*.03},tip,{-.10,0,side*.06},feather);}
            place(mesh,begin,p,heading);
        }
    }
    return mesh;
}
} // namespace apex3d::three
