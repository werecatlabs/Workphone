#ifndef __CMaterialManagerOgre_h__
#define __CMaterialManagerOgre_h__

#include <WPGraphicsOgre/WPGraphicsOgrePrerequisites.hpp>
#include <Workphone/Graphics/MaterialManager.hpp>
#include <Workphone/Core/Array.hpp>

namespace workphone
{
    namespace render
    {
        /**
         * @file CMaterialManagerOgre.hpp
         * @brief Ogre-backed implementation of the Workphone material manager.
         *
         * This class bridges the engine's generic MaterialManager interface to the
         * underlying Ogre-based graphics implementation. It is responsible for
         * creating, cloning, loading, retrieving and saving material resources
         * using the engine's resource and material abstractions.
         */

        /**
         * @class CMaterialManagerOgre
         * @brief Concrete MaterialManager implementation using Ogre.
         *
         * Provides material lifecycle operations (create, clone, load, unload,
         * retrieve) adapted for the Ogre graphics backend. Most methods return
         * SmartPtr-managed resources defined by the engine's interfaces.
         */
        class CMaterialManagerOgre : public MaterialManager
        {
        public:
            /** @brief Construct a new Ogre material manager. */
            CMaterialManagerOgre();

            /** @brief Destroy the Ogre material manager. */
            ~CMaterialManagerOgre() override;

            /**
             * @brief Load material data from a shared object.
             *
             * Loads material(s) described by `data` into the manager. The exact
             * expected contents of `data` are implementation-defined (typically
             * material definition structures or serialized material blobs).
             *
             * @param data Smart pointer to a shared object that contains material data.
             *
             * @copydoc IObject::load
             */
            void load( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Unload material data associated with the provided shared object.
             *
             * Releases any resources that were created when `load` was called with the
             * corresponding `data`. After unload the manager will no longer provide the
             * material resources created from that data.
             *
             * @param data Smart pointer to the shared object that was previously loaded.
             *
             * @copydoc IObject::unload
             */
            void unload( SmartPtr<ISharedObject> data ) override;

            /**
             * @brief Clone an existing material instance.
             *
             * Create a new material by cloning an existing material instance.
             *
             * @param material Smart pointer to the source material to clone.
             * @param clonedMaterialName Name to assign to the cloned material.
             * @return SmartPtr<IMaterial> Smart pointer to the newly cloned material.
             *
             * @copydoc IMaterialManager::cloneMaterial
             */
            SmartPtr<IMaterial> cloneMaterial( SmartPtr<IMaterial> material,
                                               const String &clonedMaterialName ) override;

            /**
             * @brief Clone a material identified by name.
             *
             * Locate the material by `name` and create a clone with `clonedMaterialName`.
             *
             * @param name Name of the existing material to clone.
             * @param clonedMaterialName Name to assign to the cloned material.
             * @return SmartPtr<IMaterial> Smart pointer to the newly cloned material.
             *
             * @copydoc IMaterialManager::cloneMaterial
             */
            SmartPtr<IMaterial> cloneMaterial( const String &name,
                                               const String &clonedMaterialName ) override;

            /**
             * @brief Create a new resource with the given name.
             *
             * Creates an engine resource representing a material. The returned resource
             * is managed by SmartPtr.
             *
             * @param name Resource name to create.
             * @return SmartPtr<IResource> Smart pointer to the created resource.
             *
             * @copydoc IResourceManager::create
             */
            SmartPtr<IResource> create( const String &name ) override;

            /**
             * @brief Create a new resource with the given UUID and name.
             *
             * Use this overload when a stable UUID is available and should be associated
             * with the created resource.
             *
             * @param uuid Unique identifier for the resource.
             * @param name Human-readable name for the resource.
             * @return SmartPtr<IResource> Smart pointer to the created resource.
             *
             * @copydoc IResourceManager::create
             */
            SmartPtr<IResource> create( const String &uuid, const String &name ) override;

            /**
             * @brief Create or retrieve an existing resource identified by uuid/path/type.
             *
             * If a matching resource already exists the method returns the existing
             * resource and `bool` set to false. If no resource exists it will create a
             * new one and return it with `bool` set to true.
             *
             * @param uuid The unique identifier to search for or assign.
             * @param path Resource path or location.
             * @param type Resource type string (e.g. "material").
             * @return Pair<SmartPtr<IResource>, bool> First = resource pointer, Second = true if newly created.
             *
             * @copydoc IResourceManager::createOrRetrieve
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &uuid, const String &path,
                                                              const String &type ) override;

            /**
             * @brief Create or retrieve a resource by path.
             *
             * Convenience overload that searches by path only. Returns a pair where
             * the boolean indicates whether a new resource was created.
             *
             * @param path Resource path or identifier.
             * @return Pair<SmartPtr<IResource>, bool> Resource pointer and created flag.
             */
            Pair<SmartPtr<IResource>, bool> createOrRetrieve( const String &path );

            /**
             * @brief Save a resource (material) to disk.
             *
             * Serializes and writes `resource` to the file at `filePath`. Behavior and
             * format are implementation-dependent.
             *
             * @param filePath Destination file path to save the resource.
             * @param resource Smart pointer to the resource to save.
             */
            void saveToFile( const String &filePath, SmartPtr<IResource> resource ) override;

            /**
             * @brief Load a resource from file.
             *
             * Reads and constructs a resource from the file specified by `filePath`.
             *
             * @param filePath Path to the serialized resource file.
             * @return SmartPtr<IResource> Smart pointer to the loaded resource, or null on failure.
             *
             * @copydoc IResourceManager::loadFromFile
             */
            SmartPtr<IResource> loadFromFile( const String &filePath ) override;

            /**
             * @brief Load a resource previously created with `create`.
             *
             * Retrieve and return a resource by its name. This overload may return
             * nullptr if no matching resource exists.
             *
             * @param name Resource name to load.
             * @return SmartPtr<IResource> Smart pointer to the loaded resource.
             *
             * @copydoc IResourceManager::load
             */
            SmartPtr<IResource> load( const String &name );

            /**
             * @brief Retrieve a resource by its name.
             *
             * @param name Name of the resource to retrieve.
             * @return SmartPtr<IResource> Smart pointer to the resource, or null if not found.
             *
             * @copydoc IResourceManager::getByName
             */
            SmartPtr<IResource> getByName( const String &name ) override;

            /**
             * @brief Retrieve a resource by its unique identifier.
             *
             * @param uuid Unique identifier of the resource.
             * @return SmartPtr<IResource> Smart pointer to the resource, or null if not found.
             *
             * @copydoc IResourceManager::getById
             */
            SmartPtr<IResource> getById( const String &uuid ) override;

            /**
             * @brief Obtain the underlying graphics-system-specific object.
             *
             * Outputs a pointer to the native graphics object into `ppObject`. The
             * concrete type and ownership semantics are specific to the graphics backend.
             * In this Ogre implementation this method may set *ppObject to nullptr.
             *
             * @param ppObject Pointer to a void* that will receive the native object pointer.
             */
            void _getObject( void **ppObject ) const override;

            WP_CLASS_REGISTER_DECL;
        };
    }  // end namespace render
}  // namespace workphone

#endif  // __CMaterialManagerOgre_h__
