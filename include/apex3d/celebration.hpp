#pragma once
// Included from scene3d.hpp after monza_landmarks.hpp.
// After every car has parked: the Monza track invasion. Fans pour onto the main
// straight and gather under the podium, which overhangs the track; the top three
// appear on the steps, then champagne and confetti. This follows Monza's real
// post-race tradition (the 2025 crowd invaded the straight beneath the podium),
// but every person, path and timing here is illustrative.
#include <array>
#include <cstdint>
namespace apex3d::three {
inline constexpr int invasionFans=640;
inline double celebrationNoise(std::uint32_t v){v^=v>>16;v*=0x7feb352dU;v^=v>>15;v*=0x846ca68bU;v^=v>>16;return v/4294967296.0;}
// A low-poly standing person (1.75 m), arms either down or raised.
inline void fan(Mesh& mesh,V3 p,double yaw,Color shirt,bool armsUp,double jump){
    const auto begin=mesh.size();const Color skin{.80F,.60F,.45F},trousers{.13F,.15F,.20F};
    const double y=jump;
    box(mesh,{-.10,y,-.17},{.10,y+.80,-.03},trousers);box(mesh,{-.10,y,.03},{.10,y+.80,.17},trousers);
    box(mesh,{-.13,y+.80,-.21},{.13,y+1.42,.21},shirt);
    box(mesh,{-.10,y+1.45,-.10},{.10,y+1.70,.10},skin);
    for(double side:{-1.,1.}){
        if(armsUp)box(mesh,{-.05,y+1.35,side*.21-.05},{.05,y+1.98,side*.21+.05},shirt);
        else box(mesh,{-.05,y+.82,side*.24-.05},{.05,y+1.40,side*.24+.05},shirt);
    }
    place(mesh,begin,p,yaw);
}
inline Mesh celebration(const std::vector<cp3::PathSample>& path,double seconds){
    Mesh mesh;if(seconds<0)return mesh;
    const auto origin=cp3::poseAtProgress(path,0);
    const auto base=world(origin.position,physical::roadHeight);
    const auto local=[&](double along,double y,double lateral){return base+rotateY({along,y,lateral},origin.headingRadians);};
    // --- Track invasion: fans enter from the grandstand side over ~25 s ---
    for(int i=0;i<invasionFans;++i){
        const auto r=[&](int k){return celebrationNoise(static_cast<std::uint32_t>(i*7+k)*2654435761U);};
        const double startAlong=-260+340*r(1),startLat=-15-4*r(2);
        // Crowd density peaks under the podium (along = podiumAlong).
        const double spread=std::pow(r(3),1.6)*70*(r(4)<.5?-1:1);
        const double endAlong=podiumAlong+spread,endLat=-6.5+13.0*r(5);
        const double depart=r(6)*24,speed=3.6+2.2*r(7);
        const double travel=std::hypot(endAlong-startAlong,endLat-startLat);
        const double t=std::clamp((seconds-depart)*speed/travel,0.0,1.0);
        if(seconds<depart)continue;
        const double along=startAlong+(endAlong-startAlong)*t,lat=startLat+(endLat-startLat)*t;
        const bool arrived=t>=1;
        const double run=arrived?0:std::abs(std::sin((seconds-depart)*9+i))*.08;
        const double jump=arrived&&r(8)<.55?std::max(0.0,std::sin(seconds*5.5+i*.7))*.28:run;
        // Arrived fans face the podium (pit side); runners face where they are going.
        const double yaw=origin.headingRadians+(arrived?-std::numbers::pi/2:std::atan2(-(endLat-startLat),endAlong-startAlong));
        const double c=r(9);
        const Color shirt=c<.62?Color{.80F,.05F,.08F}:c<.74?Color{1.F,.50F,.05F}:c<.84?Color{.10F,.16F,.36F}:c<.93?Color{.94F,.94F,.92F}:Color{.95F,.80F,.10F};
        fan(mesh,local(along,0,lat),yaw,shirt,arrived&&r(10)<.7,jump);
        // About one fan in eleven carries a flag: plain red, or the Italian tricolore.
        if(i%11==0){
            const auto hand=local(along,1.9+jump,lat),top=hand+V3{0,2.4,0};
            tube(mesh,hand,top,.025,{.75F,.75F,.72F},4);
            const double wave=std::sin(seconds*4+i)*.35;
            const auto dir=rotateY({1,0,0},origin.headingRadians+wave);
            const bool tricolore=i%22==0;
            for(int band=0;band<3;++band){
                const Color colour=tricolore?(band==0?Color{.0F,.55F,.27F}:band==1?Color{.95F,.95F,.95F}:Color{.81F,.10F,.15F}):Color{.82F,.04F,.08F};
                const auto a=top+dir*(band*.55),b=top+dir*((band+1)*.55);
                quad(mesh,a,b,b+V3{0,-.85,0},a+V3{0,-.85,0},colour);quad(mesh,b,a,a+V3{0,-.85,0},b+V3{0,-.85,0},colour);
            }
        }
    }
    // --- The top three on the podium steps (from 20 s) ---
    if(seconds>=20){
        constexpr std::array<Color,3> suits{{{.07F,.11F,.30F},{1.F,.50F,.05F},{1.F,.50F,.05F}}}; // VER Red Bull, NOR / PIA McLaren
        for(int rank=1;rank<=3;++rank){
            const double cx=podiumAlong+(rank==1?0:rank==2?-3.2:3.2),h=rank==1?1.2:rank==2?.8:.5;
            const bool celebrate=std::fmod(seconds+rank,4.0)<2.6;
            fan(mesh,local(cx,podiumHeight+h,5.2),origin.headingRadians-std::numbers::pi/2,suits[rank-1],celebrate,0);
        }
    }
    // --- Champagne (from 34 s): arcs of droplets sprayed over the crowd ---
    if(seconds>=34&&seconds<60){
        for(int rank=1;rank<=3;++rank){
            const double cx=podiumAlong+(rank==1?0:rank==2?-3.2:3.2),h=rank==1?1.2:rank==2?.8:.5;
            const double sweep=std::sin(seconds*1.3+rank*2)*.8;
            for(int k=0;k<40;++k){
                const double age=std::fmod(seconds*1.7+k*.025,1.0),speed=11.0;
                const V3 dir=unit(V3{std::sin(sweep)*.6,.55,-std::cos(sweep)});
                const auto p=local(cx,podiumHeight+h+1.6,4.8)+rotateY(dir,origin.headingRadians)*(speed*age)+V3{0,-4.9*age*age,0};
                sphere(mesh,p,{.06,.06,.06},{.98F,.93F,.70F},2,4);
            }
        }
    }
    // --- Confetti (from 30 s): tricolore and gold flakes drifting down over the crowd ---
    if(seconds>=30){
        for(int k=0;k<520;++k){
            const auto r=[&](int j){return celebrationNoise(static_cast<std::uint32_t>(k*13+j+9001)*2246822519U);};
            const double cycle=9+4*r(1),age=std::fmod(seconds-30+r(2)*cycle,cycle)/cycle;
            const double along=podiumAlong-45+90*r(3)+std::sin(seconds*.8+k)*1.5,lat=-9+20*r(4);
            const double y=podiumHeight+6-(podiumHeight+6)*age;
            const double spin=seconds*3+k;
            const double c=r(5);
            const Color colour=c<.25?Color{.0F,.55F,.27F}:c<.5?Color{.95F,.95F,.95F}:c<.75?Color{.81F,.10F,.15F}:Color{.95F,.78F,.20F};
            const auto p=local(along,y,lat);
            const V3 a{std::cos(spin)*.07,std::sin(spin*.7)*.05,std::sin(spin)*.07},b{-std::sin(spin)*.05,.04,std::cos(spin)*.05};
            quad(mesh,p-a-b,p+a-b,p+a+b,p-a+b,colour);quad(mesh,p-a+b,p+a+b,p+a-b,p-a-b,colour);
        }
    }
    return mesh;
}
} // namespace apex3d::three
