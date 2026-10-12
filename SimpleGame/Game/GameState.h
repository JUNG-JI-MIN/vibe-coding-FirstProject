#pragma once
#include "StationWorld.h"
#include <array>

namespace Game
{
    enum Tool
    {
        Hands = -1,
        Flashlight,
        Crowbar,
        Drill,
        Wrench,
        Scanner,
        StunBaton,
        Medkit,
        ToolCount
    };

    struct PlayerState
    {
        Vec3 position;
        float yaw = 0, pitch = 0, health = 100, stamina = 150;
        float scanTimer = 0;
        bool npc = false;
        int color = 0, hidden = -1, tool = Flashlight, flashlight = 0;
        int buttons = 0, previousButtons = 0, medkits = 3;
    };

    enum InputBits
    {
        Forward = 1,
        Backward = 2,
        Left = 4,
        Right = 8,
        Run = 16,
        Interact = 32,
        Use = 64,
        PressureDown = 128,
        PressureUp = 256
    };

    struct GameState
    {
        std::array<PlayerState, 3> players;
        int chapter = 0, substep = 0, outcome = 0, ownerRoom = 0, scientistDead = 0, difficulty = 1;
        int batonTaken = 0, hideVisited = 0, shadowDone = 0;
        int crowbar = 0, stunCharges = 0, sample = 0, card = 0, power = 0, wire = 0, valve = 0;
        int fuel = 0, clamp = 0, pressureDone = 0, manual = 0;
        float work = 0, timer = 120, pressure = 0, stageTime = 0, elapsed = 0;
        std::array<float, 19> taskProgress = {};
        unsigned long long controlledDoors = 0;
        Vec3 alienPosition, scientistPosition;
        float alienStun = 0;
        int alienActive = 0, alienMode = 0;
        int helpersWorking = 0;
    };

    struct Noise
    {
        Vec3 position;
        float strength, age = 0;
        unsigned id;
    };

    struct Site
    {
        int id;
        const char* label;
        Vec3 position;
        float duration;
        int requiredTool = Hands;
    };

    const char* ToolName(int tool);

    bool OwnTool(const GameState& state, int tool);
} // namespace Game
