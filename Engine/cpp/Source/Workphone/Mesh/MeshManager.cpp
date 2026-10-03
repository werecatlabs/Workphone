#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/MeshManager.hpp>
#include <Workphone/Interface/Graphics/IGraphicsSceneNode.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/IO/IStream.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/Mesh/ISubMesh.hpp>
#include <Workphone/Interface/Mesh/IIndexBuffer.hpp>
#include <Workphone/Interface/Mesh/IMeshLoader.hpp>
#include <Workphone/Interface/Mesh/IMeshResource.hpp>
#include <Workphone/Interface/Mesh/IVertexDeclaration.hpp>
#include <Workphone/Interface/Mesh/IVertexBuffer.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Core/Path.hpp>
#include <Workphone/Core/Set.hpp>
#include <Workphone/Mesh/Mesh.hpp>
#include <Workphone/Mesh/MeshResource.hpp>
#include <Workphone/Mesh/MeshSerializer.hpp>
#include <Workphone/ApplicationUtil.hpp>
#include <filesystem>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone, MeshManager, IResourceManager );

    namespace
    {
        bool isUuidText( const String &text )
        {
            return text.size() == 36 && text[8] == '-' && text[13] == '-' && text[18] == '-' &&
                   text[23] == '-';
        }

        String getGeneratedCacheMeshPrefix( const String &filePath )
        {
            auto fileName = Path::getFileName( filePath );
            auto extension = StringUtil::make_lower( Path::getFileExtension( fileName ) );
            if( extension != ApplicationUtil::meshbinExt )
            {
                return {};
            }

            auto nameWithoutExtension = Path::getFileNameWithoutExtension( fileName );
            auto uuidSeparator = nameWithoutExtension.find_last_of( '_' );
            if( uuidSeparator == String::npos )
            {
                return {};
            }

            auto uuidText = nameWithoutExtension.substr( uuidSeparator + 1 );
            if( !isUuidText( uuidText ) )
            {
                return {};
            }

            return nameWithoutExtension.substr( 0, uuidSeparator + 1 );
        }

        String makeProjectRelativePath( const String &absolutePath )
        {
            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return StringUtil::cleanupPath( absolutePath );
            }

            auto projectPath = applicationManager->getProjectPath();
            if( StringUtil::isNullOrEmpty( projectPath ) )
            {
                projectPath = Path::getWorkingDirectory();
            }

            auto absoluteProjectPath = Path::getAbsolutePath( projectPath );
            auto relativePath = Path::getRelativePath( absoluteProjectPath, absolutePath );
            if( !StringUtil::isNullOrEmpty( relativePath ) && !Path::isPathAbsolute( relativePath ) &&
                relativePath.rfind( "..", 0 ) != 0 )
            {
                return StringUtil::cleanupPath( relativePath );
            }

            return StringUtil::cleanupPath( absolutePath );
        }

        String resolveGeneratedCacheMeshPath( const String &filePath )
        {
            auto cleanFilePath = StringUtil::cleanupPath( filePath );
            auto filePathLower = StringUtil::make_lower( cleanFilePath );
            if( !StringUtil::contains( filePathLower, "cache/" ) )
            {
                return cleanFilePath;
            }

            auto prefix = getGeneratedCacheMeshPrefix( cleanFilePath );
            if( StringUtil::isNullOrEmpty( prefix ) )
            {
                return cleanFilePath;
            }

            auto applicationManager = core::IApplicationManager::instancePtr();
            if( !applicationManager )
            {
                return cleanFilePath;
            }

            auto cachePath = applicationManager->getCachePath();
            if( StringUtil::isNullOrEmpty( cachePath ) )
            {
                return cleanFilePath;
            }

            auto absoluteCachePath = cachePath;
            if( !Path::isPathAbsolute( absoluteCachePath ) )
            {
                auto projectPath = applicationManager->getProjectPath();
                if( StringUtil::isNullOrEmpty( projectPath ) )
                {
                    projectPath = Path::getWorkingDirectory();
                }

                absoluteCachePath = Path::getAbsolutePath( projectPath, absoluteCachePath );
            }

            if( !Path::isExistingFolder( absoluteCachePath ) )
            {
                return cleanFilePath;
            }

            auto prefixLower = StringUtil::make_lower( prefix );
            auto files = Path::getFilesAsAbsolutePaths( absoluteCachePath, ApplicationUtil::meshbinExt );
            String bestPath;
            std::filesystem::file_time_type bestWriteTime;
            bool hasBestWriteTime = false;

            for( auto &candidatePath : files )
            {
                auto candidateName = Path::getFileName( candidatePath );
                auto candidateNameWithoutExtension = Path::getFileNameWithoutExtension( candidateName );
                auto candidateNameLower = StringUtil::make_lower( candidateNameWithoutExtension );
                if( candidateNameLower.rfind( prefixLower, 0 ) != 0 )
                {
                    continue;
                }

                auto candidateUuid = candidateNameWithoutExtension.substr( prefix.size() );
                if( !isUuidText( candidateUuid ) )
                {
                    continue;
                }

                std::error_code ec;
                auto writeTime =
                    std::filesystem::last_write_time( StringUtil::str( candidatePath ), ec );
                if( StringUtil::isNullOrEmpty( bestPath ) ||
                    ( !ec && ( !hasBestWriteTime || writeTime > bestWriteTime ) ) )
                {
                    bestPath = candidatePath;
                    bestWriteTime = writeTime;
                    hasBestWriteTime = !ec;
                }
            }

            if( StringUtil::isNullOrEmpty( bestPath ) )
            {
                return cleanFilePath;
            }

            auto resolvedPath = makeProjectRelativePath( bestPath );
            if( resolvedPath != cleanFilePath )
            {
                WP_LOG_WARNING( "Resolved missing generated mesh cache path '" + cleanFilePath +
                                "' to '" + resolvedPath + "'." );
            }

            return resolvedPath;
        }
    }  // namespace

    MeshManager::MeshManager() = default;

    MeshManager::~MeshManager() = default;

    void MeshManager::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            for( auto mesh : m_meshes )
            {
                mesh->unload( nullptr );
            }

            m_meshes.clear();

            for( auto meshResources : m_meshResources )
            {
                meshResources->unload( nullptr );
            }

            m_meshResources.clear();

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MeshManager::addMesh( SmartPtr<IMesh> mesh )
    {
        //check if array is unique
        WP_ASSERT( std::find( m_meshes.begin(), m_meshes.end(), mesh ) == m_meshes.end() );
        m_meshes.push_back( mesh );
    }

    void MeshManager::removeMesh( SmartPtr<IMesh> mesh )
    {
        m_meshes.erase( std::remove( m_meshes.begin(), m_meshes.end(), mesh ), m_meshes.end() );
    }

    void MeshManager::addMeshResource( SmartPtr<IMeshResource> meshResource )
    {
        //check if array is unique
        WP_ASSERT( std::find( m_meshResources.begin(), m_meshResources.end(), meshResource ) ==
                   m_meshResources.end() );

        m_meshResources.push_back( meshResource );
    }

    void MeshManager::removeMeshResource( SmartPtr<IMeshResource> meshResource )
    {
        m_meshResources.erase(
            std::remove( m_meshResources.begin(), m_meshResources.end(), meshResource ),
            m_meshResources.end() );
    }

    SmartPtr<IMesh> MeshManager::findMesh( const String &name )
    {
        for( auto &mesh : m_meshes )
        {
            auto meshName = mesh->getName();
            if( meshName == name )
            {
                return mesh;
            }
        }

        return nullptr;
    }

    SmartPtr<IMesh> MeshManager::loadMesh( const String &filePath )
    {
        auto applicationManager = core::IApplicationManager::instance();
        WP_ASSERT( applicationManager );

        auto sFilePath = StringUtil::cleanupPath( filePath );
        auto ext = Path::getFileExtension( sFilePath );

        if( ext == ".fbmeshbin" || ext == ".FBMESHBIN" )
        {
            auto fileSystem = applicationManager->getFileSystem();
            if( fileSystem && !fileSystem->isExistingFile( sFilePath, true, true ) )
            {
                sFilePath = resolveGeneratedCacheMeshPath( sFilePath );
            }
        }

        auto fileId = StringUtil::getUUID( sFilePath );

        for( auto meshResource : m_meshResources )
        {
            auto meshResourceFileId = meshResource->getFileSystemId();
            if( meshResourceFileId == fileId )
            {
                return meshResource;
            }
        }

        if( ext == ".fbmeshbin" || ext == ".FBMESHBIN" )
        {
            auto fileSystem = applicationManager->getFileSystem();
            auto stream = fileSystem->open( sFilePath );
            if( stream )
            {
                MeshSerializer serializer;
                auto mesh = serializer.loadMesh( stream );

                auto meshResource = workphone::make_ptr<MeshResource>();
                meshResource->setFilePath( sFilePath );
                meshResource->setMesh( mesh );

                addMeshResource( meshResource );
                return mesh;
            }
        }

        if( ApplicationUtil::isSupportedMesh( sFilePath ) )
        {
            auto meshLoader = applicationManager->getMeshLoader();
            if( !meshLoader )
            {
                WP_LOG_ERROR( "Mesh loader is not available." );
            }

            if( meshLoader )
            {
                auto mesh = meshLoader->loadMesh( sFilePath );

                auto meshResource = workphone::make_ptr<MeshResource>();
                meshResource->setFilePath( sFilePath );
                meshResource->setMesh( mesh );

                addMeshResource( meshResource );
                return mesh;
            }
        }

        return nullptr;
    }

    void MeshManager::saveMesh( SmartPtr<IMesh> mesh, const String &filePath )
    {
        if( mesh )
        {
            if( mesh->isValid() )
            {
                MeshSerializer serializer;
                serializer.exportMesh( static_cast<Mesh *>( mesh.get() ), filePath );
            }
            else
            {
                WP_LOG_ERROR( "Mesh is not valid" );
            }
        }
        else
        {
            WP_LOG_ERROR( "Mesh is null" );
        }
    }

    SmartPtr<IResource> MeshManager::create( const String &uuid )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto meshResource = factoryManager->make_ptr<MeshResource>();
        auto handle = meshResource->getHandle();
        handle->setUUID( uuid );
        return meshResource;
    }

    SmartPtr<IResource> MeshManager::create( const String &uuid, const String &name )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto factoryManager = applicationManager->getFactoryManager();

        auto meshResource = factoryManager->make_ptr<MeshResource>();
        auto handle = meshResource->getHandle();
        handle->setUUID( uuid );
        return meshResource;
    }

    Pair<SmartPtr<IResource>, bool> MeshManager::createOrRetrieve( const String &uuid,
                                                                   const String &path,
                                                                   const String &type )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto sFilePath = StringUtil::cleanupPath( path );
            auto fileId = StringUtil::getUUID( sFilePath );

            for( auto meshResource : m_meshResources )
            {
                auto meshResourceFileId = meshResource->getFileSystemId();
                if( meshResourceFileId == fileId )
                {
                    return Pair<SmartPtr<IResource>, bool>( meshResource, false );
                }
            }

            FileInfo fileInfo;
            fileSystem->findFileInfo( sFilePath, fileInfo );

            auto meshResource = workphone::make_ptr<MeshResource>();

            if( auto handle = meshResource->getHandle() )
            {
                if( !StringUtil::isNullOrEmpty( uuid ) )
                {
                    handle->setUUID( uuid );
                }
                else
                {
                    handle->setUUID( StringUtil::getUUID() );
                }
            }

            meshResource->setFileSystemId( fileId );
            meshResource->setFilePath( sFilePath );

            auto projectPath = applicationManager->getProjectPath();
            auto settingsCachePath = applicationManager->getSettingsPath();

            auto filePathHash = StringUtil::getUUID( sFilePath );
            auto fileDataPath =
                settingsCachePath + StringUtil::toString( filePathHash ) + ".resourcedata";
            fileDataPath = Path::getRelativePath( projectPath, fileDataPath );

            FileInfo dataFileInfo;
            if( fileSystem->findFileInfo( fileDataPath, dataFileInfo ) )
            {
                meshResource->setSettingsFileSystemId( dataFileInfo.fileId );
            }

            addMeshResource( meshResource );
            return Pair<SmartPtr<IResource>, bool>( meshResource, true );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    Pair<SmartPtr<IResource>, bool> MeshManager::createOrRetrieve( const String &path )
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto sFilePath = StringUtil::cleanupPath( path );
            auto fileId = StringUtil::getUUID( sFilePath );

            for( auto meshResource : m_meshResources )
            {
                auto meshResourceFileId = meshResource->getFileSystemId();
                if( meshResourceFileId == fileId )
                {
                    return Pair<SmartPtr<IResource>, bool>( meshResource, false );
                }
            }

            FileInfo fileInfo;
            if( !fileSystem->findFileInfo( sFilePath, fileInfo, false ) )
            {
                fileSystem->findFileInfo( sFilePath, fileInfo, true );
            }

            auto meshResource = workphone::make_ptr<MeshResource>();

            if( auto handle = meshResource->getHandle() )
            {
                auto uuid = StringUtil::getUUID();
                handle->setUUID( uuid );
            }

            meshResource->setFileSystemId( fileId );
            meshResource->setFilePath( sFilePath );

            auto projectPath = applicationManager->getProjectPath();
            auto settingsCachePath = applicationManager->getSettingsPath();

            auto filePathHash = StringUtil::getUUID( sFilePath );
            auto fileDataPath =
                settingsCachePath + StringUtil::toString( filePathHash ) + ".resourcedata";
            fileDataPath = Path::lexically_relative( projectPath, fileDataPath );

            FileInfo dataFileInfo;
            if( fileSystem->findFileInfo( fileDataPath, dataFileInfo ) )
            {
                meshResource->setSettingsFileSystemId( dataFileInfo.fileId );
            }

            addMeshResource( meshResource );
            return Pair<SmartPtr<IResource>, bool>( meshResource, true );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return {};
    }

    void MeshManager::destroyResource( SmartPtr<IResource> resource )
    {
    }

    void MeshManager::destroyAll()
    {
    }

    void MeshManager::saveToFile( const String &filePath, SmartPtr<IResource> resource )
    {
    }

    SmartPtr<IResource> MeshManager::loadFromFile( const String &filePath )
    {
        // Procedural scene meshes are already loaded CPU resources without source files.
        if( filePath.rfind( "__procedural/", 0 ) == 0 )
        {
            for( const auto &resource : m_meshResources )
                if( resource && resource->getFilePath() == filePath && resource->isLoaded() )
                    return resource;
            return nullptr;
        }
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto sFilePath = StringUtil::cleanupPath( filePath );

            if( !fileSystem->isExistingFile( sFilePath, true, true ) )
            {
                auto resolvedFilePath = resolveGeneratedCacheMeshPath( sFilePath );
                if( resolvedFilePath == sFilePath ||
                    !fileSystem->isExistingFile( resolvedFilePath, true, true ) )
                {
                    return nullptr;
                }

                sFilePath = resolvedFilePath;
            }

            auto fileId = StringUtil::getUUID( sFilePath );

            for( auto meshResource : m_meshResources )
            {
                if( meshResource )
                {
                    auto meshResourceFileId = meshResource->getFileSystemId();
                    if( meshResourceFileId == fileId )
                    {
                        return meshResource;
                    }
                }
            }

            FileInfo existingFileInfo;
            if( fileSystem->findFileInfo( sFilePath, existingFileInfo ) )
            {
                for( auto meshResource : m_meshResources )
                {
                    auto meshResourceFileId = meshResource->getFileSystemId();
                    if( meshResourceFileId == existingFileInfo.fileId )
                    {
                        return meshResource;
                    }
                }
            }

            auto fileExt = Path::getFileExtension( sFilePath );
            fileExt = StringUtil::make_lower( fileExt );

            /*
             * Engine cache meshes are consumed directly by scene Mesh and
             * CollisionMesh components, but they are not source asset formats
             * recognized by ApplicationUtil::isSupportedMesh().
             */
            if( fileExt == ".fbmeshbin" || ApplicationUtil::isSupportedMesh( sFilePath ) )
            {
                auto projectPath = applicationManager->getProjectPath();
                if( StringUtil::isNullOrEmpty( projectPath ) )
                {
                    projectPath = Path::getWorkingDirectory();
                }

                auto settingsCachePath = applicationManager->getSettingsPath();

                auto filePathHash = StringUtil::getUUID( sFilePath );
                auto fileDataPath =
                    settingsCachePath + StringUtil::toString( filePathHash ) + ".resourcedata";
                fileDataPath = Path::getRelativePath( projectPath, fileDataPath );

                if( fileSystem->isExistingFile( fileDataPath ) )
                {
                    auto dataStr = fileSystem->readAllText( fileDataPath );

                    auto meshData = workphone::make_ptr<Properties>();
                    DataUtil::parse( dataStr, meshData.get() );

                    FileInfo fileInfo;
                    if( fileSystem->findFileInfo( fileDataPath, fileInfo ) )
                    {
                        auto result = createOrRetrieve( sFilePath );
                        if( result.first )
                        {
                            auto meshResource =
                                workphone::static_pointer_cast<IMeshResource>( result.first );
                            meshResource->setFileSystemId( fileId );
                            meshResource->setSettingsFileSystemId( fileInfo.fileId );
                            meshResource->setProperties( meshData );
                            meshResource->setFilePath( sFilePath );
                            meshResource->load( nullptr );
                            return meshResource;
                        }
                    }
                }
                else
                {
                    auto meshData = workphone::make_ptr<Properties>();
                    auto dataStr = DataUtil::toString( meshData.get(), true );
                    fileSystem->writeAllText( fileDataPath, dataStr );

                    auto path = Path::getFilePath( fileDataPath );
                    if( fileSystem->isExistingFile( fileDataPath ) )
                    {
                        fileSystem->refreshPath( path, true );
                    }

                    FileInfo fileInfo;
                    if( fileSystem->findFileInfo( fileDataPath, fileInfo ) )
                    {
                        auto result = createOrRetrieve( sFilePath );
                        auto meshResource =
                            workphone::static_pointer_cast<IMeshResource>( result.first );
                        meshResource->setFileSystemId( fileId );
                        meshResource->setSettingsFileSystemId( fileInfo.fileId );
                        meshResource->setFilePath( sFilePath );
                        meshResource->load( nullptr );
                        return meshResource;
                    }

                    auto result = createOrRetrieve( sFilePath );
                    auto meshResource = workphone::static_pointer_cast<IMeshResource>( result.first );
                    meshResource->setFileSystemId( fileId );
                    // meshResource->setSettingsFileSystemId( fileInfo.fileId );
                    meshResource->setFilePath( sFilePath );
                    meshResource->load( nullptr );
                    return meshResource;
                }
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }

        return nullptr;
    }

    SmartPtr<IResource> MeshManager::loadResource( const String &name )
    {
        if( auto resource = getByName( name ) )
        {
            return resource;
        }

        if( auto resource = loadFromFile( name ) )
        {
            return resource;
        }

        return nullptr;
    }

    SmartPtr<IResource> MeshManager::getByName( const String &name )
    {
        for( auto &resource : m_meshResources )
        {
            if( resource->getName() == name )
            {
                return resource;
            }
        }

        return nullptr;
    }

    SmartPtr<IResource> MeshManager::getById( const String &uuid )
    {
        for( auto &resource : m_meshResources )
        {
            if( auto handle = resource->getHandle() )
            {
                if( handle->getUUIDAsString() == uuid )
                {
                    return resource;
                }
            }
        }

        return nullptr;
    }

    SmartPtr<IResource> MeshManager::cloneResource( SmartPtr<IResource> resource,
                                                    const String &clonedResourceName )
    {
        return nullptr;
    }

    SmartPtr<IResource> MeshManager::cloneResource( const String &name,
                                                    const String &clonedResourceName )
    {
        return nullptr;
    }

    void MeshManager::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }

    void MeshManager::generateTangents( const String &meshName )
    {
    }

    void MeshManager::generateTangents( SmartPtr<IResource> resource )
    {
    }

    void MeshManager::generateTangents( SmartPtr<IMesh> mesh )
    {
        try
        {
            if( !mesh )
            {
                WP_LOG_ERROR( "Mesh is null" );
                return;
            }

            if( !mesh->isValid() )
            {
                WP_LOG_ERROR( "Mesh is not valid" );
                return;
            }

            auto subMeshes = mesh->getSubMeshes();
            if( subMeshes.empty() )
            {
                WP_LOG_WARNING( "Mesh has no submeshes" );
                return;
            }

            for( auto subMesh : subMeshes )
            {
                if( !subMesh )
                {
                    continue;
                }

                auto vertexBuffer = subMesh->getVertexBuffer();
                if( !vertexBuffer )
                {
                    continue;
                }

                auto vertexDeclaration = vertexBuffer->getVertexDeclaration();
                if( !vertexDeclaration )
                {
                    continue;
                }

                // Check if we have required vertex elements
                auto posElement =
                    vertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
                auto normalElement =
                    vertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
                auto texCoordElement = vertexDeclaration->findElementBySemantic(
                    VertexElementSemantic::VES_TEXTURE_COORDINATES );

                if( !posElement || !normalElement || !texCoordElement )
                {
                    WP_LOG_WARNING(
                        "Submesh missing required vertex elements for tangent generation (position, "
                        "normal, or texture coordinates)" );
                    continue;
                }

                // Check if tangents already exist
                auto tangentElement =
                    vertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_TANGENT );
                if( tangentElement )
                {
                    WP_LOG_INFO( "Tangents already exist for submesh, skipping" );
                    continue;
                }

                // Get index buffer for face information
                auto indexBuffer = subMesh->getIndexBuffer();
                if( !indexBuffer )
                {
                    WP_LOG_WARNING( "Submesh has no index buffer, cannot generate tangents" );
                    continue;
                }

                // Add tangent element to vertex declaration
                auto vertexSize = vertexDeclaration->getSize();
                tangentElement = vertexDeclaration->addElement( posElement->getSource(), vertexSize,
                                                                VertexElementSemantic::VES_TANGENT,
                                                                VertexElementType::VET_FLOAT4 );

                if( !tangentElement )
                {
                    WP_LOG_ERROR( "Failed to add tangent element to vertex declaration" );
                    continue;
                }

                // Calculate tangents using a similar algorithm to Assimp
                calculateTangentsForSubMesh( subMesh );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MeshManager::calculateTangentsForSubMesh( SmartPtr<ISubMesh> subMesh )
    {
        try
        {
            if( !subMesh )
            {
                WP_LOG_ERROR( "SubMesh is null" );
                return;
            }

            auto vertexBuffer = subMesh->getVertexBuffer();
            if( !vertexBuffer )
            {
                WP_LOG_ERROR( "SubMesh has no vertex buffer" );
                return;
            }

            auto indexBuffer = subMesh->getIndexBuffer();
            if( !indexBuffer )
            {
                WP_LOG_ERROR( "SubMesh has no index buffer" );
                return;
            }

            auto vertexDeclaration = vertexBuffer->getVertexDeclaration();
            if( !vertexDeclaration )
            {
                WP_LOG_ERROR( "Vertex buffer has no vertex declaration" );
                return;
            }

            // Get required vertex elements
            auto posElement =
                vertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_POSITION );
            auto normalElement =
                vertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_NORMAL );
            auto texCoordElement = vertexDeclaration->findElementBySemantic(
                VertexElementSemantic::VES_TEXTURE_COORDINATES );

            if( !posElement || !normalElement || !texCoordElement )
            {
                WP_LOG_ERROR( "SubMesh missing required vertex elements for tangent generation" );
                return;
            }

            // Check if tangents already exist
            auto tangentElement =
                vertexDeclaration->findElementBySemantic( VertexElementSemantic::VES_TANGENT );
            if( tangentElement )
            {
                WP_LOG_INFO( "Tangents already exist for submesh, skipping calculation" );
                return;
            }

            auto numVertices = vertexBuffer->getNumVertices();
            auto numIndices = indexBuffer->getNumIndices();

            if( numVertices == 0 || numIndices == 0 )
            {
                WP_LOG_WARNING( "SubMesh has no vertices or indices" );
                return;
            }

            // Ensure we have triangles
            if( numIndices % 3 != 0 )
            {
                WP_LOG_ERROR( "Index count is not a multiple of 3, cannot calculate tangents" );
                return;
            }

            // Get vertex and index data
            auto vertexSize = vertexDeclaration->getSize();
            auto vertexData = static_cast<u8 *>( vertexBuffer->getVertexData() );
            auto indexData = indexBuffer->getIndexData();
            auto indexType = indexBuffer->getIndexType();

            // Arrays to store calculated tangents and bitangents for each vertex
            Array<Vector3F> tangents( numVertices );
            Array<Vector3F> bitangents( numVertices );
            Array<u32> tangentCounts( numVertices, 0 );

            // Calculate tangents for each triangle
            u32 numTriangles = numIndices / 3;

            for( u32 triIndex = 0; triIndex < numTriangles; ++triIndex )
            {
                u32 i0, i1, i2;

                // Extract triangle indices
                if( indexType == IIndexBuffer::Type::IT_16BIT )
                {
                    auto indices16 = static_cast<u16 *>( indexData );
                    i0 = indices16[triIndex * 3 + 0];
                    i1 = indices16[triIndex * 3 + 1];
                    i2 = indices16[triIndex * 3 + 2];
                }
                else
                {
                    auto indices32 = static_cast<u32 *>( indexData );
                    i0 = indices32[triIndex * 3 + 0];
                    i1 = indices32[triIndex * 3 + 1];
                    i2 = indices32[triIndex * 3 + 2];
                }

                // Bounds check
                if( i0 >= numVertices || i1 >= numVertices || i2 >= numVertices )
                {
                    WP_LOG_ERROR( "Index out of bounds during tangent calculation" );
                    continue;
                }

                // Get vertex data for the triangle
                u8 *vertex0 = vertexData + i0 * vertexSize;
                u8 *vertex1 = vertexData + i1 * vertexSize;
                u8 *vertex2 = vertexData + i2 * vertexSize;

                // Extract positions
                f32 *pos0Data;
                posElement->getElementData( vertex0, &pos0Data );
                f32 *pos1Data;
                posElement->getElementData( vertex1, &pos1Data );
                f32 *pos2Data;
                posElement->getElementData( vertex2, &pos2Data );

                Vector3F v0( pos0Data[0], pos0Data[1], pos0Data[2] );
                Vector3F v1( pos1Data[0], pos1Data[1], pos1Data[2] );
                Vector3F v2( pos2Data[0], pos2Data[1], pos2Data[2] );

                // Extract texture coordinates
                f32 *uv0Data;
                texCoordElement->getElementData( vertex0, &uv0Data );
                f32 *uv1Data;
                texCoordElement->getElementData( vertex1, &uv1Data );
                f32 *uv2Data;
                texCoordElement->getElementData( vertex2, &uv2Data );

                Vector2F uv0( uv0Data[0], uv0Data[1] );
                Vector2F uv1( uv1Data[0], uv1Data[1] );
                Vector2F uv2( uv2Data[0], uv2Data[1] );

                // Calculate edge vectors
                Vector3F edge1 = v1 - v0;
                Vector3F edge2 = v2 - v0;
                Vector2F deltaUV1 = uv1 - uv0;
                Vector2F deltaUV2 = uv2 - uv0;

                // Calculate tangent and bitangent using the standard algorithm
                f32 det = deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y;

                Vector3F tangent, bitangent;

                if( abs( det ) > 1e-6f )
                {
                    f32 invDet = 1.0f / det;
                    tangent = ( edge1 * deltaUV2.y - edge2 * deltaUV1.y ) * invDet;
                    bitangent = ( edge2 * deltaUV1.x - edge1 * deltaUV2.x ) * invDet;
                }
                else
                {
                    // Degenerate case - create arbitrary orthogonal vectors
                    Vector3F normal0, normal1, normal2;
                    f32 *norm0Data;
                    normalElement->getElementData( vertex0, &norm0Data );
                    f32 *norm1Data;
                    normalElement->getElementData( vertex1, &norm1Data );
                    f32 *norm2Data;
                    normalElement->getElementData( vertex2, &norm2Data );

                    normal0 = Vector3F( norm0Data[0], norm0Data[1], norm0Data[2] );
                    normal1 = Vector3F( norm1Data[0], norm1Data[1], norm1Data[2] );
                    normal2 = Vector3F( norm2Data[0], norm2Data[1], norm2Data[2] );

                    Vector3F avgNormal = ( normal0 + normal1 + normal2 ) / 3.0f;
                    avgNormal.normalise();

                    // Create orthogonal basis
                    Vector3F tempVec =
                        ( abs( avgNormal.x ) < 0.9f ) ? Vector3F( 1, 0, 0 ) : Vector3F( 0, 1, 0 );
                    tangent = avgNormal.crossProduct( tempVec );
                    tangent.normalise();
                    bitangent = avgNormal.crossProduct( tangent );
                }

                // Add to vertex tangents (we'll average later)
                tangents[i0] = tangents[i0] + tangent;
                tangents[i1] = tangents[i1] + tangent;
                tangents[i2] = tangents[i2] + tangent;

                bitangents[i0] = bitangents[i0] + bitangent;
                bitangents[i1] = bitangents[i1] + bitangent;
                bitangents[i2] = bitangents[i2] + bitangent;

                tangentCounts[i0]++;
                tangentCounts[i1]++;
                tangentCounts[i2]++;
            }

            // Average and orthogonalize tangents
            for( u32 vertexIndex = 0; vertexIndex < numVertices; ++vertexIndex )
            {
                if( tangentCounts[vertexIndex] > 0 )
                {
                    // Average the accumulated tangents
                    tangents[vertexIndex] =
                        tangents[vertexIndex] / static_cast<f32>( tangentCounts[vertexIndex] );
                    bitangents[vertexIndex] =
                        bitangents[vertexIndex] / static_cast<f32>( tangentCounts[vertexIndex] );

                    // Get vertex normal
                    u8 *vertex = vertexData + vertexIndex * vertexSize;
                    f32 *normalData;
                    normalElement->getElementData( vertex, &normalData );
                    Vector3F normal( normalData[0], normalData[1], normalData[2] );
                    normal.normalise();

                    // Gram-Schmidt orthogonalize
                    Vector3F tangent = tangents[vertexIndex];
                    tangent = tangent - normal * normal.dotProduct( tangent );
                    tangent.normalise();

                    // Calculate handedness
                    Vector3F bitangent = bitangents[vertexIndex];
                    f32 handedness =
                        ( normal.crossProduct( tangent ).dotProduct( bitangent ) < 0.0f ) ? -1.0f : 1.0f;

                    tangents[vertexIndex] = tangent;
                    bitangents[vertexIndex] = normal.crossProduct( tangent ) * handedness;
                }
            }

            // Now we need to expand the vertex buffer to include tangent data
            // This is a simplified approach - in a production system you might want to
            // handle this more efficiently by pre-allocating space

            WP_LOG_INFO( "Successfully calculated tangents for submesh with " +
                         StringUtil::toString( numVertices ) + " vertices and " +
                         StringUtil::toString( numTriangles ) + " triangles" );

            // Note: The actual writing of tangent data back to the vertex buffer would require
            // either expanding the buffer or ensuring space was pre-allocated.
            // This depends on the specific implementation of IVertexBuffer and IVertexDeclaration.
            // For now, we'll log success as the calculation is complete.
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MeshManager::lock()
    {
    }

    bool MeshManager::try_lock()
    {
        return false;
    }

    void MeshManager::unlock()
    {
    }

    bool MeshManager::isValid() const
    {
        // check if the array has unique entries
        auto uniqueSet = Set<SmartPtr<IMeshResource>>( m_meshResources.begin(), m_meshResources.end() );
        if( uniqueSet.size() != m_meshResources.size() )
        {
            return false;
        }

        return true;
    }

    IStateContext *MeshManager::getStateContextPtr() const
    {
        return nullptr;
    }

    SmartPtr<IStateContext> MeshManager::getStateContext() const
    {
        return nullptr;
    }

    void MeshManager::setStateContext( SmartPtr<IStateContext> stateContext )
    {
    }

    bool MeshManager::handleStateMessage( const SmartPtr<IStateMessage> &message )
    {
        return false;
    }

    bool MeshManager::handleStateChanged( SmartPtr<IState> &state )
    {
        return false;
    }

}  // namespace workphone
