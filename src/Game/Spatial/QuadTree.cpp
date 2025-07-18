#include "QuadTree.h"

namespace CppMMO
{
    namespace Game
    {
        namespace Spatial
        {
            /**
             * @brief Constructs a QuadTree with the specified spatial bounds.
             *
             * Initializes the root node to cover the rectangular area defined by the given x and y coordinates, width, and height.
             */
            QuadTree::QuadTree(float x, float y, float width, float height)
            {
                if (width <= 0.0f || height <= 0.0f) {
                    throw std::invalid_argument("QuadTree bounds must have positive width and height");
                }
                m_root = CreateNode(Bounds(x, y, width, height));
            }
            /**
             * @brief Inserts or updates a player's position in the QuadTree.
             *
             * If the player ID already exists, their position is updated; otherwise, the player is added at the specified position.
             */
            void QuadTree::Insert(uint64_t playerId, const Vec3& position) 
            {
                m_playerPositions[playerId] = position;
                InsertIntoNode(m_root.get(), playerId, position, 0);
            }
            /**
             * @brief Removes a player from the QuadTree by player ID.
             *
             * If the player exists, their position is removed from the internal map and the player ID is deleted from the corresponding node in the tree.
             */
            void QuadTree::Remove(uint64_t playerId) 
            {
                auto it = m_playerPositions.find(playerId);
                if (it != m_playerPositions.end()) {
                    Vec3 position = it->second;
                    m_playerPositions.erase(it);
                    RemoveFromNode(m_root.get(), playerId, position);
                }
            }
            /**
             * @brief Updates a player's position in the QuadTree.
             *
             * Removes the player's previous position and inserts the new position, ensuring the player's spatial data is current.
             */
            void QuadTree::Update(uint64_t playerId, const Vec3& newPosition) 
            {
                Remove(playerId);
                Insert(playerId, newPosition);
            }
            /**
             * @brief Returns player IDs within a circular area centered at the given point.
             *
             * Searches the QuadTree for all players whose positions lie within the specified radius of the center point.
             *
             * @param center The center of the search circle.
             * @param radius The radius of the search circle.
             * @return std::vector<uint64_t> Vector of player IDs found within the area.
             */
            std::vector<uint64_t> QuadTree::Query(const Vec3& center, float radius) const 
            {
                std::vector<uint64_t> result;
                QueryNode(m_root.get(), center, radius, result);
                return result;
            }
            /**
 * @brief Returns the total number of nodes in the QuadTree.
 *
 * This includes all internal and leaf nodes currently present in the tree.
 *
 * @return size_t The total count of nodes.
 */
size_t QuadTree::GetTotalNodes() const { return CountNodes(m_root.get()); }
            /**
 * @brief Returns the total number of players currently stored in the QuadTree.
 *
 * @return size_t The number of players managed by the QuadTree.
 */
size_t QuadTree::GetTotalPlayers() const { return m_playerPositions.size(); }
            /**
             * @brief Removes all players and resets the QuadTree to its initial state.
             *
             * Clears all stored player positions and removes all child nodes from the root, effectively emptying the tree.
             */
            void QuadTree::Clear() 
            {
                m_playerPositions.clear();
                m_root->playerIds.clear();
                m_root->nw.reset();
                m_root->ne.reset();
                m_root->sw.reset();
                m_root->se.reset();
            }

            /**
 * @brief Constructs a bounding rectangle with the specified position and size.
 *
 * @param x The x-coordinate of the rectangle's origin.
 * @param y The y-coordinate of the rectangle's origin.
 * @param width The width of the rectangle.
 * @param height The height of the rectangle.
 */
QuadTree::Bounds::Bounds(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}
            /**
 * @brief Checks if a 2D point lies within the bounds.
 *
 * The check uses the x and y coordinates of the point and the rectangle's position and size.
 *
 * @param point The point to test for containment.
 * @return true if the point is inside the bounds; false otherwise.
 */
bool QuadTree::Bounds::Contains(const Vec3& point) const { return point.x >= x && point.x < x + width && point.y >= y && point.y < y + height;}
            /**
             * @brief Determines whether the bounds intersect with a given circle.
             *
             * Checks if the axis-aligned rectangle defined by this bounds overlaps with a circle specified by its center and radius.
             *
             * @param center The center of the circle.
             * @param radius The radius of the circle.
             * @return true if the rectangle and circle intersect; false otherwise.
             */
            bool QuadTree::Bounds::Intersects(const Vec3& center, float radius) const 
            {
                float closestX = std::max(x, std::min(center.x, x + width));
                float closestY = std::max(y, std::min(center.y, y + height));
                
                float distanceSquared = std::pow(center.x - closestX, 2) + std::pow(center.y - closestY, 2);
                return distanceSquared <= radius * radius;
            }

            /**
 * @brief Constructs a QuadTree node with the specified spatial bounds.
 *
 * @param bounds The axis-aligned bounding rectangle for this node.
 */
QuadTree::Node::Node(const Bounds& bounds) : bounds(bounds) {}
            /**
             * @brief Recursively inserts a player ID into the appropriate node of the QuadTree.
             *
             * If the node exceeds the maximum allowed players and the depth limit is not reached, the node is subdivided and all players are redistributed among the child nodes.
             *
             * @param node The node into which the player should be inserted.
             * @param playerId The unique identifier of the player.
             * @param position The 3D position of the player (only x and y are used).
             * @param depth The current depth of the node in the tree.
             */
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
                            Vec3 playerPos = (pid == playerId) ? position : m_playerPositions[pid];

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
            /**
             * @brief Recursively removes a player ID from the node containing the specified position.
             *
             * Searches for the player ID in the subtree rooted at the given node, removing it if found in a leaf node whose bounds contain the position.
             *
             * @param node Pointer to the current node in the QuadTree.
             * @param playerId The unique identifier of the player to remove.
             * @param position The position associated with the player.
             * @return true if the player was found and removed; false otherwise.
             */
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
            /**
             * @brief Recursively collects player IDs within a circular area from the specified node.
             *
             * Traverses the QuadTree starting from the given node, adding player IDs to the result vector if their positions are within the specified radius of the center point. Only nodes whose bounds intersect the query circle are searched.
             *
             * @param node The current QuadTree node to search.
             * @param center The center of the query circle.
             * @param radius The radius of the query circle.
             * @param result Vector to which matching player IDs are appended.
             */
            void QuadTree::QueryNode(const Node* node, const Vec3& center, float radius, std::vector<uint64_t>& result) const 
            {
                if (!node->bounds.Intersects(center, radius)) {
                    return;
                }
                
                if (node->IsLeaf()) {
                    float radiusSquared = radius * radius;
                    for (uint64_t playerId : node->playerIds) {
                        auto it = m_playerPositions.find(playerId);
                        if (it == m_playerPositions.end()) {
                            continue; // 플레이어가 이미 제거됨
                        }
                        Vec3 playerPos = it->second;
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
            /**
             * @brief Splits a node into four child quadrants.
             *
             * Divides the specified node's bounds into northwest, northeast, southwest, and southeast quadrants,
             * creating a new child node for each region.
             */
            void QuadTree::SubdivideNode(Node* node) 
            {
                float halfWidth = node->bounds.width * 0.5f;
                float halfHeight = node->bounds.height * 0.5f;
                
                node->nw = CreateNode(Bounds(node->bounds.x, node->bounds.y, halfWidth, halfHeight));
                node->ne = CreateNode(Bounds(node->bounds.x + halfWidth, node->bounds.y, halfWidth, halfHeight));
                node->sw = CreateNode(Bounds(node->bounds.x, node->bounds.y + halfHeight, halfWidth, halfHeight));
                node->se = CreateNode(Bounds(node->bounds.x + halfWidth, node->bounds.y + halfHeight, halfWidth, halfHeight));
            }
            /**
             * @brief Creates a new QuadTree node with the specified bounds.
             *
             * @param bounds The spatial bounds for the new node.
             * @return A unique pointer to the newly created node.
             */
            std::unique_ptr<QuadTree::Node> QuadTree::CreateNode(const Bounds& bounds)
            {
                return std::make_unique<Node>(bounds);
            }
            /**
             * @brief Recursively counts the total number of nodes in the subtree rooted at the given node.
             *
             * @param node Pointer to the root node of the subtree to count.
             * @return size_t Total number of nodes in the subtree, including the root node.
             */
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
            /**
             * @brief Recursively counts the total number of players in the subtree rooted at the given node.
             *
             * @param node Pointer to the root node of the subtree.
             * @return size_t Total number of player IDs stored in the subtree.
             */
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