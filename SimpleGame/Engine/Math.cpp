#include "stdafx.h"
#include "Math.h"

namespace Engine
{
    float Clamp(float value, float low, float high)
    {
        return value < low ? low : (value > high ? high : value);
    }

    Vec3 Vec3::operator+(Vec3 b) const
    {
        return {x + b.x, y + b.y, z + b.z};
    }

    Vec3 Vec3::operator-(Vec3 b) const
    {
        return {x - b.x, y - b.y, z - b.z};
    }

    Vec3 Vec3::operator*(float scale) const
    {
        return {x * scale, y * scale, z * scale};
    }

    float Dot(Vec3 a, Vec3 b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    Vec3 Cross(Vec3 a, Vec3 b)
    {
        return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
    }

    float Length(Vec3 a)
    {
        return std::sqrt(Dot(a, a));
    }

    float Distance(Vec3 a, Vec3 b)
    {
        return Length(a - b);
    }

    Vec3 Normal(Vec3 a)
    {
        return a * (1.f / (std::max)(Length(a), .00001f));
    }

    Vec3 Flat(Vec3 a)
    {
        return {a.x, 0, a.z};
    }

    float FlatDistance(Vec3 a, Vec3 b)
    {
        return Length(Flat(a - b));
    }

    void Bounds::Include(Vec3 p)
    {
        low = {(std::min)(low.x, p.x), (std::min)(low.y, p.y), (std::min)(low.z, p.z)};
        high = {(std::max)(high.x, p.x), (std::max)(high.y, p.y), (std::max)(high.z, p.z)};
    }

    Vec3 Bounds::Center() const
    {
        return (low + high) * .5f;
    }

    Vec3 Bounds::Size() const
    {
        return high - low;
    }

    float SegmentDistance(Vec3 p, Vec3 a, Vec3 b)
    {
        p = Flat(p);
        a = Flat(a);
        b = Flat(b);
        Vec3 edge = b - a;
        float t = Clamp(Dot(p - a, edge) / (std::max)(Dot(edge, edge), .00001f), 0, 1);
        return Distance(p, a + edge * t);
    }

    float Side(Vec3 a, Vec3 b, Vec3 c)
    {
        return (b.x - a.x) * (c.z - a.z) - (b.z - a.z) * (c.x - a.x);
    }

    bool InTriangle(Vec3 p, Vec3 a, Vec3 b, Vec3 c)
    {
        float x = Side(a, b, p), y = Side(b, c, p), z = Side(c, a, p);
        return !((x < -.0001f || y < -.0001f || z < -.0001f) && (x > .0001f || y > .0001f || z > .0001f));
    }

} // namespace Engine
