#ifndef InstancePool_h__
#define InstancePool_h__

#include <Workphone/Interface/Memory/ISharedObject.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/Memory/ISharedObjectListener.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/ConcurrentQueue.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Memory/AtomicSharedPtr.hpp>
#include <Workphone/Memory/RawPtr.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    /**
     * @brief A thread-safe object pool for managing instances of type T.
     *
     * The pool allocates contiguous blocks of T (via new[]) and exposes individual
     * elements as SmartPtr<T>. Each SmartPtr<T> returned from the pool has its
     * ISharedObjectListener set to an internal Listener which returns the object
     * to the pool when the SmartPtr's reference count goes to zero (unless the
     * pool is shutting down).
     *
     * Template parameter:
     * @tparam T The element type stored in the pool. T must be default-constructible
     *           and compatible with SmartPtr/RawPtr semantics used in this codebase.
     *
     * Thread-safety:
     * - ConcurrentArray and ConcurrentQueue are used to allow concurrent access to
     *   pool metadata and free lists.
     * - The pool is intended for concurrent get/free operations from multiple threads.
     *
     * Ownership/Lifetime:
     * - The pool owns the allocated blocks (m_pool); elements themselves are not
     *   individually delete[]'d until pool destruction.
     * - Returned SmartPtr<T> objects are created from addresses of elements inside
     *   those blocks. The Listener prevents elements from being destroyed while
     *   returning them to the pool.
     */
    template <class T>
    class InstancePool : public ISharedObject
    {
    public:
        /**
         * @brief Listener used to detect when a pooled object is released.
         *
         * The Listener implements ISharedObjectListener::destroy and calls
         * InstancePool::freeInstance to put the object back into the free queue.
         * The Listener holds a RawPtr back to the owning InstancePool.
         */
        class Listener : public ISharedObjectListener
        {
        public:
            Listener();
            explicit Listener( InstancePool *owner );
            Listener( const Listener &other );
            ~Listener() override;

            /**
             * @brief Called by shared-object machinery when the object is being destroyed.
             *
             * The pointer provided is the raw address of the pooled object. If the
             * pool is not shutting down the object will be re-wrapped in a SmartPtr
             * and returned to the pool via freeInstance().
             *
             * @param ptr Raw pointer to the object being destroyed.
             * @return true if destruction was handled (object returned to pool), false otherwise.
             */
            bool destroy( void *ptr ) override;

            /**
             * @brief Gets the owner InstancePool.
             * @return Pointer to the owner InstancePool.
             */
            SmartPtr<InstancePool> getOwner() const;

            /**
             * @brief Sets the owner InstancePool.
             * @param owner Pointer to the owner InstancePool.
             */
            void setOwner( SmartPtr<InstancePool> owner );

        protected:
            AtomicWeakPtr<InstancePool> m_owner;
        };

        /**
         * @brief Default constructor. Initializes an empty pool with grow size 0.
         */
        InstancePool();

        /**
         * @brief Constructor that initializes the pool with a specified number of elements.
         */
        explicit InstancePool( u32 numElements );

        /**
         * @brief Destructor. Cleans up allocated memory blocks.
         */
        ~InstancePool() override;

        /**
         * @brief Allocates a block of elements using the current grow size.
         *
         * If the pool's grow size is <= 0 this is a no-op. The function will
         * respect m_maxSize when allocating additional blocks.
         */
        void allocate_data();

        /**
         * @brief Acquire an instance from the pool.
         *
         * If the free-elements queue is empty the pool will attempt to allocate
         * more elements (allocate_data). Returns a SmartPtr<T> to a pooled object,
         * or nullptr if none could be provided (e.g. grow size is zero or pool
         * has reached its max size and is empty).
         *
         * @return SmartPtr<T> to a pooled object or nullptr.
         */
        SmartPtr<T> getInstance();

        /**
         * @brief Return a previously acquired instance back to the pool.
         *
         * The instance will be pushed onto the internal free queue and can be
         * reused by subsequent getInstance() calls. If the pool is shutting down
         * the instance will not be queued.
         *
         * @param instance SmartPtr<T> referencing the element to free.
         */
        void freeInstance( SmartPtr<T> instance );

        /**
         * @brief Empties the free-elements queue.
         *
         * This pops all SmartPtr<T> entries currently available in the free queue.
         * The underlying elements remain allocated and can still be referenced by
         * other SmartPtr instances that are currently checked out.
         */
        void freeAll();

        /**
         * @brief Gets the current grow size (number of elements allocated per block).
         * @return The grow size.
         */
        s32 getGrowSize() const;

        /**
         * @brief Sets the grow size (number of elements allocated per block).
         * @param growSize The new grow size.
         */
        void setGrowSize( s32 growSize );

        /**
         * @brief Gets the maximum number of elements allowed in the pool.
         * @return The maximum size, or -1 if unlimited.
         */
        s32 getMaxSize() const;

        /**
         * @brief Sets the maximum number of elements allowed in the pool.
         * @param maxSize The new maximum size, or -1 for unlimited.
         */
        void setMaxSize( s32 maxSize );

        /**
         * @brief Returns the number of free elements currently available.
         *
         * This uses an "unsafe" size method on the concurrent queue which may be
         * approximate in highly concurrent scenarios but is cheap to call.
         */
        size_t getNumFreeElements() const;

        /**
         * @brief Returns the total number of elements that have been created and tracked.
         *
         * This is the number of element pointers stored in m_elements (one entry per element).
         */
        size_t getNumElements() const;

        /**
         * @brief Access the internal list of tracked element pointers.
         *
         * The returned arrays contain RawPtr<T> entries pointing at every element
         * that has been allocated into the pool. This is primarily intended for
         * debugging or inspection; do not modify the array from multiple threads
         * without synchronization.
         */
        ConcurrentArray<AtomicRawPtr<T>> &getElements();

        /**
         * @brief Access the internal list of tracked element pointers.
         *
         * The returned arrays contain RawPtr<T> entries pointing at every element
         * that has been allocated into the pool. This is primarily intended for
         * debugging or inspection; do not modify the array from multiple threads
         * without synchronization.
         */
        const ConcurrentArray<AtomicRawPtr<T>> &getElements() const;

    protected:
        /** Listener instance (atomic shared pointer) used to attach to pooled objects. */
        AtomicSharedPtr<Listener> m_listener;

        /**
         * @brief Keeps the underlying contiguous blocks allocated with new[].
         *
         * Each entry is a pointer to a T[] block allocated with size m_growSize.
         * Blocks are deleted (delete[]) in the pool destructor.
         */
        ConcurrentArray<AtomicRawPtr<T>> m_pool;

        /**
         * @brief Flat list of RawPtr<T> for all elements created in the pool.
         *
         * This is used for bookkeeping and inspection; elements are not owned by
         * m_elements (ownership is by m_pool blocks).
         */
        ConcurrentArray<AtomicRawPtr<T>> m_elements;

        /**
         * @brief Queue of available SmartPtr<T> objects that can be checked out.
         *
         * When an object is freed it is pushed back to this queue. getInstance pops
         * from this queue to provide objects to callers.
         */
        ConcurrentQueue<AtomicSmartPtr<T>> m_freeElements;

        /** Number of elements to allocate per grow (per block). Defaults to 0. */
        atomic_s32 m_growSize = 0;

        /**
         * @brief Maximum number of elements allowed in the pool.
         *
         * A value of -1 indicates unlimited growth. When set, allocation will stop
         * once m_elements.size() >= m_maxSize.
         */
        atomic_s32 m_maxSize = 128;

        /** When true the pool is shutting down and will not accept returned instances. */
        atomic_bool m_isShuttingDown = false;
    };

    // Listener definitions
    template <class T>
    InstancePool<T>::Listener::Listener() = default;

    template <class T>
    InstancePool<T>::Listener::Listener( InstancePool *owner ) : m_owner( owner )
    {
    }

    template <class T>
    InstancePool<T>::Listener::Listener( const Listener &other ) : m_owner( other.m_owner )
    {
    }

    template <class T>
    InstancePool<T>::Listener::~Listener() = default;

    template <class T>
    bool InstancePool<T>::Listener::destroy( void *ptr )
    {
        if( auto owner = getOwner() )
        {
            if( !owner->m_isShuttingDown )
            {
                RawPtr<T> instancePtr = static_cast<T *>( ptr );
                instancePtr->addReference();

                SmartPtr<T> instance = static_cast<T *>( ptr );
                owner->freeInstance( instance );

                return true;
            }
        }

        return false;
    }

    template <class T>
    SmartPtr<InstancePool<T>> InstancePool<T>::Listener::getOwner() const
    {
        auto p = m_owner.load();
        return p.lock();
    }

    template <class T>
    void InstancePool<T>::Listener::setOwner( SmartPtr<InstancePool<T>> owner )
    {
        m_owner = owner;
    }

    // InstancePool definitions
    template <class T>
    InstancePool<T>::InstancePool()
    {
        auto listener = workphone::make_shared<Listener>();
        listener->setOwner( this );
        m_listener = listener;
    }

    template <class T>
    InstancePool<T>::InstancePool( u32 numElements ) :
        m_growSize( numElements ),
        m_maxSize( -1 ),
        m_isShuttingDown( false )
    {
        auto listener = workphone::make_shared<Listener>();
        listener->setOwner( this );
        m_listener = listener;
    }

    template <class T>
    InstancePool<T>::~InstancePool()
    {
        for( auto &p : m_pool )
        {
            delete[] p;
        }

        m_pool.clear();

        m_isShuttingDown = true;
        m_listener = nullptr;
        freeAll();
    }

    template <class T>
    void InstancePool<T>::allocate_data()
    {
        if( m_growSize > 0 )
        {
            if( m_maxSize == -1 )
            {
                auto pool = new T[m_growSize];

                for( u32 i = 0; i < static_cast<u32>( m_growSize ); ++i )
                {
                    auto ptr = &pool[i];
                    auto newInstance = SmartPtr<T>( ptr );
                    if( newInstance )
                    {
                        auto listener = m_listener.load();
                        auto pListener = listener.get();

                        newInstance->setSharedObjectListener( pListener );

                        m_elements.push_back( newInstance.get() );
                        WP_ASSERT( m_elements.size() < 1e5 );

                        WP_ASSERT( newInstance );
                        m_freeElements.push( newInstance );
                    }
                }

                m_pool.push_back( pool );
            }
            else if( m_maxSize >= 0 )
            {
                if( m_elements.size() < m_maxSize )
                {
                    auto pool = new T[m_growSize];

                    for( u32 i = 0; i < static_cast<u32>( m_growSize ); ++i )
                    {
                        auto ptr = &pool[i];
                        auto newInstance = SmartPtr<T>( ptr );

                        auto listener = m_listener.load();
                        auto pListener = listener.get();

                        newInstance->setSharedObjectListener( pListener );

                        m_elements.push_back( newInstance.get() );

                        WP_ASSERT( newInstance );
                        m_freeElements.push( newInstance );
                    }

                    m_pool.push_back( pool );
                }
            }
        }
    }

    template <class T>
    SmartPtr<T> InstancePool<T>::getInstance()
    {
        if( m_freeElements.empty() )
        {
            allocate_data();
        }

        if( !m_freeElements.empty() )
        {
            SmartPtr<T> instance;
            while( !m_freeElements.try_pop( instance ) )
            {
                WP_ASSERT( instance );
                return instance;
            }
        }

        return nullptr;
    }

    template <class T>
    void InstancePool<T>::freeInstance( SmartPtr<T> instance )
    {
        if( !m_isShuttingDown )
        {
            WP_ASSERT( instance );
            m_freeElements.push( instance );
        }
    }

    template <class T>
    s32 InstancePool<T>::getGrowSize() const
    {
        return m_growSize;
    }

    template <class T>
    void InstancePool<T>::setGrowSize( s32 growSize )
    {
        m_growSize = growSize;

        if( m_growSize > m_maxSize )
        {
            m_maxSize = m_growSize;
        }
    }

    template <class T>
    s32 InstancePool<T>::getMaxSize() const
    {
        return m_maxSize;
    }

    template <class T>
    void InstancePool<T>::setMaxSize( s32 maxSize )
    {
        m_maxSize = maxSize;
    }

    template <class T>
    size_t InstancePool<T>::getNumFreeElements() const
    {
        return m_freeElements.unsafe_size();
    }

    template <class T>
    size_t InstancePool<T>::getNumElements() const
    {
        return m_elements.size();
    }

    template <class T>
    ConcurrentArray<AtomicRawPtr<T>> &InstancePool<T>::getElements()
    {
        return m_elements;
    }

    template <class T>
    const ConcurrentArray<AtomicRawPtr<T>> &InstancePool<T>::getElements() const
    {
        return m_elements;
    }

    template <class T>
    void InstancePool<T>::freeAll()
    {
        AtomicSmartPtr<T> instance;
        while( m_freeElements.try_pop( instance ) )
        {
        }
    }

}  // namespace workphone

#endif  // InstancePool_h__
