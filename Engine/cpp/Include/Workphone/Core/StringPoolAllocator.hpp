#ifndef StringPoolAllocator_h__
#define StringPoolAllocator_h__

#include <Workphone/Core/StringPool.hpp>
#include <memory>
#include <limits>

namespace workphone
{
    /**
     * A standard allocator adapter for std::string that uses StringPool for memory management.
     * This allows std::basic_string to allocate character buffers from a StringPool.
     */
    template <typename T>
    class StringPoolAllocator
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

        // Rebind structure for allocator conversion
        template <typename U>
        struct rebind
        {
            using other = StringPoolAllocator<U>;
        };

        // Constructors
        WPCore_API StringPoolAllocator() noexcept;

        explicit StringPoolAllocator( StringPool<T> *pool ) noexcept : m_pool( pool )
        {
        }

        // Copy constructor
        StringPoolAllocator( const StringPoolAllocator &other ) noexcept : m_pool( other.m_pool )
        {
        }

        // Rebind copy constructor
        template <typename U>
        StringPoolAllocator( const StringPoolAllocator<U> &other ) noexcept :
            m_pool( reinterpret_cast<StringPool<T> *>( other.getPool() ) )
        {
        }

        // Assignment operator
        StringPoolAllocator &operator=( const StringPoolAllocator &other ) noexcept
        {
            m_pool = other.m_pool;
            return *this;
        }

        // Destructor
        ~StringPoolAllocator() = default;

        /**
         * Allocate memory for n elements
         */
        pointer allocate( size_type n, const void *hint = nullptr )
        {
            if( m_pool == nullptr )
            {
                // Fallback to standard allocation if no pool is set
                return static_cast<pointer>( ::operator new( n * sizeof( T ) ) );
            }

            // For string allocation, we typically allocate the full string buffer
            // The pool manages fixed-size blocks, so we use the pool's create method
            if( n <= m_pool->getMaxStringSize() )
            {
                pointer ptr = m_pool->create( n );
                if( ptr != nullptr )
                {
                    return ptr;
                }
            }

            // If the pool is exhausted or the requested size exceeds max string size,
            // fallback to standard allocation
            return static_cast<pointer>( ::operator new( n * sizeof( T ) ) );
        }

        /**
         * Deallocate memory
         */
        void deallocate( pointer p, size_type n ) noexcept
        {
            if( m_pool == nullptr )
            {
                ::operator delete( p );
                return;
            }

            // Check if the pointer belongs to the pool
            // If it does, return it to the pool; otherwise, use standard deallocation
            m_pool->destroy( p, n );
        }

        /**
         * Construct an object at the given location
         */
        template <typename U, typename... Args>
        void construct( U *p, Args &&...args )
        {
            ::new( static_cast<void *>( p ) ) U( std::forward<Args>( args )... );
        }

        /**
         * Destroy an object at the given location
         */
        template <typename U>
        void destroy( U *p )
        {
            p->~U();
        }

        /**
         * Return the maximum number of elements that can be allocated
         */
        size_type max_size() const noexcept
        {
            if( m_pool != nullptr )
            {
                return m_pool->getMaxStringSize();
            }
            return std::numeric_limits<size_type>::max() / sizeof( T );
        }

        /**
         * Get the underlying string pool
         */
        StringPool<T> *getPool() const noexcept
        {
            return m_pool;
        }

        /**
         * Set the underlying string pool
         */
        void setPool( StringPool<T> *pool ) noexcept
        {
            m_pool = pool;
        }

        // Comparison operators
        bool operator==( const StringPoolAllocator &other ) const noexcept
        {
            return m_pool == other.m_pool;
        }

        bool operator!=( const StringPoolAllocator &other ) const noexcept
        {
            return m_pool != other.m_pool;
        }

        template <typename U>
        bool operator==( const StringPoolAllocator<U> &other ) const noexcept
        {
            return m_pool == reinterpret_cast<StringPool<T> *>( other.getPool() );
        }

        template <typename U>
        bool operator!=( const StringPoolAllocator<U> &other ) const noexcept
        {
            return m_pool != reinterpret_cast<StringPool<T> *>( other.getPool() );
        }

    private:
        StringPool<T> *m_pool;
    };

}  // namespace workphone

#endif  // StringPoolAllocator_h__
