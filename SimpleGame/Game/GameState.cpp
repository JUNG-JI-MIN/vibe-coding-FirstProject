#include "stdafx.h"
#include "GameState.h"

namespace Game
{
    const char* ToolName(int tool)
    {
        const char* names[] = {u8"손전등", u8"쇠지렛대", u8"드릴",    u8"렌치",
                               u8"스캐너", u8"전압봉",   u8"구급상자"};
        return tool < 0 ? u8"맨손" : names[tool];
    }

    bool OwnTool(const GameState& state, int tool)
    {
        return tool == Hands ||
               (tool == Crowbar ? state.crowbar > 0 : (tool == StunBaton ? state.stunCharges > 0 : true));
    }

} // namespace Game
