#pragma once
#include "pch.h"
#include "Game/GameCommand.h"
#include "Network/ISession.h"

/**
 * Represents a player entity in the game, encapsulating identity, state, stats, input, and connection status.
 *
 * Provides methods to access and modify player attributes such as position, velocity, rotation, health, mana, input flags, mouse position, and connection state. Also manages synchronization and timing information relevant to gameplay and networking.
 */
namespace CppMMO
{
    namespace Game
    {
        namespace Models
        {
            class Player
            {
            public:
                // Constructor
                Player(uint64_t playerId, const std::string& name, const Vec3& spawnPosition);
                Player() = default;

                // === Basic Info ===
                uint64_t GetPlayerId() const { return m_playerId; }
                const std::string& GetName() const { return m_name; }
                
                // === Transform ===
                const Vec3& GetPosition() const { return m_position; }
                void SetPosition(const Vec3& position) { m_position = position; }
                
                const Vec3& GetVelocity() const { return m_velocity; }
                void SetVelocity(const Vec3& velocity) { m_velocity = velocity; }
                
                float GetRotation() const { return m_rotation; }
                void SetRotation(float rotation) { m_rotation = rotation; }

                // === Stats ===
                int GetHp() const { return m_hp; }
                int GetMaxHp() const { return m_maxHp; }
                void SetHp(int hp) { m_hp = std::clamp(hp, 0, m_maxHp); }
                
                int GetMp() const { return m_mp; }
                int GetMaxMp() const { return m_maxMp; }
                void SetMp(int mp) { m_mp = std::clamp(mp, 0, m_maxMp); }

                // === Input & Sync ===
                uint8_t GetCurrentInputFlags() const { return m_currentInputFlags; }
                void SetCurrentInputFlags(uint8_t flags) { m_currentInputFlags = flags; }
                
                const Vec3& GetMousePosition() const { return m_mousePosition; }
                void SetMousePosition(const Vec3& mousePos) { m_mousePosition = mousePos; }
                
                uint32_t GetLastInputSequence() const { return m_lastInputSequence; }
                void SetLastInputSequence(uint32_t sequence) { m_lastInputSequence = sequence; }

                // === Connection State ===
                bool IsActive() const { return m_isActive; }
                void SetActive(bool active);
                bool ShouldRemove() const;
                
                // === Game State ===
                uint64_t GetLastUpdateTick() const { return m_lastUpdateTick; }
                void SetLastUpdateTick(uint64_t tick) { m_lastUpdateTick = tick; }
                
                float GetMoveSpeed() const { return m_moveSpeed; }
                void SetMoveSpeed(float speed) { m_moveSpeed = speed; }

                // === Update ===
                void Update(float deltaTime);

            private:
                // === Basic Info ===
                uint64_t m_playerId = 0;
                std::string m_name;
                
                // === Transform ===
                Vec3 m_position{};
                Vec3 m_velocity{};
                float m_rotation = 0.0f;
                
                // === Stats ===
                int m_hp = 100;
                int m_maxHp = 100;
                int m_mp = 50;
                int m_maxMp = 50;
                
                // === Input & Sync ===
                uint8_t m_currentInputFlags = 0;
                Vec3 m_mousePosition{};
                uint32_t m_lastInputSequence = 0;
                
                // === Connection State ===
                bool m_isActive = true;
                std::chrono::steady_clock::time_point m_disconnectTime;
                
                // === Game State ===
                uint64_t m_lastUpdateTick = 0;
                float m_moveSpeed = 5.0f;
                
                // === Constants ===
                static constexpr std::chrono::minutes RECONNECT_TIMEOUT{5};
            };
        }
    }
}