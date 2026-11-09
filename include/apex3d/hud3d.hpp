#pragma once
#include "cp3_hud.hpp"
#include "scene3d.hpp"
#include "race_telemetry.hpp"
namespace apex3d::three {
inline cp3::HudAction focusAction(cp3::DriverFocus focus){return static_cast<cp3::HudAction>(100+static_cast<int>(focus));}
inline constexpr double driverRowTop(int row){return -0.815-row*0.053;}
inline cp3::HudAction overlayAction(double x,double y,bool visible=true,int page=0) {
    if(!visible||x<0.45)return cp3::HudAction::None;
    page=std::clamp(page,0,6);
    if(y>=-0.805&&y<=-0.775){
        if(x>=0.85&&x<0.90)return cp3::HudAction::FieldPrevious;
        if(x>=0.90&&x<=0.96)return cp3::HudAction::FieldNext;
    }
    if(x>=0.465&&x<=0.955)for(int row=0;row<3;++row) {
        const int index=page*3+row;
        if(index<20&&y<=driverRowTop(row)&&y>=driverRowTop(row)-0.043)
            return focusAction(cp3::raceDrivers[index].focus);
    }
    return cp3::hudActionAt(0.17+(x-0.45)*1.5,y);
}
struct GaugeLevels {double speed,rpm;int gear;};
inline GaugeLevels gaugeLevels(const cp3::RecordedTelemetry& data) {
    return {std::clamp(data.speedKmh/400.0,0.0,1.0),std::clamp(data.engineRpm/15000.0,0.0,1.0),std::clamp(data.gear,0,8)};
}
inline void speedGauge(std::vector<cp3::Vertex>& mesh,const cp3::RecordedTelemetry& data,bool valid,
                       double aspect,cp3::Color accent) {
    constexpr cp3::Color white{0.94F,0.97F,0.97F},muted{0.55F,0.68F,0.72F};
    aspect=std::clamp(aspect,0.8,1.85);
    constexpr double x=0.59,y=-0.645,r=0.085;
    const auto point=[&](double angle,double radius){return Vec2{x+radius*std::cos(angle),y+radius*std::sin(angle)*aspect};};
    const auto level=gaugeLevels(data);
    // A calibrated 0-400 km/h dial. Segment fills and the needle use the same raw sample.
    for(int i=0;i<40;++i) {
        const double a=std::numbers::pi*(1.0-i/40.0),b=std::numbers::pi*(1.0-(i+0.85)/40.0);
        const bool lit=valid&&(i+0.5)/40.0<=level.speed;
        cp3::quad(mesh,point(a,r),point(b,r),point(b,r-0.007),point(a,r-0.007),lit?accent:cp3::Color{0.12F,0.20F,0.24F});
    }
    for(int i=0;i<=4;++i) {
        const double angle=std::numbers::pi*(1.0-i/4.0),offset=0.010;
        cp3::quad(mesh,point(angle-0.012,r-offset),point(angle+0.012,r-offset),
            point(angle+0.012,r-offset-0.006),point(angle-0.012,r-offset-0.006),muted);
    }
    if(valid) {
        const double angle=std::numbers::pi*(1-level.speed);
        cp3::triangle(mesh,point(angle+std::numbers::pi/2,0.003),point(angle,r-0.015),point(angle-std::numbers::pi/2,0.003),white);
        const auto first=mesh.size();cp3::disk(mesh,{x,y},0.004,accent);
        for(auto i=first;i<mesh.size();++i)mesh[i].y=static_cast<float>(y+(mesh[i].y-y)*aspect);
    }
    cp3::hudText(mesh,0.488,-0.650,0.0018,"0",muted);
    cp3::hudText(mesh,0.687,-0.650,0.0018,"400",muted);
    const auto value=valid?std::to_string(data.speedKmh):"---";
    cp3::hudText(mesh,x-value.size()*0.0048*3,-0.665,0.0048,value,white);
    cp3::hudText(mesh,x-0.023,-0.703,0.0019,"KM/H",muted);
}
inline void telemetryGraphic(std::vector<cp3::Vertex>& mesh,cp3::DriverFocus focus,double seconds,
                             bool waiting,double aspect) {
    const auto& info=cp3::driverInfo(focus);
    const auto data=cp3::telemetryAt(seconds,static_cast<int>(focus));const bool valid=data.available&&!waiting&&cp3::driverActive(focus,seconds);
    const auto level=gaugeLevels(data);
    constexpr cp3::Color white{0.94F,0.97F,0.97F},muted{0.55F,0.68F,0.72F};
    cp3::hudText(mesh,0.475,-0.468,0.0025,std::string(info.tag)+" / RECORDED TELEMETRY",info.color);
    speedGauge(mesh,data,valid,aspect,info.color);
    cp3::hudText(mesh,0.798,-0.513,0.0027,"GEAR",muted);
    cp3::hudRectangle(mesh,0.78,-0.685,0.94,-0.54,{0.075F,0.14F,0.18F});
    cp3::hudText(mesh,0.83,-0.550,0.012,valid?(data.gear?std::to_string(data.gear):"N"):"-",white);
    for(int i=0;i<8;++i) {
        const double left=0.795+i*0.017;
        cp3::hudRectangle(mesh,left,-0.671,left+0.013,-0.657,
            valid&&i<level.gear?info.color:cp3::Color{0.15F,0.23F,0.27F});
    }
    cp3::hudText(mesh,0.475,-0.728,0.0022,valid?"ENGINE "+std::to_string(data.engineRpm)+" RPM":
        info.archive->dns?"DID NOT START":!cp3::driverActive(focus,seconds)?"RETIRED":waiting?"WAITING FOR LIGHTS":"TELEMETRY GAP",white);
    cp3::hudText(mesh,0.905,-0.728,0.0018,"15K",muted);
    for(int i=0;i<20;++i) {
        const double left=0.475+i*0.0235;
        const cp3::Color color=i<14?info.color:i<18?cp3::Color{0.98F,0.74F,0.25F}:cp3::Color{0.94F,0.28F,0.27F};
        cp3::hudRectangle(mesh,left,-0.768,left+0.0195,-0.752,
            valid&&(i+0.5)/20.0<=level.rpm?color:cp3::Color{0.12F,0.19F,0.23F});
    }
}
inline std::vector<cp3::Vertex> overlay(const cp3::Playback& p,const cp3::RaceSample& sample,
    double pathLength,double aspect,bool audio,CameraMode camera,bool visible,cp3::HudAction hover,cp3::DriverFocus focus=cp3::DriverFocus::Verstappen,int page=0) {
    auto displayed=p;displayed.showWheels=false;
    auto source=p.isRace()?cp3::makeRaceHudMesh(displayed,sample,pathLength,aspect,audio,hover,focus):
        cp3::makeHudMesh(p.progress,p.speed,p.playing,p.scaleDemo,p.route,p.showWheels,p.travelled*physical::metresPerSceneUnit()/cp3::raceTyreCircumference*cp3::wheelCircumference,aspect,p.looping,
            p.playing?pathLength*physical::metresPerSceneUnit()*p.speed/cp3::lapSeconds/cp3::raceTyreCircumference*60.0:0.0);
    std::vector<cp3::Vertex> result;
    for(std::size_t i=0;i<source.size();i+=3) {
        const bool lights=source[i].x<0.17F;
        if(lights&&p.started&&p.elapsed>3.0)continue;
        if(!lights&&!visible)continue;
        for(std::size_t j=0;j<3;++j) {
            auto v=source[i+j];
            if(lights){v.x=static_cast<float>((v.x+0.565)*0.75+(visible?-0.285:0.0));v.y=static_cast<float>((v.y-0.775)*0.75+0.80);}
            else v.x=static_cast<float>(0.45+(v.x-0.17)*(2.0/3.0));
            result.push_back(v);
        }
    }
    if(p.isRace()&&visible) {
        cp3::hudRectangle(result,0.45,-0.975,0.97,-0.455,{0.035F,0.075F,0.11F});
        telemetryGraphic(result,focus,p.elapsed,!p.started||p.starting(),aspect);
        page=std::clamp(page,0,6);
        cp3::hudText(result,0.475,-0.790,0.0020,"FINAL CLASSIFICATION "+std::to_string(page+1)+"/7",{0.55F,0.68F,0.72F});
        cp3::hudText(result,0.86,-0.790,0.0025,"<   >",{0.94F,0.97F,0.97F});
        for(int row=0;row<3;++row) {
            const int index=page*3+row;if(index>=20)break;
            const auto& info=cp3::raceDrivers[index];const auto& entry=*info.archive;const double top=driverRowTop(row);
            const bool selected=focus==info.focus;
            cp3::hudRectangle(result,0.465,top-0.043,0.955,top,
                selected?cp3::Color{0.08F,0.19F,0.23F}:hover==focusAction(info.focus)?cp3::Color{0.10F,0.16F,0.19F}:cp3::Color{0.045F,0.09F,0.12F});
            if(selected)cp3::hudRectangle(result,0.465,top-0.043,0.47,top,info.color);
            cp3::hudText(result,0.482,top-0.012,0.0021,(entry.dnf||entry.dns?std::string("NC"):std::to_string(index+1))+" "+info.name,
                selected?info.color:cp3::Color{0.72F,0.78F,0.80F});
            std::ostringstream resultText;resultText<<std::fixed<<std::setprecision(3);
            if(entry.dns)resultText<<"DNS";
            else if(entry.dnf)resultText<<"DNF";
            else if(entry.totalLaps<53)resultText<<"+1 LAP";
            else if(index==0)resultText<<"WINNER";
            else resultText<<"+"<<entry.classified-cp3::raceTotalSeconds<<"S";
            cp3::hudText(result,0.82,top-0.014,0.0019,resultText.str(),
                selected?info.color:cp3::Color{0.55F,0.68F,0.72F});
        }
    }
    if(p.isRace()&&p.showWheels&&visible) {
        std::vector<cp3::Vertex> diagram;
        const auto tyres=cp3::tyreState(focus,p.elapsed);
        cp3::wheelInspector(diagram,sample.travelledMetres/cp3::raceTyreCircumference*cp3::wheelCircumference,aspect,
            p.playing&&!p.starting()?cp3::raceWheelRpm(sample.velocity*p.speed,pathLength):0,cp3::driverInfo(focus).color,&tyres);
        for(auto v:diagram){v.x=static_cast<float>((v.x-0.17)*0.75-0.97);v.y=static_cast<float>(v.y*0.75-0.60);result.push_back(v);}
    }
    cp3::hudRectangle(result,-0.985,-0.985,visible?0.42:0.985,-0.825,{0.035F,0.075F,0.11F});
    cp3::hudText(result,-0.95,-0.85,0.0040,std::string("1 UNIT = 1 M / ")+cameraName(camera)+"   C SWITCH CAMERA",{0.94F,0.97F,0.97F});
    cp3::hudText(result,-0.95,-0.925,0.0035,"DRAG ORBIT  SCROLL ZOOM  F RESET  H DASHBOARD",{0.62F,0.73F,0.76F});
    return result;
}
struct MarkerPosition {double x,y;bool onScreen;};
inline MarkerPosition markerPosition(const CameraFrame& camera,const cp3::Pose& pose,double sceneFraction) {
    const auto clip=camera.viewProjection.transform(world(pose.position,1.1));
    if(std::abs(clip[3])<1e-8)return {2*sceneFraction-1,0,false};
    const double x=clip[0]/std::abs(clip[3]),y=clip[1]/std::abs(clip[3]);
    return {(x+1)*sceneFraction-1,y,clip[3]>0&&std::abs(x)<1&&std::abs(y)<1};
}
inline void overviewMarker(std::vector<cp3::Vertex>& mesh,const CameraFrame& camera,const cp3::Pose& pose,
                           double fraction,double aspect,const std::string& label,cp3::Color color,bool focus) {
    const auto point=markerPosition(camera,pose,fraction);
    // Offscreen cars get an edge indicator; the focused car remains the zoom target.
    const double x=std::clamp(point.x,-0.94,2*fraction-1-0.19);
    const double lift=focus?0.0:label=="NOR 4"?0.11:0.22;
    const double y=std::clamp(point.y+lift,-0.72,0.80);
    const double tipY=std::clamp(point.y,-0.72,0.80);
    const double r=focus?0.010:0.007;
    const auto first=mesh.size();cp3::disk(mesh,{x,tipY},r,{0.02F,0.04F,0.06F});
    for(auto i=first;i<mesh.size();++i)mesh[i].y=static_cast<float>(tipY+(mesh[i].y-tipY)*aspect);
    cp3::triangle(mesh,{x,tipY+0.009*aspect},{x-0.010,tipY+0.030*aspect},{x+0.010,tipY+0.030*aspect},color);
    cp3::hudRectangle(mesh,x-0.0015,tipY+0.025*aspect,x+0.0015,y+0.035*aspect,color);
    cp3::hudRectangle(mesh,x-0.025,y+0.035*aspect,x+0.17,y+0.073*aspect,{0.035F,0.075F,0.11F});
    cp3::hudText(mesh,x-0.017,y+0.060*aspect,0.0026,point.onScreen?label:label+" OFF",color);
}
} // namespace apex3d::three
