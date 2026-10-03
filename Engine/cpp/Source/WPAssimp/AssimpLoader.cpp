#include <WPAssimp/WPAssimpPCH.hpp>
#include <WPAssimp/AssimpLoader.hpp>
#include <WPAssimp/LogStream.hpp>
#include <WPAssimp/IOSystem.hpp>
#include <Workphone/Workphone.hpp>

#if WP_USE_FBXSDK
#    include <FBMesh/FBX/FBXConverter.h>
#elif WP_USE_ASSET_IMPORT
#    include <assimp/DefaultLogger.hpp>
#    include <assimp/config.h>
#endif

#include <Workphone/Animation/KeyFrameTransform3.hpp>
#include <Workphone/Animation/Animation.hpp>
#include <Workphone/Mesh/MeshSkeleton.hpp>
#include <Workphone/Mesh/XMLSkeletonSerializer.hpp>

#include <set>
#include <tinyxml.h>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, AssimpLoader, IMeshLoader );

    namespace
    {
        TiXmlElement *appendElement( TiXmlNode *parent, const char *name )
        {
            auto element = new TiXmlElement( name );
            parent->LinkEndChild( element );
            return element;
        }

        SmartPtr<scene::Mesh> ensureMeshComponent( SmartPtr<scene::IGameActor> actor,
                                                   const String &meshFilePath,
                                                   SmartPtr<IMeshResource> meshResource )
        {
            if( !actor )
            {
                return nullptr;
            }

            auto meshComponent = actor->getComponent<scene::Mesh>();
            if( !meshComponent )
            {
                meshComponent = actor->addComponent<scene::Mesh>();
            }

            if( meshComponent )
            {
                if( !StringUtil::isNullOrEmpty( meshFilePath ) )
                {
                    meshComponent->setMeshPath( meshFilePath );
                }

                if( meshResource )
                {
                    meshComponent->setMeshResource( meshResource );
                }
            }

            return meshComponent;
        }

        SmartPtr<scene::MeshRenderer> ensureMeshRenderer( SmartPtr<scene::IGameActor> actor,
                                                          const String &meshFilePath )
        {
            if( !actor )
            {
                return nullptr;
            }

            auto meshRenderer = actor->getComponent<scene::MeshRenderer>();
            if( !meshRenderer )
            {
                meshRenderer = actor->addComponent<scene::MeshRenderer>();
            }

            return meshRenderer;
        }

        bool ensureMeshActorComponents( SmartPtr<scene::IGameActor> actor, const String &meshFilePath,
                                        SmartPtr<IMeshResource> meshResource,
                                        SmartPtr<scene::MeshResourceDirector> meshDirector = nullptr )
        {
            auto meshComponent = ensureMeshComponent( actor, meshFilePath, meshResource );
            if( meshComponent && meshDirector )
            {
                meshComponent->setProgressiveMeshOptions(
                    meshDirector->getProgressiveMeshOptions() );
            }
            auto meshRenderer = ensureMeshRenderer( actor, meshFilePath );
            return meshComponent && meshRenderer;
        }

        String getMeshCacheSourceKey( const String &meshPath )
        {
            auto cleanMeshPath = StringUtil::cleanupPath( meshPath );
            if( StringUtil::isNullOrEmpty( cleanMeshPath ) )
            {
                return {};
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return StringUtil::make_lower( cleanMeshPath );
            }

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            auto absoluteMeshPath = cleanMeshPath;
            if( Path::isPathAbsolute( cleanMeshPath ) )
            {
                absoluteMeshPath = Path::getAbsolutePath( cleanMeshPath );
            }
            else
            {
                absoluteMeshPath = Path::getAbsolutePath( projectPath, cleanMeshPath );
            }

            auto sourceKey = absoluteMeshPath;
            if( !StringUtil::isNullOrEmpty( projectPath ) )
            {
                auto absoluteProjectPath = Path::getAbsolutePath( projectPath );
                auto relativeMeshPath = Path::getRelativePath( absoluteProjectPath, absoluteMeshPath );
                if( !StringUtil::isNullOrEmpty( relativeMeshPath ) &&
                    !Path::isPathAbsolute( relativeMeshPath ) && relativeMeshPath.rfind( "..", 0 ) != 0 )
                {
                    sourceKey = relativeMeshPath;
                }
            }

            return StringUtil::make_lower( StringUtil::cleanupPath( sourceKey ) );
        }

        String getAbsoluteCacheMeshPath( const String &meshFilePath )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return meshFilePath;
            }

            auto cachePath = applicationManager->getCachePath();
            if( StringUtil::isNullOrEmpty( cachePath ) )
            {
                return meshFilePath;
            }

            auto cacheFileName = Path::getFileName( meshFilePath );
            if( StringUtil::isNullOrEmpty( cacheFileName ) )
            {
                return meshFilePath;
            }

            if( Path::isPathAbsolute( cachePath ) )
            {
                return Path::lexically_normal( cachePath, cacheFileName );
            }

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            return Path::getAbsolutePath( projectPath,
                                          Path::lexically_normal( cachePath, cacheFileName ) );
        }

        String getAbsoluteCacheSkeletonPath( const String &meshFilePath, const UUID &fileUUID )
        {
            auto skeletonName = Path::getFileNameWithoutExtension( meshFilePath );
            if( StringUtil::isNullOrEmpty( skeletonName ) )
            {
                skeletonName = Path::getFileName( meshFilePath );
            }

            auto fileUUIDString = StringUtil::toString( fileUUID );
            if( !StringUtil::isNullOrEmpty( fileUUIDString ) )
            {
                skeletonName += "_" + fileUUIDString;
            }

            skeletonName += ".skeleton.xml";
            return getAbsoluteCacheMeshPath( skeletonName );
        }

        String getAbsoluteCacheSceneAnimationPath( const String &meshFilePath, const UUID &fileUUID )
        {
            auto animationName = Path::getFileNameWithoutExtension( meshFilePath );
            if( StringUtil::isNullOrEmpty( animationName ) )
            {
                animationName = Path::getFileName( meshFilePath );
            }

            auto fileUUIDString = StringUtil::toString( fileUUID );
            if( !StringUtil::isNullOrEmpty( fileUUIDString ) )
            {
                animationName += "_" + fileUUIDString;
            }

            animationName += ".sceneanim.xml";
            return getAbsoluteCacheMeshPath( animationName );
        }

        void deleteCacheFileIfExists( const String &filePath )
        {
            if( !StringUtil::isNullOrEmpty( filePath ) && Path::isExistingFile( filePath ) )
            {
                Path::deleteFile( filePath );
            }
        }

        bool loadMeshFromCacheFile( const String &meshFilePath, SmartPtr<IMesh> mesh )
        {
            if( !mesh )
            {
                return false;
            }

            auto cachedMesh = workphone::dynamic_pointer_cast<Mesh>( mesh );
            if( !cachedMesh )
            {
                return false;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return false;
            }

            auto fileSystem = applicationManager->getFileSystem();
            if( !fileSystem )
            {
                return false;
            }

            auto cacheMeshPath = getAbsoluteCacheMeshPath( meshFilePath );
            if( !fileSystem->isExistingFile( cacheMeshPath ) )
            {
                return false;
            }

            auto stream = fileSystem->open( cacheMeshPath, true, true, false, false, false );
            if( !stream )
            {
                stream = fileSystem->open( meshFilePath, true, true, false, true, true );
            }

            if( !stream )
            {
                return false;
            }

            MeshSerializer serializer;
            serializer.importMesh( stream, cachedMesh.get() );
            return true;
        }

#if WP_USE_ASSET_IMPORT
        const aiMesh *getAssimpMesh( const aiScene *scene, u32 meshIndex )
        {
            if( !scene || !scene->mMeshes || meshIndex >= scene->mNumMeshes )
            {
                return nullptr;
            }

            return scene->mMeshes[meshIndex];
        }

        const aiMaterial *getAssimpMaterial( const aiScene *scene, const aiMesh *mesh )
        {
            if( !scene || !mesh || !scene->mMaterials || mesh->mMaterialIndex >= scene->mNumMaterials )
            {
                return nullptr;
            }

            return scene->mMaterials[mesh->mMaterialIndex];
        }

        bool isEmbeddedTexturePath( const String &texturePath )
        {
            return !StringUtil::isNullOrEmpty( texturePath ) && texturePath[0] == '*';
        }

        void hashBytes( u64 &hash, const void *data, size_t size )
        {
            const auto bytes = static_cast<const u8 *>( data );
            for( size_t i = 0; i < size; ++i )
            {
                hash ^= bytes[i];
                hash *= 1099511628211ull;
            }
        }

        template <class T>
        void hashValue( u64 &hash, const T &value )
        {
            hashBytes( hash, &value, sizeof( value ) );
        }

        void hashAiVector3D( u64 &hash, const aiVector3D &value )
        {
            hashValue( hash, value.x );
            hashValue( hash, value.y );
            hashValue( hash, value.z );
        }

        void hashAiColor4D( u64 &hash, const aiColor4D &value )
        {
            hashValue( hash, value.r );
            hashValue( hash, value.g );
            hashValue( hash, value.b );
            hashValue( hash, value.a );
        }

        u32 getAssimpImportFlags( SmartPtr<scene::MeshResourceDirector> director )
        {
            if( !director )
            {
                return aiProcessPreset_TargetRealtime_MaxQuality;
            }

            u32 flags = 0;

            if( director->getGenNormals() )
            {
                flags |= aiProcess_GenNormals;
            }

            if( director->getGenSmoothNormals() )
            {
                flags |= aiProcess_GenSmoothNormals;
            }

            if( director->getGenTangents() )
            {
                flags |= aiProcess_CalcTangentSpace;
            }

            if( director->getGenUVCoords() || director->getLightmapUVs() )
            {
                flags |= aiProcess_GenUVCoords;
            }

            if( director->getTriangulate() )
            {
                flags |= aiProcess_Triangulate;
            }

            if( director->getJoinIdenticalVertices() )
            {
                flags |= aiProcess_JoinIdenticalVertices;
            }

            if( !director->getAnimation() || !director->getConstraints() || !director->getCameras() ||
                !director->getLights() )
            {
                flags |= aiProcess_RemoveComponent;
            }

            return flags;
        }

        void setAssimpImportOptions( Assimp::Importer &importer,
                                     SmartPtr<scene::MeshResourceDirector> director )
        {
            if( !director )
            {
                return;
            }

            importer.SetPropertyBool( AI_CONFIG_IMPORT_FBX_READ_ANIMATIONS, director->getAnimation() );
            importer.SetPropertyBool( AI_CONFIG_IMPORT_FBX_READ_WEIGHTS, director->getConstraints() );
            importer.SetPropertyBool( AI_CONFIG_IMPORT_FBX_READ_CAMERAS, director->getCameras() );
            importer.SetPropertyBool( AI_CONFIG_IMPORT_FBX_READ_LIGHTS, director->getLights() );

            u32 removeComponentFlags = 0;
            if( !director->getAnimation() )
            {
                removeComponentFlags |= aiComponent_ANIMATIONS;
            }

            if( !director->getConstraints() )
            {
                removeComponentFlags |= aiComponent_BONEWEIGHTS;
            }

            if( !director->getCameras() )
            {
                removeComponentFlags |= aiComponent_CAMERAS;
            }

            if( !director->getLights() )
            {
                removeComponentFlags |= aiComponent_LIGHTS;
            }

            if( removeComponentFlags != 0 )
            {
                importer.SetPropertyInteger( AI_CONFIG_PP_RVC_FLAGS, removeComponentFlags );
            }
        }
#endif
    }  // namespace

    AssimpLoader::AssimpLoader()
    {
        setObjectFlag( OBJECT_FLAG_TRIGGER_EVENTS, true );
        setObjectFlag( OBJECT_FLAG_GLOBAL_EVENTS, true );

        setUseSingleMesh( false );
        m_meshScale = Vector3<f32>::unit() * 1.0f;
    }

    AssimpLoader::~AssimpLoader()
    {
        unload( nullptr );
    }

    void AssimpLoader::load( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Loading );
        m_meshes.reserve( 1024 );
        setLoadingState( LoadingState::Loaded );
    }

    void AssimpLoader::unload( SmartPtr<ISharedObject> data )
    {
        setLoadingState( LoadingState::Unloading );

        m_rootActor = nullptr;
        m_meshDirector = nullptr;
        m_meshResource = nullptr;
        m_skeleton = nullptr;
        m_meshes.clear();
#if WP_USE_ASSET_IMPORT
        m_boneNodesByName.clear();
        m_bonesByName.clear();
        m_nodeDerivedTransformByName.clear();
        m_sceneActorsByName.clear();
        m_meshInstancesByKey.clear();
        m_loadedMeshSignatures.clear();
        m_aiSkeleton = nullptr;
#endif

        setLoadingState( LoadingState::Unloaded );
    }

    void AssimpLoader::createMaterials( Array<render::IMaterial> &materials, const aiScene *mScene,
                                        const aiNode *pNode, const String &mDir )
    {
        if( !mScene || !pNode )
        {
            return;
        }

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto numMeshes = pNode->mNumMeshes;
        if( numMeshes > 0 && !pNode->mMeshes )
        {
            return;
        }

        String meshFilePath;

        for( u32 idx = 0; idx < numMeshes; ++idx )
        {
            auto meshIndex = pNode->mMeshes[idx];

            auto pAIMesh = getAssimpMesh( mScene, meshIndex );
            if( !pAIMesh )
            {
                continue;
            }

            if( !m_quietMode )
            {
                WP_LOG_INFO( String( "SubMesh " ) + StringUtil::toString( idx ) +
                             String( " for mesh '" ) + String( pNode->mName.data ) + "'" );
            }

            // Create a material instance for the mesh.
            const auto pAIMaterial = getAssimpMaterial( mScene, pAIMesh );
            if( !pAIMaterial )
            {
                continue;
            }

            auto fileMeshCachePath = getAbsoluteCacheMeshPath( meshFilePath );

            if( !fileSystem->isExistingFile( fileMeshCachePath ) )
            {
                auto nodeNode = String( pNode->mName.data );
                //createSubMesh( nodeNode, idx, pNode, pAIMesh, pAIMaterial, mesh, mDir );
            }
        }
    }

    Array<render::IMaterial> AssimpLoader::createMaterials( const String &meshPath )
    {
        auto materials = Array<render::IMaterial>();

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto meshDirector = getMeshDirector();
        auto resourceDatabase = applicationManager->getResourceDatabase();
        if( !meshDirector && resourceDatabase )
        {
            meshDirector = resourceDatabase->loadDirectorFromResourcePath(
                meshPath, scene::MeshResourceDirector::typeInfo() );
        }

        auto fileExt = Path::getFileExtension( meshPath );
        fileExt = StringUtil::make_lower( fileExt );

        if( fileExt == ApplicationUtil::meshbinExt )
        {
        }
        else if( ApplicationUtil::isSupportedMesh( meshPath ) )
        {
#if WP_USE_FBXSDK
#elif WP_USE_ASSET_IMPORT
            Assimp::Importer importer;
            importer.SetIOHandler( new IOSystem() );
            setAssimpImportOptions( importer, meshDirector );

            u32 flags = getAssimpImportFlags( meshDirector );

            auto scene = importer.ReadFile( meshPath.c_str(), flags );
            if( scene && scene->mRootNode )
            {
                auto hash = StringUtil::getUUID( getMeshCacheSourceKey( meshPath ) );
                m_fileUUID = hash;

                computeNodesDerivedTransform( scene, scene->mRootNode,
                                              scene->mRootNode->mTransformation );

                //auto name = Path::getFileNameWithoutExtension( m_meshPath );
                //actor->setName( name );

                //auto path = Path::getFilePath( m_meshPath );
                createMaterials( materials, scene, scene->mRootNode, meshPath );
            }
#endif
        }

        return materials;
    }

    SmartPtr<scene::IGameActor> AssimpLoader::loadActor( SmartPtr<IMeshResource> resource )
    {
        ScopedLock lock( this );

        m_meshes.clear();
        m_meshes.reserve( 1024 );
        setMeshScale( Vector3<f32>::unit() );
        setSkeleton( nullptr );
#if WP_USE_ASSET_IMPORT
        m_boneNodesByName.clear();
        m_bonesByName.clear();
        m_nodeDerivedTransformByName.clear();
        m_sceneActorsByName.clear();
        m_meshInstancesByKey.clear();
        m_loadedMeshSignatures.clear();
        m_aiSkeleton = nullptr;
#endif

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sceneManager = applicationManager->getGameManager();
        WP_ASSERT( sceneManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto resourceDatabase = applicationManager->getResourceDatabase();

        if( resource )
        {
            setMeshResource( resource );

            auto meshScale = Vector3<f32>::unit() * resource->getScale();
            setMeshScale( meshScale );

            m_meshPath = resource->getFilePath();
            m_meshFileName = Path::getFileName( m_meshPath );
            m_meshFileExt = Path::getFileExtension( m_meshPath );
            m_meshFileExt = StringUtil::make_lower( m_meshFileExt );

            auto director = resourceDatabase->loadDirectorFromResourcePath(
                m_meshPath, scene::MeshResourceDirector::typeInfo() );
            setMeshDirector( director );
        }

        WP_ASSERT( !StringUtil::isNullOrEmpty( m_meshPath ) );

        auto fileExt = Path::getFileExtension( m_meshPath );
        fileExt = StringUtil::make_lower( fileExt );

        if( fileExt == ApplicationUtil::meshbinExt )
        {
            MeshSerializer serializer;

            auto stream = fileSystem->open( m_meshPath, true, true, false );
            if( stream )
            {
                auto mesh = workphone::make_ptr<Mesh>();
                serializer.importMesh( stream, mesh.get() );
                if( resource )
                {
                    resource->setMesh( mesh );
                }

                auto actor = sceneManager->createActor();
                ensureMeshActorComponents( actor, m_meshPath, resource, getMeshDirector() );

                setMeshResource( nullptr );
                setMeshDirector( nullptr );
                return actor;
            }
        }
        else if( ApplicationUtil::isSupportedMesh( m_meshPath ) )
        {
            auto actor = sceneManager->createActor();
            setRootActor( actor );

            if( fileExt == ".ngp" )
            {
                auto mesh = FoliageLoader::loadMesh( m_meshPath );
                if( resource )
                {
                    resource->setMesh( mesh );
                }

                actor->setName( Path::getFileNameWithoutExtension( m_meshPath ) );
                ensureMeshActorComponents( actor, m_meshPath, resource, getMeshDirector() );

                setMeshResource( nullptr );
                setMeshDirector( nullptr );
                return actor;
            }

#if WP_USE_FBXSDK
            auto outputPath = ".FBCache/" + String( "f40.mesh" );

            FBXConverter e;
            // e.exportScene(meshName.c_str(), outputPath.c_str(), "fbx.log", "", "", false, 0);
            auto sceneRoot = e.loadScene( meshName );
            if( sceneRoot )
            {
                auto name = sceneRoot->getName();
                actor->setName( name );

                auto localTransform = sceneRoot->getLocalTransform();
                auto worldTransform = sceneRoot->getWorldTransform();

                auto pLocalTransform = actor->getLocalTransform();
                *pLocalTransform = localTransform;

                auto pWorldTransform = actor->getTransform();
                *pWorldTransform = worldTransform;

                auto children = sceneRoot->getChildren();
                for( auto child : children )
                {
                    createActor( child, actor );
                }
            }

#elif WP_USE_ASSET_IMPORT
            Assimp::Importer importer;
            importer.SetIOHandler( new IOSystem() );
            setAssimpImportOptions( importer, getMeshDirector() );

            u32 flags = getAssimpImportFlags( getMeshDirector() );

            auto scene = importer.ReadFile( m_meshPath.c_str(), flags );
            if( scene && scene->mRootNode )
            {
                auto hash = StringUtil::getUUID( getMeshCacheSourceKey( m_meshPath ) );
                m_fileUUID = hash;

                computeNodesDerivedTransform( scene, scene->mRootNode,
                                              scene->mRootNode->mTransformation );

                setSkeleton( createSkeleton( scene ) );
                writeSkeletonCacheFile( m_meshPath, getSkeleton() );

                auto name = Path::getFileNameWithoutExtension( m_meshPath );
                actor->setName( name );

                auto path = Path::getFilePath( m_meshPath );
                loadDataFromActor( actor, scene, scene->mRootNode, path );
                loadAnimations( scene, actor );

                actor->updateTransform();
            }
#endif

            setMeshResource( nullptr );
            setMeshDirector( nullptr );

            return actor;
        }

        setMeshResource( nullptr );
        setMeshDirector( nullptr );
        return nullptr;
    }

    SmartPtr<scene::IGameActor> AssimpLoader::loadActor( const String &meshName )
    {
        try
        {
            WP_ASSERT( !StringUtil::isNullOrEmpty( meshName ) );

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto gameManager = applicationManager->getGameManager();
            WP_ASSERT( gameManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto meshManager = applicationManager->getMeshManager();
            auto meshLoader = applicationManager->getMeshLoader();
            if( !meshLoader || meshLoader.get() == nullptr )
            {
                WP_LOG_ERROR( "No mesh loader" );
                return nullptr;
            }

            WP_ASSERT( meshManager );

            auto resourceDatabase = applicationManager->getResourceDatabase();

            ScopedLock lock( this );

            m_meshes.clear();
            m_meshes.reserve( 1024 );
            setMeshScale( Vector3<f32>::unit() );
            setSkeleton( nullptr );
#if WP_USE_ASSET_IMPORT
            m_boneNodesByName.clear();
            m_bonesByName.clear();
            m_nodeDerivedTransformByName.clear();
            m_sceneActorsByName.clear();
            m_meshInstancesByKey.clear();
            m_loadedMeshSignatures.clear();
            m_aiSkeleton = nullptr;
#endif

            auto resource = meshManager->loadFromFile( meshName );
            if( !resource )
            {
                return nullptr;
            }

            auto meshResource = workphone::static_pointer_cast<IMeshResource>( resource );
            if( meshResource )
            {
                setMeshResource( meshResource );

                auto meshScale = Vector3<f32>::unit() * meshResource->getScale();
                setMeshScale( meshScale );

                m_meshPath = resource->getFilePath();
                m_meshFileName = Path::getFileName( m_meshPath );
                m_meshFileExt = Path::getFileExtension( m_meshPath );
                m_meshFileExt = StringUtil::make_lower( m_meshFileExt );

                auto director = resourceDatabase->loadDirectorFromResourcePath(
                    m_meshPath, scene::MeshResourceDirector::typeInfo() );
                setMeshDirector( director );
            }

            m_meshPath = meshName;

            auto fileExt = Path::getFileExtension( meshName );
            fileExt = StringUtil::make_lower( fileExt );

            if( fileExt == ApplicationUtil::meshbinExt )
            {
                MeshSerializer serializer;

                auto stream = fileSystem->open( m_meshPath, true, true, false );
                if( stream )
                {
                    auto mesh = workphone::make_ptr<Mesh>();
                    serializer.importMesh( stream, mesh.get() );
                    if( meshResource )
                    {
                        meshResource->setMesh( mesh );
                    }

                    auto actor = gameManager->createActor();
                    ensureMeshActorComponents( actor, m_meshPath, meshResource, getMeshDirector() );

                    setMeshResource( nullptr );
                    setMeshDirector( nullptr );

                    return actor;
                }
            }
            else if( ApplicationUtil::isSupportedMesh( meshName ) )
            {
                auto actor = gameManager->createActor();
                setRootActor( actor );

                if( fileExt == ".ngp" )
                {
                    auto mesh = FoliageLoader::loadMesh( m_meshPath );
                    if( meshResource )
                    {
                        meshResource->setMesh( mesh );
                    }

                    actor->setName( Path::getFileNameWithoutExtension( m_meshPath ) );
                    ensureMeshActorComponents( actor, m_meshPath, meshResource, getMeshDirector() );

                    setMeshResource( nullptr );
                    setMeshDirector( nullptr );
                    return actor;
                }

#if WP_USE_FBXSDK
                auto outputPath = ".FBCache/" + String( "f40.mesh" );

                FBXConverter e;
                // e.exportScene(meshName.c_str(), outputPath.c_str(), "fbx.log", "", "", false, 0);
                auto sceneRoot = e.loadScene( meshName );
                if( sceneRoot )
                {
                    auto name = sceneRoot->getName();
                    actor->setName( name );

                    auto localTransform = sceneRoot->getLocalTransform();
                    auto worldTransform = sceneRoot->getWorldTransform();

                    auto pLocalTransform = actor->getLocalTransform();
                    *pLocalTransform = localTransform;

                    auto pWorldTransform = actor->getTransform();
                    *pWorldTransform = worldTransform;

                    auto children = sceneRoot->getChildren();
                    for( auto child : children )
                    {
                        createActor( child, actor );
                    }
                }

#elif WP_USE_ASSET_IMPORT
                auto meshFilePath = resource->getFilePath();

                Assimp::Importer importer;
                importer.SetIOHandler( new IOSystem() );
                setAssimpImportOptions( importer, getMeshDirector() );

                u32 flags = getAssimpImportFlags( getMeshDirector() );

                auto scene = importer.ReadFile( meshFilePath.c_str(), flags );
                if( scene && scene->mRootNode )
                {
                    auto numMaterials = scene->mNumMaterials;
                    for( size_t i = 0; scene->mMaterials && i < numMaterials; i++ )
                    {
                        auto pAIMaterial = scene->mMaterials[i];
                        if( pAIMaterial )
                        {
                            createMaterial( nullptr, static_cast<s32>( i ), pAIMaterial, "" );
                        }
                    }

                    auto hash = StringUtil::getUUID( getMeshCacheSourceKey( meshFilePath ) );
                    m_fileUUID = hash;

                    computeNodesDerivedTransform( scene, scene->mRootNode,
                                                  scene->mRootNode->mTransformation );

                    setSkeleton( createSkeleton( scene ) );
                    writeSkeletonCacheFile( meshName, getSkeleton() );

                    auto name = Path::getFileNameWithoutExtension( meshName );
                    actor->setName( name );

                    auto path = Path::getFilePath( meshName );
                    loadDataFromActor( actor, scene, scene->mRootNode, path );
                    loadAnimations( scene, actor );
                }
#endif

                setMeshResource( nullptr );
                setMeshDirector( nullptr );
                return actor;
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        setMeshResource( nullptr );
        setMeshDirector( nullptr );
        return nullptr;
    }

    SmartPtr<IMesh> AssimpLoader::loadMesh( SmartPtr<IMeshResource> resource )
    {
        ScopedLock lock( this );

        m_meshes.clear();
        m_meshes.reserve( 1024 );
        setMeshScale( Vector3<f32>::unit() );
        setSkeleton( nullptr );

#if WP_USE_ASSET_IMPORT
        m_boneNodesByName.clear();
        m_bonesByName.clear();
        m_nodeDerivedTransformByName.clear();
        m_sceneActorsByName.clear();
        m_meshInstancesByKey.clear();
        m_loadedMeshSignatures.clear();
        m_aiSkeleton = nullptr;
#endif

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();
        auto fileSystem = applicationManager->getFileSystem();
        auto meshManager = applicationManager->getMeshManager();

        auto resourceDatabase = applicationManager->getResourceDatabase();

        if( resource )
        {
            setMeshResource( resource );

            auto meshScale = Vector3<f32>::unit() * resource->getScale();
            setMeshScale( meshScale );

            m_meshPath = resource->getFilePath();
            m_meshFileName = Path::getFileName( m_meshPath );
            m_meshFileExt = Path::getFileExtension( m_meshPath );
            m_meshFileExt = StringUtil::make_lower( m_meshFileExt );

            auto director = resourceDatabase->loadDirectorFromResourcePath(
                m_meshPath, scene::MeshResourceDirector::typeInfo() );
            setMeshDirector( director );
        }

        WP_ASSERT( !StringUtil::isNullOrEmpty( m_meshPath ) );

        auto fileExt = Path::getFileExtension( m_meshPath );
        fileExt = StringUtil::make_lower( fileExt );

        static const auto meshExt = String( ".fbmeshbin" );

        if( fileExt == meshExt )
        {
            MeshSerializer serializer;

            auto stream = fileSystem->open( m_meshPath, true, true, false, false, false );
            if( !stream )
            {
                stream = fileSystem->open( m_meshPath, true, true, false, true, true );
            }

            if( stream )
            {
                auto mesh = workphone::safe_pointer_cast<Mesh>( resource->getMesh() );
                if( !mesh )
                {
                    auto newMesh = factoryManager->make_ptr<Mesh>();
                    serializer.importMesh( stream, newMesh.get() );

                    setMeshResource( nullptr );
                    setMeshDirector( nullptr );
                    return newMesh;
                }

                setMeshResource( nullptr );
                setMeshDirector( nullptr );
                return workphone::static_pointer_cast<Mesh>( mesh );
            }
        }
        else if( ApplicationUtil::isSupportedMesh( m_meshPath ) )
        {
            auto mesh = factoryManager->make_ptr<Mesh>();

#if WP_USE_FBXSDK
            auto outputPath = ".FBCache/" + String( "f40.mesh" );

            FBXConverter e;
            // e.exportScene(meshName.c_str(), outputPath.c_str(), "fbx.log", "", "", false, 0);
            auto sceneRoot = e.loadScene( meshName );
            if( sceneRoot )
            {
                auto name = sceneRoot->getName();
                actor->setName( name );

                auto localTransform = sceneRoot->getLocalTransform();
                auto worldTransform = sceneRoot->getWorldTransform();

                auto pLocalTransform = actor->getLocalTransform();
                *pLocalTransform = localTransform;

                auto pWorldTransform = actor->getTransform();
                *pWorldTransform = worldTransform;

                auto children = sceneRoot->getChildren();
                for( auto child : children )
                {
                    createActor( child, actor );
                }
            }

#elif WP_USE_ASSET_IMPORT
            auto projectFolder = applicationManager->getProjectPath();
            auto absolutePath = Path::getAbsolutePath( projectFolder, m_meshPath );

            Assimp::Importer importer;
            setAssimpImportOptions( importer, getMeshDirector() );
            u32 flags = getAssimpImportFlags( getMeshDirector() );

            const auto scene = importer.ReadFile( absolutePath.c_str(), flags );
            if( scene && scene->mRootNode )
            {
                auto hash = StringUtil::getUUID( getMeshCacheSourceKey( m_meshPath ) );
                m_fileUUID = hash;

                computeNodesDerivedTransform( scene, scene->mRootNode,
                                              scene->mRootNode->mTransformation );

                setSkeleton( createSkeleton( scene ) );
                mesh->setSkeleton( getSkeleton() );
                mesh->setHasSkeleton( getSkeleton() != nullptr );
                writeSkeletonCacheFile( m_meshPath, getSkeleton() );

                auto name = Path::getFileNameWithoutExtension( m_meshPath );
                mesh->setName( name );

                auto numMaterials = scene->mNumMaterials;
                for( size_t i = 0; scene->mMaterials && i < numMaterials; i++ )
                {
                    auto pAIMaterial = scene->mMaterials[i];
                    if( pAIMaterial )
                    {
                        createMaterial( nullptr, static_cast<s32>( i ), pAIMaterial, "" );
                    }
                }

                auto path = Path::getFilePath( m_meshPath );
                loadToMesh( mesh, scene, scene->mRootNode, path );
            }

            //applicationManager->triggerEvent( EventType::Renderer, IEvent::meshesImported, m_meshes,
            //                                  this, resource, nullptr, false );

            setMeshResource( nullptr );
            setMeshDirector( nullptr );
            return mesh;
#endif
        }

        setMeshResource( nullptr );
        setMeshDirector( nullptr );
        return nullptr;
    }

    SmartPtr<IMesh> AssimpLoader::loadMesh( const String &meshName )
    {
        ScopedLock lock( this );

        m_meshes.clear();
        m_meshes.reserve( 1024 );
        setMeshScale( Vector3<f32>::unit() );
        setSkeleton( nullptr );
#if WP_USE_ASSET_IMPORT
        m_boneNodesByName.clear();
        m_bonesByName.clear();
        m_nodeDerivedTransformByName.clear();
        m_sceneActorsByName.clear();
        m_meshInstancesByKey.clear();
        m_loadedMeshSignatures.clear();
        m_aiSkeleton = nullptr;
#endif

        WP_ASSERT( !StringUtil::isNullOrEmpty( meshName ) );

        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto fileSystem = applicationManager->getFileSystem();
        WP_ASSERT( fileSystem );

        auto resourceDatabase = applicationManager->getResourceDatabase();

        // auto graphicsSystem = applicationManager->getGraphicsSystem();
        // WP_ASSERT(graphicsSystem);

        auto meshManager = applicationManager->getMeshManager();
        WP_ASSERT( meshManager );

        m_meshFileName = Path::getFileName( meshName );
        auto meshFileExt = Path::getFileExtension( meshName );
        meshFileExt = StringUtil::make_lower( meshFileExt );

        auto resource = meshManager->loadFromFile( meshName );
        auto meshResource = workphone::static_pointer_cast<IMeshResource>( resource );
        if( meshResource )
        {
            setMeshResource( meshResource );

            auto meshScale = Vector3<f32>::unit() * meshResource->getScale();
            setMeshScale( meshScale );

            m_meshPath = resource->getFilePath();

            auto director = resourceDatabase->loadDirectorFromResourcePath(
                m_meshPath, scene::MeshResourceDirector::typeInfo() );
            setMeshDirector( director );
        }

        m_meshPath = meshName;

        auto fileExt = Path::getFileExtension( meshName );
        fileExt = StringUtil::make_lower( fileExt );

        if( fileExt == ".fbmeshbin" )
        {
            MeshSerializer serializer;

            auto stream = fileSystem->open( m_meshPath );
            if( stream )
            {
                auto mesh = workphone::make_ptr<Mesh>();
                serializer.importMesh( stream, mesh.get() );

                setMeshResource( nullptr );
                setMeshDirector( nullptr );
                return mesh;
            }
        }
        else if( ApplicationUtil::isSupportedMesh( m_meshPath ) )
        {
            auto mesh = workphone::make_ptr<Mesh>();

#if WP_USE_FBXSDK
            auto outputPath = ".FBCache/" + String( "f40.mesh" );

            FBXConverter e;
            // e.exportScene(meshName.c_str(), outputPath.c_str(), "fbx.log", "", "", false, 0);
            auto sceneRoot = e.loadScene( meshName );
            if( sceneRoot )
            {
                auto name = sceneRoot->getName();
                actor->setName( name );

                auto localTransform = sceneRoot->getLocalTransform();
                auto worldTransform = sceneRoot->getWorldTransform();

                auto pLocalTransform = actor->getLocalTransform();
                *pLocalTransform = localTransform;

                auto pWorldTransform = actor->getTransform();
                *pWorldTransform = worldTransform;

                auto children = sceneRoot->getChildren();
                for( auto child : children )
                {
                    createActor( child, actor );
                }
            }

#elif WP_USE_ASSET_IMPORT
            auto projectFolder = applicationManager->getProjectPath();
            auto absolutePath = Path::getAbsolutePath( projectFolder, meshName );

            Assimp::Importer importer;
            setAssimpImportOptions( importer, getMeshDirector() );
            u32 flags = getAssimpImportFlags( getMeshDirector() );
            const auto scene = importer.ReadFile( absolutePath.c_str(), flags );
            if( scene && scene->mRootNode )
            {
                auto hash = StringUtil::getUUID( getMeshCacheSourceKey( m_meshPath ) );
                m_fileUUID = hash;

                auto numMaterials = scene->mNumMaterials;
                for( size_t i = 0; scene->mMaterials && i < numMaterials; i++ )
                {
                    auto pAIMaterial = scene->mMaterials[i];
                    if( pAIMaterial )
                    {
                        createMaterial( nullptr, static_cast<s32>( i ), pAIMaterial, "" );
                    }
                }

                computeNodesDerivedTransform( scene, scene->mRootNode,
                                              scene->mRootNode->mTransformation );

                setSkeleton( createSkeleton( scene ) );
                mesh->setSkeleton( getSkeleton() );
                mesh->setHasSkeleton( getSkeleton() != nullptr );
                writeSkeletonCacheFile( meshName, getSkeleton() );

                auto name = Path::getFileNameWithoutExtension( meshName );
                mesh->setName( name );

                auto path = Path::getFilePath( meshName );
                loadToMesh( mesh, scene, scene->mRootNode, path );
            }

            setMeshResource( nullptr );
            setMeshDirector( nullptr );
            return mesh;
#endif
        }

        setMeshResource( nullptr );
        setMeshDirector( nullptr );
        return nullptr;
    }

#if WP_USE_FBXSDK
#elif WP_USE_ASSET_IMPORT

    bool AssimpLoader::getUseMeshInstancing() const
    {
        auto meshResource = getMeshResource();
        return meshResource && meshResource->getUseMeshInstancing();
    }

    String AssimpLoader::getAssimpMeshSignature( const aiMesh *mesh ) const
    {
        if( !mesh )
        {
            return {};
        }

        u64 hash = 14695981039346656037ull;

        hashValue( hash, mesh->mPrimitiveTypes );
        hashValue( hash, mesh->mMaterialIndex );
        hashValue( hash, mesh->mNumVertices );
        hashValue( hash, mesh->mNumFaces );
        hashValue( hash, mesh->mNumBones );
        hashValue( hash, mesh->GetNumUVChannels() );
        hashValue( hash, mesh->GetNumColorChannels() );
        hashValue( hash, mesh->HasNormals() );
        hashValue( hash, mesh->HasTangentsAndBitangents() );

        for( u32 i = 0; mesh->mVertices && i < mesh->mNumVertices; ++i )
        {
            hashAiVector3D( hash, mesh->mVertices[i] );
        }

        for( u32 i = 0; mesh->mNormals && i < mesh->mNumVertices; ++i )
        {
            hashAiVector3D( hash, mesh->mNormals[i] );
        }

        for( u32 i = 0; mesh->mTangents && i < mesh->mNumVertices; ++i )
        {
            hashAiVector3D( hash, mesh->mTangents[i] );
        }

        for( u32 i = 0; mesh->mBitangents && i < mesh->mNumVertices; ++i )
        {
            hashAiVector3D( hash, mesh->mBitangents[i] );
        }

        for( u32 channel = 0; channel < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++channel )
        {
            auto textureCoords = mesh->mTextureCoords[channel];
            hashValue( hash, textureCoords != nullptr );
            hashValue( hash, mesh->mNumUVComponents[channel] );
            if( textureCoords )
            {
                for( u32 i = 0; i < mesh->mNumVertices; ++i )
                {
                    hashAiVector3D( hash, textureCoords[i] );
                }
            }
        }

        for( u32 channel = 0; channel < AI_MAX_NUMBER_OF_COLOR_SETS; ++channel )
        {
            auto colors = mesh->mColors[channel];
            hashValue( hash, colors != nullptr );
            if( colors )
            {
                for( u32 i = 0; i < mesh->mNumVertices; ++i )
                {
                    hashAiColor4D( hash, colors[i] );
                }
            }
        }

        for( u32 i = 0; mesh->mFaces && i < mesh->mNumFaces; ++i )
        {
            const auto &face = mesh->mFaces[i];
            hashValue( hash, face.mNumIndices );
            for( u32 index = 0; face.mIndices && index < face.mNumIndices; ++index )
            {
                hashValue( hash, face.mIndices[index] );
            }
        }

        for( u32 i = 0; mesh->mBones && i < mesh->mNumBones; ++i )
        {
            auto bone = mesh->mBones[i];
            hashValue( hash, bone != nullptr );
            if( !bone )
            {
                continue;
            }

            const auto boneName = bone->mName.C_Str();
            if( boneName )
            {
                hashBytes( hash, boneName, strlen( boneName ) );
            }

            hashValue( hash, bone->mNumWeights );
            for( u32 weightIdx = 0; bone->mWeights && weightIdx < bone->mNumWeights; ++weightIdx )
            {
                hashValue( hash, bone->mWeights[weightIdx].mVertexId );
                hashValue( hash, bone->mWeights[weightIdx].mWeight );
            }
        }

        return StringUtil::toString( hash );
    }

    String AssimpLoader::getMeshInstanceKey( const aiScene *scene, const aiNode *node ) const
    {
        if( !scene || !node || !node->mMeshes || node->mNumMeshes == 0 )
        {
            return {};
        }

        auto key = StringUtil::toString( node->mNumMeshes );
        for( u32 idx = 0; idx < node->mNumMeshes; ++idx )
        {
            auto mesh = getAssimpMesh( scene, node->mMeshes[idx] );
            auto signature = getAssimpMeshSignature( mesh );
            if( StringUtil::isNullOrEmpty( signature ) )
            {
                return {};
            }

            key += "|" + signature;
        }

        return key;
    }

    void AssimpLoader::loadDataFromActor( SmartPtr<scene::IGameActor> parent, const aiScene *mScene,
                                          const aiNode *pNode, const String &mDir )
    {
        try
        {
            if( !parent || !mScene || !pNode )
            {
                return;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );
            if( !applicationManager )
            {
                return;
            }

            auto gameManager = applicationManager->getGameManagerPtr();
            WP_ASSERT( gameManager );
            if( !gameManager )
            {
                return;
            }

            auto fileSystem = applicationManager->getFileSystem();
            auto factoryManager = applicationManager->getFactoryManagerPtr();
            auto resourceDatabase = applicationManager->getResourceDatabase();
            WP_ASSERT( fileSystem );
            WP_ASSERT( factoryManager );
            WP_ASSERT( resourceDatabase );
            if( !fileSystem || !factoryManager || !resourceDatabase )
            {
                return;
            }

            auto actor = gameManager->createActorPtr();
            if( !actor )
            {
                return;
            }

            parent->addChild( actor );

            const auto &name = pNode->mName;
            actor->setName( name.C_Str() );
            m_sceneActorsByName[String( name.C_Str() )] = actor;

            auto localTransform = convertAssimpMatrix( pNode->mTransformation );

            auto position = Vector3<f32>::zero();
            auto scale = Vector3<f32>::unit();
            auto rotation = Quaternion<f32>::identity();

            localTransform.decomposition( position, scale, rotation );

            auto actorTransform = actor->getTransform();
            WP_ASSERT( actorTransform );

            parent->updateTransform();

            auto parentTransform = parent->getTransform();
            auto parentWorldTransform = parentTransform->getWorldTransform();
            auto parentLocalTransform = parentTransform->getLocalTransform();

            WP_ASSERT( parentWorldTransform.isSane() );
            WP_ASSERT( parentLocalTransform.isSane() );

            auto actorPosition = position * m_meshScale;
            auto actorOrientation = rotation;
            auto actorScale = scale;

            if( !actorPosition.isFinite() )
            {
                actorPosition = Vector3<real_Num>::zero();
            }

            if( !actorOrientation.isSane() )
            {
                actorOrientation = Quaternion<real_Num>::identity();
            }

            // check for valid scale
            if( MathF::equals( actorScale.x, 0.0f ) )
            {
                actorScale.x = 0.0001f;
            }

            if( MathF::equals( actorScale.y, 0.0f ) )
            {
                actorScale.y = 0.0001f;
            }

            if( MathF::equals( actorScale.z, 0.0f ) )
            {
                actorScale.z = 0.0001f;
            }

            WP_ASSERT( actorPosition.isValid() );
            WP_ASSERT( actorOrientation.isValid() );
            WP_ASSERT( actorScale.isValid() );

            actorTransform->setLocalPosition( actorPosition );
            actorTransform->setLocalOrientation( actorOrientation );
            actorTransform->setLocalScale( actorScale );
            actorTransform->setDirty( true );
            actorTransform->updateWorldFromLocal();

            auto numMeshes = pNode->mNumMeshes;

            static const auto meshExt = String( ".fbmeshbin" );
            const auto nodeName = String( pNode->mName.data );

            auto meshFilePath = "Cache/" + m_meshFileName + "." + nodeName + "_" +
                                StringUtil::toString( m_fileUUID ) + meshExt;
            meshFilePath = StringUtil::cleanupPath( meshFilePath );

            if( numMeshes > 0 && pNode->mMeshes )
            {
                auto meshInstanceKey =
                    getUseMeshInstancing() ? getMeshInstanceKey( mScene, pNode ) : String();
                auto usingMeshInstance = false;
                auto meshResource = SmartPtr<IMeshResource>();

                SmartPtr<IMesh> mesh;

                auto createNodeMesh = [&]( SmartPtr<IMesh> targetMesh ) {
                    if( !targetMesh )
                    {
                        return false;
                    }

                    auto createdSubMesh = false;
                    for( u32 idx = 0; idx < numMeshes; ++idx )
                    {
                        try
                        {
                            auto meshIndex = pNode->mMeshes[idx];

                            auto pAIMesh = getAssimpMesh( mScene, meshIndex );
                            if( !pAIMesh )
                            {
                                continue;
                            }

                            if( !m_quietMode )
                            {
                                WP_LOG_INFO( String( "SubMesh " ) + StringUtil::toString( idx ) +
                                             String( " for mesh '" ) + String( pNode->mName.data ) + "'" );
                            }

                            const auto pAIMaterial = getAssimpMaterial( mScene, pAIMesh );
                            if( !pAIMaterial )
                            {
                                continue;
                            }

                            auto nodeNode = String( pNode->mName.data );
                            createdSubMesh =
                                createSubMesh( nodeNode, idx, pNode, pAIMesh, pAIMaterial, targetMesh,
                                               mDir ) ||
                                createdSubMesh;
                        }
                        catch( std::exception &e )
                        {
                            WP_LOG_EXCEPTION( e );
                        }
                    }

                    return createdSubMesh;
                };

                auto instanceIt = m_meshInstancesByKey.end();
                if( !StringUtil::isNullOrEmpty( meshInstanceKey ) )
                {
                    instanceIt = m_meshInstancesByKey.find( meshInstanceKey );
                    if( instanceIt != m_meshInstancesByKey.end() && instanceIt->second.meshResource )
                    {
                        auto candidateMesh = factoryManager->make_ptr<Mesh>();
                        if( candidateMesh && createNodeMesh( candidateMesh ) && instanceIt->second.mesh &&
                            candidateMesh->compare( instanceIt->second.mesh ) )
                        {
                            meshFilePath = instanceIt->second.meshFilePath;
                            meshResource = instanceIt->second.meshResource;
                            mesh = instanceIt->second.mesh;
                            usingMeshInstance = true;
                        }
                        else
                        {
                            mesh = candidateMesh;
                        }
                    }
                }

                if( !meshResource )
                {
                    auto meshResourceResult =
                        resourceDatabase->createOrRetrieveByType<IMeshResource>( meshFilePath );
                    meshResource = meshResourceResult.first;
                }

                if( meshResource && !usingMeshInstance )
                {
                    if( !mesh )
                    {
                        mesh = factoryManager->make_ptr<Mesh>();
                    }

                    Parameter meshParam;
                    meshParam.object = mesh;
                    m_meshes.push_back( meshParam );

                    auto importMesh = getImportMesh();
                    if( importMesh )
                    {
                        meshResource->setMesh( mesh );

                        auto meshAlreadyBuilt = mesh->getNumSubMeshes() > 0;
                        auto loadedCachedMesh = !meshAlreadyBuilt && !getOverwrite() &&
                                                loadMeshFromCacheFile( meshFilePath, mesh );
                        if( !loadedCachedMesh && !meshAlreadyBuilt )
                        {
                            createNodeMesh( mesh );
                        }
                    }

                    if( mesh )
                    {
                        mesh->setName( meshFilePath );
                        mesh->setSkeleton( getSkeleton() );
                    }

                    if( !StringUtil::isNullOrEmpty( meshInstanceKey ) )
                    {
                        MeshInstanceRecord record;
                        record.meshFilePath = meshFilePath;
                        record.meshResource = meshResource;
                        record.mesh = mesh;
                        m_meshInstancesByKey[meshInstanceKey] = record;
                    }
                }

                for( u32 idx = 0; idx < pNode->mNumMeshes; ++idx )
                {
                    const auto &meshIndex = pNode->mMeshes[idx];
                    const auto pAIMesh = getAssimpMesh( mScene, meshIndex );
                    if( !pAIMesh )
                    {
                        continue;
                    }

                    const auto pAIMaterial = getAssimpMaterial( mScene, pAIMesh );
                    if( !pAIMaterial )
                    {
                        continue;
                    }

                    if( m_materialImportOptions.assignMaterialComponents )
                    {
                        assignMaterialComponent( actor, idx, pAIMaterial, mDir );
                    }
                }

                if( ensureMeshActorComponents( actor, meshFilePath, meshResource,
                                               getMeshDirector() ) )
                {
                    if( auto meshComponent = actor->getComponent<scene::Mesh>() )
                    {
                        meshComponent->setSkeleton( getSkeleton() );
                    }
                }
                actor->updateTransform();
            }

            // Traverse all child nodes of the current node instance
            for( u32 childIdx = 0; pNode->mChildren && childIdx < pNode->mNumChildren; childIdx++ )
            {
                const auto pChildNode = pNode->mChildren[childIdx];
                loadDataFromActor( actor, mScene, pChildNode, mDir );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void AssimpLoader::loadToMesh( SmartPtr<IMesh> mesh, const aiScene *mScene, const aiNode *pNode,
                                   const String &mDir )
    {
        try
        {
            if( !mesh || !mScene || !pNode )
            {
                return;
            }

            mesh->setSkeleton( getSkeleton() );

            auto applicationManager = core::IApplicationManager::instance();
            auto fileSystem = applicationManager->getFileSystem();

            auto name = pNode->mName;

            auto localTransform = pNode->mTransformation;
            localTransform = localTransform.Inverse();

            aiQuaternion rot;
            aiVector3D pos;
            aiVector3D scale;

            localTransform.Decompose( scale, rot, pos );

            aiMatrix4x4 aiM;

            auto nodeTransformData = m_nodeDerivedTransformByName.find( pNode->mName.data );
            if( nodeTransformData != m_nodeDerivedTransformByName.end() )
            {
                aiM = nodeTransformData->second;
            }

            aiM.Decompose( scale, rot, pos );

            /*
            SmartPtr<TransformComponent> actorTransform = actor->getTransform();
            auto parentWorldTransform = actorTransform->getWorldTransform();
            auto parentLocalTransform = actorTransform->getLocalTransform();

            WP_ASSERT(parentWorldTransform.isSane());
            WP_ASSERT(parentLocalTransform.isSane());

            parent->updateTransform();

            auto actorPosition = Vector3<real_Num>(pos.x, pos.y, pos.z) * m_meshScale;
            auto actorOrientation = Quaternion<real_Num>(rot.w, rot.x, rot.y, rot.z);
            auto actorScale = Vector3<real_Num>(scale.x, scale.y, scale.z);

            actorPosition *= actorScale;

            WP_ASSERT(actorPosition.isValid());
            WP_ASSERT(actorOrientation.isValid());
            WP_ASSERT(actorScale.isValid());

            if (!localTransform.IsIdentity())
            {
                auto actorLocalTransform = actorTransform->getLocalTransform();
                auto actorWorldTransform = actorTransform->getWorldTransform();

                Transform3<real_Num> t(actorPosition, actorOrientation, actorScale);
                actorLocalTransform.fromWorldToLocal(parentWorldTransform, t);

                WP_ASSERT(actorLocalTransform.isSane());

                actorWorldTransform.setPosition(actorPosition);
                actorWorldTransform.setOrientation(actorOrientation);
                actorWorldTransform.setScale(actorScale);

                auto actorLocalPosition = actorLocalTransform.getPosition();
                auto actorLocalOrientation = actorLocalTransform.getOrientation();
                auto actorLocalScale = actorLocalTransform.getScale();

                actorTransform->setLocalPosition(actorLocalPosition);
                actorTransform->setLocalOrientation(actorLocalOrientation);
                actorTransform->setLocalScale(actorLocalScale);

                actorTransform->setPosition(actorPosition);
                actorTransform->setOrientation(actorOrientation);
                actorTransform->setScale(actorScale);

                WP_ASSERT(actorWorldTransform.isSane());

                //actorTransform->updateWorldFromLocal();
                //actorTransform->updateLocalFromWorld();
                actor->updateTransform();
            }
            */

            auto numMeshes = pNode->mNumMeshes;

            static const auto engineMeshExtention = String( ".fbmeshbin" );
            auto meshFilePath = String( pNode->mName.data ) + "_" + StringUtil::toString( m_fileUUID ) +
                                engineMeshExtention;

            if( numMeshes > 0 && pNode->mMeshes )
            {
                if( getUseSingleMesh() )
                {
                    // if(mMeshes.size() == 0)
                    //{
                    //	static int nameExt = 0;
                    //	mesh = m_meshMgr->create(String("ROOTMesh") + StringUtil::toString(nameExt++),
                    //IResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

                    //	SmartPtr<IMesh> pMesh(new CMesh);
                    //	mesh->setMesh(pMesh);

                    //	mMeshes.push_back(mesh);
                    //}
                    // else
                    //{
                    //	mesh = mMeshes[0];
                    //}
                }

                for( u32 idx = 0; idx < numMeshes; ++idx )
                {
                    try
                    {
                        auto meshIndex = pNode->mMeshes[idx];

                        auto pAIMesh = getAssimpMesh( mScene, meshIndex );
                        if( !pAIMesh )
                        {
                            continue;
                        }

                        auto meshSignature = String();
                        if( getUseMeshInstancing() )
                        {
                            meshSignature = getAssimpMeshSignature( pAIMesh );
                            if( !StringUtil::isNullOrEmpty( meshSignature ) &&
                                m_loadedMeshSignatures.find( meshSignature ) !=
                                    m_loadedMeshSignatures.end() )
                            {
                                continue;
                            }
                        }

                        if( !m_quietMode )
                        {
                            WP_LOG_INFO( String( "SubMesh " ) + StringUtil::toString( idx ) +
                                         String( " for mesh '" ) + String( pNode->mName.data ) + "'" );
                        }

                        // Create a material instance for the mesh.
                        const auto pAIMaterial = getAssimpMaterial( mScene, pAIMesh );
                        if( !pAIMaterial )
                        {
                            continue;
                        }

                        auto nodeNode = String( pNode->mName.data );
                        if( createSubMesh( nodeNode, idx, pNode, pAIMesh, pAIMaterial, mesh, mDir ) &&
                            !StringUtil::isNullOrEmpty( meshSignature ) )
                        {
                            m_loadedMeshSignatures.insert( meshSignature );
                        }
                    }
                    catch( std::exception &e )
                    {
                        WP_LOG_EXCEPTION( e );
                    }
                }

                for( u32 idx = 0; idx < pNode->mNumMeshes; ++idx )
                {
                    auto meshIndex = pNode->mMeshes[idx];
                    auto pAIMesh = getAssimpMesh( mScene, meshIndex );
                    if( !pAIMesh )
                    {
                        continue;
                    }

                    const auto pAIMaterial = getAssimpMaterial( mScene, pAIMesh );
                    if( !pAIMaterial )
                    {
                        continue;
                    }
                }
            }

            // Traverse all child nodes of the current node instance
            for( u32 childIdx = 0; pNode->mChildren && childIdx < pNode->mNumChildren; childIdx++ )
            {
                const auto pChildNode = pNode->mChildren[childIdx];
                loadToMesh( mesh, mScene, pChildNode, mDir );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void AssimpLoader::setRootActor( SmartPtr<scene::IGameActor> rootActor )
    {
        m_rootActor = rootActor;
    }

    bool AssimpLoader::getImportMesh() const
    {
        return m_importMesh;
    }

    void AssimpLoader::setImportMesh( bool importMesh )
    {
        m_importMesh = importMesh;
    }

    Vector3<f32> AssimpLoader::getMeshScale() const
    {
        return m_meshScale;
    }

    void AssimpLoader::setMeshScale( const Vector3<f32> &meshScale )
    {
        m_meshScale = meshScale;
    }

    bool AssimpLoader::getOverwrite() const
    {
        return m_overwrite;
    }

    void AssimpLoader::setOverwrite( bool overwrite )
    {
        m_overwrite = overwrite;
    }

    Matrix4F AssimpLoader::convertAssimpMatrix( const aiMatrix4x4 &from )
    {
        return Matrix4F( from.a1, from.a2, from.a3, from.a4, from.b1, from.b2, from.b3, from.b4, from.c1,
                         from.c2, from.c3, from.c4, from.d1, from.d2, from.d3, from.d4 );
    }

    SmartPtr<ISkeleton> AssimpLoader::getSkeleton() const
    {
        return m_skeleton;
    }

    void AssimpLoader::setSkeleton( SmartPtr<ISkeleton> skeleton )
    {
        m_skeleton = skeleton;
    }

    void AssimpLoader::lock()
    {
        m_mutex.lock();
    }

    bool AssimpLoader::try_lock()
    {
        return m_mutex.try_lock();
    }

    void AssimpLoader::unlock()
    {
        m_mutex.unlock();
    }

    SmartPtr<scene::IGameActor> AssimpLoader::getRootActor() const
    {
        return m_rootActor;
    }

    String AssimpLoader::getMeshPath() const
    {
        return m_meshPath;
    }

    AssimpLoader::MaterialImportOptions AssimpLoader::getMaterialImportOptions() const
    {
        return m_materialImportOptions;
    }

    void AssimpLoader::setMaterialImportOptions( const MaterialImportOptions &options )
    {
        m_materialImportOptions = options;
    }

    String AssimpLoader::getMaterialsFolderName() const
    {
        return m_materialImportOptions.materialsFolderName;
    }

    void AssimpLoader::setMaterialsFolderName( const String &materialsFolderName )
    {
        m_materialImportOptions.materialsFolderName = StringUtil::isNullOrEmpty( materialsFolderName )
                                                          ? String( "Materials" )
                                                          : materialsFolderName;
    }

    String AssimpLoader::getDefaultMaterialNamePrefix() const
    {
        return m_materialImportOptions.defaultMaterialNamePrefix;
    }

    void AssimpLoader::setDefaultMaterialNamePrefix( const String &defaultMaterialNamePrefix )
    {
        m_materialImportOptions.defaultMaterialNamePrefix =
            StringUtil::isNullOrEmpty( defaultMaterialNamePrefix ) ? String( "Material" )
                                                                   : defaultMaterialNamePrefix;
    }

    bool AssimpLoader::getCreateMaterialFiles() const
    {
        return m_materialImportOptions.createMaterialFiles;
    }

    void AssimpLoader::setCreateMaterialFiles( bool createMaterialFiles )
    {
        m_materialImportOptions.createMaterialFiles = createMaterialFiles;
    }

    bool AssimpLoader::getOverwriteMaterialFiles() const
    {
        return m_materialImportOptions.overwriteMaterialFiles;
    }

    void AssimpLoader::setOverwriteMaterialFiles( bool overwriteMaterialFiles )
    {
        m_materialImportOptions.overwriteMaterialFiles = overwriteMaterialFiles;
    }

    bool AssimpLoader::getAssignMaterialComponents() const
    {
        return m_materialImportOptions.assignMaterialComponents;
    }

    void AssimpLoader::setAssignMaterialComponents( bool assignMaterialComponents )
    {
        m_materialImportOptions.assignMaterialComponents = assignMaterialComponents;
    }

    bool AssimpLoader::getImportTextures() const
    {
        return m_materialImportOptions.importTextures;
    }

    void AssimpLoader::setImportTextures( bool importTextures )
    {
        m_materialImportOptions.importTextures = importTextures;
    }

    bool AssimpLoader::getImportMaterialColours() const
    {
        return m_materialImportOptions.importMaterialColours;
    }

    void AssimpLoader::setImportMaterialColours( bool importMaterialColours )
    {
        m_materialImportOptions.importMaterialColours = importMaterialColours;
    }

    bool AssimpLoader::getImportPbrProperties() const
    {
        return m_materialImportOptions.importPbrProperties;
    }

    void AssimpLoader::setImportPbrProperties( bool importPbrProperties )
    {
        m_materialImportOptions.importPbrProperties = importPbrProperties;
    }

    bool AssimpLoader::getUseFallbackTextureNames() const
    {
        return m_materialImportOptions.useFallbackTextureNames;
    }

    void AssimpLoader::setUseFallbackTextureNames( bool useFallbackTextureNames )
    {
        m_materialImportOptions.useFallbackTextureNames = useFallbackTextureNames;
    }

    String AssimpLoader::getMaterialName( const aiMaterial *mat, s32 index ) const
    {
        auto materialName = String();
        if( mat )
        {
            auto assimpName = mat->GetName();
            auto pName = assimpName.C_Str();
            materialName = String( pName ? pName : "" );
        }

        if( StringUtil::isNullOrEmpty( materialName ) )
        {
            materialName = m_materialImportOptions.defaultMaterialNamePrefix + "_" +
                           StringUtil::toString( index < 0 ? 0 : index );
        }

        materialName = StringUtil::replaceAll( materialName, "\\", "_" );
        materialName = StringUtil::replaceAll( materialName, "/", "_" );
        materialName = StringUtil::replaceAll( materialName, ":", "_" );
        materialName = StringUtil::replaceAll( materialName, "*", "_" );
        materialName = StringUtil::replaceAll( materialName, "?", "_" );
        materialName = StringUtil::replaceAll( materialName, "\"", "_" );
        materialName = StringUtil::replaceAll( materialName, "<", "_" );
        materialName = StringUtil::replaceAll( materialName, ">", "_" );
        materialName = StringUtil::replaceAll( materialName, "|", "_" );

        return materialName;
    }

    String AssimpLoader::getMaterialsPath( const String &folderPath ) const
    {
        auto materialFolder = m_materialImportOptions.materialsFolderName;
        if( StringUtil::isNullOrEmpty( materialFolder ) )
        {
            materialFolder = "Materials";
        }

        auto basePath = folderPath;
        if( StringUtil::isNullOrEmpty( basePath ) )
        {
            basePath = Path::getFilePath( getMeshPath() );
        }

        return StringUtil::cleanupPath( StringUtil::isNullOrEmpty( basePath )
                                            ? materialFolder
                                            : Path::lexically_normal( basePath, materialFolder ) );
    }

    String AssimpLoader::getMaterialPath( const aiMaterial *mat, s32 index,
                                          const String &folderPath ) const
    {
        auto materialFileName = getMaterialName( mat, index ) + ApplicationUtil::materialExt;
        return StringUtil::cleanupPath(
            Path::lexically_normal( getMaterialsPath( folderPath ), materialFileName ) );
    }

    SmartPtr<render::IMaterialPass> AssimpLoader::ensureMaterialPass(
        SmartPtr<render::IMaterial> material )
    {
        if( !material )
        {
            return nullptr;
        }

        auto technique = SmartPtr<render::IMaterialTechnique>();
        auto techniques = material->getTechniques();
        if( !techniques.empty() )
        {
            technique = techniques.front();
        }
        else
        {
            technique = material->createTechnique();
        }

        if( !technique )
        {
            return nullptr;
        }

        auto passes = technique->getPasses();
        if( !passes.empty() )
        {
            return passes.front();
        }

        return technique->createPass();
    }

    SmartPtr<render::IMaterial> AssimpLoader::createOrRetrieveMaterial( const aiMaterial *mat, s32 index,
                                                                        const String &folderPath )
    {
        auto applicationManager = core::IApplicationManager::instancePtr();
        WP_ASSERT( applicationManager );

        auto resourceDatabase = applicationManager->getResourceDatabasePtr();
        WP_ASSERT( resourceDatabase );

        auto materialPath = getMaterialPath( mat, index, folderPath );
        auto result = resourceDatabase->createOrRetrieveByType<render::IMaterial>( materialPath );
        return result.first;
    }

    void AssimpLoader::applyAssimpMaterialProperties( SmartPtr<render::IMaterial> material,
                                                      SmartPtr<render::IMaterialPass> pass,
                                                      const aiMaterial *mat, s32 index,
                                                      const String &folderPath )
    {
        if( !material || !mat )
        {
            return;
        }

        if( m_materialImportOptions.importMaterialColours )
        {
            aiColor4D clr( 1.0f, 1.0f, 1.0f, 1.0f );
            if( AI_SUCCESS == aiGetMaterialColor( mat, AI_MATKEY_COLOR_DIFFUSE, &clr ) )
            {
                auto diffuse = ColourF( clr.r, clr.g, clr.b, clr.a );
                material->setDiffuse( diffuse );
                if( pass )
                {
                    pass->setDiffuse( diffuse );
                }
            }

            clr = aiColor4D( 1.0f, 1.0f, 1.0f, 1.0f );
            if( AI_SUCCESS == aiGetMaterialColor( mat, AI_MATKEY_COLOR_SPECULAR, &clr ) )
            {
                auto specular = ColourF( clr.r, clr.g, clr.b, clr.a );
                material->setSpecular( specular );
                if( pass )
                {
                    pass->setSpecular( specular );
                }
            }

            clr = aiColor4D( 0.0f, 0.0f, 0.0f, 1.0f );
            if( AI_SUCCESS == aiGetMaterialColor( mat, AI_MATKEY_COLOR_EMISSIVE, &clr ) )
            {
                auto emissive = ColourF( clr.r, clr.g, clr.b, clr.a );
                material->setEmissive( emissive );
                material->setEmissionEnabled( clr.r > 0.0f || clr.g > 0.0f || clr.b > 0.0f );
                if( pass )
                {
                    pass->setEmissive( emissive );
                    pass->setEmissionEnabled( material->isEmissionEnabled() );
                }
            }

            f32 opacity = 1.0f;
            if( AI_SUCCESS == aiGetMaterialFloat( mat, AI_MATKEY_OPACITY, &opacity ) )
            {
                material->setOpacity( opacity );
                if( pass && opacity < 0.999f )
                {
                    pass->setTransparent( true );
                }
            }
        }

        if( m_materialImportOptions.importPbrProperties )
        {
            f32 metalness = 0.5f;
            aiGetMaterialFloat( mat, AI_MATKEY_METALLIC_FACTOR, &metalness );
            material->setMetalness( metalness );
            if( pass )
            {
                pass->setMetalness( metalness );
            }

            f32 roughness = 0.0f;
            if( AI_SUCCESS == aiGetMaterialFloat( mat, AI_MATKEY_ROUGHNESS_FACTOR, &roughness ) )
            {
                material->setRoughness( roughness );
                if( pass )
                {
                    pass->setRoughness( roughness );
                }
            }
        }

        if( m_materialImportOptions.importTextures )
        {
            const aiTextureType textureTypes[] = {
                aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE,          aiTextureType_EMISSIVE,
                aiTextureType_SPECULAR,   aiTextureType_NORMALS,          aiTextureType_HEIGHT,
                aiTextureType_METALNESS,  aiTextureType_DIFFUSE_ROUGHNESS
            };

            u32 layerIdx = 0;
            for( auto textureType : textureTypes )
            {
                aiString path;
                if( mat->GetTexture( textureType, 0, &path ) != AI_SUCCESS )
                {
                    continue;
                }

                auto textureName = String( path.C_Str() ? path.C_Str() : "" );
                if( StringUtil::isNullOrEmpty( textureName ) )
                {
                    continue;
                }

                if( isEmbeddedTexturePath( textureName ) )
                {
                    WP_LOG( "Skipping embedded Assimp texture " + textureName + " for material " +
                            getMaterialName( mat, index ) );
                    continue;
                }

                auto applicationManager = core::IApplicationManager::instancePtr();
                auto fileSystem = applicationManager ? applicationManager->getFileSystemPtr() : nullptr;
                auto texturePath = textureName;
                if( fileSystem && !fileSystem->isExistingFile( texturePath ) )
                {
                    auto localTexturePath = Path::lexically_normal( folderPath, textureName );
                    if( fileSystem->isExistingFile( localTexturePath ) )
                    {
                        texturePath = localTexturePath;
                    }
                    else if( m_materialImportOptions.useFallbackTextureNames )
                    {
                        texturePath = Path::getFileName( textureName );
                    }
                }

                material->setTexture( texturePath, layerIdx );
                if( pass )
                {
                    pass->setTexture( texturePath, layerIdx );
                }

                ++layerIdx;
            }
        }
    }

    void AssimpLoader::createMaterial( SmartPtr<scene::Material> omat, s32 index, const aiMaterial *mat,
                                       const String &folderPath )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystemPtr();
            WP_ASSERT( fileSystem );

            auto projectFolder = applicationManager->getProjectPath();

            auto graphicsSystem = applicationManager->getGraphicsSystemPtr();
            if( !graphicsSystem )
            {
                WP_LOG( "No graphics system." );
                return;
            }

            auto materialManager = graphicsSystem->getMaterialManagerPtr();
            WP_ASSERT( materialManager );

            if( !mat )
            {
                WP_LOG( "Assimp material is null." );
                return;
            }

            auto materialsPath = getMaterialsPath( folderPath );
            auto absoluteMaterialsPath = Path::isPathAbsolute( materialsPath )
                                             ? materialsPath
                                             : Path::getAbsolutePath( projectFolder, materialsPath );
            absoluteMaterialsPath = StringUtil::cleanupPath( absoluteMaterialsPath );

            if( !fileSystem->isExistingFolder( absoluteMaterialsPath ) )
            {
                fileSystem->createDirectories( absoluteMaterialsPath );
            }

            auto materialPath = getMaterialPath( mat, index, folderPath );

            if( omat )
            {
                omat->setMaterialPath( materialPath );
            }

            auto material = createOrRetrieveMaterial( mat, index, folderPath );
            if( !material )
            {
                WP_LOG( "Failed to create material from " + materialPath );
                return;
            }

            auto pass = ensureMaterialPass( material );
            if( m_materialImportOptions.createMaterialFiles )
            {
                auto absoluteMaterialPath = Path::isPathAbsolute( materialPath )
                                                ? materialPath
                                                : Path::getAbsolutePath( projectFolder, materialPath );
                auto shouldWriteMaterial = m_materialImportOptions.overwriteMaterialFiles ||
                                           !fileSystem->isExistingFile( absoluteMaterialPath );
                if( shouldWriteMaterial )
                {
                    applyAssimpMaterialProperties( material, pass, mat, index, folderPath );
                    materialManager->saveToFile( materialPath, material );
                }
            }

            if( omat )
            {
                omat->setMaterial( material );
                omat->updateMaterial();
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    SmartPtr<scene::Material> AssimpLoader::assignMaterialComponent( SmartPtr<scene::IGameActor> actor,
                                                                     u32 index, const aiMaterial *mat,
                                                                     const String &folderPath )
    {
        if( !actor || !mat )
        {
            return nullptr;
        }

        if( m_materialImportOptions.createMaterialFiles )
        {
            createMaterial( nullptr, static_cast<s32>( index ), mat, folderPath );
        }

        auto material = createOrRetrieveMaterial( mat, static_cast<s32>( index ), folderPath );
        if( !material )
        {
            createMaterial( nullptr, static_cast<s32>( index ), mat, folderPath );
            material = createOrRetrieveMaterial( mat, static_cast<s32>( index ), folderPath );
        }

        auto materialComponent = actor->addComponent<scene::Material>();
        if( !materialComponent )
        {
            return nullptr;
        }

        materialComponent->setIndex( index );
        materialComponent->setMaterialPath(
            getMaterialPath( mat, static_cast<s32>( index ), folderPath ) );
        materialComponent->setMaterial( material );
        materialComponent->updateMaterial();

        return materialComponent;
    }

    SmartPtr<ISkeleton> AssimpLoader::createSkeleton( const aiScene *scene )
    {
        m_bonesByName.clear();
        m_boneNodesByName.clear();

        if( !scene )
        {
            return nullptr;
        }

        std::set<String> boneNames;
        for( u32 meshIdx = 0; scene->mMeshes && meshIdx < scene->mNumMeshes; ++meshIdx )
        {
            const auto mesh = scene->mMeshes[meshIdx];
            if( !mesh || !mesh->HasBones() )
            {
                continue;
            }

            for( u32 boneIdx = 0; mesh->mBones && boneIdx < mesh->mNumBones; ++boneIdx )
            {
                const auto bone = mesh->mBones[boneIdx];
                if( !bone )
                {
                    continue;
                }

                const String boneName = bone->mName.C_Str();
                if( StringUtil::isNullOrEmpty( boneName ) )
                {
                    continue;
                }

                boneNames.insert( boneName );
                m_bonesByName[boneName] = bone;
            }
        }

        for( u32 animIdx = 0; scene->mAnimations && animIdx < scene->mNumAnimations; ++animIdx )
        {
            const auto animation = scene->mAnimations[animIdx];
            if( !animation )
            {
                continue;
            }

            for( u32 channelIdx = 0; animation->mChannels && channelIdx < animation->mNumChannels;
                 ++channelIdx )
            {
                const auto channel = animation->mChannels[channelIdx];
                if( channel )
                {
                    const String boneName = channel->mNodeName.C_Str();
                    if( !StringUtil::isNullOrEmpty( boneName ) )
                    {
                        boneNames.insert( boneName );
                    }
                }
            }
        }

        if( boneNames.empty() )
        {
            return nullptr;
        }

        collectBoneNodes( scene->mRootNode );

        auto skeleton = workphone::make_ptr<MeshSkeleton>();
        u32 handle = 0;
        for( const auto &boneName : boneNames )
        {
            auto bone = skeleton->createBone( boneName, handle++ );
            if( !bone )
            {
                continue;
            }

            auto nodeIt = m_boneNodesByName.find( boneName );
            if( nodeIt != m_boneNodesByName.end() && nodeIt->second )
            {
                aiVector3D scale;
                aiQuaternion rotation;
                aiVector3D position;
                nodeIt->second->mTransformation.Decompose( scale, rotation, position );

                bone->setPosition( Vector3<real_Num>( position.x, position.y, position.z ) *
                                   getMeshScale() );
                bone->setOrientation(
                    Quaternion<real_Num>( rotation.w, rotation.x, rotation.y, rotation.z ) );
                bone->setBindingPose();
            }
        }

        importAnimations( scene, skeleton );
        return skeleton;
    }

    void AssimpLoader::writeSkeletonCacheFile( const String &meshPath, SmartPtr<ISkeleton> skeleton )
    {
        if( StringUtil::isNullOrEmpty( meshPath ) )
        {
            return;
        }

        const auto skeletonPath = getAbsoluteCacheSkeletonPath( meshPath, m_fileUUID );
        if( StringUtil::isNullOrEmpty( skeletonPath ) )
        {
            return;
        }

        if( !skeleton )
        {
            deleteCacheFileIfExists( skeletonPath );
            return;
        }

        auto meshSkeleton = workphone::dynamic_pointer_cast<MeshSkeleton>( skeleton );
        if( !meshSkeleton || meshSkeleton->getAnimations().empty() )
        {
            deleteCacheFileIfExists( skeletonPath );
            return;
        }

        auto folder = Path::getFilePath( skeletonPath );
        if( !StringUtil::isNullOrEmpty( folder ) && !Path::isExistingFolder( folder ) )
        {
            Path::createDirectories( folder );
        }

        XMLSkeletonSerializer serializer;
        serializer.exportSkeleton( skeleton.get(), skeletonPath );

        for( auto &meshParam : m_meshes )
        {
            auto mesh = workphone::dynamic_pointer_cast<Mesh>( meshParam.object );
            if( mesh )
            {
                mesh->setSkeletonName( skeletonPath );
            }
        }
    }

    Array<SmartPtr<IAnimation> > AssimpLoader::importSceneAnimations( const aiScene *scene )
    {
        Array<SmartPtr<IAnimation> > animations;
        if( !scene || !scene->HasAnimations() || m_sceneActorsByName.empty() )
        {
            return animations;
        }

        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager ? applicationManager->getFactoryManager() : nullptr;
        if( !factoryManager )
        {
            return animations;
        }

        for( u32 animIdx = 0; scene->mAnimations && animIdx < scene->mNumAnimations; ++animIdx )
        {
            const auto assimpAnim = scene->mAnimations[animIdx];
            if( !assimpAnim )
            {
                continue;
            }

            const double ticksPerSecond =
                assimpAnim->mTicksPerSecond > 0.0 ? assimpAnim->mTicksPerSecond : 25.0;
            const auto length = static_cast<f32>( assimpAnim->mDuration / ticksPerSecond );
            const String animationName =
                StringUtil::isNullOrEmpty( assimpAnim->mName.C_Str() )
                    ? String( "SceneAnimation_" ) + StringUtil::toString( animIdx )
                    : String( assimpAnim->mName.C_Str() );

            auto animation = factoryManager->make_ptr<Animation>();
            if( !animation )
            {
                continue;
            }

            animation->setName( animationName );
            animation->setLength( std::max( length, 0.0001f ) );
            animation->setInterpolationMode( InterpolationMode::LINEAR );
            animation->setRotationInterpolationMode( RotationInterpolationMode::LINEAR );

            for( u32 channelIdx = 0; assimpAnim->mChannels && channelIdx < assimpAnim->mNumChannels;
                 ++channelIdx )
            {
                const auto channel = assimpAnim->mChannels[channelIdx];
                if( !channel )
                {
                    continue;
                }

                const String actorName = channel->mNodeName.C_Str();
                auto actorIt = m_sceneActorsByName.find( actorName );
                if( actorIt == m_sceneActorsByName.end() || !actorIt->second )
                {
                    continue;
                }

                auto trackBase = animation->addTrack( StringUtil::getHash( "ActorAnimationTrack" ),
                                                      actorName, static_cast<u16>( channelIdx ) );
                auto track = workphone::dynamic_pointer_cast<IActorAnimationTrack>( trackBase );
                if( !track )
                {
                    continue;
                }

                track->removeAllKeyFrames();
                track->setActor( actorIt->second );
                track->setPropertyName( StringUtil::EmptyString );
                track->setTrackType( IActorAnimationTrack::TrackType::Transform );

                std::set<f32> keyTimes;
                for( u32 i = 0; i < channel->mNumPositionKeys; ++i )
                {
                    keyTimes.insert(
                        static_cast<f32>( channel->mPositionKeys[i].mTime / ticksPerSecond ) );
                }
                for( u32 i = 0; i < channel->mNumRotationKeys; ++i )
                {
                    keyTimes.insert(
                        static_cast<f32>( channel->mRotationKeys[i].mTime / ticksPerSecond ) );
                }
                for( u32 i = 0; i < channel->mNumScalingKeys; ++i )
                {
                    keyTimes.insert(
                        static_cast<f32>( channel->mScalingKeys[i].mTime / ticksPerSecond ) );
                }

                for( auto time : keyTimes )
                {
                    auto keyFrame = workphone::dynamic_pointer_cast<KeyFrameTransform3>(
                        track->createKeyFrame( time ) );
                    if( !keyFrame )
                    {
                        continue;
                    }

                    aiVector3D position = channel->mNumPositionKeys > 0
                                              ? channel->mPositionKeys[0].mValue
                                              : aiVector3D( 0.0f, 0.0f, 0.0f );
                    for( u32 i = 0; i < channel->mNumPositionKeys; ++i )
                    {
                        if( channel->mPositionKeys[i].mTime / ticksPerSecond <= time )
                        {
                            position = channel->mPositionKeys[i].mValue;
                        }
                    }

                    aiQuaternion rotation = channel->mNumRotationKeys > 0
                                                ? channel->mRotationKeys[0].mValue
                                                : aiQuaternion();
                    for( u32 i = 0; i < channel->mNumRotationKeys; ++i )
                    {
                        if( channel->mRotationKeys[i].mTime / ticksPerSecond <= time )
                        {
                            rotation = channel->mRotationKeys[i].mValue;
                        }
                    }

                    aiVector3D scale = channel->mNumScalingKeys > 0 ? channel->mScalingKeys[0].mValue
                                                                    : aiVector3D( 1.0f, 1.0f, 1.0f );
                    for( u32 i = 0; i < channel->mNumScalingKeys; ++i )
                    {
                        if( channel->mScalingKeys[i].mTime / ticksPerSecond <= time )
                        {
                            scale = channel->mScalingKeys[i].mValue;
                        }
                    }

                    keyFrame->setPosition( Vector3<real_Num>( position.x, position.y, position.z ) *
                                           getMeshScale() );
                    keyFrame->setOrientation(
                        Quaternion<real_Num>( rotation.w, rotation.x, rotation.y, rotation.z ) );
                    keyFrame->setScale( Vector3<real_Num>( scale.x, scale.y, scale.z ) );
                }
            }

            if( animation->getNumNodeTracks() > 0 )
            {
                animations.push_back( animation );
            }
        }

        return animations;
    }

    void AssimpLoader::writeSceneAnimationCacheFile( const String &meshPath,
                                                     const Array<SmartPtr<IAnimation> > &animations )
    {
        if( StringUtil::isNullOrEmpty( meshPath ) )
        {
            return;
        }

        const auto animationPath = getAbsoluteCacheSceneAnimationPath( meshPath, m_fileUUID );
        if( StringUtil::isNullOrEmpty( animationPath ) )
        {
            return;
        }

        if( animations.empty() )
        {
            deleteCacheFileIfExists( animationPath );
            return;
        }

        auto folder = Path::getFilePath( animationPath );
        if( !StringUtil::isNullOrEmpty( folder ) && !Path::isExistingFolder( folder ) )
        {
            Path::createDirectories( folder );
        }

        TiXmlDocument doc;
        auto rootNode = appendElement( &doc, "sceneAnimations" );
        rootNode->SetAttribute( "source", meshPath.c_str() );

        for( const auto &animation : animations )
        {
            if( !animation )
            {
                continue;
            }

            auto animationNode = appendElement( rootNode, "animation" );
            animationNode->SetAttribute( "name", animation->getName().c_str() );
            animationNode->SetDoubleAttribute( "length", animation->getLength() );

            auto tracksNode = appendElement( animationNode, "tracks" );
            const auto &nodeTracks = animation->_getNodeTrackList();
            for( const auto &trackPair : nodeTracks )
            {
                const auto track = trackPair.second;
                if( !track )
                {
                    continue;
                }

                auto trackNode = appendElement( tracksNode, "track" );
                auto actor = track->getActor();
                auto actorName = actor ? actor->getName() : track->getPropertyName();
                trackNode->SetAttribute( "actor", actorName.c_str() );
                trackNode->SetAttribute(
                    "handle", std::to_string( static_cast<unsigned int>( trackPair.first ) ).c_str() );

                auto keyFramesNode = appendElement( trackNode, "keyframes" );
                for( u16 keyIdx = 0; keyIdx < track->getNumKeyFrames(); ++keyIdx )
                {
                    auto keyFrame = workphone::dynamic_pointer_cast<KeyFrameTransform3>(
                        track->getKeyFrame( keyIdx ) );
                    if( !keyFrame )
                    {
                        continue;
                    }

                    auto keyNode = appendElement( keyFramesNode, "keyframe" );
                    keyNode->SetDoubleAttribute( "time", keyFrame->getTime() );

                    auto position = keyFrame->getPosition();
                    auto translateNode = appendElement( keyNode, "translate" );
                    translateNode->SetDoubleAttribute( "x", position.x );
                    translateNode->SetDoubleAttribute( "y", position.y );
                    translateNode->SetDoubleAttribute( "z", position.z );

                    auto orientation = keyFrame->getOrientation();
                    auto rotationNode = appendElement( keyNode, "rotation" );
                    rotationNode->SetDoubleAttribute( "qw", orientation.w );
                    rotationNode->SetDoubleAttribute( "qx", orientation.x );
                    rotationNode->SetDoubleAttribute( "qy", orientation.y );
                    rotationNode->SetDoubleAttribute( "qz", orientation.z );

                    auto scale = keyFrame->getScale();
                    auto scaleNode = appendElement( keyNode, "scale" );
                    scaleNode->SetDoubleAttribute( "x", scale.x );
                    scaleNode->SetDoubleAttribute( "y", scale.y );
                    scaleNode->SetDoubleAttribute( "z", scale.z );
                }
            }
        }

        if( !doc.SaveFile( animationPath.c_str() ) )
        {
            WP_LOG_ERROR( "AssimpLoader failed writing scene animation XML file: " + animationPath );
            return;
        }

        WP_LOG( "AssimpLoader scene animation export successful: " + animationPath );
    }

    void AssimpLoader::collectBoneNodes( const aiNode *node )
    {
        if( !node )
        {
            return;
        }

        const String nodeName = node->mName.C_Str();
        if( !StringUtil::isNullOrEmpty( nodeName ) )
        {
            m_boneNodesByName[nodeName] = node;
        }

        for( u32 childIdx = 0; node->mChildren && childIdx < node->mNumChildren; ++childIdx )
        {
            collectBoneNodes( node->mChildren[childIdx] );
        }
    }

    void AssimpLoader::importAnimations( const aiScene *scene, SmartPtr<ISkeleton> skeleton )
    {
        if( !scene || !skeleton || !scene->HasAnimations() )
        {
            return;
        }

        for( u32 animIdx = 0; scene->mAnimations && animIdx < scene->mNumAnimations; ++animIdx )
        {
            const auto assimpAnim = scene->mAnimations[animIdx];
            if( !assimpAnim )
            {
                continue;
            }

            const double ticksPerSecond =
                assimpAnim->mTicksPerSecond > 0.0 ? assimpAnim->mTicksPerSecond : 25.0;
            const auto length = static_cast<f32>( assimpAnim->mDuration / ticksPerSecond );
            const String animationName = StringUtil::isNullOrEmpty( assimpAnim->mName.C_Str() )
                                             ? String( "Animation_" ) + StringUtil::toString( animIdx )
                                             : String( assimpAnim->mName.C_Str() );

            auto animation = skeleton->createAnimation( animationName, std::max( length, 0.0001f ) );
            if( !animation )
            {
                continue;
            }

            animation->setInterpolationMode( InterpolationMode::LINEAR );
            animation->setRotationInterpolationMode( RotationInterpolationMode::LINEAR );

            for( u32 channelIdx = 0; assimpAnim->mChannels && channelIdx < assimpAnim->mNumChannels;
                 ++channelIdx )
            {
                const auto channel = assimpAnim->mChannels[channelIdx];
                if( !channel )
                {
                    continue;
                }

                const String boneName = channel->mNodeName.C_Str();
                auto bone = skeleton->getBone( boneName );
                if( !bone )
                {
                    continue;
                }

                auto trackBase = animation->addTrack( StringUtil::getHash( "ActorAnimationTrack" ),
                                                      bone->getBoneHandle(), bone );
                auto track = workphone::dynamic_pointer_cast<IActorAnimationTrack>( trackBase );
                if( !track )
                {
                    continue;
                }

                track->removeAllKeyFrames();
                track->setPropertyName( boneName );
                track->setTrackType( IActorAnimationTrack::TrackType::Transform );

                std::set<f32> keyTimes;
                for( u32 i = 0; i < channel->mNumPositionKeys; ++i )
                {
                    keyTimes.insert(
                        static_cast<f32>( channel->mPositionKeys[i].mTime / ticksPerSecond ) );
                }
                for( u32 i = 0; i < channel->mNumRotationKeys; ++i )
                {
                    keyTimes.insert(
                        static_cast<f32>( channel->mRotationKeys[i].mTime / ticksPerSecond ) );
                }
                for( u32 i = 0; i < channel->mNumScalingKeys; ++i )
                {
                    keyTimes.insert(
                        static_cast<f32>( channel->mScalingKeys[i].mTime / ticksPerSecond ) );
                }

                for( auto time : keyTimes )
                {
                    auto keyFrame = workphone::dynamic_pointer_cast<KeyFrameTransform3>(
                        track->createKeyFrame( time ) );
                    if( !keyFrame )
                    {
                        continue;
                    }

                    aiVector3D position = channel->mNumPositionKeys > 0
                                              ? channel->mPositionKeys[0].mValue
                                              : aiVector3D( 0.0f, 0.0f, 0.0f );
                    for( u32 i = 0; i < channel->mNumPositionKeys; ++i )
                    {
                        if( channel->mPositionKeys[i].mTime / ticksPerSecond <= time )
                        {
                            position = channel->mPositionKeys[i].mValue;
                        }
                    }

                    aiQuaternion rotation = channel->mNumRotationKeys > 0
                                                ? channel->mRotationKeys[0].mValue
                                                : aiQuaternion();
                    for( u32 i = 0; i < channel->mNumRotationKeys; ++i )
                    {
                        if( channel->mRotationKeys[i].mTime / ticksPerSecond <= time )
                        {
                            rotation = channel->mRotationKeys[i].mValue;
                        }
                    }

                    aiVector3D scale = channel->mNumScalingKeys > 0 ? channel->mScalingKeys[0].mValue
                                                                    : aiVector3D( 1.0f, 1.0f, 1.0f );
                    for( u32 i = 0; i < channel->mNumScalingKeys; ++i )
                    {
                        if( channel->mScalingKeys[i].mTime / ticksPerSecond <= time )
                        {
                            scale = channel->mScalingKeys[i].mValue;
                        }
                    }

                    keyFrame->setPosition( Vector3<real_Num>( position.x, position.y, position.z ) *
                                           getMeshScale() );
                    keyFrame->setOrientation(
                        Quaternion<real_Num>( rotation.w, rotation.x, rotation.y, rotation.z ) );
                    keyFrame->setScale( Vector3<real_Num>( scale.x, scale.y, scale.z ) );
                }
            }
        }
    }

    void AssimpLoader::computeNodesDerivedTransform( const aiScene *mScene, const aiNode *pNode,
                                                     const aiMatrix4x4 accTransform )
    {
        try
        {
            if( !pNode )
            {
                return;
            }

            auto it = m_nodeDerivedTransformByName.find( pNode->mName.data );
            if( it == m_nodeDerivedTransformByName.end() )
            {
                m_nodeDerivedTransformByName[pNode->mName.data] = accTransform;
            }

            for( u32 childIdx = 0; pNode->mChildren && childIdx < pNode->mNumChildren; ++childIdx )
            {
                const auto child = pNode->mChildren[childIdx];
                if( child )
                {
                    computeNodesDerivedTransform( mScene, child, accTransform * child->mTransformation );
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    bool AssimpLoader::createSubMesh( const String &name, int index, const aiNode *pNode,
                                      const aiMesh *mesh, const aiMaterial *mat, SmartPtr<IMesh> pMesh,
                                      const String &mDir )
    {
        if( !pMesh )
        {
            WP_LOG_ERROR( "Mesh is null" );
            return false;
        }

        auto applicationManager = core::IApplicationManager::instancePtr();
        auto factoryManager = applicationManager->getFactoryManagerPtr();

        // if animated all submeshes must have bone weights
        if( m_bonesByName.size() && !mesh->HasBones() )
        {
            if( !m_quietMode )
            {
                WP_LOG_INFO( String( "Skipping Mesh " ) + String( mesh->mName.data ) +
                             String( "with no bone weights" ) );
            }

            return false;
        }

        auto matName = mat->GetName();
        auto pName = matName.C_Str();
        String materialName = pName ? pName : "";

        // now begin the object definition
        // We create a submesh per material
        auto submesh = factoryManager->make_ptr<SubMesh>();
        pMesh->addSubMesh( submesh );

        bool swapZY = false;

        if( auto director = getMeshDirector() )
        {
            swapZY = director->getSwapZY();
        }

        // prime pointers to vertex related data
        aiVector3D *vec = mesh->mVertices;
        aiVector3D *tangent = mesh->mTangents;
        aiVector3D *bitangent = mesh->mBitangents;

        aiVector3D *norm = mesh->mNormals;
        //aiVector3D *uv = mesh->mTextureCoords[0];
        aiColor4D *col = mesh->mColors[0];

        Array<Array<Vector3<f32> > > uvs;
        uvs.reserve( mesh->GetNumUVChannels() );

        for( auto uv : mesh->mTextureCoords )
        {
            if( uv )
            {
                Array<Vector3<f32> > uvArray;
                uvArray.reserve( mesh->mNumVertices );

                for( size_t j = 0; j < mesh->mNumVertices; ++j )
                {
                    uvArray.emplace_back( uv->x, uv->y, uv->z );
                    uv++;
                }

                uvs.push_back( uvArray );
            }
        }

        // We must create the vertex data, indicating how many vertices there will be
        // submesh->useSharedVertices = false;

        auto vertexData = factoryManager->make_ptr<VertexBuffer>();
        submesh->setVertexBuffer( vertexData );
        vertexData->setNumVertices( mesh->mNumVertices );

        // We must now declare what the vertex data contains
        auto declaration = factoryManager->make_ptr<VertexDeclaration>();
        vertexData->setVertexDeclaration( declaration );

        auto source = 0u;
        auto offset = 0u;

        auto vertexElem = declaration->addElement( source, offset, VertexElementSemantic::VES_POSITION,
                                                   VertexElementType::VET_FLOAT3 );
        offset += vertexElem->getSize();

        if( !m_quietMode )
        {
            WP_LOG_INFO( StringUtil::toString( mesh->mNumVertices ) + " vertices" );
        }

        if( norm )
        {
            if( !m_quietMode )
            {
                WP_LOG_INFO( StringUtil::toString( mesh->mNumVertices ) + " normals" );
            }

            auto normalElem = declaration->addElement( source, offset, VertexElementSemantic::VES_NORMAL,
                                                       VertexElementType::VET_FLOAT3 );
            offset += normalElem->getSize();
        }

        if( tangent )
        {
            if( !m_quietMode )
            {
                WP_LOG_INFO( StringUtil::toString( mesh->mNumVertices ) + " tangents" );
            }

            auto tangentElem = declaration->addElement(
                source, offset, VertexElementSemantic::VES_TANGENT, VertexElementType::VET_FLOAT3 );
            offset += tangentElem->getSize();
        }

        if( bitangent )
        {
            if( !m_quietMode )
            {
                WP_LOG_INFO( StringUtil::toString( mesh->mNumVertices ) + " bitangents" );
            }

            auto bitangentElem = declaration->addElement(
                source, offset, VertexElementSemantic::VES_BINORMAL, VertexElementType::VET_FLOAT3 );
            offset += bitangentElem->getSize();
        }

        for( u32 i = 0; i < uvs.size(); ++i )
        {
            auto uvElem =
                declaration->addElement( source, offset, VertexElementSemantic::VES_TEXTURE_COORDINATES,
                                         VertexElementType::VET_FLOAT3, i );
            offset += uvElem->getSize();
        }

        if( col )
        {
            auto colElem = declaration->addElement( source, offset, VertexElementSemantic::VES_DIFFUSE,
                                                    VertexElementType::VET_FLOAT4, 0 );
            offset += colElem->getSize();
        }

        auto vdata = static_cast<f32 *>( vertexData->createVertexData() );
        WP_ASSERT( vdata );

        auto meshScale = getMeshScale();

        AABB3<f32> aabb;

        Vector3<f32> pos;

        for( u32 i = 0; i < mesh->mNumVertices; ++i )
        {
            // Position
            if( !swapZY )
            {
                pos = Vector3<f32>( vec->x, vec->y, vec->z ) * meshScale;
            }
            else
            {
                pos = Vector3<f32>( vec->x, vec->z, vec->y ) * meshScale;
            }

            aabb.merge( pos );

            *vdata++ = pos.x;
            *vdata++ = pos.y;
            *vdata++ = pos.z;
            vec++;

            // Normal
            if( norm )
            {
                if( !swapZY )
                {
                    *vdata++ = norm->x;
                    *vdata++ = norm->y;
                    *vdata++ = norm->z;
                }
                else
                {
                    *vdata++ = norm->x;
                    *vdata++ = norm->z;
                    *vdata++ = norm->y;
                }

                norm++;
            }

            if( tangent )
            {
                if( !swapZY )
                {
                    *vdata++ = tangent->x;
                    *vdata++ = tangent->y;
                    *vdata++ = tangent->z;
                }
                else
                {
                    *vdata++ = tangent->x;
                    *vdata++ = tangent->z;
                    *vdata++ = tangent->y;
                }

                tangent++;
            }

            if( bitangent )
            {
                if( !swapZY )
                {
                    *vdata++ = bitangent->x;
                    *vdata++ = bitangent->y;
                    *vdata++ = bitangent->z;
                }
                else
                {
                    *vdata++ = bitangent->x;
                    *vdata++ = bitangent->z;
                    *vdata++ = bitangent->y;
                }

                bitangent++;
            }

            if( uvs.size() > 0 )
            {
                for( auto &j : uvs )
                {
                    auto uv = j[i];
                    *vdata++ = uv.x;
                    *vdata++ = 1.0f - uv.y;
                    *vdata++ = uv.z;
                }
            }

            if( col )
            {
                *vdata++ = col->r;
                *vdata++ = col->g;
                *vdata++ = col->b;
                *vdata++ = col->a;
                col++;
            }
        }

        WP_LOG_INFO( StringUtil::toString( mesh->mNumFaces ) + " faces" );

        // Creates the index data
        auto indexBuffer = factoryManager->make_ptr<IndexBuffer>();
        submesh->setIndexBuffer( indexBuffer );

        if( mesh->mNumVertices < std::numeric_limits<u16>::max() )
        {
            indexBuffer->setIndexType( IndexBuffer::Type::IT_16BIT );

            Array<u16> indices;
            indices.reserve( mesh->mNumFaces * 3 );

            for( u32 i = 0; i < mesh->mNumFaces; ++i )
            {
                auto f = &mesh->mFaces[i];

                // Check for triangles
                if( f->mNumIndices == 3 )
                {
                    for( u32 index = 0; index < f->mNumIndices; ++index )
                    {
                        auto faceIndex = f->mIndices[index];
                        WP_ASSERT( faceIndex < std::numeric_limits<u16>::max() );
                        indices.push_back( faceIndex );
                    }
                }
            }

            indexBuffer->setNumIndices( static_cast<u32>( indices.size() ) );
            auto idata = static_cast<u16 *>( indexBuffer->createIndexData() );
            memcpy( idata, indices.data(), indices.size() * sizeof( u16 ) );
        }
        else
        {
            indexBuffer->setIndexType( IndexBuffer::Type::IT_32BIT );

            Array<u32> indices;
            indices.reserve( mesh->mNumFaces * 3 );

            for( u32 i = 0; i < mesh->mNumFaces; ++i )
            {
                auto f = &mesh->mFaces[i];
                // Check for triangles
                if( f->mNumIndices == 3 )
                {
                    for( u32 index = 0; index < f->mNumIndices; ++index )
                    {
                        auto faceIndex = f->mIndices[index];
                        indices.push_back( faceIndex );
                    }
                }
            }

            indexBuffer->setNumIndices( static_cast<u32>( indices.size() ) );
            auto idata = static_cast<u32 *>( indexBuffer->createIndexData() );
            memcpy( idata, indices.data(), indices.size() * sizeof( u32 ) );
        }

        // set bone weights
        if( mesh->HasBones() )
        {
            for( u32 i = 0; i < mesh->mNumBones; i++ )
            {
                aiBone *pAIBone = mesh->mBones[i];
                if( nullptr != pAIBone )
                {
                    String bname = pAIBone->mName.data;
                    u16 boneIndex = static_cast<u16>( i );
                    if( auto skeleton = getSkeleton() )
                    {
                        if( auto bone = skeleton->getBone( bname ) )
                        {
                            boneIndex = bone->getBoneHandle();
                        }
                    }

                    for( u32 weightIdx = 0; weightIdx < pAIBone->mNumWeights; weightIdx++ )
                    {
                        aiVertexWeight aiWeight = pAIBone->mWeights[weightIdx];

                        auto vba = workphone::make_ptr<VertexBoneAssignment>();
                        vba->setVertexIndex( aiWeight.mVertexId );
                        vba->setBoneIndex( boneIndex );
                        vba->setWeight( aiWeight.mWeight );

                        submesh->addBoneAssignment( vba );
                    }
                }
            }
        }  // if mesh has bones

        // Finally we set a material to the submesh
        submesh->setMaterialName( materialName );

        auto meshAABB = pMesh->getAABB();
        meshAABB.merge( aabb );
        pMesh->setAABB( meshAABB );

        return true;
    }

    void AssimpLoader::importAssimpAnimationToOgre( const aiScene *scene, SmartPtr<ISkeleton> skeleton )
    {
        importAnimations( scene, skeleton );
    }

    void AssimpLoader::loadAnimations( const aiScene *scene, SmartPtr<scene::IGameActor> actor )
    {
        if( !actor )
        {
            return;
        }

        auto skeleton = getSkeleton();
        auto sceneAnimations = importSceneAnimations( scene );
        writeSceneAnimationCacheFile( m_meshPath, sceneAnimations );

        if( !sceneAnimations.empty() )
        {
            auto animator = actor->getComponent<scene::Animator>();
            if( !animator )
            {
                animator = actor->addComponent<scene::Animator>();
            }

            for( const auto &animation : sceneAnimations )
            {
                animator->addAnimation( animation );
            }
        }

        if( scene->HasAnimations() && skeleton )
        {
            auto animator = actor->getComponent<scene::Animator>();
            if( !animator )
            {
                animator = actor->addComponent<scene::Animator>();
            }

            auto mesh = actor->getComponent<scene::Mesh>();
            if( mesh )
            {
                mesh->setSkeleton( skeleton );
            }

            animator->setSkeleton( skeleton );

            for( u32 animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex )
            {
                auto anim = scene->mAnimations[animIndex];
                if( !anim )
                {
                    continue;
                }

                const String animationName =
                    StringUtil::isNullOrEmpty( anim->mName.C_Str() )
                        ? String( "Animation_" ) + StringUtil::toString( animIndex )
                        : String( anim->mName.C_Str() );

                if( auto animation = skeleton->getAnimation( animationName ) )
                {
                    animator->addAnimation( animation );
                }
            }
        }
    }

    //void MeshLoader::loadAnimations( const aiScene *scene, SmartPtr<scene::IGameActor> actor )
    //{
#    if 0
        // Check if the scene contains animations
        if( scene->HasAnimations() )
        {
            // Iterate through each animation in the scene
            for( unsigned int animIndex = 0; animIndex < scene->mNumAnimations; ++animIndex )
            {
                aiAnimation *anim = scene->mAnimations[animIndex];

                // Create an Ogre animation
                auto ogreAnim =
                //    entity->getSkeleton()->createAnimation( anim->mName.data, anim->mDuration );
                //ogreAnim->setInterpolationMode( Ogre::Animation::IM_LINEAR );

                // Iterate through each channel in the animation
                /*
                for( unsigned int channelIndex = 0; channelIndex < anim->mNumChannels;
                          ++channelIndex )
            {
                auto channel = anim->mChannels[channelIndex];
                //Ogre::Node *bone = entity->getSkeleton()->getBone( channel->mNodeName.data );

                // Create a track for the bone
                auto keyFrame =
                    ogreAnim->createNodeKeyFrame( channel->mPositionKeys[0].mTime );
                keyFrame->setTranslate( Vector3F( channel->mPositionKeys[0].mValue.x,
                                                       channel->mPositionKeys[0].mValue.y,
                                                       channel->mPositionKeys[0].mValue.z ) );

                // Interpolate position for the rest of the keyframes
                for( unsigned int keyIndex = 1; keyIndex < channel->mNumPositionKeys; ++keyIndex )
                {
                    auto keyFrame =
                        ogreAnim->createNodeKeyFrame( channel->mPositionKeys[keyIndex].mTime );
                    keyFrame->setTranslate(
                        Ogre::Vector3( channel->mPositionKeys[keyIndex].mValue.x,
                                       channel->mPositionKeys[keyIndex].mValue.y,
                                       channel->mPositionKeys[keyIndex].mValue.z ) );
                }

                // Repeat the process for rotation and scaling keys
                // ...

                // Attach the track to the bone
                ogreAnim->getNodeTrack( channel->mNodeName.data )->setAssociatedNode( bone );
            }
            */
            }
        }
#    endif
    //}

#endif

    bool AssimpLoader::getUseSingleMesh() const
    {
        return m_useSingleMesh;
    }

    void AssimpLoader::setUseSingleMesh( bool useSingleMesh )
    {
        m_useSingleMesh = useSingleMesh;
    }

    bool AssimpLoader::getQuietMode() const
    {
        return m_quietMode;
    }

    void AssimpLoader::setQuietMode( bool quietMode )
    {
        m_quietMode = quietMode;
    }

    SmartPtr<scene::MeshResourceDirector> AssimpLoader::getMeshDirector() const
    {
        return m_meshDirector;
    }

    void AssimpLoader::setMeshDirector( SmartPtr<scene::MeshResourceDirector> meshDirector )
    {
        m_meshDirector = meshDirector;
    }

    SmartPtr<IMeshResource> AssimpLoader::getMeshResource() const
    {
        return m_meshResource;
    }

    void AssimpLoader::setMeshResource( SmartPtr<IMeshResource> meshResource )
    {
        m_meshResource = meshResource;
    }
}  // namespace workphone
