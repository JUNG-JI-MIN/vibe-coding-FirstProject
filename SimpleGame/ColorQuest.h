#pragma once
#include "PipePuzzle.h"
#include <cstdio>

class ColorQuest {
    PipePuzzle puzzles[4];
    bool collected[4]={},inserted[4]={},gateOpen[2]={};
    bool cardReady=false,hasCard=false,previousE=false;
    int selected=-1;
    char prompt[128]={};
    float time=0;
    static void Rotation(int i,float& c,float& s) {
        const float cs[]={1,0,-1,0},sn[]={0,-1,0,1};c=cs[i];s=sn[i];
    }
    static void Cube(float x,float y,float z,float w,float h,float d) {
        glPushMatrix();glTranslatef(x,y,z);glScalef(w,h,d);glutSolidCube(1);glPopMatrix();
    }
    static void Transform(int i) {glTranslatef(50,0,50);glRotatef(-90.f*i,0,1,0);glTranslatef(-50,0,-50);}
public:
    static const char* Name(int i) {const char* names[]={"RED / WEST","GREEN / NORTH","BLUE / EAST","BLACK / SOUTH"};return names[i];}
    static void World(int i,float x,float z,float& wx,float& wz) {
        float c,s;Rotation(i,c,s);wx=50+c*(x-50)+s*(z-50);wz=50-s*(x-50)+c*(z-50);
    }
    static void Local(int i,float x,float z,float& lx,float& lz) {
        float c,s;Rotation(i,c,s);lx=50+c*(x-50)-s*(z-50);lz=50+s*(x-50)+c*(z-50);
    }
    static void Color(int i) {
        const float colors[4][3]={{.9f,.08f,.07f},{.08f,.8f,.18f},{.08f,.25f,.95f},{.09f,.1f,.13f}};
        glColor3fv(colors[i]);
    }
    bool HasCard() const {return hasCard;}
    const char* Prompt() const {return prompt;}
    float RepairProgress() const {return selected>=0?puzzles[selected].RepairProgress():0;}
    const char* ItemState(int i) const {return inserted[i]?"INSERTED":(collected[i]?"CARRIED":"MISSING");}
    bool Blocks(float x,float y,float z) const {
        for(int i=0;i<4;++i) {float lx,lz;Local(i,x,z,lx,lz);if(puzzles[i].Blocks(lx,y,lz))return true;}
        if(y<2.8f&&y+1.8f>0) {
            if(!gateOpen[0]&&x>20.34f&&x<23.86f&&std::fabs(z-44)<.38f)return true;
            if(!gateOpen[1]&&x>72.34f&&x<75.86f&&std::fabs(z-56)<.38f)return true;
        }
        return false;
    }
    template<class Visible>
    void Update(float dt,float px,float py,float pz,float yaw,float pitch,bool e,bool active,Visible visible) {
        time+=dt;prompt[0]=0;selected=-1;
        bool press=e&&!previousE;previousE=e;
        for(int i=0;i<4;++i) {
            float lx,lz;Local(i,px,pz,lx,lz);
            puzzles[i].Update(dt,lx,py,lz,yaw-90*i,pitch,e,active,[&](float x,float y,float z) {
                float wx,wz;World(i,x,z,wx,wz);return visible(wx,y,wz);
            });
            if(puzzles[i].Prompt()[0]) {selected=i;sprintf_s(prompt,"%s | %s",Name(i),puzzles[i].Prompt());}
        }
        auto Aim=[&](float x,float y,float z) {
            float dx=x-px,dy=y-py-1.65f,dz=z-pz,d=std::sqrt(dx*dx+dy*dy+dz*dz);
            float a=yaw*.0174532925f,b=pitch*.0174532925f;
            return active&&d>.01f&&d<2.4f&&(dx*std::sin(a)*std::cos(b)-dy*std::sin(b)-dz*std::cos(a)*std::cos(b))/d>.85f&&visible(x,y,z);
        };
        for(int i=0;i<4;++i) {
            float x,z;World(i,39,43.5f,x,z);
            if(!collected[i]&&!inserted[i]&&puzzles[i].IsOpen()&&Aim(x,-2.7f,z)) {
                sprintf_s(prompt,"E COLLECT %s CORE",Name(i));
                if(press) {collected[i]=true;sprintf_s(prompt,"%s CORE COLLECTED",Name(i));}
            }
        }
        if(Aim(50,-2.45f,50)) {
            int owned=0,done=0;for(int i=0;i<4;++i){if(collected[i])++owned;if(inserted[i])++done;}
            if(hasCard) sprintf_s(prompt,"2F ACCESS CARD ACQUIRED");
            else if(cardReady) {sprintf_s(prompt,"E TAKE 2F ACCESS CARD");if(press){hasCard=true;sprintf_s(prompt,"2F ACCESS CARD ACQUIRED");}}
            else if(owned) {
                sprintf_s(prompt,"E INSERT COLLECTED CORES (%d / 4 INSTALLED)",done);
                if(press) {
                    for(int i=0;i<4;++i) if(collected[i]){inserted[i]=true;collected[i]=false;}
                    cardReady=inserted[0]&&inserted[1]&&inserted[2]&&inserted[3];
                    if(cardReady)sprintf_s(prompt,"CARD ISSUED / RELEASE E, THEN TAKE CARD");
                }
            } else sprintf_s(prompt,"COLLECT RED GREEN BLUE BLACK (%d / 4)",done);
        }
        for(int i=0;i<2;++i) if(!gateOpen[i]&&Aim(i?74.1f:22.1f,1.4f,i?55.8f:44.2f)) {
            sprintf_s(prompt,hasCard?"E USE CARD / UNLOCK 2F":"LOCKED / 2F ACCESS CARD REQUIRED");
            if(press&&hasCard)gateOpen[i]=true;
        }
    }
    void Draw(bool hdr,MetalMaterial& material,float px,float py,float pz) {
        for(int i=0;i<4;++i) {
            float lx,lz;Local(i,px,pz,lx,lz);
            glPushMatrix();Transform(i);puzzles[i].Draw(hdr,material,lx,py,lz);glPopMatrix();
        }
        if(hdr)material.Use(.65f,.35f,0,true);else glEnable(GL_LIGHTING);
        if(py<1) {
            for(int i=0;i<4;++i) {
                float x,z;World(i,39,43.5f,x,z);
                // Reward pedestal remains after pickup; white trim keeps BLACK distinguishable.
                // Solid pedestal is rendered by ShipLayout, which also owns its collision.
                if(!collected[i]&&!inserted[i]) {
                    glColor3f(.7f,.75f,.8f);Cube(x,-2.78f,z,.46f,.16f,.46f);
                    Color(i);glPushMatrix();glTranslatef(x,-2.63f,z);glRotatef(time*25,0,1,0);glutSolidCube(.32f);glPopMatrix();
                }
            }
            // Central cabinet is also part of the shared static layout.
            for(int i=0;i<4;++i) {
                glColor3f(.65f,.7f,.73f);Cube(49.52f+i*.32f,-2.76f,50,.28f,.08f,.35f);
                if(inserted[i])Color(i);else glColor3f(.03f,.04f,.045f);
                Cube(49.52f+i*.32f,-2.67f,50,.22f,.14f,.28f);
            }
            if(cardReady&&!hasCard) {glColor3f(.8f,.9f,.85f);Cube(50,-2.46f,50,.38f,.03f,.23f);}
        }
        for(int i=0;i<2;++i) if(!gateOpen[i]) {
            glColor3f(.28f,.34f,.38f);Cube(i?74.1f:22.1f,1.4f,i?56:44,3,2.8f,.22f);
            glColor3f(hasCard?.12f:.8f,hasCard?.8f:.1f,.1f);
            Cube(i?74.1f:22.1f,1.4f,i?55.87f:44.13f,.3f,.4f,.05f);
        }
        glUseProgram(0);
        // Render translucent steam after every opaque puzzle and reward object.
        for(int i=0;i<4;++i) {
            float lx,lz;Local(i,px,pz,lx,lz);
            glPushMatrix();Transform(i);puzzles[i].Draw(hdr,material,lx,py,lz,true);glPopMatrix();
        }
    }
};
