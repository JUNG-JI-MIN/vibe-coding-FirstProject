#pragma once
#include <cstdio>
#include "Dependencies/glew.h"

// Captures the completed scene before the HUD and composites a fullscreen pass.
class PostProcessing {
    GLuint program=0, texture=0;
    int textureWidth=0, textureHeight=0;
    bool attempted=false;
    GLuint Compile(GLenum type,const char* source) {
        GLuint shader=glCreateShader(type);
        if(!shader) return 0;
        glShaderSource(shader,1,&source,nullptr); glCompileShader(shader);
        GLint ok=0; glGetShaderiv(shader,GL_COMPILE_STATUS,&ok);
        if(!ok) {
            char log[2048]={}; glGetShaderInfoLog(shader,sizeof(log),nullptr,log);
            std::fprintf(stderr,"Post-processing shader: %s\n",log);
            glDeleteShader(shader); return 0;
        }
        return shader;
    }
    bool Initialize() {
        if(attempted) return program!=0;
        attempted=true;
        const char* vs=
            "#version 120\n"
            "varying vec2 uv;\n"
            "void main(){ uv=gl_MultiTexCoord0.xy; gl_Position=gl_Vertex; }\n";
        const char* fs=
            "#version 120\n"
            "uniform sampler2D scene;\n"
            "uniform float exposure;\n"
            "uniform float strength;\n"
            "varying vec2 uv;\n"
            "void main(){\n"
            " vec3 color=texture2D(scene,uv).rgb;\n"
            // Normalized screen coordinates create a soft oval at any aspect ratio.
            " vec2 p=(uv-0.5)*2.0;\n"
            " float edge=smoothstep(0.35,1.25,length(p));\n"
            " color*=exposure*(1.0-strength*edge);\n"
            " gl_FragColor=vec4(color,1.0);\n"
            "}\n";
        GLuint vertex=Compile(GL_VERTEX_SHADER,vs), fragment=Compile(GL_FRAGMENT_SHADER,fs);
        if(!vertex||!fragment) {
            if(vertex) glDeleteShader(vertex);
            if(fragment) glDeleteShader(fragment);
            return false;
        }
        program=glCreateProgram();
        if(program) {
            glAttachShader(program,vertex); glAttachShader(program,fragment); glLinkProgram(program);
        }
        glDeleteShader(vertex); glDeleteShader(fragment);
        GLint ok=0;
        if(program) glGetProgramiv(program,GL_LINK_STATUS,&ok);
        if(!ok) {
            if(program) {
                char log[2048]={}; glGetProgramInfoLog(program,sizeof(log),nullptr,log);
                std::fprintf(stderr,"Post-processing link: %s\n",log);
                glDeleteProgram(program); program=0;
            }
            return false;
        }
        glGenTextures(1,&texture);
        return true;
    }
public:
    float exposure=.78f;
    float vignetteStrength=.46f;
    void Release() {
        if(texture) glDeleteTextures(1,&texture);
        if(program) glDeleteProgram(program);
        texture=program=0; textureWidth=textureHeight=0; attempted=false;
    }
    void Apply(int width,int height) {
        if(width<=0||height<=0||!Initialize()||!texture) return;
        GLint oldProgram=0, oldActive=0, oldTexture=0;
        glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgram);
        glGetIntegerv(GL_ACTIVE_TEXTURE,&oldActive);
        glActiveTexture(GL_TEXTURE0);
        glGetIntegerv(GL_TEXTURE_BINDING_2D,&oldTexture);
        glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_TEXTURE_BIT);
        glBindTexture(GL_TEXTURE_2D,texture);
        if(width!=textureWidth||height!=textureHeight) {
            glTexImage2D(GL_TEXTURE_2D,0,GL_RGB8,width,height,0,GL_RGB,GL_UNSIGNED_BYTE,nullptr);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
            textureWidth=width; textureHeight=height;
        }
        GLint oldRead=0; glGetIntegerv(GL_READ_BUFFER,&oldRead);
        glReadBuffer(GL_BACK);
        glCopyTexSubImage2D(GL_TEXTURE_2D,0,0,0,0,0,width,height);
        glReadBuffer(oldRead);
        glDisable(GL_DEPTH_TEST); glDepthMask(GL_FALSE);
        glDisable(GL_BLEND); glDisable(GL_LIGHTING); glDisable(GL_FOG);
        glUseProgram(program);
        glUniform1i(glGetUniformLocation(program,"scene"),0);
        glUniform1f(glGetUniformLocation(program,"exposure"),exposure);
        glUniform1f(glGetUniformLocation(program,"strength"),vignetteStrength);
        glBegin(GL_QUADS);
        glTexCoord2f(0,0); glVertex2f(-1,-1);
        glTexCoord2f(1,0); glVertex2f(1,-1);
        glTexCoord2f(1,1); glVertex2f(1,1);
        glTexCoord2f(0,1); glVertex2f(-1,1);
        glEnd();
        glUseProgram(oldProgram); glPopAttrib();
        glBindTexture(GL_TEXTURE_2D,oldTexture); glActiveTexture(oldActive);
    }
};
