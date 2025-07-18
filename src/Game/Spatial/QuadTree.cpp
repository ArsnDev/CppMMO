#include "QuadTree.h"

namespace CppMMO
{
    namespace Game
    {
        namespace Spatial
        {
            QuadTree::QuadTree(float x, float y, float width, float height)
            {
                m_root = CreateNode(Bounds(x, y, width, height));
            }
            void QuadTree::Insert(uint64_t playerId, const Vec3& position) 
            {
                m_playerPositions[playerId] = position;
                InsertIntoNode(m_root.get(), playerId, position, 0);
            }
            void QuadTree::Remove(uint64_t playerId) 
            {
                auto it = m_playerPositions.find(playerId);
                if (it != m_playerPositions.end()) {
                    Vec3 position = it->second;
                    m_playerPositions.erase(it);
                    RemoveFromNode(m_root.get(), playerId, position);
                }
            }
            void QuadTree::Update(uint64_t playerId, const Vec3& newPosition) 
            {
                Remove(playerId);
                Insert(playerId, newPosition);
            }
            std::vector<uint64_t> QuadTree::Query(const Vec3& center, float radius) const 
            {
                std::vector<uint64_t> result;
                QueryNode(m_root.get(), center, radius, result);
                return result;
            }
            size_t QuadTree::GetTotalNodes() const { return CountNodes(m_root.get()); }
            size_t QuadTree::GetTotalPlayers() const { return m_playerPositions.size(); }
            void QuadTree::Clear() 
            {
                m_playerPositions.clear();
                m_root->playerIds.clear();
                m_root->nw.reset();
                m_root->ne.reset();
                m_root->sw.reset();
                m_root->se.reset();
            }

            QuadTree::Bounds::Bounds(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}
            bool QuadTree::Bounds::Contains(const Vec3& point) const { return point.x >= x && point.x < x + width && point.y >= y && point.y < y + height;}
            bool QuadTree::Bounds::Intersects(const Vec3& center, float radius) const 
            {
                float closestX = std::max(x, std::min(center.x, x + width));
                float closestY = std::max(y, std::min(center.y, y + height));
                
                float distanceSquared = std::pow(center.x - closestX, 2) + std::pow(center.y - closestY, 2);
                return distanceSquared <= radius * radius;
            }

            QuadTree::Node::Node(const Bounds& bounds) : bounds(bounds) {}
            void QuadTree::InsertIntoNode(Node* node, uint64_t playerId, const Vec3& position, size_t depth)
            {
                if (node->IsLeaf())
                {
                    node->playerIds.push_back(playerId);

                    if (node->playerIds.size() > MAX_PLAYERS_PER_NODE && depth < MAX_DEPTH)
                    {
                        SubdivideNode(node);

                        std::vector<uint64_t> playersToReinsert = node->playerIds;
                        node->playerIds.clear();

                        for (uint64_t pid : playersToReinsert)
                        {
                            Vec3 playerPos = m_playerPositions[pid];

                            if (node->nw->bounds.Contains(playerPos))
                                InsertIntoNode(node->nw.get(), pid, playerPos, depth + 1);
                            else if (node->ne->bounds.Contains(playerPos))
                                InsertIntoNode(node->ne.get(), pid, playerPos, depth + 1);
                            else if (node->sw->bounds.Contains(playerPos))
                                InsertIntoNode(node->sw.get(), pid, playerPos, depth + 1);
                            else if (node->se->bounds.Contains(playerPos))
                                InsertIntoNode(node->se.get(), pid, playerPos, depth + 1);
                        }
                    }
                }
                else
                {
                    if (node->nw->bounds.Contains(position))
                        InsertIntoNode(node->nw.get(), playerId, position, depth + 1);
                    else if (node->ne->bounds.Contains(position))
                        InsertIntoNode(node->ne.get(), playerId, position, depth + 1);
                    else if (node->sw->bounds.Contains(position))
                        InsertIntoNode(node->sw.get(), playerId, position, depth + 1);
                    else if (node->se->bounds.Contains(position))
                        InsertIntoNode(node->se.get(), playerId, position, depth + 1);
                }
            }
            bool QuadTree::RemoveFromNode(Node* node, uint64_t playerId, const Vec3& position) 
            {
                if (!node->bounds.Contains(position)) {
                    return false;
                }
                
                if (node->IsLeaf()) {
                    auto it = std::find(node->playerIds.begin(), node->playerIds.end(), playerId);
                    if (it != node->playerIds.end()) {
                        node->playerIds.erase(it);
                        return true;
                    }
                    return false;
                } else {
                    return RemoveFromNode(node->nw.get(), playerId, position) ||
                           RemoveFromNode(node->ne.get(), playerId, position) ||
                           RemoveFromNode(node->sw.get(), playerId, position) ||
                           RemoveFromNode(node->se.get(), playerId, position);
                }
            }
            void QuadTree::QueryNode(const Node* node, const Vec3& center, float radius, std::vector<uint64_t>& result) const 
            {
                if (!node->bounds.Intersects(center, radius)) {
                    return;
                }
                
                if (node->IsLeaf()) {
                    float radiusSquared = radius * radius;
                    for (uint64_t playerId : node->playerIds) {
                        Vec3 playerPos = m_playerPositions.at(playerId);
                        float distanceSquared = std::pow(playerPos.x - center.x, 2) + std::pow(playerPos.y - center.y, 2);
                        if (distanceSquared <= radiusSquared) {
                            result.push_back(playerId);
                        }
                    }
                } else {
                    QueryNode(node->nw.get(), center, radius, result);
                    QueryNode(node->ne.get(), center, radius, result);
                    QueryNode(node->sw.get(), center, radius, result);
                    QueryNode(node->se.get(), center, radius, result);
                }
            }
            void QuadTree::SubdivideNode(Node* node) 
            {
                float halfWidth = node->bounds.width * 0.5f;
                float halfHeight = node->bounds.height * 0.5f;
                
                node->nw = CreateNode(Bounds(node->bounds.x, node->bounds.y, halfWidth, halfHeight));
                node->ne = CreateNode(Bounds(node->bounds.x + halfWidth, node->bounds.y, halfWidth, halfHeight));
                node->sw = CreateNode(Bounds(node->bounds.x, node->bounds.y + halfHeight, halfWidth, halfHeight));
                node->se = CreateNode(Bounds(node->bounds.x + halfWidth, node->bounds.y + halfHeight, halfWidth, halfHeight));
            }
            std::unique_ptr<QuadTree::Node> QuadTree::CreateNode(const Bounds& bounds)
            {
                return std::make_unique<Node>(bounds);
            }
            size_t QuadTree::CountNodes(const Node* node) const 
            {
                if (!node) return 0;
                
                size_t count = 1;
                if (!node->IsLeaf()) {
                    count += CountNodes(node->nw.get());
                    count += CountNodes(node->ne.get());
                    count += CountNodes(node->sw.get());
                    count += CountNodes(node->se.get());
                }
                return count;
            }
            size_t QuadTree::CountPlayers(const Node* node) const 
            {
                if (!node) return 0;
                
                size_t count = node->playerIds.size();
                if (!node->IsLeaf()) {
                    count += CountPlayers(node->nw.get());
                    count += CountPlayers(node->ne.get());
                    count += CountPlayers(node->sw.get());
                    count += CountPlayers(node->se.get());
                }
                return count;
            }
        }
    }
}