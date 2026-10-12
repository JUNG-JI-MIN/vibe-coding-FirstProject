#include "stdafx.h"
#include "GameSession.h"

namespace Game
{
    constexpr int GameSession::PlayerSlot;
    constexpr int GameSession::CompanionCount;

    GameSession::GameSession(int difficulty, int playerColor)
    {
        Reset(difficulty, playerColor);
    }

    void GameSession::Reset(int difficulty, int playerColor)
    {
        state = GameState();
        state.difficulty = difficulty;
        state.ownerRoom = GetTickCount() % 3;
        for (int i = 0; i < 3; ++i)
        {
            auto& player = state.players[i];
            player.npc = i != PlayerSlot;
            player.color = (playerColor + i) % 3;
        }
        input = PlayerInput();
        doorCommand = -1;
    }

    void GameSession::RequestDoor(int door)
    {
        doorCommand = door;
    }

} // namespace Game
