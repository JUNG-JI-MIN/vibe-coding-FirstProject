/*
Copyright 2022 Lee Taek Hee (Tech University of Korea)
This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/
#include "stdafx.h"
#include "Game/SceneFactories.h"
#include <iostream>

std::unique_ptr<Engine::Framework> framework;
int lastTick = 0;

void Display()
{
    if (framework)
    {
        framework->Render();
    }
}

void Timer(int)
{
    int now = glutGet(GLUT_ELAPSED_TIME);
    float dt = Engine::Clamp((now - lastTick) / 1000.f, 0, .05f);
    lastTick = now;
    if (framework)
    {
        try
        {
            framework->Update(dt);
        }
        catch (const std::exception& error)
        {
            std::cerr << error.what() << "\n";
            framework->error = u8"씬 실행 중 오류가 발생했습니다. 콘솔 로그를 확인하세요.";
            framework->Replace(Game::MakeTitle(*framework));
        }
        glutPostRedisplay();
    }
    glutTimerFunc(16, Timer, 0);
}

void Resize(int width, int height)
{
    glViewport(0, 0, width, height);
    if (framework)
    {
        framework->Resize(width, height);
    }
}

void KeyDown(unsigned char key, int, int)
{
    if (framework)
    {
        framework->Key(key, true);
    }
}

void KeyUp(unsigned char key, int, int)
{
    if (framework)
    {
        framework->Key(key, false);
    }
}

void Motion(int x, int y)
{
    if (framework)
    {
        framework->Mouse(x, y);
    }
}

void Button(int button, int state, int x, int y)
{
    if (framework)
    {
        framework->Button(button, state, x, y);
    }
}

void Special(int key, int, int)
{
    if (!framework)
    {
        return;
    }
    if (key == GLUT_KEY_UP)
    {
        framework->Key('w', true);
        framework->Key('w', false);
    }
    if (key == GLUT_KEY_DOWN)
    {
        framework->Key('s', true);
        framework->Key('s', false);
    }
}

void Close()
{
    if (framework)
    {
        framework->Shutdown();
    }
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Orbital Station");
    SetWindowTextW(WindowFromDC(wglGetCurrentDC()), L"우주정거장 - 구조 신호");
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "OpenGL initialization failed\n";
        return 1;
    }
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    try
    {
        framework = std::make_unique<Engine::Framework>();
        framework->Run(Game::MakeTitle(*framework));
    }
    catch (const std::exception& error)
    {
        MessageBoxA(nullptr, error.what(), "Station resource error", MB_OK | MB_ICONERROR);
        if (framework)
        {
            framework->Shutdown();
        }
        return 1;
    }
    glutDisplayFunc(Display);
    glutReshapeFunc(Resize);
    glutKeyboardFunc(KeyDown);
    glutKeyboardUpFunc(KeyUp);
    glutSpecialFunc(Special);
    glutPassiveMotionFunc(Motion);
    glutMotionFunc(Motion);
    glutIgnoreKeyRepeat(1);
    glutCloseFunc(Close);
    glutMouseFunc(Button);
    lastTick = glutGet(GLUT_ELAPSED_TIME);
    glutTimerFunc(16, Timer, 0);
    glutMainLoop();
    if (framework)
    {
        framework->Shutdown();
    }
    framework.reset();
    return 0;
}