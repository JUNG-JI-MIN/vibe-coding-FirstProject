#pragma once
#include "GameSession.h"
#include "../Engine/Framework.h"

namespace Game
{
    std::unique_ptr<Engine::Scene> MakeTitle(Engine::Framework& framework);
    std::unique_ptr<Engine::Scene> MakeSettings(Engine::Framework& framework);
    std::unique_ptr<Engine::Scene> MakePlay(Engine::Framework& framework,
                                            std::shared_ptr<GameSession> session);
} // namespace Game
