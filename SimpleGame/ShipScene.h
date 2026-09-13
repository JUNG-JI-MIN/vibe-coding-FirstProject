#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdio>
#include "Dependencies/glew.h"
#include "Dependencies/freeglut.h"
#include "PostProcessing.h"
#include "ShipLayout.h"
#include "ColorQuest.h"
#include "ToolWheel.h"
#pragma comment(lib, "opengl32.lib")

// Traversable three-deck ship; rendering and movement share ShipLayout geometry.
class ShipScene {
    ShipLayout layout;
    ColorQuest pipePuzzle;
    ToolWheel tools;
    PostProcessing postProcessing;
    MetalMaterial metalMaterial;
    bool keys[256] = {};
    int width=1280, height=720, lastTime=0, mapDeck=1;
    float px=50, py=0, pz=85, yaw=0, pitch=0;
    bool overview=false, captured=true;
    float health=100.f, stamina=100.f, heartbeatPhase=0.f, staminaAlpha=0.f;
    bool exhausted=false;
    static float Clamp(float v,float lo,float hi) { return ShipLayout::Clamp(v,lo,hi); }
    int CurrentDeck() const { return int(Clamp(std::floor((py+4)/4+.5f),0,2)); }
    void Move(float dx,float dz) {
        // Short substeps prevent tunnelling through doorway posts while sprinting.
        int count=int(std::ceil(std::sqrt(dx*dx+dz*dz)/.08f));
        if(count<1) return;
        dx/=count; dz/=count;
        for(int i=0;i<count;++i) {
            float nextY=py;
            if(layout.Support(px+dx,pz,py,nextY)&&layout.CanStand(px+dx,nextY,pz)&&!pipePuzzle.Blocks(px+dx,nextY,pz)) {px+=dx;py=nextY;}
            if(layout.Support(px,pz+dz,py,nextY)&&layout.CanStand(px,nextY,pz+dz)&&!pipePuzzle.Blocks(px,nextY,pz+dz)) {pz+=dz;py=nextY;}
        }
    }
    void Text(float x,float y,const char* s) {
        glRasterPos2f(x,y);
        for(;*s;++s) glutBitmapCharacter(GLUT_BITMAP_8_BY_13,*s);
    }
    void Panel(float x,float y,float w,float h,float r,float g,float b,float a) {
        glColor4f(r,g,b,a); glBegin(GL_QUADS);
        glVertex2f(x,y); glVertex2f(x+w,y); glVertex2f(x+w,y+h); glVertex2f(x,y+h); glEnd();
    }
    static float Pulse(float phase) {
        phase-=std::floor(phase);
        const float t[]={0,.12f,.18f,.24f,.32f,.36f,.40f,.45f,.50f,.65f,.75f,.87f,1};
        const float v[]={0,0,.12f,0,0,-.2f,1,-.42f,0,0,.22f,0,0};
        for(int i=1;i<13;++i) if(phase<=t[i])
            return v[i-1]+(v[i]-v[i-1])*(phase-t[i-1])/(t[i]-t[i-1]);
        return 0;
    }
    void DrawHud() {
        // A fixed logical canvas keeps panels separated at any window size.
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,1280,720,0,-1,1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
        const float danger=1-health/100.f;
        const float red=.12f+.88f*danger, green=.7f-.58f*danger, blue=1-.88f*danger;
        const float x=982,y=558,w=270,h=134;
        Panel(x,y,w,h,.015f,.035f,.045f,.86f);
        Panel(x,y,3,h,red,green,blue,.9f);
        glColor4f(.65f,.76f,.8f,1); Text(x+16,y+23,"VITALS / ECG");
        char label[40]; sprintf_s(label,"HP %03d",int(health));
        glColor4f(red,green,blue,1); Text(x+192,y+23,label);
        glColor4f(.18f,.38f,.43f,.22f); glBegin(GL_LINES);
        for(int i=0;i<=12;++i) { float gx=x+16+i*20.f; glVertex2f(gx,y+36); glVertex2f(gx,y+100); }
        for(int i=0;i<5;++i) { float gy=y+36+i*16.f; glVertex2f(x+16,gy); glVertex2f(x+256,gy); }
        glEnd();
        // Scroll a continuous waveform; damage changes phase velocity without resetting it.
        for(int pass=0;pass<2;++pass) {
            glLineWidth(pass==0?4.f:1.5f); glBegin(GL_LINE_STRIP);
            for(int i=0;i<=240;++i) {
                float fade=.22f+.78f*i/240.f;
                glColor4f(red,green,blue,fade*(pass==0?.16f:1.f));
                float signal=health>0?Pulse(heartbeatPhase-(240-i)/100.f):0;
                glVertex2f(x+16+i,y+72-signal*27);
            }
            glEnd();
        }
        glLineWidth(1);
        glColor4f(red,green,blue,1);
        Text(x+16,y+119,health<=0?"NO SIGNAL":(health<30?"CRITICAL":(health<65?"CAUTION":"STABLE")));
        // Entire stamina widget disappears at full charge, including its background.
        if(staminaAlpha>0) {
            const float sx=490,sy=659;
            glColor4f(1,1,1,staminaAlpha*.75f); Text(sx,sy-12,exhausted?"RECOVERING":"STAMINA");
            Panel(sx,sy,300,5,1,1,1,staminaAlpha*.16f);
            Panel(sx,sy,300*stamina/100.f,5,1,1,1,staminaAlpha*.95f);
        }
        Panel(1052,230,200,112,.015f,.025f,.035f,.8f);
        glColor4f(.8f,.82f,.84f,1);Text(1066,254,"EQUIPPED / HOLD TAB");
        Text(1066,278,ToolWheel::Name(tools.Equipped()));
        Text(1066,304,pipePuzzle.HasCard()?"2F CARD: OWNED":"2F CARD: MISSING");
        char charges[40];sprintf_s(charges,"MEDKIT CHARGES: %d",tools.Medkits());Text(1066,326,charges);
        if(tools.Scanning()) {
            float best=10000;int index=0;
            for(int i=0;i<4;++i) {
                float x,z;ColorQuest::World(i,39,43.5f,x,z);
                float d=std::sqrt((x-px)*(x-px)+(z-pz)*(z-pz)+(py+4)*(py+4));
                if(d<best){best=d;index=i;}
            }
            char scan[96];sprintf_s(scan,"SCAN / NEAREST CORE: %s / %.1f m / B1",ColorQuest::Name(index),best);
            glColor4f(.7f,.9f,.8f,1);Text(22,220,scan);
        }
        for(int i=0;i<4;++i) {
            glColor4f(.72f,.8f,.85f,1); char coreLabel[64];
            sprintf_s(coreLabel,"%s: %s",ColorQuest::Name(i),pipePuzzle.ItemState(i));
            Text(22,112+i*20.f,coreLabel);
        }
        glDisable(GL_BLEND); glColor4f(1,1,1,1);
    }
public:
    void Button(int button,int state) {if(button==GLUT_LEFT_BUTTON)tools.Button(state==GLUT_DOWN,!overview&&captured);}
    void ReleaseGraphics() { metalMaterial.Release(); postProcessing.Release(); }
    void SetHealth(float value) { health=Clamp(value,0,100); }
    ShipScene() {
        lastTime=glutGet(GLUT_ELAPSED_TIME);
        glEnable(GL_DEPTH_TEST); glEnable(GL_NORMALIZE);
        glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
        glutSetCursor(GLUT_CURSOR_NONE);
    }
    void Resize(int w,int h) { width=w>0?w:1; height=h>0?h:1; glViewport(0,0,width,height); }
    void Key(unsigned char k,bool down) {
        if(k>='A' && k<='Z') k+=32;
        keys[k]=down;
        if(k==9) {
            if(down&&!overview&&captured) {
                if(!tools.IsOpen()) {tools.Begin(width,height);glutSetCursor(GLUT_CURSOR_INHERIT);glutWarpPointer(width/2,height/2);}
            } else if(!down&&tools.IsOpen()) {
                tools.End();glutSetCursor(GLUT_CURSOR_NONE);glutWarpPointer(width/2,height/2);
            }
            return;
        }
        if(tools.IsOpen()) {if(down&&k==27){tools.Cancel();glutSetCursor(GLUT_CURSOR_NONE);glutWarpPointer(width/2,height/2);}return;}
        if(!down) return;
        // Temporary UI preview controls until enemy damage and healing are connected.
        if(k=='[') SetHealth(health-10);
        if(k==']') SetHealth(health+10);
        if(k=='m') { overview=!overview; mapDeck=CurrentDeck(); }
        if(overview&&(k==','||k=='<')) mapDeck=mapDeck>0?mapDeck-1:0;
        if(overview&&(k=='.'||k=='>')) mapDeck=mapDeck<2?mapDeck+1:2;
        if(k==27) captured=!captured;
        glutSetCursor(captured&&!overview?GLUT_CURSOR_NONE:GLUT_CURSOR_INHERIT);
        if(captured&&!overview) glutWarpPointer(width/2,height/2);
    }
    void Mouse(int x,int y) {
        if(tools.IsOpen()){tools.Mouse(x,y,width,height);return;}
        if(!captured||overview) return;
        int dx=x-width/2,dy=y-height/2;
        if(!dx&&!dy) return;
        yaw+=dx*.12f; pitch=Clamp(pitch+dy*.12f,-80,80);
        glutWarpPointer(width/2,height/2);
    }
    void Update() {
        int now=glutGet(GLUT_ELAPSED_TIME);
        float dt=Clamp((now-lastTime)/1000.f,0,.05f); lastTime=now;
        heartbeatPhase+=dt*(1.0f+2.0f*(1-health/100.f));
        heartbeatPhase=std::fmod(heartbeatPhase,1000.f);
        bool focused=GetForegroundWindow()==GetActiveWindow();
        if(!focused&&tools.IsOpen()){tools.Cancel();glutSetCursor(GLUT_CURSOR_NONE);}
        bool active=!overview&&captured&&focused&&!tools.IsOpen();
        tools.Update(dt,health,active);
        metalMaterial.flashlight=tools.LightOn();
        pipePuzzle.Update(dt,px,py,pz,yaw,pitch,(keys['e']||tools.RepairHeld())&&active,active,[this](float x,float y,float z) {
            // Prevent interactions through bulkheads, slabs or furniture.
            for(int i=1;i<24;++i) {
                float t=i/24.f,rx=px+(x-px)*t,ry=py+1.65f+(y-py-1.65f)*t,rz=pz+(z-pz)*t;
                for(const auto& b:layout.boxes) {
                    if(!b.solid&&!b.slab) continue;
                    if(std::fabs(rx-b.x)<b.w*.5f&&std::fabs(ry-b.y)<b.h*.5f&&std::fabs(rz-b.z)<b.d*.5f) return false;
                }
            }
            return true;
        });
        // Poll Shift because freeglut's modifier mask is only valid inside input callbacks.
        bool shift=(GetAsyncKeyState(VK_SHIFT)&0x8000)!=0;
        bool moving=active&&(keys['w']!=keys['s']||keys['a']!=keys['d']);
        if(exhausted&&stamina>=20) exhausted=false;
        bool running=moving&&shift&&!exhausted&&stamina>0;
        stamina=Clamp(stamina+(running?-22.f:12.f)*dt,0,100);
        if(stamina<=0) exhausted=true;
        float target=stamina<100?1.f:0.f;
        staminaAlpha=Clamp(staminaAlpha+(target>staminaAlpha?5.f:-2.f)*dt,0,1);
        if(!active) return;
        float f=(keys['w']?1.f:0)-(keys['s']?1.f:0);
        float s=(keys['d']?1.f:0)-(keys['a']?1.f:0);
        float length=std::sqrt(f*f+s*s); if(length==0) return;
        float angle=yaw*.0174532925f, step=(running?4.3f:2.4f)*dt/length;
        float dx=(std::sin(angle)*f+std::cos(angle)*s)*step;
        float dz=(-std::cos(angle)*f+std::sin(angle)*s)*step;
        Move(dx,dz);

    }

    void Draw() {
        const bool hdr=!overview&&metalMaterial.Initialize()&&postProcessing.Begin(width,height);
        glClearColor(.012f,.02f,.028f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(0); glMatrixMode(GL_PROJECTION); glLoadIdentity();
        double aspect=double(width)/height;
        if(overview) {
            double sx=56*(aspect>1?aspect:1),sy=56*(aspect<1?1/aspect:1);
            glOrtho(-sx,sx,-sy,sy,.1,160);
        } else {
            double top=.07; glFrustum(-top*aspect,top*aspect,-top,top,.1,85);
        }
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        if(overview) { glRotatef(90,1,0,0); glTranslatef(-50,-80,-50); }
        else { glRotatef(pitch,1,0,0); glRotatef(yaw,0,1,0); glTranslatef(-px,-py-1.65f,-pz); }
        glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_LIGHT1);
        GLfloat ambient[]={.22f,.26f,.3f,1}; glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient);
        // Select nearby fixtures on the current vertical level instead of moving lamps with the player.
        std::vector<ShipLayout::Lamp> nearest=layout.lamps;
        std::sort(nearest.begin(),nearest.end(),[this](const ShipLayout::Lamp& a,const ShipLayout::Lamp& b) {
            float ay=a.y-(py+3.35f),by=b.y-(py+3.35f);
            float da=(a.x-px)*(a.x-px)+(a.z-pz)*(a.z-pz)+ay*ay*100;
            float db=(b.x-px)*(b.x-px)+(b.z-pz)*(b.z-pz)+by*by*100;
            return da<db;
        });
        for(int i=0;i<6&&i<int(nearest.size());++i) {
            GLfloat p[]={nearest[i].x,nearest[i].y,nearest[i].z,1};
            GLfloat color[]={.58f,.72f,.8f,1};
            glLightfv(GL_LIGHT0+i,GL_POSITION,p); glLightfv(GL_LIGHT0+i,GL_DIFFUSE,color);
            glLightf(GL_LIGHT0+i,GL_QUADRATIC_ATTENUATION,.04f);
        }
        if(!overview) {
            glEnable(GL_FOG); GLfloat fog[]={.012f,.02f,.028f,1};
            glFogfv(GL_FOG_COLOR,fog); glFogi(GL_FOG_MODE,GL_LINEAR);
            glFogf(GL_FOG_START,12); glFogf(GL_FOG_END,48);
        }
        for(const auto& b:layout.boxes) {
            if(overview) {
                if(b.deck!=mapDeck) continue;
                // Omit overhead doorway headers and luminaires in the cutaway plan.
                if(!b.slab&&b.y-b.h*.5f>ShipLayout::DeckY(mapDeck)+2.7f) continue;
            } else {
                float dx=Clamp(px,b.x-b.w*.5f,b.x+b.w*.5f)-px;
                float dz=Clamp(pz,b.z-b.d*.5f,b.z+b.d*.5f)-pz;
                if(dx*dx+dz*dz>65*65) continue;
                if(b.y+b.h*.5f<py-4.2f||b.y-b.h*.5f>py+7) continue;
            }
            if(hdr) metalMaterial.Use(b.metallic,b.roughness,b.emission,true);
            if(b.emission>0) glDisable(GL_LIGHTING);
            glColor3f(b.r,b.g,b.b); glPushMatrix(); glTranslatef(b.x,b.y,b.z);
            glScalef(b.w,b.h,b.d); glutSolidCube(1); glPopMatrix();
            if(b.emission>0) glEnable(GL_LIGHTING);
        }
        glUseProgram(0);
        if(!overview) pipePuzzle.Draw(hdr,metalMaterial,px,py,pz);
        if(overview) {
            glDisable(GL_LIGHTING);
            if(mapDeck==CurrentDeck()) {
                glColor3f(1,.7f,.15f); glPushMatrix(); glTranslatef(px,12,pz);
                glutSolidSphere(.8,12,8); glPopMatrix();
                float a=yaw*.0174532925f;
                glBegin(GL_LINES); glVertex3f(px,12,pz);
                glVertex3f(px+std::sin(a)*3,12,pz-std::cos(a)*3); glEnd();
            }
            glColor3f(.5f,.85f,1);
            for(const auto& room:layout.rooms) if(room.deck==mapDeck) {
                glRasterPos3f(room.x-7,12,room.z);
                for(const char* p=room.name;*p;++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10,*p);
            }
            glRasterPos3f(20,12,42);
            for(const char* p="STAIR W";*p;++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10,*p);
            glRasterPos3f(72,12,60);
            for(const char* p="STAIR E";*p;++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10,*p);
            if(mapDeck==0) {
                for(int i=0;i<4;++i) {
                    float x,z;ColorQuest::World(i,35,48,x,z);glRasterPos3f(x-3,12,z);
                    for(const char* p=ColorQuest::Name(i);*p;++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10,*p);
                }
                glRasterPos3f(48,12,50);
                for(const char* p="CORE HUB";*p;++p) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10,*p);
            }
        }
        if(hdr) postProcessing.Apply();
        if(!overview) tools.DrawHeld(width,height);
        glDisable(GL_FOG); glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,width,height,0,-1,1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        glColor3f(.7f,.86f,.87f);
        char title[128]; sprintf_s(title,"VESSEL / %s / 100 x 100 m",ShipLayout::DeckName(overview?mapDeck:CurrentDeck()));
        Text(22,28,title);
        Text(22,50,"WASD Move | SHIFT Run | M Map | ESC Cursor | [ ] Preview HP");
        if(overview) {
            Text(22,80,"< / > Select deck | Two stair cores connect B1 / 1F / 2F");
            Text(22,100,"6 m loop corridors + cross routes | Yellow marker: player");
        } else {
            glBegin(GL_LINES); glVertex2i(width/2-5,height/2); glVertex2i(width/2+5,height/2);
            glVertex2i(width/2,height/2-5); glVertex2i(width/2,height/2+5); glEnd();
            DrawHud();
            glColor3f(.95f,.72f,.65f);Text(420,598,pipePuzzle.Prompt());
            if(pipePuzzle.Prompt()[0]&&pipePuzzle.RepairProgress()>0&&pipePuzzle.RepairProgress()<1)
                Panel(490,612,300*pipePuzzle.RepairProgress(),4,.85f,.25f,.2f,1);
        }
        tools.DrawMenu(width,height);
        glEnable(GL_DEPTH_TEST); glutSwapBuffers();
    }
};


