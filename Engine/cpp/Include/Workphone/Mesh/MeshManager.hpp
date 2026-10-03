#ifndef __FBMeshManager__H
#define __FBMeshManager__H

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/ConcurrentArray.hpp>
#include <Workphone/Thread/RecursiveMutex.hpp>

namespace workphone
{
    /**
     * @class MeshManager
     * @brief Manages mesh resources and provides mesh loading, saving, and manipulation functionality.
     *
     * The MeshManager is responsible for:
     * - Loading and saving mesh files from various formats
     * - Managing collections of mesh objects and mesh resources
     * - Providing mesh creation and retrieval operations
     * - Generating tangent data for meshes
     * - Thread-safe mesh resource management using concurrent arrays
     *
     * This class inherits from IResourceManager and provides mesh-specific implementations
     * for resource management operations. It supports loading meshes from supported formats
     * and maintains internal collections of both IMesh and IMeshResource objects.
     *
     * @see IResourceManager
     * @see IMesh
     * @see IMeshResource
     */
    class WPCore_API MeshManager : public IResourceManager
    {
    public:
        /**
         * @brief Default constructor.
         *
         * Initializes a new MeshManager instance with empty mesh collections.
         */
        MeshManager();

        /**
         * @brief Virtual destructor.
         *
         * Properly destroys the MeshManager and releases all managed resources.
         */
        ~MeshManager() override;

        /**
         * @brief Unloads all managed mesh resources and clears internal collections.
         *
         * This method unloads all meshes and mesh resources, clearing the internal
         * concurrent arrays and setting the loading state appropriately.
         *
         * @param data Shared object data (unused in this implementation)
         *
         * @see ISharedObject::unload
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Adds a mesh to the managed collection.
         *
         * Adds the specified mesh to the internal concurrent array of meshes.
         * The method ensures uniqueness - the same mesh cannot be added twice.
         *
         * @param mesh The mesh to add to the collection
         *
         * @pre mesh must not be null
         * @pre mesh must not already exist in the collection
         *
         * @note This operation is thread-safe
         */
        void addMesh( SmartPtr<IMesh> mesh );

        /**
         * @brief Removes a mesh from the managed collection.
         *
         * Removes the specified mesh from the internal concurrent array of meshes.
         * If the mesh is not found, the operation has no effect.
         *
         * @param mesh The mesh to remove from the collection
         *
         * @note This operation is thread-safe
         */
        void removeMesh( SmartPtr<IMesh> mesh );

        /**
         * @brief Adds a mesh resource to the managed collection.
         *
         * Adds the specified mesh resource to the internal concurrent array of mesh resources.
         * The method ensures uniqueness - the same mesh resource cannot be added twice.
         *
         * @param meshResource The mesh resource to add to the collection
         *
         * @pre meshResource must not be null
         * @pre meshResource must not already exist in the collection
         *
         * @note This operation is thread-safe
         */
        void addMeshResource( SmartPtr<IMeshResource> meshResource );

        /**
         * @brief Removes a mesh resource from the managed collection.
         *
         * Removes the specified mesh resource from the internal concurrent array of mesh resources.
         * If the mesh resource is not found, the operation has no effect.
         *
         * @param meshResource The mesh resource to remove from the collection
         *
         * @note This operation is thread-safe
         */
        void removeMeshResource( SmartPtr<IMeshResource> meshResource );

        /**
         * @brief Finds a mesh by name in the managed collection.
         *
         * Searches through all managed meshes to find one with the specified name.
         *
         * @param name The name of the mesh to find
         * @return Smart pointer to the mesh if found, nullptr otherwise
         *
         * @note This operation performs a linear search through all meshes
         */
        SmartPtr<IMesh> findMesh( const String &name );

        /**
         * @brief Loads a mesh from a file path.
         *
         * Loads a mesh from the specified file path. Supports various mesh formats
         * including .fbmeshbin and other formats supported by the mesh loader.
         * Creates a corresponding mesh resource and adds it to the managed collection.
         *
         * @param filePath The file path to load the mesh from
         * @return Smart pointer to the loaded mesh, nullptr if loading failed
         *
         * @note Supported formats include .fbmeshbin and formats supported by
         * ApplicationUtil::isSupportedMesh
         * @note Creates and manages associated MeshResource internally
         */
        SmartPtr<IMesh> loadMesh( const String &filePath );

        /**
         * @brief Saves a mesh to a file path.
         *
         * Saves the specified mesh to the given file path using the mesh serializer.
         * The mesh must be valid before saving.
         *
         * @param mesh The mesh to save
         * @param filePath The file path where to save the mesh
         *
         * @pre mesh must not be null
         * @pre mesh must be valid (mesh->isValid() returns true)
         *
         * @note Currently saves in binary mesh format
         */
        void saveMesh( SmartPtr<IMesh> mesh, const String &filePath );

        /**
         * @brief Creates a new mesh resource with the specified UUID.
         *
         * @param uuid The unique identifier for the new mesh resource
         * @return Smart pointer to the created mesh resource
         *
         * @see IResourceManager::create
         */
        SmartPtr<IResource> create( const String &uuid ) override;

        /**
         * @brief Creates a new mesh resource with the specified UUID and name.
         *
         * @param uuid The unique identifier for the new mesh resource
         * @param name The name for the new mesh resource (currently unused)
         * @return Smart pointer to the created mesh resource
         *
         * @see IResourceManager::create
         */
        SmartPtr<IResource> create( const String &uuid, const String &name ) override;

        /**
         * @brief Creates or retrieves a mesh resource by UUID, path, and type.
         *
         * If a mesh resource with the same file system ID already exists, it returns
         * the existing resource. Otherwise, creates a new mesh resource with the
         * specified parameters and settings file handling.
         *
         * @param uuid The unique identifier for the mesh resource
         * @param path The file path of the mesh resource
         * @param type The type of the mesh resource (currently unused)
         * @return Pair containing the mesh resource and a boolean indicating if it was newly created
         *
         * @see IResourceManager::createOrRetrieve
         */
        Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                          const String &type ) override;

        /**
         * @brief Creates or retrieves a mesh resource by path.
         *
         * If a mesh resource with the same file system ID already exists, it returns
         * the existing resource. Otherwise, creates a new mesh resource with an
         * auto-generated UUID and settings file handling.
         *
         * @param path The file path of the mesh resource
         * @return Pair containing the mesh resource and a boolean indicating if it was newly created
         *
         * @see IResourceManager::createOrRetrieve
         */
        Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path ) override;

        /**
         * @brief Destroys a mesh resource.
         *
         * @param resource The mesh resource to destroy
         *
         * @note Currently not implemented
         *
         * @see IResourceManager::destroyResource
         */
        void destroyResource( SmartPtr<IResource> resource ) override;

        /**
         * @brief Destroys all managed mesh resources.
         *
         * @note Currently not implemented
         *
         * @see IResourceManager::destroyAll
         */
        void destroyAll() override;

        /**
         * @brief Saves a mesh resource to a file.
         *
         * @param filePath The file path where to save the resource
         * @param resource The mesh resource to save
         *
         * @note Currently not implemented
         *
         * @see IResourceManager::saveToFile
         */
        void saveToFile( const String &filePath, SmartPtr<IResource> resource ) override;

        /**
         * @brief Loads a mesh resource from a file.
         *
         * Loads a mesh resource from the specified file path. Handles various mesh formats
         * and creates appropriate mesh resources with settings file management.
         *
         * @param filePath The file path to load the mesh resource from
         * @return Smart pointer to the loaded mesh resource, nullptr if loading failed
         *
         * @see IResourceManager::loadFromFile
         */
        SmartPtr<IResource> loadFromFile( const String &filePath ) override;

        /**
         * @brief Loads a mesh resource by name.
         *
         * @param name The name of the mesh resource to load
         * @return Smart pointer to the loaded mesh resource, nullptr if not found
         *
         * @note Currently not implemented
         *
         * @see IResourceManager::loadResource
         */
        SmartPtr<IResource> loadResource( const String &name ) override;

        /**
         * @brief Gets a mesh resource by name.
         *
         * @param name The name of the mesh resource to retrieve
         * @return Smart pointer to the mesh resource, nullptr if not found
         *
         * @note Currently not implemented
         *
         * @see IResourceManager::getByName
         */
        SmartPtr<IResource> getByName( const String &name ) override;

        /**
         * @brief Gets a mesh resource by UUID.
         *
         * @param uuid The UUID of the mesh resource to retrieve
         * @return Smart pointer to the mesh resource, nullptr if not found
         *
         * @note Currently not implemented
         *
         * @see IResourceManager::getById
         */
        SmartPtr<IResource> getById( const String &uuid ) override;

        /**
         * @brief Clones a mesh resource.
         *
         * @param resource The mesh resource to clone
         * @param clonedResourceName The name for the cloned resource
         * @return Smart pointer to the cloned mesh resource, nullptr if cloning failed
         *
         * @note Currently not implemented
         *
         * @see IResourceManager::cloneResource
         */
        SmartPtr<IResource> cloneResource( SmartPtr<IResource> resource,
                                           const String &clonedResourceName ) override;

        /**
         * @brief Clones a mesh resource by name.
         *
         * @param name The name of the mesh resource to clone
         * @param clonedResourceName The name for the cloned resource
         * @return Smart pointer to the cloned mesh resource, nullptr if cloning failed
         *
         * @note Currently not implemented
         *
         * @see IResourceManager::cloneResource
         */
        SmartPtr<IResource> cloneResource( const String &name,
                                           const String &clonedResourceName ) override;

        /**
         * @brief Gets a pointer to the underlying graphics system object.
         *
         * @param ppObject Output parameter to store the object pointer (always set to nullptr)
         *
         * @see IResourceManager::_getObject
         */
        void _getObject( void **ppObject ) const override;

        /**
         * @brief Generates tangent vectors for a mesh by name.
         *
         * Calculates and adds tangent and bitangent vectors to the mesh geometry,
         * which are required for normal mapping and other advanced shading techniques.
         *
         * @param meshName The name of the mesh to generate tangents for
         *
         * @note Currently not implemented
         */
        void generateTangents( const String &meshName );

        /**
         * @brief Generates tangent vectors for a mesh resource.
         *
         * Calculates and adds tangent and bitangent vectors to the mesh geometry,
         * which are required for normal mapping and other advanced shading techniques.
         *
         * @param resource The mesh resource to generate tangents for
         *
         * @note Currently not implemented
         */
        void generateTangents( SmartPtr<IResource> resource );

        /**
         * @brief Generates tangent vectors for a mesh.
         *
         * Calculates and adds tangent and bitangent vectors to the mesh geometry,
         * which are required for normal mapping and other advanced shading techniques.
         *
         * @param mesh The mesh to generate tangents for
         *
         * @note Currently not implemented
         */
        void generateTangents( SmartPtr<IMesh> mesh );

        /**
         * @brief Calculates tangent vectors for a specific submesh.
         *
         * Computes tangent and bitangent vectors for the specified submesh based on its vertex data.
         * This is typically called after loading or modifying the submesh geometry.
         *
         * @param subMesh The submesh to calculate tangents for
         *
         * @note This operation is necessary for normal mapping and other advanced shading techniques
         */
        void calculateTangentsForSubMesh( SmartPtr<ISubMesh> subMesh );

        /**
         * @brief Locks the mesh for thread-safe operations.
         *
         * Acquires the internal mutex to ensure thread-safe access to mesh data.
         * This should be called before performing read operations from multiple threads.
         *
         * @post The mesh is locked for the current thread
         *
         * @note Always pair with unlock() to avoid deadlocks
         * @see unlock(), isValid()
         */
        void lock() override;

        /**
         * @brief Attempts to lock the mesh for thread-safe operations.
         *
         * Tries to acquire the internal mutex without blocking. If the mutex is already
         * locked by another thread, this method returns false.
         *
         * @return True if the mesh was successfully locked, false if it is already locked
         *
         * @note Always pair with unlock() if lock() succeeds to avoid deadlocks
         * @see lock(), unlock(), isValid()
         */
        bool try_lock() override;

        /**
         * @brief Unlocks the mesh after thread-safe operations.
         *
         * Releases the internal mutex to allow other threads to access mesh data.
         * This should be called after completing thread-safe operations.
         *
         * @pre The mesh must be locked by the current thread
         * @post The mesh is unlocked
         *
         * @note Always call after lock() to avoid deadlocks
         * @see lock(), isValid()
         */
        void unlock() override;

        /**
         * @brief Validates the internal state of the mesh manager.
         *
         * Checks that all internal collections are in a valid state, including
         * verifying that mesh resource arrays contain unique entries.
         *
         * @return true if the mesh manager is in a valid state, false otherwise
         *
         * @see ISharedObject::isValid
         */
        bool isValid() const override;

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

        WP_CLASS_REGISTER_DECL;

    private:
        /**
         * @brief Thread-safe collection of managed mesh objects.
         *
         * Contains all mesh objects currently managed by this manager.
         * Uses atomic shared pointer for thread-safe access.
         */
        ConcurrentArray<SmartPtr<IMesh>> m_meshes;

        /**
         * @brief Thread-safe collection of managed mesh resource objects.
         *
         * Contains all mesh resource objects currently managed by this manager.
         * Uses atomic shared pointer for thread-safe access.
         */
        ConcurrentArray<SmartPtr<IMeshResource>> m_meshResources;
    };
}  // namespace workphone

#endif
