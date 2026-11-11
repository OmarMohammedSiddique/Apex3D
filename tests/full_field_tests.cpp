#include "../include/apex3d/field_simulation.hpp"
#include "../include/apex3d/hud3d.hpp"
#include <iostream>
#include <set>
#include <stdexcept>
#define CHECK(x) do {if(!(x))throw std::runtime_error("Check failed: " #x);}while(false)
int main(){
    using namespace apex3d;using namespace cp3;
    const auto path=makePath();const RaceMotion geometry(path);
    std::set<int> ids;int starters=0,retirements=0,lapped=0,pitStarts=0;
    for(std::size_t i=0;i<archivedEntries.size();++i){
        const auto& entry=archivedEntries[i];FieldMotion car(path,geometry,entry);
        CHECK(ids.insert(entry.number).second);
        CHECK(driverIndex(static_cast<DriverFocus>(entry.number))==int(i));
        const auto focus=static_cast<DriverFocus>(entry.number);
        CHECK(three::overlayAction(.7,three::driverRowTop(i%3)-.02,true,i/3)==three::focusAction(focus));
        CHECK(three::overlayAction(.7,three::driverRowTop(i%3)-.02,false,i/3)==HudAction::None);
        if(entry.dns){CHECK(!car.sample(0).visible&&car.sample(4000).dns);CHECK(recordedSeries(entry.number).empty());
            CHECK(!tyreState(focus,0).available);CHECK(driverClock(focus,4000).completed==0);continue;}
        ++starters;retirements+=entry.dnf;lapped+=entry.totalLaps==52;pitStarts+=entry.pitStart;
        CHECK(!recordedSeries(entry.number).empty());CHECK(tyreState(focus,0).available);
        int total=0;
        for(int lap=0;lap<entry.totalLaps;++lap){total+=entry.laps[lap].milliseconds();
            CHECK(std::abs(car.sample(total/1000.0).progress-(lap+1))<1e-6);}
        CHECK(std::abs(total/1000.0-entry.elapsed)<1e-6);
        CHECK(std::abs(entry.classified-entry.elapsed-entry.penaltyMs/1000.0)<1e-6);
        double previous=car.sample(0).progress;
        for(double t=.5;t<=raceTotalSeconds;t+=.5){const auto sample=car.sample(t);
            if(sample.progress+1e-7<previous)std::cerr<<entry.number<<" backwards at "<<t<<"\n";
            CHECK(sample.progress+1e-7>=previous);previous=sample.progress;
            CHECK(std::isfinite(sample.pose.position.x)&&std::isfinite(sample.pose.position.y));
            CHECK(sample.velocity>=-1e-7&&std::isfinite(sample.velocity));}
        for(const auto& pit:archivedPits)if(pit.driver==entry.number){
            CHECK(car.sample(pit.entry+.01).inPit);CHECK(car.sample(pit.exit-.01).inPit);
            CHECK(car.sample(pit.exit+.01).inPit);CHECK(!car.sample(pit.exit+pitLaneMarginSeconds).inPit);
            CHECK(!car.sample(pit.entry-pitLaneMarginSeconds).inPit);
            // No jump where the car turns into or out of the measured lane.
            auto before=car.sample(pit.entry-pitLaneMarginSeconds);
            for(double t=pit.entry-pitLaneMarginSeconds;t<pit.exit+pitLaneMarginSeconds;t+=.02){const auto s=car.sample(t);
                CHECK(std::hypot(s.pose.position.x-before.pose.position.x,s.pose.position.y-before.pose.position.y)*physical::metresPerSceneUnit()<2.5);before=s;}
            CHECK(car.sample(pit.boxStart+pit.service/2).servicing);
            CHECK(std::abs(car.sample(pit.boxStart+.1).travelledMetres-car.sample(pit.boxStart+pit.service-.1).travelledMetres)<1e-6);
            CHECK(tyreState(focus,pit.boxStart+.01).changing);
            CHECK(tyreState(focus,pit.boxStart+pit.service+.01).stint->number==2);
        }
        if(entry.dnf){CHECK(car.sample(entry.retire-.01).visible);CHECK(!car.sample(entry.retire+.01).visible);
            CHECK(car.sample(entry.retire+100).retired);CHECK(driverClock(focus,4404).completed==24);
            CHECK(!driverActive(focus,entry.retire));
            CHECK(nextDriverLap(focus,3000)==3000);
            CHECK(std::abs(driverClock(focus,entry.retire+100).lapElapsed-(entry.retire-entry.elapsed))<1e-6);}
        else {CHECK(car.sample(entry.elapsed+1).cooldown);
            CHECK(car.sample(parkingRelease(entry)+parkingDuration+1).parked);
            const auto parked=car.sample(parkingRelease(entry)+parkingDuration+1);
            const auto target=parcFermePose(path,entry.rank);
            CHECK(std::hypot(parked.pose.position.x-target.position.x,parked.pose.position.y-target.position.y)<1e-8);}
        if(entry.pitStart){CHECK(car.sample(0).inPit);const auto start=car.sample(0);
            const auto road=poseAtProgress(path,start.progress);
            CHECK(std::hypot(start.pose.position.x-road.position.x,start.pose.position.y-road.position.y)*physical::metresPerSceneUnit()>10);}
        Playback playback;playback.seek(entry.dnf?entry.retire+1:120);playback.showWheels=true;
        const auto overlay=three::overlay(playback,car.sample(playback.elapsed),path.back().distance,16.0/9,true,
            three::CameraMode::Chase,true,HudAction::None,focus,i/3);
        for(const auto& v:overlay)CHECK(std::isfinite(v.x)&&std::isfinite(v.y)&&v.x>=-1&&v.x<=1&&v.y>=-1&&v.y<=1);
    }
    CHECK(ids.size()==20&&starters==19&&retirements==1&&lapped==4&&pitStarts==2);
    CHECK(recordedSeries(999).empty());
    CHECK(three::overlayAction(.875,-.79,true,3)==HudAction::FieldPrevious);
    CHECK(three::overlayAction(.93,-.79,true,3)==HudAction::FieldNext);
    CHECK(three::overlayAction(.7,three::driverRowTop(2)-.02,true,6)==HudAction::None);
    std::cout<<"Full official field, timing gates, pit stops, retirement, DNS and all selector pages passed\n";
}
