#pragma once

#include "Math.h"
#include "../MetalMaterial.h"
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#pragma comment(lib, "opengl32.lib")

namespace Engine
{
    struct Vertex
    {
        Vec3 position, normal;
    };

    struct Triangle
    {
        Vec3 a, b, c;
        std::string group;
    };

    struct Material
    {
        float r = .3f, g = .36f, b = .39f, metal = .7f, rough = .55f, glow = 0;
    };

    class Mesh
    {
        struct Part
        {
            std::vector<Vertex> vertices;
            Material material;
            GLuint buffer = 0;
        };

        std::map<std::string, Part> parts;
        int repairedFaces = 0;

        static Material Fallback(const std::string& name);

        void Face(const std::vector<Vec3>& points, const std::vector<Vec3>& normals, const std::string& group,
                  const std::string& material);

    public:
        std::vector<Triangle> triangles;
        std::map<std::string, Bounds> groups;

        ~Mesh();

        Mesh() = default;
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;

        void LoadObj(const std::wstring& path);

        void Cube();

        void Draw(MetalMaterial& shader, bool hdr, const Material* overrideMaterial = nullptr);

        void Release();
    };
} // namespace Engine
