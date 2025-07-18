#include "pch.h"
#include "World.h"

namespace CppMMO
{
    namespace Game
    {
        namespace Models
        {
            void World::AddPlayer(Player player)
            {
                uint64_t playerId = player.GetPlayerId();
                m_players[playerId] = std::move(player);
            }

            void World::RemovePlayer(uint64_t playerId)
            {
                m_players.erase(playerId);
            }

            std::optional<std::reference_wrapper<Player>> World::GetPlayer(uint64_t playerId)
            {
                auto it = m_players.find(playerId);
                if (it != m_players.end()) {
                    return std::ref(it->second);
                }
                return std::nullopt;
            }

            std::optional<std::reference_wrapper<const Player>> World::GetPlayer(uint64_t playerId) const
            {
                auto it = m_players.find(playerId);
                if (it != m_players.end()) {
                    return std::cref(it->second);
                }
                return std::nullopt;
            }

            const std::unordered_map<uint64_t, Player>& World::GetAllPlayers() const
            {
                return m_players;
            }

            std::vector<uint64_t> World::GetPlayerIds() const
            {
                std::vector<uint64_t> ids;
                ids.reserve(m_players.size());
                for (const auto& pair : m_players) {
                    ids.push_back(pair.first);
                }
                return ids;
            }

            size_t World::GetPlayerCount() const
            {
                return m_players.size();
            }

            void World::Update(float deltaTime)
            {
                for (auto& pair : m_players) {
                    pair.second.Update(deltaTime);
                }
            }
        }
    }
}