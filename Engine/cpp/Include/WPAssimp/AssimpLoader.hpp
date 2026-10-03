#ifndef MeshLoader_h__
#define MeshLoader_h__

#include <WPAssimp/WPAssimpPrerequisites.hpp>
#include <Workphone/Interface/Mesh/IMeshLoader.hpp>
#include <Workphone/Core/Array.hpp>
#include <Workphone/Core/Parameter.hpp>
#include <Workphone/Core/StringTypes.hpp>
#include <Workphone/Math/Vector3.hpp>
#include <Workphone/Math/Matrix4.hpp>
#include <set>

#if WP_USE_FBXSDK
//#include <fbxsdk.hpp>
#elif WP_USE_ASSET_IMPORT
#    include <assimp/importer.hpp>
#    include <assimp/scene.h>
#    include <assimp/postprocess.h>
#endif

namespace workphone
{
    /**
     * @brief Loads and manages 3D mesh resources using Assimp or FBX SDK.
     *
     * MeshLoader is responsible for importing, processing, and managing mesh data,
     * including materials, skeletons, and animations. It supports loading from file paths
     * or mesh resources, and provides options for single-mesh import, scaling, and more.
     */
    class AssimpLoader : public IMeshLoader
    {
    public:
        struct MaterialImportOptions
        {
            String materialsFolderName = String( "Materials" );
            String defaultMaterialNamePrefix = String( "Material" );
            bool createMaterialFiles = true;
            bool overwriteMaterialFiles = false;
            bool assignMaterialComponents = true;
            bool importTextures = true;
            bool importMaterialColours = true;
            bool importPbrProperties = true;
            bool useFallbackTextureNames = false;
        };

        /**
         * @brief Constructs a MeshLoader instance.
         */
        AssimpLoader();

        /**
         * @brief Destroys the MeshLoader instance and releases resources.
         */
        ~AssimpLoader() override;

        /**
         * @brief Loads mesh data from the given shared object.
         * @param data Shared pointer to the data object to load.
         */
        void load( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Unloads mesh data associated with the given shared object.
         * @param data Shared pointer to the data object to unload.
         */
        void unload( SmartPtr<ISharedObject> data ) override;

        /**
         * @brief Creates materials for a mesh at the specified path.
         * @param meshPath Path to the mesh file.
         * @return Array of created material interfaces.
         */
        Array<render::IMaterial> createMaterials( const String &meshPath ) override;

        /**
         * @copydoc IMeshLoader::loadActor
         * @brief Loads an actor from a mesh resource.
         * @param resource Shared pointer to the mesh resource.
         * @return Shared pointer to the loaded actor.
         */
        SmartPtr<scene::IGameActor> loadActor( SmartPtr<IMeshResource> resource ) override;

        /**
         * @copydoc IMeshLoader::loadActor
         * @brief Loads an actor from a mesh file name.
         * @param meshName Name or path of the mesh file.
         * @return Shared pointer to the loaded actor.
         */
        SmartPtr<scene::IGameActor> loadActor( const String &meshName ) override;

        /**
         * @copydoc IMeshLoader::loadMesh
         * @brief Loads a mesh from a mesh resource.
         * @param resource Shared pointer to the mesh resource.
         * @return Shared pointer to the loaded mesh.
         */
        SmartPtr<IMesh> loadMesh( SmartPtr<IMeshResource> resource ) override;

        /**
         * @copydoc IMeshLoader::loadMesh
         * @brief Loads a mesh from a mesh file name.
         * @param meshName Name or path of the mesh file.
         * @return Shared pointer to the loaded mesh.
         */
        SmartPtr<IMesh> loadMesh( const String &meshName ) override;

        /**
         * @brief Gets whether to import the mesh as a single mesh.
         * @return True if using single mesh import, false otherwise.
         */
        bool getUseSingleMesh() const override;

        /**
         * @brief Sets whether to import the mesh as a single mesh.
         * @param useSingleMesh True to use single mesh import, false otherwise.
         */
        void setUseSingleMesh( bool useSingleMesh ) override;

        /**
         * @brief Gets whether quiet mode is enabled (suppresses logging).
         * @return True if quiet mode is enabled, false otherwise.
         */
        bool getQuietMode() const;

        /**
         * @brief Sets whether quiet mode is enabled (suppresses logging).
         * @param quietMode True to enable quiet mode, false otherwise.
         */
        void setQuietMode( bool quietMode );

        /**
         * @brief Gets the mesh resource director.
         * @return Shared pointer to the mesh resource director.
         */
        SmartPtr<scene::MeshResourceDirector> getMeshDirector() const;

        /**
         * @brief Sets the mesh resource director.
         * @param meshDirector Shared pointer to the mesh resource director.
         */
        void setMeshDirector( SmartPtr<scene::MeshResourceDirector> meshDirector );

        /**
         * @brief Gets the current mesh resource.
         * @return Shared pointer to the mesh resource.
         */
        SmartPtr<IMeshResource> getMeshResource() const;

        /**
         * @brief Sets the current mesh resource.
         * @param meshResource Shared pointer to the mesh resource.
         */
        void setMeshResource( SmartPtr<IMeshResource> meshResource );

        MaterialImportOptions getMaterialImportOptions() const;

        void setMaterialImportOptions( const MaterialImportOptions &options );

        String getMaterialsFolderName() const;

        void setMaterialsFolderName( const String &materialsFolderName );

        String getDefaultMaterialNamePrefix() const;

        void setDefaultMaterialNamePrefix( const String &defaultMaterialNamePrefix );

        bool getCreateMaterialFiles() const;

        void setCreateMaterialFiles( bool createMaterialFiles );

        bool getOverwriteMaterialFiles() const;

        void setOverwriteMaterialFiles( bool overwriteMaterialFiles );

        bool getAssignMaterialComponents() const;

        void setAssignMaterialComponents( bool assignMaterialComponents );

        bool getImportTextures() const;

        void setImportTextures( bool importTextures );

        bool getImportMaterialColours() const;

        void setImportMaterialColours( bool importMaterialColours );

        bool getImportPbrProperties() const;

        void setImportPbrProperties( bool importPbrProperties );

        bool getUseFallbackTextureNames() const;

        void setUseFallbackTextureNames( bool useFallbackTextureNames );

        /**
         * @brief Gets the file path of the loaded mesh.
         * @return Mesh file path as a string.
         */
        String getMeshPath() const;

        /**
         * @brief Gets the root actor created from the mesh.
         * @return Shared pointer to the root actor.
         */
        SmartPtr<scene::IGameActor> getRootActor() const;

        /**
         * @brief Sets the root actor created from the mesh.
         * @param rootActor Shared pointer to the root actor.
         */
        void setRootActor( SmartPtr<scene::IGameActor> rootActor );

        /**
         * @brief Gets whether mesh import is enabled.
         * @return True if mesh import is enabled, false otherwise.
         */
        bool getImportMesh() const;

        /**
         * @brief Sets whether mesh import is enabled.
         * @param importMesh True to enable mesh import, false otherwise.
         */
        void setImportMesh( bool importMesh );

        /**
         * @brief Gets the scale applied to the mesh during import.
         * @return Mesh scale as a 3D vector.
         */
        Vector3<f32> getMeshScale() const;

        /**
         * @brief Sets the scale to apply to the mesh during import.
         * @param meshScale Mesh scale as a 3D vector.
         */
        void setMeshScale( const Vector3<f32> &meshScale );

        /**
         * @brief Gets whether to overwrite existing mesh data.
         * @return True if overwrite is enabled, false otherwise.
         */
        bool getOverwrite() const override;

        /**
         * @brief Sets whether to overwrite existing mesh data.
         * @param overwrite True to enable overwrite, false otherwise.
         */
        void setOverwrite( bool overwrite ) override;

        /**
         * @brief Converts an Assimp matrix to the engine's Matrix4F format.
         * @param from Assimp matrix to convert.
         * @return Converted matrix in Matrix4F format.
         */
        Matrix4F convertAssimpMatrix( const aiMatrix4x4 &from );

        SmartPtr<ISkeleton> getSkeleton() const;

        void setSkeleton( SmartPtr<ISkeleton> skeleton );

        /**
         * @copydoc IMeshLoader::lock
         * @brief Locks the mesh loader for thread-safe operations.
         */
        void lock() override;

        /**
         * @copydoc IMeshLoader::try_lock
         * @brief Attempts to lock the mesh loader for thread-safe operations.
         * @return True if the lock was acquired, false otherwise.
         */
        bool try_lock() override;

        /**
         * @copydoc IMeshLoader::unlock
         * @brief Unlocks the mesh loader.
         */
        void unlock() override;

        WP_CLASS_REGISTER_DECL;

    protected:
#if WP_USE_ASSET_IMPORT
        /**
         * @brief Creates materials from an Assimp scene node.
         * @param materials Output array of created materials.
         * @param mScene Pointer to the Assimp scene.
         * @param pNode Pointer to the Assimp node.
         * @param mDir Directory path for material resources.
         */
        void createMaterials( Array<render::IMaterial> &materials, const aiScene *mScene,
                              const aiNode *pNode, const String &mDir );

        /**
         * @brief Loads mesh data from an Assimp scene node into an actor.
         * @param actor Target actor to populate.
         * @param mScene Pointer to the Assimp scene.
         * @param pNode Pointer to the Assimp node.
         * @param mDir Directory path for resources.
         */
        void loadDataFromActor( SmartPtr<scene::IGameActor> actor, const aiScene *mScene,
                                const aiNode *pNode, const String &mDir );

        /**
         * @brief Loads mesh data from an Assimp scene node into a mesh.
         * @param mesh Target mesh to populate.
         * @param mScene Pointer to the Assimp scene.
         * @param pNode Pointer to the Assimp node.
         * @param mDir Directory path for resources.
         */
        void loadToMesh( SmartPtr<IMesh> mesh, const aiScene *mScene, const aiNode *pNode,
                         const String &mDir );

        /**
         * @brief Computes the derived transform for each node in the Assimp scene.
         * @param mScene Pointer to the Assimp scene.
         * @param pNode Pointer to the current Assimp node.
         * @param accTransform Accumulated transformation matrix.
         */
        void computeNodesDerivedTransform( const aiScene *mScene, const aiNode *pNode,
                                           aiMatrix4x4 accTransform );

        SmartPtr<ISkeleton> createSkeleton( const aiScene *scene );

        void collectBoneNodes( const aiNode *node );

        void importAnimations( const aiScene *scene, SmartPtr<ISkeleton> skeleton );

        void writeSkeletonCacheFile( const String &meshPath, SmartPtr<ISkeleton> skeleton );

        Array<SmartPtr<IAnimation>> importSceneAnimations( const aiScene *scene );

        void writeSceneAnimationCacheFile( const String &meshPath,
                                           const Array<SmartPtr<IAnimation>> &animations );

        /**
         * @brief Creates a sub-mesh from an Assimp mesh and node.
         * @param name Name of the sub-mesh.
         * @param index Index of the sub-mesh.
         * @param pNode Pointer to the Assimp node.
         * @param mesh Pointer to the Assimp mesh.
         * @param mat Pointer to the Assimp material.
         * @param mMesh Target mesh to populate.
         * @param mDir Directory path for resources.
         * @return True if the sub-mesh was created successfully.
         */
        bool createSubMesh( const String &name, int index, const aiNode *pNode, const aiMesh *mesh,
                            const aiMaterial *mat, SmartPtr<IMesh> mMesh, const String &mDir );

        /**
         * @brief Imports animation data from Assimp to the engine's skeleton.
         * @param scene Pointer to the Assimp scene.
         * @param skeleton Target skeleton to populate.
         */
        void importAssimpAnimationToOgre( const aiScene *scene, SmartPtr<ISkeleton> skeleton );

        /**
         * @brief Loads animations from an Assimp scene into an actor.
         * @param scene Pointer to the Assimp scene.
         * @param actor Target actor to populate with animations.
         */
        void loadAnimations( const aiScene *scene, SmartPtr<scene::IGameActor> actor );

        /**
         * @brief Creates a material from an Assimp material.
         * @param omat Target material to populate.
         * @param index Material index.
         * @param mat Pointer to the Assimp material.
         * @param folderPath Directory path for material resources.
         */
        void createMaterial( SmartPtr<scene::Material> omat, s32 index, const aiMaterial *mat,
                             const String &folderPath );

        String getMaterialName( const aiMaterial *mat, s32 index ) const;

        String getMaterialsPath( const String &folderPath ) const;

        String getMaterialPath( const aiMaterial *mat, s32 index, const String &folderPath ) const;

        SmartPtr<render::IMaterial> createOrRetrieveMaterial( const aiMaterial *mat, s32 index,
                                                              const String &folderPath );

        SmartPtr<render::IMaterialPass> ensureMaterialPass( SmartPtr<render::IMaterial> material );

        void applyAssimpMaterialProperties( SmartPtr<render::IMaterial> material,
                                            SmartPtr<render::IMaterialPass> pass, const aiMaterial *mat,
                                            s32 index, const String &folderPath );

        SmartPtr<scene::Material> assignMaterialComponent( SmartPtr<scene::IGameActor> actor, u32 index,
                                                           const aiMaterial *mat,
                                                           const String &folderPath );

        bool getUseMeshInstancing() const;

        String getMeshInstanceKey( const aiScene *scene, const aiNode *node ) const;

        String getAssimpMeshSignature( const aiMesh *mesh ) const;

        /// Map of bone names to Assimp nodes.
        using BoneNodeMap = std::map<String, const aiNode *>;
        BoneNodeMap m_boneNodesByName;

        /// Map of bone names to Assimp bones.
        using BoneMap = std::map<String, const aiBone *>;
        BoneMap m_bonesByName;

        /// Map of node names to their derived transformation matrices.
        using NodeTransformMap = std::map<String, aiMatrix4x4>;
        NodeTransformMap m_nodeDerivedTransformByName;

        using SceneActorMap = std::map<String, SmartPtr<scene::IGameActor>>;
        SceneActorMap m_sceneActorsByName;

        struct MeshInstanceRecord
        {
            String meshFilePath;
            SmartPtr<IMeshResource> meshResource;
            SmartPtr<IMesh> mesh;
        };

        using MeshInstanceMap = std::map<String, MeshInstanceRecord>;
        MeshInstanceMap m_meshInstancesByKey;

        using MeshSignatureSet = std::set<String>;
        MeshSignatureSet m_loadedMeshSignatures;

        /// Pointer to the Assimp skeleton structure.
        aiSkeleton *m_aiSkeleton = nullptr;
#endif

        /// Engine skeleton associated with the mesh.
        SmartPtr<ISkeleton> m_skeleton;

        /// Root actor created from the mesh.
        SmartPtr<scene::IGameActor> m_rootActor;

        /// Director for mesh resource management.
        SmartPtr<scene::MeshResourceDirector> m_meshDirector;

        /// Mesh resource currently being loaded or managed.
        SmartPtr<IMeshResource> m_meshResource;

        /// Scale applied to the mesh during import.
        Vector3<f32> m_meshScale = Vector3<f32>::unit();

        /// If true, imports the mesh as a single mesh.
        bool m_useSingleMesh = false;

        /// If true, suppresses logging output.
        bool m_quietMode = false;

        /// If true, enables mesh import.
        bool m_importMesh = true;

        /// If true, overwrites existing mesh data.
        bool m_overwrite = false;

        MaterialImportOptions m_materialImportOptions;

        /// Unique identifier for the mesh file.
        UUID m_fileUUID;

        /// File path of the loaded mesh.
        String m_meshPath;

        String m_meshFileName;

        String m_meshFileExt;

        /// Array of mesh parameters or sub-meshes.
        Array<Parameter> m_meshes;

        mutable RecursiveSpinMutex m_mutex;
    };
}  // namespace workphone

#endif  // MeshLoader_h__
