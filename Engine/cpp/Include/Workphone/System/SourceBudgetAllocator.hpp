#ifndef WP_SOURCE_BUDGET_ALLOCATOR_HPP
#define WP_SOURCE_BUDGET_ALLOCATOR_HPP

#include <Workphone/System/WorldStreamingScheduler.hpp>
#include <limits>
#include <new>
#include <type_traits>

namespace workphone
{
    /** Allocation-local SourceCpu ownership, including the budget header and
     * alignment padding. Retained capacity remains charged until physical free. */
    WPCore_API void *allocateSourceBudgetStorage( size_t bytes, size_t alignment );
    WPCore_API void freeSourceBudgetStorage( void *pointer, size_t alignment ) noexcept;
    WPCore_API size_t sourceBudgetStorageBytes( size_t bytes, size_t alignment );

    template <class T> class SourceBudgetAllocator
    {
    public:
        using value_type = T;
        using is_always_equal = std::true_type;
        using propagate_on_container_move_assignment = std::true_type;
        template <class U> struct rebind { using other = SourceBudgetAllocator<U>; };
        SourceBudgetAllocator() noexcept = default;
        template <class U> SourceBudgetAllocator( const SourceBudgetAllocator<U> & ) noexcept {}
        T *allocate( size_t count )
        {
            if( count > std::numeric_limits<size_t>::max() / sizeof( T ) ) throw std::bad_alloc();
            return static_cast<T *>( allocateSourceBudgetStorage( count * sizeof( T ), alignof( T ) ) );
        }
        void deallocate( T *pointer, size_t ) noexcept { freeSourceBudgetStorage( pointer, alignof( T ) ); }
        template <class U> bool operator==( const SourceBudgetAllocator<U> & ) const noexcept { return true; }
        template <class U> bool operator!=( const SourceBudgetAllocator<U> & ) const noexcept { return false; }
    };
}
#endif
