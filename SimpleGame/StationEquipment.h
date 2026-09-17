#pragma once

#include "StationLayout.h"
#include "Dependencies/freeglut.h"

namespace Station
{
    namespace Equipment
    {
        inline void Box(float x, float y, float z, float w, float h, float d)
        {
            glPushMatrix();
            glTranslatef(x, y, z);
            glScalef(w, h, d);
            glutSolidCube(1);
            glPopMatrix();
        }

        inline void Cylinder(float x, float y, float z, float radius, float length)
        {
            glPushMatrix();
            glTranslatef(x, y, z);
            glRotatef(-90, 1, 0, 0);
            glutSolidCylinder(radius, length, 16, 1);
            glPopMatrix();
        }

        inline float Height(int index)
        {
            return index == 8 ? .72f : (index == 0 || index == 5 || index == 11 ? 1.42f : 1.22f);
        }

        inline void Furnish(Layout& layout, int index, Point p)
        {
            if (index == 1 || index == 2 || index == 18)
            {
                return;
            }
            // Each fixture gets its own footprint instead of an identical square podium.
            float width = 1.05f, depth = .65f, height = .94f;
            if (index == 0 || index == 5 || index == 11 || index == 20)
            {
                width = .46f;
                depth = .42f;
            }
            else if (index == 4)
            {
                width = 1.35f;
                depth = .65f;
            }
            else if (index == 8)
            {
                width = .62f;
                depth = .45f;
                height = .58f;
            }
            else if (index == 15 || index == 17)
            {
                width = .72f;
                depth = .7f;
                height = .82f;
            }
            layout.AddObstacle(p.x, height * .5f, p.z, width, height, depth);
            // Stationary legs and housings join the material batches, not the per-frame mesh generation.
            int material = index == 6 || index == 12 ? 5 : (index == 7 || index == 8 ? 2 : 3);
            if (index == 0 || index == 5 || index == 11 || index == 20)
            {
                layout.AddBox(p.x, .48f, p.z, .14f, .96f, .14f, material, false);
                layout.AddBox(p.x, .045f, p.z, .5f, .09f, .48f, material, false);
            }
            else
            {
                for (int side : {-1, 1})
                {
                    layout.AddBox(p.x + side * width * .4f, height * .5f, p.z, .09f, height, depth, material,
                                  false);
                }
                layout.AddBox(p.x, height, p.z, width, .08f, depth, material, false);
            }
        }

        inline void Monitor(float x, float y, float z, bool on)
        {
            glColor3f(.16f, .2f, .23f);
            Box(x, y, z, .8f, .48f, .12f);
            glColor3f(on ? .2f : .06f, on ? .64f : .12f, on ? .63f : .14f);
            Box(x, y, z - .07f, .69f, .36f, .025f);
            glColor3f(.46f, .76f, .73f);
            for (int i = 0; i < 4; ++i)
            {
                Box(x - .13f, y + .1f - i * .067f, z - .085f, .33f - i * .04f, .012f, .008f);
            }
        }

        inline void Draw(int index, Point p, float progress, bool done, bool powered, float elapsed, bool hdr,
                         MetalMaterial& material)
        {
            if (index == 1 || index == 2)
            {
                return; // Horizontal pipe valves keep their dedicated animation.
            }
            if (hdr)
            {
                material.Use(.65f, .46f, 0, true);
            }
            else
            {
                glEnable(GL_LIGHTING);
            }
            glPushMatrix();
            glTranslatef(p.x, 0, p.z);
            glColor3f(.3f, .36f, .38f);
            switch (index)
            {
            case 0: // Life support: round dial, tick marks and a physical pressure needle.
                Cylinder(0, .85f, .1f, .065f, .52f);
                glPushMatrix();
                glTranslatef(0, 1.42f, 0);
                glRotatef(180, 0, 1, 0);
                glutSolidCylinder(.3, .11, 24, 1);
                glColor3f(.73f, .78f, .7f);
                glTranslatef(0, 0, .115f);
                glutSolidCylinder(.26, .015, 24, 1);
                for (int i = 0; i <= 8; ++i)
                {
                    glPushMatrix();
                    glRotatef(-120 + i * 30.f, 0, 0, 1);
                    glColor3f(.12f, .17f, .17f);
                    Box(0, .21f, .023f, .012f, .035f, .012f);
                    glPopMatrix();
                }
                glRotatef(powered ? 0 : 70 + std::sin(elapsed * 2) * 4, 0, 0, 1);
                glColor3f(.8f, .12f, .07f);
                Box(0, .08f, .03f, .017f, .19f, .015f);
                glPopMatrix();
                break;
            case 3:  // Generator schematic board with visible branches.
            case 13: // Cargo manifest, barcode strips and shipping labels.
                glColor3f(index == 3 ? .18f : .49f, .35f, .29f);
                Box(0, 1.15f, .12f, .94f, .49f, .05f);
                glColor3f(.67f, .75f, .64f);
                for (int i = 0; i < 6; ++i)
                {
                    Box(-.26f + i * .1f, 1.18f, .086f, .025f, index == 3 ? .27f : .12f, .012f);
                }
                Box(0, 1.28f, .08f, .64f, .018f, .018f);
                Box(0, 1.05f, .08f, .64f, .018f, .018f);
                break;
            case 4: // Exposed coolant manifold; the sleeve seats as the repair progresses.
                for (float x : {-.4f, 0.f, .4f})
                {
                    glColor3f(.27f, .48f, .41f);
                    Cylinder(x, .96f, 0, .075f, .55f);
                    glColor3f(.58f, .6f, .51f);
                    Cylinder(x, 1.14f + (1 - progress) * .16f, 0, .11f, .13f);
                }
                glColor3f(.47f, .16f, .11f);
                if (progress < 1)
                {
                    Box(0, 1.25f, -.1f, .7f * (1 - progress), .045f, .06f);
                }
                break;
            case 5:  // Breaker cabinet with a lever instead of a flat button.
            case 11: // Research air damper: slats close visibly.
                Box(0, 1.38f, .08f, .56f, .82f, .24f);
                if (index == 5)
                {
                    glPushMatrix();
                    glTranslatef(0, 1.35f, -.1f);
                    glRotatef(-45 + progress * 90, 1, 0, 0);
                    glColor3f(.64f, .43f, .12f);
                    Box(0, .17f, 0, .06f, .35f, .07f);
                    Box(0, .34f, 0, .3f, .075f, .1f);
                    glPopMatrix();
                }
                else
                {
                    for (int i = 0; i < 5; ++i)
                    {
                        glPushMatrix();
                        glTranslatef(0, 1.14f + i * .12f, -.06f);
                        glRotatef((1 - progress) * 65, 1, 0, 0);
                        glColor3f(.57f, .64f, .66f);
                        Box(0, 0, 0, .47f, .11f, .028f);
                        glPopMatrix();
                    }
                }
                break;
            case 6: // Medical archive: terminal and a recognizable medical cross.
                Monitor(0, 1.29f, .1f, powered);
                glColor3f(.75f, .8f, .78f);
                Box(.37f, 1.11f, -.14f, .21f, .05f, .03f);
                Box(.37f, 1.11f, -.14f, .05f, .21f, .03f);
                break;
            case 7: // Crew record is inside a pulled-out bedside drawer.
                glColor3f(.35f, .29f, .24f);
                Box(0, 1.04f, .14f, .86f, .16f, .34f);
                Box(-.4f, 1.16f, 0, .06f, .28f, .65f);
                Box(.4f, 1.16f, 0, .06f, .28f, .65f);
                glColor3f(.51f, .44f, .34f);
                Box(0, 1.06f, -.1f - progress * .35f, .71f, .06f, .5f);
                Box(0, 1.17f, -.36f - progress * .35f, .76f, .22f, .04f);
                glColor3f(.74f, .78f, .68f);
                Box(0, 1.11f, -.1f - progress * .35f, .38f, .02f, .26f);
                break;
            case 8: // Flat credential with a stripe and lanyard, not a spinning cube.
                glColor3f(.77f, .8f, .72f);
                Box(0, .72f, -.08f, .22f, .035f, .34f);
                glColor3f(.13f, .4f, .55f);
                Box(0, .742f, -.08f, .2f, .012f, .08f);
                glColor3f(.35f, .43f, .46f);
                Box(0, .71f, .18f, .03f, .02f, .2f);
                break;
            case 9: // Command: a tabletop orbital schematic and three displays.
                for (float x : {-.38f, .38f})
                {
                    Monitor(x, 1.38f, .18f, powered);
                }
                glPushMatrix();
                glTranslatef(0, 1.03f, -.1f);
                glRotatef(90, 1, 0, 0);
                glColor3f(.16f, .65f, .7f);
                glutSolidTorus(.014, .25, 6, 24);
                glutSolidTorus(.014, .14, 6, 24);
                glPopMatrix();
                break;
            case 10:
            case 18: // Escape craft: three sockets fill as modules are installed.
                Monitor(0, 1.45f, .15f, done);
                for (int i = 0; i < 3; ++i)
                {
                    glColor3f(.13f, .18f, .21f);
                    Box(-.32f + i * .32f, 1.1f, -.04f, .27f, .22f, .3f);
                    if (progress * 3 > i)
                    {
                        glColor3f(.32f, .61f, .53f);
                        Box(-.32f + i * .32f, 1.13f, -.06f, .19f, .17f, .2f);
                    }
                }
                break;
            case 12: // Research navigation unit: antenna and protected electronics.
                glColor3f(.62f, .67f, .7f);
                Box(0, 1.12f, 0, .45f, .22f, .4f);
                Cylinder(.14f, 1.23f, .1f, .014f, .3f);
                glColor3f(.15f, .63f, .7f);
                Box(0, 1.13f, -.22f, .25f, .08f, .025f);
                break;
            case 14: // Cargo winch control and tension drum.
                glPushMatrix();
                glTranslatef(0, 1.22f, 0);
                glRotatef(progress * 540, 0, 0, 1);
                glColor3f(.67f, .44f, .12f);
                glutSolidTorus(.035, .24, 8, 20);
                Box(0, 0, 0, .45f, .04f, .05f);
                glPopMatrix();
                break;
            case 15: // Heavy removable power module with a carrying handle.
                glColor3f(.52f, .43f, .16f);
                Box(0, 1.04f, 0, .5f, .42f, .4f);
                glColor3f(.16f, .2f, .23f);
                Box(-.17f, 1.31f, 0, .04f, .15f, .06f);
                Box(.17f, 1.31f, 0, .04f, .15f, .06f);
                Box(0, 1.37f, 0, .38f, .04f, .06f);
                for (int i = 0; i < 4; ++i)
                {
                    Box(-.16f + i * .1f, 1.03f, -.21f, .045f, .24f, .02f);
                }
                break;
            case 16: // Docking coupler rotates loose from the service connection.
                glColor3f(.34f, .42f, .45f);
                Cylinder(0, .95f, 0, .12f, .5f);
                glPushMatrix();
                glTranslatef(0, 1.17f + progress * .16f, 0);
                glRotatef(progress * 270, 0, 1, 0);
                glRotatef(90, 1, 0, 0);
                glColor3f(.68f, .43f, .13f);
                glutSolidTorus(.055, .18, 6, 8);
                glPopMatrix();
                break;
            case 17: // Sealed fuel cylinder with a cap and hazard band.
                glColor3f(.43f, .5f, .47f);
                Cylinder(0, .88f, 0, .19f, .55f);
                glColor3f(.72f, .43f, .08f);
                Cylinder(0, 1.1f, 0, .195f, .1f);
                glColor3f(.2f, .25f, .27f);
                Cylinder(0, 1.43f, 0, .085f, .065f);
                break;
            case 19: // Air purge filters and fan housing.
                for (float x : {-.22f, .22f})
                {
                    glColor3f(.58f, .65f, .6f);
                    Cylinder(x, .98f, 0, .12f, .4f);
                }
                glColor3f(.13f, .48f, .45f);
                if (progress > 0)
                {
                    Box(0, 1.1f, -.2f, .7f * progress, .045f, .04f);
                }
                break;
            case 20: // External locking claw retracts rather than changing a button's color.
                glColor3f(.68f, .45f, .14f);
                for (int side : {-1, 1})
                {
                    float x = side * (.12f + progress * .18f);
                    Box(x, 1.2f, 0, .1f, .42f, .15f);
                    Box(x, 1.4f, -.06f, .18f, .1f, .25f);
                }
                break;
            case 21:
                Monitor(0, 1.3f, .1f, powered);
                glColor3f(.64f, .38f, .12f);
                Box(-.27f, 1.09f, -.15f, .06f, .15f, .05f);
                Box(.27f, 1.09f, -.15f, .06f, .15f, .05f);
                break;
            }
            glPopMatrix();
        }
    } // namespace Equipment
} // namespace Station
