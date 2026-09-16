#pragma once
#include <string>
#include <vector>
#include <array>
#include <cstdio>
#include "StationLayout.h"
#include "Dependencies/freeglut.h"

namespace Station
{
    struct Task
    {
        const char* label;
        const char* detail;
        Point p;
        float hold;
        int room;
        bool pickup;
    };

    class Mission
    {
        std::array<bool, 22> done = {};
        std::array<float, 22> progress = {};
        std::vector<float> doorSlide;
        bool previousE = false;
        int focus = -1;
        float messageTime = 0, damperTime = 0, elapsed = 0;
        std::string message;

        struct Steam
        {
            float x, y, z, age, life, vx, vz;
        };

        std::vector<Steam> steam;
        unsigned rng = 8823;
        float emissionClock = 0;

        float Random()
        {
            rng = rng * 1664525u + 1013904223u;
            return float(rng >> 8) / 16777216.f;
        }

        bool Ready(int i) const
        {
            switch (i)
            {
            case 0:
                return true;
            case 1:
                return done[0];
            case 2:
                return done[1];
            case 3:
                return done[2];
            case 4:
                return done[3];
            case 5:
                return done[4];
            case 6:
                return done[5];
            case 7:
                return done[6];
            case 8:
                return done[7];
            case 9:
                return done[5] && done[8];
            case 10:
                return true;
            case 11:
                return done[9];
            case 12:
                return done[11];
            case 13:
                return done[9];
            case 14:
                return done[13];
            case 15:
                return done[14];
            case 16:
                return done[9];
            case 17:
                return done[16];
            case 18:
                return done[10] && done[12] && done[15] && done[17];
            case 19:
            case 20:
                return done[18];
            case 21:
                return done[19] && done[20];
            default:
                return false;
            }
        }

        void Complete(int i)
        {
            done[i] = true;
            progress[i] = 1;
            message = tasks[i].detail;
            messageTime = 9;
            log.push_back(message);
            if (log.size() > 8)
            {
                log.erase(log.begin());
            }
            if (i == 11)
            {
                damperTime = 30;
            }
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
        std::vector<Task> tasks;
        std::vector<std::string> log;

        Mission(Layout& layout)
        {
            tasks = {{"READ PRESSURE GAUGE",
                      "Pressure imbalance: isolate valve A, then balance valve B.",
                      {11, 65},
                      0,
                      0,
                      false},
                     {"ISOLATE VALVE A",
                      "Valve A isolated. Balance valve B on the other side of the room.",
                      {21, 61},
                      1.2f,
                      0,
                      false},
                     {"BALANCE VALVE B",
                      "Pressure stable. Life-support emergency door released. Find the generator.",
                      {21, 72},
                      1.5f,
                      0,
                      false},
                     {"READ POWER DIAGRAM",
                      "Cooling loop first, then main breaker. Controls are separated for crew operation.",
                      {17, 128},
                      0,
                      1,
                      false},
                     {"REPAIR COOLING LOOP",
                      "Cooling loop repaired. The main breaker can now be engaged.",
                      {27, 143},
                      2.5f,
                      1,
                      false},
                     {"ENGAGE MAIN BREAKER",
                      "Emergency power online. Medical archive is available.",
                      {12, 143},
                      0,
                      1,
                      false},
                     {"READ MEDICAL ARCHIVE",
                      "Technician I. Han was transferred to habitation berth C-07. Check the crew log.",
                      {181, 68},
                      0,
                      2,
                      false},
                     {"READ CREW LOG C-07",
                      "C-07: technician last seen near the covered casualty in the southeast corner.",
                      {182, 121},
                      0,
                      3,
                      false},
                     {"RECOVER ACCESS CARD",
                      "Engineering access recovered. Command bulkheads are unlocked.",
                      {191, 130},
                      0,
                      3,
                      true},
                     {"CHECK STATION STATUS",
                      "Escape craft detected northeast. Inspect it, then recover navigation, power and fuel "
                      "modules.",
                      {100, 96},
                      0,
                      4,
                      false},
                     {"INSPECT ESCAPE CRAFT",
                      "Craft offline: navigation unit, B-12 power module and fuel coupler required.",
                      {180, 23},
                      0,
                      8,
                      false},
                     {"ISOLATE RESEARCH DAMPER",
                      "Research vent branch isolated for 30 seconds. Navigation unit released.",
                      {96, 10},
                      2,
                      5,
                      false},
                     {"TAKE NAVIGATION UNIT",
                      "Navigation unit secured. Remaining components are in cargo and docking.",
                      {104, 20},
                      0,
                      5,
                      true},
                     {"READ CARGO MANIFEST",
                      "Compatible power module: B-12. Use the cargo lift to release its transport cradle.",
                      {54, 175},
                      0,
                      6,
                      false},
                     {"OPERATE CARGO LIFT",
                      "B-12 cradle lowered. Retrieve the power module from the platform.",
                      {66, 173},
                      3,
                      6,
                      false},
                     {"TAKE B-12 POWER MODULE", "B-12 power module secured.", {63, 183}, 0, 6, true},
                     {"RELEASE DOCK COUPLER",
                      "Service-craft fuel coupler released. Collect the sealed fuel cell.",
                      {110, 180},
                      2,
                      7,
                      false},
                     {"TAKE FUEL CELL",
                      "Fuel cell secured. Return to the northeast escape craft.",
                      {90, 190},
                      0,
                      7,
                      true},
                     {"INSTALL ESCAPE COMPONENTS",
                      "All components installed. Purge the cabin and release the external clamp.",
                      {180, 23},
                      2,
                      8,
                      false},
                     {"PURGE ESCAPE CABIN",
                      "Cabin pressure stable. Finish external checks before boarding.",
                      {173, 30},
                      2,
                      8,
                      false},
                     {"RELEASE ESCAPE CLAMP",
                      "External clamp released. Board the escape craft and initiate launch.",
                      {189, 30},
                      2,
                      8,
                      false},
                     {"BOARD AND LAUNCH",
                      "Escape sequence complete. Local station prototype finished.",
                      {183, 18},
                      1.5f,
                      8,
                      false}};
            doorSlide.assign(layout.doors.size(), 0);
            for (int i = 0; i < int(tasks.size()); ++i)
            {
                if (i == 18)
                {
                    continue;
                }
                const auto& t = tasks[i];
                layout.AddBox(t.p.x, .48f, t.p.z, .7f, .96f, .7f, 3, true);
            }
            message =
                "You wake in life support. Read the pressure gauge. Tab equips tools; left-click uses them.";
            messageTime = 12;
            log.push_back(message);
        }

        bool Power() const
        {
            return done[5];
        }

        bool Card() const
        {
            return done[8];
        }

        bool Won() const
        {
            return done[21];
        }

        bool DamperClosed() const
        {
            return damperTime > 0;
        }

        bool Done(int i) const
        {
            return done[i];
        }

        int NearestActionable(Point p) const
        {
            int result = ObjectiveIndex();
            float best = 10000;
            for (int i = 0; i < int(tasks.size()); ++i)
            {
                if (!done[i] && Ready(i))
                {
                    float d = Distance(p, tasks[i].p);
                    if (d < best)
                    {
                        best = d;
                        result = i;
                    }
                }
            }
            return result;
        }

        void Reset()
        {
            done.fill(false);
            progress.fill(0);
            for (auto& slide : doorSlide)
            {
                slide = 0;
            }
            previousE = false;
            focus = -1;
            damperTime = 0;
            steam.clear();
            emissionClock = 0;
            elapsed = 0;
            log.clear();
            message = "You wake in life support. Read the pressure gauge.";
            messageTime = 10;
            log.push_back(message);
        }

        float DoorAmount(int i) const
        {
            return doorSlide[i];
        }

        int Target() const
        {
            return focus;
        }

        float Progress() const
        {
            return focus >= 0 ? progress[focus] : 0;
        }

        const char* Message() const
        {
            return messageTime > 0 ? message.c_str() : "";
        }

        int ObjectiveIndex() const
        {
            const int order[] = {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10,
                                 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21};
            for (int i : order)
            {
                if (!done[i])
                {
                    return i;
                }
            }
            return 21;
        }

        const char* Objective() const
        {
            return Won() ? "ESCAPED" : tasks[ObjectiveIndex()].label;
        }

        std::string Prompt() const
        {
            if (focus < 0 || Won())
            {
                return "";
            }
            if (done[focus])
            {
                return std::string("E REVIEW / ") + tasks[focus].label;
            }
            if (!Ready(focus))
            {
                return "LOCKED / COMPLETE THE CURRENT OBJECTIVE FIRST";
            }
            return std::string(tasks[focus].hold > 0 ? "HOLD E / " : "E / ") + tasks[focus].label;
        }

        bool Blocks(const Layout& layout, Point p, float radius = .28f) const
        {
            for (int i = 0; i < int(layout.doors.size()); ++i)
            {
                if (doorSlide[i] > .94f)
                {
                    continue;
                }
                const auto& d = layout.doors[i];
                float dx = p.x - d.p.x, dz = p.z - d.p.z, c = std::cos(d.angle), s = std::sin(d.angle);
                if (std::fabs(dx * c + dz * s) < .18f + radius &&
                    std::fabs(-dx * s + dz * c) < d.width * .5f + radius)
                {
                    return true;
                }
            }
            return false;
        }

        bool Visible(const Layout& layout, Point eye, Point target, float targetY) const
        {
            if (!layout.Visible(eye, 1.65f, target, targetY))
            {
                return false;
            }
            int count = int(std::ceil(Distance(eye, target) / .12f));
            for (int i = 1; i < count; ++i)
            {
                float t = float(i) / count;
                if (Blocks(layout, {eye.x + (target.x - eye.x) * t, eye.z + (target.z - eye.z) * t}, 0))
                {
                    return false;
                }
            }
            return true;
        }

        void Update(Layout& layout, float dt, Point player, float yaw, float pitch, bool e, bool repairTool,
                    bool active)
        {
            elapsed += dt;
            messageTime = (std::max)(0.f, messageTime - dt);
            damperTime = (std::max)(0.f, damperTime - dt);
            for (int i = 0; i < int(layout.doors.size()); ++i)
            {
                int access = layout.doors[i].access;
                bool released = access == 0
                                    ? done[2]
                                    : (access == 1 ? (done[5] && done[8]) : (access == 2 ? done[9] : false));
                doorSlide[i] = Clamp(doorSlide[i] + (released ? dt : -dt), 0, 1);
            }
            focus = -1;
            float nearest = 2.6f;
            float a = yaw * Pi / 180, b = pitch * Pi / 180;
            if (active && !Won())
            {
                for (int i = 0; i < int(tasks.size()); ++i)
                {
                    if (tasks[i].pickup && done[i])
                    {
                        continue;
                    }
                    if (i == 10 && done[10])
                    {
                        continue;
                    }
                    if (i == 18 && !done[10])
                    {
                        continue;
                    }
                    const auto& task = tasks[i];
                    float dx = task.p.x - player.x, dz = task.p.z - player.z, dy = 1.22f - 1.65f;
                    float dist = std::sqrt(dx * dx + dy * dy + dz * dz);
                    if (dist < nearest && dist > .01f &&
                        (dx * std::sin(a) * std::cos(b) - dy * std::sin(b) - dz * std::cos(a) * std::cos(b)) /
                                dist >
                            .82f &&
                        Visible(layout, player, task.p, 1.22f))
                    {
                        nearest = dist;
                        focus = i;
                    }
                }
            }
            bool pressed = e && !previousE;
            previousE = e;
            if (active && focus >= 0)
            {
                if (done[focus] && pressed)
                {
                    message = tasks[focus].detail;
                    messageTime = 9;
                }
                else if (!done[focus] && Ready(focus))
                {
                    if (tasks[focus].hold <= 0 && pressed)
                    {
                        Complete(focus);
                    }
                    else if (tasks[focus].hold > 0 && (e || repairTool))
                    {
                        progress[focus] = Clamp(progress[focus] + dt / tasks[focus].hold, 0, 1);
                        if (progress[focus] >= 1)
                        {
                            Complete(focus);
                        }
                    }
                }
            }
            if (!done[2])
            {
                emissionClock += dt * 35;
                while (emissionClock >= 1)
                {
                    emissionClock -= 1;
                    if (steam.size() < 180)
                    {
                        steam.push_back(
                            {23, 1.6f, 69, 0, 1.3f + Random(), (Random() - .5f) * .5f, .6f + Random()});
                    }
                }
            }
            for (auto& p : steam)
            {
                p.age += dt;
                p.x += p.vx * dt;
                p.z += p.vz * dt;
                p.y += dt * .4f;
            }
            steam.erase(std::remove_if(steam.begin(), steam.end(),
                                       [](const Steam& p)
                                       {
                                           return p.age >= p.life;
                                       }),
                        steam.end());
        }

        void Draw(bool hdr, MetalMaterial& material, const Layout& layout)
        {
            if (hdr)
            {
                material.Use(.65f, .45f, 0, true);
            }
            else
            {
                glEnable(GL_LIGHTING);
            }
            for (int i = 0; i < int(layout.doors.size()); ++i)
            {
                const auto& door = layout.doors[i];
                glPushMatrix();
                glTranslatef(door.p.x, 0, door.p.z);
                glRotatef(-door.angle * 180 / Pi, 0, 1, 0);
                glColor3f(.32f, .36f, .38f);
                // Local X is the corridor axis; panels slide along local Z into side pockets.
                for (int side : {-1, 1})
                {
                    Cube(0, 1.6f, side * (door.width * .25f + doorSlide[i] * door.width * .55f), .25f, 3.2f,
                         door.width * .5f);
                }
                glColor3f(.6f, .4f, .12f);
                Cube(0, 3.35f, 0, .4f, .25f, door.width + .3f);
                if (hdr)
                {
                    material.Use(.1f, .4f, 2, true);
                }
                glColor3f(doorSlide[i] > .94f ? .1f : .8f, doorSlide[i] > .94f ? .65f : .15f, .08f);
                Cube(.23f, 2.65f, 0, .05f, .12f, 1.1f);
                if (hdr)
                {
                    material.Use(.65f, .45f, 0, true);
                }
                glPopMatrix();
            }
            for (int i = 0; i < int(tasks.size()); ++i)
            {
                if (i == 18)
                {
                    continue;
                }
                const auto& task = tasks[i];
                if (task.pickup)
                {
                    if (done[i])
                    {
                        continue;
                    }
                    if (hdr)
                    {
                        material.Use(.55f, .3f, 0, true);
                    }
                    glColor3f(.62f, .69f, .64f);
                    glPushMatrix();
                    glTranslatef(task.p.x, 1.16f, task.p.z);
                    glRotatef(elapsed * 20, 0, 1, 0);
                    glutSolidCube(.26);
                    glPopMatrix();
                }
                else if (i == 1 || i == 2)
                {
                    if (hdr)
                    {
                        material.Use(.65f, .4f, 0, true);
                    }
                    glColor3f(.55f, .14f, .09f);
                    glPushMatrix();
                    glTranslatef(task.p.x, 1.22f, task.p.z);
                    glRotatef(progress[i] * 180, 0, 0, 1);
                    glutSolidTorus(.035, .23, 8, 20);
                    Cube(0, 0, 0, .44f, .045f, .05f);
                    Cube(0, 0, 0, .045f, .44f, .05f);
                    glPopMatrix();
                }
                else
                {
                    if (hdr)
                    {
                        material.Use(.1f, .4f, 2, true);
                    }
                    glColor3f(done[i] ? .12f : .12f, done[i] ? .68f : .38f, done[i] ? .4f : .68f);
                    Cube(task.p.x, 1.07f, task.p.z, .56f, .16f, .5f);
                }
            }
            // The cargo cradle descends as work progresses, even if the operator pauses.
            if (hdr)
            {
                material.Use(.65f, .5f, 0, true);
            }
            glColor3f(.38f, .32f, .19f);
            Cube(63, .8f + (1 - progress[14]) * 1.8f, 184, 1.4f, 1, 1.3f);
            if (DamperClosed())
            {
                if (hdr)
                {
                    material.Use(.8f, .4f, 0, true);
                }
                glColor3f(.38f, .29f, .19f);
                Cube(100, 4.4f, 15, 1.4f, .08f, 1.4f);
            }
            glUseProgram(0);
        }

        void DrawSteam()
        {
            glUseProgram(0);
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            float m[16];
            glGetFloatv(GL_MODELVIEW_MATRIX, m);
            std::sort(steam.begin(), steam.end(),
                      [&](const Steam& a, const Steam& b)
                      {
                          return m[2] * a.x + m[6] * a.y + m[10] * a.z <
                                 m[2] * b.x + m[6] * b.y + m[10] * b.z;
                      });
            for (const auto& p : steam)
            {
                float t = p.age / p.life, r = .12f + t * .45f;
                glBegin(GL_TRIANGLE_FAN);
                glColor4f(.35f, .56f, .52f, std::sin(t * Pi) * .25f);
                glVertex3f(p.x, p.y, p.z);
                glColor4f(.2f, .35f, .3f, 0);
                for (int i = 0; i <= 16; ++i)
                {
                    float a = i * 2 * Pi / 16, x = std::cos(a) * r, y = std::sin(a) * r;
                    glVertex3f(p.x + m[0] * x + m[1] * y, p.y + m[4] * x + m[5] * y,
                               p.z + m[8] * x + m[9] * y);
                }
                glEnd();
            }
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
            glColor4f(1, 1, 1, 1);
        }
    };
} // namespace Station
