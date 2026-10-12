#include "stdafx.h"
#include "GameUI.h"

namespace Game
{
    namespace UI
    {
        void Screen()
        {
            glUseProgram(0);
            glDisable(GL_LIGHTING);
            glDisable(GL_FOG);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_CULL_FACE);
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glOrtho(0, 1280, 720, 0, -1, 1);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        }

        void Rect(float x, float y, float w, float h, float r, float g, float b, float a)
        {
            glColor4f(r, g, b, a);
            glBegin(GL_QUADS);
            glVertex2f(x, y);
            glVertex2f(x + w, y);
            glVertex2f(x + w, y + h);
            glVertex2f(x, y + h);
            glEnd();
        }

        void Text(float x, float y, const char* text, float r, float g, float b)
        {
            glColor3f(r, g, b);
            KoreanText::Draw(x, y, text);
        }

        void Center(float x, float y, const char* text)
        {
            Text(x - KoreanText::Width(text) * .5f, y, text);
        }

        void Wrap(float x, float y, const char* text, float width)
        {
            glColor3f(.73f, .85f, .86f);
            glRasterPos2f(x, y);
            float used = 0;
            for (wchar_t character : KoreanText::Decode(text))
            {
                const auto& g = KoreanText::Get(character);
                if (character == L'\n' || used + g.width > width)
                {
                    y += 22;
                    used = 0;
                    glRasterPos2f(x, y);
                }
                if (character == L'\n')
                {
                    continue;
                }
                if (g.list)
                {
                    glCallList(g.list);
                }
                else
                {
                    glBitmap(0, 0, 0, 0, float(g.width), 0, nullptr);
                }
                used += g.width;
            }
        }

        void Ring(float x, float y, float radius, float fraction, float r, float g, float b)
        {
            glColor3f(r, g, b);
            glLineWidth(3);
            glBegin(GL_LINE_STRIP);
            for (int i = 0; i <= 64; ++i)
            {
                float t = i / 64.f;
                if (t > fraction)
                {
                    break;
                }
                float a = t * 2 * Pi - Pi * .5f;
                glVertex2f(x + std::cos(a) * radius, y + std::sin(a) * radius);
            }
            glEnd();
            glLineWidth(1);
        }

        void Camera(int width, int height, Vec3 eye, float yaw, float pitch)
        {
            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            double aspect = double(width) / (std::max)(height, 1);
            glFrustum(-.07 * aspect, .07 * aspect, -.07, .07, .1, 250);
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glRotatef(pitch, 1, 0, 0);
            glRotatef(yaw, 0, 1, 0);
            glTranslatef(-eye.x, -eye.y, -eye.z);
        }

        void Lights(bool power, float clock)
        {
            glEnable(GL_DEPTH_TEST);
            glEnable(GL_LIGHTING);
            glEnable(GL_NORMALIZE);
            glEnable(GL_COLOR_MATERIAL);
            glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
            GLfloat ambient[] = {.25f, .3f, .34f, 1};
            glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambient);
            const Vec3 positions[] = {{0, 3, 0},   {-87, 3, 20}, {55, 3, -18},
                                      {0, 3, -65}, {0, 3, 70},   {73, 3, -73}};
            for (int i = 0; i < 6; ++i)
            {
                float intensity = power ? .8f : .35f;
                if (std::sin(clock * 3 + i) > .96f)
                {
                    intensity *= .3f;
                }
                GLfloat p[] = {positions[i].x, positions[i].y, positions[i].z, 1},
                        c[] = {intensity * .65f, intensity * .84f, intensity, 1};
                glEnable(GL_LIGHT0 + i);
                glLightfv(GL_LIGHT0 + i, GL_POSITION, p);
                glLightfv(GL_LIGHT0 + i, GL_DIFFUSE, c);
                glLightf(GL_LIGHT0 + i, GL_QUADRATIC_ATTENUATION, .025f);
            }
        }

        void Radar(const Mesh& mesh, const StationWorld& world, Vec3 player, Vec3 target, bool objective,
                   float x, float y, float scale, Vec3 center)
        {
            GLint viewport[4];
            glGetIntegerv(GL_VIEWPORT, viewport);
            glEnable(GL_SCISSOR_TEST);
            glScissor(int((x - 260) / 1280 * viewport[2]), int((720 - y - 260) / 720 * viewport[3]),
                      int(520.f / 1280 * viewport[2]), int(520.f / 720 * viewport[3]));
            Rect(x - 260, y - 260, 520, 520, .015f, .045f, .06f, .97f);
            glColor4f(.13f, .3f, .33f, .45f);
            glBegin(GL_LINES);
            for (int i = -10; i <= 10; ++i)
            {
                glVertex2f(x + i * 25, y - 250);
                glVertex2f(x + i * 25, y + 250);
                glVertex2f(x - 250, y + i * 25);
                glVertex2f(x + 250, y + i * 25);
            }
            glEnd();
            auto plot = [&](Vec3 p)
            {
                glVertex2f(x + (p.x - center.x) * scale, y + (p.z - center.z) * scale);
            };
            glColor3f(.12f, .34f, .4f);
            glBegin(GL_TRIANGLES);
            for (const auto& t : mesh.triangles)
            {
                if (t.group.find("Floor_") == 0 && std::fabs(t.a.y) < .01f && std::fabs(t.b.y) < .01f &&
                    std::fabs(t.c.y) < .01f)
                {
                    plot(t.a);
                    plot(t.b);
                    plot(t.c);
                }
            }
            glEnd();
            for (const auto& room : world.rooms)
            {
                Text(x + (room.center.x - center.x) * scale - 22, y + (room.center.z - center.z) * scale,
                     room.name);
            }
            glColor3f(1, 1, 1);
            glPointSize(6);
            glBegin(GL_POINTS);
            plot(player);
            glEnd();
            glPointSize(1);
            if (objective)
            {
                float tx = x + (target.x - center.x) * scale, ty = y + (target.z - center.z) * scale;
                glColor3f(.96f, .73f, .12f);
                glBegin(GL_LINE_LOOP);
                glVertex2f(tx, ty - 7);
                glVertex2f(tx + 7, ty);
                glVertex2f(tx, ty + 7);
                glVertex2f(tx - 7, ty);
                glEnd();
            }
            glDisable(GL_SCISSOR_TEST);
        }

    } // namespace UI
} // namespace Game
