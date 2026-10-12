#pragma once

#include "GameSession.h"
#include "../AI/StationAgents.h"

namespace Game
{
    struct HideSpot
    {
        Vec3 position, front;
    };

    class Simulation
    {
        GameSession& session;
        StationWorld& world;
        std::unique_ptr<AI::Agent> alien;
        std::unique_ptr<AI::Agent> researcher;
        std::array<std::unique_ptr<AI::Agent>, 2> companions;
        unsigned noiseId = 0;
        float footstep = 0;
        Vec3 escortOrigin;

        void Advance(int chapter);

        bool Near(const PlayerState& p, Vec3 target, float range = 2.6f) const;

        bool PlayerWorking(const Site& site) const;

        void UpdatePlayer(float dt);

        void Step(const Site& site, float dt, std::function<void()> complete);

        void Attack(int target);

        void Unlock(const std::string& name);

    public:
        std::vector<Site> sites;
        std::vector<HideSpot> cabinets;
        std::vector<Noise> noises;
        std::string message;
        const char* chapters[12] = {u8"제1장 · 정거장 도착",        u8"제2장 · 관제실의 정전",
                                    u8"제3장 · 발전실 진입",        u8"제4장 · 비상전력 복구",
                                    u8"제5장 · 구조 신호의 기록",   u8"제6장 · 첫 은신과 파손된 우주선",
                                    u8"제7장 · 탈출정 카드 담당자", u8"제8장 · 연구원의 요구",
                                    u8"제9장 · 샘플과 추격",        u8"제10장 · 카드 키와 최종 도주",
                                    u8"제11장 · 탈출정 가동",       u8"탈출 성공"};

        Simulation(GameSession& session, StationWorld& world);

        const char* AlienAction() const;

        const char* CompanionAction(int i) const;

        Vec3 CabinetFront(int i) const;

        int TargetSite() const;

        const char* Objective() const;

        void NoiseAt(Vec3 p, float strength);

        int CabinetFocus(const PlayerState& p) const;

        int Focus(const PlayerState& p) const;

        void Tick(float dt);
    };
} // namespace Game
