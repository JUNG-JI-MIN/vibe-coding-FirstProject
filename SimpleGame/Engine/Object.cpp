#include "stdafx.h"
#include "Object.h"

namespace Engine
{
    Object::Object(ResourceManager& resources, const std::string& type) : mesh(resources.GetMesh(type))
    {
    }

    void Object::Update(float)
    {
    }

    void Object::Draw(MetalMaterial& shader, bool hdr)
    {
        if (!visible)
        {
            return;
        }
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glRotatef(yaw, 0, 1, 0);
        glScalef(scale.x, scale.y, scale.z);
        mesh->Draw(shader, hdr, ownMaterial ? &material : nullptr);
        glPopMatrix();
    }

} // namespace Engine
