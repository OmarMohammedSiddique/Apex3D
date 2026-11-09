#pragma once
#include <glad/gl.h>
#include "scene3d.hpp"
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
void main(){
    worldPosition=position;worldNormal=normal;color=vertexColor;
    lightPosition=lightMatrix*vec4(position,1.0);
    gl_Position=viewProjection*vec4(position,1.0);
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
out vec4 fragmentColor;
float shade(vec3 n,vec3 sun){
    if(!shadowsEnabled)return 0.0;
    vec3 p=lightPosition.xyz/lightPosition.w*0.5+0.5;
    if(p.z>1.0||p.z<0.0)return 0.0;
    float bias=max(0.00065*(1.0-dot(n,sun)),0.00030);
    vec2 texel=1.0/vec2(textureSize(shadowMap,0));
    float blocked=0.0;
    for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y)
        blocked += p.z-bias>texture(shadowMap,p.xy+vec2(x,y)*texel).r?1.0:0.0;
    return blocked/9.0;
}
void main(){
    vec3 n=normalize(worldNormal),sun=normalize(vec3(-3.5,6.0,3.0));
    float diffuse=max(dot(n,sun),0.0),shadow=shade(n,sun);
    vec3 view=normalize(eyePosition-worldPosition);
    float spec=pow(max(dot(n,normalize(sun+view)),0.0),48.0)*0.16;
    vec3 linearColor=pow(color,vec3(2.2));
    vec3 lit=linearColor*(0.38+(1.0-shadow*0.82)*diffuse*0.85)+vec3(spec*(1.0-shadow)*0.32);
    // Red gantry lamps remain legible in shadow without an engine/bloom effect.
    if(color.r>0.98&&color.g<0.04)lit=max(lit,linearColor*0.8);
    fragmentColor=vec4(pow(max(lit,vec3(0.0)),vec3(1.0/2.2)),1.0);
})";
inline constexpr char depthVertex[] = R"(
#version 330 core
layout(location=0) in vec3 position;
uniform mat4 lightMatrix;
void main(){gl_Position=lightMatrix*vec4(position,1.0);}
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
    void draw() const {glBindVertexArray(vao);glDrawArrays(GL_TRIANGLES,0,count);}
    void destroy(){glDeleteBuffers(1,&vbo);glDeleteVertexArrays(1,&vao);}
};
class Renderer {
public:
    bool initialize() {
        lit_=program(litVertex,litFragment);depth_=program(depthVertex,depthFragment);
        if(!lit_||!depth_)return false;
        world_.create();car_.create();lights_.create();crew_.create();anchors_.create();
        glGenFramebuffers(1,&shadowFbo_);glGenTextures(1,&shadowTexture_);glBindTexture(GL_TEXTURE_2D,shadowTexture_);
        glTexImage2D(GL_TEXTURE_2D,0,GL_DEPTH_COMPONENT24,shadowSize,shadowSize,0,GL_DEPTH_COMPONENT,GL_FLOAT,nullptr);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_BORDER);glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_BORDER);
        const GLfloat border[]{1,1,1,1};glTexParameterfv(GL_TEXTURE_2D,GL_TEXTURE_BORDER_COLOR,border);
        glBindFramebuffer(GL_FRAMEBUFFER,shadowFbo_);glFramebufferTexture2D(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_TEXTURE_2D,shadowTexture_,0);
        glDrawBuffer(GL_NONE);glReadBuffer(GL_NONE);shadows_=glCheckFramebufferStatus(GL_FRAMEBUFFER)==GL_FRAMEBUFFER_COMPLETE;
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        if(!shadows_)std::cerr<<"Shadow buffer unavailable; directional lighting remains enabled.\n";
        lightMatrix_=orthographic(-3.2,3.2,-3.2,3.2,0.5,12.0)*lookAt({-3.5,6,3.0},{0,0,0});
        return true;
    }
    void setWorld(const Mesh& mesh,const Mesh& anchors){world_.update(mesh,GL_STATIC_DRAW);anchors_.update(anchors,GL_STATIC_DRAW);}
    void update(const Mesh& car,const Mesh& lights,const Mesh& crew){car_.update(car);lights_.update(lights);crew_.update(crew);}
    void draw(const CameraFrame& camera,int width,int height,int sceneWidth,bool points) {
        glEnable(GL_DEPTH_TEST);glDepthFunc(GL_LEQUAL);glDepthMask(GL_TRUE);glDisable(GL_BLEND);glDisable(GL_CULL_FACE);
        if(shadows_) {
            glBindFramebuffer(GL_FRAMEBUFFER,shadowFbo_);glViewport(0,0,shadowSize,shadowSize);glClear(GL_DEPTH_BUFFER_BIT);
            glUseProgram(depth_);glUniformMatrix4fv(glGetUniformLocation(depth_,"lightMatrix"),1,GL_FALSE,lightMatrix_.values.data());
            glEnable(GL_POLYGON_OFFSET_FILL);glPolygonOffset(1.1F,2.0F);
            world_.draw();car_.draw();crew_.draw();glDisable(GL_POLYGON_OFFSET_FILL);
            glBindFramebuffer(GL_FRAMEBUFFER,0);
        }
        glViewport(0,0,width,height);glClearColor(0.025F,0.045F,0.055F,1);glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glEnable(GL_SCISSOR_TEST);glScissor(0,0,sceneWidth,height);glClearColor(0.68F,0.76F,0.80F,1);glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);glViewport(0,0,sceneWidth,height);
        glUseProgram(lit_);glUniformMatrix4fv(glGetUniformLocation(lit_,"viewProjection"),1,GL_FALSE,camera.viewProjection.values.data());
        glUniformMatrix4fv(glGetUniformLocation(lit_,"lightMatrix"),1,GL_FALSE,lightMatrix_.values.data());
        glUniform3f(glGetUniformLocation(lit_,"eyePosition"),static_cast<float>(camera.eye.x),static_cast<float>(camera.eye.y),static_cast<float>(camera.eye.z));
        glUniform1i(glGetUniformLocation(lit_,"shadowsEnabled"),shadows_?1:0);
        glActiveTexture(GL_TEXTURE0);glBindTexture(GL_TEXTURE_2D,shadowTexture_);glUniform1i(glGetUniformLocation(lit_,"shadowMap"),0);
        world_.draw();car_.draw();lights_.draw();crew_.draw();if(points)anchors_.draw();
        glDisable(GL_DEPTH_TEST);glViewport(0,0,width,height);
    }
    void destroy() {
        world_.destroy();car_.destroy();lights_.destroy();crew_.destroy();anchors_.destroy();
        glDeleteFramebuffers(1,&shadowFbo_);glDeleteTextures(1,&shadowTexture_);glDeleteProgram(lit_);glDeleteProgram(depth_);
    }
private:
    static constexpr int shadowSize=4096;
    GLuint lit_{},depth_{},shadowFbo_{},shadowTexture_{};bool shadows_{};
    M4 lightMatrix_{};Buffer world_,car_,lights_,crew_,anchors_;
};
} // namespace apex3d::three
