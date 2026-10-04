#ifndef ISharedObject_h__
#define ISharedObject_h__

#include <Workphone/Interface/Memory/IObject.hpp>
#include <Workphone/Atomics/AtomicTypes.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Memory/SmartPtr.hpp>

#if WP_TRACK_REFERENCES
#    include <Workphone/Memory/SharedObjectTracker.hpp>
#endif

namespace workphone
{

    /**
     * @brief Base interface for objects managed by the engine's shared pointer system.
     *
     * ISharedObject implements the contract used by the project's SmartPtr/WeakPtr
     * reference-counting system. Objects derived from ISharedObject are reference
     * counted (strong references) and also support weak references for tracking
     * without extending object lifetime.
     *
     * Key responsibilities:
     * - Maintain and expose strong and weak reference counts.
     * - Expose loading lifecycle (load/reload/unload) and a LoadingState.
     * - Provide basic locking primitives for thread-safety where required.
     * - Provide serialization-like methods to/from data representations.
     *
     * Lifetime and thread-safety notes:
     * - The object is created with a strong reference count of 1.
     * - Removing the last strong reference will call destroySharedObject() and
     *   perform object teardown. Weak references do not prevent destruction.
     * - Several atomic types are used to allow lock-free reference operations.
     *
     * @see IObject
     */
    class WPCore_API ISharedObject : public IObject
    {
    public:
        static const Array<String> loadingStateNames;

        static const String loadedStr;
        static const String loadingStateStr;
        static const String referencesStr;
        static const String weakReferencesStr;
        static const String typeNameStr;

        /**
         * @brief RAII guard that prevents the object from being unloaded while in scope.
         *
         * Increment the load-lock count on construction and decrement it on destruction.
         * Code that may unload the object (e.g. an unload() implementation) should call
         * isLoadLocked() and defer or skip the unload when the count is non-zero.
         *
         * Typical usage in an update loop:
         * @code
         *   ISharedObject::ScopedLoadLock guard( obj.get() );
         *   // obj->unload() will now see isLoadLocked() == true
         * @endcode
         */
        class WPCore_API ScopedLoadLock
        {
        public:
            /**
             * @brief Acquire the load lock on @p object.
             * @param object Object whose load-lock count should be incremented. May be null.
             */
            explicit ScopedLoadLock( ISharedObject *object );

            /**
             * @brief Release the load lock acquired in the constructor.
             */
            ~ScopedLoadLock();

            ScopedLoadLock( const ScopedLoadLock & ) = delete;
            ScopedLoadLock &operator=( const ScopedLoadLock & ) = delete;

            bool isLoaded() const;

        private:
            ISharedObject *m_object = nullptr;
        };

        /**
         * @brief Used to wait for the object until there is no loading lock.
         *
         * Blocks the calling thread (via Thread::yield() spin-wait) until
         * isLoadLocked() returns false for the given object.
         *
         * Typical usage:
         * @code
         *   ISharedObject::ScopedLoadstateWait wait( obj.get() );
         *   // isLoadLocked() is now false; safe to proceed
         * @endcode
         */
        class WPCore_API ScopedLoadstateWait
        {
        public:
            /**
             * @brief Spin-wait until @p object has no active load locks.
             * @param object Object to wait on. May be null.
             */
            explicit ScopedLoadstateWait( ISharedObject *object );

            ~ScopedLoadstateWait() = default;

            ScopedLoadstateWait( const ScopedLoadstateWait & ) = delete;
            ScopedLoadstateWait &operator=( const ScopedLoadstateWait & ) = delete;

        private:
            ISharedObject *m_object = nullptr;
        };

        /**
         * @brief Construct a shared object.
         *
         * The initial strong reference count is set to 1 and weak references to 0.
         * Subclasses should perform lightweight initialization here; heavier initialization
         * belongs in load() so it can be managed by the resource system.
         */
        ISharedObject();

        ISharedObject( u32 typeId );

        /**
         * @brief Virtual destructor.
         *
         * Subclasses should release non-managed resources here. Destruction is invoked
         * when the final strong reference is removed.
         */
        ~ISharedObject() override;

#ifdef _DEBUG
        /**
         * @brief Increment the weak reference count.
         *
         * Use weak references to observe an object without extending its lifetime.
         * This returns the new weak reference count after incrementing.
         *
         * @return The updated weak reference count.
         */
        virtual s32 addWeakReference();

        /**
         * @brief Increment weak reference count with debug tracking information.
         *
         * This overload is used in tracking builds to record where the weak reference
         * was created.
         *
         * @param address Address that holds the weak reference (may be pointer-to-pointer).
         * @param file Source file name where the reference was created.
         * @param line Source line number.
         * @param func Function name where the reference was created.
         * @return The updated weak reference count.
         */
        virtual s32 addWeakReference( void *address, const c8 *file, u32 line, const c8 *func );

        /**
         * @brief Decrement the weak reference count.
         *
         * Weak references do not influence object lifetime. This will decrement the
         * internal weak reference counter; no deletion occurs when weakRefs reaches 0.
         *
         * @return True if the object was deleted as a result of this operation (normally false).
         */
        virtual bool removeWeakReference();

        /**
         * @brief Decrement the weak reference count with tracking information.
         *
         * In debug/tracking builds this will remove the corresponding tracking entry.
         *
         * @param address Pointer to the address holding the weak reference (may be null).
         * @param file Source file where the weak reference is removed (default "??").
         * @param line Line number where the weak reference is removed (default 0).
         * @param func Function name where the weak reference is removed (default "??").
         * @return True if the weak reference count reached zero after decrement.
         */
        virtual bool removeWeakReference( void *address, const c8 *file = "??", u32 line = 0,
                                          const c8 *func = "??" );

        /**
         * @brief Increment the strong reference count and record debug info.
         *
         * Strong references extend object lifetime. Callers that obtain a strong
         * reference must later call removeReference() to release ownership.
         *
         * @param address Address that holds the strong reference (may be pointer-to-pointer).
         * @param file Source file where the reference was added.
         * @param line Source line number.
         * @param func Function name where the reference was added.
         * @return The new strong reference count after increment.
         */
        virtual s32 addReference( void *address, const c8 *file, u32 line, const c8 *func );

        /**
         * @brief Increment the strong reference count.
         *
         * Use this when debug tracking information is not required.
         *
         * @return The new strong reference count.
         */
        virtual s32 addReference();

        /**
         * @brief Decrement the strong reference count (with optional debug info).
         *
         * When the strong reference count reaches zero this triggers object destruction
         * via destroySharedObject().
         *
         * @param address Optional pointer used for tracking the removal site.
         * @param file Source file name (default "??").
         * @param line Source line number (default 0).
         * @param func Function name (default "??").
         * @return True if the object was deleted as a result of this call.
         */
        virtual bool removeReference( void *address, const c8 *file = "??", u32 line = 0,
                                      const c8 *func = "??" );

        /**
         * @brief Decrement the strong reference count.
         * Non-debug version of removeReference.
         * @return True if the object was deleted as a result of this call.
         */
        virtual bool removeReference();
#else
        /**
         * @brief Increment the weak reference count.
         *
         * Use weak references to observe an object without extending its lifetime.
         * This returns the new weak reference count after incrementing.
         *
         * @return The updated weak reference count.
         */
        s32 addWeakReference();

        /**
         * @brief Increment weak reference count with debug tracking information.
         *
         * This overload is used in tracking builds to record where the weak reference
         * was created.
         *
         * @param address Address that holds the weak reference (may be pointer-to-pointer).
         * @param file Source file name where the reference was created.
         * @param line Source line number.
         * @param func Function name where the reference was created.
         * @return The updated weak reference count.
         */
        s32 addWeakReference( void *address, const c8 *file, u32 line, const c8 *func );

        /**
         * @brief Decrement the weak reference count.
         *
         * Weak references do not influence object lifetime. This will decrement the
         * internal weak reference counter; no deletion occurs when weakRefs reaches 0.
         *
         * @return True if the object was deleted as a result of this operation (normally false).
         */
        bool removeWeakReference();

        /**
         * @brief Decrement the weak reference count with tracking information.
         *
         * In debug/tracking builds this will remove the corresponding tracking entry.
         *
         * @param address Pointer to the address holding the weak reference (may be null).
         * @param file Source file where the weak reference is removed (default "??").
         * @param line Line number where the weak reference is removed (default 0).
         * @param func Function name where the weak reference is removed (default "??").
         * @return True if the weak reference count reached zero after decrement.
         */
        bool removeWeakReference( void *address, const c8 *file = "??", u32 line = 0,
                                  const c8 *func = "??" );

        /**
         * @brief Increment the strong reference count and record debug info.
         *
         * Strong references extend object lifetime. Callers that obtain a strong
         * reference must later call removeReference() to release ownership.
         *
         * @param address Address that holds the strong reference (may be pointer-to-pointer).
         * @param file Source file where the reference was added.
         * @param line Source line number.
         * @param func Function name where the reference was added.
         * @return The new strong reference count after increment.
         */
        s32 addReference( void *address, const c8 *file, u32 line, const c8 *func );

        /**
         * @brief Increment the strong reference count.
         *
         * Use this when debug tracking information is not required.
         *
         * @return The new strong reference count.
         */
        s32 addReference();

        /**
         * @brief Decrement the strong reference count (with optional debug info).
         *
         * When the strong reference count reaches zero this triggers object destruction
         * via destroySharedObject().
         *
         * @param address Optional pointer used for tracking the removal site.
         * @param file Source file name (default "??").
         * @param line Source line number (default 0).
         * @param func Function name (default "??").
         * @return True if the object was deleted as a result of this call.
         */
        bool removeReference( void *address, const c8 *file = "??", u32 line = 0,
                              const c8 *func = "??" );

        /**
         * @brief Decrement the strong reference count.
         *
         * Non-debug version of removeReference.
         *
         * @return True if the object was deleted as a result of this call.
         */
        bool removeReference();
#endif

        /**
         * @brief Get the current strong reference count.
         *
         * Thread-safe snapshot of the strong reference count.
         *
         * @return Current strong reference count.
         */
        s32 getReferences() const;

        /**
         * @brief Get the current weak reference count.
         *
         * @return Current weak reference count.
         */
        s32 getWeakReferences() const;

        /**
         * @brief Load the object from a data representation.
         *
         * Default implementation is a no-op. Subclasses should override to perform
         * initialization that may fail or be deferred.
         *
         * @param data Optional data object used to populate this object.
         */
        virtual void load( SmartPtr<ISharedObject> data );

        /**
         * @brief Reload the object using provided data.
         *
         * Called to refresh or update resources while the object remains alive.
         *
         * @param data Optional data object containing updated values.
         */
        virtual void reload( SmartPtr<ISharedObject> data );

        /**
         * @brief Unload any heavy resources held by this object.
         *
         * After unload() the object should be in a state where load() can be called again.
         *
         * @param data Optional data relevant to unloading.
         */
        virtual void unload( SmartPtr<ISharedObject> data );

        /**
         * @brief Get the object's loading state.
         *
         * @return Current LoadingState value.
         */
        LoadingState getLoadingState() const;

        /**
         * @brief Set the object's loading state.
         *
         * @param state The new loading state to set.
         */
        void setLoadingState( LoadingState state );

        /**
         * @brief Returns true if the object is currently performing a load operation.
         *
         * Subclasses that perform asynchronous loads should override to indicate activity.
         *
         * @return True if loading is in progress.
         */
        virtual bool isLoading() const;

        /**
         * @brief Returns true if the object is queued for loading work.
         *
         * @return True if the object is queued for load processing.
         */
        virtual bool isLoadingQueued() const;

        /**
         * @brief Returns true when the object is in the Loaded state.
         *
         * @return True if loadingState == LoadingState::Loaded.
         */
        bool isLoaded() const;

        /**
         * @brief Returns true if public API on the object is safe to be called from multiple threads.
         *
         * Subclasses that require external synchronization should override to return false.
         *
         * @return True when thread-safe.
         */
        virtual bool isThreadSafe() const;

        /**
         * @brief Return whether the object has been allocated / is valid (alive).
         *
         * This flag is separate from reference counts and generally indicates that the
         * object has completed construction and not been marked for deletion.
         *
         * @return True if the object is considered alive.
         */
        bool isAlive() const;

        /**
         * @brief Mark the object as a pool element.
         *
         * Pool elements may be reused rather than destroyed; overriding classes
         * may use this to change destruction semantics.
         *
         * @param poolElement True if this object belongs to a pool.
         */
        virtual void setPoolElement( bool poolElement );

        /**
         * @brief Query whether the object is a pooled element.
         *
         * @return True when the object is part of an object pool.
         */
        virtual bool isPoolElement() const;

        /**
         * @brief Convert the object to a data representation.
         *
         * Used by save/serialize flows. Default returns a SmartPtr to this or an equivalent.
         *
         * @return A SmartPtr containing the object's data representation.
         */
        virtual SmartPtr<ISharedObject> toData() const;

        /**
         * @brief Populate this object from a data representation.
         *
         * @param data Data object used to set this object's fields.
         */
        virtual void fromData( SmartPtr<ISharedObject> data );

        /** @copydoc IObject::toString */
        virtual String toString() const override;

        /**
         * @brief Get object properties as a Properties object.
         *
         * Used to expose key/value pairs for editors or serialization.
         *
         * @return Properties object representing this object's properties.
         */
        virtual SmartPtr<Properties> getProperties() const;

        /**
         * @brief Set this object's properties from a Properties object.
         *
         * @param properties Properties to apply to this object.
         */
        virtual void setProperties( SmartPtr<Properties> properties );

        /**
         * @brief Get direct child objects that compose this object.
         *
         * Default implementation returns an empty array. Subclasses should return any
         * child ISharedObject instances that are owned or referenced by this object.
         *
         * @return Array of child SmartPtr<ISharedObject>.
         */
        virtual Array<SmartPtr<ISharedObject>> getChildObjects() const;

        /**
         * @brief Get registered object listeners.
         * Listeners are notified of changes and lifecycle events.
         * @return Array of listeners.
         */
        virtual Array<SmartPtr<IEventListener>> getObjectListeners() const;

        /**
         * @brief Get registered object listeners.
         * Listeners are notified of changes and lifecycle events.
         * @return Array of listeners.
         */
        virtual void getObjectListenersList( SmartPtr<IEventListener> *listeners, u32 *count,
                                             u32 maxCount ) const;

        /**
         * @brief Get the number of registered listeners.
         * @return Count of listener objects.
         */
        virtual u32 getNumListeners() const;

        /**
         * @brief Check whether the provided listener is registered.
         * @param listener Listener to check for.
         * @return True if listener is present.
         */
        virtual bool hasObjectListener( SmartPtr<IEventListener> listener ) const;

        /**
         * @brief Register a listener for this object's events.
         *
         * @param listener Listener to add.
         */
        virtual void addObjectListener( SmartPtr<IEventListener> listener );

        /**
         * @brief Remove a listener previously registered.
         *
         * @param listener Listener to remove.
         */
        virtual void removeObjectListener( SmartPtr<IEventListener> listener );

        /**
         * @brief Remove all registered listeners.
         *
         * Use with care; callers should ensure no code will attempt to use removed listeners.
         */
        virtual void removeObjectListeners();

        /**
         * @brief Find and return an object listener by name.
         *
         * @param name Listener name to search for.
         * @return Matching listener SmartPtr or null if not found.
         */
        virtual SmartPtr<IEventListener> findObjectListener( const String &name ) const;

        /**
         * @brief Get the raw shared object listener pointer.
         *
         * This returns a raw pointer (non owning) to the object's listener interface,
         * useful for integration with external systems where SmartPtr isn't convenient.
         *
         * @return Pointer to ISharedObjectListener or nullptr.
         */
        virtual ISharedObjectListener *getSharedObjectListener() const;

        /**
         * @brief Set the raw shared object listener pointer.
         *
         * The provided pointer is NOT reference-counted by this object.
         *
         * @param listener Non-owning listener pointer.
         */
        virtual void setSharedObjectListener( ISharedObjectListener *listener );

        /**
         * @brief Query whether this object should be subject to garbage collection.
         *
         * When true, the object may be considered for collection by engine systems.
         *
         * @return True if garbage collected.
         */
        virtual bool isGarbageCollected() const;

        /**
         * @brief Set whether this object is subject to garbage collection.
         *
         * @param garbageCollected True to enable garbage collection behavior.
         */
        virtual void setGarbageCollected( bool garbageCollected );

        /**
         * @brief Get the raw script-data pointer associated with this object.
         *
         * This raw pointer provides a low-overhead way to expose language/runtime
         * specific data attached to the object. Ownership semantics depend on the
         * embedding: often the script runtime owns this data.
         *
         * @return Pointer to associated ISharedObject used as script data (may be null).
         */
        ISharedObject *getScriptDataPtr() const;

        /**
         * @brief Get the script data as a SmartPtr.
         *
         * @return SmartPtr wrapping the script data object.
         */
        SmartPtr<ISharedObject> getScriptData() const;

        /**
         * @brief Attach script data to this object.
         *
         * The script data is stored as a SmartPtr to ensure safe lifetime management
         * according to the engine's conventions.
         *
         * @param data SmartPtr to object used by scripting systems.
         */
        void setScriptData( SmartPtr<ISharedObject> data );

        /**
         * @brief Get the event task flags for this object.
         *
         * Flags may be used by the job/event system to control execution behavior.
         *
         * @return Flags bitmask.
         */
        u32 getEventTaskFlags() const;

        /**
         * @brief Set event task flags for this object.
         *
         * @param eventTaskFlags Flags bitmask to set.
         */
        void setEventTaskFlags( u32 eventTaskFlags );

        /**
         * @brief Increment the load-lock count.
         *
         * Prefer using ScopedLoadLock instead of calling this directly.
         * Each call must be paired with a matching unlockLoad().
         */
        void lockLoad();

        /**
         * @brief Decrement the load-lock count.
         *
         * Must be called once for every lockLoad() call.
         */
        void unlockLoad();

        /**
         * @brief Returns true when at least one ScopedLoadLock is active.
         *
         * Unload implementations should check this and defer teardown while
         * the object is load-locked.
         *
         * @return True if the load-lock count is greater than zero.
         */
        bool isLoadLocked() const;

        /**
         * @brief Lock the object for exclusive access.
         *
         * Default implementation uses internal synchronization primitives.
         * Subclasses may override to provide custom locking behavior.
         */
        virtual void lock();

        /**
         * @brief Try to lock the object for exclusive access.
         *
         * @return True if the lock was acquired.
         */
        virtual bool try_lock();

        /**
         * @brief Unlock exclusive access.
         */
        virtual void unlock();

        /**
         * @brief Lock the object for shared/read access.
         *
         * Multiple readers may hold the shared lock concurrently.
         */
        virtual void lock_shared();

        /**
         * @brief Unlock the shared/read lock.
         */
        virtual void unlock_shared();

        /**
         * @brief Obtain a SmartPtr<B> to this object from a const context.
         *
         * This enables the pattern where 'this' is converted into a SmartPtr to
         * the concrete type without allocating a new control block. It relies on
         * SmartPtr's support for wrapping raw pointers to objects that are already
         * managed by the shared pointer system.
         *
         * @tparam B Concrete type derived from ISharedObject.
         * @return SmartPtr<B> referencing this object.
         */
        template <class B>
        const SmartPtr<B> getSharedFromThis() const;

        /**
         * @brief Obtain a SmartPtr<B> to this object from a non-const context.
         *
         * @tparam B Concrete type derived from ISharedObject.
         * @return SmartPtr<B> referencing this object.
         */
        template <class B>
        SmartPtr<B> getSharedFromThis();

        WP_CLASS_REGISTER_DECL;

        ///< Pointer to the next shared object in a linked list (used by some managers).
        AtomicRawPtr<ISharedObject> next;

    protected:
        /**
         * @brief Perform object destruction when the final strong reference is released.
         *
         * destroySharedObject() encapsulates teardown logic invoked from removeReference().
         * Subclasses may override to implement custom destruction or pooling behavior
         * but must ensure base-class cleanup is performed.
         */
        void destroySharedObject();

        ///< Atomic strong reference count (initialised to 1).
        atomic_s32 m_references = 1;

        ///< Atomic weak reference count (initialised to 0).
        atomic_s32 m_weakReferences = 0;

        ///< Atomic loading state for this object.
        AtomicNumber<LoadingState> m_loadingState = LoadingState::Allocated;

        ///< Atomic load-lock count. Non-zero while one or more ScopedLoadLock guards are active.
        atomic_s32 m_loadLockCount = 0;

        ///< Pointer to additional shared object metadata (atomic raw pointer for lock-free access).
        mutable AtomicRawPtr<SharedObjectData> m_sharedObjectData;

#if WP_TRACK_REFERENCES
        ///< Optional tracking information used in reference-tracking builds.
        SharedObjectTracker::ObjectData *m_referenceData = nullptr;
#endif
    };

    template <class B>
    const SmartPtr<B> ISharedObject::getSharedFromThis() const
    {
        auto p = static_cast<const B *>( this );
        return SmartPtr<B>( p );
    }

    template <class B>
    SmartPtr<B> ISharedObject::getSharedFromThis()
    {
        auto p = static_cast<B *>( this );
        return SmartPtr<B>( p );
    }

#if WP_TRACK_REFERENCES == 0
    WPForceInline s32 ISharedObject::addReference()
    {
        return ++m_references;
    }
#endif

    WPForceInline s32 ISharedObject::addWeakReference()
    {
        return ++m_weakReferences;
    }

    WPForceInline s32 ISharedObject::getWeakReferences() const
    {
        return m_weakReferences;
    }

    WPForceInline s32 ISharedObject::getReferences() const
    {
        return m_references;
    }

    WPForceInline bool ISharedObject::isAlive() const
    {
        return ( m_objectFlags & OBJECT_FLAG_ALIVE ) != 0;
    }

    WPForceInline bool ISharedObject::isLoaded() const
    {
        return m_loadingState == LoadingState::Loaded;
    }

    WPForceInline bool ISharedObject::isPoolElement() const
    {
        return ( m_objectFlags & OBJECT_FLAG_POOL_ELEMENT ) != 0;
    }

    WPForceInline bool ISharedObject::isGarbageCollected() const
    {
        return ( m_objectFlags & OBJECT_FLAG_GARBAGE_COLLECTED ) != 0;
    }

    WPForceInline bool ISharedObject::isLoading() const
    {
        return m_loadingState == LoadingState::Loading;
    }

    WPForceInline bool ISharedObject::isLoadingQueued() const
    {
        return m_loadingState == LoadingState::LoadingQueued;
    }

    WPForceInline LoadingState ISharedObject::getLoadingState() const
    {
        return m_loadingState;
    }

    WPForceInline ISharedObject::ScopedLoadLock::ScopedLoadLock( ISharedObject *object ) :
        m_object( object )
    {
        if( m_object )
            m_object->lockLoad();
    }

    WPForceInline ISharedObject::ScopedLoadLock::~ScopedLoadLock()
    {
        if( m_object )
            m_object->unlockLoad();
    }

    WPForceInline void ISharedObject::lockLoad()
    {
        ++m_loadLockCount;
    }

    WPForceInline void ISharedObject::unlockLoad()
    {
        --m_loadLockCount;
    }

    WPForceInline bool ISharedObject::isLoadLocked() const
    {
        return m_loadLockCount > 0;
    }

    WPForceInline bool ISharedObject::ScopedLoadLock::isLoaded() const
    {
        if( m_object )
        {
            return m_object->isLoaded();
        }

        return false;
    }

}  // namespace workphone

#endif  // ISharedObject_h__
