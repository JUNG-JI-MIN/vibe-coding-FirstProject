#include "stdafx.h"
#include "SceneFactories.h"
#include "MenuScenes.h"
#include "PlayScene.h"

namespace Game
{
    std::unique_ptr<Engine::Scene> MakeTitle(Engine::Framework& framework)
    {
        return std::make_unique<MenuScene>(framework, MenuPage::Title);
    }

    std::unique_ptr<Engine::Scene> MakeSettings(Engine::Framework& framework)
    {
        return std::make_unique<MenuScene>(framework, MenuPage::Settings);
    }

    std::unique_ptr<Engine::Scene> MakePlay(Engine::Framework& framework,
                                            std::shared_ptr<GameSession> session)
    {
        return std::make_unique<PlayScene>(framework, std::move(session));
    }
} // namespace Game
