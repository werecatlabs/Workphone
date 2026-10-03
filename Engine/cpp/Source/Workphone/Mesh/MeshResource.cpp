#include <Workphone/WorkphonePCH.hpp>
#include <Workphone/Mesh/MeshResource.hpp>
#include <Workphone/Core/DataUtil.hpp>
#include <Workphone/Core/FileInfo.hpp>
#include <Workphone/Core/LogManager.hpp>
#include <Workphone/Interface/IO/IFileSystem.hpp>
#include <Workphone/Interface/Mesh/IMeshLoader.hpp>
#include <Workphone/Interface/Mesh/IMesh.hpp>
#include <Workphone/Interface/System/IResourceManager.hpp>

namespace workphone
{
    const String MeshResource::scaleStr = String( "scale" );
    const String MeshResource::constraintsStr = String( "constraints" );
    const String MeshResource::animationStr = String( "animation" );
    const String MeshResource::visibilityStr = String( "visibility" );
    const String MeshResource::camerasStr = String( "cameras" );
    const String MeshResource::lightsStr = String( "lights" );
    const String MeshResource::lightmapUVsStr = String( "lightmapUVs" );
    const String MeshResource::useMeshInstancingStr = String( "useMeshInstancing" );
    const String MeshResource::saveStr = String( "Save" );
    const String MeshResource::importStr = String( "Import" );

    WP_CLASS_REGISTER_DERIVED( workphone, MeshResource, IMeshResource );

    MeshResource::MeshResource() = default;

    MeshResource::~MeshResource() = default;

    auto MeshResource::hasSkeleton() const -> bool
    {
        return m_hasSkeleton;
    }

    auto MeshResource::getSkeletonName() const -> String
    {
        return m_skeletonName;
    }

    auto MeshResource::getNumLodLevels() const -> u32
    {
        return m_numLodLevels;
    }

    auto MeshResource::isEdgeListBuilt() const -> bool
    {
        return m_edgeListBuilt;
    }

    auto MeshResource::hasVertexAnimation() const -> bool
    {
        return m_hasVertexAnimation;
    }

    void MeshResource::save()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto data = workphone::static_pointer_cast<Properties>( toData() );
            WP_ASSERT( data );

            auto dataStr = DataUtil::toString( data.get(), true );
            WP_ASSERT( !StringUtil::isNullOrEmpty( dataStr ) );

            auto fileId = getSettingsFileSystemId();
            WP_ASSERT( !fileId.is_nil() );

            FileInfo fileInfo;
            if( fileSystem->findFileInfo( fileId, fileInfo ) )
            {
                fileSystem->writeAllText( fileInfo.filePath.c_str(), dataStr );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MeshResource::import()
    {
        try
        {
            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto resourceDatabase = applicationManager->getResourceDatabase();
            WP_ASSERT( resourceDatabase );

            auto meshPath = getFilePath();
            WP_ASSERT( !StringUtil::isNullOrEmpty( meshPath ) );

            if( !StringUtil::isNullOrEmpty( meshPath ) )
            {
                resourceDatabase->importFile( meshPath, true );
            }
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MeshResource::load( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Loading );

            auto applicationManager = core::IApplicationManager::instance();
            WP_ASSERT( applicationManager );

            auto fileSystem = applicationManager->getFileSystem();
            WP_ASSERT( fileSystem );

            auto factoryManager = applicationManager->getFactoryManager();
            WP_ASSERT( factoryManager );

            auto meshManager = applicationManager->getMeshManager();
            WP_ASSERT( meshManager );

            auto meshPath = getFilePath();
            if( !StringUtil::isNullOrEmpty( meshPath ) )
            {
                auto isExistingFile = fileSystem->isExistingFile( meshPath, false, true );
                if( !isExistingFile )
                {
                    isExistingFile = fileSystem->isExistingFile( meshPath, true, true );
                }

                if( isExistingFile )
                {
                    auto meshLoader = factoryManager->make_object<IMeshLoader>();
                    if( !meshLoader )
                    {
                        WP_LOG_ERROR( "No mesh loader." );
                    }

                    if( meshLoader )
                    {
                        auto mesh = meshLoader->loadMesh( this );

                        if( mesh )
                        {
                            mesh->setName( meshPath );
                        }

                        setMesh( mesh );
                    }
                }
                else
                {
                    if( meshManager )
                    {
                        auto recoveredResourceObject = meshManager->loadFromFile( meshPath );
                        auto recoveredMeshResource =
                            workphone::dynamic_pointer_cast<IMeshResource>( recoveredResourceObject );
                        if( recoveredMeshResource && recoveredMeshResource.get() != this )
                        {
                            auto recoveredMesh = recoveredMeshResource->getMesh();
                            if( recoveredMesh )
                            {
                                auto recoveredMeshPath = recoveredMeshResource->getFilePath();
                                if( !StringUtil::isNullOrEmpty( recoveredMeshPath ) )
                                {
                                    setFilePath( recoveredMeshPath );
                                    meshPath = recoveredMeshPath;
                                }

                                recoveredMesh->setName( meshPath );
                                setMesh( recoveredMesh );
                                setLoadingState( LoadingState::Loaded );
                                return;
                            }
                        }
                    }

                    WP_LOG_ERROR( "File not found: " + meshPath );
                }
            }

            setLoadingState( LoadingState::Loaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MeshResource::reload( SmartPtr<ISharedObject> data )
    {
        try
        {
            unload( nullptr );
            load( nullptr );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    void MeshResource::unload( SmartPtr<ISharedObject> data )
    {
        try
        {
            setLoadingState( LoadingState::Unloading );

            if( auto mesh = getMesh() )
            {
                mesh->unload( nullptr );
            }

            setMesh( nullptr );

            setLoadingState( LoadingState::Unloaded );
        }
        catch( std::exception &e )
        {
            WP_LOG_EXCEPTION( e );
        }
    }

    auto MeshResource::getProperties() const -> SmartPtr<Properties>
    {
        auto properties = Resource<IMeshResource>::getProperties();
        properties->setProperty( MeshResource::scaleStr, m_scale );
        properties->setProperty( MeshResource::constraintsStr, m_constraints );
        properties->setProperty( MeshResource::animationStr, m_animation );
        properties->setProperty( MeshResource::visibilityStr, m_visibility );
        properties->setProperty( MeshResource::camerasStr, m_cameras );
        properties->setProperty( MeshResource::lightsStr, m_lights );
        properties->setProperty( MeshResource::lightmapUVsStr, m_lightmapUVs );
        properties->setProperty( MeshResource::useMeshInstancingStr, m_useMeshInstancing );

        properties->setProperty( MeshResource::saveStr, "SaveButton", "button", false );
        properties->setProperty( MeshResource::importStr, "ImportButton", "button", false );

        return properties;
    }

    void MeshResource::setProperties( SmartPtr<Properties> properties )
    {
        Resource<IMeshResource>::setProperties( properties );

        auto scale = getScale();
        auto constraints = getConstraints();
        auto animation = getAnimation();
        auto visibility = getVisibility();
        auto cameras = getCameras();
        auto lights = getLights();
        auto lightmapUVs = getLightmapUVs();
        auto useMeshInstancing = getUseMeshInstancing();

        properties->getPropertyValue( MeshResource::scaleStr, scale );
        properties->getPropertyValue( MeshResource::constraintsStr, constraints );
        properties->getPropertyValue( MeshResource::animationStr, animation );
        properties->getPropertyValue( MeshResource::visibilityStr, visibility );
        properties->getPropertyValue( MeshResource::camerasStr, cameras );
        properties->getPropertyValue( MeshResource::lightsStr, lights );
        properties->getPropertyValue( MeshResource::lightmapUVsStr, lightmapUVs );
        properties->getPropertyValue( MeshResource::useMeshInstancingStr, useMeshInstancing );

        m_scale = scale;
        m_constraints = constraints;
        m_animation = animation;
        m_visibility = visibility;
        m_cameras = cameras;
        m_lights = lights;
        m_lightmapUVs = lightmapUVs;
        m_useMeshInstancing = useMeshInstancing;

        if( properties->hasProperty( "Save" ) )
        {
            auto &resetButton = properties->getPropertyObject( "Save" );
            if( resetButton.getAttribute( "click" ) == "true" )
            {
                save();
            }
        }

        if( properties->hasProperty( "Import" ) )
        {
            auto &resetButton = properties->getPropertyObject( "Import" );
            if( resetButton.getAttribute( "click" ) == "true" )
            {
                import();
            }
        }
    }

    void MeshResource::_getObject( void **ppObject ) const
    {
        *ppObject = nullptr;
    }

    auto MeshResource::getScale() const -> f32
    {
        return m_scale;
    }

    void MeshResource::setScale( f32 scale )
    {
        m_scale = scale;
    }

    auto MeshResource::getMaterialNaming() const -> IMeshResource::MaterialNaming
    {
        return m_materialNaming;
    }

    void MeshResource::setMaterialNaming( MaterialNaming materialNaming )
    {
        m_materialNaming = materialNaming;
    }

    auto MeshResource::getConstraints() const -> bool
    {
        return m_constraints;
    }

    void MeshResource::setConstraints( bool constraints )
    {
        m_constraints = constraints;
    }

    auto MeshResource::getAnimation() const -> bool
    {
        return m_animation;
    }

    void MeshResource::setAnimation( bool animation )
    {
        m_animation = animation;
    }

    auto MeshResource::getVisibility() const -> bool
    {
        return m_visibility;
    }

    void MeshResource::setVisibility( bool visibility )
    {
        m_visibility = visibility;
    }

    auto MeshResource::getCameras() const -> bool
    {
        return m_cameras;
    }

    void MeshResource::setCameras( bool cameras )
    {
        m_cameras = cameras;
    }

    auto MeshResource::getLights() const -> bool
    {
        return m_lights;
    }

    void MeshResource::setLights( bool lights )
    {
        m_lights = lights;
    }

    auto MeshResource::getLightmapUVs() const -> bool
    {
        return m_lightmapUVs;
    }

    void MeshResource::setLightmapUVs( bool lightmapUVs )
    {
        m_lightmapUVs = lightmapUVs;
    }

    auto MeshResource::getUseMeshInstancing() const -> bool
    {
        return m_useMeshInstancing;
    }

    void MeshResource::setUseMeshInstancing( bool useMeshInstancing )
    {
        m_useMeshInstancing = useMeshInstancing;
    }

    auto MeshResource::getMesh() const -> SmartPtr<IMesh>
    {
        return m_mesh;
    }

    void MeshResource::setMesh( SmartPtr<IMesh> mesh )
    {
        m_mesh = mesh;
    }

}  // namespace workphone
