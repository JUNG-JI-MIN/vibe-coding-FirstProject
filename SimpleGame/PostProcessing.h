#pragma once
#include "MetalMaterial.h"

// Scene radiance is preserved above 1.0 in an RGBA16F framebuffer until tone mapping.
class PostProcessing {
    GLuint program=0, texture=0, framebuffer=0, depth=0;
    int textureWidth=0, textureHeight=0;
    bool attempted=false, available=false, active=false;
    bool Initialize() {
        if(attempted) return available;
        attempted=true;
        if(!GLEW_VERSION_3_0) {
            std::fprintf(stderr,"HDR requires OpenGL 3.0; using the original scene renderer.\n");
            return false;
        }
        const char* vs=R"GLSL(#version 120
varying vec2 uv;
void main(){ uv=gl_MultiTexCoord0.xy; gl_Position=gl_Vertex; }
)GLSL";
        const char* fs=R"GLSL(#version 120
uniform sampler2D scene;
uniform float exposure;
uniform float strength;
uniform float bloomStrength;
uniform vec2 texel;
varying vec2 uv;
float finiteRadiance(float v) {
    // GLSL 1.20 has no isnan/isinf: unordered or negative values fail this test.
    if(!(v>=0.0)) return 0.0;
    return min(v,60000.0);
}
vec3 readScene(vec2 p) {
    vec3 c=texture2D(scene,p).rgb;
    return vec3(finiteRadiance(c.r),finiteRadiance(c.g),finiteRadiance(c.b));
}
vec3 bright(vec2 p) {
    vec3 c=readScene(p);
    float peak=max(c.r,max(c.g,c.b));
    return c*max(peak-1.0,0.0)/max(peak,0.0001);
}
vec3 toneMap(vec3 c) {
    // ACES-style fitted filmic curve, followed by display gamma conversion.
    return clamp((c*(2.51*c+0.03))/(c*(2.43*c+0.59)+0.14),0.0,1.0);
}
void main() {
    vec3 color=readScene(uv);
    vec3 bloom=vec3(0.0);
    for(int y=-2;y<=2;++y) for(int x=-2;x<=2;++x) {
        vec2 offset=vec2(float(x),float(y));
        float weight=(3.0-abs(float(x)))*(3.0-abs(float(y)));
        bloom+=bright(uv+offset*texel*2.5)*weight/81.0;
    }
    color=toneMap((color+bloom*bloomStrength)*exposure);
    vec2 p=(uv-0.5)*2.0;
    color*=1.0-strength*smoothstep(0.35,1.25,length(p));
    gl_FragColor=vec4(pow(max(color,vec3(0.0)),vec3(1.0/2.2)),1.0);
}
)GLSL";
        program=ShipProgram(vs,fs);
        if(!program) return false;
        glGenTextures(1,&texture); glGenFramebuffers(1,&framebuffer); glGenRenderbuffers(1,&depth);
        available=texture&&framebuffer&&depth; return available;
    }
public:
    float exposure=.85f;
    float vignetteStrength=.55f;
    float bloomStrength=.12f;
    bool Begin(int width,int height) {
        active=false;
        if(width<=0||height<=0||!Initialize()) return false;
        glBindFramebuffer(GL_FRAMEBUFFER,framebuffer);
        if(width!=textureWidth||height!=textureHeight) {
            glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,texture);
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16F,width,height,0,GL_RGBA,GL_FLOAT,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            glBindRenderbuffer(GL_RENDERBUFFER,depth);
            glRenderbufferStorage(GL_RENDERBUFFER,GL_DEPTH_COMPONENT24,width,height);
            glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0,GL_TEXTURE_2D,texture,0);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER,GL_DEPTH_ATTACHMENT,GL_RENDERBUFFER,depth);
            glDrawBuffer(GL_COLOR_ATTACHMENT0);
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glBindTexture(GL_TEXTURE_2D,0); glBindRenderbuffer(GL_RENDERBUFFER,0);
            if(glCheckFramebufferStatus(GL_FRAMEBUFFER)!=GL_FRAMEBUFFER_COMPLETE) {
                std::fprintf(stderr,"HDR framebuffer unavailable; using the original scene renderer.\n");
                glBindFramebuffer(GL_FRAMEBUFFER,0); available=false; return false;
            }
            textureWidth=width; textureHeight=height;
        }
        glDisable(GL_FRAMEBUFFER_SRGB);
        // The scene shader, not fixed-function color clamping, controls HDR radiance.
        glClampColor(GL_CLAMP_FRAGMENT_COLOR,GL_FALSE);
        glViewport(0,0,width,height); active=true; return true;
    }
    void Apply() {
        if(!active) return;
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glDrawBuffer(GL_BACK); glViewport(0,0,textureWidth,textureHeight);
        glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE);
        glDisable(GL_BLEND); glDisable(GL_LIGHTING); glDisable(GL_FOG);
        glDisable(GL_FRAMEBUFFER_SRGB); // Gamma is applied exactly once in the shader.
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D,texture);
        glUseProgram(program);
        glUniform1i(glGetUniformLocation(program,"scene"),0);
        glUniform1f(glGetUniformLocation(program,"exposure"),exposure);
        glUniform1f(glGetUniformLocation(program,"strength"),vignetteStrength);
        glUniform1f(glGetUniformLocation(program,"bloomStrength"),bloomStrength);
        glUniform2f(glGetUniformLocation(program,"texel"),1.f/textureWidth,1.f/textureHeight);
        glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex2f(-1,-1);
        glTexCoord2f(1,0); glVertex2f(1,-1);
        glTexCoord2f(1,1); glVertex2f(1,1);
        glTexCoord2f(0,1); glVertex2f(-1,1);
        glEnd();
        glUseProgram(0); glBindTexture(GL_TEXTURE_2D,0); glDepthMask(GL_TRUE);
        glClampColor(GL_CLAMP_FRAGMENT_COLOR,GL_FIXED_ONLY); active=false;
    }
    void Release() {
        if(depth) glDeleteRenderbuffers(1,&depth);
        if(framebuffer) glDeleteFramebuffers(1,&framebuffer);
        if(texture) glDeleteTextures(1,&texture);
        if(program) glDeleteProgram(program);
        texture=program=framebuffer=depth=0; textureWidth=textureHeight=0;
        attempted=available=active=false;
    }
};
