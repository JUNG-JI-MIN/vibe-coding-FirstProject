#pragma once
#include "Scene.h"
#include "../KoreanText.h"
#include "../Dependencies/freeglut.h"
#include <deque>

namespace Engine
{
    struct Settings
    {
        int difficulty = 1;
        float exposure = .9f, mouseSensitivity = .12f;
        bool cameraMotion = true;
    };

    class Framework
    {
        enum class Operation
        {
            Replace,
            Push,
            Pop
        };

        struct Transition
        {
            Operation operation;
            std::unique_ptr<Scene> scene;
        };

        std::vector<std::unique_ptr<Scene>> scenes;
        std::deque<Transition> pending;
        int width = 1280, height = 720;
        bool closed = false;

        void Apply();

    public:
        ResourceManager resources;
        Settings settings;
        std::string error;

        void Run(std::unique_ptr<Scene> scene);

        void Replace(std::unique_ptr<Scene> scene);

        void Push(std::unique_ptr<Scene> scene);

        void Pop();

        void Quit();

        void Update(float dt);

        void Render();

        void Resize(int w, int h);

        void Key(unsigned char key, bool down);

        void Mouse(int x, int y);

        void Button(int button, int state, int x, int y);

        void Shutdown();
    };
} // namespace Engine
