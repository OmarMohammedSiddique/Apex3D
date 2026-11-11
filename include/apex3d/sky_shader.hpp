#pragma once
namespace apex3d::three {
inline constexpr char skyVertex[]=R"(
#version 330 core
out vec2 uv;
void main(){vec2 p=vec2((gl_VertexID<<1)&2,gl_VertexID&2);uv=p*2.0-1.0;gl_Position=vec4(uv,1,1);}
)";
inline constexpr char skyFragment[]=R"(
#version 330 core
in vec2 uv;
uniform vec3 forwardDirection,rightDirection,upDirection;
uniform float aspectRatio,tanHalfFov;
uniform vec3 eyePosition;
uniform vec3 sunDirection;
out vec4 fragmentColor;
float hash(vec2 p){return fract(sin(dot(p,vec2(127.1,311.7)))*43758.5453);}
float noise(vec2 p){vec2 i=floor(p),f=fract(p);f=f*f*(3.0-2.0*f);return mix(mix(hash(i),hash(i+vec2(1,0)),f.x),mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),f.x),f.y);}
float cloud(vec2 p){return noise(p)*.57+noise(p*2.1)*.28+noise(p*4.3)*.15;}
void main(){
 vec3 ray=normalize(forwardDirection+rightDirection*uv.x*aspectRatio*tanHalfFov+upDirection*uv.y*tanHalfFov);
 float elevation=max(ray.y,0.0);
 vec3 sky=mix(vec3(.83,.88,.90),vec3(.17,.43,.75),pow(elevation,.46));
 vec3 sun=sunDirection;float glow=max(dot(ray,sun),0.0);
 sky+=vec3(1,.83,.57)*(pow(glow,22.0)*.10+pow(glow,1200.0)*.8);
 if(ray.y>.035){vec2 p=ray.xz/(ray.y+.17)*2.0+vec2(8,13);
  float c=smoothstep(.52,.73,cloud(p))*smoothstep(.035,.18,ray.y);
  float detail=cloud(p+vec2(.15));sky=mix(sky,mix(vec3(.76,.80,.84),vec3(1,.99,.96),detail),c*.90);
 }
 sky=mix(sky,vec3(.84,.87,.86),(1.0-smoothstep(-.12,.04,ray.y))*.6);
 if(ray.y<0.0){
  float distanceToGround=max(eyePosition.y,0.0)/max(-ray.y,.0000001);
  // Same shading as the lit ground mesh (upward normal, no shadow), so the far ground matches it.
  vec3 grass=pow(pow(vec3(.24,.36,.23),vec3(2.2))*(.38+.85*max(sun.y,0.0)),vec3(1.0/2.2));
  float haze=1.0-exp(-max(distanceToGround-6000.0,0.0)/16000.0);
  sky=mix(grass,vec3(.83,.88,.90),haze);
 }
 fragmentColor=vec4(sky,1);
}
)";
} // namespace apex3d::three
