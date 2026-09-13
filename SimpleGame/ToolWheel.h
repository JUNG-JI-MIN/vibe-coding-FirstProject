#pragma once
#include <cmath>
#include "Dependencies/glew.h"
#include "Dependencies/freeglut.h"

class ToolWheel {
    bool open=false,pressed=false,flash=false;
    int hover=-1,equipped=-1,medkits=3;
    float animation=0,useTime=0,spin=0,scan=0;
    int mx=0,my=0;
    static void Box(float x,float y,float z,float w,float h,float d) {
        glPushMatrix();glTranslatef(x,y,z);glScalef(w,h,d);glutSolidCube(1);glPopMatrix();
    }
    static void Text(float x,float y,const char* p) {
        glRasterPos2f(x,y);for(;*p;++p)glutBitmapCharacter(GLUT_BITMAP_8_BY_13,*p);
    }
public:
    static const char* Name(int i) {
        const char* names[]={"FLASHLIGHT","DRILL","WRENCH","SCANNER","MEDKIT"};return i<0?"BARE HANDS":names[i];
    }
    bool IsOpen() const {return open;}
    int Equipped() const {return equipped;}
    bool LightOn() const {return equipped==0&&flash;}
    bool RepairHeld() const {return !open&&pressed&&(equipped==1||equipped==2);}
    bool Scanning() const {return scan>0&&equipped==3;}
    int Medkits() const {return medkits;}
    void Begin(int w,int h) {if(open)return;open=true;hover=-1;mx=w/2;my=h/2;animation=0;pressed=false;useTime=0;}
    void End() {if(!open)return;equipped=hover;open=false;pressed=false;useTime=0;scan=0;}
    void Cancel() {open=false;pressed=false;useTime=0;}
    void Mouse(int x,int y,int w,int h) {
        mx=x;my=y;if(!open)return;
        float dx=float(x-w/2),dy=float(y-h/2),scale=float(w)/1280;
        if(float(h)/720<scale)scale=float(h)/720;
        if(dx*dx+dy*dy<70*70*scale*scale) {hover=-1;return;}
        float angle=std::atan2(dy,dx)+3.14159265f/2+3.14159265f/5;
        if(angle<0)angle+=6.2831853f;
        hover=int(angle/(6.2831853f/5))%5;
    }
    void Button(bool down,bool active) {
        if(!active||open){pressed=false;return;}
        bool edge=down&&!pressed;pressed=down;
        if(edge&&equipped==0)flash=!flash;
        if(edge&&equipped==3)scan=5;
        if(!down)useTime=0;
    }
    void Update(float dt,float& health,bool active) {
        if(open) {animation+=dt*7;if(animation>1)animation=1;}
        if(!active) {pressed=false;useTime=0;return;}
        if(scan>0)scan-=dt;
        if(pressed)spin+=dt*(equipped==1?1800:240);
        if(pressed&&equipped==4&&medkits>0&&health<100) {
            useTime+=dt;if(useTime>=1.5f){health+=25;if(health>100)health=100;--medkits;useTime=0;pressed=false;}
        }
    }
    void DrawHeld(int w,int h) {
        if(equipped<0||open)return;
        glUseProgram(0);glDisable(GL_LIGHTING);glDisable(GL_FOG);glDisable(GL_BLEND);
        glClear(GL_DEPTH_BUFFER_BIT);glEnable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);glLoadIdentity();
        double aspect=double(w)/h;glFrustum(-.07*aspect,.07*aspect,-.07,.07,.1,10);
        glMatrixMode(GL_MODELVIEW);glLoadIdentity();
        glTranslatef(.43f,-.38f,-.95f);
        if(pressed)glRotatef(std::sin(spin*.0174533f)*6,1,0,0);
        glColor3f(.18f,.19f,.2f);Box(0,-.14f,.10f,.15f,.28f,.17f); // Gloved hand.
        glColor3f(.3f,.33f,.35f);
        if(equipped==0) {
            glutSolidCylinder(.09,.28,16,1);
            glColor3f(LightOn()?.85f:.2f,LightOn()?.9f:.22f,LightOn()?1.f:.25f);
            Box(0,0,-.01f,.13f,.13f,.025f);
        } else if(equipped==1) {
            Box(0,.04f,0,.24f,.18f,.35f);Box(.02f,-.1f,.08f,.11f,.27f,.11f);
            glPushMatrix();glTranslatef(0,.04f,-.30f);glRotatef(spin,0,0,1);
            glColor3f(.7f,.72f,.74f);glutSolidCylinder(.025,.18,6,1);glPopMatrix();
        } else if(equipped==2) {
            Box(0,.08f,0,.045f,.55f,.045f);
            Box(-.07f,.34f,0,.045f,.14f,.065f);Box(.07f,.34f,0,.045f,.14f,.065f);
            Box(0,.29f,0,.18f,.06f,.065f);
        } else if(equipped==3) {
            Box(0,.03f,0,.25f,.32f,.08f);
            glColor3f(.08f,.35f,.29f);Box(0,.07f,.05f,.20f,.19f,.015f);
        } else {
            Box(0,.03f,0,.28f,.25f,.14f);
            glColor3f(.8f,.8f,.8f);Box(0,.03f,.08f,.15f,.035f,.015f);Box(0,.03f,.08f,.035f,.15f,.015f);
        }
        glDisable(GL_DEPTH_TEST);
    }
    void DrawMenu(int w,int h) {
        if(!open)return;
        glMatrixMode(GL_PROJECTION);glLoadIdentity();glOrtho(0,w,h,0,-1,1);
        glMatrixMode(GL_MODELVIEW);glLoadIdentity();
        glDisable(GL_DEPTH_TEST);glDisable(GL_LIGHTING);glDisable(GL_FOG);
        glEnable(GL_BLEND);glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(0,0,0,.48f);glBegin(GL_QUADS);
        glVertex2f(0,0);glVertex2f(float(w),0);glVertex2f(float(w),float(h));glVertex2f(0,float(h));glEnd();
        float scale=float(w)/1280;if(float(h)/720<scale)scale=float(h)/720;
        float grow=1-std::pow(1-animation,3),inner=70*scale*grow,outer=205*scale*grow;
        for(int i=0;i<5;++i) {
            float center=-1.5707963f+i*6.2831853f/5;
            float shade=i==hover?.40f:.17f;
            glColor4f(shade,shade,shade,.97f);glBegin(GL_QUAD_STRIP);
            for(int n=0;n<=24;++n) {
                float a=center-.6283185f+.025f+(1.256637f-.05f)*n/24;
                glVertex2f(w*.5f+std::cos(a)*inner,h*.5f+std::sin(a)*inner);
                glVertex2f(w*.5f+std::cos(a)*outer,h*.5f+std::sin(a)*outer);
            }glEnd();
            if(animation>.6f) {
                glColor4f(.9f,.9f,.9f,1);
                float x=w*.5f+std::cos(center)*137*scale,y=h*.5f+std::sin(center)*137*scale;
                const char* name=Name(i);int len=0;for(const char* p=name;*p;++p)++len;
                Text(x-len*4,y,name);
            }
        }
        glColor4f(.85f,.85f,.85f,1);Text(w*.5f-40,h*.5f,"BARE HANDS");
        Text(w*.5f-132,h*.5f+240*scale,"RELEASE TAB TO EQUIP / CENTER: NONE");
        // In-window marker supplements the native mouse pointer.
        glColor4f(1,1,1,1);glBegin(GL_LINE_LOOP);
        for(int i=0;i<16;++i){float a=i*6.2831853f/16;glVertex2f(mx+std::cos(a)*4,my+std::sin(a)*4);}glEnd();
        glDisable(GL_BLEND);
    }
};
