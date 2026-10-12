#include "stdafx.h"
#include "ResourceManager.h"

namespace Engine
{
    Mesh* ResourceManager::GetMesh(const std::string& key)
    {
        auto found = meshes.find(key);
        if (found != meshes.end())
        {
            return found->second.get();
        }
        auto mesh = std::make_unique<Mesh>();
        if (key == "Cube")
        {
            mesh->Cube();
        }
        else if (key == "Station")
        {
            const std::wstring roots[] = {ShaderProgram::ExecutableDirectory(), L"", L"SimpleGame/"};
            bool loaded = false;
            for (const auto& root : roots)
            {
                std::wstring path = root + L"Res/Station_Early_Model.obj";
                if (GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES)
                {
                    mesh->LoadObj(path);
                    loaded = true;
                    break;
                }
            }
            if (!loaded)
            {
                throw std::runtime_error("Missing Res/Station_Early_Model.obj beside executable");
            }
        }
        else
        {
            throw std::runtime_error("Unknown mesh resource: " + key);
        }
        Mesh* result = mesh.get();
        meshes.emplace(key, std::move(mesh));
        return result;
    }

    size_t ResourceManager::Count() const
    {
        return meshes.size();
    }

    void ResourceManager::Clear()
    {
        meshes.clear();
    }

} // namespace Engine
