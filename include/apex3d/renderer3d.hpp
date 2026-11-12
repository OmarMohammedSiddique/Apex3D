#pragma once
#include <glad/gl.h>
#include "scene3d.hpp"
#include "sky_shader.hpp"
#include <cstddef>
#include <iostream>

namespace apex3d::three {
inline constexpr char litVertex[] = R"(
#version 330 core
layout(location=0) in vec3 position;
layout(location=1) in vec3 normal;
layout(location=2) in vec3 vertexColor;
uniform mat4 viewProjection;
uniform mat4 lightMatrix;
out vec3 worldPosition;
out vec3 worldNormal;
out vec3 color;
out vec4 lightPosition;
// Terrain height under a world point: the same clamped bilinear lookup as
// Terrain::height in terrain.hpp, so CPU cameras and GPU geometry agree.
uniform sampler2D terrainMap;
uniform vec3 terrainFrame; // min x, min z, 1/cell
float terrainHeight(vec2 p){
    ivec2 size=textureSize(terrainMap,0);
    vec2 u=clamp((p-terrainFrame.xy)*terrainFrame.z,vec2(0.0),vec2(size-1));
    ivec2 i=min(ivec2(u),size-2);vec2 f=u-vec2(i);
    float a=texelFetch(terrainMap,i,0).r,b=texelFetch(terrainMap,i+ivec2(1,0),0).r;
    float c=texelFetch(terrainMap,i+ivec2(0,1),0).r,d=texelFetch(terrainMap,i+ivec2(1,1),0).r;
    return mix(mix(a,b,f.x),mix(c,d,f.x),f.y);
}
void main(){
    vec3 p=position+vec3(0.0,terrainHeight(position.xz),0.0);
    // Raising by h(x,z) is a shear, so normals take its inverse transpose:
    // n' = (nx - ny*dh/dx, ny, nz - ny*dh/dz). Slopes then catch the sun.
    float e=1.0/terrainFrame.z;
    vec2 slope=vec2(terrainHeight(position.xz+vec2(e,0.0))-terrainHeight(position.xz-vec2(e,0.0)),
                    terrainHeight(position.xz+vec2(0.0,e))-terrainHeight(position.xz-vec2(0.0,e)))/(2.0*e);
    worldPosition=p;worldNormal=vec3(normal.x-normal.y*slope.x,normal.y,normal.z-normal.y*slope.y);color=vertexColor;
    lightPosition=lightMatrix*vec4(p,1.0);
    gl_Position=viewProjection*vec4(p,1.0);
})";
inline constexpr char litFragment[] = R"(
#version 330 core
in vec3 worldPosition;
in vec3 worldNormal;
in vec3 color;
in vec4 lightPosition;
uniform vec3 eyePosition;
uniform sampler2D shadowMap;
uniform bool shadowsEnabled;
uniform vec3 sunDirection;
uniform vec3 sunTint;
out vec4 fragmentColor;
float shade(vec3 n,vec3 sun){
    if(!shadowsEnabled)return 0.0;
    vec3 p=lightPosition.xyz/lightPosition.w*0.5+0.5;
    if(p.z>1.0||p.z<0.0||p.x<0.0||p.x>1.0||p.y<0.0||p.y>1.0)return 0.0;
    float bias=max(0.00025*(1.0-dot(n,sun)),0.00010);
    vec2 texel=1.0/vec2(textureSize(shadowMap,0));
    float blocked=0.0;
    for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)
        blocked += p.z-bias>texture(shadowMap,p.xy+vec2(x,y)*texel).r?1.0:0.0;
    return blocked/9.0;
}
void main(){
    vec3 n=normalize(worldNormal),sun=sunDirection;
    float diffuse=max(dot(n,sun),0.0),shadow=shade(n,sun);
    vec3 view=normalize(eyePosition-worldPosition);
    float spec=pow(max(dot(n,normalize(sun+view)),0.0),48.0)*0.16;
    vec3 linearColor=pow(color,vec3(2.2));
    // Direct sun (diffuse and highlight) carries the late-race tint; ambient sky light doesn't.
    vec3 lit=linearColor*(0.38+(1.0-shadow*0.82)*diffuse*0.85*sunTint)+sunTint*(spec*(1.0-shadow)*0.32);
    // Red gantry lamps remain legible in shadow without an engine/bloom effect.
    if(color.r>0.98&&color.g<0.04)lit=max(lit,linearColor*0.8);
    vec3 surface=pow(max(lit,vec3(0.0)),vec3(1.0/2.2));
    float haze=1.0-exp(-max(length(worldPosition-eyePosition)-6000.0,0.0)/16000.0);
    fragmentColor=vec4(mix(surface,vec3(.83,.88,.90),haze),1.0);
})";
inline constexpr char depthVertex[] = R"(
#version 330 core
layout(location=0) in vec3 position;
uniform mat4 lightMatrix;
// Terrain height under a world point: the same clamped bilinear lookup as
// Terrain::height in terrain.hpp, so CPU cameras and GPU geometry agree.
uniform sampler2D terrainMap;
uniform vec3 terrainFrame; // min x, min z, 1/cell
float terrainHeight(vec2 p){
    ivec2 size=textureSize(terrainMap,0);
    vec2 u=clamp((p-terrainFrame.xy)*terrainFrame.z,vec2(0.0),vec2(size-1));
    ivec2 i=min(ivec2(u),size-2);vec2 f=u-vec2(i);
    float a=texelFetch(terrainMap,i,0).r,b=texelFetch(terrainMap,i+ivec2(1,0),0).r;
    float c=texelFetch(terrainMap,i+ivec2(0,1),0).r,d=texelFetch(terrainMap,i+ivec2(1,1),0).r;
    return mix(mix(a,b,f.x),mix(c,d,f.x),f.y);
}
void main(){gl_Position=lightMatrix*vec4(position+vec3(0.0,terrainHeight(position.xz),0.0),1.0);}
)";
inline constexpr char depthFragment[] = R"(
#version 330 core
void main(){}
)";
inline GLuint program(const char* vs,const char* fs) {
    const auto shader=[](GLenum kind,const char* source){
        GLuint id=glCreateShader(kind);glShaderSource(id,1,&source,nullptr);glCompileShader(id);
        GLint okay=GL_FALSE;glGetShaderiv(id,GL_COMPILE_STATUS,&okay);
        if(okay!=GL_TRUE){char log[2048]{};glGetShaderInfoLog(id,sizeof(log),nullptr,log);std::cerr<<"3D shader: "<<log<<'\n';glDeleteShader(id);return GLuint{0};}
        return id;
    };
    GLuint a=shader(GL_VERTEX_SHADER,vs),b=shader(GL_FRAGMENT_SHADER,fs);
    if(!a||!b){glDeleteShader(a);glDeleteShader(b);return 0;}
    GLuint id=glCreateProgram();glAttachShader(id,a);glAttachShader(id,b);glLinkProgram(id);glDeleteShader(a);glDeleteShader(b);
    GLint okay=GL_FALSE;glGetProgramiv(id,GL_LINK_STATUS,&okay);
    if(okay!=GL_TRUE){char log[2048]{};glGetProgramInfoLog(id,sizeof(log),nullptr,log);std::cerr<<"3D program: "<<log<<'\n';glDeleteProgram(id);return 0;}
    return id;
}
struct Buffer {
    GLuint vao{},vbo{};GLsizei count{};
    void create() {
        glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);glBindVertexArray(vao);glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),nullptr);glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,nx)));glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(Vertex),reinterpret_cast<void*>(offsetof(Vertex,r)));glEnableVertexAttribArray(2);
        glBindVertexArray(0);
    }
    void update(const Mesh& mesh,GLenum usage=GL_DYNAMIC_DRAW) {
        count=static_cast<GLsizei>(mesh.size());glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,static_cast<GLsizeiptr>(mesh.size()*sizeof(Vertex)),mesh.data(),usage);
    }
    void draw(GLsizei first=0) const {glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,first,std::max(GLsizei{0},count-first));}
    void destroy(){glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
};
class Renderer {
public:
    bool initialize() {
        lit_=program(litVertex,litFragment);depth_=program(depthVertex,depthFragment);
        sky_=program(skyVertex,skyFragment);glGenVertexArrays(1,&skyVao_);
        if(!lit_||!depth_||!sky_)return false;
        // Terrain heights as a single-channel float texture, read with texelFetch.
        const auto& ground=terrain();
        glGenTextures(1,&terrainTexture_);glBindTexture(GL_TEXTURE_2D,terrainTexture_);
        glPixelStorei(GL_UNPACK_ALIGNMENT,4);
        glTexImage2D(GL_TEXTURE_2D,0,GL_R32F,ground.columns,ground.rows,0,GL_RED,GL_FLOAT,ground.heights.data());
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        world_.create();heat_.create();car_.create();lights_.create();crew_.create();anchors_.create();
        glGenFramebuffers(1,&shadowFbo_);glGenTextures(1,&shadowTexture_);glBindTexture(GL_TEXTURE_2D,shadowTexture_);
        glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,shadowSize,shadowSize,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
        const GLfloat border[]{1,1,1,1};glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,border);
        glBindFramebuffer(GL_FRAMEBUFFER,shadowFbo_);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,shadowTexture_,0);
        glDrawBuffer(GL_NONE);glReadBuffer(GL_NONE);shadows_=glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        if(!shadows_)std::cerr<<"Shadow buffer unavailable; directional lighting remains enabled.\n";

        return true;
    }
    void setWorld(const Mesh& mesh,const Mesh& anchors){world_.update(mesh,GL_STATIC_DRAW);anchors_.update(anchors,GL_STATIC_DRAW);}
    void setHeatmap(const Mesh& mesh){heat_.update(mesh,GL_STATIC_DRAW);}
    void update(const Mesh& car,const Mesh& lights,const Mesh& crew){car_.update(car);lights_.update(lights);crew_.update(crew);}
    // `sun` points towards the sun (sunDirection in sun_position.hpp).
    void draw(const CameraFrame& camera,int width,int height,int sceneWidth,bool points,V3 sun,V3 tint={1,1,1}) {
        glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
        const double extent=camera.shadowExtent;
        // Follow the car at metre precision; overview uses a whole-circuit map.
        const auto centre=camera.target;
        const auto& ground=terrain();
        const auto bindTerrain=[&](GLuint id){
            glActiveTexture(GL_TEXTURE1);glBindTexture(GL_TEXTURE_2D,terrainTexture_);glUniform1i(glGetUniformLocation(id,"terrainMap"),1);
            glUniform3f(glGetUniformLocation(id,"terrainFrame"),float(ground.minX),float(ground.minZ),float(1.0/ground.cell));
            glActiveTexture(GL_TEXTURE0);
        };
        lightMatrix_=orthographic(-extent,extent,-extent,extent,0.5,extent*5)*
            lookAt(centre+sun*(extent*2),centre);
        if(shadows_) {
            glBindFramebuffer(GL_FRAMEBUFFER,shadowFbo_);glViewport(0,0,shadowSize,shadowSize);glClear(GL_DEPTH_BUFFER_BIT);
            glUseProgram(depth_);glUniformMatrix4fv(glGetUniformLocation(depth_,"lightMatrix"),1,GL_FALSE,lightMatrix_.values.data());
            bindTerrain(depth_);
            glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(1.1F,2.0F);
            // The ground receives shadows but cannot cast one; skip it to avoid self-shadow precision artifacts.
            world_.draw(groundVertexCount());car_.draw();crew_.draw();glDisable(GL_POLYGON_OFFSET_FILL);
            glBindFramebuffer(GL_FRAMEBUFFER,0);
        }
        glViewport(0,0,width,height);glClearColor(0.025F,0.045F,0.055F,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST);glScissor(0,0,sceneWidth,height);glClearColor(0.68F,0.76F,0.80F,1);glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);glViewport(0,0,sceneWidth,height);
        glDisable(GL_DEPTH_TEST);glDepthMask(GL_FALSE);glUseProgram(sky_);glBindVertexArray(skyVao_);
        const auto forward=unit(camera.target-camera.eye),right=unit(cross(forward,{0,1,0})),up=cross(right,forward);
        glUniform3f(glGetUniformLocation(sky_,"forwardDirection"),float(forward.x),float(forward.y),float(forward.z));
        glUniform3f(glGetUniformLocation(sky_,"rightDirection"),float(right.x),float(right.y),float(right.z));
        glUniform3f(glGetUniformLocation(sky_,"upDirection"),float(up.x),float(up.y),float(up.z));
        glUniform1f(glGetUniformLocation(sky_,"aspectRatio"),float(sceneWidth)/height);
        glUniform1f(glGetUniformLocation(sky_,"tanHalfFov"),float(camera.tanHalfFov));
        // The sky pass draws the ground beyond the meshes analytically, so it needs height above the terrain.
        glUniform3f(glGetUniformLocation(sky_,"eyePosition"),float(camera.eye.x),float(camera.eye.y-ground.height(camera.eye.x,camera.eye.z)),float(camera.eye.z));
        glUniform3f(glGetUniformLocation(sky_,"sunDirection"),float(sun.x),float(sun.y),float(sun.z));
        glUniform3f(glGetUniformLocation(sky_,"sunTint"),float(tint.x),float(tint.y),float(tint.z));
        glDrawArrays(GL_TRIANGLES,0,3);glDepthMask(GL_TRUE);glEnable(GL_DEPTH_TEST);
        glUseProgram(lit_);glUniformMatrix4fv(glGetUniformLocation(lit_,"viewProjection"),1,GL_FALSE,camera.viewProjection.values.data());
        glUniformMatrix4fv(glGetUniformLocation(lit_,"lightMatrix"),1,GL_FALSE,lightMatrix_.values.data());
        glUniform3f(glGetUniformLocation(lit_,"eyePosition"),static_cast<float>(camera.eye.x),static_cast<float>(camera.eye.y),static_cast<float>(camera.eye.z));
        glUniform1i(glGetUniformLocation(lit_,"shadowsEnabled"),shadows_?1:0);
        glUniform3f(glGetUniformLocation(lit_,"sunDirection"),float(sun.x),float(sun.y),float(sun.z));
        glUniform3f(glGetUniformLocation(lit_,"sunTint"),float(tint.x),float(tint.y),float(tint.z));
        bindTerrain(lit_);
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,shadowTexture_);glUniform1i(glGetUniformLocation(lit_,"shadowMap"),0);
        world_.draw();
        // Pull the flat overlay slightly towards the camera so the asphalt never shows through.
        // A stronger pull (slope -2, 8 depth steps) keeps the 5 mm overlay ahead of the
        // asphalt; the depth test stays LEQUAL so exact ties go to the overlay.
        glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(-2.0F,-8.0F);heat_.draw();glDisable(GL_POLYGON_OFFSET_FILL);
        car_.draw();lights_.draw();crew_.draw();if(points)anchors_.draw();
        glDisable(GL_DEPTH_TEST);glViewport(0,0,width,height);
    }
    void destroy() {
        world_.destroy();heat_.destroy();car_.destroy();lights_.destroy();crew_.destroy();anchors_.destroy();
        glDeleteFramebuffers(1,&shadowFbo_);glDeleteTextures(1,&shadowTexture_);glDeleteTextures(1,&terrainTexture_);glDeleteProgram(lit_);glDeleteProgram(depth_);glDeleteProgram(sky_);glDeleteVertexArrays(1,&skyVao_);
    }
private:
    GLuint sky_{},skyVao_{};
    static constexpr int shadowSize=4096;
    GLuint lit_{},depth_{},shadowFbo_{},shadowTexture_{},terrainTexture_{};bool shadows_{};
    M4 lightMatrix_{};Buffer world_,heat_,car_,lights_,crew_,anchors_;
};
} // namespace apex3d::three
