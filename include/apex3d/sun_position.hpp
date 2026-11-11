#pragma once
// The real sun over Monza during the race.
//
// NOAA's general solar position equations (the ones behind its online solar
// calculator) give the sun's elevation and azimuth from the UTC time and the
// finish line's latitude/longitude. Time is the recorded race-clock origin
// (13:03:35 UTC, 7 September 2025, OpenF1 session 9912) plus race time, so the
// light and shadows move as the race runs. Atmospheric refraction (about one
// arcminute at these elevations) is ignored. Clouds and haze stay artistic.
#include "math3d.hpp"
#include "generated/monza_track_2025.hpp"
#include <algorithm>
#include <cmath>
#include <numbers>

namespace apex3d::three {
struct SolarPosition { double elevationDegrees, azimuthDegrees; }; // azimuth clockwise from true north

inline SolarPosition solarPosition(double unixSeconds,double latitudeDegrees,double longitudeDegrees){
    constexpr double rad=std::numbers::pi/180;
    const auto wrap=[](double degrees){return degrees-360.0*std::floor(degrees/360.0);};
    const double julian=unixSeconds/86400.0+2440587.5,t=(julian-2451545.0)/36525.0;
    const double meanLongitude=wrap(280.46646+t*(36000.76983+t*0.0003032));
    const double meanAnomaly=357.52911+t*(35999.05029-0.0001537*t);
    const double eccentricity=0.016708634-t*(0.000042037+0.0000001267*t);
    const double centre=std::sin(meanAnomaly*rad)*(1.914602-t*(0.004817+0.000014*t))
        +std::sin(2*meanAnomaly*rad)*(0.019993-0.000101*t)+std::sin(3*meanAnomaly*rad)*0.000289;
    const double omega=125.04-1934.136*t;
    const double apparentLongitude=meanLongitude+centre-0.00569-0.00478*std::sin(omega*rad);
    const double meanObliquity=23.0+(26.0+(21.448-t*(46.815+t*(0.00059-t*0.001813)))/60.0)/60.0;
    const double obliquity=meanObliquity+0.00256*std::cos(omega*rad);
    const double declination=std::asin(std::sin(obliquity*rad)*std::sin(apparentLongitude*rad));
    const double y=std::pow(std::tan(obliquity*rad/2),2);
    const double equationOfTime=4.0/rad*(y*std::sin(2*meanLongitude*rad)-2*eccentricity*std::sin(meanAnomaly*rad)
        +4*eccentricity*y*std::sin(meanAnomaly*rad)*std::cos(2*meanLongitude*rad)
        -0.5*y*y*std::sin(4*meanLongitude*rad)-1.25*eccentricity*eccentricity*std::sin(2*meanAnomaly*rad)); // minutes
    const double minutesUtc=(julian+0.5-std::floor(julian+0.5))*1440.0;
    const double trueSolarMinutes=wrap((minutesUtc+equationOfTime+4.0*longitudeDegrees)/4.0)*4.0;
    const double hourAngle=(trueSolarMinutes/4.0-180.0)*rad,latitude=latitudeDegrees*rad;
    const double zenith=std::acos(std::clamp(std::sin(latitude)*std::sin(declination)+std::cos(latitude)*std::cos(declination)*std::cos(hourAngle),-1.0,1.0));
    const double azimuth=std::atan2(std::sin(hourAngle),std::cos(hourAngle)*std::sin(latitude)-std::tan(declination)*std::cos(latitude))/rad+180.0;
    return {90.0-zenith/rad,wrap(azimuth)};
}
// True north and east as world directions (world x = scene x, world z = -scene y).
inline V3 worldNorth(){return unit(V3{cp3::sceneNorthX,0,-cp3::sceneNorthY});}
inline V3 worldEast(){return cross(worldNorth(),V3{0,1,0});}
// Unit vector towards the sun at a given race time (seconds from the race-clock origin).
inline V3 sunDirection(double raceSeconds){
    constexpr double rad=std::numbers::pi/180;
    const auto sun=solarPosition(cp3::raceStartUnixSeconds+raceSeconds,cp3::siteLatitude,cp3::siteLongitude);
    const double elevation=sun.elevationDegrees*rad,azimuth=sun.azimuthDegrees*rad;
    return unit(worldNorth()*(std::cos(elevation)*std::cos(azimuth))+worldEast()*(std::cos(elevation)*std::sin(azimuth))
        +V3{0,std::sin(elevation),0});
}
// Colour of direct sunlight relative to the race start. As the sun sinks its light
// crosses more air, and Rayleigh scattering removes blue faster than red. Air mass
// uses the Kasten-Young (1989) formula. The optical depths are the clear-sky Rayleigh
// values at 680/550/450 nm (0.044/0.097/0.22). The tint is normalised to 1 at the
// start, so the race-start look is unchanged and only the change through the race
// is modelled. Aerosols and clouds are ignored.
inline double airMass(double elevationDegrees){
    return 1.0/(std::sin(elevationDegrees*std::numbers::pi/180)+0.50572*std::pow(elevationDegrees+6.07995,-1.6364));
}
inline V3 sunlightTint(double raceSeconds){
    const auto now=solarPosition(cp3::raceStartUnixSeconds+raceSeconds,cp3::siteLatitude,cp3::siteLongitude);
    const auto start=solarPosition(cp3::raceStartUnixSeconds,cp3::siteLatitude,cp3::siteLongitude);
    const double extra=airMass(now.elevationDegrees)-airMass(start.elevationDegrees);
    return {std::exp(-0.044*extra),std::exp(-0.097*extra),std::exp(-0.22*extra)};
}
} // namespace apex3d::three
