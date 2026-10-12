#pragma once
#include "SceneFactories.h"
#include "GameUI.h"
#include "../PostProcessing.h"

namespace Game
{
    enum class MenuPage
    {
        Title,
        Preparation,
        Settings
    };

    class MenuScene : public Scene
    {
        MenuPage page;
        Mesh* station;
        MetalMaterial material;
        PostProcessing post;
        float clock = 0;
        int selected = 0;
        int hover = -1;
        int playerColor = 0;

        std::vector<const char*> Labels() const;

        void Activate(int index);

        void DrawTeam(bool hdr);

    public:
        MenuScene(Framework& framework, MenuPage page);

        ~MenuScene();

        void Enter() override;

        void Resume() override;

        void Update(float dt) override;

        void Render() override;

        void Key(unsigned char key, bool down) override;

        void Mouse(int x, int y) override;

        void Button(int button, int state, int x, int y) override;
    };
} // namespace Game
