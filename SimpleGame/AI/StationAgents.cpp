#include "stdafx.h"
#include "StationAgents.h"

namespace AI
{
    Status MoveTo(Blackboard& b, Vec3 goal, float speed, float dt)
    {
        if (FlatDistance(b.position, goal) < .7f)
        {
            return Status::Success;
        }
        b.repath -= dt;
        if (b.repath <= 0 || FlatDistance(goal, b.goal) > 2)
        {
            b.goal = goal;
            b.path = b.world->Path(b.position, goal);
            b.waypoint = 0;
            b.repath = 1;
        }
        if (b.path.empty())
        {
            return Status::Failure;
        }
        while (b.waypoint < b.path.size() && FlatDistance(b.position, b.path[b.waypoint]) < .3f)
        {
            ++b.waypoint;
        }
        if (b.waypoint >= b.path.size())
        {
            return Status::Success;
        }
        Vec3 delta = b.path[b.waypoint] - b.position;
        float length = Length(delta);
        b.facing = Normal(Flat(delta));
        Vec3 before = b.position;
        b.world->Move(b.position, Normal(delta) * (std::min)(length, speed * dt), .32f);
        if (FlatDistance(before, b.position) < .001f)
        {
            b.repath = (std::min)(b.repath, .15f);
        }
        return Status::Running;
    }

    NodePtr Check(const char* name, std::function<bool(const Blackboard&)> condition)
    {
        return std::make_unique<Condition>(name, std::move(condition));
    }

    NodePtr Do(const char* name, std::function<Status(Blackboard&, float)> action,
               std::function<void(Blackboard&)> abort)
    {
        return std::make_unique<Action>(name, std::move(action), std::move(abort));
    }

    NodePtr AlienTree()
    {
        auto root = std::make_unique<Selector>(u8"에일리언 우선순위");
        auto stunned = std::make_unique<Sequence>(u8"전압봉 경직");
        stunned->Add(Check("Stunned",
                           [](const Blackboard& b)
                           {
                               return b.stun > 0;
                           }));
        stunned->Add(Do(u8"경직",
                        [](Blackboard&, float)
                        {
                            return Status::Running;
                        }));
        root->Add(std::move(stunned));
        auto attack = std::make_unique<Sequence>(u8"공격");
        attack->Add(Check("CanAttack",
                          [](const Blackboard& b)
                          {
                              return b.visiblePlayer >= 0 && b.attackCooldown <= 0 &&
                                     FlatDistance(b.position, b.state->players[b.visiblePlayer].position) <
                                         1.15f;
                          }));
        attack->Add(Do(u8"공격",
                       [](Blackboard& b, float)
                       {
                           b.attack(b.visiblePlayer);
                           b.attackCooldown = 1.6f;
                           return Status::Success;
                       }));
        root->Add(std::move(attack));
        auto scripted = std::make_unique<Sequence>(u8"지정 이벤트");
        scripted->Add(Check("ScriptedEvent",
                            [](const Blackboard& b)
                            {
                                return b.scripted;
                            }));
        scripted->Add(Do(u8"이벤트 위치로 이동",
                         [](Blackboard& b, float dt)
                         {
                             return MoveTo(b, b.scriptGoal, 5.4f, dt);
                         }));
        scripted->Add(Do(u8"이벤트 수행",
                         [](Blackboard& b, float)
                         {
                             b.scripted = false;
                             if (b.eventComplete)
                             {
                                 b.eventComplete();
                             }
                             return Status::Success;
                         }));
        root->Add(std::move(scripted));
        auto chase = std::make_unique<Sequence>(u8"시야 추격");
        chase->Add(Check("VisibleTarget",
                         [](const Blackboard& b)
                         {
                             return b.visiblePlayer >= 0;
                         }));
        chase->Add(Do(u8"추격",
                      [](Blackboard& b, float dt)
                      {
                          return MoveTo(b, b.lastSeen, 5.4f, dt);
                      }));
        root->Add(std::move(chase));
        auto cabinet = std::make_unique<Sequence>(u8"목격한 은신처 확인");
        cabinet->Add(Check("WitnessedEntry",
                           [](const Blackboard& b)
                           {
                               return b.witnessedCabinet >= 0;
                           }));
        cabinet->Add(Do(u8"캐비넷 접근",
                        [](Blackboard& b, float dt)
                        {
                            return MoveTo(b, b.cabinetFront(b.witnessedCabinet), 3.5f, dt);
                        }));
        cabinet->Add(Do(u8"캐비넷 확인",
                        [](Blackboard& b, float dt)
                        {
                            b.search += dt;
                            if (b.search < 2)
                            {
                                return Status::Running;
                            }
                            for (int i = 0; i < 3; ++i)
                            {
                                if (b.state->players[i].hidden == b.witnessedCabinet &&
                                    b.state->players[i].health > 0)
                                {
                                    b.attack(i);
                                }
                            }
                            b.witnessedCabinet = -1;
                            b.search = 0;
                            return Status::Success;
                        }));
        root->Add(std::move(cabinet));
        auto memory = std::make_unique<Sequence>(u8"마지막 목격 위치 수색");
        memory->Add(Check("RecentSight",
                          [](const Blackboard& b)
                          {
                              return b.memory > 0;
                          }));
        memory->Add(Do(u8"마지막 위치 접근",
                       [](Blackboard& b, float dt)
                       {
                           return MoveTo(b, b.lastSeen, 4.1f, dt);
                       }));
        memory->Add(Do(u8"주변 수색",
                       [](Blackboard& b, float dt)
                       {
                           b.search += dt;
                           if (b.search < 3)
                           {
                               return Status::Running;
                           }
                           b.memory = 0;
                           b.search = 0;
                           return Status::Success;
                       }));
        root->Add(std::move(memory));
        auto sound = std::make_unique<Sequence>(u8"소음 조사");
        sound->Add(Check("NewNoise",
                         [](const Blackboard& b)
                         {
                             return b.heard;
                         }));
        sound->Add(Do(u8"소음 위치 조사",
                      [](Blackboard& b, float dt)
                      {
                          Status result = MoveTo(b, b.noisePosition, 3.8f, dt);
                          if (result != Status::Running)
                          {
                              b.heard = false;
                          }
                          return result;
                      }));
        root->Add(std::move(sound));
        root->Add(Do(u8"순찰",
                     [](Blackboard& b, float dt)
                     {
                         Status result = MoveTo(b, b.world->patrol[b.patrolIndex], 2.3f, dt);
                         if (result != Status::Running)
                         {
                             b.patrolIndex = (b.patrolIndex + 1) % int(b.world->patrol.size());
                         }
                         return Status::Running;
                     }));
        return root;
    }

    NodePtr CompanionTree()
    {
        auto root = std::make_unique<Selector>(u8"동료 NPC 우선순위");
        auto danger = std::make_unique<Sequence>(u8"위험 회피");
        danger->Add(Check("AlienThreat",
                          [](const Blackboard& b)
                          {
                              return b.threat;
                          }));
        danger->Add(Do(u8"위험에서 이탈",
                       [](Blackboard& b, float dt)
                       {
                           b.working = false;
                           MoveTo(b, b.noisePosition, 4.5f, dt);
                           return Status::Running;
                       }));
        root->Add(std::move(danger));
        auto work = std::make_unique<Sequence>(u8"협동 작업 지원");
        work->Add(Check("AssignedTask",
                        [](const Blackboard& b)
                        {
                            return b.workRequested;
                        }));
        work->Add(Do(u8"작업 지점으로 이동",
                     [](Blackboard& b, float dt)
                     {
                         return MoveTo(b, b.lastSeen, 3.6f, dt);
                     }));
        work->Add(Do(
            u8"협동 작업 수행",
            [](Blackboard& b, float)
            {
                b.working = true;
                return Status::Running;
            },
            [](Blackboard& b)
            {
                b.working = false;
            }));
        root->Add(std::move(work));
        auto follow = std::make_unique<Sequence>(u8"플레이어 동행");
        follow->Add(Check("LeaderFar",
                          [](const Blackboard& b)
                          {
                              return FlatDistance(b.position, b.state->players[b.leader].position) > 4;
                          }));
        follow->Add(Do(u8"플레이어 따라가기",
                       [](Blackboard& b, float dt)
                       {
                           return MoveTo(b, b.lastSeen, 3.8f, dt);
                       }));
        root->Add(std::move(follow));
        root->Add(Do(u8"대기 및 주변 경계",
                     [](Blackboard& b, float)
                     {
                         b.working = false;
                         return Status::Running;
                     }));
        return root;
    }

    NodePtr ScientistTree()
    {
        auto root = std::make_unique<Selector>(u8"연구원 우선순위");
        auto dead = std::make_unique<Sequence>(u8"사망 상태");
        dead->Add(Check("ScientistDead",
                        [](const Blackboard& b)
                        {
                            return b.state->scientistDead != 0;
                        }));
        dead->Add(Do(u8"사망",
                     [](Blackboard&, float)
                     {
                         return Status::Running;
                     }));
        root->Add(std::move(dead));
        auto escort = std::make_unique<Sequence>(u8"카드 교환 후 동행");
        escort->Add(Check("CardExchanged",
                          [](const Blackboard& b)
                          {
                              return b.state->chapter >= 9;
                          }));
        escort->Add(Do(u8"연구원 동행",
                       [](Blackboard& b, float dt)
                       {
                           return MoveTo(b, b.lastSeen, 3.8f, dt);
                       }));
        root->Add(std::move(escort));
        root->Add(Do(u8"샘플을 기다리며 대화",
                     [](Blackboard&, float)
                     {
                         return Status::Running;
                     }));
        return root;
    }

    Agent::Agent(bool alien, bool scientist)
        : tree(scientist ? ScientistTree() : (alien ? AlienTree() : CompanionTree()))
    {
    }

    void Agent::Sense(const std::vector<Noise>& noises, float dt)
    {
        auto& b = blackboard;
        clock += dt;
        b.memory = (std::max)(0.f, b.memory - dt);
        b.attackCooldown = (std::max)(0.f, b.attackCooldown - dt);
        b.stun = (std::max)(0.f, b.stun - dt);
        b.visiblePlayer = -1;
        float best = 24;
        for (int i = 0; i < 3; ++i)
        {
            const auto& p = b.state->players[i];
            if (p.health <= 0)
            {
                continue;
            }
            float distance = FlatDistance(b.position, p.position);
            bool inCone = distance < 3 || Dot(b.facing, Normal(Flat(p.position - b.position))) > .5f;
            bool visible = p.hidden < 0 && distance < best && inCone &&
                           b.world->Visible(b.position + Vec3{0, 1.8f, 0}, p.position + Vec3{0, 1.6f, 0});
            if (visible)
            {
                best = FlatDistance(b.position, p.position);
                b.visiblePlayer = i;
                b.lastSeen = p.position;
                b.memory = 7;
                lastVisible[i] = clock;
            }
            if (p.hidden >= 0 && previousHidden[i] < 0 && clock - lastVisible[i] < .8f &&
                b.world->Visible(b.position + Vec3{0, 1.8f, 0}, b.cabinetFront(p.hidden) + Vec3{0, 1.5f, 0}))
            {
                b.witnessedCabinet = p.hidden;
                b.search = 0;
            }
            previousHidden[i] = p.hidden;
        }
        for (const auto& noise : noises)
        {
            if (noise.age > 2 || noise.id <= b.heardNoise)
            {
                continue;
            }
            float distance = FlatDistance(b.position, noise.position);
            float attenuation =
                b.world->Visible(b.position + Vec3{0, 1.4f, 0}, noise.position + Vec3{0, 1.4f, 0}) ? 1.f
                                                                                                   : .12f;
            if (distance < noise.strength * attenuation)
            {
                b.heard = true;
                b.noisePosition = noise.position;
                b.heardNoise = noise.id;
            }
        }
    }

    void Agent::Tick(float dt)
    {
        blackboard.working = false;
        tree->Tick(blackboard, dt);
    }

    void Agent::Abort()
    {
        tree->Abort(blackboard);
    }

} // namespace AI
