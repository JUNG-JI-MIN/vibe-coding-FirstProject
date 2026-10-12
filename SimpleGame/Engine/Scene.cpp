#include "stdafx.h"
#include "Scene.h"

namespace Engine
{
    Scene::Scene(Framework& framework) : framework(framework)
    {
    }

    void Scene::Enter()
    {
    }

    void Scene::Pause()
    {
    }

    void Scene::Resume()
    {
    }

    void Scene::Update(float dt)
    {
        for (auto& object : objects)
        {
            object->Update(dt);
        }
    }

    void Scene::Resize(int w, int h)
    {
        width = (std::max)(1, w);
        height = (std::max)(1, h);
    }

    void Scene::Key(unsigned char, bool)
    {
    }

    void Scene::Mouse(int, int)
    {
    }

    void Scene::Button(int, int, int, int)
    {
    }

} // namespace Engine
