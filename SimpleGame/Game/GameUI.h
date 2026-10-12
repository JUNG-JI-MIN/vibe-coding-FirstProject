#pragma once
#include "GameObjects.h"
#include "../KoreanText.h"

namespace Game
{
    namespace UI
    {
        void Screen();

        void Rect(float x, float y, float w, float h, float r = .025f, float g = .055f, float b = .07f,
                  float a = .9f);

        void Text(float x, float y, const char* text, float r = .73f, float g = .85f, float b = .86f);

        void Center(float x, float y, const char* text);

        void Wrap(float x, float y, const char* text, float width);

        void Ring(float x, float y, float radius, float fraction, float r = .8f, float g = .7f,
                  float b = .25f);

        void Camera(int width, int height, Vec3 eye, float yaw, float pitch);

        void Lights(bool power, float clock);

        void Radar(const Mesh& mesh, const StationWorld& world, Vec3 player, Vec3 target, bool objective,
                   float x, float y, float scale, Vec3 center);
    } // namespace UI
} // namespace Game
