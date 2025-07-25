#include "Player.h"

namespace CppMMO
{
    namespace Game
    {
        namespace Models
        {
            Player::Player(uint64_t playerId, const std::string& name, const Vec3& spawnPosition)
                : m_playerId(playerId), m_name(name), m_position(spawnPosition)
            {
            }


            void Player::SetActive(bool active)
            {
                m_isActive = active;
                if (!active)
                {
                    m_disconnectTime = std::chrono::steady_clock::now();
                }
            }

            bool Player::ShouldRemove() const
            {
                if (m_isActive)
                {
                    return false;
                }
                
                auto now = std::chrono::steady_clock::now();
                auto timeSinceDisconnect = std::chrono::duration_cast<std::chrono::minutes>(now - m_disconnectTime);
                return timeSinceDisconnect >= RECONNECT_TIMEOUT;
            }

            void Player::Update([[maybe_unused]] float deltaTime)
            {
                // TODO: Implement player update logic
            }

            bool Player::IsInputAllowed() const
            {
                static constexpr int INPUT_RATE_LIMIT_MS = 33;  // 30fps = 33ms minimum interval
                
                auto now = std::chrono::steady_clock::now();
                auto timeSinceLastInput = std::chrono::duration_cast<std::chrono::milliseconds>(now - m_lastInputTime);
                return timeSinceLastInput.count() >= INPUT_RATE_LIMIT_MS;
            }

            void Player::UpdateLastInputTime()
            {
                m_lastInputTime = std::chrono::steady_clock::now();
            }
        }
    }
}