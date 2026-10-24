#ifndef WP_SOURCE_BUDGET_ALLOCATOR_HPP
#define WP_SOURCE_BUDGET_ALLOCATOR_HPP

#include <Workphone/WorkphoneTypes.hpp>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>

namespace workphone
{
    /** Allocation-local SourceCpu ownership, including the budget header and
     * alignment padding. Retained capacity remains charged until physical free. */
    WPCore_API void *allocateSourceBudgetStorage( size_t bytes, size_t alignment );
    WPCore_API void freeSourceBudgetStorage( void *pointer, size_t alignment ) noexcept;
    WPCore_API size_t sourceBudgetStorageBytes( size_t bytes, size_t alignment );
    // Per-thread epoch lets a staged operation detect denial swallowed by a
    // legacy bool/null-result adapter before publishing an incomplete snapshot.
    WPCore_API u64 sourceBudgetDenialCount() noexcept;

    // Opt-in policy for asset wrappers and authoring nodes. Factory and pool
    // storage use the same marker, including unused slots retained by a pool.
    struct SourceBudgetObject
    {
        static void *operator new( size_t bytes ) { return allocateSourceBudgetStorage(bytes, alignof(std::max_align_t)); }
        static void operator delete( void *p ) noexcept { freeSourceBudgetStorage(p, alignof(std::max_align_t)); }
        static void *operator new( size_t, void *p ) noexcept { return p; }
        static void operator delete( void *, void * ) noexcept {}
        // Diagnostic placement construction also borrows its caller's slot.
        static void *operator new( size_t, void *p, const char *, int, const char * ) noexcept { return p; }
        static void operator delete( void *, void *, const char *, int, const char * ) noexcept {}
        static void *operator new( size_t bytes, int, const char *, int ) { return operator new(bytes); }
        static void operator delete( void *p, int, const char *, int ) noexcept { operator delete(p); }
        static void *operator new( size_t bytes, std::align_val_t a ) { return allocateSourceBudgetStorage(bytes, size_t(a)); }
        static void operator delete( void *p, std::align_val_t a ) noexcept { freeSourceBudgetStorage(p, size_t(a)); }
    };

    template <class T> class SourceBudgetAllocator
    {
    public:
        using value_type = T;
        using size_type = size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using const_pointer = const T *;
        using is_always_equal = std::true_type;
        using propagate_on_container_move_assignment = std::true_type;
        template <class U> struct rebind { using other = SourceBudgetAllocator<U>; };
        SourceBudgetAllocator() noexcept = default;
        template <class U> SourceBudgetAllocator( const SourceBudgetAllocator<U> & ) noexcept {}
        T *allocate( size_t count )
        {
            if( count > std::numeric_limits<size_t>::max() / sizeof( T ) ) throw std::bad_alloc();
#if defined( _MSC_VER ) && _ITERATOR_DEBUG_LEVEL != 0
            // MSVC creates iterator proxies inside unconditional noexcept
            // container constructors/moves. Budget denial there would terminate
            // instead of propagating bad_alloc from a payload allocation. Only
            // this Debug STL bookkeeping is excluded from the SourceCpu ledger;
            // physical heap profiling still includes it.
            if constexpr( std::is_same_v<T, std::_Container_proxy> )
                return std::allocator<T>{}.allocate( count );
#endif
            return static_cast<T *>( allocateSourceBudgetStorage( count * sizeof( T ), alignof( T ) ) );
        }
        void deallocate( T *pointer, size_t count ) noexcept
        {
#if defined( _MSC_VER ) && _ITERATOR_DEBUG_LEVEL != 0
            if constexpr( std::is_same_v<T, std::_Container_proxy> )
            {
                std::allocator<T>{}.deallocate( pointer, count );
                return;
            }
#endif
            (void)count;
            freeSourceBudgetStorage( pointer, alignof( T ) );
        }
        template <class U> bool operator==( const SourceBudgetAllocator<U> & ) const noexcept { return true; }
        template <class U> bool operator!=( const SourceBudgetAllocator<U> & ) const noexcept { return false; }
    };
}
#endif
