#ifndef CMaterialManager_h__
#define CMaterialManager_h__

#include <WPGraphics/WPClawHammerPrerequisites.hpp>
#include <Workphone/Graphics/SharedGraphicsObject.hpp>
#include <Workphone/Interface/Graphics/IMaterialManager.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {

        /**
         * @brief Manager responsible for creating, cloning, loading and
         *        storing material resources.
         *
         * MaterialManager implements the IMaterialManager interface and
         * extends SharedGraphicsObject to provide lifecycle management
         * (load/unload/clone) for materials used by the renderer.
         *
         * Responsibilities:
         * - Create new material resources by name or uuid.
         * - Clone existing materials (and resources) producing a copy with a new name.
         * - Locate, save and load material resources from files.
         * - Destroy individual resources or clear all managed materials.
         */
        class WPCore_API MaterialManager : public SharedGraphicsObject<IMaterialManager>
        {
        public:
            class WPCore_API StateListener : public IStateListener
            {
            public:
                /**
                 * @brief Default constructor.
                 * Creates a state listener without an associated material owner.
                 * The owner must be set separately using setOwner().
                 */
                StateListener();

                /**
                 * @brief Virtual destructor.
                 * Properly cleans up the state listener and removes any
                 * remaining state context associations.
                 */
                ~StateListener() override;

                /**
                 * @brief Handles the unload state change.
                 * Called when the material or its resources need to be unloaded.
                 * This ensures proper cleanup of graphics resources.
                 *
                 * @param data Optional context data for the unload operation
                 */
                void unload( SmartPtr<ISharedObject> data ) override;

                /**
                 * @brief Handles incoming state messages.
                 * Processes state messages that may affect the material,
                 * such as resource loading notifications or graphics context changes.
                 * @param message The state message to handle
                 * @return True if the message was handled, false otherwise
                 */
                bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

                /**
                 * @brief Handles state transitions.
                 * Called when the material's state changes, allowing the listener
                 * to respond to loading state transitions and resource changes.
                 * @param state Reference to the new state
                 * @return True if the state change was handled successfully
                 */
                bool handleStateChanged( SmartPtr<IState> &state ) override;

                /**
                 * @brief Gets the material that owns this listener.
                 * Returns a pointer to the material associated with this listener.
                 * @return Pointer to the owner material
                 */
                MaterialManager *getOwnerPtr() const;

                /**
                 * @brief Gets the material that owns this listener.
                 * Returns a smart pointer to the material associated with this listener.
                 * @return Smart pointer to the owner material
                 */
                SmartPtr<MaterialManager> getOwner() const;

                /**
                 * @brief Sets the material owner for this listener.
                 * Associates this listener with the specified material.
                 * @param owner Smart pointer to the material to associate with
                 */
                void setOwner( SmartPtr<MaterialManager> owner );

                WP_CLASS_REGISTER_DECL;

            protected:
                /** Weak pointer to the owning material to avoid circular references */
                AtomicWeakPtr<MaterialManager> m_owner;
            };

            /**
             * @brief Construct a new MaterialManager.
             *
             * The constructor initializes internal containers used to track
             * material objects. Heavy initialization is expected to be done
             * in `load`.
             */
            MaterialManager();

            /**
             * @brief Virtual destructor.
             *
             * Ensures derived cleanup runs and releases any remaining
             * resources when the manager is destroyed.
             */
            ~MaterialManager() override;

            /**
             * @copydoc SharedGraphicsObject<IMaterialManager>::load
             *
             * @param data Optional initialization data passed as a shared object.
             *             Implementation should interpret this pointer according
             *             to the current graphics backend or configuration.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc SharedGraphicsObject<IMaterialManager>::unload
             *
             * @param data Optional data provided when unloading. Implementations
             *             should use this to persist state if required.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Clone a material instance.
             *
             * Produces a duplicated `IMaterial` object based on `material`
             * and registers it under `clonedMaterialName`. The returned smart
             * pointer references the new material. Original material is not
             * modified.
             *
             * @param material Material to clone.
             * @param clonedMaterialName Name to assign to the cloned material.
             * @return SmartPtr<IMaterial> Pointer to the cloned material.
             */
            SmartPtr<IMaterial> cloneMaterial( SmartPtr<IMaterial> material,
                                               const String &clonedMaterialName ) override;

            /**
             * @brief Clone a material by name.
             *
             * Finds the material identified by `name`, clones it and assigns
             * `clonedMaterialName` to the clone. If the source material cannot
             * be found, implementations should return a null SmartPtr.
             *
             * @param name Name of the material to clone.
             * @param clonedMaterialName Name to assign to the cloned material.
             * @return SmartPtr<IMaterial> Pointer to the cloned material or null.
             */
            SmartPtr<IMaterial> cloneMaterial( const String &name,
                                               const String &clonedMaterialName ) override;

            /**
             * @brief Create a new resource (material) with the given name.
             *
             * If a resource with the same name already exists, behavior depends
             * on the underlying implementation (may return existing resource or a new one).
             *
             * @param name User-visible name for the material resource.
             * @return SmartPtr<IResource> New or existing resource instance.
             */
            SmartPtr<IResource> create( const String &name ) override;

            /**
             * @brief Create a new resource (material) with a UUID and name.
             *
             * This overload ensures the resource can be uniquely identified
             * across sessions by `uuid`.
             *
             * @param uuid Unique identifier for the resource.
             * @param name Human readable name for the resource.
             * @return SmartPtr<IResource> New or existing resource instance.
             */
            SmartPtr<IResource> create( const String &uuid, const String &name ) override;

            /**
             * @brief Create or retrieve a resource given uuid, path and type.
             *
             * If a resource matching `uuid` or `path` already exists, it is returned
             * and the boolean return value indicates retrieval (true = created, false = retrieved).
             *
             * @param uuid Unique identifier for the resource.
             * @param path Path or key used to locate the resource.
             * @param type Resource type hint (e.g. "material").
             * @return Pair<SmartPtr<IResource>, bool> First = resource, Second = true if created, false
             * if retrieved.
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                              const String &type ) override;

            /**
             * @brief Create or retrieve a resource by path.
             *
             * Convenience overload that uses only a path to locate or create
             * a resource.
             *
             * @param path Path or key used to locate the resource.
             * @return Pair<SmartPtr<IResource>, bool> First = resource, Second = true if created, false
             * if retrieved.
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override;

            /**
             * @brief Destroy/erase the provided resource from the manager.
             *
             * After calling this the provided SmartPtr should no longer be
             * usable from the manager; actual memory release may occur when
             * all references are released.
             *
             * @param resource Resource to destroy.
             */
            void destroyResource( SmartPtr<IResource> resource ) override;

            /**
             * @brief Destroy all resources managed by this manager.
             *
             * Use with care: this will remove every material tracked by the manager.
             */
            void destroyAll() override;

            /**
             * @brief Save a resource to disk.
             *
             * Implementations should serialize the provided resource to a file
             * at `filePath`. Exceptions or error values should be handled by
             * the caller or underlying IO subsystem.
             *
             * @param filePath Filesystem path to save the resource to.
             * @param resource Resource instance to serialize.
             */
            void saveToFile( const String &filePath, SmartPtr<IResource> resource ) override;

            /**
             * @brief Load a resource from disk.
             *
             * Loads and returns a resource deserialized from `filePath`.
             * Returns a null SmartPtr on failure.
             *
             * @param filePath Filesystem path to load the resource from.
             * @return SmartPtr<IResource> Loaded resource or null if loading failed.
             */
            SmartPtr<IResource> loadFromFile( const String &filePath ) override;

            /**
             * @brief Load a resource by name from internal storage or external source.
             *
             * This is the resource-specific `load` operation (not to be confused with
             * the lifecycle `load` that initializes the manager).
             *
             * @param name Name of the resource to load.
             * @return SmartPtr<IResource> Loaded or found resource.
             */
            SmartPtr<IResource> loadResource( const String &name ) override;

            /**
             * @brief Retrieve a managed resource by its name.
             *
             * @param name Name of the resource.
             * @return SmartPtr<IResource> Resource if found, otherwise null.
             */
            SmartPtr<IResource> getByName( const String &name ) override;

            /**
             * @brief Retrieve a managed resource by its unique id (uuid).
             *
             * @param uuid Unique identifier of the resource.
             * @return SmartPtr<IResource> Resource if found, otherwise null.
             */
            SmartPtr<IResource> getById( const String &uuid ) override;

            /**
             * @brief Internal accessor for the underlying native graphics object.
             *
             * Many graphics APIs use raw pointers to native objects. This method
             * fills `ppObject` with a pointer to the underlying implementation
             * object if available.
             *
             * @param ppObject Output pointer location to receive the raw object pointer.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Clone a generic resource (material or other resource types).
             *
             * Creates a copy of `resource` and assigns `clonedResourceName` to the copy.
             *
             * @param resource Resource to clone.
             * @param clonedResourceName Name to assign to the cloned resource.
             * @return SmartPtr<IResource> Pointer to the cloned resource.
             */
            SmartPtr<IResource> cloneResource( SmartPtr<IResource> resource,
                                               const String &clonedResourceName ) override;

            /**
             * @brief Clone a generic resource by name.
             *
             * Finds a resource identified by `name`, clones it and returns the clone.
             *
             * @param name Name of the resource to clone.
             * @param clonedResourceName Name to assign to the cloned resource.
             * @return SmartPtr<IResource> Pointer to the cloned resource or null if source not found.
             */
            SmartPtr<IResource> cloneResource( const String &name,
                                               const String &clonedResourceName ) override;

            /**
             * @brief Get the raw pointer to the attached IStateContext.
             * Useful when C-style pointer access is required. The returned pointer is not
             * accompanied by ownership guarantees; prefer getStateContext() for ownership.
             * @return Raw IStateContext pointer or nullptr if none set.
             */
            virtual IStateContext *getStateContextPtr() const;

            /**
             * @brief Get the SmartPtr to the attached IStateContext.
             * Returns the internal smart pointer that owns the state context.
             * @return SmartPtr<IStateContext> reference (may be nullptr).
             */
            virtual SmartPtr<IStateContext> getStateContext() const;

            /**
             * @brief Attach a state context to this object.
             * The provided SmartPtr will be stored and later unloaded/removed by
             * destroyStateContext() during object unload.
             * @param stateContext Smart pointer to the state context to attach.
             */
            virtual void setStateContext( SmartPtr<IStateContext> stateContext );

            /**
             * @brief Handles incoming state messages.
             * Processes state messages that may affect the material,
             * such as resource loading notifications or graphics context changes.
             * @param message The state message to handle
             * @return True if the message was handled, false otherwise
             */
            virtual bool handleStateMessage( const SmartPtr<IStateMessage> &message );

            /**
             * @brief Handles state transitions.
             * Called when the material's state changes, allowing the listener
             * to respond to loading state transitions and resource changes.
             * @param state Reference to the new state
             * @return True if the state change was handled successfully
             */
            virtual bool handleStateChanged( SmartPtr<IState> &state );

            WP_CLASS_REGISTER_DECL;

        protected:
            SmartPtr<IStateContext> m_stateContext;

            /**
             * @brief Container holding currently managed material objects.
             *
             * ConcurrentArray is used to allow safe access from multiple threads
             * where supported. Elements are SmartPtr<IMaterial> instances
             * representing the managed materials.
             */
            ConcurrentArray<SmartPtr<IMaterial>> m_materials;
        };

        inline IStateContext *MaterialManager::getStateContextPtr() const
        {
            return m_stateContext.get();
        }

        inline MaterialManager *MaterialManager::StateListener::getOwnerPtr() const
        {
            return m_owner.get();
        }

    }  // end namespace render
}  // namespace workphone

#endif  // CMaterialManager_h__
