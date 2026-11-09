#include "../include/apex3d/field_simulation.hpp"
#include "../include/apex3d/hud3d.hpp"
#include <iostream>
#include <set>
#include <stdexcept>
#define CHECK(x) do{if(!(x))throw std::runtime_error("Check failed: " #x);}while(false)
int main(){using namespace apex3d;using namespace cp3;using namespace three;
    CHECK(driverLabel(DriverFocus::Leclerc,150)=="LEC P3");
    CHECK(driverLabel(DriverFocus::Piastri,4700)=="PIA P3");
    CHECK(driverLabel(DriverFocus::Hulkenberg,100)=="HUL DNS");
    CHECK(driverLabel(DriverFocus::Alonso,2100)=="ALO DNF");
    for(const auto& e:recordedPositions)if(e.driver!=27&&e.raceMs>=0&&e.raceMs<4404325)
        CHECK(currentPosition(static_cast<DriverFocus>(e.driver),e.raceMs/1000.0)==e.position);
    for(const auto& interval:recordedDrs){const double start=interval.startMs/1000.0,end=interval.endMs/1000.0;
        CHECK(drsActive(interval.driver,(start+end)/2));CHECK(drsOpening(interval.driver,(start+end)/2)>0);
        CHECK(!drsActive(interval.driver,5000));}
    CHECK(!drsActive(27,140));CHECK(!drsActive(16,-1));
    CHECK(teamLogos.size()==10);std::set<int> marks;
    const Pose origin{{0,0},0,0};
    for(const auto& e:archivedEntries){const int index=logoIndex(e.number);CHECK(index>=0&&index<10);marks.insert(index);
        const auto& logo=teamLogos[index];CHECK(logo.count>0&&logo.first+logo.count<=int(logoRuns.size()));
        const auto marked=makeCar(origin,0,false,0,e.color,{1,1,1},{1,1,1},e.number);
        const auto open=makeCar(origin,0,false,0,e.color,{1,1,1},{1,1,1},e.number,1,true);
        CHECK(open.size()>marked.size());
        for(const auto& v:open)CHECK(std::isfinite(v.x)&&std::isfinite(v.y)&&std::isfinite(v.z)&&v.r>=0&&v.r<=1&&v.g>=0&&v.g<=1&&v.b>=0&&v.b<=1);
    }
    CHECK(marks.size()==10);
    const auto path=makePath();const RaceMotion geometry(path);const auto trees=woodlandTrees(path);
    CHECK(trees.size()>400);
    for(const auto& tree:trees){CHECK(tree.height>=11&&tree.height<=23);CHECK(roadDistance(tree.p,path)>=23-1e-7);}
    int woods=0,buildings=0;for(const auto& f:mapFootprints){woods+=f.wood;buildings+=!f.wood;CHECK(f.count>=3);}
    CHECK(woods==19&&buildings==77);
    for(const auto& e:archivedEntries){const FieldMotion car(path,geometry,e);
        if(e.dns||e.dnf){CHECK(!car.sample(4800).visible);continue;}
        const auto final=car.sample(4800);CHECK(final.parked&&final.velocity==0);CHECK(final.cooldown);
        auto previous=car.sample(e.elapsed-.01);
        for(double t=e.elapsed;t<e.elapsed+cooldownDuration+parkingDuration+1;t+=.05){const auto s=car.sample(t);
            const double metres=std::hypot(s.pose.position.x-previous.pose.position.x,s.pose.position.y-previous.pose.position.y)*physical::metresPerSceneUnit();
            CHECK(metres<5);CHECK(s.travelledMetres+1e-6>=previous.travelledMetres);previous=s;
        }
        for(const auto& other:archivedEntries)if(other.rank>e.rank&&!other.dns&&!other.dnf){const auto a=parcFermePose(path,e.rank),b=parcFermePose(path,other.rank);
            CHECK(std::hypot(a.position.x-b.position.x,a.position.y-b.position.y)*physical::metresPerSceneUnit()>3.4);}
    }
    Playback p;p.seek(raceTotalSeconds-.5);p.playing=true;p.advance(1,path.back().distance);
    CHECK(p.finished()&&p.playing&&p.elapsed==raceTotalSeconds&&std::abs(p.postRaceElapsed-.5)<1e-6);
    p.togglePlaying();p.advance(100,path.back().distance);CHECK(p.postRaceElapsed==.5);
    p.togglePlaying();p.advance(500,path.back().distance);CHECK(!p.playing&&p.postRaceElapsed==postRaceDuration&&p.elapsed==raceTotalSeconds);
    p.togglePlaying();CHECK(p.elapsed==0&&p.postRaceElapsed==0&&p.starting());
    std::cout<<"Live positions, ten team marks, observed DRS, "<<trees.size()<<" broadleaf trees, mapped scenery and post-race parking passed\n";
}
