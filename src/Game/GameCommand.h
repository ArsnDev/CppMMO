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

        // Input flags for keyboard input (bit flags)
        enum InputFlags : uint8_t
        {
            None = 0,
            Up = 1 << 0,      // 0000 0001 = Up (W)
            Down = 1 << 1,    // 0000 0010 = Down (S)
            Left = 1 << 2,    // 0000 0100 = Left (A)
            Right = 1 << 3,   // 0000 1000 = Right (D)
            Sprint = 1 << 4,  // 0001 0000 = Sprint (Shift) - for future use
            Jump = 1 << 5     // 0010 0000 = Jump (Space) - for future use
        };

        // Convert input flags to direction vector
        inline Vec2 InputFlagsToDirection(uint8_t inputFlags)
        {
            Vec2 direction{0.0f, 0.0f};
            
            // Handle opposing inputs (Up+Down, Left+Right cancel each other)
            bool up = (inputFlags & InputFlags::Up) != 0;
            bool down = (inputFlags & InputFlags::Down) != 0;
            bool left = (inputFlags & InputFlags::Left) != 0;
            bool right = (inputFlags & InputFlags::Right) != 0;
            
            // Vertical direction processing (Up and Down cancel each other)
            if (up && !down) direction.y += 1.0f;
            else if (down && !up) direction.y -= 1.0f;
            
            // Horizontal direction processing (Left and Right cancel each other)
            if (left && !right) direction.x -= 1.0f;
            else if (right && !left) direction.x += 1.0f;
            
            // Normalize diagonal movement (maintain consistent speed)
            float magnitude = std::sqrt(direction.x * direction.x + direction.y * direction.y);
            if (magnitude > 0.0f) {
                direction.x /= magnitude;
                direction.y /= magnitude;
            }
            
            return direction;
        }

        // Check if player is moving
        inline bool IsMoving(uint8_t inputFlags)
        {
            return (inputFlags & (InputFlags::Up | InputFlags::Down | InputFlags::Left | InputFlags::Right)) != 0;
        }

        struct MoveCommandData
        {
            uint64_t playerId = 0;
            Vec2 currentPosition{};
            uint8_t inputFlags = 0;        // bit flags for input
            int64_t timestamp = 0;
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