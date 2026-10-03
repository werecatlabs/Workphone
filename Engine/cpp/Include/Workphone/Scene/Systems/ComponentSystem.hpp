#ifndef ComponentSystem_h__
#define ComponentSystem_h__

#include <Workphone/Interface/Scene/IComponentSystem.hpp>
#include <Workphone/Atomics/AtomicValue.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Thread/RecursiveSpinMutex.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @brief Concrete implementation of a component system.
         *
         * The component system manages storage, lifecycle and state of scene components.
         * It provides allocation/reservation of internal storage, addition and removal of
         * components, and maintains a "dirty" list of components that require processing
         * (for example: update, reload or upload operations). The class is thread-safe
         * and exposes locking primitives for external synchronization when iterating or
         * performing batch updates on the internal arrays.
         */
        class WPCore_API ComponentSystem : public IComponentSystem
        {
        public:
            /**
             * @brief Default constructor.
             *
             * Initializes internal containers and default parameters (such as grow size).
             */
            ComponentSystem();

            /**
             * @brief Virtual destructor.
             *
             * Ensures proper cleanup of held components and resources in derived classes.
             */
            ~ComponentSystem() override;

            /**
             * @copydoc IComponentSystem::load
             *
             * Called to initialize the system using shared configuration or state data.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IComponentSystem::unload
             *
             * Called to teardown the system and release resources.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IComponentSystem::addComponent
             *
             * Adds a component to the internal storage and returns its allocated index.
             */
            u32 addComponent( SmartPtr<IComponent> component ) override;

            /**
             * @copydoc IComponentSystem::removeComponent
             *
             * Removes the provided component instance from the system.
             */
            void removeComponent( SmartPtr<IComponent> component ) override;

            /**
             * @copydoc IComponentSystem::removeComponent
             *
             * Removes the component identified by the supplied id/index.
             */
            void removeComponent( u32 id ) override;

            /**
             * @copydoc IComponentSystem::reserve
             *
             * Ensures internal arrays can hold at least `size` components without
             * reallocating.
             */
            void reserve( size_t size ) override;

            /**
             * @copydoc IComponentSystem::getSize
             *
             * @return Current number of allocated slots in the system.
             */
            size_t getSize() const override;

            /**
             * @copydoc IComponentSystem::setSize
             *
             * Resize the internal storage to contain `size` slots. Shrinking may
             * invalidate existing indices if called while components are present.
             */
            void setSize( size_t size ) override;

            /**
             * @brief Get the grow size used when expanding internal storage.
             *
             * The grow size determines how many slots are added when the storage
             * needs to grow to accommodate new components.
             */
            u32 getGrowSize() const override;

            /**
             * @brief Set the grow size used when expanding internal storage.
             * @param growSize Number of slots to add on expansion.
             */
            void setGrowSize( u32 growSize ) override;

            /**
             * @brief Return a copy of the internal component pointer array.
             *
             * The returned array contains raw pointers to currently stored components.
             */
            Array<IComponent *> getComponents() const;

            /**
             * @brief Replace the internal component pointer array.
             * @param components New array of component pointers to adopt.
             *
             * Primarily used for bulk operations or hot-reload scenarios. The caller
             * must ensure pointers are valid for the lifetime of the system or until
             * they are replaced again.
             */
            void setComponents( const Array<IComponent *> &components );

            /**
             * @copydoc IComponentSystem::isDirty
             *
             * Indicates whether the system has pending changes that require processing.
             */
            bool isDirty() const override;

            /**
             * @copydoc IComponentSystem::setDirty
             *
             * Mark the entire system dirty or clear the dirty flag.
             */
            void setDirty( bool dirty ) override;

            /**
             * @copydoc IComponentSystem::makeDirty
             *
             * Convenience method to mark the system as dirty (equivalent to
             * setDirty(true)).
             */
            void makeDirty() override;

            /**
             * @copydoc IComponentSystem::addDirtyComponent
             *
             * Adds the specified component to the dirty list so it will be processed
             * during the next update pass.
             */
            void addDirtyComponent( SmartPtr<IComponent> component ) override;

            /**
             * @copydoc IComponentSystem::removeDirtyComponent
             *
             * Removes the component from the dirty list if present.
             */
            void removeDirtyComponent( SmartPtr<IComponent> component ) override;

            /**
             * @brief Get the list of components currently marked dirty.
             * @return Array of component pointers that need processing.
             */
            Array<IComponent *> getDirtyComponents() const;

            /**
             * @brief Replace the current dirty list with a new list.
             * @param dirtyComponents New list of components to mark as dirty.
             */
            void setDirtyComponents( const Array<IComponent *> &dirtyComponents );

            /**
             * @brief Handle events dispatched to this component system.
             * @copydoc IEventListener::handleEvent
             *
             * Processes incoming events and routes them to components or adjusts
             * system state accordingly.
             */
            Parameter handleEvent( EventType eventType, hash_type eventValue,
                                   const Array<Parameter> &arguments, SmartPtr<ISharedObject> sender,
                                   SmartPtr<ISharedObject> object, SmartPtr<IEvent> event );

            /**
             * @brief Acquire the internal recursive mutex.
             *
             * Provides external callers the ability to perform synchronized operations
             * on the internal containers.
             */
            void lock() override;

            /**
             * @brief Try to acquire the internal recursive mutex without blocking.
             * @return True if the lock was acquired, false otherwise.
             */
            bool try_lock() override;

            /**
             * @brief Release the internal recursive mutex.
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        protected:
            /**
             * @brief Ensure internal storage can hold at least `size` slots.
             *
             * Subclasses may override to allocate or re-map auxiliary per-slot arrays
             * such as loading state arrays.
             */
            virtual void reserveData( size_t size );

            /**
             * @brief Check whether the given slot index is free (no component stored).
             * @param slot Slot index to test.
             * @return True if the slot is free, false otherwise.
             */
            bool isFreeSlot( u32 slot );

            /**
             * @brief Access the per-slot loading state.
             * @param id Slot index.
             * @return Reference to the AtomicValue holding the loading state for the slot.
             */
            const AtomicValue<LoadingState> &getLoadingState( u32 id ) const;

            /**
             * @brief Set the loading state for a specific slot.
             * @param id Slot index.
             * @param state New loading state to set.
             */
            void setLoadingState( u32 id, LoadingState state );

            /**
             * @brief Store a component pointer into the specified slot.
             * @param index Slot index.
             * @param component Component to store (may be null to clear).
             */
            void setObject( u32 index, SmartPtr<IComponent> component );

            /**
             * @brief Retrieve the component stored at the specified slot.
             * @param index Slot index to query.
             * @return SmartPtr to the component at the slot or null if empty.
             */
            SmartPtr<IComponent> getObject( u32 index ) const;

            /**
             * @brief Number of allocated slots in the internal storage.
             */
            size_t m_size = 0;

            /**
             * @brief Index of the last freed object slot.
             *
             * Stored to accelerate allocation of freed slots.
             */
            atomic_u32 m_lastFreeSlot = 0;

            /**
             * @brief Amount by which the storage will grow when expanded.
             */
            u32 m_growSize = 128;

            /**
             * @brief Global dirty flag for the system. When true, the system has
             * pending changes that should be processed.
             */
            atomic_bool m_dirty = false;

            /**
             * @brief List of component pointers that have been marked dirty and need
             * processing.
             */
            ConcurrentArray<IComponent *> m_dirtyComponents;

            /**
             * @brief Primary storage array of component pointers indexed by slot.
             */
            ConcurrentArray<IComponent *> m_components;

            /**
             * @brief Per-slot loading state flags used during asynchronous load/unload
             * operations.
             */
            ConcurrentArray<AtomicValue<LoadingState>> m_loadingStates;

            /**
             * @brief Recursive mutex protecting access to internal arrays.
             *
             * Marked mutable to allow locking from const methods when necessary.
             */
            mutable RecursiveSpinMutex m_mutex;
        };
    }  // namespace scene
}  // namespace workphone

#endif  // ComponentSystem_h__
