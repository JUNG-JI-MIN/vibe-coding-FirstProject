#pragma once
#include <vector>
#include <cmath>
#include <algorithm>

// Ship interior: 100 x 100 metres, deck surfaces at -4, 0 and +4 metres.
class ShipLayout {
public:
    struct Box {
        float x,y,z,w,h,d,r,g,b;
        bool solid;
        float metallic,roughness,emission;
        int deck;
        bool slab;
    };
    struct Surface { float x0,x1,z0,z1,y0,slope; };
    struct Lamp { float x,y,z; };
    struct Room { float x,z; int deck; const char* name; };
    std::vector<Box> boxes;
    std::vector<Surface> surfaces;
    std::vector<Lamp> lamps;
    std::vector<Room> rooms;
    static float Clamp(float v,float a,float b) { return v<a?a:(v>b?b:v); }
    static float DeckY(int deck) { return (deck-1)*4.f; }
    static const char* DeckName(int deck) { return deck==0?"B1 / ENGINEERING":(deck==1?"1F / HABITATION":"2F / OPERATIONS"); }
private:
    int buildingDeck=0;
    float base=0;
    void Add(float x,float y,float z,float w,float h,float d,float r,float g,float b,
        bool solid=true,float metal=.75f,float rough=.45f,float emission=0,bool slab=false) {
        boxes.push_back({x,y+base,z,w,h,d,r,g,b,solid,metal,rough,emission,buildingDeck,slab});
    }
    void Wall(float x,float z,float w,float d) {
        Add(x,1.85f,z,w,3.7f,d,.28f,.33f,.37f);
        Add(x,.18f,z,w+.02f,.12f,d+.02f,.1f,.16f,.17f,false);
    }
    // Split a bulkhead into solid spans and 2.4 m wide, 2.8 m high doorways.
    void Bulkhead(bool horizontal,float fixed,float lo,float hi,std::vector<float> doors) {
        std::sort(doors.begin(),doors.end());
        float begin=lo;
        for(float door:doors) {
            float end=door-1.2f;
            if(end>begin) {
                if(horizontal) Wall((begin+end)*.5f,fixed,end-begin,.3f);
                else Wall(fixed,(begin+end)*.5f,.3f,end-begin);
            }
            if(horizontal) {
                Add(door,3.25f,fixed,2.4f,.9f,.3f,.25f,.3f,.33f);
                Add(door,2.85f,fixed,1.8f,.07f,.34f,.1f,.6f,.55f,false,.1f,.5f,3);
            } else {
                Add(fixed,3.25f,door,.3f,.9f,2.4f,.25f,.3f,.33f);
                Add(fixed,2.85f,door,.34f,.07f,1.8f,.1f,.6f,.55f,false,.1f,.5f,3);
            }
            begin=door+1.2f;
        }
        if(hi>begin) {
            if(horizontal) Wall((begin+hi)*.5f,fixed,hi-begin,.3f);
            else Wall(fixed,(begin+hi)*.5f,.3f,hi-begin);
        }
    }
    void SurfaceRect(float x0,float x1,float z0,float z1,float y,float slope=0) {
        surfaces.push_back({x0,x1,z0,z1,y,slope});
    }
    void Slabs(int deck) {
        buildingDeck=deck; base=DeckY(deck);
        // Partition around both stair shafts; no deck slab intersects the flights.
        const float xs[]={0,20,28,72,80,100};
        const float zs[]={0,37,44,56,63,100};
        for(int i=0;i<5;++i) for(int j=0;j<5;++j) {
            float x0=xs[i],x1=xs[i+1],z0=zs[j],z1=zs[j+1];
            bool shaft=(x0==20&&z0==37)||(x0==72&&z0==56);
            if(shaft&&deck<3) continue;
            Add((x0+x1)*.5f,-.15f,(z0+z1)*.5f,x1-x0,.3f,z1-z0,
                .24f,.28f,.31f,false,.8f,.58f,0,true);
            if(deck<3) SurfaceRect(x0,x1,z0,z1,base);
        }
    }
    void Locker(float x,float z) {
        Add(x,1.1f,z,.9f,2.2f,.65f,.19f,.27f,.28f,true,.4f,.52f);
        Add(x,1.1f,z+.34f,.76f,2.05f,.04f,.13f,.2f,.21f,false,.3f,.6f);
        Add(x+.25f,1.1f,z+.39f,.06f,.3f,.05f,.55f,.62f,.65f,false,.9f,.2f);
    }
    void Furnish(float x,float z,int index) {
        // Keep furniture in the northern half, clear of central cross routes and stair cores.
        if(buildingDeck==0) {
            Add(x,1.2f,z,4,2.4f,2,.29f,.32f,.34f);
            Add(x,1.35f,z+1.02f,2.8f,.3f,.04f,.7f,.22f,.06f,false,0,.5f,4);
            for(int i=-1;i<=1;++i) Add(x+i*1.3f,3.25f,z,.12f,.12f,8,.36f,.4f,.42f,false);
        } else if(buildingDeck==1) {
            if(index%2==0) {
                Add(x,.35f,z,2.2f,.7f,1,.25f,.3f,.32f);
                Add(x,.77f,z,2.05f,.14f,.9f,.35f,.39f,.4f,false,0,.9f);
                Locker(x+3,z);
            } else {
                Add(x,.7f,z,3,1.4f,1.2f,.25f,.29f,.3f);
                Add(x+3,.5f,z,1,1,1,.29f,.26f,.2f,true,.1f,.75f);
            }
        } else {
            Add(x,.6f,z,3.4f,1.2f,1.1f,.18f,.25f,.28f);
            for(int i=-1;i<=1;++i) {
                Add(x+i,1.5f,z,.85f,.65f,.12f,.1f,.15f,.17f);
                Add(x+i,1.5f,z+.08f,.72f,.5f,.03f,.08f,.65f,.6f,false,0,.4f,4);
            }
        }
    }
    void RoomBlock(float x0,float z0,int block) {
        float x1=x0+35,z1=z0+35,xc=x0+17.5f,zc=z0+17.5f;
        std::vector<float> southDoors={x0+8,x0+27},northDoors=southDoors;
        // Stair vestibules have their own corridor entrance and enclosed side walls.
        if(block==0) southDoors={24,39};
        if(block==3) northDoors={61,76,84};
        Bulkhead(true,z0,x0,x1,northDoors);
        Bulkhead(true,z1,x0,x1,southDoors);
        Bulkhead(false,x0,z0,z1,{z0+8,z0+27});
        Bulkhead(false,x1,z0,z1,{z0+8,z0+27});
        Bulkhead(true,zc,x0,x1,{x0+8,x0+27});
        Bulkhead(false,xc,z0,z1,{z0+8,z0+27});
        // Aligned deck-to-deck columns and deep overhead beams on bulkhead lines.
        for(float x : {x0,xc,x1}) for(float z : {z0,zc,z1})
            Add(x,1.85f,z,.55f,3.7f,.55f,.22f,.27f,.3f);
        Add(xc,3.35f,zc,35,.7f,.55f,.21f,.27f,.3f);
        Add(xc,3.35f,zc,.55f,.7f,35,.21f,.27f,.3f);
        const char* names[3][4]={
            {"POWER / PUMPS","LIFE SUPPORT","CARGO / WORKSHOP","COOLING / AUX"},
            {"CREW / MEDICAL","MESS / GALLEY","STORES / SECURITY","CREW / ASSEMBLY"},
            {"NAV / COMMS","SCIENCE / CONTROL","DATA / BRIEFING","ESCAPE / RESCUE"}
        };
        rooms.push_back({xc,zc,buildingDeck,names[buildingDeck][block]});
        for(int ix=0;ix<2;++ix) for(int iz=0;iz<2;++iz) {
            float x=x0+5+ix*17.5f,z=z0+4+iz*17.5f;
            // Leave the room containing the east stair core clear of furniture.
            if(block==3&&ix==1&&iz==0) continue;
            Furnish(x,z,ix+iz*2);
            lamps.push_back({x0+8.75f+ix*17.5f,base+3.35f,z0+8.75f+iz*17.5f});
            Add(x0+8.75f+ix*17.5f,3.42f,z0+8.75f+iz*17.5f,1.8f,.08f,.25f,
                .3f,.72f,.8f,false,.1f,.5f,4);
        }
    }
    void Core(float x,float z,bool mirror) {
        auto Z=[=](float t){return mirror?z+10-t:z+t;};
        for(int deck=0;deck<3;++deck) {
            buildingDeck=deck; base=DeckY(deck);
            Wall(x,Z(5),.3f,10); Wall(x+8,Z(5),.3f,10);
            Wall(x+4,Z(0),8,.3f);
            Wall(x+4,Z(5.2f),.8f,3.6f); // Full-height central spine, between flights.
            lamps.push_back({x+4,base+3.4f,Z(8.5f)});
            Add(x+4,2.85f,Z(9.8f),2,.1f,.12f,.15f,.65f,.9f,false,.1f,.5f,4);
            if(deck==0) Add(x+5.9f,.6f,Z(7),3,1.2f,.12f,.35f,.4f,.42f);
            if(deck==2) Add(x+2.1f,.6f,Z(7),3,1.2f,.12f,.35f,.4f,.42f);
        }
        for(int deck=0;deck<2;++deck) {
            buildingDeck=deck; base=DeckY(deck);
            // 24 risers per storey: 0.1667 m rise, 0.30 m tread, two 12-step flights.
            for(int i=0;i<12;++i) {
                float t=7-(i+.5f)*.3f;
                float top=(i+1)/6.f;
                Add(x+2.1f,top-.10f,Z(t),3,.20f,.3f,.34f,.39f,.42f,false,.85f,.4f);
                t=3.4f+(i+.5f)*.3f; top=2+(i+1)/6.f;
                Add(x+5.9f,top-.10f,Z(t),3,.20f,.3f,.34f,.39f,.42f,false,.85f,.4f);
            }
            Add(x+4,1.9f,Z(1.7f),7.7f,.2f,3.4f,.28f,.33f,.36f,false);
            float za=Z(3.4f),zb=Z(7),zm0=Z(.15f),zm1=Z(3.4f);
            if(!mirror) {
                SurfaceRect(x+.6f,x+3.6f,za,zb,base+2,-2/3.6f);
                SurfaceRect(x+4.4f,x+7.4f,za,zb,base+2,2/3.6f);
                SurfaceRect(x+.15f,x+7.85f,zm0,zm1,base+2);
            } else {
                SurfaceRect(x+.6f,x+3.6f,zb,za,base,2/3.6f);
                SurfaceRect(x+4.4f,x+7.4f,zb,za,base+4,-2/3.6f);
                SurfaceRect(x+.15f,x+7.85f,zm1,zm0,base+2);
            }
            // Handrails follow each slope; short segments preserve the stair silhouette.
            for(int i=0;i<12;++i) {
                float t=7-(i+.5f)*.3f, y=(i+.5f)/6.f+1;
                Add(x+.42f,y,Z(t),.07f,.07f,.34f,.5f,.55f,.58f,false,.9f,.25f);
                t=3.4f+(i+.5f)*.3f; y=2+(i+.5f)/6.f+1;
                Add(x+7.58f,y,Z(t),.07f,.07f,.34f,.5f,.55f,.58f,false,.9f,.25f);
            }
        }
    }
public:
    ShipLayout() {
        for(int deck=0;deck<4;++deck) Slabs(deck);
        for(int deck=0;deck<3;++deck) {
            buildingDeck=deck; base=DeckY(deck);
            Wall(50,.2f,100,.4f); Wall(50,99.8f,100,.4f);
            Wall(.2f,50,.4f,100); Wall(99.8f,50,.4f,100);
            // Sealed outer service belt, with a continuous six-metre corridor inside it.
            Wall(50,6,88,.3f); Wall(50,94,88,.3f);
            Wall(6,50,.3f,88); Wall(94,50,.3f,88);
            RoomBlock(12,12,0); RoomBlock(53,12,1);
            RoomBlock(12,53,2); RoomBlock(53,53,3);
            for(float axis : {9.f,50.f,91.f}) for(float t : {9.f,25.f,41.f,59.f,75.f,91.f}) {
                lamps.push_back({axis,base+3.35f,t});
                if(t!=axis) lamps.push_back({t,base+3.35f,axis});
                Add(axis,3.42f,t,1.8f,.08f,.25f,.3f,.72f,.8f,false,.1f,.5f,4);
                Add(t,3.42f,axis,.25f,.08f,1.8f,.3f,.72f,.8f,false,.1f,.5f,4);
                Add(axis-2.5f,.015f,t,.08f,.025f,2,.08f,.4f,.42f,false,.1f,.6f,1.5f);
                Add(t,.015f,axis-2.5f,2,.025f,.08f,.08f,.4f,.42f,false,.1f,.6f,1.5f);
            }
        }
        Core(20,37,false); Core(72,53,true);
        // Carry forward the key and final escape placeholders without blocking loop routes.
        buildingDeck=1; base=DeckY(1);
        Add(17,.89f,57,.3f,.08f,.12f,.9f,.65f,.14f,false,.8f,.3f);
        buildingDeck=2; base=DeckY(2);
        Add(93.8f,1.4f,80,.12f,2.8f,2.4f,.3f,.4f,.42f);
        Add(93.7f,2.95f,80,.08f,.12f,2.2f,.1f,.8f,.4f,false,.1f,.5f,4);
    }
    bool Support(float x,float z,float currentY,float& result) const {
        float best=1000; bool found=false;
        for(const auto& s:surfaces) {
            if(x<s.x0||x>s.x1||z<s.z0||z>s.z1) continue;
            float y=s.y0+(z-s.z0)*s.slope;
            float difference=std::fabs(y-currentY);
            if(difference<=.24f&&difference<best) { best=difference; result=y; found=true; }
        }
        return found;
    }
    bool CanStand(float x,float y,float z) const {
        const float radius=.26f;
        if(x<6.15f+radius||x>93.85f-radius||z<6.15f+radius||z>93.85f-radius) return false;
        for(const auto& b:boxes) {
            if(!b.solid||b.y-b.h*.5f>=y+1.8f||b.y+b.h*.5f<=y+.05f) continue;
            float nx=Clamp(x,b.x-b.w*.5f,b.x+b.w*.5f),nz=Clamp(z,b.z-b.d*.5f,b.z+b.d*.5f);
            if((x-nx)*(x-nx)+(z-nz)*(z-nz)<radius*radius) return false;
        }
        // Overhead slabs and treads are non-solid supports but still limit headroom.
        for(const auto& b:boxes) {
            if(b.solid||b.h<.19f||b.w<2||b.d<.29f) continue;
            float bottom=b.y-b.h*.5f,top=b.y+b.h*.5f;
            if(bottom>y+.25f&&bottom<y+1.8f&&top>y+.25f&&
                x>b.x-b.w*.5f-radius&&x<b.x+b.w*.5f+radius&&
                z>b.z-b.d*.5f-radius&&z<b.z+b.d*.5f+radius) return false;
        }
        return true;
    }
};
