#pragma once
#include "pch.h"
#include "Player.h"

namespace CppMMO
{
    namespace Game
    {
        namespace Models
        {
            /**
             * @brief Manages the collection of players in the game world.
             *
             * The World class provides methods to add, remove, and retrieve players,
             * as well as update all players in the world.
             */
            class World
            {
            public:
                /**
                 * @brief Adds or updates a player in the world.
                 *
                 * Inserts the given player into the world, replacing any existing player with the same ID.
                 */
                void AddPlayer(Player player);

                /**
                 * @brief Removes a player from the world by their unique player ID.
                 *
                 * If the specified player ID does not exist, the method has no effect.
                 */
                void RemovePlayer(uint64_t playerId);

                /**
                 * @brief Retrieves a mutable reference to a player by their unique ID.
                 *
                 * @param playerId The unique identifier of the player to retrieve.
                 * @return std::optional<std::reference_wrapper<Player>> A reference to the player if found, or std::nullopt if the player does not exist.
                 */
                std::optional<std::reference_wrapper<Player>> GetPlayer(uint64_t playerId);

                /**
                 * @brief Retrieves a const reference to a player by their unique ID.
                 *
                 * @param playerId The unique identifier of the player to retrieve.
                 * @return std::optional<std::reference_wrapper<const Player>> A const reference to the player if found, or std::nullopt if the player does not exist.
                 */
                std::optional<std::reference_wrapper<const Player>> GetPlayer(uint64_t playerId) const;

                /**
                 * @brief Returns a reference to the map of all players in the world.
                 *
                 * @return Reference to an unordered map associating player IDs with Player objects.
                 */
                std::unordered_map<uint64_t, Player>& GetAllPlayers();

                /**
                 * @brief Returns a const reference to the map of all players in the world.
                 *
                 * @return Const reference to an unordered map associating player IDs with Player objects.
                 */
                const std::unordered_map<uint64_t, Player>& GetAllPlayers() const;

                /**
                 * @brief Returns a vector containing the IDs of all players in the world.
                 *
                 * @return std::vector<uint64_t> List of player IDs currently stored.
                 */
                std::vector<uint64_t> GetPlayerIds() const;

                /**
                 * @brief Returns the number of players currently in the world.
                 *
                 * @return size_t The count of players managed by this world.
                 */
                size_t GetPlayerCount() const;

                /**
                 * @brief Updates all players in the world by advancing their state.
                 *
                 * Calls the `Update` method on each player with the specified time delta.
                 *
                 * @param deltaTime The elapsed time in seconds since the last update.
                 */
                void Update(float deltaTime);

            private:
                std::unordered_map<uint64_t, Player> m_players;
            };
        }
    }
}