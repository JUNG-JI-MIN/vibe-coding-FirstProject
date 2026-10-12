#include "stdafx.h"
#include "Simulation.h"

namespace Game
{
    void Simulation::Advance(int chapter)
    {
        auto& s = session.state;
        s.chapter = chapter;
        s.substep = 0;
        s.work = 0;
        s.stageTime = 0;
    }

    bool Simulation::Near(const PlayerState& p, Vec3 target, float range) const
    {
        return p.hidden < 0 && p.health > 0 && FlatDistance(p.position, target) < range &&
               world.Visible(p.position + Vec3{0, 1.65f, 0}, target + Vec3{0, 1.4f, 0});
    }

    bool Simulation::PlayerWorking(const Site& site) const
    {
        const auto& player = session.state.players[GameSession::PlayerSlot];
        return Near(player, site.position) && (player.buttons & Interact) &&
               (site.requiredTool == Hands || player.tool == site.requiredTool ||
                (site.id == 4 && player.tool == Drill));
    }

    void Simulation::Step(const Site& site, float dt, std::function<void()> complete)
    {
        auto& s = session.state;
        if (PlayerWorking(site))
        {
            auto& progress = s.taskProgress[site.id];
            bool drill = s.players[GameSession::PlayerSlot].tool == Drill;
            progress = Clamp(progress + dt / (std::max)(site.duration * (drill ? .65f : 1.f), .1f), 0, 1);
            s.work = progress;
            NoiseAt(site.position, drill ? 28 : 12);
            if (progress >= 1)
            {
                s.work = 0;
                complete();
            }
        }
    }

    void Simulation::Attack(int target)
    {
        auto& s = session.state;
        auto& p = s.players[target];
        if (p.health <= 0)
        {
            return;
        }
        if (s.stunCharges > 0 && !p.npc)
        {
            --s.stunCharges;
            alien->blackboard.stun = 4.5f;
            s.alienStun = 4.5f;
            if (p.hidden >= 0)
            {
                p.position = world.Nearest(CabinetFront(p.hidden));
            }
            p.hidden = -1;
            p.health = (std::max)(1.f, p.health - 15);
        }
        else
        {
            p.health = 0;
            p.hidden = -1;
        }
    }

    void Simulation::Unlock(const std::string& name)
    {
        for (auto& door : world.doors)
        {
            if (door.name == name)
            {
                door.locked = false;
            }
        }
    }

    Simulation::Simulation(GameSession& session, StationWorld& world) : session(session), world(world)
    {
        const Site definitions[] = {{0, u8"도킹 기록 확인", {0, 0, 69}, .8f},
                                    {1, u8"관제실 전력 확인", {-3, 0, -5}, .8f},
                                    {2, u8"창고의 쇠지렛대 회수", {-29, 0, 60}, .5f},
                                    {3, u8"지렛대를 걸고 출입문 밀기", {-75, 0, 23}, 5, Crowbar},
                                    {4, u8"비상전력 배선 변경", {-83, 0, 28}, 4, Wrench},
                                    {5, u8"발전기 공급 밸브 조절", {-83, 0, 13}, 3},
                                    {6, u8"구조 신호 기록 재생", {-3, 0, -5}, 1.2f},
                                    {7, u8"파손된 우주선 조사", {3, 0, 74}, 1.5f},
                                    {8, u8"카드 담당자 기록 확인", {-3, 0, -5}, 1},
                                    {9, u8"연구원과 대화", {51, 0, -23}, 1},
                                    {10, u8"에일리언 샘플 회수", {0, 0, -66}, 1},
                                    {11, u8"샘플 전달과 카드 교환", {51, 0, -23}, 1.5f},
                                    {12, u8"카드로 탈출정 해제", {64, 0, -64}, 1},
                                    {13, u8"조종석 매뉴얼 확인", {73, 0, -72}, 1},
                                    {14, u8"연료 공급", {69, 0, -73}, 4},
                                    {15, u8"외부 고정 장치 해제", {78, 0, -73}, 3},
                                    {16, u8"압력 52로 조절 · Q 증가 / Z 감소", {73, 0, -78}, 0},
                                    {17, u8"발진", {73, 0, -72}, 1},
                                    {18, u8"의료실 전압봉 회수", {58, 0, -23}, .6f}};
        for (auto site : definitions)
        {
            site.position = world.Nearest(site.position);
            sites.push_back(site);
        }
        const Vec3 owners[] = {{51, 0, -23}, {58, 0, 18}, {-83, 0, -18}};
        sites[9].position = sites[11].position = world.Nearest(owners[session.state.ownerRoom % 3]);
        const Vec3 locations[] = {{-82, 0, -32}, {-80, 0, 34}, {60, 0, -25}, {51, 0, 12}, {59, 0, 27},
                                  {7, 0, 7},     {16, 0, -78}, {-25, 0, 73}, {20, 0, 89}, {79, 0, -80}};
        for (Vec3 location : locations)
        {
            Vec3 entrance = world.Nearest(location);
            Vec3 center = entrance + Vec3{0, 0, -1};
            for (const auto& room : world.rooms)
            {
                if (entrance.x >= room.bounds.low.x && entrance.x <= room.bounds.high.x &&
                    entrance.z >= room.bounds.low.z && entrance.z <= room.bounds.high.z)
                {
                    center = room.center;
                }
            }
            Vec3 front = Normal(center - entrance);
            // Cardinal cabinet facing keeps its shell and collision box aligned.
            front = std::fabs(front.x) > std::fabs(front.z) ? Vec3{front.x > 0 ? 1.f : -1.f, 0, 0}
                                                            : Vec3{0, 0, front.z > 0 ? 1.f : -1.f};
            cabinets.push_back({entrance, front});
        }
        alien = std::make_unique<AI::Agent>(true);
        auto& b = alien->blackboard;
        b.world = &world;
        b.state = &session.state;
        b.position = world.Nearest({-68, 0, 10});
        b.attack = [this](int i)
        {
            Attack(i);
        };
        b.cabinetFront = [this](int i)
        {
            return cabinets.at(i).position + cabinets.at(i).front * 1.2f;
        };
        for (int i = 0; i < 2; ++i)
        {
            companions[i] = std::make_unique<AI::Agent>(false);
            auto& c = companions[i]->blackboard;
            c.world = &world;
            c.state = &session.state;
            c.helperIndex = i + 1;
        }
        researcher = std::make_unique<AI::Agent>(false, true);
        researcher->blackboard.world = &world;
        researcher->blackboard.state = &session.state;
        {
            auto& s = session.state;
            s.alienPosition = b.position;
            s.scientistPosition = sites[9].position;
            for (int i = 0; i < 3; ++i)
            {
                s.players[i].position = world.Nearest({float(i * 1.3f - 1.3f), 0, 66});
            }
            message = u8"구조 신호를 따라 도킹했습니다. 도킹 기록을 확인하세요.";
        }
    }

    const char* Simulation::AlienAction() const
    {
        return alien->blackboard.activeAction.c_str();
    }

    const char* Simulation::CompanionAction(int i) const
    {
        return companions.at(i)->blackboard.activeAction.c_str();
    }

    Vec3 Simulation::CabinetFront(int i) const
    {
        return cabinets.at(i).position + cabinets.at(i).front * 1.4f;
    }

    int Simulation::TargetSite() const
    {
        const auto& s = session.state;
        switch (s.chapter)
        {
        case 0:
            return 0;
        case 1:
            return 1;
        case 2:
            return s.crowbar ? 3 : 2;
        case 3:
            return s.wire ? 5 : 4;
        case 4:
            return 6;
        case 5:
            return s.substep == 0 ? -1 : 7;
        case 6:
            return 8;
        case 7:
            return 9;
        case 8:
            return s.sample ? 11 : 10;
        case 9:
            return 12;
        case 10:
            return !s.manual ? 13 : (!s.fuel ? 14 : (!s.clamp ? 15 : (!s.pressureDone ? 16 : 17)));
        default:
            return -1;
        }
    }

    const char* Simulation::Objective() const
    {
        int target = TargetSite();
        return target < 0 ? u8"캐비넷에 숨어 에일리언이 지나가길 기다리세요." : sites[target].label;
    }

    void Simulation::NoiseAt(Vec3 p, float strength)
    {
        // Merge sustained work into spatial events at a bounded frequency.
        if (!noises.empty() && noises.back().age < .35f && FlatDistance(p, noises.back().position) < 1)
        {
            return;
        }
        noises.push_back({p, strength, 0, ++noiseId});
    }

    int Simulation::CabinetFocus(const PlayerState& p) const
    {
        float best = 2.1f;
        int result = -1;
        Vec3 direction = {std::sin(p.yaw * Pi / 180), 0, -std::cos(p.yaw * Pi / 180)};
        for (int i = 0; i < int(cabinets.size()); ++i)
        {
            const auto& c = cabinets[i];
            Vec3 target = c.position + c.front * .66f;
            float distance = FlatDistance(p.position, target);
            if (Dot(p.position - target, c.front) < 0 || distance >= best ||
                Dot(Normal(target - p.position), direction) < .8f)
            {
                continue;
            }
            bool used = false;
            for (const auto& other : session.state.players)
            {
                if (other.hidden == i)
                {
                    used = true;
                }
            }
            if (!used && world.Visible(p.position + Vec3{0, 1.65f, 0}, target + Vec3{0, 1.5f, 0}))
            {
                result = i;
                best = distance;
            }
        }
        return result;
    }

    int Simulation::Focus(const PlayerState& p) const
    {
        int target = TargetSite();
        if (target >= 0 && Near(p, sites[target].position))
        {
            return target;
        }
        if (session.state.chapter == 3)
        {
            if (!session.state.wire && Near(p, sites[4].position))
            {
                return 4;
            }
            if (!session.state.valve && Near(p, sites[5].position))
            {
                return 5;
            }
        }
        if (session.state.chapter == 10 && session.state.manual)
        {
            for (int i = 14; i <= 16; ++i)
            {
                if (Near(p, sites[i].position))
                {
                    return i;
                }
            }
        }
        if (!session.state.batonTaken && Near(p, sites[18].position))
        {
            return 18;
        }
        return -1;
    }

    void Simulation::UpdatePlayer(float dt)
    {
        auto& s = session.state;
        auto& p = s.players[GameSession::PlayerSlot];
        p.scanTimer = (std::max)(0.f, p.scanTimer - dt);
        if (p.health <= 0)
        {
            return;
        }
        const auto& input = session.input;
        int requestedDoor = session.doorCommand;
        session.doorCommand = -1;
        if (requestedDoor >= 0 && requestedDoor < int(world.doors.size()) && s.power && s.chapter >= 7 &&
            Near(p, sites[8].position, 4))
        {
            int door = requestedDoor;
            int closed = 0;
            for (int j = 0; j < 40; ++j)
            {
                if (s.controlledDoors & (1ull << j))
                {
                    ++closed;
                }
            }
            if (world.doors[door].name.find("Door_Spoke") == 0 &&
                ((s.controlledDoors & (1ull << door)) || closed < 2))
            {
                s.controlledDoors ^= 1ull << door;
                world.doors[door].commandControl = (s.controlledDoors & (1ull << door)) != 0;
            }
        }
        p.previousButtons = p.buttons;
        p.buttons = input.buttons;
        p.yaw = input.yaw;
        p.pitch = input.pitch;
        if (OwnTool(s, input.tool))
        {
            p.tool = input.tool;
        }
        bool e = (p.buttons & Interact) && !(p.previousButtons & Interact),
             use = (p.buttons & Use) && !(p.previousButtons & Use);
        if (e)
        {
            if (p.hidden >= 0)
            {
                Vec3 exit = CabinetFront(p.hidden);
                if (world.Walkable(exit) && FlatDistance(exit, s.alienPosition) > 1.2f)
                {
                    p.hidden = -1;
                    p.position = exit;
                }
            }
            else
            {
                int cabinet = CabinetFocus(p);
                if (cabinet >= 0)
                {
                    p.hidden = cabinet;
                    p.position = cabinets[cabinet].position;
                    p.flashlight = 0;
                }
            }
        }
        if (p.hidden >= 0)
        {
            p.stamina = Clamp(p.stamina + dt * 12, 0, 150);
            return;
        }
        if (use && p.tool == Flashlight)
        {
            p.flashlight = !p.flashlight;
        }
        if (use && p.tool == Scanner)
        {
            p.scanTimer = 5;
            NoiseAt(p.position, 8);
        }
        if (use && p.tool == StunBaton && s.stunCharges > 0 &&
            FlatDistance(p.position, s.alienPosition) < 3 &&
            world.Visible(p.position + Vec3{0, 1.6f, 0}, s.alienPosition + Vec3{0, 1.6f, 0}))
        {
            --s.stunCharges;
            alien->blackboard.stun = 4.5f;
        }
        if (use && p.tool == Medkit && p.medkits > 0 && p.health < 100)
        {
            p.health = Clamp(p.health + 25, 0, 100);
            --p.medkits;
        }
        float f = ((p.buttons & Forward) ? 1.f : 0) - ((p.buttons & Backward) ? 1.f : 0),
              side = ((p.buttons & Right) ? 1.f : 0) - ((p.buttons & Left) ? 1.f : 0);
        bool run = (p.buttons & Run) && p.stamina > 1;
        float length = std::sqrt(f * f + side * side);
        if (length > 0)
        {
            float a = p.yaw * Pi / 180;
            Vec3 movement = {std::sin(a) * f + std::cos(a) * side, 0, -std::cos(a) * f + std::sin(a) * side};
            Vec3 before = p.position;
            world.Move(p.position, movement * ((run ? 6.45f : 3.6f) * dt / length));
            if (FlatDistance(before, p.position) > .001f)
            {
                p.stamina = Clamp(p.stamina + (run ? -22 : 12) * dt, 0, 150);
                if (footstep > .4f)
                {
                    NoiseAt(p.position, run ? 15 : 5);
                }
            }
            else
            {
                p.stamina = Clamp(p.stamina + 12 * dt, 0, 150);
            }
        }
        else
        {
            p.stamina = Clamp(p.stamina + 12 * dt, 0, 150);
        }
        if (e && Near(p, sites[18].position) && !s.batonTaken)
        {
            s.stunCharges = 1;
            s.batonTaken = 1;
            message = u8"전압봉을 확보했습니다. 붙잡혔을 때 한 번 자동으로 밀쳐냅니다.";
        }
    }

    void Simulation::Tick(float dt)
    {
        auto& s = session.state;
        if (s.outcome)
        {
            return;
        }
        s.elapsed += dt;
        s.stageTime += dt;
        for (auto& noise : noises)
        {
            noise.age += dt;
        }
        noises.erase(std::remove_if(noises.begin(), noises.end(),
                                    [](const Noise& n)
                                    {
                                        return n.age > 5;
                                    }),
                     noises.end());
        std::vector<Vec3> positions;
        for (const auto& p : s.players)
        {
            if (p.health > 0 && p.hidden < 0)
            {
                positions.push_back(p.position);
            }
        }
        if (s.alienActive)
        {
            positions.push_back(alien->blackboard.position);
        }
        world.UpdateDoors(dt, positions);
        footstep += dt;
        UpdatePlayer(dt);
        if (footstep > .4f)
        {
            footstep = 0;
        }

        s.helpersWorking = 0;
        for (int i = 0; i < 2; ++i)
        {
            int slot = i + 1;
            auto& p = s.players[slot];
            auto& b = companions[i]->blackboard;
            if (!p.npc || p.health <= 0)
            {
                continue;
            }
            b.position = p.position;
            b.threat = s.alienActive && FlatDistance(p.position, s.alienPosition) < 11 &&
                       world.Visible(p.position + Vec3{0, 1.6f, 0}, s.alienPosition + Vec3{0, 1.8f, 0});
            if (b.threat)
            {
                b.noisePosition = world.Nearest(p.position + Normal(p.position - s.alienPosition) * 8);
            }
            b.workRequested = false;
            b.leader = GameSession::PlayerSlot;
            b.lastSeen = world.Nearest(s.players[b.leader].position + Vec3{slot == 1 ? -2.f : 2.f, 0, 2});
            int task = TargetSite();
            if ((s.chapter == 2 && s.crowbar) || (s.chapter == 3))
            {
                const auto& leader = s.players[GameSession::PlayerSlot];
                bool leaderWorking = (s.chapter == 3 && s.wire && !s.valve) ||
                                     (leader.health > 0 && leader.hidden < 0 && (leader.buttons & Interact) &&
                                      FlatDistance(leader.position, sites[task].position) < 6);
                b.workRequested = leaderWorking;
                b.lastSeen = s.chapter == 2
                                 ? world.Nearest(sites[3].position + Vec3{0, 0, slot == 1 ? -1.f : 1.f})
                                 : sites[s.wire ? 5 : 4].position;
                if (s.chapter == 3 && PlayerWorking(sites[4]) > 0)
                {
                    b.lastSeen = sites[5].position;
                }
            }
            companions[i]->Tick(dt);
            p.position = b.position;
            p.yaw = std::atan2(b.facing.x, -b.facing.z) * 180 / Pi;
            if (b.working)
            {
                s.helpersWorking |= (1 << slot);
            }
        }

        if (s.chapter >= 7)
        {
            Vec3 leader = s.players[GameSession::PlayerSlot].position;
            auto& b = researcher->blackboard;
            b.position = s.scientistPosition;
            b.lastSeen = world.Nearest(leader + Vec3{0, 0, 2});
            researcher->Tick(dt);
            s.scientistPosition = b.position;
        }

        if (s.chapter == 0)
        {
            Step(sites[0], dt,
                 [&]
                 {
                     Advance(1);
                 });
        }
        else if (s.chapter == 1)
        {
            Step(sites[1], dt,
                 [&]
                 {
                     Advance(2);
                     message = u8"주 전력이 끊겼습니다. 창고의 쇠지렛대로 발전실 문을 열어야 합니다.";
                 });
        }
        else if (s.chapter == 2)
        {
            bool approaching = false;
            for (const auto& p : s.players)
            {
                if (!p.npc && p.health > 0 && FlatDistance(p.position, sites[3].position) < 22)
                {
                    approaching = true;
                }
            }
            if (s.crowbar && !s.shadowDone && approaching)
            {
                s.shadowDone = 1;
                s.alienActive = 2;
                alien->blackboard.scripted = true;
                alien->blackboard.scriptGoal = world.Nearest({-68, 0, -8});
                alien->blackboard.eventComplete = [this]
                {
                    session.state.alienActive = 0;
                };
            }
            if (!s.crowbar)
            {
                Step(sites[2], dt,
                     [&]
                     {
                         s.crowbar = 1;
                         message = u8"쇠지렛대 확보. 발전실 문에서 함께 힘을 가하세요.";
                     });
            }
            else
            {
                int count = PlayerWorking(sites[3]);
                for (int i = 1; i < 3; ++i)
                {
                    if (s.helpersWorking & (1 << i))
                    {
                        ++count;
                    }
                }
                if (count >= 3)
                {
                    Step(sites[3], dt,
                         [&]
                         {
                             Unlock("Door_Power");
                             Unlock("Door_LifePower");
                             Advance(3);
                         });
                }
            }
        }
        else if (s.chapter == 3)
        {
            if (!s.wire)
            {
                Step(sites[4], dt,
                     [&]
                     {
                         s.wire = 1;
                     });
            }
            if (!s.valve)
            {
                bool helper = false;
                for (int i = 1; i < 3; ++i)
                {
                    if ((s.helpersWorking & (1 << i)) &&
                        FlatDistance(s.players[i].position, sites[5].position) < 2)
                    {
                        helper = true;
                    }
                }
                if (helper && s.wire)
                {
                    s.taskProgress[5] = Clamp(s.taskProgress[5] + dt / 3, 0, 1);
                    if (s.taskProgress[5] >= 1)
                    {
                        s.valve = 1;
                    }
                }
                else
                {
                    Step(sites[5], dt,
                         [&]
                         {
                             s.valve = 1;
                         });
                }
            }
            if (s.wire && s.valve)
            {
                s.power = 1;
                Advance(4);
                message = u8"비상전력이 복구되었습니다. 관제실로 돌아가세요.";
            }
        }
        else if (s.chapter == 4)
        {
            Step(sites[6], dt,
                 [&]
                 {
                     Advance(5);
                     s.alienActive = 1;
                     alien->blackboard.scripted = true;
                     alien->blackboard.scriptGoal = world.Nearest({10, 0, 0});
                     alien->blackboard.eventComplete = [this]
                     {
                         session.state.hideVisited = 1;
                     };
                     NoiseAt(world.GetRoom("Control").center, 60);
                     message = u8"도킹 베이에서 파손 경보! 관제실 주변 캐비넷으로 숨으세요.";
                 });
        }
        else if (s.chapter == 5)
        {
            if (s.substep == 0)
            {
                bool hidden = false;
                for (const auto& p : s.players)
                {
                    if (!p.npc && p.health > 0 && p.hidden >= 0 &&
                        FlatDistance(s.alienPosition, p.position) > 7)
                    {
                        hidden = true;
                    }
                }
                if (FlatDistance(s.alienPosition, world.GetRoom("Control").center) < 15)
                {
                    s.hideVisited = 1;
                }
                if (hidden && s.hideVisited && s.stageTime > 8)
                {
                    s.substep = 1;
                }
            }
            else
            {
                Step(sites[7], dt,
                     [&]
                     {
                         Advance(6);
                         message =
                             u8"우주선은 수리할 수 없습니다. 관제실에서 탈출정 카드 담당자를 확인하세요.";
                     });
            }
        }
        else if (s.chapter == 6)
        {
            Step(sites[8], dt,
                 [&]
                 {
                     Advance(7);
                 });
        }
        else if (s.chapter == 7)
        {
            Step(sites[9], dt,
                 [&]
                 {
                     Advance(8);
                     message = u8"연구원: 연구실의 에일리언 샘플을 가져오면 카드 키를 주겠네.";
                 });
        }
        else if (s.chapter == 8)
        {
            if (!s.sample)
            {
                Step(sites[10], dt,
                     [&]
                     {
                         s.sample = 1;
                         NoiseAt(sites[10].position, 160);
                         alien->blackboard.scripted = true;
                         alien->blackboard.scriptGoal = sites[10].position;
                         alien->blackboard.eventComplete = [this]
                         {
                             alien->blackboard.lastSeen = sites[10].position;
                             alien->blackboard.memory = 7;
                         };
                         message = u8"샘플을 확보했습니다. 경보로 에일리언이 접근합니다!";
                     });
            }
            else
            {
                Step(sites[11], dt,
                     [&]
                     {
                         s.card = 1;
                         escortOrigin = s.scientistPosition;
                         Advance(9);
                     });
            }
        }
        else if (s.chapter == 9)
        {
            if (!s.scientistDead)
            {
                Vec3 leader = s.players[GameSession::PlayerSlot].position;
                if (FlatDistance(leader, escortOrigin) > 16)
                {
                    alien->blackboard.scripted = true;
                    alien->blackboard.scriptGoal = s.scientistPosition;
                    alien->blackboard.eventComplete = [this]
                    {
                        session.state.scientistDead = 1;
                        NoiseAt(session.state.scientistPosition, 160);
                        message = u8"연구원이 공격당했습니다! 탈출정으로 도주하세요.";
                    };
                }
            }
            if (s.scientistDead)
            {
                Step(sites[12], dt,
                     [&]
                     {
                         Unlock("Door_Escape");
                         Advance(10);
                         s.timer = s.difficulty == 2 ? 90.f : 120.f;
                         message = u8"제한 시간 안에 매뉴얼을 확인하고 연료·고정 장치·압력을 준비하세요.";
                     });
            }
        }
        else if (s.chapter == 10)
        {
            s.timer -= dt;
            if (s.timer <= 0)
            {
                s.outcome = 2;
                message = u8"탈출정 가동 시간이 초과되었습니다.";
            }
            if (!s.manual)
            {
                Step(sites[13], dt,
                     [&]
                     {
                         s.manual = 1;
                         message = u8"매뉴얼: 연료 공급 → 고정 장치 해제 → 압력 52±2. Q/Z로 압력 조절 후 "
                                   u8"E로 확정.";
                     });
            }
            else
            {
                if (!s.fuel)
                {
                    Step(sites[14], dt,
                         [&]
                         {
                             s.fuel = 1;
                         });
                }
                if (!s.clamp)
                {
                    Step(sites[15], dt,
                         [&]
                         {
                             s.clamp = 1;
                         });
                }
                for (const auto& p : s.players)
                {
                    if (!p.npc && Near(p, sites[16].position))
                    {
                        if (p.buttons & PressureUp)
                        {
                            s.pressure = Clamp(s.pressure + dt * 18, 0, 100);
                        }
                        if (p.buttons & PressureDown)
                        {
                            s.pressure = Clamp(s.pressure - dt * 18, 0, 100);
                        }
                        if ((p.buttons & Interact) && !(p.previousButtons & Interact) && s.pressure >= 50 &&
                            s.pressure <= 54)
                        {
                            s.pressureDone = 1;
                        }
                    }
                }
                if (s.fuel && s.clamp && s.pressureDone)
                {
                    Step(sites[17], dt,
                         [&]
                         {
                             s.outcome = 1;
                             Advance(11);
                         });
                }
            }
        }

        if (s.outcome == 1)
        {
            return;
        }
        if (s.alienActive)
        {
            if (s.alienActive == 1)
            {
                alien->Sense(noises, dt);
            }
            alien->Tick(dt);
            s.alienPosition = alien->blackboard.position;
            s.alienStun = alien->blackboard.stun;
            s.alienMode = alien->blackboard.visiblePlayer >= 0 ? 2 : (alien->blackboard.heard ? 1 : 0);
        }
        if (s.players[GameSession::PlayerSlot].health <= 0)
        {
            s.outcome = 2;
            message = u8"플레이어가 사망했습니다.";
        }
    }

} // namespace Game
