#include "stdafx.h"
#include "StationWorld.h"

namespace Game
{
    constexpr int StationWorld::Size;
    constexpr float StationWorld::Cell;

    bool Door::Blocks(Vec3 point, float radius, bool navigation) const
    {
        if (navigation && !locked && !commandControl)
        {
            return false;
        }
        Vec3 delta = point - center;
        float side = Dot(Flat(delta), across);
        Vec3 forward = {-across.z, 0, across.x};
        if (std::fabs(Dot(Flat(delta), forward)) > .08f + radius)
        {
            return false;
        }
        for (int sign : {-1, 1})
        {
            if (std::fabs(side - sign * (width * .25f + amount * width * .55f)) < width * .25f + radius)
            {
                return true;
            }
        }
        return false;
    }

    int StationWorld::X(float x) const
    {
        return int(std::floor((x + 101) / Cell));
    }

    int StationWorld::Z(float z) const
    {
        return int(std::floor((z + 101) / Cell));
    }

    int StationWorld::Index(Vec3 p) const
    {
        int x = X(p.x), z = Z(p.z);
        return x < 0 || x >= Size || z < 0 || z >= Size ? -1 : z * Size + x;
    }

    Vec3 StationWorld::Position(int index) const
    {
        return {(index % Size + .5f) * Cell - 101, 0, (index / Size + .5f) * Cell - 101};
    }

    void StationWorld::Cells(const Bounds& b, float padding, const std::function<void(int)>& fn)
    {
        int x0 = (std::max)(0, X(b.low.x - padding)), x1 = (std::min)(Size - 1, X(b.high.x + padding));
        int z0 = (std::max)(0, Z(b.low.z - padding)), z1 = (std::min)(Size - 1, Z(b.high.z + padding));
        for (int z = z0; z <= z1; ++z)
        {
            for (int x = x0; x <= x1; ++x)
            {
                fn(z * Size + x);
            }
        }
    }

    bool StationWorld::FloorAt(Vec3 p) const
    {
        int cell = Index(p);
        if (cell < 0)
        {
            return false;
        }
        for (int index : floorIndex[cell])
        {
            const auto& t = floor[index];
            if (InTriangle(p, t.a, t.b, t.c))
            {
                return true;
            }
        }
        return false;
    }

    StationWorld::StationWorld(const Mesh& mesh)
    {
        floorIndex.resize(Size * Size);
        wallIndex.resize(Size * Size);
        navigation.resize(Size * Size);
        for (const auto& t : mesh.triangles)
        {
            if (t.group.find("Floor_") == 0 && std::fabs(t.a.y) < .01f && std::fabs(t.b.y) < .01f &&
                std::fabs(t.c.y) < .01f && std::fabs(Side(t.a, t.b, t.c)) > .001f)
            {
                int index = int(floor.size());
                floor.push_back(t);
                Bounds b;
                b.Include(t.a);
                b.Include(t.b);
                b.Include(t.c);
                Cells(b, .01f,
                      [&](int cell)
                      {
                          floorIndex[cell].push_back(index);
                      });
            }
            bool wall = t.group.find("Walls_") == 0 ||
                        t.group.find("Hab_") == 0 && t.group.find("Hab_Bed") == std::string::npos ||
                        t.group.find("DAMAGED") == 0 || t.group.find("Door_") == 0;
            float low = (std::min)(t.a.y, (std::min)(t.b.y, t.c.y)),
                  high = (std::max)(t.a.y, (std::max)(t.b.y, t.c.y));
            if (wall && low < 1.8f && high > .3f && high - low > .1f)
            {
                Vec3 a = Flat(t.a), b = Flat(t.b), c = Flat(t.c);
                Segment edge = {a, b};
                if (Distance(a, c) > Distance(edge.a, edge.b))
                {
                    edge = {a, c};
                }
                if (Distance(b, c) > Distance(edge.a, edge.b))
                {
                    edge = {b, c};
                }
                if (Distance(edge.a, edge.b) > .01f)
                {
                    int index = int(walls.size());
                    walls.push_back(edge);
                    Bounds box;
                    box.Include(edge.a);
                    box.Include(edge.b);
                    Cells(box, .8f,
                          [&](int cell)
                          {
                              wallIndex[cell].push_back(index);
                          });
                }
            }
        }
        const std::pair<const char*, const char*> names[] = {
            {"Control", u8"관제실"}, {"Research", u8"연구실"},   {"LifeSupport", u8"생명유지실"},
            {"Power", u8"발전실"},   {"Medical", u8"의료실"},    {"Habitation", u8"거주구"},
            {"Warehouse", u8"창고"}, {"Docking", u8"도킹 베이"}, {"Escape", u8"탈출정"}};
        for (auto name : names)
        {
            auto it = mesh.groups.find(std::string("Floor_") + name.first);
            if (it == mesh.groups.end())
            {
                throw std::runtime_error("Required OBJ room is missing");
            }
            rooms.push_back({name.first, name.second, it->second, Flat(it->second.Center())});
        }
        for (const auto& group : mesh.groups)
        {
            const auto& name = group.first;
            const auto& b = group.second;
            if (name.find("_Header") != std::string::npos)
            {
                std::string root = name.substr(0, name.size() - 7);
                auto left = mesh.groups.find(root + "_JambL"), right = mesh.groups.find(root + "_JambR");
                if (left != mesh.groups.end() && right != mesh.groups.end())
                {
                    Vec3 a = Flat(left->second.Center()), c = Flat(right->second.Center());
                    Door door;
                    door.name = root;
                    door.center = (a + c) * .5f;
                    door.across = Normal(c - a);
                    door.width = Distance(a, c) - .6f;
                    door.locked = name.find("Door_Power_") == 0 || name.find("Door_LifePower_") == 0 ||
                                  name.find("Door_Escape_") == 0;
                    doors.push_back(door);
                }
            }
            bool equipment = name.find("Floor_") != 0 && name.find("Walls_") != 0 &&
                             name.find("Ceiling") != 0 && name.find("Door_") != 0 &&
                             name.find("Vent_") != 0 && name.find("Label_") != 0 &&
                             name.find("DAMAGED") != 0 && name.find("Player") != 0 &&
                             !(name.find("Hab_") == 0 && name.find("Hab_Bed") != 0);
            if (equipment && b.low.y < 1.8f && b.high.y > .3f)
            {
                obstacles.push_back(b);
            }
        }
        Bake();
        for (int i = 0; i < 16; ++i)
        {
            float angle = i * 2 * Pi / 16;
            patrol.push_back(Nearest({68 * std::cos(angle), 0, 68 * std::sin(angle)}));
        }
    }

    const Room& StationWorld::GetRoom(const std::string& key) const
    {
        for (const auto& room : rooms)
        {
            if (room.key == key)
            {
                return room;
            }
        }
        throw std::runtime_error("Unknown room " + key);
    }

    const char* StationWorld::Area(Vec3 p) const
    {
        for (const auto& room : rooms)
        {
            if (p.x >= room.bounds.low.x && p.x <= room.bounds.high.x && p.z >= room.bounds.low.z &&
                p.z <= room.bounds.high.z)
            {
                return room.name;
            }
        }
        return FlatDistance(p, {}) < 46 ? u8"내측 정비 순환로" : u8"외측 생활 순환로";
    }

    bool StationWorld::Walkable(Vec3 p, float radius, bool dynamic) const
    {
        if (!FloorAt(p) || !FloorAt(p + Vec3{radius, 0, 0}) || !FloorAt(p - Vec3{radius, 0, 0}) ||
            !FloorAt(p + Vec3{0, 0, radius}) || !FloorAt(p - Vec3{0, 0, radius}))
        {
            return false;
        }
        int index = Index(p);
        if (index < 0)
        {
            return false;
        }
        for (int i : wallIndex[index])
        {
            if (SegmentDistance(p, walls[i].a, walls[i].b) < radius + .015f)
            {
                return false;
            }
        }
        for (const auto& box : obstacles)
        {
            Vec3 closest = {Clamp(p.x, box.low.x, box.high.x), 0, Clamp(p.z, box.low.z, box.high.z)};
            if (FlatDistance(p, closest) < radius)
            {
                return false;
            }
        }
        if (dynamic)
        {
            for (const auto& door : doors)
            {
                if (door.Blocks(p, radius))
                {
                    return false;
                }
            }
        }
        return true;
    }

    void StationWorld::AddObstacle(Bounds bounds)
    {
        obstacles.push_back(bounds);
    }

    void StationWorld::Bake()
    {
        for (int i = 0; i < Size * Size; ++i)
        {
            navigation[i] = Walkable(Position(i), .35f, false) ? 1 : 0;
        }
    }

    Vec3 StationWorld::Nearest(Vec3 p, float radius) const
    {
        float best = 1e9f;
        Vec3 result = p;
        int cx = X(p.x), cz = Z(p.z), range = int(radius / Cell);
        for (int z = (std::max)(0, cz - range); z <= (std::min)(Size - 1, cz + range); ++z)
        {
            for (int x = (std::max)(0, cx - range); x <= (std::min)(Size - 1, cx + range); ++x)
            {
                int index = z * Size + x;
                if (!navigation[index])
                {
                    continue;
                }
                Vec3 point = Position(index);
                float distance = FlatDistance(p, point);
                if (distance < best)
                {
                    best = distance;
                    result = point;
                }
            }
        }
        if (best == 1e9f)
        {
            throw std::runtime_error("No walkable floor near game anchor");
        }
        return result;
    }

    bool StationWorld::Visible(Vec3 a, Vec3 b) const
    {
        float distance = FlatDistance(a, b);
        int steps = int(distance / .2f) + 1;
        for (int i = 1; i < steps; ++i)
        {
            Vec3 p = a + (b - a) * (float(i) / steps);
            int index = Index(p);
            if (index < 0)
            {
                return false;
            }
            for (int j : wallIndex[index])
            {
                if (SegmentDistance(p, walls[j].a, walls[j].b) < .035f)
                {
                    return false;
                }
            }
            for (const auto& box : obstacles)
            {
                if (p.x > box.low.x && p.x < box.high.x && p.z > box.low.z && p.z < box.high.z &&
                    p.y > box.low.y && p.y < box.high.y)
                {
                    return false;
                }
            }
            for (const auto& door : doors)
            {
                if (door.Blocks(p, 0))
                {
                    return false;
                }
            }
        }
        return true;
    }

    std::vector<Vec3> StationWorld::Path(Vec3 from, Vec3 to) const
    {
        int start = Index(Nearest(from, 3)), goal = Index(Nearest(to, 3));

        struct Step
        {
            float priority;
            int index;

            bool operator<(const Step& b) const
            {
                return priority > b.priority;
            }
        };

        std::priority_queue<Step> open;
        std::vector<float> cost(Size * Size, 1e9f);
        std::vector<int> parent(Size * Size, -1);
        cost[start] = 0;
        open.push({0, start});
        int visits = 0;
        auto pass = [&](int index)
        {
            if (index < 0 || index >= Size * Size || !navigation[index])
            {
                return false;
            }
            for (const auto& door : doors)
            {
                if (door.Blocks(Position(index), .35f, true))
                {
                    return false;
                }
            }
            return true;
        };
        while (!open.empty() && visits++ < 60000)
        {
            int current = open.top().index;
            open.pop();
            if (current == goal)
            {
                break;
            }
            int x = current % Size, z = current / Size;
            for (int dz = -1; dz <= 1; ++dz)
            {
                for (int dx = -1; dx <= 1; ++dx)
                {
                    if (!dx && !dz || x + dx < 0 || x + dx >= Size || z + dz < 0 || z + dz >= Size)
                    {
                        continue;
                    }
                    int next = (z + dz) * Size + x + dx;
                    if (!pass(next))
                    {
                        continue;
                    }
                    if (dx && dz && (!pass(z * Size + x + dx) || !pass((z + dz) * Size + x)))
                    {
                        continue;
                    }
                    float value = cost[current] + (dx && dz ? .7071068f : .5f);
                    if (value < cost[next])
                    {
                        cost[next] = value;
                        parent[next] = current;
                        open.push({value + FlatDistance(Position(next), Position(goal)), next});
                    }
                }
            }
        }
        std::vector<Vec3> result;
        if (start != goal && parent[goal] < 0)
        {
            return result;
        }
        for (int index = goal; index != start; index = parent[index])
        {
            if (index < 0)
            {
                return {};
            }
            result.push_back(Position(index));
        }
        std::reverse(result.begin(), result.end());
        return result;
    }

    void StationWorld::Move(Vec3& position, Vec3 delta, float radius) const
    {
        int steps = (std::max)(1, int(std::ceil(Length(delta) / .07f)));
        delta = delta * (1.f / steps);
        for (int i = 0; i < steps; ++i)
        {
            Vec3 next = position + Vec3{delta.x, 0, 0};
            if (Walkable(next, radius))
            {
                position = next;
            }
            next = position + Vec3{0, 0, delta.z};
            if (Walkable(next, radius))
            {
                position = next;
            }
        }
    }

    void StationWorld::UpdateDoors(float dt, const std::vector<Vec3>& actors)
    {
        for (auto& door : doors)
        {
            bool nearby = false;
            for (Vec3 p : actors)
            {
                if (FlatDistance(p, door.center) < 5)
                {
                    nearby = true;
                }
            }
            if (nearby && !door.locked && !door.commandControl)
            {
                door.hold = 1.25f;
            }
            else
            {
                door.hold = (std::max)(0.f, door.hold - dt);
            }
            float next = Clamp(door.amount +
                                   (door.hold > 0 && !door.locked && !door.commandControl ? 2.f : -1.2f) * dt,
                               0, 1);
            for (Vec3 p : actors)
            {
                if (next < door.amount && door.Blocks(p, .55f))
                {
                    next = Clamp(door.amount + dt * 2, 0, 1);
                    break;
                }
            }
            door.amount = next;
        }
    }

} // namespace Game
