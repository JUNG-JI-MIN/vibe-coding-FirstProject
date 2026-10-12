#include "stdafx.h"
#include "MenuScenes.h"

namespace Game
{
    std::vector<const char*> MenuScene::Labels() const
    {
        switch (page)
        {
        case MenuPage::Title:
            return {u8"시작", u8"설정", u8"종료"};
        case MenuPage::Preparation:
            return {u8"난이도 변경", u8"플레이어 색 변경", u8"탐사 시작", u8"뒤로가기"};
        case MenuPage::Settings:
            return {u8"밝기 조절", u8"마우스 감도 조절", u8"카메라 흔들림 변경", u8"뒤로가기"};
        }
        return {};
    }

    void MenuScene::Activate(int index)
    {
        if (page == MenuPage::Title)
        {
            if (index == 0)
            {
                framework.Push(std::make_unique<MenuScene>(framework, MenuPage::Preparation));
            }
            else if (index == 1)
            {
                framework.Push(MakeSettings(framework));
            }
            else
            {
                framework.Quit();
            }
        }
        else if (page == MenuPage::Preparation)
        {
            if (index == 0)
            {
                framework.settings.difficulty = (framework.settings.difficulty + 1) % 3;
            }
            else if (index == 1)
            {
                playerColor = (playerColor + 1) % 3;
            }
            else if (index == 2)
            {
                auto session = std::make_shared<GameSession>(framework.settings.difficulty, playerColor);
                framework.Replace(MakePlay(framework, std::move(session)));
            }
            else
            {
                framework.Pop();
            }
        }
        else
        {
            if (index == 0)
            {
                framework.settings.exposure += .15f;
                if (framework.settings.exposure > 1.8f)
                {
                    framework.settings.exposure = .6f;
                }
            }
            else if (index == 1)
            {
                framework.settings.mouseSensitivity += .03f;
                if (framework.settings.mouseSensitivity > .25f)
                {
                    framework.settings.mouseSensitivity = .06f;
                }
            }
            else if (index == 2)
            {
                framework.settings.cameraMotion = !framework.settings.cameraMotion;
            }
            else
            {
                framework.Pop();
            }
        }
    }

    void MenuScene::DrawTeam(bool hdr)
    {
        int left = int(width * 570.f / 1280);
        glViewport(left, 0, width - left, height);
        UI::Camera(width - left, height, {0, 2.8f, 7}, 0, 5);
        const Material colors[] = {
            {.19f, .56f, .7f, .1f, .6f, 0}, {.7f, .49f, .18f, .1f, .6f, 0}, {.41f, .7f, .23f, .1f, .6f, 0}};
        for (int i = 0; i < 3; ++i)
        {
            Object body(framework.resources);
            body.position = {float(i * 2 - 2), 1, 0};
            body.scale = {.65f, 1.75f, .5f};
            body.material = colors[(playerColor + i) % 3];
            body.Draw(material, hdr);

            Object lamp(framework.resources);
            lamp.position = {float(i * 2 - 2), 2.5f, 0};
            lamp.scale = {1, .07f, .6f};
            lamp.material = {.18f, .72f, .64f, 0, .5f, 3};
            lamp.Draw(material, hdr);
        }
    }

    MenuScene::MenuScene(Framework& framework, MenuPage page)
        : Scene(framework), page(page), station(framework.resources.GetMesh("Station"))
    {
        auto backdrop = std::make_unique<Object>(framework.resources, "Station");
        backdrop->ownMaterial = false;
        objects.push_back(std::move(backdrop));
    }

    MenuScene::~MenuScene()
    {
        material.Release();
        post.Release();
    }

    void MenuScene::Enter()
    {
        glutSetCursor(GLUT_CURSOR_INHERIT);
    }

    void MenuScene::Resume()
    {
        glutSetCursor(GLUT_CURSOR_INHERIT);
    }

    void MenuScene::Update(float dt)
    {
        clock += dt;
    }

    void MenuScene::Render()
    {
        bool hdr = material.Initialize() && post.Begin(width, height);
        post.exposure = framework.settings.exposure;
        glClearColor(.004f, .008f, .013f, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        float a = clock * .035f;
        UI::Camera(width, height, {38 * std::cos(a), 1.65f, 38 * std::sin(a)}, a * 180 / Pi + 180, 0);
        UI::Lights(true, clock);
        objects[0]->Draw(material, hdr);
        if (page == MenuPage::Preparation)
        {
            glClear(GL_DEPTH_BUFFER_BIT);
            DrawTeam(hdr);
            glViewport(0, 0, width, height);
        }
        glUseProgram(0);
        if (hdr)
        {
            post.Apply();
        }
        UI::Screen();
        UI::Rect(0, 0, 570, 720, .004f, .012f, .02f, .8f);
        const char* titles[] = {u8"우주정거장 · 구조 신호", u8"탐사 준비", u8"설정"};
        UI::Text(52, 75, titles[int(page)]);
        UI::Text(52, 112, u8"1인 탐사 · 행동 트리로 움직이는 동료 NPC 2명");
        auto labels = Labels();
        float start = page == MenuPage::Title ? 470.f : 390.f;
        for (int i = 0; i < int(labels.size()); ++i)
        {
            bool active = i == selected || i == hover;
            UI::Rect(45, start + i * 48 - 26, 465, 39, active ? .13f : .03f, active ? .26f : .08f,
                     active ? .29f : .1f, .9f);
            UI::Text(62, start + i * 48, labels[i], active ? .9f : .4f, active ? .94f : .5f,
                     active ? .93f : .53f);
        }
        if (page == MenuPage::Preparation)
        {
            const char* difficulty[] = {u8"쉬움", u8"보통", u8"어려움"};
            UI::Text(52, 210, difficulty[framework.settings.difficulty]);
            UI::Wrap(52, 246, u8"동료는 플레이어를 따라가고 위험을 피하며 문 밀기와 밸브 작업을 지원합니다.",
                     450);
            const char* members[] = {u8"플레이어", u8"동료 NPC 1", u8"동료 NPC 2"};
            for (int i = 0; i < 3; ++i)
            {
                UI::Center(925 + (i - 1) * 165.f, 540, members[i]);
            }
        }
        if (page == MenuPage::Settings)
        {
            char line[160];
            sprintf_s(line, u8"밝기 %.2f · 감도 %.2f · 흔들림 %s", framework.settings.exposure,
                      framework.settings.mouseSensitivity,
                      framework.settings.cameraMotion ? u8"켜짐" : u8"꺼짐");
            UI::Text(52, 225, line);
        }
        if (!framework.error.empty())
        {
            UI::Wrap(560, 640, framework.error.c_str(), 660);
        }
        UI::Text(52, 702, u8"마우스 선택 / W·S 또는 방향키 이동 / Enter 실행 / Esc 뒤로");
    }

    void MenuScene::Key(unsigned char key, bool down)
    {
        if (!down)
        {
            return;
        }
        if (key == 'w' || key == 'W')
        {
            selected = (selected + int(Labels().size()) - 1) % int(Labels().size());
        }
        else if (key == 's' || key == 'S')
        {
            selected = (selected + 1) % int(Labels().size());
        }
        else if (key == 13)
        {
            Activate(selected);
        }
        else if (key == 27 && page != MenuPage::Title)
        {
            framework.Pop();
        }
    }

    void MenuScene::Mouse(int x, int y)
    {
        float px = x * 1280.f / width;
        float py = y * 720.f / height;
        float start = page == MenuPage::Title ? 470.f : 390.f;
        hover = -1;
        if (px >= 45 && px <= 510 && py >= start - 26)
        {
            int row = int((py - (start - 26)) / 48);
            if (row < int(Labels().size()) && py < start + row * 48 + 13)
            {
                hover = row;
            }
        }
    }

    void MenuScene::Button(int button, int state, int x, int y)
    {
        if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
        {
            return;
        }
        Mouse(x, y);
        if (hover >= 0)
        {
            selected = hover;
            Activate(selected);
        }
    }

} // namespace Game
