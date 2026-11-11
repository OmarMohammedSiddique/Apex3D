#pragma once
#include "recorded_motion.hpp"
#include <algorithm>
#include <cmath>

namespace apex3d::cp3 {
struct RecordedTelemetry {
    bool available{};
    int speedKmh{},gear{},engineRpm{};
    double sampleAge{};
    int throttlePct{};bool braking{},controlsValid{}; // OpenF1 104 = invalid pedal sample

};
// Hold the last received observation. Never blend gears or invent RPM values.
// Leave gaps >1 s unavailable, and keep playback multiplier out of the data.
inline RecordedTelemetry telemetryAt(double raceSeconds,int driverNumber=1) {
    if(!std::isfinite(raceSeconds))return {};
    const auto points=recordedSeries(driverNumber);
    const double ms=raceSeconds*1000;
    const auto upper=std::upper_bound(points.begin(),points.end(),ms,
        [](double t,const TelemetryPoint& p){return t<p.raceMs;});
    if(upper==points.begin())return {};
    const auto& p=*(upper-1);const double age=(ms-p.raceMs)/1000;
    if(age>1.0)return {};
    const bool valid=p.throttlePct<=100&&p.brake!=104;
    return {true,p.speedKmh,p.gear,p.engineRpm,age,valid?p.throttlePct:0,valid&&p.brake>=100,valid};
}
} // namespace apex3d::cp3
