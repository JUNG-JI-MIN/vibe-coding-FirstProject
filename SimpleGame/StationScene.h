#pragma once
#include "KoreanText.h"
#include <cmath>
#include <cstdio>
#include <string>
#include "StationMission.h"
#include "ToolWheel.h"
#include "PostProcessing.h"
#include "MetalMaterial.h"
#pragma comment(lib, "opengl32.lib")

class StationScene
{
    static constexpr float WalkSpeed = 3.6f;
    static constexpr float RunSpeed = 6.45f;
    static constexpr float MaxStamina = 150.f;

    Station::Layout layout;
    Station::Mission mission;
    ToolWheel tools;
    MetalMaterial material;
    PostProcessing post;
    int width = 1280, height = 720, lastTime = 0;
    Station::Point player = {16, 67}, mapCenter = {100, 100};
    float yaw = -70, pitch = 0, health = 100, stamina = MaxStamina, staminaAlpha = 0, heartbeat = 0,
          mapZoom = 1;
    bool keys[256] = {}, map = false, journal = false, help = false, captured = true, exhausted = false,
         showVents = true, drag = false;
    int dragX = 0, dragY = 0;
    float mapScale = 1;

    static void Rect(float x, float y, float w, float h, float r, float g, float b, float a = 1)
    {
        glColor4f(r, g, b, a);
        glBegin(GL_QUADS);
        glVertex2f(x, y);
        glVertex2f(x + w, y);
        glVertex2f(x + w, y + h);
        glVertex2f(x, y + h);
        glEnd();
    }

    static void Text(float x, float y, const char* text)
    {
        KoreanText::Draw(x, y, text);
    }

    static void Wrap(float x, float y, const std::string& text, int columns = 66)
    {
        // Decode first: a Korean character occupies multiple UTF-8 bytes.
        const std::wstring characters = KoreanText::Decode(text.c_str());
        float baseline = y;
        int used = 0;
        glRasterPos2f(x, baseline);
        for (wchar_t character : characters)
        {
            const auto& glyph = KoreanText::Get(character);
            if (character == L'\n' || (used > 0 && used + glyph.width > columns * 8))
            {
                baseline += 21;
                used = 0;
                glRasterPos2f(x, baseline);
            }
            if (character == L'\n' || (character == L' ' && used == 0))
            {
                continue;
            }
            if (glyph.list)
            {
                glCallList(glyph.list);
            }
            else
            {
                glBitmap(0, 0, 0, 0, float(glyph.width), 0, nullptr);
            }
            used += glyph.width;
        }
    }

    void Screen(float w, float h)
    {
        glUseProgram(0);
        glDisable(GL_LIGHTING);
        glDisable(GL_FOG);
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0, w, h, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    }

    void Cursor()
    {
        glutSetCursor(captured && !map && !journal && !help && !tools.IsOpen() ? GLUT_CURSOR_NONE
                                                                               : GLUT_CURSOR_INHERIT);
    }

    void Restart()
    {
        mission.Reset();
        tools = ToolWheel();
        player = {16, 67};
        yaw = -70;
        pitch = 0;
        health = 100;
        stamina = MaxStamina;
        exhausted = false;
        staminaAlpha = 0;
        heartbeat = 0;
        map = journal = help = false;
        for (auto& key : keys)
        {
            key = false;
        }
        Cursor();
        glutWarpPointer(width / 2, height / 2);
    }

    float Move(float dx, float dz)
    {
        Station::Point old = player;
        int steps = int(std::ceil(std::sqrt(dx * dx + dz * dz) / .07f));
        if (steps < 1)
        {
            return 0;
        }
        dx /= steps;
        dz /= steps;
        for (int i = 0; i < steps; ++i)
        {
            Station::Point next = {player.x + dx, player.z};
            if (layout.CanStand(next) && !mission.Blocks(layout, next))
            {
                player = next;
            }
            next = {player.x, player.z + dz};
            if (layout.CanStand(next) && !mission.Blocks(layout, next))
            {
                player = next;
            }
        }
        return Station::Distance(old, player);
    }

    void Sky()
    {
        glUseProgram(0);
        glDisable(GL_LIGHTING);
        glDisable(GL_FOG);
        glDisable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        double aspect = double(width) / height;
        glFrustum(-.07 * aspect, .07 * aspect, -.07, .07, .1, 2000);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(pitch, 1, 0, 0);
        glRotatef(yaw, 0, 1, 0);
        unsigned seed = 7331;
        auto random = [&]()
        {
            seed = seed * 1664525u + 1013904223u;
            return float(seed >> 8) / 16777216.f;
        };
        glPointSize(1.5f);
        glBegin(GL_POINTS);
        for (int i = 0; i < 1100; ++i)
        {
            float x = random() * 2 - 1, y = random() * 2 - 1, z = random() * 2 - 1,
                  n = std::sqrt(x * x + y * y + z * z) + .001f, c = .3f + random() * .6f;
            glColor3f(c * .8f, c * .9f, c);
            glVertex3f(x / n * 900, y / n * 900, z / n * 900);
        }
        glEnd();
        glPointSize(1);
        glColor3f(.045f, .14f, .2f);
        glPushMatrix();
        glTranslatef(430, -230, 650);
        glutSolidSphere(200, 32, 24);
        glPopMatrix();
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
    }

    void Hud()
    {
        Screen(1280, 720);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glColor4f(.65f, .8f, .83f, 1);
        Text(24, 28, u8"우주정거장 / 주 갑판");
        Text(24, 49, layout.Area(player));
        Rect(18, 67, 530, 63, .02f, .045f, .055f, .82f);
        glColor4f(.68f, .8f, .8f, 1);
        Text(30, 87, u8"현재 목표");
        glColor4f(.95f, .8f, .46f, 1);
        Text(30, 110, mission.Objective());
        glColor4f(.57f, .69f, .7f, 1);
        Text(24, 153, u8"M 지도 / V 환풍구 / J 기록 / H 도움말 / Tab 도구");
        Rect(1044, 204, 212, 164, .018f, .033f, .04f, .88f);
        glColor4f(.68f, .78f, .79f, 1);
        Text(1058, 227, u8"장착 도구 / Tab 길게");
        Text(1058, 249, ToolWheel::Name(tools.Equipped()));
        char line[128];
        sprintf_s(line, u8"카드: %s", mission.Card() ? u8"기술부" : u8"없음");
        Text(1058, 274, line);
        sprintf_s(line, u8"구급상자: %d개", tools.Medkits());
        Text(1058, 296, line);
        sprintf_s(line, u8"항법:%s 전력:%s 연료:%s", mission.Done(12) ? u8"유" : "-",
                  mission.Done(15) ? u8"유" : "-", mission.Done(17) ? u8"유" : "-");
        Text(1058, 320, line);
        Text(1058, 344, mission.Power() ? u8"정거장 전력: 정상" : u8"정거장 전력: 비상");
        if (tools.Scanning())
        {
            int target = mission.NearestActionable(player);
            sprintf_s(line, u8"탐지 / %s / %.1f m", layout.rooms[mission.tasks[target].room].code,
                      Station::Distance(player, mission.tasks[target].p));
            glColor4f(.48f, .89f, .68f, 1);
            Text(24, 180, line);
        }
        // Preserve the ECG design and its health-dependent rhythm.
        float danger = 1 - health / 100, r = .12f + .88f * danger, g = .7f - .58f * danger,
              b = 1 - .88f * danger;
        Rect(980, 557, 276, 137, .015f, .035f, .045f, .88f);
        Rect(980, 557, 3, 137, r, g, b);
        glColor4f(r, g, b, 1);
        sprintf_s(line, u8"심전도 / 체력 %03d", int(health));
        Text(996, 582, line);
        glColor4f(.18f, .38f, .43f, .25f);
        glBegin(GL_LINES);
        for (int i = 0; i <= 12; ++i)
        {
            glVertex2f(996 + i * 20.f, 592);
            glVertex2f(996 + i * 20.f, 659);
        }
        for (int i = 0; i < 5; ++i)
        {
            glVertex2f(996, 592 + i * 16.f);
            glVertex2f(1236, 592 + i * 16.f);
        }
        glEnd();
        glColor4f(r, g, b, 1);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= 240; ++i)
        {
            float phase = heartbeat - (240 - i) / 100.f;
            phase -= std::floor(phase);
            float pulse = 0;
            if (phase > .32f && phase < .40f)
            {
                pulse = (phase - .32f) / .08f;
            }
            else if (phase >= .40f && phase < .46f)
            {
                pulse = 1 - (phase - .40f) / .06f * 1.4f;
            }
            else if (phase >= .46f && phase < .52f)
            {
                pulse = -.4f + (phase - .46f) / .06f * .4f;
            }
            else if (phase > .67f && phase < .87f)
            {
                pulse = std::sin((phase - .67f) / .2f * Station::Pi) * .18f;
            }
            glVertex2f(996 + i, 633 - (health > 0 ? pulse * 27 : 0));
        }
        glEnd();
        Text(996, 679, health < 30 ? u8"위험" : u8"안정");
        if (staminaAlpha > 0)
        {
            glColor4f(1, 1, 1, staminaAlpha);
            Text(490, 649, exhausted ? u8"회복 중" : u8"스태미너");
            Rect(490, 661, 300, 5, 1, 1, 1, staminaAlpha * .15f);
            Rect(490, 661, 300 * stamina / MaxStamina, 5, 1, 1, 1, staminaAlpha * .9f);
        }
        std::string prompt = mission.Prompt();
        if (!prompt.empty())
        {
            Rect(342, 545, 596, 44, .018f, .03f, .04f, .85f);
            glColor4f(.93f, .8f, .55f, 1);
            Wrap(356, 565, prompt, 70);
            if (mission.Progress() > 0 && mission.Progress() < 1)
            {
                Rect(490, 598, 300 * mission.Progress(), 4, .45f, .78f, .72f);
            }
        }
        if (mission.Message()[0])
        {
            Rect(18, 420, 606, 90, .01f, .025f, .03f, .83f);
            glColor4f(.68f, .8f, .78f, 1);
            Wrap(30, 443, mission.Message(), 70);
        }
        if (!tools.IsOpen() && !journal && !help)
        {
            glColor4f(.65f, .79f, .77f, .8f);
            glBegin(GL_LINES);
            glVertex2f(635, 360);
            glVertex2f(645, 360);
            glVertex2f(640, 355);
            glVertex2f(640, 365);
            glEnd();
        }
        if (help || journal)
        {
            Rect(220, 130, 840, 470, .015f, .027f, .035f, .97f);
            glColor4f(.7f, .86f, .85f, 1);
            Text(246, 163, help ? u8"조작 안내 / H 닫기" : u8"정거장 기록 / J 닫기");
            if (help)
            {
                const char* lines[] = {
                    u8"WASD 이동 / Shift 달리기 / 마우스 시점 / Esc 마우스 해제",
                    u8"E 상호작용 / 안내에 따라 E 길게 누르기 / 작업 진행도 유지",
                    u8"Tab을 누른 채 마우스로 도구 선택 / Tab을 놓으면 장착",
                    u8"도구 메뉴 중앙: 맨손 / 마우스 왼쪽 버튼: 도구 사용",
                    u8"M 지도 / 휠 또는 +/- 확대·축소 / 드래그 이동 / C 중심 복귀",
                    u8"V 지도에 연결된 환풍구망 표시·숨기기",
                    u8"금색 목표를 따라가세요. 두 순환로와 여덟 연결 통로를 이용할 수 있습니다.",
                    u8"외측 격벽이 파손되어 있습니다. 내측 정비 순환로로 우회하세요.",
                    u8"현재 모드: 로컬 1인 프로토타입. 멀티플레이와 적 AI는 미구현입니다.",
                    u8"탈출 또는 체력 소진 후 R 재시작 / [ ] 체력 미리보기"};
                for (int i = 0; i < 10; ++i)
                {
                    Text(246, 201 + i * 32.f, lines[i]);
                }
            }
            else
            {
                float y = 204;
                for (const auto& entry : mission.log)
                {
                    Wrap(246, y, entry, 94);
                    y += 49;
                }
            }
        }
        if (mission.Won() || health <= 0)
        {
            Rect(0, 0, 1280, 720, .01f, .02f, .025f, .86f);
            glColor4f(.7f, .9f, .86f, 1);
            Text(472, 318, mission.Won() ? u8"탈출 성공" : u8"생체 신호 소실");
            Text(448, 363, u8"R 재시작 / M 정거장 지도 확인");
        }
        glDisable(GL_BLEND);
    }

    void DrawMap()
    {
        Screen(float(width), float(height));
        glClearColor(.014f, .026f, .04f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        float fit = (std::min)((width - 285.f) / 205, (height - 125.f) / 205);
        fit = (std::max)(.35f, fit);
        mapScale = fit * mapZoom;
        float ox = 250 + (width - 270) * .5f - mapCenter.x * mapScale,
              oy = (height + 30) * .5f - mapCenter.z * mapScale;
        glColor4f(.17f, .32f, .4f, .2f);
        glBegin(GL_LINES);
        for (int i = -20; i <= 40; ++i)
        {
            float x = ox + i * 10 * mapScale, z = oy + i * 10 * mapScale;
            glVertex2f(x, 64);
            glVertex2f(x, float(height - 40));
            glVertex2f(240, z);
            glVertex2f(float(width), z);
        }
        glEnd();
        glPushMatrix();
        glTranslatef(ox, oy, 0);
        glScalef(mapScale, mapScale, 1);
        glColor4f(.06f, .22f, .26f, .7f);
        layout.DrawFloorPlan();
        for (int pass = 0; pass < 2; ++pass)
        {
            glColor4f(.28f, .64f, .7f, pass == 0 ? .12f : .8f);
            glLineWidth(pass == 0 ? 4.f : 1.f);
            glBegin(GL_LINES);
            for (const auto& e : layout.contours)
            {
                glVertex2f(e.a.x, e.a.z);
                glVertex2f(e.b.x, e.b.z);
            }
            glEnd();
        }
        glLineWidth(1);
        if (showVents)
        {
            glEnable(GL_LINE_STIPPLE);
            glLineStipple(1, 0x0f0f);
            glBegin(GL_LINES);
            for (const auto& edge : layout.ventLinks)
            {
                bool blocked = edge.research && mission.DamperClosed();
                glColor4f(blocked ? .35f : .85f, blocked ? .4f : .16f, blocked ? .4f : .6f, .8f);
                auto a = layout.vents[edge.a].p, b = layout.vents[edge.b].p;
                glVertex2f(a.x, a.z);
                glVertex2f(b.x, b.z);
            }
            glEnd();
            glDisable(GL_LINE_STIPPLE);
            glColor4f(.9f, .2f, .65f, .9f);
            glBegin(GL_LINES);
            for (const auto& n : layout.vents)
            {
                if (n.grate)
                {
                    glVertex2f(n.p.x - .8f, n.p.z - .8f);
                    glVertex2f(n.p.x + .8f, n.p.z + .8f);
                    glVertex2f(n.p.x - .8f, n.p.z + .8f);
                    glVertex2f(n.p.x + .8f, n.p.z - .8f);
                }
            }
            glEnd();
        }
        for (int i = 0; i < int(layout.doors.size()); ++i)
        {
            const auto& d = layout.doors[i];
            float s = std::sin(d.angle), c = std::cos(d.angle);
            bool open = mission.DoorAmount(i) > .94f;
            glColor4f(open ? .18f : .95f, open ? .6f : .55f, .15f, 1);
            glLineWidth(3);
            glBegin(GL_LINES);
            glVertex2f(d.p.x - s * d.width * .5f, d.p.z + c * d.width * .5f);
            glVertex2f(d.p.x + s * d.width * .5f, d.p.z - c * d.width * .5f);
            glEnd();
        }
        glLineWidth(1);
        int objective = mission.ObjectiveIndex();
        PointMarker(mission.tasks[objective].p, 2, .95f, .64f, .2f);
        for (const auto& room : layout.rooms)
        {
            glColor4f(.77f, .85f, .87f, 1);
            Text(room.center.x - 3, room.center.z - 5, room.code);
        }
        glColor4f(.4f, .95f, .6f, 1);
        float t = 1 + std::sin(glutGet(GLUT_ELAPSED_TIME) * .002f) * .1f;
        for (int ring = 0; ring < 2; ++ring)
        {
            glColor4f(.4f, .95f, .6f, ring == 0 ? 1.f : .3f);
            glBegin(GL_LINE_LOOP);
            for (int i = 0; i < 48; ++i)
            {
                float a = i * 2 * Station::Pi / 48, r = ring == 0 ? 1.f : 4 * t;
                glVertex2f(player.x + std::cos(a) * r, player.z + std::sin(a) * r);
            }
            glEnd();
        }
        glColor4f(.4f, .95f, .6f, 1);
        glBegin(GL_LINES);
        glVertex2f(player.x, player.z);
        glVertex2f(player.x + std::sin(yaw * Station::Pi / 180) * 4,
                   player.z - std::cos(yaw * Station::Pi / 180) * 4);
        glEnd();
        glPopMatrix();
        Rect(0, 0, float(width), 64, .016f, .03f, .045f, .98f);
        Rect(0, 64, 242, float(height - 64), .016f, .03f, .045f, .97f);
        glColor4f(.67f, .81f, .86f, 1);
        Text(22, 28, u8"우주정거장 / 이중 고리형");
        Text(22, 49, u8"주 갑판 + 천장 환풍구망");
        float labelY = 104;
        for (const auto& room : layout.rooms)
        {
            std::string name = std::string(room.code) + " / " + room.name;
            glColor4f(.6f, .74f, .78f, 1);
            Wrap(16, labelY, name, 26);
            labelY += 43;
        }
        glColor4f(.8f, .62f, .26f, 1);
        Wrap(16, labelY + 10, std::string(u8"목표: ") + mission.Objective(), 26);
        glColor4f(.66f, .76f, .78f, 1);
        Text(260, float(height - 22), u8"M 닫기 / V 환풍구 / 휠 +/- 확대·축소 / 드래그 이동 / C 중심");
        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
        glutSwapBuffers();
    }

    static void PointMarker(Station::Point p, float size, float r, float g, float b)
    {
        glColor4f(r, g, b, 1);
        glBegin(GL_LINE_LOOP);
        glVertex2f(p.x, p.z - size);
        glVertex2f(p.x + size, p.z);
        glVertex2f(p.x, p.z + size);
        glVertex2f(p.x - size, p.z);
        glEnd();
    }

public:
    StationScene() : mission(layout)
    {
        layout.Finish();
        lastTime = glutGet(GLUT_ELAPSED_TIME);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_NORMALIZE);
        glEnable(GL_COLOR_MATERIAL);
        glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
        Cursor();
    }

    void ReleaseGraphics()
    {
        KoreanText::Release();
        layout.Release();
        material.Release();
        post.Release();
    }

    void Resize(int w, int h)
    {
        width = (std::max)(1, w);
        height = (std::max)(1, h);
        glViewport(0, 0, width, height);
    }

    void Key(unsigned char key, bool down)
    {
        if (key >= 'A' && key <= 'Z')
        {
            key += 32;
        }
        keys[key] = down;
        if (key == 9)
        {
            if (down && !map && !journal && !help && captured && !mission.Won())
            {
                if (!tools.IsOpen())
                {
                    tools.Begin(width, height);
                    Cursor();
                    glutWarpPointer(width / 2, height / 2);
                }
            }
            else if (!down && tools.IsOpen())
            {
                tools.End();
                Cursor();
                glutWarpPointer(width / 2, height / 2);
            }
            return;
        }
        if (tools.IsOpen())
        {
            if (down && key == 27)
            {
                tools.Cancel();
                Cursor();
                glutWarpPointer(width / 2, height / 2);
            }
            return;
        }
        if (!down)
        {
            return;
        }
        if (key == 'r' && (mission.Won() || health <= 0))
        {
            Restart();
            return;
        }
        if (key == 'm')
        {
            map = !map;
            journal = help = false;
            drag = false;
            tools.Button(false, false);
        }
        if (key == 'j')
        {
            journal = !journal;
            map = help = false;
            tools.Button(false, false);
        }
        if (key == 'h')
        {
            help = !help;
            map = journal = false;
            tools.Button(false, false);
        }
        if (key == 'v')
        {
            showVents = !showVents;
        }
        if (map)
        {
            if (key == '+' || key == '=')
            {
                mapZoom = Station::Clamp(mapZoom * 1.2f, .65f, 4);
            }
            if (key == '-')
            {
                mapZoom = Station::Clamp(mapZoom / 1.2f, .65f, 4);
            }
            if (key == 'c')
            {
                mapCenter = {100, 100};
                mapZoom = 1;
            }
        }
        if (key == '[')
        {
            health = Station::Clamp(health - 10, 0, 100);
        }
        if (key == ']')
        {
            health = Station::Clamp(health + 10, 0, 100);
        }
        if (key == 27)
        {
            if (map || journal || help)
            {
                map = journal = help = false;
            }
            else
            {
                captured = !captured;
            }
        }
        Cursor();
        if (captured && !map && !journal && !help)
        {
            glutWarpPointer(width / 2, height / 2);
        }
    }

    void Mouse(int x, int y)
    {
        if (tools.IsOpen())
        {
            tools.Mouse(x, y, width, height);
            return;
        }
        if (map)
        {
            if (drag)
            {
                mapCenter.x = Station::Clamp(mapCenter.x - (x - dragX) / mapScale, -20, 220);
                mapCenter.z = Station::Clamp(mapCenter.z - (y - dragY) / mapScale, -20, 220);
                dragX = x;
                dragY = y;
            }
            return;
        }
        if (!captured || journal || help || mission.Won())
        {
            return;
        }
        int dx = x - width / 2, dy = y - height / 2;
        if (!dx && !dy)
        {
            return;
        }
        yaw += dx * .12f;
        pitch = Station::Clamp(pitch + dy * .12f, -80, 80);
        glutWarpPointer(width / 2, height / 2);
    }

    void Button(int button, int state, int x, int y)
    {
        if (map)
        {
            if ((button == 3 || button == 4) && state == GLUT_DOWN)
            {
                mapZoom = Station::Clamp(mapZoom * (button == 3 ? 1.2f : 1 / 1.2f), .65f, 4);
            }
            if (button == GLUT_LEFT_BUTTON)
            {
                drag = state == GLUT_DOWN;
                dragX = x;
                dragY = y;
            }
            return;
        }
        if (button == GLUT_LEFT_BUTTON)
        {
            tools.Button(state == GLUT_DOWN, captured && !journal && !help && !mission.Won() && health > 0);
        }
    }

    void Update()
    {
        int now = glutGet(GLUT_ELAPSED_TIME);
        float dt = Station::Clamp((now - lastTime) / 1000.f, 0, .05f);
        lastTime = now;
        bool focused = GetForegroundWindow() == GetActiveWindow();
        if (!focused)
        {
            tools.Cancel();
            drag = false;
            for (auto& key : keys)
            {
                key = false;
            }
            Cursor();
        }
        bool active = focused && captured && !map && !journal && !help && !tools.IsOpen() && !mission.Won() &&
                      health > 0;
        tools.Update(dt, health, active);
        material.flashlight = tools.LightOn();
        heartbeat = std::fmod(heartbeat + dt * (1 + 2 * (1 - health / 100)), 1000.f);
        mission.Update(layout, dt, player, yaw, pitch, keys['e'] && active, tools.RepairHeld() && active,
                       active);
        float moved = 0;
        bool run = false;
        if (exhausted && stamina >= 20)
        {
            exhausted = false;
        }
        if (active && !mission.Won())
        {
            float forward = (keys['w'] ? 1.f : 0) - (keys['s'] ? 1.f : 0),
                  side = (keys['d'] ? 1.f : 0) - (keys['a'] ? 1.f : 0),
                  len = std::sqrt(forward * forward + side * side);
            if (len > 0)
            {
                run = (GetAsyncKeyState(VK_SHIFT) & 0x8000) && !exhausted && stamina > 0;
                float step = (run ? RunSpeed : WalkSpeed) * dt / len, a = yaw * Station::Pi / 180;
                moved = Move((std::sin(a) * forward + std::cos(a) * side) * step,
                             (-std::cos(a) * forward + std::sin(a) * side) * step);
            }
        }
        stamina = Station::Clamp(stamina + (run && moved > .00001f ? -22 : 12) * dt, 0, MaxStamina);
        if (stamina <= 0)
        {
            exhausted = true;
        }
        staminaAlpha = Station::Clamp(staminaAlpha + (stamina < MaxStamina ? 5 : -2) * dt, 0, 1);
    }

    void Draw()
    {
        if (map)
        {
            DrawMap();
            return;
        }
        bool hdr = material.Initialize() && post.Begin(width, height);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
        glClearColor(.006f, .012f, .018f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        Sky();
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        double aspect = double(width) / height;
        glFrustum(-.07 * aspect, .07 * aspect, -.07, .07, .1, 240);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glRotatef(pitch, 1, 0, 0);
        glRotatef(yaw, 0, 1, 0);
        glTranslatef(-player.x, -1.65f, -player.z);
        std::vector<Station::Lamp> lights = layout.lamps;
        std::sort(lights.begin(), lights.end(),
                  [&](const Station::Lamp& a, const Station::Lamp& b)
                  {
                      return Station::Distance(player, {a.x, a.z}) < Station::Distance(player, {b.x, b.z});
                  });
        glEnable(GL_LIGHTING);
        GLfloat ambient[] = {.16f, .20f, .24f, 1};
        glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
        for (int i = 0; i < 6 && i < int(lights.size()); ++i)
        {
            auto light = lights[i];
            float power = mission.Power() ? 1.f : .4f;
            GLfloat position[] = {light.x, light.y, light.z, 1},
                    color[] = {light.r * power, light.g * power, light.b * power, 1};
            glEnable(GL_LIGHT0 + i);
            glLightfv(GL_LIGHT0 + i, GL_POSITION, position);
            glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, color);
            glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, .08f);
        }
        layout.Draw(hdr, material);
        mission.Draw(hdr, material, layout);
        layout.DrawGlass();
        mission.DrawSteam();
        if (hdr)
        {
            post.Apply();
        }
        tools.DrawHeld(width, height);
        Hud();
        tools.DrawMenu(width, height);
        glEnable(GL_DEPTH_TEST);
        glutSwapBuffers();
    }
};
