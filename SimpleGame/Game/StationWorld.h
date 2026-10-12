#pragma once

#include "../Engine/Object.h"
#include <queue>
#include <functional>

namespace Game
{
    using namespace Engine;

    struct Room
    {
        std::string key;
        const char* name;
        Bounds bounds;
        Vec3 center;
    };

    struct Door
    {
        std::string name;
        Vec3 center, across;
        float width = 2.8f, amount = 0, hold = 0;
        bool locked = false, commandControl = false;

        bool Blocks(Vec3 point, float radius, bool navigation = false) const;
    };

    class StationWorld
    {
        static constexpr int Size = 404;
        static constexpr float Cell = .5f;
        std::vector<std::vector<int>> floorIndex, wallIndex;
        std::vector<Triangle> floor;

        struct Segment
        {
            Vec3 a, b;
        };

        std::vector<Segment> walls;
        std::vector<Bounds> obstacles;
        std::vector<unsigned char> navigation;

        int X(float x) const;

        int Z(float z) const;

        int Index(Vec3 p) const;

        Vec3 Position(int index) const;

        void Cells(const Bounds& b, float padding, const std::function<void(int)>& fn);

        bool FloorAt(Vec3 p) const;

    public:
        std::vector<Room> rooms;
        std::vector<Door> doors;
        std::vector<Vec3> patrol;

        explicit StationWorld(const Mesh& mesh);

        const Room& GetRoom(const std::string& key) const;

        const char* Area(Vec3 p) const;

        bool Walkable(Vec3 p, float radius = .28f, bool dynamic = true) const;

        void AddObstacle(Bounds bounds);

        void Bake();

        Vec3 Nearest(Vec3 p, float radius = 12) const;

        bool Visible(Vec3 a, Vec3 b) const;

        std::vector<Vec3> Path(Vec3 from, Vec3 to) const;

        void Move(Vec3& position, Vec3 delta, float radius = .28f) const;

        void UpdateDoors(float dt, const std::vector<Vec3>& actors);
    };
} // namespace Game
