#ifndef Factory_h__
#define Factory_h__

#include <Workphone/Interface/System/IFactory.hpp>
#include <Workphone/Memory/AtomicWeakPtr.hpp>
#include <Workphone/Interface/Memory/ISharedObjectListener.hpp>
#include <Workphone/Memory/AtomicRawPtr.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{

    /**
     * @class Factory
     * @brief Thread-safe concrete implementation of the `IFactory` interface.
     *
     * The `Factory` is responsible for creating and destroying objects of a single
     * concrete type and for managing an optional memory pool used for fast
     * allocation/deallocation of those objects. It maintains metadata about the
     * object type (name, hash and id), optional tags, a listener for lifecycle
     * notifications and a list of live instances. The implementation is safe to
     * use concurrently thanks to an internal recursive mutex.
     *
     * Responsibilities:
     * - allocate/free raw memory for single objects or arrays
     * - construct and destroy object instances managed by the factory
     * - track instantiated objects and notify a listener when they are destroyed
     * - expose type information and tags for runtime lookup
     */
    class WPCore_API Factory : public IFactory
    {
    public:
        /**
         * @class Listener
         * @brief Helper listener that forwards shared-object lifecycle events to the parent factory.
         *
         * `Listener` implements `ISharedObjectListener` and holds a weak reference to
         * the owning `Factory`. It is registered on instances so that when an instance
         * is destroyed the factory can update its internal instance list without creating
         * reference cycles.
         */
        class Listener : public ISharedObjectListener
        {
        public:
            /**
             * @brief Default constructor.
             */
            Listener();

            /**
             * @brief Construct a listener bound to a specific factory.
             * @param factory Raw pointer to the parent `Factory`.
             */
            explicit Listener( Factory *factory );

            /**
             * @brief Destructor.
             */
            ~Listener() override;

            /**
             * @brief Called to inform the listener that the shared object is unloading.
             * @param data Shared pointer to the object being unloaded.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Notification invoked when a tracked shared object is being destroyed.
             * @param ptr Raw pointer to the object about to be destroyed.
             * @return `true` if the listener handled the destruction, `false` otherwise.
             */
            bool destroy( void *ptr ) override;

            /**
             * @brief Obtain a strong reference to the parent factory if it is still alive.
             * @return Smart pointer to the parent `Factory` or `nullptr` if the factory was destroyed.
             */
            Factory *getFactoryPtr() const;

            /**
             * @brief Obtain a strong reference to the parent factory if it is still alive.
             * @return Smart pointer to the parent `Factory` or `nullptr` if the factory was destroyed.
             */
            SmartPtr<Factory> getFactory() const;

            /**
             * @brief Set the parent factory (strong reference will be converted to a weak one).
             * @param factory Smart pointer to the parent `Factory`.
             */
            void setFactory( SmartPtr<Factory> factory );

            WP_CLASS_REGISTER_DECL;

        protected:
            /// Weak pointer to the parent factory to avoid circular references.
            AtomicWeakPtr<Factory> m_factory;
        };

        /**
         * @brief Default constructor.
         *
         * Initializes internal members to sensible defaults. No pool is created
         * until `allocatePoolData()` is called or the factory is otherwise
         * configured by the owning code.
         */
        Factory();

        /**
         * @brief Destructor.
         *
         * Ensures any allocated pool data is released and that remaining
         * instances are properly destroyed. The factory will also notify the
         * listener when instances are removed.
         */
        ~Factory() override;

        /**
         * @brief Perform initialization using a shared data object.
         * @param data Optional shared object containing initialization/configuration
         *             information. The exact type and interpretation is defined by
         *             higher-level code that uses this factory.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unload/teardown with optional data.
         * @param data Optional shared object that can be used to perform
         *             additional cleanup steps. Can be nullptr.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Returns the pool grow size.
         *
         * When the memory pool must be expanded this value indicates how many
         * additional object slots should be allocated.
         * @return Number of objects to allocate on pool growth.
         */
        u32 getGrowSize() const override;

        /**
         * @brief Set the pool grow size.
         * @param size Number of object slots to allocate when expanding the pool.
         */
        void setGrowSize( u32 size ) override;

        /**
         * @brief Allocate internal structures and memory used by the pool.
         *
         * This prepares the factory to begin handing out pooled allocations via
         * `allocateMemory()` and `createObject()`.
         */
        void allocatePoolData() override;

        /**
         * @brief Release any memory and structures used by the pool.
         *
         * After calling this the factory will no longer service pooled
         * allocations until `allocatePoolData()` is called again.
         */
        void freePoolData() override;

        /**
         * @brief Allocate raw memory for a single object managed by this factory.
         *
         * Uses the pool if present; otherwise falls back to a standard heap
         * allocation. The returned memory is uninitialized and suitable for
         * placement-new.
         * @return Pointer to raw memory for one object (may be nullptr on failure).
         */
        void *allocateMemory() override;

        /**
         * @brief Free raw memory previously returned by `allocateMemory()`.
         * @param ptr Pointer to memory to free (must have been allocated by this factory).
         */
        void freeMemory( void *ptr ) override;

        /**
         * @brief Allocate memory and construct a new object instance.
         *
         * This function will allocate raw memory (pooled or heap) and perform
         * object construction. The concrete type and construction parameters
         * are determined by the factory specialization.
         * @return Pointer to the constructed object, or nullptr on failure.
         */
        void *createObject() override;

        /**
         * @brief Destroy an object previously created by this factory and free its memory.
         * @param object Pointer to the object to destroy (must have been created by this factory).
         */
        void freeObject( void *object ) override;

        /**
         * @brief Returns the size (in bytes) of the objects produced by this factory.
         * @return Object size in bytes.
         */
        u32 getObjectSize() const override;

        /**
         * @brief Set the object size in bytes. This must match the concrete
         * object type size so the factory can correctly manage memory.
         * @param objectSize Size of the object in bytes.
         */
        void setObjectSize( u32 objectSize ) override;

        /**
         * @brief Returns the registered listener for shared object events.
         * @return Raw pointer to the `ISharedObjectListener` instance or nullptr.
         */
        ISharedObjectListener *getListener() const override;

        /**
         * @brief Check whether the factory's object type derives from the provided type info.
         * @param typeInfo Hash value representing a type to compare against.
         * @return `true` if the factory's object type is equal to or derives from `typeInfo`.
         */
        bool isObjectDerivedFromByInfo( u32 typeInfo ) const override;

        /**
         * @brief Get a C-string pointer to the factory's type name.
         * @return Pointer to a null-terminated C-string representing the type name.
         */
        const c8 *getTypeNamePtr() const override;

        /**
         * @brief Get the factory's type name as a `String`.
         * @return Copy of the type name.
         */
        String getTypeName() const override;

        /**
         * @brief Set the factory's visible type name.
         * @param typeName New type name to assign.
         */
        void setTypeName( const String &typeName ) override;

        /**
         * @brief Allocate and construct an array of objects of this factory's type.
         * @param numElements Number of elements to allocate and construct.
         * @return Pointer to the beginning of the array, or nullptr on failure.
         */
        void *createArray( u32 numElements ) override;

        /**
         * @brief Alternative accessor for the object type name (C-string).
         * @return Pointer to a null-terminated C-string representing the object type name.
         */
        const c8 *getObjectTypeNamePtr() const override;

        /**
         * @brief Alternative accessor for the object type name.
         * @return The object type name as a `String`.
         */
        String getObjectTypeName() const override;

        /**
         * @brief Set the object type name used by this factory.
         * @param type New object type name.
         */
        void setObjectTypeName( const String &type ) override;

        /**
         * @brief Returns the object's type hash (unique runtime identifier).
         * @return Hash value associated with the object type.
         */
        hash_type getObjectTypeHash() const override;

        /**
         * @brief Set the type hash used to identify the factory's object type.
         * @param hash Hash value that identifies the object type.
         */
        void setObjectTypeHash( hash_type hash ) override;

        /**
         * @copydoc IFactory::getObjectTypeId
         * @return Returns the registered numeric id for the object type managed by this factory.
         */
        u32 getObjectTypeId() const override;

        /**
         * @copydoc IFactory::setObjectTypeId
         * Assign a numeric id to the object type managed by this factory.
         */
        void setObjectTypeId( u32 objectTypeId ) override;

        /**
         * @brief Returns an estimate of memory (in bytes) currently used by the factory
         *        for its internal structures and pools.
         * @return Memory used in bytes.
         */
        s32 getMemoryUsed() const override;

        /**
         * @brief Return a snapshot list of instantiated shared objects managed by this factory.
         * @return Array of smart pointers to currently tracked instances.
         */
        Array<SmartPtr<ISharedObject>> getInstanceObjects() const override;

        /**
         * @brief Get the tags associated with this factory.
         * @return Array of tag strings used for categorization or lookup.
         */
        Array<String> getTags() const override;

        /**
         * @brief Replace the tag set associated with this factory.
         * @param tags New array of tag strings.
         */
        void setTags( const Array<String> &tags ) override;

        /**
         * @brief Returns true if the factory currently manages an internal memory pool.
         */
        bool hasPool() const override;

        /**
         * @copydoc IFactory::getFactoryManagerPtr
         */
        IFactoryManager *getFactoryManagerPtr() const override;

        /**
         * @copydoc IFactory::getFactoryManager
         */
        SmartPtr<IFactoryManager> getFactoryManager() const override;

        /**
         * @copydoc IFactory::setFactoryManager
         */
        void setFactoryManager( SmartPtr<IFactoryManager> factoryManager ) override;

        /**
         * @brief Lock the factory's internal mutex for exclusive access.
         * @return Multiple calls on the same thread are allowed because the mutex is recursive.
         */
        void lock() override;

        /**
         * @brief Try to acquire the factory lock without blocking.
         * @return `true` if the lock was acquired, `false` otherwise.
         */
        bool try_lock() override;

        /**
         * @brief Release the factory lock.
         */
        void unlock() override;

        WP_CLASS_REGISTER_DECL;

    protected:
        ///< Reference to the factory manager responsible for this factory (may be nullptr).
        AtomicWeakPtr<IFactoryManager> m_factoryManager;

        /// Raw pointer to an optional listener that receives shared-object events.
        AtomicRawPtr<ISharedObjectListener> m_listener;

        /// Hash value representing the concrete object type managed by this factory.
        atomic_s64 m_objectTypeHash = 0;

        /// Numeric id for the object type (used by higher-level registries).
        atomic_u32 m_objectTypeId = 0;

        /// Count of currently instantiated objects (updated atomically).
        atomic_u32 m_instanceCount = 0;

        /// Pool growth size (how many elements to allocate when expanding the pool).
        atomic_u32 m_nextSize = 0;

        /// Size in bytes of the object instances created by this factory.
        atomic_u32 m_objectSize = 0;

        /// Object type name (used for display or lookup).
        FixedString<WP_MAX_CLASSNAME> m_objectTypeName;

        /// Factory type/name (may differ from object type in some uses).
        FixedString<WP_MAX_CLASSNAME> m_typeName;

        /// Tags associated with the factory for classification or searching.
        ConcurrentArray<String> m_tags;

        /// Thread-safe container of raw instance pointers currently tracked by the factory.
        ConcurrentArray<void *> m_instances;
    };

    WPForceInline IFactoryManager *Factory::getFactoryManagerPtr() const
    {
        return m_factoryManager.get();
    }

    WPForceInline Factory *Factory::Listener::getFactoryPtr() const
    {
        return m_factory.get();
    }

}  // namespace workphone

#endif  // Factory_h__
