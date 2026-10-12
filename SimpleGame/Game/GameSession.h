#pragma once
#include "GameState.h"

namespace Game
{
    struct PlayerInput
    {
        int buttons = 0;
        int tool = Flashlight;
        float yaw = 0;
        float pitch = 0;
    };

    class GameSession
    {
    public:
        static constexpr int PlayerSlot = 0;
        static constexpr int CompanionCount = 2;

        GameState state;
        PlayerInput input;
        int doorCommand = -1;

        explicit GameSession(int difficulty = 1, int playerColor = 0);

        void Reset(int difficulty, int playerColor);

        void RequestDoor(int door);
    };
} // namespace Game
