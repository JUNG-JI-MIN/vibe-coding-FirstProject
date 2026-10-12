#pragma once

#include "BehaviorTree.h"
#include "../Game/GameState.h"

namespace AI
{
    using namespace Game;

    Status MoveTo(Blackboard& b, Vec3 goal, float speed, float dt);

    NodePtr Check(const char* name, std::function<bool(const Blackboard&)> condition);

    NodePtr Do(const char* name, std::function<Status(Blackboard&, float)> action,
               std::function<void(Blackboard&)> abort = {});

    NodePtr AlienTree();

    NodePtr CompanionTree();

    NodePtr ScientistTree();

    class Agent
    {
        NodePtr tree;
        std::array<int, 3> previousHidden = {{-1, -1, -1}};
        std::array<float, 3> lastVisible = {{-100, -100, -100}};
        float clock = 0;

    public:
        Blackboard blackboard;

        explicit Agent(bool alien, bool scientist = false);

        Agent(const Agent&) = delete;
        Agent& operator=(const Agent&) = delete;

        void Sense(const std::vector<Noise>& noises, float dt);

        void Tick(float dt);

        void Abort();
    };
} // namespace AI
