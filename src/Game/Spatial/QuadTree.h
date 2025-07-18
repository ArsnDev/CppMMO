#pragma once
#include "pch.h"
#include "Game/GameCommand.h"

/**
 * Represents a spatial quadtree for efficient partitioning and querying of player positions in 2D space.
 *
 * The QuadTree supports insertion, removal, and updating of player positions, as well as querying for player IDs within a circular area. Nodes subdivide automatically when player counts exceed a threshold, enabling efficient spatial queries and updates.
 */
namespace CppMMO
{
    namespace Game
    {
        namespace Spatial
        {
            class QuadTree
            {
            public:
                QuadTree(float x, float y, float width, float height);
                ~QuadTree() = default;

                void Insert(uint64_t playerId, const Vec3& position);
                void Remove(uint64_t playerId);
                void Update(uint64_t playerId, const Vec3& newPosition);

                std::vector<uint64_t> Query(const Vec3& center, float radius) const;

                size_t GetTotalNodes() const;
                size_t GetTotalPlayers() const;
                void Clear();
                
            private:
                struct Bounds
                {
                    float x, y;
                    float width, height;

                    bool Contains(const Vec3& point) const;
                    bool Intersects(const Vec3& center, float radius) const;
                };

                struct Node
                {
                    Bounds bounds;
                    std::vector<uint64_t> playerIds;

                    std::unique_ptr<Node> nw;
                    std::unique_ptr<Node> ne;
                    std::unique_ptr<Node> sw;
                    std::unique_ptr<Node> se;

                    bool IsLeaf() const{return nw == nullptr;}
                };

                std::unique_ptr<Node> m_root;

                static constexpr size_t MAX_PLAYERS_PER_NODE = 4;
                static constexpr size_t MAX_DEPTH = 6;

                std::unordered_map<uint64_t, Vec3> m_playerPositions;

                void InsertIntoNode(Node* node, uint64_t playerId, const Vec3& position, size_t depth);
                bool RemoveFromNode(Node* node, uint64_t playerId, const Vec3& position);
                void QueryNode(const Node* node, const Vec3& center, float radius, std::vector<uint64_t>& result) const;
                void SubdivideNode(Node* node);
                std::unique_ptr<Node> CreateNode(const Bounds& bounds);
                size_t CountNodes(const Node* node) const;
                size_t CountPlayers(const Node* node) const;
            };
        }
    }
}