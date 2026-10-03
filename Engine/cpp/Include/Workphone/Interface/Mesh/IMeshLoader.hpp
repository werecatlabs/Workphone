#ifndef IMeshLoader_h__
#define IMeshLoader_h__

#include <Workphone/WorkphonePrerequisites.hpp>
#include <Workphone/Interface/Memory/ISharedObject.hpp>

namespace workphone
{

    /**
     * @brief Interface for a mesh loader.
     *
     * This interface defines methods for loading meshes and actors from various sources,
     * such as resource data or file paths. It also provides functionality for creating
     * materials and managing mesh-related settings.
     */
    class WPCore_API IMeshLoader : public ISharedObject
    {
    public:
        /**
         * @brief Virtual destructor.
         *
         * Ensures proper cleanup of derived classes.
         */
        ~IMeshLoader() override;

        /**
         * @brief Loads an actor from the given mesh resource.
         *
         * @param resource A smart pointer to the mesh resource to load the actor from.
         * @return A smart pointer to the loaded actor.
         */
        virtual SmartPtr<scene::IGameActor> loadActor( SmartPtr<IMeshResource> resource ) = 0;

        /**
         * @brief Loads an actor from the specified file path.
         *
         * @param meshPath The file path of the mesh to load the actor from.
         * @return A smart pointer to the loaded actor.
         */
        virtual SmartPtr<scene::IGameActor> loadActor( const String &meshPath ) = 0;

        /**
         * @brief Loads a mesh from the given mesh resource.
         *
         * @param resource A smart pointer to the mesh resource to load the mesh from.
         * @return A smart pointer to the loaded mesh.
         */
        virtual SmartPtr<IMesh> loadMesh( SmartPtr<IMeshResource> resource ) = 0;

        /**
         * @brief Loads a mesh from the specified file path.
         *
         * @param meshPath The file path of the mesh to load.
         * @return A smart pointer to the loaded mesh.
         */
        virtual SmartPtr<IMesh> loadMesh( const String &meshPath ) = 0;

        /**
         * @brief Creates materials from the specified mesh path.
         *
         * @param meshPath The file path of the mesh to create materials for.
         * @return An array of materials created from the mesh.
         */
        virtual Array<render::IMaterial> createMaterials( const String &meshPath ) = 0;

        /**
         * @brief Checks if nodes are combined into a single mesh.
         *
         * @return True if nodes are combined into one mesh, false otherwise.
         */
        virtual bool getUseSingleMesh() const = 0;

        /**
         * @brief Sets whether nodes should be combined into a single mesh.
         *
         * @param useSingleMesh True to combine nodes into one mesh, false otherwise.
         */
        virtual void setUseSingleMesh( bool useSingleMesh ) = 0;

        /**
         * @brief Checks if existing meshes are overwritten during loading.
         *
         * @return True if overwriting is enabled, false otherwise.
         */
        virtual bool getOverwrite() const = 0;

        /**
         * @brief Sets whether existing meshes should be overwritten during loading.
         *
         * @param overwrite True to enable overwriting, false otherwise.
         */
        virtual void setOverwrite( bool overwrite ) = 0;

        WP_CLASS_REGISTER_DECL;
    };

}  // namespace workphone

#endif  // IMeshLoader_h__
