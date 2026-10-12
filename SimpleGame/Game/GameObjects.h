#pragma once
#include "Simulation.h"

namespace Game
{
    class ModelObject : public Object
    {
    public:
        struct Piece
        {
            Vec3 position, scale;
            Material material;
            float yaw = 0;
        };

        std::vector<Piece> pieces;

        explicit ModelObject(ResourceManager& resources);

        void Box(Vec3 p, Vec3 size, Material color = {.3f, .38f, .4f, .7f, .6f, 0});

        void Draw(MetalMaterial& shader, bool hdr) override;
    };

    class EquipmentObject : public ModelObject
    {
        const GameState& state;
        int id;

    public:
        EquipmentObject(ResourceManager& resources, const Site& site, const GameState& state);

        void Update(float) override;
    };

    class CabinetObject : public ModelObject
    {
        const GameState& state;
        int id;
        int lastOccupant = -1;
        float animation = 1;
        std::vector<Piece> door;

    public:
        CabinetObject(ResourceManager& resources, const HideSpot& spot, int id, const GameState& state);

        void Update(float dt) override;

        void Draw(MetalMaterial& shader, bool hdr) override;
    };

    class DoorObject : public Object
    {
        const Door& door;

    public:
        DoorObject(ResourceManager& resources, const Door& door);

        void Draw(MetalMaterial& shader, bool hdr) override;
    };
} // namespace Game
