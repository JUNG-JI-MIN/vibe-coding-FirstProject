#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include "MetalMaterial.h"

namespace Station
{
    constexpr float Pi = 3.14159265359f;

    inline float Clamp(float v, float a, float b)
    {
        return v < a ? a : (v > b ? b : v);
    }

    struct Point
    {
        float x, z;
    };

    constexpr float LifeSupportPipeHeight = 3.35f;
    constexpr float LifeSupportPipeHalfSize = .065f;
    constexpr Point LifeSupportLeak = {23, 73};

    inline float Distance(Point a, Point b)
    {
        float x = a.x - b.x, z = a.z - b.z;
        return std::sqrt(x * x + z * z);
    }

    inline Point Polar(float radius, float angle)
    {
        return {100 + radius * std::cos(angle), 100 + radius * std::sin(angle)};
    }

    inline float SegmentDistance(Point p, Point a, Point b)
    {
        float dx = b.x - a.x, dz = b.z - a.z;
        float t = Clamp(((p.x - a.x) * dx + (p.z - a.z) * dz) / (dx * dx + dz * dz + .000001f), 0, 1);
        return Distance(p, {a.x + dx * t, a.z + dz * t});
    }

    struct Room
    {
        const char* name;
        const char* code;
        float x0, z0, x1, z1;
        int style;
        Point center;
    };

    struct Passage
    {
        Point a, b;
        float halfWidth;
    };

    struct Vertex
    {
        float x, y, z, nx, ny, nz;
    };

    struct Material
    {
        float r, g, b, metal, rough, glow;
    };

    struct Box
    {
        float x, y, z, w, h, d;
        int material;
        bool solid;
        bool visible = true;
    };

    struct Lamp
    {
        float x, y, z, r, g, b;
    };

    struct Edge
    {
        Point a, b;
    };

    struct VentNode
    {
        Point p;
        bool grate;
    };

    struct VentLink
    {
        int a, b;
        bool research;
    };

    struct Door
    {
        Point p;
        float angle, width;
        int access;
        const char* name;
        bool automatic = false;

        Point Local(Point world) const
        {
            float dx = world.x - p.x, dz = world.z - p.z;
            float c = std::cos(angle), s = std::sin(angle);
            return {dx * c + dz * s, -dx * s + dz * c};
        }

        float PanelCenter(int side, float amount) const
        {
            return side * (width * .25f + amount * width * .55f);
        }

        bool Blocks(Point world, float radius, float amount) const
        {
            Point local = Local(world);
            if (std::fabs(local.x) >= .125f + radius)
            {
                return false;
            }
            for (int side : {-1, 1})
            {
                if (std::fabs(local.z - PanelCenter(side, amount)) < width * .25f + radius)
                {
                    return true;
                }
            }
            return false;
        }
    };

    struct Batch
    {
        std::vector<Vertex> vertices;
        GLuint buffer = 0;
    };

    class Layout
    {
        std::array<Batch, 16> batches;
        std::vector<std::vector<int>> obstacleGrid;
        static constexpr int Grid = 40;

        void Quad(int mat, Point a, Point b, float bottom, float top, float nx, float nz)
        {
            Vertex v[6] = {{a.x, bottom, a.z, nx, 0, nz}, {b.x, bottom, b.z, nx, 0, nz},
                           {b.x, top, b.z, nx, 0, nz},    {a.x, bottom, a.z, nx, 0, nz},
                           {b.x, top, b.z, nx, 0, nz},    {a.x, top, a.z, nx, 0, nz}};
            batches[mat].vertices.insert(batches[mat].vertices.end(), v, v + 6);
        }

        void Plane(int mat, Point a, Point b, Point c, float y, float normal)
        {
            Vertex v[3] = {
                {a.x, y, a.z, 0, normal, 0}, {b.x, y, b.z, 0, normal, 0}, {c.x, y, c.z, 0, normal, 0}};
            batches[mat].vertices.insert(batches[mat].vertices.end(), v, v + 3);
        }

        void Tube(Point a, Point b, float y, float radius, int mat)
        {
            float length = Distance(a, b);
            if (length < .001f)
            {
                return;
            }
            float nx = -(b.z - a.z) / length, nz = (b.x - a.x) / length;
            for (int i = 0; i < 10; ++i)
            {
                float p = i * 2 * Pi / 10, q = (i + 1) * 2 * Pi / 10;
                auto vertex = [&](Point center, float angle)
                {
                    float c = std::cos(angle), s = std::sin(angle);
                    return Vertex{center.x + nx * c * radius,
                                  y + s * radius,
                                  center.z + nz * c * radius,
                                  nx * c,
                                  s,
                                  nz * c};
                };
                Vertex v[6] = {vertex(a, p), vertex(b, p), vertex(b, q),
                               vertex(a, p), vertex(b, q), vertex(a, q)};
                batches[mat].vertices.insert(batches[mat].vertices.end(), v, v + 6);
            }
        }

        void Wall(Point a, Point b)
        {
            Point mid = {(a.x + b.x) * .5f, (a.z + b.z) * .5f};
            float dx = b.x - a.x, dz = b.z - a.z, len = std::sqrt(dx * dx + dz * dz);
            if (len < .0001f)
            {
                return;
            }
            float nx = -dz / len, nz = dx / len;
            if (FloorDistance({mid.x + nx * .05f, mid.z + nz * .05f}) < 0)
            {
                nx = -nx;
                nz = -nz;
            }
            contours.push_back({a, b});
            float radial = Distance(mid, {100, 100});
            int room = RoomAt(mid), mat = room >= 0 ? rooms[room].style : ((radial < 46) ? 3 : 2);
            bool window = room < 0 && radial > 69.7f && radial < 70.3f &&
                          ((int((std::atan2(mid.z - 100, mid.x - 100) + Pi) * 24 / Pi) % 6) < 4);
            if (window)
            {
                Quad(mat, a, b, 0, 1, nx, nz);
                Quad(mat, a, b, 3.45f, 4.5f, nx, nz);
                glass.push_back({a, b});
                AddBox(a.x, 2.25f, a.z, .09f, 2.5f, .09f, 3, false);
            }
            else
            {
                Quad(mat, a, b, 0, 4.5f, nx, nz);
            }
            Quad(3, {a.x + nx * .02f, a.z + nz * .02f}, {b.x + nx * .02f, b.z + nz * .02f}, .08f, .22f, nx,
                 nz);
        }

        void Triangle(Point a, Point b, Point c)
        {
            Point input[3] = {a, b, c};
            float d[3] = {FloorDistance(a), FloorDistance(b), FloorDistance(c)};
            Point poly[6], crossings[3];
            int count = 0, cross = 0;
            for (int i = 0; i < 3; ++i)
            {
                int j = (i + 1) % 3;
                bool inside = d[i] >= 0, next = d[j] >= 0;
                if (inside)
                {
                    poly[count++] = input[i];
                }
                if (inside != next)
                {
                    float t = d[i] / (d[i] - d[j]);
                    Point p = {input[i].x + (input[j].x - input[i].x) * t,
                               input[i].z + (input[j].z - input[i].z) * t};
                    poly[count++] = p;
                    crossings[cross++] = p;
                }
            }
            if (count >= 3)
            {
                Point center = {0, 0};
                for (int i = 0; i < count; ++i)
                {
                    center.x += poly[i].x / count;
                    center.z += poly[i].z / count;
                }
                int room = RoomAt(center), mat = 0;
                if (room >= 0)
                {
                    mat = (room == 0 || room == 6) ? 1 : 0;
                }
                else if (Distance(center, {100, 100}) < 46)
                {
                    mat = 1;
                }
                bool ventOpening = false;
                for (const auto& n : vents)
                {
                    if (n.grate && std::fabs(center.x - n.p.x) < .72f && std::fabs(center.z - n.p.z) < .72f)
                    {
                        ventOpening = true;
                        break;
                    }
                }
                for (int i = 1; i < count - 1; ++i)
                {
                    Plane(mat, poly[0], poly[i], poly[i + 1], 0, 1);
                    if (!ventOpening)
                    {
                        Plane(3, poly[0], poly[i + 1], poly[i], 4.5f, -1);
                    }
                }
            }
            if (cross == 2)
            {
                Wall(crossings[0], crossings[1]);
            }
        }

        void BoxMesh(const Box& b)
        {
            Point a = {b.x - b.w * .5f, b.z - b.d * .5f}, c = {b.x + b.w * .5f, b.z + b.d * .5f};
            Point q = {c.x, a.z}, s = {a.x, c.z};
            float lo = b.y - b.h * .5f, hi = b.y + b.h * .5f;
            Quad(b.material, a, q, lo, hi, 0, -1);
            Quad(b.material, q, c, lo, hi, 1, 0);
            Quad(b.material, c, s, lo, hi, 0, 1);
            Quad(b.material, s, a, lo, hi, -1, 0);
            Plane(b.material, a, c, q, hi, 1);
            Plane(b.material, a, s, c, hi, 1);
            Plane(b.material, a, q, c, lo, -1);
            Plane(b.material, a, c, s, lo, -1);
        }

        void AddLamp(float x, float z, bool warm = false)
        {
            lamps.push_back({x, 3.85f, z, warm ? 1.f : .58f, warm ? .42f : .8f, warm ? .15f : 1.f});
            AddBox(x, 4.18f, z, 1.2f, .09f, .3f, warm ? 9 : 8, false);
        }

        void Tank(float x, float z, float bottom, float height, float radius, int material)
        {
            for (int i = 0; i < 20; ++i)
            {
                float a = i * 2 * Pi / 20, b = (i + 1) * 2 * Pi / 20;
                Point p = {x + std::cos(a) * radius, z + std::sin(a) * radius};
                Point q = {x + std::cos(b) * radius, z + std::sin(b) * radius};
                Quad(material, p, q, bottom, bottom + height, std::cos((a + b) * .5f),
                     std::sin((a + b) * .5f));
                Plane(material, {x, z}, q, p, bottom + height, 1);
                Plane(material, {x, z}, p, q, bottom, -1);
            }
        }

        void Furnish()
        {
            // Life support: green oxygen equipment and recycling pipes, localized wet trays.
            for (float z : {55.f, 77.f})
            {
                for (float x : {9.f, 13.f, 17.f})
                {
                    AddObstacle(x, 1.45f, z, 1.5f, 2.9f, 1.5f);
                    Tank(x, z, .12f, 2.6f, .62f, 4);
                    Tank(x, z, .1f, .18f, .7f, 3);
                    Tank(x, z, 2.58f, .18f, .7f, 3);
                    Tank(x, z, 2.76f, .24f, .16f, 5);
                    AddBox(x, 2.4f, z + .78f, .7f, .12f, .04f, 8, false);
                    AddBox(x, .01f, z + 1.3f, 2.2f, .02f, .7f, 10, false);
                }
            }
            for (float z : {55.f, 58.f, LifeSupportLeak.z})
            {
                AddBox(22, LifeSupportPipeHeight, z, 7, LifeSupportPipeHalfSize * 2,
                       LifeSupportPipeHalfSize * 2, 4, false);
                AddBox(24, 1.65f, z, .16f, 3.3f, .16f, 4, true);
            }
            // Generator: machinery islands, cable trays and warning stripes.
            for (float x : {13.f, 23.f})
            {
                AddBox(x, 1.4f, 135, 4, 2.8f, 5, 3, true);
                AddBox(x, 1.65f, 137.55f, 2, .4f, .05f, 9, false);
                AddBox(x, 3.6f, 136, .2f, .2f, 20, 3, false);
                for (int rib = 0; rib < 8; ++rib)
                {
                    AddBox(x, 2.84f, 133 + rib * .5f, 3.7f, .1f, .16f, 5, false);
                }
            }
            for (int i = 0; i < 8; ++i)
            {
                AddBox(8 + i * 3, .015f, 146, 1, .03f, .28f, 7, false);
            }
            // Medical beds and glazed separation frames; readable, dry surfaces.
            for (float z : {63.f, 76.f})
            {
                for (float x : {178.f, 188.f})
                {
                    AddBox(x, .45f, z, 2.4f, .9f, 1, 5, true);
                    AddBox(x, .98f, z, 2.3f, .16f, .94f, 6, false);
                    AddBox(x - .78f, 1.13f, z, .5f, .13f, .69f, 5, false);
                    AddBox(x + 1.12f, .64f, z, .09f, .6f, 1.03f, 5, false);
                    for (float side : {-.54f, .54f})
                    {
                        AddBox(x, 1.13f, z + side, 1.45f, .065f, .05f, 5, false);
                    }
                    AddBox(x + 1.4f, 1.5f, z, .08f, 3, .08f, 5, false);
                }
            }
            // Habitation: textile bunks and personal lockers, kept clear of room entry.
            for (float z : {116.f, 133.f})
            {
                for (float x : {178.f, 184.f, 191.f})
                {
                    AddBox(x, .38f, z, 2.1f, .76f, 1, 3, true);
                    AddBox(x, .82f, z, 2, .14f, .95f, 6, false);
                    AddBox(x - .69f, .93f, z, .48f, .12f, .77f, 5, false);
                    AddBox(x + .4f, .92f, z, .72f, .07f, .9f, 2, false);
                    AddBox(x, 2.65f, z, 2.1f, .1f, 1.08f, 3, false);
                }
            }
            // Habitation lockers are hollow hiding places created by CabinetSystem.
            // A covered casualty and dropped credential: story prop, no unrelated combat system.
            AddBox(192, .28f, 128, 1.8f, .55f, .65f, 6, true);
            // Research benches leave a central aisle.
            for (float x : {91.f, 108.f})
            {
                AddBox(x, .6f, 17, 3, 1.2f, 6, 5, true);
                AddBox(x, 1.8f, 17, 1.2f, 1.2f, 1.2f, 4, true);
                for (int sample = 0; sample < 5; ++sample)
                {
                    Tank(x - .9f, 15 + sample * .55f, 1.25f, .28f, .055f, 8);
                }
            }
            // Command consoles around a central table, radial entry lanes stay open.
            AddBox(100, .7f, 100, 4, 1.4f, 3, 3, true);
            AddBox(100, 1.42f, 100, 3.6f, .04f, 2.6f, 8, false);
            for (int i = 0; i < 8; ++i)
            {
                Point p = Polar(8, (i + .5f) * Pi / 4);
                AddBox(p.x, .6f, p.z, 2, 1.2f, 1, 3, true);
                AddBox(p.x, 1.4f, p.z, 1.5f, .4f, .1f, 8, false);
            }
            // Cargo shelving on the perimeter and a lift platform.
            for (float x : {50.f, 69.f})
            {
                for (float z : {173.f, 180.f, 187.f})
                {
                    AddObstacle(x, 1.4f, z, 2, 2.8f, 3);
                    for (float side : {-1.f, 1.f})
                    {
                        for (float end : {-1.f, 1.f})
                        {
                            AddBox(x + side * .94f, 1.4f, z + end * 1.4f, .12f, 2.8f, .12f, 7, false);
                        }
                    }
                    for (float level : {.16f, 1.35f, 2.6f})
                    {
                        AddBox(x, level, z, 2.1f, .1f, 3.1f, 7, false);
                        if (level < 2)
                        {
                            for (float end : {-.74f, .74f})
                            {
                                AddBox(x, level + .4f, z + end, 1.55f, .7f, 1.18f, 2, false);
                                AddBox(x - .78f, level + .4f, z + end, .035f, .32f, .45f, 5, false);
                            }
                        }
                    }
                }
            }
            AddBox(63, .14f, 184, 4, .28f, 3, 3, true);
            // Docking bay: central service craft and overhead crane.
            AddBox(100, 1, 187, 6, 2, 10, 2, true);
            AddBox(100, 2.2f, 185, 3, .8f, 4, 5, true);
            AddBox(100, 3.6f, 185, 29, .4f, .6f, 7, false);
            Tube({104, 180}, {110, 180}, .13f, .09f, 4);
            for (int i = 0; i < 12; ++i)
            {
                AddBox(106, .025f, 177 + i * 1.6f, .65f, .035f, .28f, 7, false);
            }
            for (float x : {85.f, 115.f})
            {
                AddBox(x, 1.8f, 185, .4f, 3.6f, .4f, 3, true);
            }
            // Escape cabin: belted seats and a clear central aisle.
            for (float x : {174.f, 188.f})
            {
                for (float z : {20.f, 27.f})
                {
                    AddBox(x, .5f, z, 1.1f, 1, 1.2f, 6, true);
                    AddBox(x, 1.4f, z + .5f, 1.1f, 1.2f, .15f, 6, true);
                }
            }
        }

        int Vent(Point p, bool grate)
        {
            vents.push_back({p, grate});
            return int(vents.size()) - 1;
        }

        void BuildVents()
        {
            int inner[48], outer[48];
            for (int i = 0; i < 48; ++i)
            {
                inner[i] = Vent(Polar(38, i * Pi / 24), i % 6 == 0);
                outer[i] = Vent(Polar(66, i * Pi / 24), i % 6 == 0);
            }
            int hub = Vent({100, 100}, false), commandGrate = Vent({100, 104}, true);
            ventLinks.push_back({hub, commandGrate, false});
            for (int i = 0; i < 48; ++i)
            {
                ventLinks.push_back({inner[i], inner[(i + 1) % 48], false});
                ventLinks.push_back({outer[i], outer[(i + 1) % 48], false});
            }
            for (int i = 0; i < 48; i += 6)
            {
                ventLinks.push_back({hub, inner[i], false});
                ventLinks.push_back({inner[i], outer[i], false});
            }
            for (int i = 0; i < int(rooms.size()); ++i)
            {
                if (i == 4)
                {
                    continue;
                }
                int roomNode = Vent(i == 7 ? Point{90, 180} : rooms[i].center, true);
                Point anchor = branches[i < 4 ? i : i - 1].a;
                int branchNode = Vent(anchor, false), nearest = outer[0];
                float distance = 10000;
                for (int n : outer)
                {
                    float d = Distance(anchor, vents[n].p);
                    if (d < distance)
                    {
                        distance = d;
                        nearest = n;
                    }
                }
                ventLinks.push_back({nearest, branchNode, i == 5});
                ventLinks.push_back({branchNode, roomNode, i == 5});
            }
            for (const auto& node : vents)
            {
                if (node.grate)
                {
                    AddBox(node.p.x - .82f, 4.41f, node.p.z, .14f, .18f, 1.8f, 3, false);
                    AddBox(node.p.x + .82f, 4.41f, node.p.z, .14f, .18f, 1.8f, 3, false);
                    AddBox(node.p.x, 4.41f, node.p.z - .82f, 1.8f, .18f, .14f, 3, false);
                    AddBox(node.p.x, 4.41f, node.p.z + .82f, 1.8f, .18f, .14f, 3, false);
                    for (int j = 0; j < 6; ++j)
                    {
                        AddBox(node.p.x - .6f + j * .24f, 4.32f, node.p.z, .065f, .05f, 1.3f, 5, false);
                    }
                }
            }
            // Service network lies above the occupied deck, not across player corridors.
            for (const auto& edge : ventLinks)
            {
                Point a = vents[edge.a].p, b = vents[edge.b].p;
                float length = Distance(a, b);
                if (length < .01f)
                {
                    continue;
                }
                float nx = -(b.z - a.z) / length * .75f, nz = (b.x - a.x) / length * .75f;
                Point a0 = {a.x + nx, a.z + nz}, a1 = {a.x - nx, a.z - nz}, b0 = {b.x + nx, b.z + nz},
                      b1 = {b.x - nx, b.z - nz};
                Quad(3, a0, b0, 4.55f, 5.95f, -nx / .75f, -nz / .75f);
                Quad(3, b1, a1, 4.55f, 5.95f, nx / .75f, nz / .75f);
                Plane(3, a0, a1, b1, 5.95f, -1);
                Plane(3, a0, b1, b0, 5.95f, -1);
            }
        }

        void IndexObstacles()
        {
            obstacleGrid.resize(Grid * Grid);
            for (int i = 0; i < int(boxes.size()); ++i)
            {
                const auto& b = boxes[i];
                if (!b.solid)
                {
                    continue;
                }
                int x0 = int(Clamp((b.x - b.w * .5f) / 5, 0, 39)),
                    x1 = int(Clamp((b.x + b.w * .5f) / 5, 0, 39));
                int z0 = int(Clamp((b.z - b.d * .5f) / 5, 0, 39)),
                    z1 = int(Clamp((b.z + b.d * .5f) / 5, 0, 39));
                for (int z = z0; z <= z1; ++z)
                {
                    for (int x = x0; x <= x1; ++x)
                    {
                        obstacleGrid[z * Grid + x].push_back(i);
                    }
                }
            }
        }

    public:
        std::vector<Room> rooms;
        std::vector<Passage> passages, branches;
        std::vector<Box> boxes;
        std::vector<Lamp> lamps;
        std::vector<Edge> contours, glass;
        std::vector<VentNode> vents;
        std::vector<VentLink> ventLinks;
        std::vector<Door> doors;
        const std::array<Material, 16> materials = {{{.23f, .27f, .29f, .25f, .8f, 0},
                                                     {.17f, .20f, .21f, .8f, .52f, 0},
                                                     {.52f, .56f, .55f, .35f, .58f, 0},
                                                     {.16f, .20f, .23f, .85f, .43f, 0},
                                                     {.28f, .4f, .32f, .5f, .48f, 0},
                                                     {.58f, .64f, .65f, .7f, .28f, 0},
                                                     {.28f, .3f, .31f, 0, .93f, 0},
                                                     {.67f, .42f, .09f, .15f, .7f, 0},
                                                     {.14f, .66f, .78f, 0, .4f, 4},
                                                     {.92f, .35f, .07f, 0, .5f, 4},
                                                     {.10f, .19f, .20f, .8f, .16f, 0},
                                                     {.55f, .1f, .09f, .35f, .5f, 0},
                                                     {.26f, .3f, .32f, .6f, .4f, 0},
                                                     {.2f, .2f, .2f, 0, .7f, 0},
                                                     {.2f, .2f, .2f, 0, .7f, 0},
                                                     {.2f, .2f, .2f, 0, .7f, 0}}};

        Layout()
        {
            rooms = {{u8"생명유지실", u8"생명유지", 5, 51, 27, 81, 4, {16, 66}},
                     {u8"발전실", u8"발전", 7, 122, 33, 150, 3, {20, 136}},
                     {u8"의료실", u8"의료", 173, 59, 195, 83, 5, {184, 71}},
                     {u8"거주구", u8"거주", 174, 112, 196, 138, 2, {185, 125}},
                     {u8"관제실", u8"관제", 88, 88, 112, 112, 3, {100, 100}},
                     {u8"연구실", u8"연구", 87, 6, 113, 25, 5, {100, 15}},
                     {u8"창고", u8"창고", 47, 168, 73, 192, 3, {60, 180}},
                     {u8"도킹 베이", u8"도킹", 82, 173, 118, 198, 2, {100, 186}},
                     {u8"탈출정", u8"탈출정", 169, 14, 194, 35, 2, {182, 24}}};
            branches = {{{44, 66}, {26, 66}, 2},     {{45, 136}, {32, 136}, 2}, {{160, 72}, {174, 72}, 2},
                        {{161, 125}, {175, 125}, 2}, {{100, 33}, {100, 24}, 2}, {{65, 156}, {61, 169}, 2},
                        {{100, 165}, {100, 174}, 2}, {{147, 53}, {177, 29}, 2}};
            passages = branches;
            for (int i = 0; i < 8; ++i)
            {
                passages.push_back({{100, 100}, Polar(66, i * Pi / 4), 2});
            }
            doors.push_back({{29, 66}, 0, 4, 0, u8"압력 격벽", true});
            for (int i = 0; i < 8; ++i)
            {
                doors.push_back({Polar(13, i * Pi / 4), i * Pi / 4, 4, 1, u8"관제실 출입문", true});
            }
            doors.push_back({{100, 27}, Pi / 2, 4, 2, u8"연구실 출입문", true});
            // Damaged outer-ring sector can always be bypassed through the inner ring.
            doors.push_back({Polar(66, -Pi / 12), -Pi / 12 + Pi / 2, 8, 3, u8"파손된 격벽"});
            for (int i = 0; i < int(branches.size()); ++i)
            {
                if (i == 0 || i == 4)
                {
                    continue; // These entrances already have progression-controlled doors.
                }
                const auto& path = branches[i];
                const auto& room = rooms[i < 4 ? i : i + 1];
                float outside = 0, inside = 1;
                // Find the chamfered room boundary, then place the door in the narrow connector.
                for (int step = 0; step < 20; ++step)
                {
                    float t = (outside + inside) * .5f;
                    Point p = {path.a.x + (path.b.x - path.a.x) * t, path.a.z + (path.b.z - path.a.z) * t};
                    float dx = std::fabs(p.x - (room.x0 + room.x1) * .5f);
                    float dz = std::fabs(p.z - (room.z0 + room.z1) * .5f);
                    float hx = (room.x1 - room.x0) * .5f, hz = (room.z1 - room.z0) * .5f;
                    if (dx <= hx && dz <= hz && dx + dz <= hx + hz - 2)
                    {
                        inside = t;
                    }
                    else
                    {
                        outside = t;
                    }
                }
                float t = Clamp(inside - 2.5f / Distance(path.a, path.b), 0, 1);
                Point center = {path.a.x + (path.b.x - path.a.x) * t, path.a.z + (path.b.z - path.a.z) * t};
                doors.push_back({center, std::atan2(path.b.z - path.a.z, path.b.x - path.a.x),
                                 path.halfWidth * 2, 4, u8"유리 자동문", true});
            }
            BuildVents();
            for (int z = 0; z < 200; ++z)
            {
                for (int x = 0; x < 200; ++x)
                {
                    Point a = {float(x), float(z)}, b = {float(x + 1), float(z)},
                          c = {float(x + 1), float(z + 1)}, d = {float(x), float(z + 1)};
                    Triangle(a, b, c);
                    Triangle(a, c, d);
                }
            }
            for (int i = 0; i < 24; ++i)
            {
                Point p = Polar(66, i * Pi / 12);
                AddLamp(p.x, p.z);
            }
            for (int i = 0; i < 16; ++i)
            {
                Point p = Polar(38, i * Pi / 8);
                AddLamp(p.x, p.z, true);
                AddBox(p.x, 3.15f, p.z, 1.7f, .16f, .16f, 4, false);
            }
            for (const auto& room : rooms)
            {
                AddLamp(room.center.x, room.center.z, room.style == 3 || room.style == 4);
            }
            for (const auto& branch : branches)
            {
                Point m = {(branch.a.x + branch.b.x) * .5f, (branch.a.z + branch.b.z) * .5f};
                AddLamp(m.x, m.z);
            }
            Furnish();
            for (int i = 0; i < 128; ++i)
            {
                for (float radius : {36.1f, 36.6f})
                {
                    Tube(Polar(radius, i * Pi / 64), Polar(radius, (i + 1) * Pi / 64), 3.25f, .09f, 4);
                }
            }
        }

        void AddBox(float x, float y, float z, float w, float h, float d, int mat, bool solid)
        {
            boxes.push_back({x, y, z, w, h, d, mat, solid});
        }

        void AddObstacle(float x, float y, float z, float w, float h, float d)
        {
            boxes.push_back({x, y, z, w, h, d, 3, true, false});
        }

        void Finish()
        {
            for (const auto& b : boxes)
            {
                if (b.visible)
                {
                    BoxMesh(b);
                }
            }
            IndexObstacles();
        }

        int RoomAt(Point p) const
        {
            for (int i = 0; i < int(rooms.size()); ++i)
            {
                const auto& r = rooms[i];
                if (p.x >= r.x0 && p.x <= r.x1 && p.z >= r.z0 && p.z <= r.z1)
                {
                    return i;
                }
            }
            return -1;
        }

        const char* Area(Point p) const
        {
            int r = RoomAt(p);
            if (r >= 0)
            {
                return rooms[r].name;
            }
            float radius = Distance(p, {100, 100});
            return radius < 44 ? u8"내측 정비 순환로"
                               : (radius > 60 ? u8"외측 생활 순환로" : u8"방사형 연결 통로");
        }

        float FloorDistance(Point p) const
        {
            float radius = Distance(p, {100, 100});
            float best = (std::max)(4 - std::fabs(radius - 66), 2.5f - std::fabs(radius - 38));
            // Central circular command space, independent of the room's label rectangle.
            for (const auto& r : rooms)
            {
                float dx = std::fabs(p.x - (r.x0 + r.x1) * .5f), dz = std::fabs(p.z - (r.z0 + r.z1) * .5f),
                      hx = (r.x1 - r.x0) * .5f, hz = (r.z1 - r.z0) * .5f;
                float rect = (std::min)(hx - dx, hz - dz);
                rect = (std::min)(rect, (hx + hz - 2 - dx - dz) * .70710678f);
                if (r.center.x == 100 && r.center.z == 100)
                {
                    rect = 11 - radius;
                }
                best = (std::max)(best, rect);
            }
            for (const auto& path : passages)
            {
                best = (std::max)(best, path.halfWidth - SegmentDistance(p, path.a, path.b));
            }
            return best;
        }

        bool CanStand(Point p) const
        {
            const float radius = .28f;
            if (FloorDistance(p) < radius + .04f)
            {
                return false;
            }
            int x0 = int(Clamp((p.x - radius) / 5, 0, 39)), x1 = int(Clamp((p.x + radius) / 5, 0, 39));
            int z0 = int(Clamp((p.z - radius) / 5, 0, 39)), z1 = int(Clamp((p.z + radius) / 5, 0, 39));
            for (int z = z0; z <= z1; ++z)
            {
                for (int x = x0; x <= x1; ++x)
                {
                    for (int i : obstacleGrid[z * Grid + x])
                    {
                        const auto& b = boxes[i];
                        if (b.y - b.h * .5f > 1.8f || b.y + b.h * .5f < .05f)
                        {
                            continue;
                        }
                        float nx = Clamp(p.x, b.x - b.w * .5f, b.x + b.w * .5f),
                              nz = Clamp(p.z, b.z - b.d * .5f, b.z + b.d * .5f);
                        if (Distance(p, {nx, nz}) < radius)
                        {
                            return false;
                        }
                    }
                }
            }
            return true;
        }

        bool Visible(Point eye, float eyeY, Point target, float targetY) const
        {
            float length = Distance(eye, target);
            int steps = int(std::ceil(length / .15f));
            for (int i = 1; i < steps; ++i)
            {
                float t = float(i) / steps;
                Point p = {eye.x + (target.x - eye.x) * t, eye.z + (target.z - eye.z) * t};
                float y = eyeY + (targetY - eyeY) * t;
                if (FloorDistance(p) < 0)
                {
                    return false;
                }
                for (int index : obstacleGrid[int(Clamp(p.z / 5, 0, 39)) * Grid + int(Clamp(p.x / 5, 0, 39))])
                {
                    const auto& b = boxes[index];
                    if (std::fabs(p.x - b.x) < b.w * .5f && std::fabs(p.z - b.z) < b.d * .5f &&
                        std::fabs(y - b.y) < b.h * .5f)
                    {
                        return false;
                    }
                }
            }
            return true;
        }

        void Draw(bool hdr, MetalMaterial& shader)
        {
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_NORMAL_ARRAY);
            for (int i = 0; i < 16; ++i)
            {
                auto& b = batches[i];
                if (b.vertices.empty())
                {
                    continue;
                }
                if (!b.buffer)
                {
                    glGenBuffers(1, &b.buffer);
                    glBindBuffer(GL_ARRAY_BUFFER, b.buffer);
                    glBufferData(GL_ARRAY_BUFFER, b.vertices.size() * sizeof(Vertex), b.vertices.data(),
                                 GL_STATIC_DRAW);
                }
                else
                {
                    glBindBuffer(GL_ARRAY_BUFFER, b.buffer);
                }
                const auto& m = materials[i];
                if (hdr)
                {
                    shader.Use(m.metal, m.rough, m.glow, true);
                }
                if (m.glow > 0)
                {
                    glDisable(GL_LIGHTING);
                }
                else
                {
                    glEnable(GL_LIGHTING);
                }
                glColor3f(m.r, m.g, m.b);
                glVertexPointer(3, GL_FLOAT, sizeof(Vertex), nullptr);
                glNormalPointer(GL_FLOAT, sizeof(Vertex), reinterpret_cast<void*>(3 * sizeof(float)));
                glDrawArrays(GL_TRIANGLES, 0, GLsizei(b.vertices.size()));
            }
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_NORMAL_ARRAY);
            glUseProgram(0);
        }

        void DrawFloorPlan()
        {
            glBegin(GL_TRIANGLES);
            for (int mat : {0, 1})
            {
                for (const auto& v : batches[mat].vertices)
                {
                    glVertex2f(v.x, v.z);
                }
            }
            glEnd();
        }

        void DrawGlass()
        {
            glUseProgram(0);
            glDisable(GL_LIGHTING);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glDepthMask(GL_FALSE);
            glColor4f(.13f, .35f, .42f, .13f);
            glBegin(GL_QUADS);
            for (const auto& e : glass)
            {
                glVertex3f(e.a.x, 1, e.a.z);
                glVertex3f(e.b.x, 1, e.b.z);
                glVertex3f(e.b.x, 3.45f, e.b.z);
                glVertex3f(e.a.x, 3.45f, e.a.z);
            }
            glEnd();
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }

        void Release()
        {
            for (auto& b : batches)
            {
                if (b.buffer)
                {
                    glDeleteBuffers(1, &b.buffer);
                    b.buffer = 0;
                }
            }
        }
    };
} // namespace Station
