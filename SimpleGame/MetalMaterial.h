#pragma once
#include "ShaderProgram.h"

class MetalMaterial
{
    GLuint program = 0;
    bool attempted = false;

public:
    bool flashlight = false;

    bool Initialize();

    void Use(float metal, float rough, float glow, bool fog);

    void Release();
};
