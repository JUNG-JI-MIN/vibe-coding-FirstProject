#pragma once
#include <vector>
#include <cmath>
#include <algorithm>
#include "MetalMaterial.h"
#include "Dependencies/freeglut.h"

class PipePuzzle {
    enum State { Leaking, Closed, Repaired, Flowing, Open };
    State state=Leaking;
    struct Particle { float x,y,z,vx,vy,vz,age,life,size; };
    std::vector<Particle> particles;
    float repair=0,flowTime=0,dissolve=0,clock=0,spawn=0,wheel=0,debrisSpawn=0;
    unsigned randomState=73821;
    bool previousE=false;
    int target=0;
    float Random() { randomState=randomState*1664525u+1013904223u; return (randomState>>8)/16777216.f; }
    static float Clamp(float v,float lo,float hi) { return v<lo?lo:(v>hi?hi:v); }
    static float Noise(int x,int y) { return float((x*37+y*71+x*y*13)%101)/101.f; }
    void Emit(bool directed,bool debris=false) {
        if(particles.size()>=1000) return;
        Particle p={};
        p.x=debris?39+(Random()-.5f)*2.3f:35;
        p.y=debris?-4+Random()*2.8f:-2.55f;
        p.z=debris?47:48;
        p.life=directed?2.7f:1.2f+Random()*1.2f;
        p.size=debris?.10f:.14f+Random()*.16f;
        p.vx=directed?2.1f+(Random()-.5f)*.35f:(Random()-.5f)*1.8f;
        p.vy=directed?(Random()-.5f)*.22f:.7f+Random()*.8f;
        p.vz=directed?-.525f+(Random()-.5f)*.12f:.8f+Random();
        if(debris) {p.vx=(Random()-.5f)*1.2f;p.vy=.5f+Random();p.vz=(Random()-.5f)*.7f;}
        particles.push_back(p);
    }
    void Cube(float x,float y,float z,float w,float h,float d) {
        glPushMatrix(); glTranslatef(x,y,z);glScalef(w,h,d);glutSolidCube(1);glPopMatrix();
    }
    void Pipe(float x,float y,float z,float dx,float dy,float dz,float radius) {
        float length=std::sqrt(dx*dx+dy*dy+dz*dz);
        glPushMatrix();glTranslatef(x,y,z);
        if(std::fabs(dz)<length-.001f) glRotatef(std::acos(dz/length)*57.29578f,-dy,dx,0);
        else if(dz<0) glRotatef(180,1,0,0);
        glutSolidCylinder(radius,length,12,1);glPopMatrix();
    }
public:
    // Door remains collidable until all fragments have disappeared.
    bool Blocks(float x,float y,float z) const {
        if(y>-.3f||y+1.8f<-4) return false;
        if(state!=Open&&std::fabs(x-39)<1.44f&&std::fabs(z-47)<.40f) return true;
        if(std::fabs(z-48)<.40f&&x>32.6f&&x<39.4f) return true;
        return false;
    }
    template<class Visible>
    void Update(float dt,float px,float py,float pz,float yaw,float pitch,bool e,bool active,Visible visible) {
        clock+=dt;
        target=0;
        float ya=yaw*.0174532925f,pa=pitch*.0174532925f;
        auto Aim=[&](float x,float y,float z) {
            float dx=x-px,dy=y-(py+1.65f),dz=z-pz;
            float distance=std::sqrt(dx*dx+dy*dy+dz*dz);
            if(!active||py> -2.8f||distance>2.4f||distance<.001f) return false;
            float dot=(dx*std::sin(ya)*std::cos(pa)-dy*std::sin(pa)-dz*std::cos(ya)*std::cos(pa))/distance;
            return dot>.88f&&visible(x,y,z);
        };
        if(Aim(33,-2.6f,48.25f)) target=1;
        if(Aim(35,-2.55f,48)) target=2;
        bool pressed=e&&!previousE; previousE=e;
        if(target==1&&pressed) {
            if(state==Leaking) state=Closed;
            else if(state==Closed) {state=Leaking;repair=0;}
            else if(state==Repaired) {state=Flowing;flowTime=0;}
        }
        if(state==Closed&&target==2&&e) {repair=Clamp(repair+dt/3,0,1);if(repair>=1)state=Repaired;}
        float desired=(state==Closed||state==Repaired)?1.f:0.f;
        wheel+=Clamp(desired-wheel,-dt*2,dt*2);
        if(state==Flowing) {
            flowTime+=dt;
            // Steam reaches the door before dissolution begins.
            dissolve=Clamp((flowTime-2)/5,0,1);
            if(dissolve>=1) state=Open;
        }
        float rate=state==Leaking?(std::sin(clock*5)>0?100.f:25.f):
            (state==Flowing?100.f:0.f);
        spawn+=rate*dt;
        while(spawn>=1) {Emit(state==Flowing);spawn-=1;}
        if(state==Flowing&&dissolve>0) {
            debrisSpawn+=120*dt;
            while(debrisSpawn>=1) {Emit(false,true);debrisSpawn-=1;}
        }
        for(auto& p:particles) {
            p.age+=dt;p.x+=p.vx*dt;p.y+=p.vy*dt;p.z+=p.vz*dt;
            if(state==Flowing&&p.z<47.18f) { p.z=47.18f;p.vx=0;p.vz=0;p.vy=.35f; }
            if(state!=Flowing) p.vy+=dt*.2f;
        }
        particles.erase(std::remove_if(particles.begin(),particles.end(),[](const Particle& p){return p.age>=p.life;}),particles.end());
    }
    const char* Prompt() const {
        if(target==1) return state==Leaking?"E  CLOSE VALVE":(state==Closed?"VALVE CLOSED / E REOPEN":
            (state==Repaired?"E  OPEN REPAIRED PIPE":"PRESSURE ROUTED TO DOOR"));
        if(target==2) return state==Leaking?"CLOSE THE VALVE BEFORE REPAIR":
            (state==Closed?"HOLD E  REPAIR PIPE (3 SEC)":"PIPE REPAIRED");
        return "";
    }
    float RepairProgress() const {return repair;}
    void Draw(bool hdr,MetalMaterial& material,float px,float py,float pz) {
        // Basement piping installation in the west part of the central corridor.
        if(py>1||std::fabs(px-35)>60||std::fabs(pz-48)>60) return;
        if(hdr) material.Use(.8f,.32f,0,true);
        glColor3f(.55f,.62f,.56f);
        for(int i=0;i<3;++i) {
            float z=48+i*.55f;
            Pipe(30,-.85f,z,12,0,0,.11f+i*.025f);
            for(float x : {31.f,34.f,37.f,40.f}) {
                Pipe(x,-.85f,z,.13f,0,0,.19f+i*.025f);
                Cube(x,-.55f,z,.06f,.55f,.06f);
            }
            glColor3f(.55f,.08f,.06f);Pipe(36,-.85f,z,.65f,0,0,.18f);
            glColor3f(.55f,.62f,.56f);
        }
        Pipe(33,-3.8f,48,0,2.95f,0,.13f);
        Pipe(33,-2.55f,48,2,0,0,.12f);
        Pipe(35,-2.55f,48,4,0,0,.12f);
        Pipe(39,-2.55f,48,0,0,-.7f,.12f);
        // Valve body and red handwheel, turned when pressure is isolated.
        glColor3f(.25f,.34f,.25f);glPushMatrix();glTranslatef(33,-2.6f,48);glutSolidSphere(.25,12,8);glPopMatrix();
        Pipe(33,-2.6f,48,0,0,.28f,.06f);
        glColor3f(.7f,.08f,.055f);
        glPushMatrix();glTranslatef(33,-2.6f,48.3f);glRotatef(wheel*270,0,0,1);
        glutSolidTorus(.035,.26,8,20);
        Cube(0,0,0,.50f,.035f,.04f);Cube(0,0,0,.035f,.5f,.04f);glPopMatrix();
        // Damaged joint turns into a metal repair sleeve.
        glColor3f(state==Leaking||state==Closed?.25f:.55f,.3f,.32f);
        Pipe(34.8f,-2.55f,48,.4f,0,0,.20f);
        if(state==Leaking||state==Closed) {
            glColor3f(.025f,.008f,.012f);Cube(35,-2.50f,48.20f,.18f,.07f,.04f);
        }
        // Corrodible bulkhead occupies an existing doorway; each tile evaporates separately.
        if(state!=Open) {
            if(hdr) material.Use(.55f,.65f,0,true);
            for(int i=0;i<12;++i) for(int j=0;j<16;++j) {
                float threshold=(j/16.f)*.65f+Noise(i,j)*.35f;
                if(dissolve>0&&threshold<dissolve) continue;
                glColor3f(.34f,.22f,.23f);
                Cube(37.9f+i*.2f,-3.9125f+j*.175f,47,.20f,.175f,.22f);
            }
        }
        glUseProgram(0);glDisable(GL_LIGHTING);
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);glDepthMask(GL_FALSE);
        GLfloat view[16];glGetFloatv(GL_MODELVIEW_MATRIX,view);
        std::sort(particles.begin(),particles.end(),[&](const Particle& a,const Particle& b) {
            return view[2]*a.x+view[6]*a.y+view[10]*a.z < view[2]*b.x+view[6]*b.y+view[10]*b.z;
        });
        for(const auto& p:particles) {
            float t=p.age/p.life,size=p.size*(1+t*2);
            float alpha=std::sin(t*3.14159265f)*.45f;
            glBegin(GL_TRIANGLE_FAN);glColor4f(.65f,.025f,.065f,alpha);glVertex3f(p.x,p.y,p.z);
            glColor4f(.22f,.005f,.025f,0);
            for(int i=0;i<=12;++i) {
                float a=i*6.2831853f/12,rx=std::cos(a)*size,uy=std::sin(a)*size;
                glVertex3f(p.x+view[0]*rx+view[1]*uy,p.y+view[4]*rx+view[5]*uy,p.z+view[8]*rx+view[9]*uy);
            }
            glEnd();
        }
        glDepthMask(GL_TRUE);glDisable(GL_BLEND);glColor4f(1,1,1,1);
    }
};
