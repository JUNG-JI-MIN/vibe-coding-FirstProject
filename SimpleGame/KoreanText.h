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

    inline std::map<wchar_t, Glyph>& Cache()
    {
        static std::map<wchar_t, Glyph> cache;
        return cache;
    }

    inline std::wstring Decode(const char* text)
    {
        int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, nullptr, 0);
        if (count <= 1)
        {
            return L"";
        }
        std::wstring result(count, L'\0');
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, &result[0], count);
        result.resize(count - 1);
        return result;
    }

    inline const Glyph& Get(wchar_t character)
    {
        auto& cache = Cache();
        auto found = cache.find(character);
        if (found != cache.end())
        {
            return found->second;
        }
        Glyph glyph;
        HDC dc = wglGetCurrentDC();
        HFONT font =
            CreateFontW(-16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, HANGUL_CHARSET, OUT_DEFAULT_PRECIS,
                        CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH, L"Malgun Gothic");
        if (dc && font)
        {
            HGDIOBJ previous = SelectObject(dc, font);
            SIZE size = {};
            if (GetTextExtentPoint32W(dc, &character, 1, &size))
            {
                glyph.width = size.cx;
            }
            glyph.list = glGenLists(1);
            if (glyph.list && !wglUseFontBitmapsW(dc, character, 1, glyph.list))
            {
                glDeleteLists(glyph.list, 1);
                glyph.list = 0;
            }
            SelectObject(dc, previous);
        }
        if (font)
        {
            DeleteObject(font);
        }
        return cache.emplace(character, glyph).first->second;
    }

    inline int Width(const char* text)
    {
        int width = 0;
        for (wchar_t character : Decode(text))
        {
            width += Get(character).width;
        }
        return width;
    }

    inline void Draw(float x, float y, const char* text)
    {
        glRasterPos2f(x, y);
        for (wchar_t character : Decode(text))
        {
            const auto& glyph = Get(character);
            if (glyph.list)
            {
                glCallList(glyph.list);
            }
            else
            {
                glBitmap(0, 0, 0, 0, float(glyph.width), 0, nullptr);
            }
        }
    }

    inline void Release()
    {
        for (const auto& item : Cache())
        {
            if (item.second.list)
            {
                glDeleteLists(item.second.list, 1);
            }
        }
        Cache().clear();
    }
} // namespace KoreanText
