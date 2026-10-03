#ifndef __PrefabManager_H
#define __PrefabManager_H

#include <Workphone/Interface/Scene/IGamePrefabManager.hpp>
#include <Workphone/Core/StringTypes.hpp>

namespace workphone
{
    namespace scene
    {
        /**
         * @class PrefabManager
         * @brief Manages the loading, saving, instantiation, and management of prefab resources.
         *
         * The PrefabManager is responsible for handling prefab resources, including their creation,
         * loading from files, saving, instantiation into actors, and destruction. It implements the
         * IPrefabManager interface and provides resource management functionality for prefabs within
         * the engine.
         */
        class WPCore_API GamePrefabManager : public IGamePrefabManager
        {
        public:
            /**
             * @brief Constructor.
             *
             * Initializes a new instance of the PrefabManager class.
             */
            GamePrefabManager();

            /**
             * @brief Destructor.
             *
             * Cleans up resources used by the PrefabManager.
             */
            ~GamePrefabManager() override;

            /**
             * @copydoc IPrefabManager::load
             * @brief Loads prefab data from a shared object.
             * @param data The shared object containing prefab data to load.
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IPrefabManager::unload
             * @brief Unloads prefab data from a shared object.
             * @param data The shared object containing prefab data to unload.
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @copydoc IPrefabManager::createInstance
             * @brief Creates an instance of a prefab as an actor.
             * @param prefab The prefab to instantiate.
             * @return A smart pointer to the created actor instance.
             */
            SmartPtr<IGameActor> createInstance( SmartPtr<IGameActor> prefab ) override;

            /**
             * @copydoc IPrefabManager::loadActor
             * @brief Loads an actor from properties and attaches it to a parent actor.
             * @param data The properties describing the actor to load.
             * @param parent The parent actor to attach the loaded actor to.
             * @return A smart pointer to the loaded actor.
             */
            SmartPtr<IGameActor> loadActor( SmartPtr<Properties> data, SmartPtr<IGameActor> parent,
                                            bool cascade = true ) override;

            /**
             * @copydoc IPrefabManager::loadPrefab
             * @brief Loads a prefab from a file.
             * @param filePath The path to the prefab file.
             * @return A smart pointer to the loaded prefab.
             */
            SmartPtr<IGamePrefab> loadPrefab( const String &filePath ) override;

            /**
             * @copydoc IPrefabManager::savePrefab
             * @brief Saves a prefab to a file.
             * @param filePath The path to save the prefab file to.
             * @param prefab The prefab actor to save.
             */
            void savePrefab( const String &filePath, SmartPtr<IGameActor> prefab ) override;

            /**
             * @copydoc IResourceManager::create
             * @brief Creates a new resource with the specified UUID.
             * @param uuid The unique identifier for the resource.
             * @return A smart pointer to the created resource.
             */
            SmartPtr<IResource> create( const String &uuid ) override;

            /**
             * @copydoc IResourceManager::create
             * @brief Creates a new resource with the specified UUID and name.
             * @param uuid The unique identifier for the resource.
             * @param name The name of the resource.
             * @return A smart pointer to the created resource.
             */
            SmartPtr<IResource> create( const String &uuid, const String &name ) override;

            /**
             * @copydoc IResourceManager::createOrRetrieve
             * @brief Creates or retrieves a resource with the specified UUID, path, and type.
             * @param uuid The unique identifier for the resource.
             * @param path The path to the resource.
             * @param type The type of the resource.
             * @return A pair containing the resource and a boolean indicating if it was created (true)
             * or retrieved (false).
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                              const String &type ) override;

            /**
             * @copydoc IResourceManager::createOrRetrieve
             * @brief Creates or retrieves a resource with the specified path.
             * @param path The path to the resource.
             * @return A pair containing the resource and a boolean indicating if it was created (true)
             * or retrieved (false).
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override;

            /**
             * @copydoc IResourceManager::destroyResource
             * @brief Destroys the specified resource.
             * @param resource The resource to destroy.
             */
            void destroyResource( SmartPtr<IResource> resource ) override;

            /**
             * @copydoc IResourceManager::destroyAll
             * @brief Destroys all managed resources.
             */
            void destroyAll() override;

            /**
             * @copydoc IResourceManager::saveToFile
             * @brief Saves a resource to a file.
             * @param filePath The path to save the resource file to.
             * @param resource The resource to save.
             */
            void saveToFile( const String &filePath, SmartPtr<IResource> resource ) override;

            /**
             * @copydoc IResourceManager::loadFromFile
             * @brief Loads a resource from a file.
             * @param filePath The path to the resource file.
             * @return A smart pointer to the loaded resource.
             */
            SmartPtr<IResource> loadFromFile( const String &filePath ) override;

            /**
             * @copydoc IResourceManager::load
             * @brief Loads a resource by name.
             * @param name The name of the resource to load.
             * @return A smart pointer to the loaded resource.
             */
            SmartPtr<IResource> loadResource( const String &name ) override;

            /**
             * @copydoc IResourceManager::getByName
             * @brief Retrieves a resource by its name.
             * @param name The name of the resource.
             * @return A smart pointer to the resource, or nullptr if not found.
             */
            SmartPtr<IResource> getByName( const String &name ) override;

            /**
             * @copydoc IResourceManager::getById
             * @brief Retrieves a resource by its UUID.
             * @param uuid The unique identifier of the resource.
             * @return A smart pointer to the resource, or nullptr if not found.
             */
            SmartPtr<IResource> getById( const String &uuid ) override;

            /**
             * @brief Clones a resource with a new name.
             * @param resource The resource to clone.
             * @param clonedResourceName The name for the cloned resource.
             * @return A smart pointer to the cloned resource.
             */
            SmartPtr<IResource> cloneResource( SmartPtr<IResource> resource,
                                               const String &clonedResourceName ) override;

            /**
             * @brief Clones a resource by name with a new name.
             * @param name The name of the resource to clone.
             * @param clonedResourceName The name for the cloned resource.
             * @return A smart pointer to the cloned resource.
             */
            SmartPtr<IResource> cloneResource( const String &name,
                                               const String &clonedResourceName ) override;

            /**
             * @brief Gets the data format used for prefab serialization.
             * @return The data format (e.g., XML, JSON).
             */
            DataFormat getDataFormat() const;

            /**
             * @brief Sets the data format used for prefab serialization.
             * @param dataFormat The data format to set (e.g., XML, JSON).
             */
            void setDataFormat( DataFormat dataFormat );

            /**
             * @copydoc IResourceManager::_getObject
             * @brief Gets the underlying object pointer for internal use.
             * @param ppObject Pointer to the object pointer to receive the address.
             */
            void _getObject( void **ppObject ) const override;

            /**
             * @brief Get the raw pointer to the attached IStateContext.
             * Useful when C-style pointer access is required. The returned pointer is not
             * accompanied by ownership guarantees; prefer getStateContext() for ownership.
             * @return Raw IStateContext pointer or nullptr if none set.
             */
            IStateContext *getStateContextPtr() const override;

            /**
             * @brief Get the SmartPtr to the attached IStateContext.
             * Returns the internal smart pointer that owns the state context.
             * @return SmartPtr<IStateContext> reference (may be nullptr).
             */
            SmartPtr<IStateContext> getStateContext() const override;

            /**
             * @brief Attach a state context to this object.
             * The provided SmartPtr will be stored and later unloaded/removed by
             * destroyStateContext() during object unload.
             * @param stateContext Smart pointer to the state context to attach.
             */
            void setStateContext( SmartPtr<IStateContext> stateContext ) override;

            bool handleStateMessage( const SmartPtr<IStateMessage> &message ) override;

            bool handleStateChanged( SmartPtr<IState> &state ) override;

            /**
             * @copydoc IGamePrefabManager::lock
             * @brief Locks the game prefab manager for thread-safe operations.
             */
            void lock() override;

            /**
             * @copydoc IGamePrefabManager::try_lock
             * @brief Attempts to lock the game prefab manager for thread-safe operations.
             * @return True if the lock was acquired, false otherwise.
             */
            bool try_lock() override;

            /**
             * @copydoc IGamePrefabManager::unlock
             * @brief Unlocks the game prefab manager.
             */
            void unlock() override;

            WP_CLASS_REGISTER_DECL;

        private:
            /**
             * @brief Array of managed prefab resources.
             */
            Array<SmartPtr<IResource>> m_prefabs;

            /**
             * @brief The data format used for prefab serialization (default: XML).
             */
            DataFormat m_dataFormat = DataFormat::XML;
        };
    }  // namespace scene
}  // namespace workphone

#endif
