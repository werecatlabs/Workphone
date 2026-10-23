#ifndef StringPool_h__
#define StringPool_h__

#include <Workphone/WorkphoneTypes.hpp>
#include <Workphone/System/SourceBudgetAllocator.hpp>
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
            m_maxStringSize = maxStringSize;
            resize(capacity);
        }

        // Destructor
        ~StringPool() = default;
        StringPool(const StringPool &) = delete;
        StringPool &operator=(const StringPool &) = delete;

        void resize( size_type n )
        {
            if( n > m_poolSize )
            {
                // Keep the old pool and free list intact if either admission
                // fails. Successful resize retains the existing reset semantics.
                decltype(m_storage) storage(n);
                decltype(m_freeList) freeList;
                freeList.reserve(n / m_maxStringSize);
                for(size_type i = 0; i < n / m_maxStringSize; ++i)
                    freeList.push_back(storage.data() + i * m_maxStringSize);
                m_storage.swap(storage);
                m_freeList.swap(freeList);
                m_data = m_storage.data();
                m_poolSize = n;
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
            // Reserve before changing the block size or resetting live entries.
            m_freeList.reserve(m_poolSize / maxStringSize);
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
            m_freeList.reserve(m_poolSize / m_maxStringSize);
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
        size_type m_poolSize = 0;
        u32 m_maxStringSize = 32;
        std::vector<T, SourceBudgetAllocator<T>> m_storage;
        std::vector<T *, SourceBudgetAllocator<T *>> m_freeList;
    };

}  // namespace workphone

#endif  // StringPool_h__
