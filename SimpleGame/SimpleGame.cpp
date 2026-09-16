/*
Copyright 2022 Lee Taek Hee (Tech University of Korea)
This program is free software: you can redistribute it and/or modify
it under the terms of the What The Hell License. Do it plz.
This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY.
*/
#include "stdafx.h"
#include <iostream>
#include "StationScene.h"

StationScene* scene = nullptr;

void Display()
{
    if (scene)
    {
        scene->Draw();
    }
}

void Timer(int)
{
    if (scene)
    {
        scene->Update();
        glutPostRedisplay();
    }
    glutTimerFunc(16, Timer, 0);
}

void Resize(int w, int h)
{
    if (scene)
    {
        scene->Resize(w, h);
    }
}

void KeyDown(unsigned char k, int, int)
{
    if (scene)
    {
        scene->Key(k, true);
    }
}

void KeyUp(unsigned char k, int, int)
{
    if (scene)
    {
        scene->Key(k, false);
    }
}

void Motion(int x, int y)
{
    if (scene)
    {
        scene->Mouse(x, y);
    }
}

void Button(int button, int state, int x, int y)
{
    if (scene)
    {
        scene->Button(button, state, x, y);
    }
}

void Close()
{
    if (scene)
    {
        scene->ReleaseGraphics();
    }
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Orbital Station - Double Ring Survival Prototype");
    if (glewInit() != GLEW_OK)
    {
        std::cerr << "OpenGL initialization failed\n";
        return 1;
    }
    glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
    scene = new StationScene();
    glutDisplayFunc(Display);
    glutReshapeFunc(Resize);
    glutKeyboardFunc(KeyDown);
    glutKeyboardUpFunc(KeyUp);
    glutPassiveMotionFunc(Motion);
    glutMotionFunc(Motion);
    glutIgnoreKeyRepeat(1);
    glutCloseFunc(Close);
    glutMouseFunc(Button);
    glutTimerFunc(16, Timer, 0);
    glutMainLoop();
    delete scene;
    scene = nullptr;
    return 0;
}
