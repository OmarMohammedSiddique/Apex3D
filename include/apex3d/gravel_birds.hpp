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
inline Mesh birds(const std::vector<cp3::PathSample>& path,double seconds){
    Mesh mesh;
    for(int i=0;i<2;++i){const auto anchor=world(cp3::poseAtProgress(path,i?.87:.47).position,48+i*12);
        const double angle=seconds*.045+i*2.4;const V3 centre=anchor+V3{70*std::cos(angle),5*std::sin(angle*.7),50*std::sin(angle)};
        const double flap=.45*std::sin(seconds*4.2+i),yaw=-angle;
        const auto begin=mesh.size();const Color feather{.16F,.18F,.20F};
        sphere(mesh,{0,0,0},{.28,.10,.09},feather,3,6);
        for(double side:{-1.,1.}){const V3 tip{-.05,flap,side*.85};triangle(mesh,{.18,0,side*.04},{-.22,0,side*.10},tip,feather);triangle(mesh,{-.22,0,side*.10},{-.30,.1,side*.45},tip,feather);}
        place(mesh,begin,centre,yaw);
    }
    return mesh;
}
} // namespace apex3d::three
