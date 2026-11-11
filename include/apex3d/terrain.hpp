#pragma once
// Ground height from the recorded track elevation (OpenF1 z, session 9912).
//
// The flat scene is built as before; the renderer then raises every vertex by
// the terrain height under it (litVertex/depthVertex in renderer3d.hpp), so the
// road, verges, cars, buildings and shadows all follow one surface and cars
// pitch with the slope. This file is the CPU copy of that height map: cameras,
// screen markers and tests sample it with the same bilinear lookup the shader uses.
//
// On the track the height is the measured elevation (within about 0.2 m). Away
// from it there is no survey, so the field fades smoothly to the lap's mean
// height over a few hundred metres. Heights are relative to the finish line.
#include "math3d.hpp"
#include "physical_scale.hpp"
#include "generated/monza_track_2025.hpp"
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

inline Terrain buildTerrain(){
    const auto& controls=cp3::measuredMonzaControls;const auto& elevation=cp3::measuredMonzaElevation;
    const double scale=physical::metresPerSceneUnit();
    std::vector<V3> points;points.reserve(controls.size());
    double mean=0,lowX=1e9,highX=-1e9,lowZ=1e9,highZ=-1e9;
    for(std::size_t i=0;i<controls.size();++i){
        const V3 p{controls[i].x*scale,elevation[i]-elevation[0],-controls[i].y*scale};
        points.push_back(p);mean+=p.y;
        lowX=std::min(lowX,p.x);highX=std::max(highX,p.x);lowZ=std::min(lowZ,p.z);highZ=std::max(highZ,p.z);
    }
    mean/=static_cast<double>(points.size());
    Terrain terrain;terrain.mean=mean;
    constexpr double margin=900.0; // past the 450 m fade, so the border is exactly `mean`
    terrain.minX=std::floor((lowX-margin)/terrain.cell)*terrain.cell;
    terrain.minZ=std::floor((lowZ-margin)/terrain.cell)*terrain.cell;
    terrain.columns=static_cast<int>(std::ceil((highX+margin-terrain.minX)/terrain.cell))+1;
    terrain.rows=static_cast<int>(std::ceil((highZ+margin-terrain.minZ)/terrain.cell))+1;
    terrain.heights.resize(static_cast<std::size_t>(terrain.columns*terrain.rows));
    const auto smoothstep=[](double a,double b,double x){const double t=std::clamp((x-a)/(b-a),0.0,1.0);return t*t*(3-2*t);};
    for(int row=0;row<terrain.rows;++row)for(int column=0;column<terrain.columns;++column){
        const double x=terrain.minX+column*terrain.cell,z=terrain.minZ+row*terrain.cell;
        double nearest=1e18;
        for(const auto& p:points)nearest=std::min(nearest,(x-p.x)*(x-p.x)+(z-p.z)*(z-p.z));
        nearest=std::sqrt(nearest);
        const double fade=smoothstep(80.0,450.0,nearest);
        double value=mean;
        if(fade<1.0){
            // Gaussian average of nearby track heights; the kernel widens with
            // distance so the field stays smooth between track sections.
            const double sigma=std::max(40.0,0.6*nearest),reach=16*sigma*sigma;
            double sum=0,weight=0;
            for(const auto& p:points){
                const double d2=(x-p.x)*(x-p.x)+(z-p.z)*(z-p.z);
                if(d2<reach){const double w=std::exp(-d2/(2*sigma*sigma));sum+=w*p.y;weight+=w;}
            }
            value=sum/weight*(1.0-fade)+mean*fade;
        }
        terrain.heights[static_cast<std::size_t>(row*terrain.columns+column)]=static_cast<float>(value);
    }
    return terrain;
}
inline const Terrain& terrain(){static const Terrain value=buildTerrain();return value;}
inline double terrainHeight(double x,double z){return terrain().height(x,z);}
inline V3 onTerrain(V3 p){return {p.x,p.y+terrainHeight(p.x,p.z),p.z};}
} // namespace apex3d::three
