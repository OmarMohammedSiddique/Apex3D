#pragma once
// Ground height: the recorded track elevation on the road, the surveyed terrain around it.
//
// The flat scene is built as before; the renderer then raises every vertex by
// the terrain height under it (litVertex/depthVertex in renderer3d.hpp), so the
// road, verges, cars, buildings and shadows all follow one surface and cars
// pitch with the slope. This file is the CPU copy of that height map: cameras,
// screen markers and tests sample it with the same bilinear lookup the shader uses.
//
// Three zones, by distance from the measured line:
//   - within 30 m: the recorded OpenF1 elevation (a Gaussian average of nearby
//     control-point heights, within about 0.2 m of each one), so the road and its
//     verges stay level across;
//   - 30 to 100 m: a smooth blend from that to the survey;
//   - beyond: Regione Lombardia's 5 m terrain model (CC BY 4.0, sampled every 16 m by
//     tools/generate_terrain_assets.py), shifted by the median offset between it and
//     the OpenF1 elevation along the lap.
// Over the outer 400 m of the 1.2 km margin the survey fades to the mean height of
// the map's edge, so the flat ground beyond the map meets it without a step.
// Heights are relative to the finish line.
#include "math3d.hpp"
#include "physical_scale.hpp"
#include "generated/monza_track_2025.hpp"
#include "generated/monza_terrain_2025.hpp"
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <vector>

namespace apex3d::three {
struct Terrain {
    double minX{},minZ{},cell{16.0};
    int columns{},rows{};
    double mean{};
    std::vector<float> heights; // row-major: heights[row*columns+column], x along columns, z along rows
    [[nodiscard]] double texel(int column,int row)const{return heights[static_cast<std::size_t>(row*columns+column)];}
    // Bilinear between texel samples, clamped at the edge (where every texel equals `mean`).
    [[nodiscard]] double height(double x,double z)const{
        const double u=std::clamp((x-minX)/cell,0.0,double(columns-1)),v=std::clamp((z-minZ)/cell,0.0,double(rows-1));
        const int i=std::min(static_cast<int>(u),columns-2),j=std::min(static_cast<int>(v),rows-2);
        const double fu=u-i,fv=v-j;
        const double a=texel(i,j),b=texel(i+1,j),c=texel(i,j+1),d=texel(i+1,j+1);
        return (a+(b-a)*fu)+((c+(d-c)*fu)-(a+(b-a)*fu))*fv;
    }
    [[nodiscard]] double maxX()const{return minX+cell*(columns-1);}
    [[nodiscard]] double maxZ()const{return minZ+cell*(rows-1);}
};

inline constexpr double terrainRoadHold=30.0,terrainRoadBlend=100.0,terrainBorderFade=400.0;
// Surveyed height (relative to the finish line) at a grid texel, before any blending.
inline double surveyedTexel(int column,int row){
    return cp3::surveyedTerrainCentimetres[static_cast<std::size_t>(row*cp3::surveyedTerrainColumns+column)]/100.0-cp3::surveyedTerrainZeroMetres;
}
inline Terrain buildTerrain(){
    const auto& controls=cp3::measuredMonzaControls;const auto& elevation=cp3::measuredMonzaElevation;
    const double scale=physical::metresPerSceneUnit();
    std::vector<V3> points;points.reserve(controls.size());
    double lowX=1e18,highX=-1e18,lowZ=1e18,highZ=-1e18;
    for(std::size_t i=0;i<controls.size();++i){
        const V3 p{controls[i].x*scale,elevation[i]-elevation[0],-controls[i].y*scale};points.push_back(p);
        lowX=std::min(lowX,p.x);highX=std::max(highX,p.x);lowZ=std::min(lowZ,p.z);highZ=std::max(highZ,p.z);
    }
    Terrain terrain;
    terrain.minX=cp3::surveyedTerrainOriginX*scale;terrain.minZ=cp3::surveyedTerrainOriginZ*scale;
    terrain.cell=cp3::surveyedTerrainCell*scale;
    terrain.columns=cp3::surveyedTerrainColumns;terrain.rows=cp3::surveyedTerrainRows;
    // The flat ground beyond the map sits at the mean of the map's edge.
    double edge=0;int edgeCount=0;
    for(int column=0;column<terrain.columns;++column)for(int row:{0,terrain.rows-1}){edge+=surveyedTexel(column,row);++edgeCount;}
    for(int row=1;row+1<terrain.rows;++row)for(int column:{0,terrain.columns-1}){edge+=surveyedTexel(column,row);++edgeCount;}
    terrain.mean=edge/edgeCount;
    terrain.heights.resize(static_cast<std::size_t>(terrain.columns*terrain.rows));
    const auto smoothstep=[](double a,double b,double x){const double t=std::clamp((x-a)/(b-a),0.0,1.0);return t*t*(3-2*t);};
    for(int row=0;row<terrain.rows;++row)for(int column=0;column<terrain.columns;++column){
        const double x=terrain.minX+column*terrain.cell,z=terrain.minZ+row*terrain.cell;
        const double border=std::min({column,row,terrain.columns-1-column,terrain.rows-1-row})*terrain.cell;
        double value=terrain.mean+(surveyedTexel(column,row)-terrain.mean)*smoothstep(0.0,terrainBorderFade,border);
        // Far outside the track's bounding box the road cannot reach: skip the search.
        if(std::hypot(std::max({lowX-x,0.0,x-highX}),std::max({lowZ-z,0.0,z-highZ}))>=terrainRoadBlend){
            terrain.heights[static_cast<std::size_t>(row*terrain.columns+column)]=static_cast<float>(value);continue;
        }
        double nearest=1e18;
        for(const auto& p:points)nearest=std::min(nearest,(x-p.x)*(x-p.x)+(z-p.z)*(z-p.z));
        nearest=std::sqrt(nearest);
        if(const double survey=smoothstep(terrainRoadHold,terrainRoadBlend,nearest);survey<1.0){
            // Gaussian average of nearby track heights; the kernel widens with
            // distance so the field stays smooth between track sections.
            const double sigma=std::max(40.0,0.6*nearest),reach=16*sigma*sigma;
            double sum=0,weight=0;
            for(const auto& p:points){
                const double d2=(x-p.x)*(x-p.x)+(z-p.z)*(z-p.z);
                if(d2<reach){const double w=std::exp(-d2/(2*sigma*sigma));sum+=w*p.y;weight+=w;}
            }
            value=sum/weight*(1.0-survey)+value*survey;
        }
        terrain.heights[static_cast<std::size_t>(row*terrain.columns+column)]=static_cast<float>(value);
    }
    return terrain;
}
inline const Terrain& terrain(){static const Terrain value=buildTerrain();return value;}
inline double terrainHeight(double x,double z){return terrain().height(x,z);}
inline V3 onTerrain(V3 p){return {p.x,p.y+terrainHeight(p.x,p.z),p.z};}
} // namespace apex3d::three
