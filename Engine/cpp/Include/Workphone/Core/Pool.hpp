#ifndef WP_POOL_H
#define WP_POOL_H

#include <Workphone/Core/Allocator.hpp>
#include <Workphone/Thread/Thread.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/PoolData.hpp>
#include <Workphone/System/RttiClass.hpp>
#include <Workphone/System/RttiClassDefinition.hpp>

namespace workphone
{

    /**
     * @brief A thread-safe memory pool implementation for efficient object allocation and deallocation.
     *
     * The Pool class provides a memory management system that pre-allocates objects and reuses them,
     * reducing the overhead of frequent memory allocations and deallocations. It is designed to be
     * thread-safe and efficient for high-performance applications.
     *
     * @tparam T The type of objects to be stored in the pool
     * @tparam A The allocator type to use for memory allocation (defaults to std::allocator<T>)
     */
    template <class T, class A = Allocator<T>>
    class Pool : public ISharedObject
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Creates a new pool with the default initial size of 32 elements.
         */
        Pool();

        /**
         * @brief Constructor with custom initial size.
         *
         * @param nextSize The initial number of elements to allocate in the pool
         */
        explicit Pool( size_Num nextSize );

        /**
         * @brief Destructor.
         *
         * Cleans up all allocated memory and resources.
         */
        ~Pool() override;

        /**
         * @brief Allocates a new block of memory in the pool.
         *
         * This method creates a new PoolData object and allocates memory for the specified
         * number of elements. The allocated memory is added to the free elements queue.
         */
        void allocateData();

        /**
         * @brief Allocates a single object from the pool.
         *
         * If no free objects are available, this method will automatically allocate more memory.
         *
         * @return A pointer to the allocated object, or nullptr if allocation fails
         */
        RawPtr<T> allocate_object();

        /**
         * @brief Returns an object back to the pool.
         *
         * @param ptr Pointer to the object being returned to the pool
         */
        void free_object( T *ptr );

        /**
         * @brief Clears all allocated memory in the pool.
         *
         * This method releases all allocated objects and resets the pool to its initial state.
         */
        void clear();

        /**
         * @brief Gets the size of the next allocation block.
         *
         * @return The number of elements that will be allocated in the next allocation
         */
        size_Num getNextSize() const;

        /**
         * @brief Sets the size of the next allocation block.
         *
         * @param nextSize The number of elements to allocate in the next allocation
         */
        void setNextSize( size_Num nextSize );

        /**
         * @brief Gets the current number of allocation blocks in the pool.
         *
         * @return The number of PoolData blocks currently allocated
         */
        size_Num getSize() const;

        WP_CLASS_REGISTER_TEMPLATE_PAIR_DECL( Pool, T, A );

    private:
        /// Array of PoolData objects that manage the actual memory blocks
        ConcurrentArray<SmartPtr<PoolData<T, A>>> m_elements;

        /// Queue of available (free) objects that can be allocated
        ConcurrentQueue<RawPtr<T>> m_freeElements;

        /// Number of elements to allocate in the next allocation block
        size_Num m_nextSize = 32;
    };

    WP_CLASS_REGISTER_DERIVED_TEMPLATE_PAIR_TYPEID( workphone, Pool, T, A, ISharedObject );

    template <class T, class A>
    Pool<T, A>::Pool() = default;

    template <class T, class A>
    Pool<T, A>::Pool( size_Num nextSize ) : m_nextSize( nextSize )
    {
    }

    template <class T, class A>
    Pool<T, A>::~Pool()
    {
        auto elements = m_elements.snapshot();
        for( auto &element : elements )
        {
            if( element )
            {
                element->unload( nullptr );
            }
        }

        m_elements.clear();
    }

    template <class T, class A>
    void Pool<T, A>::allocateData()
    {
        auto nextSize = getNextSize();
        if( nextSize == 0 )
            return;

        auto element = workphone::make_ptr<PoolData<T, A>>();
        element->setNumElements( nextSize );
        element->load( nullptr );
        m_elements.push_back( element );

        for( size_t i = 0; i < element->getNumAllocated(); ++i )
            m_freeElements.push( &( ( *element )[i] ) );
    }

    template <class T, class A>
    RawPtr<T> Pool<T, A>::allocate_object()
    {
        if( m_freeElements.empty() )
        {
            allocateData();
        }

        if( !m_freeElements.empty() )
        {
            RawPtr<T> ptr;
            while( !m_freeElements.try_pop( ptr ) )
            {
                Thread::yield();
            }

            return ptr;
        }

        return nullptr;
    }

    template <class T, class A>
    void Pool<T, A>::free_object( T *ptr )
    {
        m_freeElements.push( ptr );
    }

    template <class T, class A>
    void Pool<T, A>::clear()
    {
        while( !m_freeElements.empty() )
        {
            RawPtr<T> ptr;
            if( m_freeElements.try_pop( ptr ) )
            {
                Thread::yield();
            }
        }

        for( size_t i = 0; i < m_elements.size(); ++i )
        {
            m_elements[i] = nullptr;
        }

        m_freeElements.clear();
        m_elements.clear();
    }

    template <class T, class A>
    size_Num Pool<T, A>::getNextSize() const
    {
        return m_nextSize;
    }

    template <class T, class A>
    void Pool<T, A>::setNextSize( size_Num nextSize )
    {
        m_nextSize = nextSize;
    }

    template <class T, class A>
    size_Num Pool<T, A>::getSize() const
    {
        return m_elements.size();
    }

}  // namespace workphone

#endif
