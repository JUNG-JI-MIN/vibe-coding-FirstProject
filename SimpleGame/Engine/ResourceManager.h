#pragma once
#include "Mesh.h"
#include <memory>
#include <unordered_map>

namespace Engine
{
    class ResourceManager
    {
        std::unordered_map<std::string, std::unique_ptr<Mesh>> meshes;

    public:
        Mesh* GetMesh(const std::string& key);

        size_t Count() const;

        void Clear();
    };
} // namespace Engine
