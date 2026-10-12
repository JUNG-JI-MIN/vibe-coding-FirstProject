#include "stdafx.h"
#include "Framework.h"

namespace Engine
{
    void Framework::Apply()
    {
        while (!pending.empty())
        {
            auto change = std::move(pending.front());
            pending.pop_front();
            if (change.operation == Operation::Pop)
            {
                if (scenes.size() > 1)
                {
                    scenes.pop_back();
                    scenes.back()->Resize(width, height);
                    scenes.back()->Resume();
                }
                continue;
            }
            if (change.operation == Operation::Replace)
            {
                scenes.clear();
            }
            else if (!scenes.empty())
            {
                scenes.back()->Pause();
            }
            scenes.push_back(std::move(change.scene));
            scenes.back()->Resize(width, height);
            scenes.back()->Enter();
        }
    }

    void Framework::Run(std::unique_ptr<Scene> scene)
    {
        Replace(std::move(scene));
        Apply();
    }

    void Framework::Replace(std::unique_ptr<Scene> scene)
    {
        pending.push_back({Operation::Replace, std::move(scene)});
    }

    void Framework::Push(std::unique_ptr<Scene> scene)
    {
        pending.push_back({Operation::Push, std::move(scene)});
    }

    void Framework::Pop()
    {
        pending.push_back({Operation::Pop, nullptr});
    }

    void Framework::Quit()
    {
        glutLeaveMainLoop();
    }

    void Framework::Update(float dt)
    {
        if (!scenes.empty())
        {
            scenes.back()->Update(dt);
        }
        Apply();
    }

    void Framework::Render()
    {
        if (!scenes.empty())
        {
            scenes.back()->Render();
        }
        glutSwapBuffers();
    }

    void Framework::Resize(int w, int h)
    {
        width = (std::max)(1, w);
        height = (std::max)(1, h);
        if (!scenes.empty())
        {
            scenes.back()->Resize(width, height);
        }
    }

    void Framework::Key(unsigned char key, bool down)
    {
        if (!scenes.empty())
        {
            scenes.back()->Key(key, down);
        }
        Apply();
    }

    void Framework::Mouse(int x, int y)
    {
        if (!scenes.empty())
        {
            scenes.back()->Mouse(x, y);
        }
    }

    void Framework::Button(int button, int state, int x, int y)
    {
        if (!scenes.empty())
        {
            scenes.back()->Button(button, state, x, y);
        }
        Apply();
    }

    void Framework::Shutdown()
    {
        if (closed)
        {
            return;
        }
        closed = true;
        pending.clear();
        scenes.clear();
        resources.Clear();
        KoreanText::Release();
    }

} // namespace Engine
