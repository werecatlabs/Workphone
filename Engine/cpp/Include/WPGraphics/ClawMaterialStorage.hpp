#ifndef WP_CLAW_MATERIAL_STORAGE_HPP
#define WP_CLAW_MATERIAL_STORAGE_HPP

#include <Workphone/System/SourceBudgetAllocator.hpp>
#include <array>
#include <cassert>
#include <cstdint>
#include <limits>
#include <memory_resource>
#include <new>
#include <type_traits>

namespace workphone::render::detail
{
    /** Typed boundary for a material workspace's memory resource. The resource
     * owns every payload allocation; the existing SourceBudgetAllocator policy
     * handles MSVC Debug iterator bookkeeping before its type is erased by PMR.
     * Containers and their resource require owning-thread use or synchronization.
     */
    template<class T> class MaterialStorageAllocator
    {
    public:
        using value_type = T;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;
        using is_always_equal = std::false_type;
        template<class U> struct rebind { using other = MaterialStorageAllocator<U>; };

        MaterialStorageAllocator() noexcept : m_resource( std::pmr::get_default_resource() ) {}
        MaterialStorageAllocator( std::pmr::memory_resource *resource ) noexcept : m_resource( resource )
        { assert( resource ); }
        template<class U> MaterialStorageAllocator( const MaterialStorageAllocator<U> &other ) noexcept :
            m_resource( other.resource() ) {}

        T *allocate( size_t count )
        {
            if( count > std::numeric_limits<size_t>::max() / sizeof( T ) ) throw std::bad_alloc();
#if defined( _MSC_VER ) && _ITERATOR_DEBUG_LEVEL != 0
            // MSVC vector/hash bookkeeping can allocate inside noexcept
            // constructors. Preserve the same narrow exemption as the typed
            // engine allocator; real payloads always use the supplied resource.
            if constexpr( std::is_same_v<T, std::_Container_proxy> )
                return SourceBudgetAllocator<T>{}.allocate( count );
#endif
            return static_cast<T *>( m_resource->allocate( count * sizeof( T ), alignof( T ) ) );
        }

        void deallocate( T *pointer, size_t count ) noexcept
        {
            if( !pointer ) return;
#if defined( _MSC_VER ) && _ITERATOR_DEBUG_LEVEL != 0
            if constexpr( std::is_same_v<T, std::_Container_proxy> )
            {
                SourceBudgetAllocator<T>{}.deallocate( pointer, count );
                return;
            }
#endif
            m_resource->deallocate( pointer, count * sizeof( T ), alignof( T ) );
        }

        std::pmr::memory_resource *resource() const noexcept { return m_resource; }
        template<class U> bool operator==( const MaterialStorageAllocator<U> &other ) const noexcept
        {
            return m_resource == other.resource() || m_resource->is_equal( *other.resource() );
        }
        template<class U> bool operator!=( const MaterialStorageAllocator<U> &other ) const noexcept
        { return !( *this == other ); }
    private:
        std::pmr::memory_resource *m_resource;
    };

    /** Lazy owning-thread pool for small material lookup nodes. Construction
     * and returns allocate nothing. Intrusive free lists and a fixed directory
     * avoid allocating STL metadata inside a noexcept resource constructor.
     * Slabs remain with this owner until destruction and are charged upstream
     * before publication. Larger or over-aligned requests retain upstream policy.
     */
    class MaterialStoragePool final : public std::pmr::memory_resource
    {
    public:
        explicit MaterialStoragePool( std::pmr::memory_resource &upstream ) noexcept : m_upstream( upstream ) {}
        ~MaterialStoragePool() override
        {
            while( m_slabs )
            {
                auto *slab = m_slabs;
                const auto bytes = slab->bytes;
                m_slabs = slab->next;
                slab->~Slab();
                m_upstream.deallocate( slab, bytes, Alignment );
            }
        }
        MaterialStoragePool( const MaterialStoragePool & ) = delete;
        MaterialStoragePool &operator=( const MaterialStoragePool & ) = delete;
    private:
        static constexpr size_t Alignment = 16;
        static constexpr size_t LargestBlock = 512;
        static constexpr size_t BinCount = LargestBlock / Alignment;
        struct FreeNode { FreeNode *next; };
        struct Slab { Slab *next; size_t bytes; };
        static constexpr size_t SlabOffset = ( sizeof( Slab ) + Alignment - 1 ) & ~( Alignment - 1 );
        static_assert( sizeof( FreeNode ) <= Alignment && alignof( FreeNode ) <= Alignment );
        static_assert( alignof( Slab ) <= Alignment );

        static bool pooled( size_t bytes, size_t alignment ) noexcept
        { return bytes <= LargestBlock && alignment <= Alignment; }
        static size_t blockSize( size_t bytes ) noexcept
        {
            if( bytes < sizeof( FreeNode ) ) bytes = sizeof( FreeNode );
            return ( bytes + Alignment - 1 ) & ~( Alignment - 1 );
        }

        void *do_allocate( size_t bytes, size_t alignment ) override
        {
            if( !alignment || ( alignment & ( alignment - 1 ) ) ) throw std::bad_alloc();
            if( !pooled( bytes, alignment ) ) return m_upstream.allocate( bytes, alignment );
            const auto stride = blockSize( bytes );
            const auto bin = stride / Alignment - 1;
            if( auto *free = m_free[bin] )
            {
                m_free[bin] = free->next;
                return free;
            }

            const size_t count = m_nextCount[bin] ? m_nextCount[bin] : 4;
            if( stride > ( std::numeric_limits<size_t>::max() - SlabOffset ) / count ) throw std::bad_alloc();
            const auto total = SlabOffset + count * stride;
            auto *storage = static_cast<unsigned char *>( m_upstream.allocate( total, Alignment ) );
            auto *slab = ::new( storage ) Slab{ m_slabs, total };
            FreeNode *free = nullptr;
            for( size_t index = count; index != 0; --index )
                free = ::new( storage + SlabOffset + ( index - 1 ) * stride ) FreeNode{ free };

            // Allocation and all non-throwing initialization finish before the
            // new slab/free slots become visible to subsequent operations.
            m_slabs = slab;
            m_nextCount[bin] = static_cast<std::uint8_t>( count < 32 ? count * 2 : 32 );
            m_free[bin] = free->next;
            return free;
        }

        void do_deallocate( void *pointer, size_t bytes, size_t alignment ) noexcept override
        {
            if( !pointer ) return;
            assert( alignment && !( alignment & ( alignment - 1 ) ) );
            if( !pooled( bytes, alignment ) )
            {
                m_upstream.deallocate( pointer, bytes, alignment );
                return;
            }
            const auto bin = blockSize( bytes ) / Alignment - 1;
            m_free[bin] = ::new( pointer ) FreeNode{ m_free[bin] };
        }

        bool do_is_equal( const std::pmr::memory_resource &other ) const noexcept override
        { return this == &other; }

        std::pmr::memory_resource &m_upstream;
        Slab *m_slabs = nullptr;
        std::array<FreeNode *, BinCount> m_free{};
        std::array<std::uint8_t, BinCount> m_nextCount{};
    };
}
#endif
