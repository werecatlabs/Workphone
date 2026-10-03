#ifndef StringPool_h__
#define StringPool_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <vector>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <stdexcept>

namespace workphone
{

    /**
     * A global pool for managing and reusing string instances.
     */
    template <typename T>
    class StringPool
    {
    public:
        // Type definitions
        using value_type = T;
        using size_type = std::size_t;

        // Constructor
        StringPool() = default;

        StringPool( u32 capacity, u32 maxStringSize = 128 )
        {
            validateMaxStringSize( maxStringSize );
            m_data = new T[capacity];
            m_poolSize = capacity;
            m_maxStringSize = maxStringSize;
            rebuildFreeList();
        }

        // Destructor
        ~StringPool()
        {
            delete[] m_data;
        }

        void resize( size_type n )
        {
            if( n > m_poolSize )
            {
                delete[] m_data;
                m_data = new T[n];
                m_poolSize = n;
                rebuildFreeList();
            }
        }

        T *create( const T *str, size_type n )
        {
            // Allocate a block from the pool
            T *ptr = allocateStr();
            if( ptr == nullptr )
                return nullptr;

            // Calculate how much we can safely copy
            // Reserve one element for the null-terminator
            size_type maxCopy = ( m_maxStringSize > 1 ) ? ( m_maxStringSize - 1 ) : 0;
            size_type toCopy = ( str != nullptr ) ? std::min( n, maxCopy ) : 0;

            // Copy the string data
            if( str != nullptr && toCopy > 0 )
            {
                std::memcpy( ptr, str, toCopy * sizeof( T ) );
            }

            // Null-terminate the string
            ptr[toCopy] = T();

            return ptr;
        }

        T *create( size_type n )
        {
            T *ptr = allocateStr();
            if( ptr != nullptr )
            {
                // Initialize with null terminator
                ptr[0] = T();
            }
            return ptr;
        }

        void destroy( T *p )
        {
            // Add the pointer to the free list for reuse
            const auto address = reinterpret_cast<std::uintptr_t>( p );
            const auto beginAddress = reinterpret_cast<std::uintptr_t>( m_data );
            const auto byteSize = static_cast<std::uintptr_t>( m_poolSize ) * sizeof( T );
            const auto blockSize = static_cast<std::uintptr_t>( m_maxStringSize ) * sizeof( T );
            if( p != nullptr && m_data != nullptr && address >= beginAddress &&
                address - beginAddress < byteSize && ( address - beginAddress ) % blockSize == 0 &&
                std::find( m_freeList.begin(), m_freeList.end(), p ) == m_freeList.end() )
            {
                m_freeList.push_back( p );
            }
        }

        void destroy( T *p, size_type n )
        {
            destroy( p );
        }

        void reset()
        {
            rebuildFreeList();
        }

        u32 getMaxStringSize() const
        {
            return m_maxStringSize;
        }

        void setMaxStringSize( u32 maxStringSize )
        {
            validateMaxStringSize( maxStringSize );
            m_maxStringSize = maxStringSize;
            rebuildFreeList();
        }

    private:
        static void validateMaxStringSize( u32 maxStringSize )
        {
            if( maxStringSize == 0 )
                throw std::invalid_argument( "StringPool maximum string size cannot be zero" );
        }

        void rebuildFreeList()
        {
            m_freeList.clear();
            if( m_data == nullptr || m_poolSize < m_maxStringSize )
                return;

            for( size_type i = 0; i <= m_poolSize - m_maxStringSize; i += m_maxStringSize )
                m_freeList.push_back( &m_data[i] );
        }

        T *allocateStr()
        {
            if( !m_freeList.empty() )
            {
                T *ptr = m_freeList.back();
                m_freeList.pop_back();
                return ptr;
            }

            return nullptr;  // Pool exhausted
        }

        T *m_data = nullptr;
        u32 m_poolSize = 0;
        u32 m_maxStringSize = 32;
        std::vector<T *> m_freeList;
    };

}  // namespace workphone

#endif  // StringPool_h__
