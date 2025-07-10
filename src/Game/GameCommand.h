#pragma once
#include "pch.h"
#include <variant>

namespace CppMMO
{
    namespace Game
    {
        struct Vec2
        {
            float x;
            float y;
        };

        struct MoveCommandData
        {
            uint64_t entityId;
            Vec2 targetPosition;
        };

        struct PlayerHpUpdateCommandData
        {
            uint64_t playerId;
            int currentHp;
        };

        struct ChangeZoneCommandData
        {
            uint64_t playerId;
            int targetZoneId;
        };

        using GameCommandPayload = std::variant<MoveCommandData,
                                                PlayerHpUpdateCommandData,
                                                ChangeZoneCommandData
                                                >;
        struct GameCommand
        {
            int64_t commandId;
            GameCommandPayload payload;
            int64_t senderSessionId;
        };
    }
}