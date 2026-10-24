#ifndef Allocator_h__
#define Allocator_h__

#include <cassert>
#include <cstddef>
#include <limits>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <Workphone/System/SourceBudgetAllocator.hpp>

// Static assertions to validate type requirements at compile time
static_assert( std::is_nothrow_default_constructible<std::size_t>::value,
               "std::size_t must be nothrow default constructible" );
static_assert( std::is_nothrow_default_constructible<std::ptrdiff_t>::value,
               "std::ptrdiff_t must be nothrow default constructible" );

namespace workphone
{
    struct SourceBudgetTag {};
    inline constexpr SourceBudgetTag sourceBudget{};

    namespace detail
    {
        struct ContainerAllocationPolicy
        {
            void *(*allocate)(size_t, size_t);
            void (*deallocate)(void *, size_t) noexcept;
        };
        // The policy lives in WPCore, so retained snapshots never point into
        // a producing plugin's static data after that plugin is unloaded.
        WPCore_API const ContainerAllocationPolicy &sourceContainerPolicy() noexcept;
    }

    /**
     * @brief Standard-compatible allocator base template.
     *
     * Provides the full named-requirements interface expected by
     * std::allocator_traits and by the engine's own containers
     * (ArrayBase, MapBase, etc.). Allocation is performed through
     * global operator new / operator delete by default. An explicit sourceBudget
     * tag selects shared SourceCpu admission and follows container ownership.
     *
     * Derived allocators (pool allocators, aligned allocators, …)
     * can override allocate / deallocate while inheriting the
     * boilerplate type definitions and construct / destroy helpers.
     *
     * @section Complexity
     * - allocate(): O(1) - just memory allocation
     * - deallocate(): O(1) - just memory deallocation
     * - construct(): O(1) - placement new
     * - destroy(): O(1) - destructor call
     * - max_size(): O(1)
     * - address(): O(1)
     *
     * @section ExceptionSafety
     * - allocate(): Strong exception guarantee (throws std::bad_alloc on failure)
     * - deallocate(): Nothrow guarantee
     * - construct(): Strong exception guarantee
     * - destroy(): Nothrow guarantee
     *
     * @tparam T Value type that this allocator manages.
     */
    template <typename T>
    class AllocatorBase
    {
    public:
        // Standard allocator type definitions
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using pointer = T *;
        using const_pointer = const T *;
        using reference = T &;
        using const_reference = const T &;
        using propagate_on_container_move_assignment = std::true_type;
        using propagate_on_container_copy_assignment = std::true_type;
        using propagate_on_container_swap = std::true_type;
        using is_always_equal = std::false_type;

        /**
         * @brief Rebind structure for allocator conversion.
         *
         * Allows conversion between allocators for different types,
         * which is required by std::allocator_traits.
         */
        template <typename U>
        struct rebind
        {
            /// @brief The allocator type for the rebound type U.
            using other = AllocatorBase<U>;
        };

        /** @name Constructors and destructor
         * @{
         */

        /**
         * @brief Default constructor.
         *
         * Constructs a raw allocator. Tagged snapshots opt into SourceCpu.
         */
        AllocatorBase() noexcept = default;
        explicit AllocatorBase( SourceBudgetTag ) noexcept :
            m_policy( &detail::sourceContainerPolicy() ) {}
        bool isSourceBudgeted() const noexcept { return m_policy != nullptr; }
        AllocatorBase &operator=( const AllocatorBase &other ) noexcept
        {
            m_policy = other.m_policy;
            return *this;
        }

        /**
         * @brief Copy constructor.
         *
         * Allocators are stateless, so copying is trivial.
         *
         * @param other The allocator to copy.
         */
        AllocatorBase( const AllocatorBase & ) noexcept = default;

        /**
         * @brief Rebind copy constructor.
         *
         * Allows constructing an allocator for type U from an allocator for type T.
         * Allocators are stateless, so this is always valid.
         *
         * @tparam U The value type of the other allocator.
         * @param other The allocator to copy.
         */
        template <typename U>
        AllocatorBase( const AllocatorBase<U> &other ) noexcept : m_policy(other.m_policy)
        {
        }

        /**
         * @brief Destructor.
         *
         * Allocators are stateless, so destruction is trivial.
         */
        ~AllocatorBase() = default;

        /** @} */

        /** @name Allocation and deallocation
         * @{
         */

        /**
         * @brief Allocate storage for n objects of type T.
         *
         * Allocates enough memory to hold n objects of type T using
         * the global operator new. The memory is uninitialized.
         *
         * @param n Number of objects to allocate storage for.
         * @return Pointer to the first element of the allocated block.
         * @throws std::bad_alloc if allocation fails or if n exceeds max_size().
         * @note Time complexity: O(1) for the allocation itself.
         *
         * @par Edge Cases
         * - n == 0: Returns nullptr
         * - n > max_size(): Throws std::bad_alloc
         * - sizeof(T) == 0: Handled as special case
         */
        pointer allocate( size_type n )
        {
            // Handle edge case: zero allocation
            if( n == 0 )
            {
                return nullptr;
            }

            // Check for requested size exceeding maximum
            if( n > max_size() )
            {
                throw std::bad_alloc();
            }

            // Handle edge case: sizeof(T) == 0
            // In this case, we still need to return a valid pointer
            if( sizeof( T ) == 0 )
            {
                return static_cast<pointer>( ::operator new( 1 ) );
            }

            // Check for multiplication overflow before allocating
            if( n > std::numeric_limits<size_type>::max() / sizeof( T ) )
            {
                throw std::bad_alloc();
            }

            const size_type bytes = n * sizeof( T );
#if defined( _MSC_VER ) && _ITERATOR_DEBUG_LEVEL != 0
            // As with SourceBudgetAllocator, MSVC's noexcept iterator-proxy
            // constructors cannot propagate budget denial. Exclude only proxies.
            if constexpr( std::is_same_v<T, std::_Container_proxy> )
                return std::allocator<T>{}.allocate(n);
#endif
            if( m_policy )
                return static_cast<pointer>( m_policy->allocate(bytes, alignof(T)) );

            // Final validation before allocation
            if( bytes == 0 )
            {
                return nullptr;
            }

            void *ptr = ::operator new( bytes );

            // Verify allocation succeeded (operator new throws on failure, but be defensive)
            if( !ptr )
            {
                throw std::bad_alloc();
            }

            return static_cast<pointer>( ptr );
        }

        /**
         * @brief Allocate storage with a locality hint.
         *
         * The hint parameter is ignored but provided for interface
         * compatibility with std::allocator and std::allocator_traits.
         *
         * @param n Number of objects to allocate storage for.
         * @param hint Locality hint (unused, may be nullptr).
         * @return Pointer to the first element of the allocated block.
         * @throws std::bad_alloc if allocation fails.
         */
        pointer allocate( size_type n, const void * /*hint*/ ) noexcept( noexcept( allocate( n ) ) )
        {
            return allocate( n );
        }

        /**
         * @brief Return storage previously obtained from allocate.
         *
         * Deallocates the memory pointed to by p, which must have been
         * returned by a previous call to allocate(). The number of
         * elements must match the original allocate call.
         *
         * @param p Pointer previously returned by allocate.
         * @param n Number of objects the block was allocated for (unused in this implementation).
         * @note Time complexity: O(1).
         * @note This function is guaranteed not to throw.
         *
         * @par Edge Cases
         * - p == nullptr: Safe to call, no-op
         * - n == 0: Safe to call, no-op
         */
        void deallocate( pointer p, size_type n ) noexcept
        {
#if defined( _MSC_VER ) && _ITERATOR_DEBUG_LEVEL != 0
            if constexpr( std::is_same_v<T, std::_Container_proxy> )
            { std::allocator<T>{}.deallocate(p, n); return; }
#endif
            (void)n;
            if( p != nullptr )
            {
                if( m_policy ) { m_policy->deallocate(p, alignof(T)); return; }
                ::operator delete( static_cast<void *>( p ) );
            }
        }

        /** @} */

        /** @name Construction / destruction helpers
         * @{
         */

        /**
         * @brief Construct an object at p by forwarding arguments.
         *
         * Uses placement new to construct an object of type U at the
         * given memory location, forwarding the provided arguments to
         * the constructor.
         *
         * @tparam U Type to construct.
         * @tparam Args Constructor argument types.
         * @param p Pointer to uninitialized storage (must be properly aligned).
         * @param args Arguments forwarded to the constructor.
         * @throws Any exception thrown by U's constructor.
         * @note Time complexity: O(1) for the construction itself.
         *
         * @par Edge Cases
         * - p == nullptr: Undefined behavior - caller must ensure valid pointer
         * - args is empty: Calls default constructor of U
         */
        template <typename U, typename... Args>
        void construct( U *p, Args &&...args )
        {
            // Defensive check: ensure pointer is not null
            assert( p != nullptr && "construct: pointer must not be null" );

            // Use placement new with perfect forwarding
            // std::forward ensures correct value category for each argument
            ::new( static_cast<void *>( p ) ) U( std::forward<Args>( args )... );
        }

        /**
         * @brief Destroy the object at p without freeing memory.
         *
         * Calls the destructor of the object at p without deallocating
         * the underlying memory. This is useful when the memory will
         * be reused or when using custom memory pools.
         *
         * @tparam U Type to destroy.
         * @param p Pointer to an object previously constructed via construct.
         * @note Time complexity: O(1) for the destruction itself.
         * @note This function is guaranteed not to throw.
         *
         * @par Edge Cases
         * - p == nullptr: Undefined behavior - caller must ensure valid pointer
         */
        template <typename U>
        void destroy( U *p ) noexcept
        {
            // Defensive check in debug builds
            assert( p != nullptr && "destroy: pointer must not be null" );

            // Call destructor - guaranteed not to throw for well-behaved types
            p->~U();
        }

        /** @} */

        /** @name Capacity and utility
         * @{
         */

        /**
         * @brief Return the maximum theoretically allocatable number of objects.
         *
         * Returns the maximum number of objects of type T that could
         * theoretically be allocated, based on the size of T and the
         * maximum value of size_type.
         *
         * @return Maximum allocatable number of T objects.
         * @note Time complexity: O(1).
         * @note This function is guaranteed not to throw.
         */
        size_type max_size() const noexcept
        {
            // Calculate maximum allocation size
            // Use numeric_limits to handle platform differences
            return std::numeric_limits<size_type>::max() / sizeof( T );
        }

        /**
         * @brief Obtain the address of a reference.
         *
         * Returns a pointer to the object referenced by x.
         * This is provided for compatibility with the standard allocator interface.
         *
         * @param x Reference whose address is to be obtained.
         * @return Pointer to the same object as x.
         * @note Time complexity: O(1).
         * @note This function is guaranteed not to throw.
         */
        pointer address( reference x ) const noexcept
        {
            return std::addressof( x );
        }

        /**
         * @brief Obtain the address of a const reference.
         *
         * Returns a const pointer to the object referenced by x.
         * This is provided for compatibility with the standard allocator interface.
         *
         * @param x Const reference whose address is to be obtained.
         * @return Const pointer to the same object as x.
         * @note Time complexity: O(1).
         * @note This function is guaranteed not to throw.
         */
        const_pointer address( const_reference x ) const noexcept
        {
            return std::addressof( x );
        }

        /** @} */

    private:
        template<class> friend class AllocatorBase;
        // Raw by default. Snapshot producers explicitly select SourceCpu;
        // copying/rebinding/moving the allocator preserves physical ownership.
        const detail::ContainerAllocationPolicy *m_policy = nullptr;
    };

    /**
     * @name Equality comparison operators
     * @{
     */

    /**
     * @brief Equality comparison for allocators.
     *
     * Instances compare equal when they use the same allocation policy.
     *
     * @tparam T Value type of the first allocator.
     * @tparam U Value type of the second allocator.
     * @return Whether storage can be exchanged without changing its policy.
     */
    template <typename T, typename U>
    bool operator==( const AllocatorBase<T> &a, const AllocatorBase<U> &b ) noexcept
    {
        return a.isSourceBudgeted() == b.isSourceBudgeted();
    }

    /**
     * @brief Inequality comparison for allocators.
     *
     * Raw storage and admitted storage require different deallocation paths.
     *
     * @tparam T Value type of the first allocator.
     * @tparam U Value type of the second allocator.
     * @return Whether the policies differ.
     */
    template <typename T, typename U>
    bool operator!=( const AllocatorBase<T> &a, const AllocatorBase<U> &b ) noexcept
    {
        return !(a == b);
    }

    /** @} */

    /**
     * @brief Convenience alias for the standard allocator type.
     *
     * This alias provides a simpler way to declare allocator instances
     * without explicitly specifying the template parameter.
     *
     * @tparam T The value type that the allocator manages.
     *
     * @par Example
     * @code
     * Allocator<int> intAllocator;  // Allocator for int
     * Allocator<double> dblAllocator;  // Allocator for double
     * @endcode
     */
    template <typename T>
    using Allocator = AllocatorBase<T>;

}  // namespace workphone

#endif  // Allocator_h__
