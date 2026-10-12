#pragma once
#include "Object.h"

namespace Engine
{
    class Framework;

    class Scene
    {
    protected:
        Framework& framework;
        std::vector<std::unique_ptr<Object>> objects;
        int width = 1280, height = 720;

    public:
        explicit Scene(Framework& framework);

        virtual ~Scene() = default;

        virtual void Enter();

        virtual void Pause();

        virtual void Resume();

        virtual void Update(float dt);

        virtual void Render() = 0;

        virtual void Resize(int w, int h);

        virtual void Key(unsigned char, bool);

        virtual void Mouse(int, int);

        virtual void Button(int, int, int, int);
    };
} // namespace Engine
