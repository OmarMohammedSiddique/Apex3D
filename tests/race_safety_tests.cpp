#include "../include/apex3d/traffic_replay.hpp"
#include "../include/apex3d/hud3d.hpp"
#include <iostream>
#include <stdexcept>
#define CHECK(x) do{if(!(x))throw std::runtime_error("Check failed: " #x);}while(false)
int main(){using namespace apex3d;using namespace cp3;
    CHECK(raceFlagAt(100).severity==0);
    CHECK(raceFlagAt(2452).title=="BLACK AND WHITE FLAG");
    CHECK(raceFlagAt(3259).title=="BLUE FLAG");
    CHECK(raceFlagAt(3343).severity==3);CHECK(raceFlagAt(3349).detail.find("6, 7")!=std::string::npos);
    CHECK(raceFlagAt(3353).severity==2);CHECK(raceFlagAt(3355).title=="TRACK CLEAR");
    CHECK(raceFlagAt(3362).severity==0);CHECK(raceFlagAt(4404).title=="CHEQUERED FLAG");
    const auto path=makePath();const RaceMotion geometry(path);TrafficReplay traffic(path,geometry);
    double maxAdjustment=0;int frames=0;
    for(double t=0;t<=raceTotalSeconds+postRaceDuration;t+=.073){auto samples=traffic.sample(t);++frames;
        for(int i=0;i<20;++i)if(samples[i].visible){
            for(int j=0;j<i;++j)if(samples[j].visible){if(carsOverlap(samples[i].pose,samples[j].pose,0))std::cerr<<"Contact at "<<t<<" cars "<<archivedEntries[i].number<<","<<archivedEntries[j].number<<"\n";
                CHECK(!carsOverlap(samples[i].pose,samples[j].pose,0));}
            if(t<raceTotalSeconds&&int(t*100)%1000<8){const FieldMotion nominal(path,geometry,archivedEntries[i]);const auto raw=nominal.sample(t);
                maxAdjustment=std::max(maxAdjustment,std::hypot(samples[i].pose.position.x-raw.pose.position.x,samples[i].pose.position.y-raw.pose.position.y)*physical::metresPerSceneUnit());}
        }
    }
    CHECK(maxAdjustment<40);
    const auto parked=traffic.sample(raceTotalSeconds+postRaceDuration);
    for(int i=0;i<18;++i){const auto target=parcFermePose(path,archivedEntries[i].rank);
        CHECK(std::hypot(parked[i].pose.position.x-target.position.x,parked[i].pose.position.y-target.position.y)*physical::metresPerSceneUnit()<.05);}
    CHECK(!parked[18].visible&&!parked[19].visible);
    // Seeking backwards reads the same deterministic trajectory, independent of playback rate.
    const auto a=traffic.sample(120),b=traffic.sample(120);CHECK(a[0].pose.position.x==b[0].pose.position.x);
    for(double t:{0.,1.,9.,80.,140.,3349.,4405.})for(double delta:{.004,.016,.033,.063}){
        const auto s=traffic.sample(t+delta);for(int i=0;i<20;++i)if(s[i].visible)for(int j=0;j<i;++j)if(s[j].visible)CHECK(!carsOverlap(s[i].pose,s[j].pose,0));}
    CHECK(introLogo.size()>0&&introWidth>introHeight);
    for(double aspect:{.8,16./9.,3.2}){Playback p;p.started=true;p.startElapsed=5;
        const auto hud=makeRaceHudMesh(p,geometry.sample(0),path.back().distance,aspect,false);
        for(const auto& v:hud)CHECK(v.x>=-1&&v.x<=1&&v.y>=-1&&v.y<=1);}
    const auto birds=three::birds(path,120);CHECK(!birds.empty());for(const auto& v:birds)CHECK(v.y>30&&v.y<90);
    std::cout<<frames<<" whole-race and parking snapshots: no body intersections; max visual spacing adjustment "<<maxAdjustment<<" m. Flags, lamps and birds passed.\n";
}
