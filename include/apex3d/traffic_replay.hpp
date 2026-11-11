#pragma once
#include "field_simulation.hpp"
#include <array>
#include <numeric>

namespace apex3d::cp3 {
// Conservative oriented body envelopes, in metres. Includes nose and both wings.
inline bool carsOverlap(const Pose& a,const Pose& b,double margin=.08){
    const double scale=physical::metresPerSceneUnit();
    const double ax=std::cos(a.headingRadians),ay=std::sin(a.headingRadians),bx=std::cos(b.headingRadians),by=std::sin(b.headingRadians);
    const double dx=(b.position.x-a.position.x)*scale+.23*(bx-ax),dy=(b.position.y-a.position.y)*scale+.23*(by-ay);
    if(std::abs(dx)>6.6||std::abs(dy)>6.6)return false;
    const double l=2.87+margin,w=1.025+margin;
    for(const auto& axis:std::array<Vec2,4>{{{ax,ay},{-ay,ax},{bx,by},{-by,bx}}}){
        const double ra=l*std::abs(axis.x*ax+axis.y*ay)+w*std::abs(-axis.x*ay+axis.y*ax);
        const double rb=l*std::abs(axis.x*bx+axis.y*by)+w*std::abs(-axis.x*by+axis.y*bx);
        if(std::abs(dx*axis.x+dy*axis.y)>=ra+rb)return false;
    }
    return true;
}
inline void separateCars(std::vector<RaceSample>& samples,const std::vector<PathSample>* path=nullptr){
    std::array<int,20> order{};std::iota(order.begin(),order.end(),0);
    std::stable_sort(order.begin(),order.end(),[&](int a,int b){
        if(samples[a].parked!=samples[b].parked)return samples[a].parked;
        return samples[a].progress>samples[b].progress;
    });
    std::vector<int> placed;
    const double unit=physical::sceneUnits(1);
    for(int i:order)if(samples[i].visible){const auto origin=samples[i].pose;
        const auto clear=[&](const Pose& p){for(int j:placed)if(carsOverlap(p,samples[j].pose,.16))return false;return true;};
        if(!clear(origin)){
            const double fx=std::cos(origin.headingRadians),fy=std::sin(origin.headingRadians),nx=-fy,ny=fx;
            const auto centre=path?poseAtProgress(*path,samples[i].progress):origin;
            bool found=false;
            // Search nearest free lateral space or queue behind. Placed leaders never move again.
            for(int step=1;step<=1200&&!found;++step){const double distance=step*.125;
                for(auto direction:std::array<Vec2,3>{{{-fx,-fy},{nx,ny},{-nx,-ny}}}){
                    auto candidate=origin;candidate.position.x+=direction.x*distance*unit;candidate.position.y+=direction.y*distance*unit;
                    if(path&&!samples[i].inPit&&!samples[i].cooldown){const double normalX=-std::sin(centre.headingRadians),normalY=std::cos(centre.headingRadians);
                        const double lane=((candidate.position.x-centre.position.x)*normalX+(candidate.position.y-centre.position.y)*normalY)/unit;
                        if(std::abs(lane)>4.7)continue;}
                    if(clear(candidate)){samples[i].pose=candidate;found=true;break;}
                }
            }
            if(!found)throw std::runtime_error("No collision-free space for replay car");
        }
        placed.push_back(i);
    }
}
class TrafficReplay {
    struct Point {float x,y,heading,metres;};
    static constexpr double step=.05;
    const std::vector<PathSample>& path_;
    std::vector<FieldMotion> field_;
    std::vector<std::array<Point,20>> frames_;
    std::array<Vec2,20> offsets_{};
    std::array<double,20> metres_{};
    void ensureFrame(std::size_t limit){
        const double scale=physical::metresPerSceneUnit();
        for(std::size_t frame=frames_.size();frame<=limit;++frame){
            const double seconds=frame*step;std::vector<RaceSample> samples,future;
            for(const auto& car:field_){samples.push_back(car.sample(seconds));future.push_back(car.sample(seconds+1.2));}
            const auto nominal=samples;
            for(int i=0;i<20;++i)if(samples[i].visible){
                const double decay=samples[i].parked?.75:.97;
                offsets_[i].x*=decay;offsets_[i].y*=decay;
                samples[i].pose.position.x+=offsets_[i].x/scale;samples[i].pose.position.y+=offsets_[i].y/scale;
            }
            // Start changing lane before a predicted contact, while keeping measured along-track timing.
            for(int i=19;i>=0;--i)if(samples[i].visible&&!samples[i].inPit&&!samples[i].cooldown){
                const double h=nominal[i].pose.headingRadians,nx=-std::sin(h),ny=std::cos(h);
                const double original=archivedEntries[i].grid%2?2.25:-2.25,current=original+offsets_[i].x*nx+offsets_[i].y*ny;
                const auto collides=[&](double lane){auto p=future[i].pose;const double delta=physical::sceneUnits(lane-original);
                    p.position.x-=std::sin(p.headingRadians)*delta;p.position.y+=std::cos(p.headingRadians)*delta;
                    for(int j=0;j<20;++j)if(j!=i&&future[j].visible){auto q=future[j].pose;q.position.x+=offsets_[j].x/scale;q.position.y+=offsets_[j].y/scale;if(carsOverlap(p,q,.30))return true;}
                    return false;
                };
                if(collides(current))for(double lane:{-4.2,-1.4,1.4,4.2})if(!collides(lane)){
                    const double move=std::clamp(lane-current,-.175,.175);
                    samples[i].pose.position.x+=nx*move/scale;samples[i].pose.position.y+=ny*move/scale;break;
                }
            }
            separateCars(samples,&path_);
            std::array<Point,20> cached{};
            for(int i=0;i<20;++i){const auto& s=samples[i];
                offsets_[i]={(s.pose.position.x-nominal[i].pose.position.x)*scale,(s.pose.position.y-nominal[i].pose.position.y)*scale};
                if(frame&&s.visible){const auto& previous=frames_.back()[i];metres_[i]+=std::hypot(s.pose.position.x-previous.x,s.pose.position.y-previous.y)*scale;}
                cached[i]={float(s.pose.position.x),float(s.pose.position.y),float(s.pose.headingRadians),float(metres_[i])};
            }
            frames_.push_back(cached);
        }
    }
public:
    TrafficReplay(const std::vector<PathSample>& path,const RaceMotion& geometry):path_(path){
        for(const auto& entry:archivedEntries)field_.emplace_back(path,geometry,entry);
        frames_.reserve(static_cast<std::size_t>((raceTotalSeconds+postRaceDuration)/step)+3);
    }
    const FieldMotion& motion(std::size_t index)const{return field_.at(index);}
    // Fraction of the race + post-race timeline whose spacing frames are already cached.
    double cachedFraction()const{
        const double total=(raceTotalSeconds+postRaceDuration)/step+2;
        return std::clamp(frames_.size()/total,0.0,1.0);
    }
    // Extend the cache a little at a time so later timeline jumps do not stall.
    // Frames are computed in the same order as a normal seek, so results are identical.
    void warm(std::size_t frames){
        const auto limit=static_cast<std::size_t>(std::ceil((raceTotalSeconds+postRaceDuration)/step))+1;
        if(frames_.size()<=limit)ensureFrame(std::min(limit,frames_.size()+frames));
    }
    std::vector<RaceSample> sample(double seconds){
        if(!std::isfinite(seconds))seconds=0;
        seconds=std::clamp(seconds,0.0,raceTotalSeconds+postRaceDuration);
        ensureFrame(static_cast<std::size_t>(std::ceil(seconds/step))+1);
        const double f=seconds/step;const auto a=std::size_t(f),b=a+1;const double t=f-a;
        std::vector<RaceSample> result;
        for(int i=0;i<20;++i){auto s=field_[i].sample(seconds);const auto& p=frames_[a][i];const auto& q=frames_[b][i];
            s.pose.position={p.x+(q.x-p.x)*t,p.y+(q.y-p.y)*t};s.pose.headingRadians=p.heading+std::atan2(std::sin(q.heading-p.heading),std::cos(q.heading-p.heading))*t;
            s.travelledMetres=p.metres+(q.metres-p.metres)*t;s.velocity=std::hypot(q.x-p.x,q.y-p.y)/step;result.push_back(s);}
        // Projection after interpolation protects arbitrary seeks and playback frame rates too.
        separateCars(result,&path_);return result;
    }
};
} // namespace apex3d::cp3
