#pragma once
#include "pch.h"
#include <flatbuffers/flatbuffers.h>
#include <queue>
#include <unordered_map>
#include "protocol_generated.h"

namespace CppMMO::Utils
{
    /**
     * @brief Thread-safe memory pool for FlatBuffers builders to eliminate dynamic allocation
     * 
     * Maintains a pool of pre-allocated FlatBufferBuilder instances that can be reused
     * across multiple packet creation operations, significantly reducing memory allocation overhead.
     */
    class FlatBufferBuilderPool
    {
    public:
        explicit FlatBufferBuilderPool(size_t poolSize = 256, size_t initialCapacity = 1024);
        ~FlatBufferBuilderPool() = default;

        // Get a builder from the pool
        std::unique_ptr<flatbuffers::FlatBufferBuilder> Acquire();
        
        // Return a builder to the pool
        void Release(std::unique_ptr<flatbuffers::FlatBufferBuilder> builder);
        
        // Get pool statistics
        size_t GetPoolSize() const { return m_poolSize; }
        size_t GetAvailableCount() const;

    private:
        const size_t m_poolSize;
        const size_t m_initialCapacity;
        
        mutable std::mutex m_mutex;
        std::queue<std::unique_ptr<flatbuffers::FlatBufferBuilder>> m_availableBuilders;
        
        std::unique_ptr<flatbuffers::FlatBufferBuilder> CreateBuilder();
    };

    /**
     * @brief RAII wrapper for automatic builder return to pool
     */
    class PooledFlatBufferBuilder
    {
    public:
        PooledFlatBufferBuilder(FlatBufferBuilderPool& pool);
        ~PooledFlatBufferBuilder();
        
        // Non-copyable, movable only
        PooledFlatBufferBuilder(const PooledFlatBufferBuilder&) = delete;
        PooledFlatBufferBuilder& operator=(const PooledFlatBufferBuilder&) = delete;
        PooledFlatBufferBuilder(PooledFlatBufferBuilder&& other) noexcept;
        PooledFlatBufferBuilder& operator=(PooledFlatBufferBuilder&& other) noexcept;
        
        flatbuffers::FlatBufferBuilder& operator*() { return *m_builder; }
        flatbuffers::FlatBufferBuilder* operator->() { return m_builder.get(); }
        const flatbuffers::FlatBufferBuilder& operator*() const { return *m_builder; }
        const flatbuffers::FlatBufferBuilder* operator->() const { return m_builder.get(); }
        
        bool IsValid() const { return m_builder != nullptr; }

    private:
        FlatBufferBuilderPool* m_pool;
        std::unique_ptr<flatbuffers::FlatBufferBuilder> m_builder;
    };

    /**
     * @brief Thread-safe object pool template for reusable objects
     */
    template<typename T>
    class ObjectPool
    {
    public:
        explicit ObjectPool(size_t poolSize = 128) : m_poolSize(poolSize) {}
        
        template<typename... Args>
        std::unique_ptr<T> Acquire(Args&&... args)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            
            if (!m_available.empty())
            {
                auto obj = std::move(m_available.front());
                m_available.pop();
                return obj;
            }
            
            // Create new object if pool is empty
            return std::make_unique<T>(std::forward<Args>(args)...);
        }
        
        void Release(std::unique_ptr<T> obj)
        {
            if (!obj) return;
            
            std::lock_guard<std::mutex> lock(m_mutex);
            
            if (m_available.size() < m_poolSize)
            {
                m_available.push(std::move(obj));
            }
            // If pool is full, let the object be destroyed
        }
        
        size_t GetAvailableCount() const
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            return m_available.size();
        }

    private:
        const size_t m_poolSize;
        mutable std::mutex m_mutex;
        std::queue<std::unique_ptr<T>> m_available;
    };

    /**
     * @brief Pre-allocated string cache for common player names
     */
    class StringCache
    {
    public:
        StringCache();
        
        // Get cached string or create new one
        std::string GetPlayerName(uint64_t playerId);
        
        // Pre-warm cache with common patterns
        void PrewarmCache(size_t maxPlayers);

    private:
        mutable std::mutex m_mutex;
        std::unordered_map<uint64_t, std::string> m_playerNames;
        static constexpr size_t MAX_CACHE_SIZE = 10000;
    };

    /**
     * @brief Thread-safe vector pool for PlayerState offsets
     */
    class PlayerStateVectorPool
    {
    public:
        using PlayerStateVector = std::vector<flatbuffers::Offset<CppMMO::Protocol::PlayerState>>;
        
        explicit PlayerStateVectorPool(size_t poolSize = 64, size_t reserveSize = 200);
        
        // Get a vector from the pool
        std::unique_ptr<PlayerStateVector> Acquire();
        
        // Return a vector to the pool
        void Release(std::unique_ptr<PlayerStateVector> vector);
        
        // Get pool statistics
        size_t GetPoolSize() const { return m_poolSize; }
        size_t GetAvailableCount() const;

    private:
        const size_t m_poolSize;
        const size_t m_reserveSize;
        
        mutable std::mutex m_mutex;
        std::queue<std::unique_ptr<PlayerStateVector>> m_availableVectors;
        
        std::unique_ptr<PlayerStateVector> CreateVector();
    };

    /**
     * @brief RAII wrapper for automatic vector return to pool
     */
    class PooledPlayerStateVector
    {
    public:
        PooledPlayerStateVector(PlayerStateVectorPool& pool);
        ~PooledPlayerStateVector();
        
        // Non-copyable, movable only
        PooledPlayerStateVector(const PooledPlayerStateVector&) = delete;
        PooledPlayerStateVector& operator=(const PooledPlayerStateVector&) = delete;
        PooledPlayerStateVector(PooledPlayerStateVector&& other) noexcept;
        PooledPlayerStateVector& operator=(PooledPlayerStateVector&& other) noexcept;
        
        PlayerStateVectorPool::PlayerStateVector& operator*() { return *m_vector; }
        PlayerStateVectorPool::PlayerStateVector* operator->() { return m_vector.get(); }
        const PlayerStateVectorPool::PlayerStateVector& operator*() const { return *m_vector; }
        const PlayerStateVectorPool::PlayerStateVector* operator->() const { return m_vector.get(); }
        
        bool IsValid() const { return m_vector != nullptr; }

    private:
        PlayerStateVectorPool* m_pool;
        std::unique_ptr<PlayerStateVectorPool::PlayerStateVector> m_vector;
    };

    /**
     * @brief Global memory pool manager
     */
    class MemoryPoolManager
    {
    public:
        static MemoryPoolManager& Instance();
        
        FlatBufferBuilderPool& GetBuilderPool() { return m_builderPool; }
        PlayerStateVectorPool& GetVectorPool() { return m_vectorPool; }
        StringCache& GetStringCache() { return m_stringCache; }
        
        // Get pooled builder with RAII wrapper
        PooledFlatBufferBuilder GetPooledBuilder() { return PooledFlatBufferBuilder(m_builderPool); }
        
        // Get pooled vector with RAII wrapper
        PooledPlayerStateVector GetPooledVector() { return PooledPlayerStateVector(m_vectorPool); }
        
        // Print pool statistics
        void PrintStats() const;

    private:
        MemoryPoolManager();
        
        FlatBufferBuilderPool m_builderPool;
        PlayerStateVectorPool m_vectorPool;
        StringCache m_stringCache;
    };
}