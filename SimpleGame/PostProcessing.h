#pragma once
#include "ShaderProgram.h"

// Scene radiance is preserved above 1.0 in an RGBA16F framebuffer until tone mapping.
class PostProcessing
{
    GLuint program = 0, texture = 0, framebuffer = 0, depth = 0;
    int textureWidth = 0, textureHeight = 0;
    bool attempted = false, available = false, active = false;

    bool Initialize();

public:
    float exposure = .85f;
    float vignetteStrength = .82f;
    float bloomStrength = .12f;

    bool Begin(int width, int height);

    void Apply();

    void Release();
};
