#include "stdafx.h"
#include "PlayScene.h"

namespace Game
{
    PlayerState& PlayScene::Local()
    {
        return session->state.players[GameSession::PlayerSlot];
    }

    void PlayScene::Cursor()
    {
        glutSetCursor(captured && !wheel && !map && !journal && !cctv && !session->state.outcome
                          ? GLUT_CURSOR_NONE
                          : GLUT_CURSOR_INHERIT);
    }

    void PlayScene::Restart()
    {
        session->Reset(session->state.difficulty, Local().color);
        framework.Replace(MakePlay(framework, session));
    }

    void PlayScene::DrawWheel()
    {
        if (!wheel)
        {
            return;
        }
        UI::Rect(0, 0, 1280, 720, .005f, .01f, .015f, .6f);
        float t = Clamp(wheelAnimation, 0, 1), bounce = 1 - std::exp(-8 * t) * std::cos(10 * t),
              outer = 205 * bounce, inner = 78 * bounce;
        for (int i = 0; i < ToolCount; ++i)
        {
            float a0 = -Pi * .5f + i * 2 * Pi / ToolCount, a1 = a0 + 2 * Pi / ToolCount;
            bool owned = OwnTool(session->state, i);
            float gray = i == hover ? .39f : (owned ? .17f : .08f);
            glColor4f(gray, gray + .015f, gray + .025f, .97f);
            glBegin(GL_QUAD_STRIP);
            for (int j = 0; j <= 20; ++j)
            {
                float a = a0 + .02f + (a1 - a0 - .04f) * j / 20;
                glVertex2f(640 + std::cos(a) * inner, 360 + std::sin(a) * inner);
                glVertex2f(640 + std::cos(a) * outer, 360 + std::sin(a) * outer);
            }
            glEnd();
            float a = (a0 + a1) * .5f;
            UI::Center(640 + std::cos(a) * 141 * bounce, 360 + std::sin(a) * 141 * bounce, ToolName(i));
            if (!owned)
            {
                UI::Center(640 + std::cos(a) * 174 * bounce, 360 + std::sin(a) * 174 * bounce, u8"미보유");
            }
        }
        UI::Center(640, 366, u8"맨손");
        UI::Center(640, 600, u8"Tab 놓기: 장착 / 중앙에서 놓기: 맨손");
    }

    void PlayScene::Hud()
    {
        UI::Screen();
        auto& s = session->state;
        auto& p = Local();
        if (p.hidden >= 0 && p.health > 0)
        {
            UI::Center(640, 679, u8"캐비넷 은신 중 · E 나오기");
            return;
        }
        if (cctv)
        {
            UI::Rect(0, 0, 1280, 85, .004f, .018f, .025f, .9f);
            UI::Text(28, 33, u8"관제실 CCTV");
            UI::Text(28, 63, world.rooms[cameraIndex].name);
            char line[128];
            int door = securityDoors[selectedGate];
            sprintf_s(line, u8"격벽 %02d · %s", selectedGate + 1,
                      s.controlledDoors & (1ull << door) ? u8"닫힘" : u8"개방");
            UI::Text(760, 40, line);
            UI::Rect(0, 665, 1280, 55);
            UI::Center(640, 695, u8"Q / Z 카메라 변경 · N 격벽 선택 · B 개폐 (최대 2개) · F 닫기");
            return;
        }
        if (map)
        {
            UI::Rect(0, 0, 1280, 720, .005f, .015f, .02f, .95f);
            int target = simulation.TargetSite();
            Vec3 goal = target >= 0 ? simulation.sites[target].position : p.position;
            UI::Radar(*stationMesh, world, p.position, goal, target >= 0, 720, 355, mapZoom, mapCenter);
            UI::Text(32, 44, u8"정거장 레이더");
            UI::Text(32, 80, world.Area(p.position));
            UI::Text(32, 127, simulation.chapters[s.chapter]);
            UI::Wrap(32, 157, simulation.Objective(), 370);
            char inventory[128];
            sprintf_s(inventory, u8"지렛대 %s / 샘플 %s / 카드 %s", s.crowbar ? u8"확보" : u8"없음",
                      s.sample ? u8"확보" : u8"없음", s.card ? u8"확보" : u8"없음");
            UI::Wrap(32, 260, inventory, 340);
            UI::Text(32, 681, u8"M 닫기 / WASD 이동 / 마우스 드래그 이동 / 휠 확대·축소 / C 중심");
            return;
        }
        if (journal)
        {
            UI::Rect(180, 90, 920, 530);
            UI::Text(210, 128, u8"구조팀 기록 · J 닫기");
            float y = 175;
            for (const auto& entry : log)
            {
                UI::Wrap(210, y, entry.c_str(), 850);
                y += 65;
                if (y > 580)
                {
                    break;
                }
            }
            return;
        }
        UI::Rect(18, 16, 570, objectiveFlash > 0 ? 110 : 68, .012f, .03f, .04f, .75f);
        UI::Text(30, 39, world.Area(p.position));
        UI::Text(30, 65, simulation.chapters[s.chapter]);
        UI::Wrap(30, objectiveFlash > 0 ? 93 : 87, simulation.Objective(), 550);
        UI::Text(26, 144, u8"M 지도 / J 기록 / Tab 도구 / Esc 설정 / 관제실 F CCTV");
        UI::Text(26, 174, ToolName(p.tool));
        char line[128];
        sprintf_s(line, u8"플레이어 1명 · 동료 NPC %d명", GameSession::CompanionCount);
        UI::Text(26, 198, line);
        if (p.scanTimer > 0 && simulation.TargetSite() >= 0)
        {
            int target = simulation.TargetSite();
            sprintf_s(line, u8"스캔 · 목표 거리 %.1fm",
                      FlatDistance(p.position, simulation.sites[target].position));
            UI::Text(26, 220, line, .35f, .87f, .66f);
        }
        if (s.chapter == 10)
        {
            sprintf_s(line, u8"남은 시간 %03d초 · 압력 %.1f / 목표 52", int(s.timer), s.pressure);
            UI::Center(640, 45, line);
        }
        if (p.health > 0 && !s.outcome)
        {
            int cabinet = simulation.CabinetFocus(p), focus = simulation.Focus(p);
            if (cabinet >= 0)
            {
                UI::Center(640, 523, u8"E 캐비넷에 숨기");
            }
            else if (focus >= 0)
            {
                const auto& site = simulation.sites[focus];
                std::string prompt =
                    site.requiredTool != Hands && p.tool != site.requiredTool &&
                            !(site.id == 4 && p.tool == Drill)
                        ? std::string(ToolName(site.requiredTool)) + u8"를 장착하세요"
                        : std::string(site.duration > .1f ? u8"E 길게 누르기 · " : u8"E · ") + site.label;
                UI::Center(640, 523, prompt.c_str());
                if (focus == 3)
                {
                    sprintf_s(line, u8"문 밀기: 플레이어 1 + NPC %d / 필요 3",
                              (s.helpersWorking & 2 ? 1 : 0) + (s.helpersWorking & 4 ? 1 : 0));
                    UI::Center(640, 548, line);
                }
            }
            glColor3f(.6f, .78f, .8f);
            glBegin(GL_LINES);
            glVertex2f(634, 360);
            glVertex2f(646, 360);
            glVertex2f(640, 354);
            glVertex2f(640, 366);
            glEnd();
            if (focus >= 0 && s.taskProgress[focus] > 0)
            {
                UI::Ring(640, 360, 28, s.taskProgress[focus]);
            }
        }
        float danger = 1 - p.health / 100, red = .12f + .88f * danger,
              green = .82f * (1 - danger) + .12f * danger;
        UI::Rect(985, 550, 275, 145, .012f, .028f, .03f);
        sprintf_s(line, u8"심전도 · 체력 %03d", int(p.health));
        UI::Text(1000, 576, line, red, green, .25f);
        glColor3f(red, green, .25f);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i <= 235; ++i)
        {
            float phase = std::fmod(clock * (1 + 2 * danger) - i * .013f + 1000, 1);
            float spike = phase > .4f && phase < .5f ? std::sin((phase - .4f) * 10 * Pi) * 25
                                                     : std::sin(phase * 2 * Pi) * 2;
            glVertex2f(1000 + i, 626 - (p.health > 0 ? spike : 0));
        }
        glEnd();
        UI::Text(1000, 677, p.health < 30 ? u8"위험" : u8"안정", red, green, .25f);
        if (p.stamina < 149.95f)
        {
            UI::Rect(490, 665, 300, 5, 1, 1, 1, .18f);
            UI::Rect(490, 665, 300 * p.stamina / 150, 5, 1, 1, 1, .85f);
        }
        if (debug)
        {
            UI::Text(26, 234, simulation.AlienAction());
            UI::Text(26, 256, simulation.CompanionAction(0));
            UI::Text(26, 278, simulation.CompanionAction(1));
            sprintf_s(line, u8"공유 메시 %zu개 / OBJ 그룹 %zu개", framework.resources.Count(),
                      stationMesh->groups.size());
            UI::Text(26, 300, line);
        }
        if (s.outcome)
        {
            UI::Rect(0, 0, 1280, 720, .005f, .01f, .015f, .9f);
            UI::Center(640, 303, s.outcome == 1 ? u8"우주정거장 탈출 성공" : u8"탈출 실패");
            UI::Center(640, 358, u8"R 다시 시작 / Esc 타이틀");
        }
        DrawWheel();
    }

    PlayScene::PlayScene(Framework& framework, std::shared_ptr<GameSession> session)
        : Scene(framework), session(std::move(session)), stationMesh(framework.resources.GetMesh("Station")),
          world(*stationMesh), simulation(*this->session, world)
    {
        auto station = std::make_unique<Object>(framework.resources, "Station");
        station->ownMaterial = false;
        objects.push_back(std::move(station));
        for (const auto& site : simulation.sites)
        {
            objects.push_back(
                std::make_unique<EquipmentObject>(framework.resources, site, this->session->state));
        }
        for (int i = 0; i < int(simulation.cabinets.size()); ++i)
        {
            const auto& spot = simulation.cabinets[i];
            objects.push_back(
                std::make_unique<CabinetObject>(framework.resources, spot, i, this->session->state));
            Bounds b;
            b.Include(spot.position - Vec3{.6f, 0, .6f});
            b.Include(spot.position + Vec3{.6f, 2.3f, .6f});
            world.AddObstacle(b);
        }
        world.Bake();
        for (const auto& door : world.doors)
        {
            objects.push_back(std::make_unique<DoorObject>(framework.resources, door));
        }
        for (int i = 0; i < int(world.doors.size()); ++i)
        {
            if (world.doors[i].name.find("Door_Spoke") == 0)
            {
                securityDoors.push_back(i);
            }
        }
        for (int i = 0; i < 3; ++i)
        {
            auto person = std::make_unique<Object>(framework.resources);
            person->scale = {.48f, 1.75f, .38f};
            people[i] = person.get();
            objects.push_back(std::move(person));
        }
        auto monster = std::make_unique<Object>(framework.resources);
        monster->scale = {.75f, 2.4f, .75f};
        monster->material = {.25f, .035f, .04f, .15f, .8f, 0};
        alien = monster.get();
        objects.push_back(std::move(monster));
        auto researcher = std::make_unique<Object>(framework.resources);
        researcher->scale = {.5f, 1.7f, .4f};
        researcher->material = {.52f, .38f, .63f, .1f, .8f, 0};
        scientist = researcher.get();
        objects.push_back(std::move(researcher));
    }

    PlayScene::~PlayScene()
    {
        material.Release();
        post.Release();
    }

    void PlayScene::Enter()
    {
        Cursor();
        glutWarpPointer(width / 2, height / 2);
    }

    void PlayScene::Pause()
    {
        session->input.buttons = 0;
        session->doorCommand = -1;
        for (auto& key : keys)
        {
            key = false;
        }
        glutSetCursor(GLUT_CURSOR_INHERIT);
    }

    void PlayScene::Resume()
    {
        Cursor();
        glutWarpPointer(width / 2, height / 2);
    }

    void PlayScene::Update(float dt)
    {
        clock += dt;
        transition = Clamp(transition + dt / 1.1f, 0, 1);
        objectiveFlash = (std::max)(0.f, objectiveFlash - dt);
        if (wheel)
        {
            wheelAnimation += dt * 4;
        }
        PlayerInput input;
        bool focused = GetForegroundWindow() == WindowFromDC(wglGetCurrentDC());
        if (!focused)
        {
            for (auto& key : keys)
            {
                key = false;
            }
            wheel = false;
        }
        input.tool = selected;
        input.yaw = yaw;
        input.pitch = pitch;
        if (focused && captured && !map && !journal && !wheel && !cctv && !session->state.outcome)
        {
            if (keys['w'])
            {
                input.buttons |= Forward;
            }
            if (keys['s'])
            {
                input.buttons |= Backward;
            }
            if (keys['a'])
            {
                input.buttons |= Left;
            }
            if (keys['d'])
            {
                input.buttons |= Right;
            }
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
            {
                input.buttons |= Run;
            }
            if (keys['e'])
            {
                input.buttons |= Interact;
            }
            if (keys['q'])
            {
                input.buttons |= PressureUp;
            }
            if (keys['z'])
            {
                input.buttons |= PressureDown;
            }
            if (keys[1])
            {
                input.buttons |= Use;
            }
        }
        session->input = input;
        simulation.Tick(dt);
        if (session->state.outcome)
        {
            map = journal = wheel = cctv = false;
        }
        auto& p = Local();
        if (p.hidden != lastHidden)
        {
            previousEye = cameraEye;
            transition = 0;
            lastHidden = p.hidden;
            if (p.hidden >= 0)
            {
                Vec3 front = simulation.cabinets[p.hidden].front;
                yaw = std::atan2(front.x, -front.z) * 180 / Pi;
                pitch = 0;
            }
        }
        if (map)
        {
            float step = dt * 50 / mapZoom;
            if (keys['a'])
            {
                mapCenter.x -= step;
            }
            if (keys['d'])
            {
                mapCenter.x += step;
            }
            if (keys['w'])
            {
                mapCenter.z -= step;
            }
            if (keys['s'])
            {
                mapCenter.z += step;
            }
        }
        int target = simulation.TargetSite();
        if (target != previousTarget)
        {
            objectiveFlash = 4;
            previousTarget = target;
            log.insert(log.begin(), simulation.Objective());
            if (log.size() > 6)
            {
                log.pop_back();
            }
        }
        if (simulation.message != lastMessage && !simulation.message.empty())
        {
            lastMessage = simulation.message;
            log.insert(log.begin(), lastMessage);
            if (log.size() > 6)
            {
                log.pop_back();
            }
        }
        for (int i = 0; i < 3; ++i)
        {
            const auto& person = session->state.players[i];
            people[i]->visible = i != GameSession::PlayerSlot && person.health > 0 && person.hidden < 0;
            people[i]->position = person.position + Vec3{0, .875f, 0};
            const Material colors[] = {{.23f, .58f, .66f, .15f, .8f, 0},
                                       {.66f, .48f, .19f, .15f, .8f, 0},
                                       {.47f, .63f, .3f, .15f, .8f, 0}};
            people[i]->material = colors[person.color % 3];
        }
        alien->visible = session->state.alienActive != 0;
        alien->position = session->state.alienPosition + Vec3{0, 1.2f, 0};
        scientist->visible = session->state.chapter >= 7;
        scientist->position =
            session->state.scientistPosition + Vec3{0, session->state.scientistDead ? .25f : .85f, 0};
        scientist->scale = session->state.scientistDead ? Vec3{1.7f, .4f, .5f} : Vec3{.5f, 1.7f, .4f};
        Scene::Update(dt);
        Cursor();
    }

    void PlayScene::Render()
    {
        auto& p = Local();
        bool hdr = material.Initialize() && post.Begin(width, height);
        post.exposure = framework.settings.exposure;
        glClearColor(.003f, .008f, .012f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        Vec3 desired = p.position + Vec3{0, 1.65f, 0};
        if (p.hidden >= 0)
        {
            desired = simulation.cabinets[p.hidden].position - simulation.cabinets[p.hidden].front * .12f +
                      Vec3{0, 1.6f, 0};
            if (framework.settings.cameraMotion)
            {
                desired.y += std::sin(clock * 1.35f) * .014f;
            }
        }
        float t = transition * transition * (3 - 2 * transition);
        cameraEye = previousEye + (desired - previousEye) * t;
        if (transition >= 1)
        {
            cameraEye = desired;
        }
        float viewYaw = yaw, viewPitch = pitch;
        if (cctv)
        {
            cameraEye = world.rooms[cameraIndex].center + Vec3{0, 2.7f, 0};
            Vec3 direction = Normal(Flat(cameraEye) * -1);
            viewYaw = cameraIndex == 0 ? clock * 10 : std::atan2(direction.x, -direction.z) * 180 / Pi;
            viewPitch = 20;
        }
        UI::Camera(width, height, cameraEye, viewYaw, viewPitch);
        UI::Lights(session->state.power != 0, clock);
        material.flashlight = p.flashlight && p.hidden < 0;
        // Opaque map/fixtures first, transparent door leaves last.
        for (auto& object : objects)
        {
            if (dynamic_cast<DoorObject*>(object.get()) == nullptr)
            {
                object->Draw(material, hdr);
            }
        }
        for (auto& object : objects)
        {
            if (dynamic_cast<DoorObject*>(object.get()) != nullptr)
            {
                object->Draw(material, hdr);
            }
        }
        glUseProgram(0);
        if (hdr)
        {
            post.Apply();
        }
        Hud();
    }

    void PlayScene::Key(unsigned char key, bool down)
    {
        if (key >= 'A' && key <= 'Z')
        {
            key += 32;
        }
        bool previous = keys[key];
        keys[key] = down;
        if (key == 9)
        {
            if (Local().hidden >= 0 || cctv)
            {
                return;
            }
            if (down && !previous)
            {
                wheel = true;
                hover = Hands;
                wheelAnimation = 0;
            }
            else if (!down && wheel)
            {
                selected = OwnTool(session->state, hover) ? hover : Hands;
                wheel = false;
            }
            Cursor();
            return;
        }
        if (!down || previous)
        {
            return;
        }
        if (key == 27)
        {
            if (session->state.outcome)
            {
                framework.Replace(MakeTitle(framework));
            }
            else
            {
                framework.Push(MakeSettings(framework));
            }
            return;
        }
        if (session->state.outcome && key == 'r')
        {
            Restart();
            return;
        }
        if (Local().hidden >= 0)
        {
            return;
        }
        if (key == 'm')
        {
            if (cctv)
            {
                return;
            }
            map = !map;
            journal = false;
            wheel = false;
        }
        if (key == 'j')
        {
            if (cctv)
            {
                return;
            }
            journal = !journal;
            map = false;
            wheel = false;
        }
        if (key == 'c' && map)
        {
            mapCenter = {};
        }
        if (key == 'v')
        {
            debug = !debug;
        }
        if (key == 'f' && session->state.power && session->state.chapter >= 7 &&
            FlatDistance(Local().position, simulation.sites[8].position) < 4)
        {
            cctv = !cctv;
            map = journal = wheel = false;
        }
        if (cctv)
        {
            if (key == 'q')
            {
                cameraIndex = (cameraIndex + 1) % int(world.rooms.size());
            }
            if (key == 'z')
            {
                cameraIndex = (cameraIndex + int(world.rooms.size()) - 1) % int(world.rooms.size());
            }
            if (key == 'n')
            {
                selectedGate = (selectedGate + 1) % int(securityDoors.size());
            }
            if (key == 'b')
            {
                session->RequestDoor(securityDoors[selectedGate]);
            }
        }
        Cursor();
    }

    void PlayScene::Mouse(int x, int y)
    {
        mouseX = int(x * 1280.f / width);
        mouseY = int(y * 720.f / height);
        if (wheel)
        {
            float dx = mouseX - 640.f, dy = mouseY - 360.f;
            if (dx * dx + dy * dy < 78 * 78)
            {
                hover = Hands;
                return;
            }
            float angle = std::atan2(dy, dx) + Pi * .5f;
            if (angle < 0)
            {
                angle += Pi * 2;
            }
            hover = int(angle / (2 * Pi / ToolCount)) % ToolCount;
            return;
        }
        if (map && keys[1])
        {
            mapCenter.x -= (x - dragX) * 1280.f / width / mapZoom;
            mapCenter.z -= (y - dragY) * 720.f / height / mapZoom;
            dragX = x;
            dragY = y;
            return;
        }
        if (!captured || map || journal || cctv || session->state.outcome)
        {
            return;
        }
        int dx = x - width / 2, dy = y - height / 2;
        if (!dx && !dy)
        {
            return;
        }
        yaw += dx * framework.settings.mouseSensitivity;
        pitch = Clamp(pitch + dy * framework.settings.mouseSensitivity, -80, 80);
        if (Local().hidden >= 0)
        {
            Vec3 front = simulation.cabinets[Local().hidden].front;
            float facing = std::atan2(front.x, -front.z) * 180 / Pi;
            yaw = Clamp(yaw, facing - 18, facing + 18);
            pitch = Clamp(pitch, -8, 8);
        }
        glutWarpPointer(width / 2, height / 2);
    }

    void PlayScene::Button(int button, int state, int x, int y)
    {
        if (button == GLUT_LEFT_BUTTON)
        {
            keys[1] = state == GLUT_DOWN;
            dragX = x;
            dragY = y;
        }
        if (map && state == GLUT_DOWN && (button == 3 || button == 4))
        {
            mapZoom = Clamp(mapZoom * (button == 3 ? 1.2f : .8333f), .8f, 5);
        }
    }

} // namespace Game
