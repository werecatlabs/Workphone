#ifndef FactoryTemplate_h__
#define FactoryTemplate_h__

#include <Workphone/System/Factory.hpp>
#include <Workphone/Memory/PointerUtil.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Core/InstancePool.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Pool.hpp>

namespace workphone
{

    /**
     * @brief Generic factory implementation for creating and recycling objects of a concrete type.
     *
     * FactoryTemplate implements Factory for a concrete type `T`. It optionally uses an internal
     * memory `Pool<T>` to preallocate raw storage and an `InstancePool<T>` to manage pre-constructed
     * instances. Created objects are tracked in a thread-friendly `ConcurrentArray<T*>` so the
     * factory can clean them up on unload or when freed.
     *
     * Responsibilities:
     * - allocate and free objects of type `T`
     * - optionally use a memory pool to reduce allocations
     * - track created instances so pooled instances can be returned to the pool on free/unload
     *
     * @tparam T The concrete object type created and managed by this factory. `T` is expected to
     *           derive from the shared object base used by the system (it must provide methods
     *           referenced below such as `setSharedObjectListener`, `setPoolElement`,
     *           `setCreatorData`, and `isPoolElement`).
     */
    template <class T>
    class FactoryTemplate : public Factory
    {
    public:
        // The type of object this factory creates.
        using type = T;

        /**
         * @brief Default constructor.
         * Initializes internal metadata (object size) based on `sizeof(T)`.
         */
        FactoryTemplate();

        /**
         * @brief Construct a factory for a named object type.
         *
         * This constructor sets the object type/name and its type id and configures the recorded
         * object size.
         *
         * @param objectType The string name of the object type this factory creates.
         * @param objectTypeHash Hash associated with the object type.
         */
        FactoryTemplate( const String &objectType, hash64 objectTypeHash );

        /**
         * @brief Virtual destructor.
         *
         * If the factory is still loaded when destroyed, `unload(nullptr)` is invoked to ensure
         * any pooled instances are returned and resources released.
         */
        ~FactoryTemplate() override;

        /**
         * @copydoc Factory::load
         *
         * Additional behavior:
         * - Reserves room in the internal instances array according to the configured growth size.
         * - Creates and configures an internal `Pool<T>` and sets its growth size to the factory's
         *   configured next size.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc Factory::unload
         *
         * Additional behavior:
         * - If pooling is enabled, destroys pooled instances, returns raw memory to the pool and
         *   clears the internal pool reference.
         * - Clears tracked instances.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @copydoc Factory::setGrowSize
         * @brief Sets the growth size for both the factory and the associated `Pool<T>` (if any).
         * @param size Number of elements to grow the pool/instance containers by.
         */
        void setGrowSize( u32 size ) override;

        /**
         * @copydoc Factory::allocatePoolData
         * Allocates pool backing storage if an internal `Pool<T>` is present.
         */
        void allocatePoolData() override;

        /**
         * @copydoc Factory::freePoolData
         * Frees/clears any allocated pool storage managed by the internal `Pool<T>`.
         */
        void freePoolData() override;

        /**
         * @brief Create a new object instance.
         *
         * Behavior:
         * - If a memory pool is available and has free slots, constructs `T` in pooled storage
         *   and marks it as a pool element. The created object will be tracked in `m_instances`.
         * - Otherwise falls back to allocating a new object using the base `Factory::createObject`
         *   raw allocation path or `new T()` depending on configuration macros.
         *
         * The returned pointer is a pointer to a fully constructed `T`. The caller receives
         * ownership in the sense that it must eventually call `freeObject` (or otherwise arrange
         * for the factory to free the object).
         *
         * @return Pointer to the created object (typed as `void*` to match the base interface).
         *
         * @note On out-of-memory conditions the implementation terminates the process.
         */
        void *createObject() override;

        /**
         * @brief Allocate raw memory for a single object of type `T`.
         * @return Pointer to raw memory for one object (may be nullptr on failure).
         */
        void *allocateMemory() override;

        /**
         * @brief Free an object previously created by this factory.
         *
         * If the object is a pooled element (as reported by `T::isPoolElement()`), the instance
         * is destroyed, removed from the tracked instances list and its raw memory is returned to
         * the internal pool. Otherwise the object is destroyed and freed via the base Factory
         * free path.
         *
         * @param object Pointer to the object to free (must be a pointer returned by
         *               `createObject()`).
         */
        void freeObject( void *object ) override;

        /**
         * @brief Free raw memory for a single object of type `T`.
         * @param object Pointer to raw memory to free (must have been allocated by this factory).
         */
        void freeMemory( void *object ) override;

        /**
         * @brief Return a snapshot of currently tracked instances as shared objects.
         * @return Array of objects currently created and tracked by this factory.
         */
        Array<SmartPtr<ISharedObject>> getInstanceObjects() const override;

        /**
         * @brief Check if this factory has an internal memory pool.
         * @return `true` if a pool is configured, `false` otherwise.
         */
        bool hasPool() const override;

        /**
         * @brief Configure the next data size used by the pool and instance pool.
         *
         * When pooling is enabled this sets the next allocation size and triggers allocation on
         * the internal pool.
         *
         * @param size Desired next size (number of elements) for pools.
         */
        void setDataSize( u32 size );

        /**
         * @brief Get the internal memory pool (if any).
         *
         * @return SmartPtr to the `Pool<T>` used by this factory, or null if none set.
         */
        SmartPtr<Pool<T>> getPool() const;

        /**
         * @brief Set the internal memory pool used by this factory.
         *
         * @param pool SmartPtr to a `Pool<T>` instance. Passing null disables pooling.
         */
        void setPool( SmartPtr<Pool<T>> pool );

        /**
         * @brief Return a snapshot of currently tracked instances.
         *
         * The returned array is a copy/snapshot of the internal `ConcurrentArray` and can be used
         * safely by callers without holding internal locks.
         *
         * @return Array of `T*` currently created and tracked by the factory.
         */
        Array<T *> getInstances() const;

        WP_CLASS_REGISTER_TEMPLATE_DECL( FactoryTemplate, T );

    protected:
        void configureObject( T *object, bool poolElement );

        void releaseObjectMemory( T *object, bool poolElement );

        /// A pool of object with preallocated memory.
        AtomicSmartPtr<Pool<T>> m_pool;

        /// A pool of instances. Normally objects that are pre-constructed.
        InstancePool<T> m_instancePool;

        /// Instances tracked by the factory (thread-safe container).
        ConcurrentArray<T *> m_instances;
    };

    WP_CLASS_REGISTER_DERIVED_TEMPLATE( workphone, FactoryTemplate, T, Factory );

    template <class T>
    FactoryTemplate<T>::FactoryTemplate()
    {
        auto typeInfo = T::typeInfo();
        setObjectTypeId( typeInfo );

        m_objectSize = sizeof( T );
    }

    template <class T>
    FactoryTemplate<T>::FactoryTemplate( const String &objectType, hash64 objectTypeHash )
    {
        setObjectTypeName( objectType );
        setObjectTypeHash( objectTypeHash );

        auto typeInfo = T::typeInfo();
        setObjectTypeId( typeInfo );

        auto objectSize = sizeof( T );
        setObjectSize( objectSize );
    }

    template <class T>
    FactoryTemplate<T>::~FactoryTemplate()
    {
        if( isLoaded() )
        {
            unload( nullptr );
        }
    }

    template <class T>
    void FactoryTemplate<T>::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );

        Factory::load( data );

        m_instances.reserve( m_nextSize );

        auto pool = workphone::make_ptr<Pool<T>>();
        pool->setNextSize( m_nextSize );
        setPool( pool );

        setLoadingState( LoadingState::Loaded );
    }

    template <class T>
    void FactoryTemplate<T>::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

#if WP_TRACK_REFERENCES
        if( !m_instances.empty() )
        {
            auto &tracker = SharedObjectTracker::instance();
            tracker.dumpReport();
        }
#endif

        // WP_ASSERT( m_instances.empty() );  // instances not freed

        auto instances = m_instances.snapshot();
        for( auto &instance : instances )
        {
            if( instance )
            {
                auto pObject = (T *)instance;
                const auto poolElement = pObject->isPoolElement();

                pObject->~T();

                releaseObjectMemory( pObject, poolElement );
            }
        }

        m_instances.clear();

#if WP_USE_MEMORY_POOL
        freePoolData();
        setPool( nullptr );
#endif

        Factory::unload( data );

        setLoadingState( LoadingState::Unloaded );
    }

    template <class T>
    void FactoryTemplate<T>::setGrowSize( u32 size )
    {
        Factory::setGrowSize( size );

        if( auto pool = getPool() )
        {
            pool->setNextSize( size );
        }
    }

    template <class T>
    void FactoryTemplate<T>::allocatePoolData()
    {
        if( auto pool = getPool() )
        {
            pool->allocateData();
        }
    }

    template <class T>
    void FactoryTemplate<T>::freePoolData()
    {
        if( auto pool = getPool() )
        {
            WP_ASSERT( m_instances.empty() );
            pool->clear();
        }
    }

    template <class T>
    void *FactoryTemplate<T>::createObject()
    {
#if WP_USE_MEMORY_POOL
        if( auto pool = getPool() )
        {
            if( pool->getSize() > 0 || pool->getNextSize() > 0 )
            {
                auto memory = pool->allocate_object();
                WP_ASSERT( memory );

                if( memory )
                {
                    T *object = nullptr;

                    try
                    {
                        object = new( memory ) T;
                    }
                    catch( ... )
                    {
                        pool->free_object( memory );
                        throw;
                    }

                    configureObject( object, true );
                    return object;
                }
            }
        }
#endif

        auto ptr = Factory::createObject();
        if( !ptr )
        {
            std::cout << "Out of memory." << std::endl;
            std::terminate();
        }

        T *object = nullptr;

        try
        {
            object = new( ptr ) T;
        }
        catch( ... )
        {
            Factory::freeMemory( ptr );
            throw;
        }

        configureObject( object, false );
        return object;
    }

    template <class T>
    void *FactoryTemplate<T>::allocateMemory()
    {
        return Factory::allocateMemory();
    }

    template <class T>
    void FactoryTemplate<T>::freeObject( void *object )
    {
        try
        {
            auto pObject = (T *)object;
            const auto typeName = TypeManager::instance()->getName( T::typeInfo() );
            const bool traceMessage = typeName && StringUtil::isEqual( typeName, "workphone::StateMessageObject" );
            if( traceMessage ) std::fprintf( stderr, "TRACE FactoryTemplate freeObject start %p\n", pObject );
            WP_ASSERT( pObject );
            if( !pObject )
            {
                return;
            }

            if( pObject->isPoolElement() )
            {
                if( traceMessage ) std::fprintf( stderr, "TRACE FactoryTemplate pooled object\n" );
                m_instances.erase( pObject );

                if( traceMessage ) std::fprintf( stderr, "TRACE FactoryTemplate destroy start\n" );
                pObject->~T();
                if( traceMessage ) std::fprintf( stderr, "TRACE FactoryTemplate destroy end\n" );

                releaseObjectMemory( pObject, true );
                if( traceMessage ) std::fprintf( stderr, "TRACE FactoryTemplate memory release end\n" );
            }
            else
            {
                if( traceMessage ) std::fprintf( stderr, "TRACE FactoryTemplate nonpool object\n" );
                m_instances.erase( pObject );

                if( traceMessage ) std::fprintf( stderr, "TRACE FactoryTemplate destroy start\n" );
                pObject->~T();
                if( traceMessage ) std::fprintf( stderr, "TRACE FactoryTemplate destroy end\n" );
                Factory::freeObject( object );
                if( traceMessage ) std::fprintf( stderr, "TRACE FactoryTemplate memory release end\n" );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    template <class T>
    void FactoryTemplate<T>::freeMemory( void *object )
    {
        try
        {
            WP_ASSERT( object );
            if( !object )
            {
                return;
            }

            Factory::freeMemory( object );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    template <class T>
    Array<SmartPtr<ISharedObject>> FactoryTemplate<T>::getInstanceObjects() const
    {
        Array<SmartPtr<ISharedObject>> result;

        auto instances = m_instances.snapshot();
        result.reserve( instances.size() );

        for( auto instance : instances )
        {
            if( instance )
            {
                result.push_back( static_cast<ISharedObject *>( instance ) );
            }
        }

        return result;
    }

    template <class T>
    bool FactoryTemplate<T>::hasPool() const
    {
#if WP_USE_MEMORY_POOL
        if( auto pool = getPool() )
        {
            return pool->getSize() > 0;
        }
#endif

        return false;
    }

    template <class T>
    void FactoryTemplate<T>::setDataSize( [[maybe_unused]] u32 size )
    {
#if WP_USE_MEMORY_POOL
        m_nextSize = size;

        auto pool = getPool();
        WP_ASSERT( pool );
        if( pool )
        {
            pool->setNextSize( size );
            pool->allocateData();
        }

        m_instancePool.setGrowSize( size );
#endif
    }

    template <class T>
    SmartPtr<Pool<T>> FactoryTemplate<T>::getPool() const
    {
        return m_pool;
    }

    template <class T>
    void FactoryTemplate<T>::setPool( SmartPtr<Pool<T>> pool )
    {
        m_pool = pool;
    }

    template <class T>
    Array<T *> FactoryTemplate<T>::getInstances() const
    {
        return m_instances.snapshot();
    }

    template <class T>
    void FactoryTemplate<T>::configureObject( T *object, bool poolElement )
    {
        WP_ASSERT( object );
        if( object )
        {
            object->setSharedObjectListener( m_listener );
            object->setPoolElement( poolElement );
            object->setCreatorData( this );

            WP_ASSERT( object->getSharedObjectListener() == m_listener );
            WP_ASSERT( object->isPoolElement() == poolElement );
            WP_ASSERT( object->getCreatorData() == this );

            m_instances.push_back( object );
        }
    }

    template <class T>
    void FactoryTemplate<T>::releaseObjectMemory( T *object, bool poolElement )
    {
        WP_ASSERT( object );
        if( !object )
        {
            return;
        }

        if( poolElement )
        {
            auto pool = getPool();
            WP_ASSERT( pool );
            if( pool )
            {
                pool->free_object( object );
            }

            return;
        }

        Factory::freeObject( object );
    }

}  // namespace workphone

#endif  // FactoryTemplate_h__
