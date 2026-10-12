#include "stdafx.h"
#include "MetalMaterial.h"

bool MetalMaterial::Initialize()
{
    if (attempted)
    {
        return program != 0;
    }
    attempted = true;

    program = ShaderProgram::Load(L"MetalMaterial.vs", L"MetalMaterial.fs");
    return program != 0;
}

void MetalMaterial::Use(float metal, float rough, float glow, bool fog)
{
    if (!Initialize())
    {
        return;
    }
    glUseProgram(program);
    glUniform1f(glGetUniformLocation(program, "metallic"), metal);
    glUniform1f(glGetUniformLocation(program, "roughness"), rough);
    glUniform1f(glGetUniformLocation(program, "emission"), glow);
    glUniform1f(glGetUniformLocation(program, "fogEnabled"), fog ? 1.f : 0.f);
    glUniform1f(glGetUniformLocation(program, "flashlightOn"), flashlight ? 1.f : 0.f);
}

void MetalMaterial::Release()
{
    if (program)
    {
        glDeleteProgram(program);
    }
    program = 0;
    attempted = false;
}
