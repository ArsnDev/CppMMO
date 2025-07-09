#pragma once
#include "pch.h"

namespace CppMMO
{
    namespace Network
    {
        class ISession;
    }
    namespace Game
    {
        namespace Models
        {
            class Player
            {
            public:
                Player();

                uint64_t GetPlayerId() const {return m_playerId;}
                void SetSession(std::shared_ptr<Network::ISession> const& session) { m_session = session;}
                void Update(float deltaTime);

            private:
                uint64_t m_playerId = 0;
                std::string m_name{};

                float m_x = 0.0f;
                float m_y = 0.0f;
                int32_t m_hp = 0;

                std::mutex m_playerMutex;
                std::weak_ptr<Network::ISession> m_session;
            };
        }
    }
}