#ifndef WORKPHONE_CHARACTER_POOL_ALLOCATOR_HPP
#define WORKPHONE_CHARACTER_POOL_ALLOCATOR_HPP

#include <Workphone/WorkphoneConfig.hpp>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace workphone
{
    /**
     * @brief Statistics for the shared character pool used by CharacterPoolAllocator.
     */
    struct CharacterPoolStats
    {
        std::size_t committedBytes = 0;
        std::size_t slabCount = 0;
        std::size_t liveBlocks = 0;
        std::size_t freeBlocks = 0;
        std::size_t bucketCount = 0;
    };

    /**
     * @brief Thread-safe size-class allocator intended for dynamic string buffers.
     *
     * All allocator instances of the same <T, UpstreamAllocator> specialization share
     * one pool. Requests are rounded to a small character quantum and served from
     * slabs. Returned blocks are cached and reused. Large allocations bypass the
     * pool so an unusually large temporary string does not permanently bloat it.
     *
     * The pool is created lazily on first use. reserve_blocks() can be called during
     * engine startup to warm a desired size class ahead of time.
     */
    template <class T, class UpstreamAllocator = std::allocator<T>>
    class CharacterPoolAllocator
    {
    public:
        using value_type = T;
        using upstream_allocator_type = UpstreamAllocator;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        template <class U>
        struct rebind
        {
            using upstream_rebind =
                typename std::allocator_traits<UpstreamAllocator>::template rebind_alloc<U>;
            using other = CharacterPoolAllocator<U, upstream_rebind>;
        };

        // Keep the number of size classes modest while avoiding power-of-two waste.
        static constexpr size_type AllocationQuantum = 64;

        // A new size class normally receives about 64 KiB of character storage.
        static constexpr size_type DefaultSlabBytes = static_cast<const size_type>( 64u * 1024u );

        // Very large buffers go directly upstream instead of being retained forever.
        static constexpr size_type MaxPooledBlockBytes = static_cast<const size_type>( 256u * 1024u );

        CharacterPoolAllocator() noexcept = default;

        template <class U, class OtherAllocator>
        CharacterPoolAllocator( const CharacterPoolAllocator<U, OtherAllocator> & ) noexcept
        {
        }

        [[nodiscard]] T *allocate( size_type count )
        {
            if( count == 0 )
            {
                return nullptr;
            }

            if( count > max_size() )
            {
                throw std::bad_array_new_length();
            }

            return pool().allocate( count );
        }

        void deallocate( T *ptr, size_type count ) noexcept
        {
            if( !ptr )
            {
                return;
            }

            pool().deallocate( ptr, count );
        }

        [[nodiscard]] constexpr size_type max_size() const noexcept
        {
            return ( std::numeric_limits<size_type>::max )() / sizeof( T );
        }

        /**
         * @brief Preallocate free blocks for one request size.
         * @param characterCount Number of T elements requested by each allocation.
         * @param blockCount Minimum number of free blocks to have ready.
         */
        static void reserve_blocks( size_type characterCount, size_type blockCount )
        {
            if( characterCount == 0 || blockCount == 0 )
            {
                return;
            }

            pool().reserveBlocks( characterCount, blockCount );
        }

        /**
         * @brief Release buckets that currently have no outstanding allocations.
         *
         * Buckets containing live string buffers are intentionally left untouched.
         */
        static void release_unused() noexcept
        {
            pool().releaseUnused();
        }

        [[nodiscard]] static CharacterPoolStats stats() noexcept
        {
            return pool().stats();
        }

        [[nodiscard]] static size_type rounded_count( size_type count )
        {
            return Pool::roundCount( count );
        }

    private:
        class Pool
        {
        private:
            using Traits = std::allocator_traits<UpstreamAllocator>;

            struct Slab
            {
                T *data = nullptr;
                size_type count = 0;
            };

            struct Bucket
            {
                size_type blockCharacters = 0;
                size_type liveBlocks = 0;
                size_type totalBlocks = 0;
                std::vector<T *> freeBlocks;
                std::vector<Slab> slabs;
            };

        public:
            Pool() = default;
            Pool( const Pool & ) = delete;
            Pool &operator=( const Pool & ) = delete;

            // A static StringBase may still own a block during shutdown. Free
            // only buckets with no live blocks, leaving those strings' storage
            // intact until their own destructors run.
            ~Pool() noexcept
            {
                std::lock_guard<std::mutex> lock( m_mutex );
                m_destroyed.store( true, std::memory_order_release );
                for( auto &entry : m_buckets )
                {
                    if( entry.second.liveBlocks == 0 )
                    {
                        releaseBucket( entry.second );
                    }
                }
            }

            static size_type roundCount( size_type count )
            {
                if( count == 0 )
                {
                    return 0;
                }

                if( !isPooledCount( count ) )
                {
                    return count;
                }

                constexpr size_type q = AllocationQuantum;
                const size_type remainder = count % q;
                if( remainder == 0 )
                {
                    return count;
                }

                const size_type extra = q - remainder;
                if( count > ( std::numeric_limits<size_type>::max )() - extra )
                {
                    throw std::bad_array_new_length();
                }

                return count + extra;
            }

            [[nodiscard]] T *allocate( size_type count )
            {
                if( !isPooledCount( count ) )
                {
                    std::lock_guard<std::mutex> lock( m_mutex );
                    return Traits::allocate( m_upstream, count );
                }

                const size_type rounded = roundCount( count );
                std::lock_guard<std::mutex> lock( m_mutex );

                Bucket &bucket = m_buckets[rounded];
                if( bucket.blockCharacters == 0 )
                {
                    bucket.blockCharacters = rounded;
                }

                if( bucket.freeBlocks.empty() )
                {
                    growBucket( bucket, 1 );
                }

                T *result = bucket.freeBlocks.back();
                bucket.freeBlocks.pop_back();
                ++bucket.liveBlocks;
                return result;
            }

            void deallocate( T *ptr, size_type count ) noexcept
            {
                if( !isPooledCount( count ) )
                {
                    std::lock_guard<std::mutex> lock( m_mutex );
                    Traits::deallocate( m_upstream, ptr, count );
                    return;
                }

                size_type rounded = 0;
                try
                {
                    rounded = roundCount( count );
                }
                catch( ... )
                {
                    // The count came from a successful earlier allocation, so this
                    // should be unreachable. Avoid throwing from deallocate().
                    assert( false && "CharacterPoolAllocator invalid deallocation size" );
                    return;
                }

                std::lock_guard<std::mutex> lock( m_mutex );
                if( m_destroyed.load( std::memory_order_acquire ) )
                {
                    // Pool has been destroyed. Buckets may have been freed; avoid
                    // touching m_buckets here and just leak the memory.
                    return;
                }
                auto it = m_buckets.find( rounded );
                if( it == m_buckets.end() )
                {
                    // Bucket was destroyed (e.g., pool shutdown during static destruction).
                    // The memory was allocated from a slab, so we cannot pass it to
                    // the upstream allocator. Just leak it; the OS reclaims at exit.
                    return;
                }

                Bucket &bucket = it->second;
                if( bucket.liveBlocks == 0 )
                {
                    // Bucket is in a corrupt state (already fully deallocated).
                    // The memory was allocated from a slab, so we cannot pass it to
                    // the upstream allocator. Just leak it; the OS reclaims at exit.
                    return;
                }

                // growBucket() reserves freeBlocks capacity for every slab before
                // making any block visible, therefore this push_back cannot allocate.
                bucket.freeBlocks.push_back( ptr );
                --bucket.liveBlocks;
            }

            void reserveBlocks( size_type count, size_type requestedFreeBlocks )
            {
                if( !isPooledCount( count ) )
                {
                    // Oversized allocations intentionally bypass the pool.
                    return;
                }

                const size_type rounded = roundCount( count );
                std::lock_guard<std::mutex> lock( m_mutex );

                Bucket &bucket = m_buckets[rounded];
                if( bucket.blockCharacters == 0 )
                {
                    bucket.blockCharacters = rounded;
                }

                if( bucket.freeBlocks.size() < requestedFreeBlocks )
                {
                    growBucket( bucket, requestedFreeBlocks - bucket.freeBlocks.size() );
                }
            }

            void releaseUnused() noexcept
            {
                std::lock_guard<std::mutex> lock( m_mutex );

                for( auto &entry : m_buckets )
                {
                    Bucket &bucket = entry.second;
                    if( bucket.liveBlocks == 0 )
                    {
                        releaseBucket( bucket );
                    }
                }
            }

            void releaseAll() noexcept
            {
                std::lock_guard<std::mutex> lock( m_mutex );

                for( auto &entry : m_buckets )
                {
                    releaseBucket( entry.second );
                }
                m_buckets.clear();
            }

            [[nodiscard]] CharacterPoolStats stats() noexcept
            {
                std::lock_guard<std::mutex> lock( m_mutex );

                CharacterPoolStats result;
                result.bucketCount = m_buckets.size();

                for( const auto &entry : m_buckets )
                {
                    const Bucket &bucket = entry.second;
                    result.liveBlocks += bucket.liveBlocks;
                    result.freeBlocks += bucket.freeBlocks.size();
                    result.slabCount += bucket.slabs.size();

                    for( const Slab &slab : bucket.slabs )
                    {
                        result.committedBytes += slab.count * sizeof( T );
                    }
                }

                return result;
            }

        private:
            static bool isPooledCount( size_type count ) noexcept
            {
                if( count == 0 )
                {
                    return false;
                }

                return count <= MaxPooledBlockBytes / sizeof( T );
            }

            void growBucket( Bucket &bucket, size_type minimumBlocks )
            {
                const size_type blockChars = bucket.blockCharacters;
                if( blockChars == 0 )
                {
                    throw std::logic_error( "CharacterPoolAllocator invalid bucket size" );
                }

                const size_type blockBytes = blockChars * sizeof( T );
                size_type normalBlocks = DefaultSlabBytes / blockBytes;
                if( normalBlocks == 0 )
                {
                    normalBlocks = 1;
                }

                const size_type blocks = (std::max)( normalBlocks, minimumBlocks );
                if( blocks > maxCount() / blockChars )
                {
                    throw std::bad_array_new_length();
                }

                const size_type slabChars = blocks * blockChars;

                // Reserve metadata before allocating the character slab. This makes
                // deallocation non-allocating for all blocks in the slab.
                bucket.freeBlocks.reserve( bucket.freeBlocks.size() + blocks );
                bucket.slabs.reserve( bucket.slabs.size() + 1 );

                T *slab = Traits::allocate( m_upstream, slabChars );
                try
                {
                    bucket.slabs.push_back( { slab, slabChars } );
                }
                catch( ... )
                {
                    Traits::deallocate( m_upstream, slab, slabChars );
                    throw;
                }

                for( size_type i = 0; i < blocks; ++i )
                {
                    bucket.freeBlocks.push_back( slab + i * blockChars );
                }

                bucket.totalBlocks += blocks;
            }

            static constexpr size_type maxCount() noexcept
            {
                return ( std::numeric_limits<size_type>::max )() / sizeof( T );
            }

            void releaseBucket( Bucket &bucket ) noexcept
            {
                for( const Slab &slab : bucket.slabs )
                {
                    if( slab.data )
                    {
                        Traits::deallocate( m_upstream, slab.data, slab.count );
                    }
                }

                bucket.slabs.clear();
                bucket.freeBlocks.clear();
                bucket.totalBlocks = 0;
                bucket.liveBlocks = 0;
            }

            UpstreamAllocator m_upstream{};
            std::unordered_map<size_type, Bucket> m_buckets;
            std::mutex m_mutex;
            std::atomic<bool> m_destroyed{ false };
        };

        static Pool &pool();
    };

    template <class T, class UpstreamAllocator>
    typename CharacterPoolAllocator<T, UpstreamAllocator>::Pool &
    CharacterPoolAllocator<T, UpstreamAllocator>::pool()
    {
        // C++11+ guarantees thread-safe initialization of local statics.
        static Pool instance;
        return instance;
    }

#if defined( WP_PLATFORM_WIN32 ) && !defined( _WP_STATIC_LIB_ )
    // Strings cross DLL boundaries by value. Their standard character pools must
    // live in Workphone, so the receiving module returns buffers to their owner.
    template <>
    WPCore_API CharacterPoolAllocator<char>::Pool &CharacterPoolAllocator<char>::pool();
    template <>
    WPCore_API CharacterPoolAllocator<wchar_t>::Pool &CharacterPoolAllocator<wchar_t>::pool();
#endif

    template <class T1, class A1, class T2, class A2>
    constexpr bool operator==( const CharacterPoolAllocator<T1, A1> &,
                               const CharacterPoolAllocator<T2, A2> & ) noexcept
    {
        // Each specialization is stateless from the caller's perspective. StringBase
        // only moves memory between equal allocator specializations.
        return std::is_same<T1, T2>::value && std::is_same<A1, A2>::value;
    }

    template <class T1, class A1, class T2, class A2>
    constexpr bool operator!=( const CharacterPoolAllocator<T1, A1> &lhs,
                               const CharacterPoolAllocator<T2, A2> &rhs ) noexcept
    {
        return !( lhs == rhs );
    }
}  // namespace workphone

#endif  // WORKPHONE_CHARACTER_POOL_ALLOCATOR_HPP
