#pragma once
#include "pch.h"
#include <variant>

namespace CppMMO
{
    namespace Game
    {
        struct Vec2
        {
            float x = 0.0f;
            float y = 0.0f;
        };

        struct MoveCommandData
        {
            uint64_t entityId = 0;
            Vec2 targetPosition{};
        };

        struct PlayerHpUpdateCommandData
        {
            uint64_t playerId = 0;
            int currentHp = 0;
        };

        struct ChangeZoneCommandData
        {
            uint64_t playerId = 0;
            int targetZoneId = 0;
        };

        using GameCommandPayload = std::variant<MoveCommandData,
                                                PlayerHpUpdateCommandData,
                                                ChangeZoneCommandData
                                                >;
        struct GameCommand
        {
            int64_t commandId = 0;
            GameCommandPayload payload{};
            int64_t senderSessionId = 0;
        };
    }
}