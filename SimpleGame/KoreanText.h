#pragma once

#include "Dependencies/glew.h"
#include <Windows.h>
#include <map>
#include <string>
#pragma comment(lib, "gdi32.lib")

// UTF-8 UI text, rendered with cached Windows font glyphs in the compatibility context.
namespace KoreanText
{
    struct Glyph
    {
        GLuint list = 0;
        int width = 8;
    };

    std::map<wchar_t, Glyph>& Cache();

    std::wstring Decode(const char* text);

    const Glyph& Get(wchar_t character);

    int Width(const char* text);

    void Draw(float x, float y, const char* text);

    void Release();
} // namespace KoreanText
