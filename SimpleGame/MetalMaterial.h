#pragma once
#include <cstdio>
#include "Dependencies/glew.h"

inline GLuint ShipProgram(const char* vertexSource,const char* fragmentSource) {
    GLuint shaders[2]={0,0};
    const char* sources[2]={vertexSource,fragmentSource};
    for(int i=0;i<2;++i) {
        shaders[i]=glCreateShader(i==0?GL_VERTEX_SHADER:GL_FRAGMENT_SHADER);
        if(shaders[i]) {
            glShaderSource(shaders[i],1,&sources[i],nullptr); glCompileShader(shaders[i]);
            GLint ok=0; glGetShaderiv(shaders[i],GL_COMPILE_STATUS,&ok);
            if(ok) continue;
            char log[2048]={}; glGetShaderInfoLog(shaders[i],sizeof(log),nullptr,log);
            std::fprintf(stderr,"Ship shader: %s\n",log);
        }
        for(int j=0;j<=i;++j) if(shaders[j]) glDeleteShader(shaders[j]);
        return 0;
    }
    GLuint program=glCreateProgram();
    if(program) {
        glAttachShader(program,shaders[0]); glAttachShader(program,shaders[1]); glLinkProgram(program);
    }
    glDeleteShader(shaders[0]); glDeleteShader(shaders[1]);
    GLint ok=0; if(program) glGetProgramiv(program,GL_LINK_STATUS,&ok);
    if(!ok) {
        if(program) {
            char log[2048]={}; glGetProgramInfoLog(program,sizeof(log),nullptr,log);
            std::fprintf(stderr,"Ship program: %s\n",log); glDeleteProgram(program);
        }
        return 0;
    }
    return program;
}

class MetalMaterial {
    GLuint program=0;
    bool attempted=false;
public:
    bool flashlight=false;
    bool Initialize() {
        if(attempted) return program!=0;
        attempted=true;
        const char* vs=R"GLSL(#version 120
varying vec3 eyePosition;
varying vec3 eyeNormal;
varying vec3 localPosition;
varying vec3 tint;
void main() {
    eyePosition=(gl_ModelViewMatrix*gl_Vertex).xyz;
    eyeNormal=gl_NormalMatrix*gl_Normal;
    localPosition=gl_Vertex.xyz;
    tint=gl_Color.rgb;
    gl_Position=gl_ProjectionMatrix*vec4(eyePosition,1.0);
}
)GLSL";
        const char* fs=R"GLSL(#version 120
varying vec3 eyePosition;
varying vec3 eyeNormal;
varying vec3 localPosition;
varying vec3 tint;
uniform float metallic;
uniform float roughness;
uniform float emission;
uniform float fogEnabled;
uniform float flashlightOn;
const float PI=3.14159265;
vec3 lightBRDF(vec3 position,vec3 radiance,vec3 N,vec3 V,vec3 base,vec3 F0,float rough) {
    vec3 delta=position-eyePosition;
    float distanceSquared=max(dot(delta,delta),0.01);
    vec3 L=delta*inversesqrt(distanceSquared);
    vec3 H=normalize(V+L);
    float NoL=max(dot(N,L),0.0), NoV=max(dot(N,V),0.001);
    float NoH=max(dot(N,H),0.0), VoH=max(dot(V,H),0.0);
    float a=rough*rough, a2=a*a;
    float d=NoH*NoH*(a2-1.0)+1.0;
    float D=a2/max(PI*d*d,0.0001);
    float k=(rough+1.0)*(rough+1.0)/8.0;
    float G=(NoV/(NoV*(1.0-k)+k))*(NoL/(NoL*(1.0-k)+k));
    vec3 F=F0+(1.0-F0)*pow(1.0-VoH,5.0);
    vec3 specular=D*G*F/max(4.0*NoV*NoL,0.001);
    vec3 diffuse=(1.0-F)*(1.0-metallic)*base/PI;
    return (diffuse+specular)*radiance*NoL/(1.0+0.12*distanceSquared);
}
void main() {
    vec3 N=normalize(eyeNormal), V=normalize(-eyePosition);
    // Fine directional variation approximates brushed metal without external textures.
    float brush=sin(localPosition.y*260.0+sin(localPosition.z*31.0)*2.0);
    float filterWidth=fwidth(localPosition.y*260.0);
    brush*=1.0-smoothstep(0.8,3.0,filterWidth);
    vec3 base=pow(max(tint,vec3(0.0)),vec3(2.2));
    base*=1.0+0.07*brush*metallic;
    float rough=clamp(roughness+0.035*brush*metallic,0.12,0.95);
    vec3 F0=mix(vec3(0.04),base,metallic);
    // Low indirect fill keeps unlit metal legible; this is not a reflected scene map.
    vec3 color=base*0.10+F0*0.025;
    color+=lightBRDF(gl_LightSource[0].position.xyz,vec3(6.0,8.0,10.0),N,V,base,F0,rough);
    color+=lightBRDF(gl_LightSource[1].position.xyz,vec3(10.0,3.8,1.2),N,V,base,F0,rough);
    color+=lightBRDF(gl_LightSource[2].position.xyz,vec3(5.0,7.0,8.0),N,V,base,F0,rough);
    color+=lightBRDF(gl_LightSource[3].position.xyz,vec3(4.0,6.0,7.0),N,V,base,F0,rough);
    color+=lightBRDF(gl_LightSource[4].position.xyz,vec3(4.0,5.0,6.0),N,V,base,F0,rough);
    color+=lightBRDF(gl_LightSource[5].position.xyz,vec3(4.0,5.0,6.0),N,V,base,F0,rough);
    float cone=smoothstep(0.86,0.96,dot(normalize(eyePosition),vec3(0.0,0.0,-1.0)));
    color+=lightBRDF(vec3(0.0),vec3(18.0,19.0,20.0)*cone*flashlightOn,N,V,base,F0,rough);
    if(emission>0.0) color=base*emission;
    float fog=smoothstep(12.0,48.0,length(eyePosition))*fogEnabled;
    color=mix(color,vec3(0.003,0.005,0.009),fog);
    gl_FragColor=vec4(color,1.0);
}
)GLSL";
        program=ShipProgram(vs,fs); return program!=0;
    }
    void Use(float metal,float rough,float glow,bool fog) {
        if(!Initialize()) return;
        glUseProgram(program);
        glUniform1f(glGetUniformLocation(program,"metallic"),metal);
        glUniform1f(glGetUniformLocation(program,"roughness"),rough);
        glUniform1f(glGetUniformLocation(program,"emission"),glow);
        glUniform1f(glGetUniformLocation(program,"fogEnabled"),fog?1.f:0.f);
        glUniform1f(glGetUniformLocation(program,"flashlightOn"),flashlight?1.f:0.f);
    }
    void Release() { if(program) glDeleteProgram(program); program=0; attempted=false; }
};
