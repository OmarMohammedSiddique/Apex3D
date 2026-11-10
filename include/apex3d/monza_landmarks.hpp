#pragma once
// Included from scene3d.hpp after the mesh primitives, world detail and grandstands.
// Monza features that make the circuit recognisable: the 1955 banking, trackside
// advertising with catch fencing, and the podium that overhangs the main straight.
#include "generated/monza_world.hpp"
#include <array>
#include <string_view>
namespace apex3d::three {

// ---------------------------------------------------------------------------
// The 1955 high-speed ring ("Sopraelevata"). Centrelines are OpenStreetMap ways
// (data/monza-oval.json). Width, banking height and bridge height are
// representative values, not a survey. The two bridge ways carry the oval over
// the Grand Prix track; the approaches ramp up on embankments.
// ---------------------------------------------------------------------------
inline constexpr double ovalWidth=12.0,ovalBridgeHeight=6.5,ovalRampLength=110.0;
inline double ovalBaseHeight(V3 p){
    // Raised near the bridges, falling smoothly to ground level along the embankments.
    double near=1e20;
    for(const auto& w:ovalWays)if(w.bridge)for(int i=0;i+1<w.count;++i)
        near=std::min(near,distanceToSegment({p.x,0,p.z},ovalPoint(w.first+i),ovalPoint(w.first+i+1)));
    const double t=std::clamp(1.0-near/ovalRampLength,0.0,1.0);
    return ovalBridgeHeight*t*t*(3-2*t);
}
inline void ovalBanking(Mesh& mesh,const std::vector<cp3::PathSample>& path){
    constexpr Color concrete{.56F,.56F,.52F},weathered{.47F,.49F,.44F},wall{.50F,.50F,.47F},rail{.62F,.64F,.62F};
    for(const auto& w:ovalWays){
        // Densify each way to ~6 m so banking and ramps vary smoothly.
        std::vector<V3> pts;
        for(int i=0;i+1<w.count;++i){const auto a=ovalPoint(w.first+i),b=ovalPoint(w.first+i+1);
            const int steps=std::max(1,static_cast<int>(length(b-a)/6));
            for(int k=0;k<steps;++k)pts.push_back(a+(b-a)*(double(k)/steps));}
        pts.push_back(ovalPoint(w.first+w.count-1));
        if(pts.size()<2)continue;
        const auto n=pts.size();
        std::vector<V3> left(n),right(n);std::vector<double> base(n),turn(n,0.0);
        for(std::size_t i=0;i<n;++i){
            const auto t=unit(pts[std::min(i+1,n-1)]-pts[i>0?i-1:0]);
            const V3 l{t.z,0,-t.x};left[i]=l;
            base[i]=w.bridge?ovalBridgeHeight:ovalBaseHeight(pts[i]);
            if(i>0&&i+1<n){const auto a=unit(pts[i]-pts[i-1]),b=unit(pts[i+1]-pts[i]);
                turn[i]=(a.z*b.x-a.x*b.z)/std::max(length(pts[i+1]-pts[i-1])/2,1.0);} // signed curvature (1/m), + = left
        }
        for(std::size_t pass=0;pass<4;++pass){auto copy=turn;for(std::size_t i=1;i+1<n;++i)turn[i]=(copy[i-1]+copy[i]+copy[i+1])/3;}
        for(std::size_t i=0;i+1<n;++i){
            // Skip at-grade pieces that would sit on the Grand Prix road.
            const auto mid=(pts[i]+pts[i+1])*0.5;
            if(!w.bridge&&base[i]<0.5&&roadDistance(mid,path)<physical::trackWidth/2+ovalWidth/2+1)continue;
            // Outer edge rises with curvature: about 6 m on the ~320 m radius curves.
            const auto edge=[&](std::size_t k,double side){
                const double bank=std::clamp(std::abs(turn[k])*2000.0,0.0,7.0);
                const bool outer=(turn[k]>0)==(side<0);
                return pts[k]+left[k]*(side*ovalWidth/2)+V3{0,base[k]+.15+(outer?bank:0.0),0};};
            const auto al=edge(i,1),ar=edge(i,-1),bl=edge(i+1,1),br=edge(i+1,-1);
            quad(mesh,ar,br,bl,al,(i/6)%2?concrete:weathered);
            // Solid embankment / banking walls down to the ground (open under bridges).
            for(const auto& [a,b]:std::array<std::pair<V3,V3>,2>{{{al,bl},{br,ar}}}){
                if(w.bridge)continue;
                if(a.y<.4&&b.y<.4)continue;
                quad(mesh,{a.x,0,a.z},{b.x,0,b.z},b,a,wall);
            }
            // Old steel guardrail along the top of both edges.
            for(const auto& [a,b]:std::array<std::pair<V3,V3>,2>{{{al,bl},{ar,br}}})
                quad(mesh,a+V3{0,.45,0},b+V3{0,.45,0},b+V3{0,.85,0},a+V3{0,.85,0},rail);
        }
        if(w.bridge){
            // Deck slab, parapets and four piers clear of the road below.
            for(std::size_t i=0;i+1<n;++i){
                const auto a=pts[i]+V3{0,base[i]-.9,0},b=pts[i+1]+V3{0,base[i+1]-.9,0};
                quad(mesh,a+left[i]*(-ovalWidth/2),a+left[i]*(ovalWidth/2),b+left[i+1]*(ovalWidth/2),b+left[i+1]*(-ovalWidth/2),wall);}
            for(std::size_t end:{std::size_t{0},n-1})for(double side:{-1.,1.}){
                const auto p=pts[end]+left[end]*(side*(ovalWidth/2-1.2));
                box(mesh,p+V3{-.6,0,-.6},p+V3{.6,base[end]-.9,.6},wall);}
        }
    }
}

// ---------------------------------------------------------------------------
// Trackside advertising and catch fencing.
// Boards carry the names of Formula 1's global partners for the 2025 season,
// in plain lettering on brand-like colours. Exact board designs and positions
// are representative, not a reconstruction of the race-day signage plan.
// ---------------------------------------------------------------------------
struct AdBrand {std::string_view name;Color board,text;};
inline constexpr std::array<AdBrand,11> adBrands{{
    {"PIRELLI",{.05F,.05F,.06F},{1.F,.82F,.0F}},
    {"ARAMCO",{.0F,.42F,.66F},{1.F,1.F,1.F}},
    {"DHL",{1.F,.80F,.0F},{.83F,.07F,.14F}},
    {"HEINEKEN 0.0",{.0F,.38F,.86F},{1.F,1.F,1.F}},
    {"AWS",{.13F,.17F,.23F},{1.F,.60F,.0F}},
    {"LENOVO",{.89F,.10F,.16F},{1.F,1.F,1.F}},
    {"TAG HEUER",{.06F,.06F,.06F},{.92F,.92F,.92F}},
    {"LOUIS VUITTON",{.33F,.23F,.16F},{.92F,.84F,.66F}},
    {"MSC CRUISES",{.0F,.13F,.33F},{1.F,1.F,1.F}},
    {"QATAR AIRWAYS",{.36F,.04F,.21F},{1.F,1.F,1.F}},
    {"SALESFORCE",{.0F,.60F,.86F},{1.F,1.F,1.F}},
}};
inline void adBoard(Mesh& mesh,V3 centre,V3 outward,double length,double height,const AdBrand& brand){
    const V3 up{0,1,0},right=unit(cross(outward,up)),half=right*(length/2);
    const auto base=centre+V3{0,.12,0};
    // Face (toward the track), back and top.
    quad(mesh,base-half,base+half,base+half+up*height,base-half+up*height,brand.board);
    const auto back=outward*.06;
    quad(mesh,base+half+back,base-half+back,base-half+back+up*height,base+half+back+up*height,{.30F,.32F,.33F});
    quad(mesh,base-half+up*height,base+half+up*height,base+half+back+up*height,base-half+back+up*height,{.22F,.24F,.25F});
    const double cell=std::min(height*.11,(length*.86)/(double(brand.name.size())*6-1));
    const double textWidth=(double(brand.name.size())*6-1)*cell;
    surfaceText(mesh,base-right*(textWidth/2)+up*((height-7*cell)/2)-outward*.012,right,up,cell,std::string(brand.name),brand.text);
}
inline void trackside(Mesh& mesh,const std::vector<cp3::PathSample>& path){
    const double scale=physical::metresPerSceneUnit(),lap=path.back().distance;
    // Boards gather where spectators and TV cameras are: within 260 m of a grandstand.
    std::vector<double> stands;for(const auto& s:monzaStands)stands.push_back(standPose(path,s).distance*scale);
    const auto nearStand=[&](double metres){for(double s:stands){double d=std::abs(metres-s);d=std::min(d,lap*scale-d);if(d<260)return true;}return false;};
    int serial=0;
    for(double metres=20;metres<lap*scale-10;metres+=9.0){
        const double progress=metres/(lap*scale);
        const bool armco=progress>=0.13&&progress<=0.94;
        const auto pose=cp3::poseAtProgress(path,progress);
        for(double side:{-1.0,1.0}){
            // The main straight has the pit wall on one side; boards go on the grandstand side there.
            if(!armco&&side>0)continue;
            if(!nearStand(metres)&&serial++%3)continue;
            const auto p=world(pose.position)+rotateY({0,0,side*11.35},pose.headingRadians);
            const auto outward=rotateY({0,0,side},pose.headingRadians);
            adBoard(mesh,p,outward,8.4,1.05,adBrands[static_cast<std::size_t>(std::abs(static_cast<int>(metres/9)+(side>0?5:0)))%adBrands.size()]);
        }
    }
    // Debris (catch) fence above the Armco: posts every 4 m, cables, and a dark mesh band.
    constexpr Color post{.40F,.43F,.43F},cable{.55F,.58F,.58F},meshBand{.20F,.23F,.24F};
    for(std::size_t i=0;i+1<path.size();++i){
        const double progress=path[i].distance/path.back().distance;
        if(progress<0.13||progress>0.94)continue;
        for(double side:{-1.0,1.0}){
            const auto a=world(cp3::offset(path[i],physical::sceneUnits(side*12.3))),b=world(cp3::offset(path[i+1],physical::sceneUnits(side*12.3)));
            if(length(b-a)<1e-6)continue;
            for(double h:{1.6,2.6,3.6})tube(mesh,a+V3{0,h,0},b+V3{0,h,0},.012,cable,4);
            quad(mesh,a+V3{0,3.15,0},b+V3{0,3.15,0},b+V3{0,3.55,0},a+V3{0,3.55,0},meshBand);
            if(i%2==0)tube(mesh,a,a+V3{0,3.8,0},.05,post,5);
        }
    }
}

// ---------------------------------------------------------------------------
// The podium. At Monza it is cantilevered from the pit building out over the
// main straight, so the crowd below can fill the track during the ceremony.
// Dimensions are representative.
// ---------------------------------------------------------------------------
inline constexpr double podiumAlong=-55.0,podiumHeight=9.0;
inline V3 podiumCentre(const std::vector<cp3::PathSample>& path){
    const auto origin=cp3::poseAtProgress(path,0);
    return world(origin.position,physical::roadHeight)+rotateY({podiumAlong,podiumHeight,8.0},origin.headingRadians);
}
inline void podium(Mesh& mesh,const std::vector<cp3::PathSample>& path){
    const auto start=mesh.size();
    constexpr Color steel{.30F,.33F,.35F},deck{.82F,.83F,.80F},glass{.20F,.30F,.36F},red{.78F,.06F,.10F};
    const double x=podiumAlong,y=podiumHeight;
    // Cantilevered deck from the pit side (+z) reaching over the pit lane and half the track.
    box(mesh,{x-9,y-.7,1.5},{x+9,y,30},deck);
    box(mesh,{x-9.2,y-1.6,1.3},{x+9.2,y-.7,1.7},red);                 // fascia over the track
    box(mesh,{x-9,y,1.5},{x+9,y+1.1,1.7},glass);                      // glass balustrade
    surfaceText(mesh,{x+8.3,y-1.45,1.28},{-1,0,0},{0,1,0},.075,"PIRELLI GRAN PREMIO D ITALIA",{1,1,1});
    // Steps: 2nd, 1st, 3rd from left as seen from the track.
    for(int rank=1;rank<=3;++rank){const double cx=x+(rank==1?0:rank==2?-3.2:3.2),h=rank==1?1.2:rank==2?.8:.5;
        box(mesh,{cx-1.4,y,4.0},{cx+1.4,y+h,6.5},rank==1?Color{.94F,.72F,.21F}:rank==2?Color{.72F,.77F,.79F}:Color{.74F,.43F,.25F});}
    // Backdrop and supports rising from the pit building side.
    box(mesh,{x-9,y,8},{x+9,y+5,8.4},{.10F,.12F,.14F});
    for(double px:{x-8.5,x+8.5})box(mesh,{px-.4,0,26},{px+.4,y,27},steel);
    const auto origin=cp3::poseAtProgress(path,0);
    place(mesh,start,world(origin.position,physical::roadHeight),origin.headingRadians);
}
} // namespace apex3d::three
