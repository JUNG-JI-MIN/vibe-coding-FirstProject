#pragma once

#include "StationLayout.h"
#include "Dependencies/freeglut.h"

namespace Station
{
    struct Cabinet
    {
        Point position;
        float yaw;

        Point World(float x, float z) const
        {
            float a = yaw * Pi / 180;
            return {position.x + std::cos(a) * x - std::sin(a) * z,
                    position.z + std::sin(a) * x + std::cos(a) * z};
        }
    };

    class CabinetSystem
    {
        int occupied = -1;
        bool leaving = false;
        float transition = 1, breathing = 0;
        Point approach = {}, exitPoint = {};
        float approachYaw = 0, approachPitch = 0;

        static float Ease(float t)
        {
            t = Clamp(t, 0, 1);
            return t * t * (3 - 2 * t);
        }

        static float Turn(float from, float to, float t)
        {
            float delta = std::remainder(to - from, 360.f);
            return from + delta * t;
        }

        static void Cube(float x, float y, float z, float w, float h, float d)
        {
            glPushMatrix();
            glTranslatef(x, y, z);
            glScalef(w, h, d);
            glutSolidCube(1);
            glPopMatrix();
        }

    public:
        const std::vector<Cabinet> cabinets = {
            {{21, 79}, 0},   {{30, 147.5f}, 0},  {{192, 80}, 0}, {{178, 136}, 0}, {{182, 136}, 0},
            {{186, 136}, 0}, {{110, 8.5f}, 180}, {{71, 189}, 0}, {{114, 195}, 0}, {{191.5f, 17}, 180}};

        void Furnish(Layout& layout) const
        {
            for (const auto& cabinet : cabinets)
            {
                auto box = [&](float x, float y, float z, float w, float h, float d, int material)
                {
                    Point p = cabinet.World(x, z);
                    layout.AddBox(p.x, y, p.z, w, h, d, material, false);
                };
                // The whole cabinet blocks walking; hiding explicitly enters the hollow shell.
                layout.AddObstacle(cabinet.position.x, 1.15f, cabinet.position.z, 1.2f, 2.3f, 1.2f);
                box(-.57f, 1.15f, 0, .06f, 2.3f, 1.2f, 3);
                box(.57f, 1.15f, 0, .06f, 2.3f, 1.2f, 3);
                box(0, 1.15f, .57f, 1.2f, 2.3f, .06f, 3);
                box(0, .04f, 0, 1.2f, .08f, 1.2f, 3);
                box(0, 2.26f, 0, 1.2f, .08f, 1.2f, 3);
                box(0, 2.03f, .22f, 1.08f, .045f, .64f, 5);
                box(0, 1.91f, .35f, .04f, .18f, .04f, 5);
                box(-.4f, 2.14f, -.61f, .13f, .08f, .025f, 8);
            }
        }

        bool IsInside() const
        {
            return occupied >= 0;
        }

        bool InTransition() const
        {
            return IsInside() && transition < 1;
        }

        float Facing() const
        {
            return cabinets[occupied].yaw;
        }

        Point PreviousPosition() const
        {
            return approach;
        }

        Point ExitCandidate(float side) const
        {
            return cabinets[occupied].World(side, -1.4f);
        }

        int Focus(const Layout& layout, Point player, float yaw, float pitch) const
        {
            int result = -1;
            float nearest = 2.1f;
            float a = yaw * Pi / 180, b = pitch * Pi / 180;
            for (int i = 0; i < int(cabinets.size()); ++i)
            {
                const auto& cabinet = cabinets[i];
                Point target = cabinet.World(0, -.66f);
                Point front = cabinet.World(0, -1.66f);
                if ((player.x - target.x) * (front.x - target.x) +
                        (player.z - target.z) * (front.z - target.z) <
                    0)
                {
                    continue;
                }
                float dx = target.x - player.x, dz = target.z - player.z, dy = -.15f;
                float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
                float alignment =
                    (dx * std::sin(a) * std::cos(b) - dy * std::sin(b) - dz * std::cos(a) * std::cos(b)) /
                    (std::max)(distance, .01f);
                if (distance < nearest && alignment > .8f && layout.Visible(player, 1.65f, target, 1.5f))
                {
                    result = i;
                    nearest = distance;
                }
            }
            return result;
        }

        void Enter(int index, Point& player, float& yaw, float& pitch)
        {
            approach = player;
            approachYaw = yaw;
            approachPitch = pitch;
            occupied = index;
            leaving = false;
            transition = breathing = 0;
            player = cabinets[index].World(0, .12f);
            yaw = cabinets[index].yaw;
            pitch = 0;
        }

        void Leave(Point destination)
        {
            exitPoint = destination;
            leaving = true;
            transition = 0;
        }

        void Update(float dt, Point& player)
        {
            if (!IsInside())
            {
                return;
            }
            breathing += dt;
            transition = Clamp(transition + dt / 1.1f, 0, 1);
            if (leaving && transition >= 1)
            {
                player = exitPoint;
                occupied = -1;
            }
        }

        void Camera(Point& position, float& height, float& yaw, float& pitch) const
        {
            if (!IsInside())
            {
                return;
            }
            Point inside = cabinets[occupied].World(0, .12f);
            Point doorway = cabinets[occupied].World(0, -1.05f);
            float t = Ease((transition - .2f) / .6f);
            float bob = std::sin(breathing * 1.35f) * .014f + std::sin(breathing * 2.7f) * .003f;
            if (leaving)
            {
                float first = Ease((transition - .2f) / .4f);
                float second = Ease((transition - .6f) / .2f);
                position = {inside.x + (doorway.x - inside.x) * first + (exitPoint.x - doorway.x) * second,
                            inside.z + (doorway.z - inside.z) * first + (exitPoint.z - doorway.z) * second};
                height = 1.60f + .05f * t + bob * (1 - t);
            }
            else
            {
                float first = Ease((transition - .1f) / .3f);
                float second = Ease((transition - .4f) / .4f);
                position = {approach.x + (doorway.x - approach.x) * first + (inside.x - doorway.x) * second,
                            approach.z + (doorway.z - approach.z) * first + (inside.z - doorway.z) * second};
                height = 1.65f - .05f * t + bob * t;
                yaw = Turn(approachYaw, yaw, t);
                pitch = approachPitch + (pitch - approachPitch) * t;
            }
        }

        void Reset()
        {
            occupied = -1;
            transition = 1;
            leaving = false;
            breathing = 0;
        }

        void Draw(bool hdr, MetalMaterial& material) const
        {
            if (hdr)
            {
                material.Use(.8f, .65f, 0, true);
            }
            else
            {
                glUseProgram(0);
                glEnable(GL_LIGHTING);
            }
            for (int i = 0; i < int(cabinets.size()); ++i)
            {
                const auto& cabinet = cabinets[i];
                glPushMatrix();
                glTranslatef(cabinet.position.x, 0, cabinet.position.z);
                glRotatef(-cabinet.yaw, 0, 1, 0);
                glTranslatef(-.54f, 0, -.57f);
                float angle = i == occupied ? std::sin(transition * Pi) * 88 : 0;
                glRotatef(angle, 0, 1, 0);
                glColor3f(.19f, .25f, .27f);
                Cube(.54f, .71f, 0, 1.08f, 1.38f, .045f);
                Cube(.54f, 2.055f, 0, 1.08f, .37f, .045f);
                Cube(.085f, 1.635f, 0, .17f, .47f, .045f);
                Cube(.995f, 1.635f, 0, .17f, .47f, .045f);
                // Real gaps between louvers reveal the world from the camera inside the cabinet.
                for (float y : {1.43f, 1.54f, 1.65f, 1.76f, 1.87f})
                {
                    Cube(.54f, y, 0, .74f, .065f, .045f);
                }
                glColor3f(.42f, .47f, .48f);
                Cube(.94f, 1.08f, -.065f, .04f, .25f, .085f);
                Cube(.94f, .88f, -.03f, .09f, .08f, .035f);
                glPopMatrix();
            }
            glUseProgram(0);
        }
    };
} // namespace Station
