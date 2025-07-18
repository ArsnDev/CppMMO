#pragma once
#include "pch.h"
#include "protocol_generated.h"
#include <variant>

/**
         * Represents a 3D vector with float components and provides basic vector operations.
         *
         * Supports construction from individual components or a FlatBuffers Vec3, 2D helpers, and common vector math such as addition, subtraction, scalar multiplication, length calculation, and normalization.
         */
        namespace CppMMO
{
    namespace Game
    {
        struct Vec3
        {
            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            
            Vec3() = default;
            Vec3(float x_, float y_, float z_ = 0.0f) : x(x_), y(y_), z(z_) {}
            
            Vec3(const Protocol::Vec3* fbVec3) 
                : x(fbVec3->X()), y(fbVec3->Y()), z(fbVec3->Z()) {}
            
            static Vec3 From2D(float x, float y) { return Vec3(x, y, 0.0f); }
            bool Is2D() const { return z == 0.0f; }
            
            // Math operations
            Vec3 operator+(const Vec3& other) const { return Vec3(x + other.x, y + other.y, z + other.z); }
            Vec3 operator-(const Vec3& other) const { return Vec3(x - other.x, y - other.y, z - other.z); }
            Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
            
            float Length() const { return std::sqrt(x * x + y * y + z * z); }
            Vec3 Normalized() const { 
                float len = Length(); 
                return len > 0.0f ? Vec3(x/len, y/len, z/len) : Vec3(); 
            }
        };

        enum InputFlags : uint8_t
        {
            None = 0,
            W = 1,        // 0000 0001 = W (Up)
            S = 2,        // 0000 0010 = S (Down)  
            A = 4,        // 0000 0100 = A (Left)
            D = 8,        // 0000 1000 = D (Right)
            Shift = 16,   // 0001 0000 = Sprint (Shift) - for future use
            Space = 32    // 0010 0000 = Jump (Space) - for future use
        };

        inline Vec3 InputFlagsToDirection(uint8_t inputFlags)
        {
            Vec3 direction{0.0f, 0.0f, 0.0f};

            bool w = (inputFlags & InputFlags::W) != 0;
            bool s = (inputFlags & InputFlags::S) != 0;
            bool a = (inputFlags & InputFlags::A) != 0;
            bool d = (inputFlags & InputFlags::D) != 0;

            if (w && !s) direction.y += 1.0f;
            else if (s && !w) direction.y -= 1.0f;
            
            if (a && !d) direction.x -= 1.0f;
            else if (d && !a) direction.x += 1.0f;
            
            return direction.Normalized();
        }

        inline bool IsMoving(uint8_t inputFlags)
        {
            return (inputFlags & (InputFlags::W | InputFlags::S | InputFlags::A | InputFlags::D)) != 0;
        }

        inline uint64_t GetCurrentTimestamp()
        {
            return std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
        }

        struct PlayerInputCommandData
        {
            uint64_t playerId = 0;
            uint64_t tickNumber = 0;
            uint64_t clientTime = 0;
            uint8_t inputFlags = 0;
            Vec3 mousePosition{};
            uint32_t sequenceNumber = 0;
            int64_t sessionId = 0;
        };

        struct EnterZoneCommandData
        {
            uint64_t playerId = 0;
            int zoneId = 0;
            int64_t sessionId = 0;
        };

        struct PlayerSpawnCommandData
        {
            uint64_t playerId = 0;
            std::string playerName;
            Vec3 spawnPosition{};
            int hp = 100;
            int maxHp = 100;
            int mp = 50;
            int maxMp = 50;
        };

        struct PlayerDisconnectCommandData
        {
            uint64_t playerId = 0;
        };

        using GameCommandPayload = std::variant<
            PlayerInputCommandData,
            EnterZoneCommandData,
            PlayerSpawnCommandData,
            PlayerDisconnectCommandData
        >;

        struct GameCommand
        {
            int64_t commandId = 0;
            GameCommandPayload payload{};
            int64_t senderSessionId = 0;
            uint64_t timestamp = 0;
        };
    }
}