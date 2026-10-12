#pragma once
#include "../Game/GameState.h"
#include <functional>

namespace AI
{
    using namespace Game;

    struct Blackboard
    {
        std::string activeAction;
        Vec3 position, lastSeen, noisePosition, goal, facing = {-1, 0, 0}, scriptGoal;
        std::vector<Vec3> path;
        size_t waypoint = 0;
        float repath = 0, memory = 0, search = 0, stun = 0, attackCooldown = 0;
        int visiblePlayer = -1, witnessedCabinet = -1, patrolIndex = 0;
        unsigned heardNoise = 0;
        bool heard = false, threat = false, workRequested = false, working = false, scripted = false;
        int helperIndex = -1, leader = 0;
        StationWorld* world = nullptr;
        GameState* state = nullptr;
        std::function<void(int)> attack;
        std::function<void()> eventComplete;
        std::function<Vec3(int)> cabinetFront;
    };
} // namespace AI
