#pragma once
// Measured track width. The edges of the paved surface come from Regione Lombardia's
// topographic survey (DBGT, CC BY 4.0); tools/track_surface.py measures them from the
// measured line every 5 m, leaving out asphalt run-off and the pit lane. The road is
// therefore not symmetric about the line: the cars' line runs close to an edge where
// the racing line uses the full width.
#include "generated/monza_track_2025.hpp"
#include <algorithm>
#include <cmath>
namespace apex3d::cp3 {
struct TrackEdges { double left,right; }; // metres from the measured line
inline TrackEdges trackEdges(double progress){
    constexpr int count=static_cast<int>(measuredTrackEdgeLeft.size());
    if(!std::isfinite(progress))progress=0;
    const double k=(progress-std::floor(progress))*count;
    const int i=std::clamp(static_cast<int>(k),0,count-1),j=(i+1)%count;const double t=k-i;
    return {measuredTrackEdgeLeft[i]*(1-t)+measuredTrackEdgeLeft[j]*t,measuredTrackEdgeRight[i]*(1-t)+measuredTrackEdgeRight[j]*t};
}
} // namespace apex3d::cp3
