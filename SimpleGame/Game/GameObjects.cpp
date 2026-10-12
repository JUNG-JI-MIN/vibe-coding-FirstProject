#include "stdafx.h"
#include "GameObjects.h"

namespace Game
{
    ModelObject::ModelObject(ResourceManager& resources) : Object(resources)
    {
    }

    void ModelObject::Box(Vec3 p, Vec3 size, Material color)
    {
        pieces.push_back({p, size, color, 0});
    }

    void ModelObject::Draw(MetalMaterial& shader, bool hdr)
    {
        if (!visible)
        {
            return;
        }
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glRotatef(yaw, 0, 1, 0);
        for (const auto& piece : pieces)
        {
            glPushMatrix();
            glTranslatef(piece.position.x, piece.position.y, piece.position.z);
            glRotatef(piece.yaw, 0, 1, 0);
            glScalef(piece.scale.x, piece.scale.y, piece.scale.z);
            mesh->Draw(shader, hdr, &piece.material);
            glPopMatrix();
        }
        glPopMatrix();
    }

    EquipmentObject::EquipmentObject(ResourceManager& resources, const Site& site, const GameState& state)
        : ModelObject(resources), state(state), id(site.id)
    {
        position = site.position;
        Material metal = {.3f, .39f, .43f, .75f, .5f, 0}, screen = {.12f, .55f, .62f, .1f, .5f, 1.8f},
                 warning = {.6f, .37f, .1f, .5f, .7f, 0};
        if (id == 2)
        {
            Box({0, .75f, 0}, {.06f, 1.1f, .065f}, warning);
            Box({.09f, 1.27f, 0}, {.24f, .07f, .07f}, warning);
            Box({-.04f, .2f, 0}, {.15f, .05f, .07f}, warning);
        }
        else if (id == 10)
        {
            Box({0, .65f, 0}, {.8f, 1.3f, .65f}, metal);
            Box({0, 1.4f, 0}, {.18f, .3f, .18f}, {.28f, .7f, .4f, .1f, .5f, 1});
            Box({0, 1.6f, 0}, {.25f, .08f, .25f}, metal);
        }
        else if (id == 18)
        {
            Box({0, 1.1f, 0}, {.075f, .65f, .075f}, metal);
            Box({0, 1.44f, 0}, {.15f, .08f, .15f}, screen);
            Box({0, .78f, 0}, {.13f, .17f, .13f}, warning);
        }
        else if (id == 3 || id == 5 || id == 15 || id == 16)
        {
            Box({0, .55f, 0}, {.16f, 1.1f, .16f}, metal);
            Box({0, 1.28f, 0}, {.58f, .44f, .12f}, warning);
            Box({0, 1.38f, -.1f}, {.4f, .055f, .06f}, metal);
            Box({0, 1.18f, -.1f}, {.055f, .34f, .06f}, metal);
        }
        else if (id == 4 || id == 14)
        {
            for (float x : {-.28f, 0.f, .28f})
            {
                Box({x, .7f, 0}, {.14f, 1.4f, .14f}, metal);
                Box({x, 1.18f, 0}, {.2f, .18f, .2f}, warning);
            }
            Box({0, .32f, 0}, {.95f, .1f, .4f}, metal);
        }
        else if (id == 9 || id == 11)
        {
            return;
        }
        else
        {
            Box({0, .52f, 0}, {.72f, 1.04f, .6f}, metal);
            Box({0, 1.35f, .08f}, {.8f, .45f, .12f}, metal);
            Box({0, 1.35f, 0}, {.69f, .33f, .025f}, screen);
            for (int i = 0; i < 4; ++i)
            {
                Box({-.16f, 1.43f - i * .057f, -.02f}, {.33f - i * .03f, .012f, .01f},
                    {.5f, .82f, .8f, 0, .8f, 2});
            }
        }
    }

    void EquipmentObject::Update(float)
    {
        visible = !(id == 2 && state.crowbar || id == 10 && state.sample || id == 18 && state.batonTaken ||
                    id == 9 || id == 11);
        if ((id == 4 || id == 14) && pieces.size() > 1)
        {
            pieces[1].position.y = 1.18f + state.work * .2f;
        }
        if (id == 15 && pieces.size() > 2)
        {
            pieces[2].position.x = state.work * .3f;
        }
    }

    CabinetObject::CabinetObject(ResourceManager& resources, const HideSpot& spot, int id,
                                 const GameState& state)
        : ModelObject(resources), state(state), id(id)
    {
        position = spot.position;
        yaw = std::atan2(spot.front.x, spot.front.z) * 180 / Pi;
        Box({-.57f, 1.15f, 0}, {.06f, 2.3f, 1.2f});
        Box({.57f, 1.15f, 0}, {.06f, 2.3f, 1.2f});
        Box({0, 1.15f, -.57f}, {1.2f, 2.3f, .06f});
        Box({0, .04f, 0}, {1.2f, .08f, 1.2f});
        Box({0, 2.26f, 0}, {1.2f, .08f, 1.2f});
        auto leaf = [&](Vec3 p, Vec3 size)
        {
            door.push_back({p, size, {.21f, .28f, .3f, .8f, .7f, 0}, 0});
        };
        leaf({.54f, .71f, 0}, {1.08f, 1.38f, .045f});
        leaf({.54f, 2.055f, 0}, {1.08f, .37f, .045f});
        leaf({.085f, 1.635f, 0}, {.17f, .47f, .045f});
        leaf({.995f, 1.635f, 0}, {.17f, .47f, .045f});
        for (float y : {1.43f, 1.54f, 1.65f, 1.76f, 1.87f})
        {
            leaf({.54f, y, 0}, {.74f, .065f, .045f});
        }
        leaf({.94f, 1.1f, .06f}, {.045f, .24f, .09f});
    }

    void CabinetObject::Update(float dt)
    {
        int occupant = -1;
        for (int i = 0; i < 3; ++i)
        {
            if (state.players[i].hidden == id)
            {
                occupant = i;
            }
        }
        if (occupant != lastOccupant)
        {
            lastOccupant = occupant;
            animation = 0;
        }
        animation = Clamp(animation + dt / 1.1f, 0, 1);
    }

    void CabinetObject::Draw(MetalMaterial& shader, bool hdr)
    {
        ModelObject::Draw(shader, hdr);
        glPushMatrix();
        glTranslatef(position.x, position.y, position.z);
        glRotatef(yaw, 0, 1, 0);
        glTranslatef(-.54f, 0, .57f);
        glRotatef(-std::sin(animation * Pi) * 88, 0, 1, 0);
        for (const auto& piece : door)
        {
            glPushMatrix();
            glTranslatef(piece.position.x, piece.position.y, piece.position.z);
            glScalef(piece.scale.x, piece.scale.y, piece.scale.z);
            mesh->Draw(shader, hdr, &piece.material);
            glPopMatrix();
        }
        glPopMatrix();
    }

    DoorObject::DoorObject(ResourceManager& resources, const Door& door) : Object(resources), door(door)
    {
    }

    void DoorObject::Draw(MetalMaterial& shader, bool hdr)
    {
        float angle = std::atan2(door.across.x, door.across.z) * 180 / Pi;
        for (int side : {-1, 1})
        {
            float center = side * (door.width * .25f + door.amount * door.width * .55f);
            glPushMatrix();
            glTranslatef(door.center.x, 0, door.center.z);
            glRotatef(angle, 0, 1, 0);
            auto box = [&](Vec3 p, Vec3 size, Material m)
            {
                glPushMatrix();
                glTranslatef(p.x, p.y, p.z);
                glScalef(size.x, size.y, size.z);
                mesh->Draw(shader, hdr, &m);
                glPopMatrix();
            };
            Material frame = {.38f, .46f, .49f, .75f, .5f, 0};
            box({0, .08f, center}, {.12f, .16f, door.width * .5f}, frame);
            box({0, 2.52f, center}, {.12f, .16f, door.width * .5f}, frame);
            box({0, 1.3f, center - door.width * .25f + .025f}, {.12f, 2.6f, .05f}, frame);
            box({0, 1.3f, center + door.width * .25f - .025f}, {.12f, 2.6f, .05f}, frame);
            glUseProgram(0);
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            glColor4f(.16f, .43f, .51f, .2f);
            glBegin(GL_QUADS);
            float half = door.width * .25f - .05f;
            glVertex3f(0, .16f, center - half);
            glVertex3f(0, .16f, center + half);
            glVertex3f(0, 2.44f, center + half);
            glVertex3f(0, 2.44f, center - half);
            glEnd();
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glPopMatrix();
        }
    }

} // namespace Game
