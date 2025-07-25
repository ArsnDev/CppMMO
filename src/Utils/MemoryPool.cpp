#include "pch.h"
#include "MemoryPool.h"
#include "Logger.h"

namespace CppMMO::Utils
{
    // Memory pool configuration constants
    static constexpr size_t DEFAULT_BUILDER_POOL_SIZE = 1024;
    static constexpr size_t DEFAULT_BUILDER_CAPACITY = 1024;
    static constexpr size_t DEFAULT_VECTOR_POOL_SIZE = 256;
    static constexpr size_t DEFAULT_VECTOR_CAPACITY = 200;
    static constexpr size_t DEFAULT_STRING_CACHE_SIZE = 1000;
    // FlatBufferBuilderPool Implementation
    FlatBufferBuilderPool::FlatBufferBuilderPool(size_t poolSize, size_t initialCapacity)
        : m_poolSize(poolSize), m_initialCapacity(initialCapacity)
    {
        // Pre-allocate builders to avoid allocation during runtime
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < poolSize; ++i)
        {
            m_availableBuilders.push(CreateBuilder());
        }
        
        LOG_INFO("FlatBufferBuilderPool initialized with {} builders, {} bytes each", 
                poolSize, initialCapacity);
    }

    std::unique_ptr<flatbuffers::FlatBufferBuilder> FlatBufferBuilderPool::Acquire()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_availableBuilders.empty())
        {
            auto builder = std::move(m_availableBuilders.front());
            m_availableBuilders.pop();
            
            // Reset builder for reuse
            builder->Clear();
            return builder;
        }
        
        // Pool exhausted, create new builder (this should be rare)
        LOG_WARN("FlatBufferBuilderPool exhausted, creating new builder");
        return CreateBuilder();
    }

    void FlatBufferBuilderPool::Release(std::unique_ptr<flatbuffers::FlatBufferBuilder> builder)
    {
        if (!builder) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Only return to pool if not full
        if (m_availableBuilders.size() < m_poolSize)
        {
            // Clear the builder before returning to pool
            builder->Clear();
            m_availableBuilders.push(std::move(builder));
        }
        // If pool is full, let the builder be destroyed
    }

    size_t FlatBufferBuilderPool::GetAvailableCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_availableBuilders.size();
    }

    std::unique_ptr<flatbuffers::FlatBufferBuilder> FlatBufferBuilderPool::CreateBuilder()
    {
        return std::make_unique<flatbuffers::FlatBufferBuilder>(m_initialCapacity);
    }

    // PooledFlatBufferBuilder Implementation
    PooledFlatBufferBuilder::PooledFlatBufferBuilder(FlatBufferBuilderPool& pool)
        : m_pool(&pool), m_builder(pool.Acquire())
    {
    }

    PooledFlatBufferBuilder::~PooledFlatBufferBuilder()
    {
        if (m_pool && m_builder)
        {
            m_pool->Release(std::move(m_builder));
        }
    }

    PooledFlatBufferBuilder::PooledFlatBufferBuilder(PooledFlatBufferBuilder&& other) noexcept
        : m_pool(other.m_pool), m_builder(std::move(other.m_builder))
    {
        other.m_pool = nullptr;
    }

    PooledFlatBufferBuilder& PooledFlatBufferBuilder::operator=(PooledFlatBufferBuilder&& other) noexcept
    {
        if (this != &other)
        {
            // Return current builder to pool
            if (m_pool && m_builder)
            {
                m_pool->Release(std::move(m_builder));
            }
            
            // Take ownership from other
            m_pool = other.m_pool;
            m_builder = std::move(other.m_builder);
            other.m_pool = nullptr;
        }
        return *this;
    }

    // StringCache Implementation
    StringCache::StringCache()
    {
        LOG_INFO("StringCache initialized");
    }

    std::string StringCache::GetPlayerName(uint64_t playerId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_playerNames.find(playerId);
        if (it != m_playerNames.end())
        {
            return it->second;
        }
        
        // Create and cache new player name
        if (m_playerNames.size() < MAX_CACHE_SIZE)
        {
            std::string name = "Player_" + std::to_string(playerId);
            m_playerNames[playerId] = name;
            return name;
        }
        
        // Cache full, return temporary string
        return "Player_" + std::to_string(playerId);
    }

    void StringCache::PrewarmCache(size_t maxPlayers)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        size_t cacheSize = std::min(maxPlayers, MAX_CACHE_SIZE);
        m_playerNames.reserve(cacheSize);
        
        for (size_t i = 1; i <= cacheSize; ++i)
        {
            m_playerNames[i] = "Player_" + std::to_string(i);
        }
        
        LOG_INFO("StringCache prewarmed with {} player names", cacheSize);
    }

    // PlayerStateVectorPool Implementation
    PlayerStateVectorPool::PlayerStateVectorPool(size_t poolSize, size_t reserveSize)
        : m_poolSize(poolSize), m_reserveSize(reserveSize)
    {
        // Pre-allocate vectors to avoid allocation during runtime
        std::lock_guard<std::mutex> lock(m_mutex);
        for (size_t i = 0; i < poolSize; ++i)
        {
            m_availableVectors.push(CreateVector());
        }
        
        LOG_INFO("PlayerStateVectorPool initialized with {} vectors, {} capacity each", 
                poolSize, reserveSize);
    }

    std::unique_ptr<PlayerStateVectorPool::PlayerStateVector> PlayerStateVectorPool::Acquire()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (!m_availableVectors.empty())
        {
            auto vector = std::move(m_availableVectors.front());
            m_availableVectors.pop();
            
            // Clear and prepare for reuse
            vector->clear();
            return vector;
        }
        
        // Pool exhausted, create new vector (this should be rare)
        LOG_WARN("PlayerStateVectorPool exhausted, creating new vector");
        return CreateVector();
    }

    void PlayerStateVectorPool::Release(std::unique_ptr<PlayerStateVector> vector)
    {
        if (!vector) return;
        
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // Only return to pool if not full
        if (m_availableVectors.size() < m_poolSize)
        {
            // Clear the vector before returning to pool
            vector->clear();
            m_availableVectors.push(std::move(vector));
        }
        // If pool is full, let the vector be destroyed
    }

    size_t PlayerStateVectorPool::GetAvailableCount() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_availableVectors.size();
    }

    std::unique_ptr<PlayerStateVectorPool::PlayerStateVector> PlayerStateVectorPool::CreateVector()
    {
        auto vector = std::make_unique<PlayerStateVector>();
        vector->reserve(m_reserveSize);
        return vector;
    }

    // PooledPlayerStateVector Implementation
    PooledPlayerStateVector::PooledPlayerStateVector(PlayerStateVectorPool& pool)
        : m_pool(&pool), m_vector(pool.Acquire())
    {
    }

    PooledPlayerStateVector::~PooledPlayerStateVector()
    {
        if (m_pool && m_vector)
        {
            m_pool->Release(std::move(m_vector));
        }
    }

    PooledPlayerStateVector::PooledPlayerStateVector(PooledPlayerStateVector&& other) noexcept
        : m_pool(other.m_pool), m_vector(std::move(other.m_vector))
    {
        other.m_pool = nullptr;
    }

    PooledPlayerStateVector& PooledPlayerStateVector::operator=(PooledPlayerStateVector&& other) noexcept
    {
        if (this != &other)
        {
            // Return current vector to pool
            if (m_pool && m_vector)
            {
                m_pool->Release(std::move(m_vector));
            }
            
            // Take ownership from other
            m_pool = other.m_pool;
            m_vector = std::move(other.m_vector);
            other.m_pool = nullptr;
        }
        return *this;
    }

    // MemoryPoolManager Implementation
    MemoryPoolManager& MemoryPoolManager::Instance()
    {
        static MemoryPoolManager instance;
        return instance;
    }

    MemoryPoolManager::MemoryPoolManager()
        : m_builderPool(DEFAULT_BUILDER_POOL_SIZE, DEFAULT_BUILDER_CAPACITY),
          m_vectorPool(DEFAULT_VECTOR_POOL_SIZE, DEFAULT_VECTOR_CAPACITY)
    {
        // Prewarm string cache for expected player count
        m_stringCache.PrewarmCache(DEFAULT_STRING_CACHE_SIZE);
        
        LOG_INFO("MemoryPoolManager initialized");
    }

    void MemoryPoolManager::PrintStats() const
    {
        LOG_INFO("=== Memory Pool Statistics ===");
        LOG_INFO("FlatBufferBuilderPool: {}/{} builders available", 
                m_builderPool.GetAvailableCount(), m_builderPool.GetPoolSize());
        LOG_INFO("PlayerStateVectorPool: {}/{} vectors available", 
                m_vectorPool.GetAvailableCount(), m_vectorPool.GetPoolSize());
        LOG_INFO("==============================");
    }
}