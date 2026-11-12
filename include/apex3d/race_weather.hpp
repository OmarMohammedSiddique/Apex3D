#pragma once
// Recorded trackside weather for the 2025 Italian Grand Prix (OpenF1 session 9912).
// The station reports about once a minute; the panel shows the latest reading at or
// before the race time, not an interpolation, so every number on screen was recorded.
#include "generated/monza_weather_2025.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>
namespace apex3d::cp3 {
inline const WeatherSample& weatherAt(double seconds){
    if(!std::isfinite(seconds))seconds=0;
    const double ms=seconds*1000;
    const auto next=std::upper_bound(recordedWeather.begin(),recordedWeather.end(),ms,
        [](double t,const WeatherSample& s){return t<s.raceMs;});
    return next==recordedWeather.begin()?recordedWeather.front():*(next-1);
}
// Eight-point compass name for a direction in degrees clockwise from north.
inline const char* compassPoint(int degrees){
    static constexpr const char* names[]{"N","NE","E","SE","S","SW","W","NW"};
    const int wrapped=((degrees%360)+360)%360;
    return names[((wrapped*2+45)/90)%8];
}
struct WeatherLines { std::string title,air,track,humidity,wind,from; };
inline WeatherLines weatherLines(double seconds){
    const auto& w=weatherAt(seconds);
    const auto fixed=[](double value,int digits){std::ostringstream s;s<<std::fixed<<std::setprecision(digits)<<value;return s.str();};
    return {std::string("WEATHER  ")+(w.rain?"RAIN":"DRY"),"AIR   "+fixed(w.airC,1)+"C","TRACK "+fixed(w.trackC,1)+"C",
        "HUMIDITY "+fixed(w.humidity,0)+"%","WIND "+fixed(w.windMs,1)+"M/S",std::string("FROM ")+compassPoint(w.windFromDegrees)};
}
} // namespace apex3d::cp3
