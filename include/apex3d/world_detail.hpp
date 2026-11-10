#pragma once
// Included after the scene mesh primitives.
#include "generated/monza_world.hpp"
#include "generated/monza_safety.hpp"
#include "bitmap_font.hpp"
#include <cstdint>
namespace apex3d::three {
inline void surfaceText(Mesh& mesh,V3 origin,V3 right,V3 up,double cell,const std::string& text,Color color){
    for(char c:text){const auto rows=cp3::glyph(c);
        for(int y=0;y<7;++y)for(int x=0;x<5;++x)if(rows[y]&(1<<(4-x))){
            const auto p=origin+right*(x*cell)+up*((6-y)*cell);
            quad(mesh,p,p+right*cell,p+(right+up)*cell,p+up*cell,color);
        }
        origin=origin+right*(6*cell);
    }
}
inline bool insideFootprint(double x,double y,const MapFootprint& f){
    bool inside=false;
    for(int i=0,j=f.count-1;i<f.count;j=i++){
        const auto a=mapPoints[f.first+i],b=mapPoints[f.first+j];
        if((a.y>y)!=(b.y>y)&&x<(b.x-a.x)*(y-a.y)/(b.y-a.y)+a.x)inside=!inside;
    }
    return inside;
}
inline double roadDistance(V3 p,const std::vector<cp3::PathSample>& path){
    double near=1e20;
    for(std::size_t i=0;i+1<path.size();++i){const auto a=world(path[i].point),b=world(path[i+1].point),d=b-a;
        const double t=std::clamp(dot(p-a,d)/dot(d,d),0.0,1.0);near=std::min(near,length(p-(a+d*t)));}
    return near;
}
inline void mappedBuildings(Mesh& mesh,const std::vector<cp3::PathSample>& path){
    int serial=0;
    for(const auto& f:mapFootprints){if(f.wood||f.count<3)continue;
        std::vector<V3> p;bool blocked=false;V3 centre{};
        for(int i=0;i<f.count;++i){const auto a=mapPoints[f.first+i];const auto v=world({a.x,a.y});
            if(roadDistance(v,path)<14)blocked=true;
            p.push_back(v);centre=centre+v;}
        if(blocked)continue;
        centre=centre*(1.0/p.size());
        double area=0;for(std::size_t i=0;i<p.size();++i){const auto a=p[i],b=p[(i+1)%p.size()];area+=a.z*b.x-a.x*b.z;}
        if(area<0)std::reverse(p.begin(),p.end());
        const double h=f.height;const Color wall=serial++%3?Color{.73F,.69F,.59F}:Color{.60F,.62F,.59F};
        for(std::size_t i=0;i<p.size();++i){const auto a=p[i],b=p[(i+1)%p.size()];
            quad(mesh,a,b,b+V3{0,h,0},a+V3{0,h,0},wall);
            const auto edge=b-a;const double span=length(edge);if(span<1)continue;
            const auto normal=unit(cross(edge,{0,1,0}));
            for(double x=2;x<span-1;x+=3)for(double y=2;y<h-1;y+=3){const auto o=a+edge*(x/span)+V3{0,y,0}+normal*.03;
                quad(mesh,o,o+edge*(1.2/span),o+edge*(1.2/span)+V3{0,1.1,0},o+V3{0,1.1,0},{.14F,.22F,.25F});}
        }
        // Ear-clipped flat roofs preserve concave mapped footprints.
        std::vector<int> remaining;for(std::size_t i=0;i<p.size();++i)remaining.push_back(int(i));
        const auto turn=[](V3 a,V3 b,V3 c){return (b.z-a.z)*(c.x-a.x)-(b.x-a.x)*(c.z-a.z);};
        int guard=0;
        while(remaining.size()>2&&guard++<int(p.size()*p.size())){
            bool cut=false;
            for(std::size_t i=0;i<remaining.size();++i){const int a=remaining[(i+remaining.size()-1)%remaining.size()],b=remaining[i],c=remaining[(i+1)%remaining.size()];
                if(turn(p[a],p[b],p[c])<=1e-8)continue;
                bool occupied=false;
                for(int v:remaining)if(v!=a&&v!=b&&v!=c&&turn(p[a],p[b],p[v])>=0&&turn(p[b],p[c],p[v])>=0&&turn(p[c],p[a],p[v])>=0)occupied=true;
                if(occupied)continue;
                triangle(mesh,p[a]+V3{0,h,0},p[b]+V3{0,h,0},p[c]+V3{0,h,0},f.pitched?Color{.46F,.25F,.17F}:Color{.38F,.40F,.39F});
                remaining.erase(remaining.begin()+i);cut=true;break;
            }
            if(!cut)break;
        }
        if(f.pitched&&p.size()<=6)for(std::size_t i=0;i<p.size();++i)
            triangle(mesh,p[i]+V3{0,h,0},p[(i+1)%p.size()]+V3{0,h,0},centre+V3{0,h+2.4,0},{.51F,.27F,.18F});
    }
}
struct TreePlacement {V3 p;double height,radius;int kind;};
inline std::vector<TreePlacement> woodlandTrees(const std::vector<cp3::PathSample>& path){
    std::vector<TreePlacement> trees;const double scale=physical::metresPerSceneUnit();
    const auto random=[](std::uint32_t v){v^=v>>16;v*=0x7feb352dU;v^=v>>15;v*=0x846ca68bU;v^=v>>16;return v/4294967296.0;};
    // Scatter candidates over the mapped woodland's bounding box.
    double minX=1e9,maxX=-1e9,minY=1e9,maxY=-1e9;
    for(const auto& f:mapFootprints)if(f.wood)for(int j=0;j<f.count;++j){const auto q=mapPoints[f.first+j];
        minX=std::min(minX,q.x);maxX=std::max(maxX,q.x);minY=std::min(minY,q.y);maxY=std::max(maxY,q.y);}
    // The pit/paddock apron runs along the main straight from the finish line to Turn 1.
    const auto turn1=cp3::monzaControls[static_cast<std::size_t>(cp3::turn1Fraction*cp3::monzaControls.size())];
    const double straightY=cp3::monzaControls[0].y;
    for(int i=0;i<20000;++i){const double x=minX+(maxX-minX)*random(i*2+105),y=minY+(maxY-minY)*random(i*2+106);
        bool woods=false,blocked=false;
        for(const auto& f:mapFootprints){if(f.wood){if(insideFootprint(x,y,f))woods=true;}
            else if(insideFootprint(x,y,f))blocked=true;}
        // Preserve the pit/paddock apron and spectator facilities.
        if(!woods||blocked||(y<straightY&&x>turn1.x-.05))continue;
        bool runoff=false;
        for(const auto& f:cp3::gravelPatches){bool inside=false;for(int a=0,b=f.count-1;a<f.count;b=a++){const auto u=cp3::gravelPoints[f.first+a],v=cp3::gravelPoints[f.first+b];if((u.y>y)!=(v.y>y)&&x<(v.x-u.x)*(y-u.y)/(v.y-u.y)+u.x)inside=!inside;}if(inside)runoff=true;}
        if(runoff)continue;
        const V3 p{x*scale,0,-y*scale};if(roadDistance(p,path)<23)continue;
        for(const auto& stand:monzaStands){const auto centre=world({stand.x,stand.y});
            if(length(p-centre)<stand.length/2+stand.setback+12){blocked=true;break;}}
        if(blocked)continue;
        // A canopy-radius margin keeps crowns out of buildings.
        for(const auto& f:mapFootprints)if(!f.wood)for(int j=0;j<f.count;++j){const auto q=mapPoints[f.first+j];
            if(std::hypot(x-q.x,y-q.y)*scale<10)blocked=true;}
        if(blocked)continue;
        trees.push_back({p,11+12*std::fmod(i*.731,1.0),4.0+2.5*random(i+90001),i%4});
    }
    return trees;
}
inline void broadleafTree(Mesh& mesh,const TreePlacement& t){
    const double h=t.height,r=t.radius;const auto p=t.p;const double crown=t.kind==1?.60:.64;
    tube(mesh,p,p+V3{.25,h*.67,0},.18+h*.006,{.32F,.26F,.18F},6);
    for(int k=0;k<4;++k){const double a=k*1.570796+std::fmod(h,1.0),spread=r*(k==0?.18:.65);
        const auto branch=p+V3{std::cos(a)*spread,h*(crown+.07*(k%2)),std::sin(a)*spread};
        tube(mesh,p+V3{0,h*.42,0},branch,.12,{.32F,.26F,.18F},5);
        const float shade=.018F*float((k+t.kind)%4);
        sphere(mesh,branch,{r*.85,h*(t.kind==1?.30:.26),r*.83},{.16F+shade,.32F+shade,.13F+shade},4,7);
    }
    sphere(mesh,p+V3{0,h*.81,0},{r*.80,h*.21,r*.78},{.23F,.40F,.17F},4,7);
}
inline int logoIndex(int number){
    switch(number){case 1:case 22:return 8;case 4:case 81:return 5;case 16:case 44:return 2;
    case 63:case 12:return 6;case 23:case 55:return 9;case 5:case 27:return 4;
    case 6:case 30:return 7;case 87:case 31:return 3;case 10:case 43:return 0;case 14:case 18:return 1;default:return -1;}
}
inline void logoDecal(Mesh& mesh,int number,V3 origin,V3 right,V3 up,double width,double height){
    const int index=logoIndex(number);if(index<0)return;const auto& logo=teamLogos[index];
    const double pixel=std::min(width/logo.width,height/logo.height);
    origin=origin+right*((width-logo.width*pixel)/2)+up*((height-logo.height*pixel)/2);
    for(int i=0;i<logo.count;++i){const auto& run=logoRuns[logo.first+i];
        const auto p=origin+right*(run.x*pixel)+up*((logo.height-1-run.y)*pixel);
        quad(mesh,p,p+right*(run.width*pixel),p+right*(run.width*pixel)+up*pixel,p+up*pixel,
            {run.r/255.F,run.g/255.F,run.b/255.F});}
}
} // namespace apex3d::three
