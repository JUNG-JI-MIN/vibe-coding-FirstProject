#pragma once

#include "Dependencies/glew.h"
#include <Windows.h>
#include <cstdio>
#include <string>
#include <vector>

namespace ShaderProgram
{
    inline std::wstring ExecutableDirectory()
    {
        std::vector<wchar_t> path(512);
        while (path.size() <= 32768)
        {
            DWORD length = GetModuleFileNameW(nullptr, path.data(), static_cast<DWORD>(path.size()));
            if (length == 0)
            {
                return L"";
            }
            if (length < path.size())
            {
                std::wstring fullPath(path.data(), length);
                return fullPath.substr(0, fullPath.find_last_of(L"\\/") + 1);
            }
            path.resize(path.size() * 2);
        }
        return L"";
    }

    inline bool Read(const std::wstring& path, std::string& source)
    {
        FILE* file = nullptr;
        if (_wfopen_s(&file, path.c_str(), L"rb") != 0 || !file)
        {
            return false;
        }
        source.clear();
        char buffer[4096];
        size_t count = 0;
        while ((count = std::fread(buffer, 1, sizeof(buffer), file)) > 0)
        {
            source.append(buffer, count);
        }
        bool ok = std::ferror(file) == 0;
        std::fclose(file);
        // Editors may save UTF-8 with a BOM; #version must remain the first directive.
        if (source.compare(0, 3, "\xEF\xBB\xBF") == 0)
        {
            source.erase(0, 3);
        }
        return ok && !source.empty();
    }

    inline GLuint Compile(GLenum type, const std::string& source, const std::wstring& path)
    {
        GLuint shader = glCreateShader(type);
        if (!shader)
        {
            std::fprintf(stderr, "Cannot create shader: %ls\n", path.c_str());
            return 0;
        }
        const char* text = source.c_str();
        glShaderSource(shader, 1, &text, nullptr);
        glCompileShader(shader);
        GLint ok = GL_FALSE;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            char log[4096] = {};
            glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
            std::fprintf(stderr, "Shader compilation failed (%ls):\n%s\n", path.c_str(), log);
            glDeleteShader(shader);
            return 0;
        }
        return shader;
    }

    inline GLuint Load(const wchar_t* vertexName, const wchar_t* fragmentName)
    {
        const std::wstring executable = ExecutableDirectory();
        const std::wstring directories[] = {executable + L"Shaders/", L"Shaders/", L"SimpleGame/Shaders/"};
        std::string vertexSource;
        std::string fragmentSource;
        std::wstring directory;
        bool loaded = false;
        // Read both stages from the same directory so deployed and source versions cannot mix.
        for (const auto& candidate : directories)
        {
            if (Read(candidate + vertexName, vertexSource) && Read(candidate + fragmentName, fragmentSource))
            {
                directory = candidate;
                loaded = true;
                break;
            }
        }
        if (!loaded)
        {
            std::fprintf(stderr,
                         "Cannot read shader pair: %ls, %ls. Expected Shaders beside the executable.\n",
                         vertexName, fragmentName);
            return 0;
        }
        GLuint vertex = Compile(GL_VERTEX_SHADER, vertexSource, directory + vertexName);
        if (!vertex)
        {
            return 0;
        }
        GLuint fragment = Compile(GL_FRAGMENT_SHADER, fragmentSource, directory + fragmentName);
        if (!fragment)
        {
            glDeleteShader(vertex);
            return 0;
        }
        GLuint program = glCreateProgram();
        if (program)
        {
            glAttachShader(program, vertex);
            glAttachShader(program, fragment);
            glLinkProgram(program);
        }
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        if (!program)
        {
            std::fprintf(stderr, "Cannot create shader program: %ls, %ls\n", vertexName, fragmentName);
            return 0;
        }
        GLint ok = GL_FALSE;
        glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok)
        {
            char log[4096] = {};
            glGetProgramInfoLog(program, sizeof(log), nullptr, log);
            std::fprintf(stderr, "Shader link failed (%ls, %ls):\n%s\n", vertexName, fragmentName, log);
            glDeleteProgram(program);
            return 0;
        }
        return program;
    }
} // namespace ShaderProgram
