#pragma once
#include <cmath>
#include <vector>
#include <algorithm>
#include <cstdio>
#include "Dependencies/glew.h"
#include "Dependencies/freeglut.h"
#include "PostProcessing.h"
#pragma comment(lib, "opengl32.lib")

// One world unit is one metre; outer hull occupies [0,20] on X and Z.
class ShipScene {
    struct Box { float x,y,z,w,h,d,r,g,b; bool solid; };
    std::vector<Box> boxes;
    PostProcessing postProcessing;
    bool keys[256] = {};
    int width=1280, height=720, lastTime=0;
    float px=10, pz=17.5f, yaw=0, pitch=0;
    bool overview=false, captured=true;
    float health=100.f, stamina=100.f, heartbeatPhase=0.f, staminaAlpha=0.f;
    bool exhausted=false;
    int selectedTool=0;
    struct ToolSlot { const char* name; int count; };
    ToolSlot toolSlots[4]={{"FLASHLIGHT",0},{"ACCESS KEY",0},{"MEDKIT",0},{"UTILITY",0}};
    static float Clamp(float v,float lo,float hi) { return v<lo?lo:(v>hi?hi:v); }
    void Add(float x,float y,float z,float w,float h,float d,float r,float g,float b,bool solid=true) {
        boxes.push_back({x,y,z,w,h,d,r,g,b,solid});
    }
    void Wall(float x,float z,float w,float d) {
        Add(x,1.5f,z,w,3,d,.16f,.21f,.24f);
        Add(x,.16f,z,w+.015f,.12f,d+.015f,.07f,.1f,.12f,false);
        Add(x,2.75f,z,w+.02f,.08f,d+.02f,.32f,.38f,.4f,false);
    }
    void DoorFrame(float x,float z,bool alongZ) {
        if(alongZ) {
            Add(x,1.3f,z-.9f,.4f,2.6f,.16f,.3f,.36f,.38f);
            Add(x,1.3f,z+.9f,.4f,2.6f,.16f,.3f,.36f,.38f);
            Add(x,2.7f,z,.4f,.6f,1.95f,.25f,.3f,.33f);
            Add(x,2.35f,z,.43f,.07f,1.4f,.12f,.8f,.7f,false);
        } else {
            Add(x-.9f,1.3f,z,.16f,2.6f,.4f,.3f,.36f,.38f);
            Add(x+.9f,1.3f,z,.16f,2.6f,.4f,.3f,.36f,.38f);
            Add(x,2.7f,z,1.95f,.6f,.4f,.25f,.3f,.33f);
            Add(x,2.35f,z,1.4f,.07f,.43f,.12f,.8f,.7f,false);
        }
    }
    void Locker(float x,float z) {
        Add(x,1.05f,z,.85f,2.1f,.65f,.19f,.27f,.28f);
        Add(x,1.05f,z+.34f,.73f,1.94f,.04f,.12f,.19f,.2f,false);
        Add(x+.23f,1.05f,z+.38f,.05f,.27f,.05f,.7f,.75f,.7f,false);
        for(int i=0;i<4;++i) Add(x,1.65f+i*.08f,z+.38f,.48f,.025f,.02f,.035f,.05f,.055f,false);
    }
    bool CanStand(float x,float z) const {
        const float radius=.22f;
        if(x<radius+.2f || x>19.8f-radius || z<radius+.2f || z>19.8f-radius) return false;
        for(const auto& b:boxes) {
            if(!b.solid || b.y-b.h*.5f>1.8f || b.y+b.h*.5f<.05f) continue;
            float nx=Clamp(x,b.x-b.w*.5f,b.x+b.w*.5f);
            float nz=Clamp(z,b.z-b.d*.5f,b.z+b.d*.5f);
            if((x-nx)*(x-nx)+(z-nz)*(z-nz)<radius*radius) return false;
        }
        return true;
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
        Panel(1072,216,180,302,.015f,.035f,.045f,.78f);
        glColor4f(.65f,.76f,.8f,1); Text(1086,240,"TOOLS / 1-4");
        for(int i=0;i<4;++i) {
            float ty=252+i*64.f;
            bool selected=i==selectedTool;
            Panel(1080,ty,164,58,.2f,.4f,.47f,selected?.26f:.07f);
            if(selected) Panel(1080,ty,2,58,.4f,.8f,1,.9f);
            glColor4f(.65f,.8f,.86f,1); sprintf_s(label,"%d",i+1); Text(1088,ty+20,label);
            Text(1105,ty+20,toolSlots[i].name);
            glColor4f(.48f,.58f,.63f,1);
            if(toolSlots[i].count>0) sprintf_s(label,"OWNED x%d",toolSlots[i].count);
            else sprintf_s(label,"EMPTY");
            Text(1105,ty+42,label);
        }
        glDisable(GL_BLEND); glColor4f(1,1,1,1);
    }
public:
    void ReleaseGraphics() { postProcessing.Release(); }
    void SetHealth(float value) { health=Clamp(value,0,100); }
    void SetToolCount(int slot,int count) { if(slot>=0&&slot<4) toolSlots[slot].count=count>0?count:0; }
    ShipScene() {
        Wall(10,.1f,20,.2f); Wall(10,19.9f,20,.2f);
        Wall(.1f,10,.2f,20); Wall(19.9f,10,.2f,20);
        // Central corridor: X=8..12. Side-room entrances at Z=4,10,16.
        for(float x : {8.f,12.f}) {
            float start=.2f;
            for(float door : {4.f,10.f,16.f}) {
                Wall(x,(start+door-1)*.5f,.2f,door-1-start);
                DoorFrame(x,door,true); start=door+1;
            }
            Wall(x,(start+19.8f)*.5f,.2f,19.8f-start);
        }
        for(float z : {7.f,13.f}) { Wall(4,z,7.8f,.2f); Wall(16,z,7.8f,.2f); }
        // The six rooms are accessible independently through the central corridor.
        for(float z : {2.f,5.5f,8.f,11.5f,14.f,18.f}) {
            Add(9,.035f,z,.045f,.02f,1.2f,.08f,.6f,.56f,false);
            Add(11,.035f,z,.045f,.02f,1.2f,.08f,.6f,.56f,false);
            Add(10,2.94f,z,1.2f,.06f,.18f,.6f,.85f,.86f,false);
        }
        // Cargo, southwest.
        Add(2,.6f,15,1.4f,1.2f,1.4f,.32f,.27f,.18f);
        Add(3.6f,.45f,16.4f,1.1f,.9f,1.1f,.27f,.25f,.19f);
        Add(2,1.55f,15,1, .7f,1,.25f,.23f,.18f);
        Locker(6,18.9f); Locker(7,18.9f);
        // Crew room, west middle.
        for(float z : {8.6f,11.3f}) {
            Add(2,.32f,z,2.5f,.64f,1.05f,.19f,.24f,.27f);
            Add(2,.7f,z,2.3f,.16f,.92f,.3f,.36f,.36f,false);
        }
        Locker(6,7.7f);
        // Engineering, northwest.
        Add(3,1.1f,3.4f,2.2f,2.2f,2.2f,.23f,.27f,.29f);
        Add(3,1.25f,4.52f,1.5f,.35f,.04f,.7f,.25f,.08f,false);
        for(float x : {1.f,2.f,3.f,4.f,5.f,6.f}) Add(x,2.65f,3.4f,.15f,.15f,5.8f,.32f,.36f,.37f,false);
        // Control room, northeast. Console faces toward entrance.
        Add(17,.55f,3.4f,3.4f,1.1f,1.1f,.13f,.2f,.22f);
        for(float x : {15.9f,17.f,18.1f}) {
            Add(x,1.35f,3.2f,.85f,.6f,.12f,.03f,.08f,.09f);
            Add(x,1.35f,3.28f,.73f,.46f,.025f,.1f,.6f,.55f,false);
        }
        // Escape room, east middle: sealed exit is a visual placeholder.
        Add(19.65f,1.25f,10,.18f,2.5f,1.8f,.22f,.31f,.33f);
        Add(19.53f,2.6f,10,.06f,.15f,1.7f,.15f,.9f,.48f,false);
        Add(17,.4f,8.4f,2,.8f,.8f,.23f,.29f,.3f);
        // Maintenance, southeast.
        Add(17,.5f,17.8f,3,1,1,.21f,.26f,.28f);
        Locker(14,18.9f); Locker(15,18.9f);
        // Key and puzzle-panel stand-ins.
        Add(17,1.06f,17.8f,.3f,.08f,.12f,.95f,.68f,.15f,false);
        Add(6.8f,1.1f,1,1,1.5f,.5f,.18f,.23f,.25f);
        Add(6.8f,1.45f,1.27f,.65f,.38f,.04f,.8f,.32f,.08f,false);
        lastTime=glutGet(GLUT_ELAPSED_TIME);
        glEnable(GL_DEPTH_TEST); glEnable(GL_NORMALIZE);
        glEnable(GL_COLOR_MATERIAL); glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
        glutSetCursor(GLUT_CURSOR_NONE);
    }
    void Resize(int w,int h) { width=w>0?w:1; height=h>0?h:1; glViewport(0,0,width,height); }
    void Key(unsigned char k,bool down) {
        if(k>='A' && k<='Z') k+=32;
        keys[k]=down;
        if(!down) return;
        if(k>='1'&&k<='4') selectedTool=k-'1';
        // Temporary UI preview controls until enemy damage and healing are connected.
        if(k=='[') SetHealth(health-10);
        if(k==']') SetHealth(health+10);
        if(k=='m') overview=!overview;
        if(k==27) captured=!captured;
        glutSetCursor(captured&&!overview?GLUT_CURSOR_NONE:GLUT_CURSOR_INHERIT);
        if(captured&&!overview) glutWarpPointer(width/2,height/2);
    }
    void Mouse(int x,int y) {
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
        bool active=!overview&&captured&&GetForegroundWindow()==GetActiveWindow();
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
        if(CanStand(px+dx,pz)) px+=dx;
        if(CanStand(px,pz+dz)) pz+=dz;
    }
    void Draw() {
        glClearColor(.012f,.02f,.028f,1); glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        glUseProgram(0); glMatrixMode(GL_PROJECTION); glLoadIdentity();
        double aspect=double(width)/height;
        if(overview) {
            double sx=11.5*(aspect>1?aspect:1), sy=11.5*(aspect<1?1/aspect:1);
            glOrtho(-sx,sx,-sy,sy,.1,60);
        } else {
            double top=.07; glFrustum(-top*aspect,top*aspect,-top,top,.1,60);
        }
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        if(overview) { glRotatef(90,1,0,0); glTranslatef(-10,-30,-10); }
        else { glRotatef(pitch,1,0,0); glRotatef(yaw,0,1,0); glTranslatef(-px,-1.65f,-pz); }
        glEnable(GL_LIGHTING); glEnable(GL_LIGHT0); glEnable(GL_LIGHT1);
        GLfloat ambient[]={.22f,.26f,.3f,1}; glLightModelfv(GL_LIGHT_MODEL_AMBIENT,ambient);
        GLfloat cool[]={.58f,.72f,.8f,1}, warm[]={.5f,.25f,.12f,1};
        GLfloat p0[]={10,2.8f,7,1},p1[]={3,2.6f,3,1};
        glLightfv(GL_LIGHT0,GL_POSITION,p0); glLightfv(GL_LIGHT0,GL_DIFFUSE,cool);
        glLightfv(GL_LIGHT1,GL_POSITION,p1); glLightfv(GL_LIGHT1,GL_DIFFUSE,warm);
        glLightf(GL_LIGHT0,GL_QUADRATIC_ATTENUATION,.012f);
        glLightf(GL_LIGHT1,GL_QUADRATIC_ATTENUATION,.04f);
        if(!overview) {
            glEnable(GL_FOG); GLfloat fog[]={.012f,.02f,.028f,1};
            glFogfv(GL_FOG_COLOR,fog); glFogi(GL_FOG_MODE,GL_LINEAR); glFogf(GL_FOG_START,5); glFogf(GL_FOG_END,23);
        }
        // Floor plates leave dark seams without extra texture files.
        for(int x=0;x<20;++x) for(int z=0;z<20;++z) {
            glColor3f(.115f,.145f,.16f); glPushMatrix(); glTranslatef(x+.5f,-.05f,z+.5f);
            glScalef(.985f,.1f,.985f); glutSolidCube(1); glPopMatrix();
        }
        for(const auto& b:boxes) {
            if(overview && b.y>2.4f) continue;
            bool emissive=b.g>.5f || b.r>.65f;
            if(emissive) glDisable(GL_LIGHTING);
            glColor3f(b.r,b.g,b.b); glPushMatrix(); glTranslatef(b.x,b.y,b.z);
            glScalef(b.w,b.h,b.d); glutSolidCube(1); glPopMatrix();
            if(emissive) glEnable(GL_LIGHTING);
        }
        if(!overview) {
            glColor3f(.075f,.095f,.115f); glPushMatrix(); glTranslatef(10,3.05f,10);
            glScalef(20,.1f,20); glutSolidCube(1); glPopMatrix();
        } else {
            glDisable(GL_LIGHTING); glColor3f(1,.7f,.15f); glPushMatrix();
            glTranslatef(px,3,pz); glutSolidSphere(.22,12,8); glPopMatrix();
        }
        // Keep the layout overview unfiltered. HUD and crosshair remain unaffected.
        if(!overview) postProcessing.Apply(width,height);
        glDisable(GL_FOG); glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION); glLoadIdentity(); glOrtho(0,width,height,0,-1,1);
        glMatrixMode(GL_MODELVIEW); glLoadIdentity();
        glColor3f(.7f,.86f,.87f); Text(22,28,"VESSEL / DECK 01     20 x 20 m");
        Text(22,50,"WASD Move | SHIFT Run | M Map | ESC Cursor | [ ] Preview HP");
        if(overview) {
            Text(22,80,"NORTH: Engineering / Control"); Text(22,100,"MIDDLE: Crew / Escape");
            Text(22,120,"SOUTH: Cargo / Maintenance | Yellow dot: player");
        } else {
            glBegin(GL_LINES); glVertex2i(width/2-5,height/2); glVertex2i(width/2+5,height/2);
            glVertex2i(width/2,height/2-5); glVertex2i(width/2,height/2+5); glEnd();
        }
        DrawHud();
        glEnable(GL_DEPTH_TEST); glutSwapBuffers();
    }
};
