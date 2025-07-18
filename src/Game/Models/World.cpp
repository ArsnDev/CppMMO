#include "pch.h"
#include "World.h"

namespace CppMMO
{
    namespace Game
    {
        namespace Models
        {
            /**
             * @brief Adds or updates a player in the world.
             *
             * Inserts the given player into the world, replacing any existing player with the same ID.
             */
            void World::AddPlayer(Player player)
            {
                uint64_t playerId = player.GetPlayerId();
                m_players[playerId] = std::move(player);
            }

            /**
             * @brief Removes a player from the world by their unique player ID.
             *
             * If the specified player ID does not exist, the method has no effect.
             */
            void World::RemovePlayer(uint64_t playerId)
            {
                m_players.erase(playerId);
            }

            /**
             * @brief Retrieves a mutable reference to a player by their unique ID.
             *
             * @param playerId The unique identifier of the player to retrieve.
             * @return std::optional<std::reference_wrapper<Player>> A reference to the player if found, or std::nullopt if the player does not exist.
             */
            std::optional<std::reference_wrapper<Player>> World::GetPlayer(uint64_t playerId)
            {
                auto it = m_players.find(playerId);
                if (it != m_players.end()) {
                    return std::ref(it->second);
                }
                return std::nullopt;
            }

            /**
             * @brief Retrieves a const reference to a player by their unique ID.
             *
             * @param playerId The unique identifier of the player to retrieve.
             * @return std::optional<std::reference_wrapper<const Player>> A const reference to the player if found, or std::nullopt if the player does not exist.
             */
            std::optional<std::reference_wrapper<const Player>> World::GetPlayer(uint64_t playerId) const
            {
                auto it = m_players.find(playerId);
                if (it != m_players.end()) {
                    return std::cref(it->second);
                }
                return std::nullopt;
            }

            /**
             * @brief Returns a const reference to the map of all players in the world.
             *
             * @return Const reference to an unordered map associating player IDs with Player objects.
             */
            const std::unordered_map<uint64_t, Player>& World::GetAllPlayers() const
            {
                return m_players;
            }

            /**
             * @brief Returns a vector containing the IDs of all players in the world.
             *
             * @return std::vector<uint64_t> List of player IDs currently stored.
             */
            std::vector<uint64_t> World::GetPlayerIds() const
            {
                std::vector<uint64_t> ids;
                ids.reserve(m_players.size());
                for (const auto& pair : m_players) {
                    ids.push_back(pair.first);
                }
                return ids;
            }

            /**
             * @brief Returns the number of players currently in the world.
             *
             * @return size_t The count of players managed by this world.
             */
            size_t World::GetPlayerCount() const
            {
                return m_players.size();
            }

            /**
             * @brief Updates all players in the world by advancing their state.
             *
             * Calls the `Update` method on each player with the specified time delta.
             *
             * @param deltaTime The elapsed time in seconds since the last update.
             */
            void World::Update(float deltaTime)
            {
                for (auto& pair : m_players) {
                    pair.second.Update(deltaTime);
                }
            }
        }
    }
}