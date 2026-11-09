#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <stdexcept>

namespace apex3d::three {
struct V3 {
    double x{}, y{}, z{};
    V3 operator+(V3 b) const { return {x+b.x,y+b.y,z+b.z}; }
    V3 operator-(V3 b) const { return {x-b.x,y-b.y,z-b.z}; }
    V3 operator*(double s) const { return {x*s,y*s,z*s}; }
};
inline double dot(V3 a,V3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
inline V3 cross(V3 a,V3 b) { return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; }
inline double length(V3 v) { return std::sqrt(dot(v,v)); }
inline V3 unit(V3 v) {
    const double magnitude=length(v);
    if (magnitude < 1e-12) throw std::invalid_argument("Cannot normalize a zero 3D vector");
    return v*(1.0/magnitude);
}
inline V3 mix(V3 a,V3 b,double t) { return a*(1-t)+b*t; }

// Column-major matrices, passed directly to OpenGL with transpose=false.
struct M4 {
    std::array<float,16> values{};
    float& at(int row,int col) { return values[static_cast<std::size_t>(col*4+row)]; }
    float at(int row,int col) const { return values[static_cast<std::size_t>(col*4+row)]; }
    static M4 identity() { M4 m; for(int i=0;i<4;++i)m.at(i,i)=1; return m; }
    M4 operator*(const M4& b) const {
        M4 result;
        for(int row=0;row<4;++row) for(int col=0;col<4;++col)
            for(int k=0;k<4;++k) result.at(row,col)+=at(row,k)*b.at(k,col);
        return result;
    }
    std::array<double,4> transform(V3 p,double w=1.0) const {
        const std::array<double,4> input{p.x,p.y,p.z,w};
        std::array<double,4> output{};
        for(int r=0;r<4;++r) for(int c=0;c<4;++c) output[static_cast<std::size_t>(r)]+=at(r,c)*input[static_cast<std::size_t>(c)];
        return output;
    }
};
inline M4 perspective(double fov,double aspect,double nearPlane,double farPlane) {
    if (!(aspect>0 && nearPlane>0 && farPlane>nearPlane && fov>0 && fov<std::numbers::pi))
        throw std::invalid_argument("Invalid perspective frustum");
    const double f=1.0/std::tan(fov/2);
    M4 m;
    m.at(0,0)=static_cast<float>(f/aspect); m.at(1,1)=static_cast<float>(f);
    m.at(2,2)=static_cast<float>((farPlane+nearPlane)/(nearPlane-farPlane));
    m.at(2,3)=static_cast<float>(2*farPlane*nearPlane/(nearPlane-farPlane));
    m.at(3,2)=-1;
    return m;
}
inline M4 orthographic(double left,double right,double bottom,double top,double nearPlane,double farPlane) {
    M4 m=M4::identity();
    m.at(0,0)=static_cast<float>(2/(right-left));m.at(1,1)=static_cast<float>(2/(top-bottom));
    m.at(2,2)=static_cast<float>(-2/(farPlane-nearPlane));
    m.at(0,3)=static_cast<float>(-(right+left)/(right-left));
    m.at(1,3)=static_cast<float>(-(top+bottom)/(top-bottom));
    m.at(2,3)=static_cast<float>(-(farPlane+nearPlane)/(farPlane-nearPlane));
    return m;
}
inline M4 lookAt(V3 eye,V3 target,V3 up={0,1,0}) {
    const auto f=unit(target-eye), s=unit(cross(f,up)), u=cross(s,f);
    M4 m=M4::identity();
    m.at(0,0)=static_cast<float>(s.x);m.at(0,1)=static_cast<float>(s.y);m.at(0,2)=static_cast<float>(s.z);
    m.at(1,0)=static_cast<float>(u.x);m.at(1,1)=static_cast<float>(u.y);m.at(1,2)=static_cast<float>(u.z);
    m.at(2,0)=static_cast<float>(-f.x);m.at(2,1)=static_cast<float>(-f.y);m.at(2,2)=static_cast<float>(-f.z);
    m.at(0,3)=static_cast<float>(-dot(s,eye));m.at(1,3)=static_cast<float>(-dot(u,eye));m.at(2,3)=static_cast<float>(dot(f,eye));
    return m;
}
} // namespace apex3d::three
