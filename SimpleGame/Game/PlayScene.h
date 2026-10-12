#pragma once
#include "SceneFactories.h"
#include "GameUI.h"
#include "../PostProcessing.h"

namespace Game
{
    class PlayScene : public Scene
    {
        std::shared_ptr<GameSession> session;
        Mesh* stationMesh;
        StationWorld world;
        Simulation simulation;
        MetalMaterial material;
        PostProcessing post;
        Object* alien = nullptr;
        Object* scientist = nullptr;
        std::array<Object*, 3> people;
        bool keys[256] = {}, wheel = false, map = false, journal = false, debug = false, captured = true;
        bool cctv = false;
        int cameraIndex = 0, selectedGate = 0, dragX = 0, dragY = 0;
        std::vector<int> securityDoors;
        int selected = Flashlight, hover = Hands, mouseX = 640, mouseY = 360, lastHidden = -1;
        float yaw = 0, pitch = 0, clock = 0, wheelAnimation = 0, mapZoom = 2.45f, transition = 1,
              objectiveFlash = 4;
        Vec3 mapCenter, previousEye, cameraEye;
        int previousTarget = -1;
        std::vector<std::string> log;
        std::string lastMessage;

        PlayerState& Local();

        void Cursor();

        void Restart();

        void DrawWheel();

        void Hud();

    public:
        PlayScene(Framework& framework, std::shared_ptr<GameSession> session);

        ~PlayScene();

        void Enter() override;

        void Pause() override;

        void Resume() override;

        void Update(float dt) override;

        void Render() override;

        void Key(unsigned char key, bool down) override;

        void Mouse(int x, int y) override;

        void Button(int button, int state, int x, int y) override;
    };
} // namespace Game
