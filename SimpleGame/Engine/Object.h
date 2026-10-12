#pragma once
#include "ResourceManager.h"

namespace Engine
{
    class Object
    {
    protected:
        Mesh* mesh;

    public:
        explicit Object(ResourceManager& resources, const std::string& type = "Cube");

        virtual ~Object() = default;
        Vec3 position, scale = {1, 1, 1};
        float yaw = 0;
        Material material;
        bool visible = true, ownMaterial = true;

        virtual void Update(float);

        virtual void Draw(MetalMaterial& shader, bool hdr);
    };
} // namespace Engine
