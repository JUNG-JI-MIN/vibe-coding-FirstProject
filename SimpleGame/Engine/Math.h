#pragma once

#include <algorithm>
#include <cmath>
#include <limits>

namespace Engine
{
    constexpr float Pi = 3.14159265359f;

    float Clamp(float value, float low, float high);

    struct Vec3
    {
        float x = 0, y = 0, z = 0;

        Vec3 operator+(Vec3 b) const;

        Vec3 operator-(Vec3 b) const;

        Vec3 operator*(float scale) const;
    };

    float Dot(Vec3 a, Vec3 b);

    Vec3 Cross(Vec3 a, Vec3 b);

    float Length(Vec3 a);

    float Distance(Vec3 a, Vec3 b);

    Vec3 Normal(Vec3 a);

    Vec3 Flat(Vec3 a);

    float FlatDistance(Vec3 a, Vec3 b);

    struct Bounds
    {
        Vec3 low = {1e9f, 1e9f, 1e9f}, high = {-1e9f, -1e9f, -1e9f};

        void Include(Vec3 p);

        Vec3 Center() const;

        Vec3 Size() const;
    };

    float SegmentDistance(Vec3 p, Vec3 a, Vec3 b);

    float Side(Vec3 a, Vec3 b, Vec3 c);

    bool InTriangle(Vec3 p, Vec3 a, Vec3 b, Vec3 c);
} // namespace Engine
