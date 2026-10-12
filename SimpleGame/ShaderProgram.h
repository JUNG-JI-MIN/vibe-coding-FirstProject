#pragma once

#include "Dependencies/glew.h"
#include <Windows.h>
#include <cstdio>
#include <string>
#include <vector>

namespace ShaderProgram
{
    std::wstring ExecutableDirectory();

    bool Read(const std::wstring& path, std::string& source);

    GLuint Compile(GLenum type, const std::string& source, const std::wstring& path);

    GLuint Load(const wchar_t* vertexName, const wchar_t* fragmentName);
} // namespace ShaderProgram
