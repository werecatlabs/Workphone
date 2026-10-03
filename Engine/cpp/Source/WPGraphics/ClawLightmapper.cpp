#include "WPGraphics/WPClawHammerPCH.hpp"
#include "ClawLightmapper.hpp"
#include "ClawLightmap.hpp"
#include <Workphone/Workphone.hpp>

namespace workphone
{
    WP_CLASS_REGISTER_DERIVED( workphone::render, Lightmapper, render::ILightmapper );

    Lightmapper::Lightmapper() :
        m_showDebugLightmaps( false ),
        m_lightmapAll( false ),
        m_textureSize( 256 ),
        m_autoTexSize( false ),
        m_startTime( 0 ),
        m_endTime( 0 )
    {
        LastGenerateRequest = 0;
        LastGenerate = 0;
        m_matCounter = 0;
        m_imageExtension = ".png";
    }

    Lightmapper::~Lightmapper()
    {
    }

    void Lightmapper::update()
    {
        generate( 0 );

        if( m_lightmapEntities.empty() )
        {
            WP_LOG( "Finished generating lightmaps" );

            auto applicationManager = core::IApplicationManager::instance();
            auto timer = applicationManager->getTimer();

            m_endTime = static_cast<u32>( timer->getTime() );
            WP_LOG( "Lightmapper time taken: " + StringUtil::toString( m_endTime - m_startTime ) );

            LastGenerate = LastGenerateRequest;

            if( m_matCounter > 0 )
            {
                m_matCounter = 0;
            }
        }
    }

    void Lightmapper::buildingEntityList( SmartPtr<scene::IGameActor> entity, const AABB3F &box,
                                          Array<SmartPtr<scene::IGameActor>> &entities )
    {
        Set<SmartPtr<scene::IGameActor>> entityPtrList;

        // Get entities within the bounding box
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();
        if( sceneManager )
        {
            // Query entities in the cell/region
            // sceneManager->getEntitiesInBounds( box, entityPtrList );
        }

        // Convert set to array and remove self
        for( auto &entityPtr : entityPtrList )
        {
            if( entityPtr != entity )
            {
                entities.push_back( entityPtr );
            }
        }
    }

    void Lightmapper::generate( int threadNumber )
    {
        SmartPtr<scene::IGameActor> entity;

        if( !m_lightmapEntities.empty() )
        {
            entity = m_lightmapEntities.back();
            m_lightmapEntities.pop_back();
        }

        if( entity )
        {
            lightMapEntity( entity );
        }
    }

    void Lightmapper::lightMapEntity( SmartPtr<scene::IGameActor> entity )
    {
        if( !entity )
        {
            return;
        }

        // Get mesh component from entity
        auto meshComponent = entity->getComponent<scene::Mesh>();
        if( !meshComponent )
        {
            return;
        }

        auto meshResource = meshComponent->getMeshResource();
        if( !meshResource )
        {
            return;
        }

        auto mesh = meshResource->getMesh();

        // Check if entity should receive shadows
        auto graphicsObject = entity->getComponent<scene::Renderer>();
        if( !graphicsObject )
        {
            return;
        }

        // if( !graphicsObject->getReceiveShadows() )
        //{
        //     return;
        // }

        // Get entities that may cast shadows onto this entity
        Array<SmartPtr<scene::IGameActor>> shadowCasters;

        auto worldAABB = scene::GameActorUtil::getActorAABB( entity );
        buildingEntityList( entity, worldAABB, shadowCasters );

        // Process each submesh
        auto subMeshes = mesh->getSubMeshes();
        for( u32 subMeshIdx = 0; subMeshIdx < subMeshes.size(); ++subMeshIdx )
        {
            auto subMesh = subMeshes[subMeshIdx];

            // Generate unique lightmap name
            String lightMapName = m_prefix;
            lightMapName += entity->getName();
            lightMapName += "_LightMap";
            lightMapName += StringUtil::toString( subMeshIdx );

            // Generate lightmap
            // auto lightmap = workphone::make_ptr<Lightmap>();
            // lightmap->initialize( entity, subMesh, lightMapName + m_imageExtension, shadowCasters,
            //                      m_textureSize, m_autoTexSize );

            // Create and assign material with lightmap
            // String curMaterialName = graphicsObject->getMaterialName( subMeshIdx );
            // createMaterial( curMaterialName, lightMapName );

            // graphicsObject->setMaterialName( lightMapName, subMeshIdx );
            // subMesh->setMaterialName( lightMapName );

            ++m_matCounter;
        }
    }

    void Lightmapper::generateLightMap( const Properties &properties )
    {
        m_properties = properties;

        // Get filename prefix
        String prefix;
        if( properties.getPropertyValue( "FilenamePrefix", prefix ) )
        {
            m_prefix = prefix;
        }

        // Get texture size
        properties.getPropertyValue( "Size", m_textureSize );

        m_imageExtension = ".png";

        // Get lightmap all flag
        properties.getPropertyValue( "LightmapAll", m_lightmapAll );

        // Get auto calculate size flag
        properties.getPropertyValue( "AutoCalculateSize", m_autoTexSize );

        // Get entities to process
        auto applicationManager = core::IApplicationManager::instance();
        auto sceneManager = applicationManager->getGameManager();

        m_lightmapEntities.clear();

        if( !m_lightmapAll )
        {
            // Get selected entities
            auto selectionManager = applicationManager->getSelectionManager();
            if( selectionManager )
            {
                auto selectedObjects = selectionManager->getSelection();
                for( auto &obj : selectedObjects )
                {
                    auto actor = workphone::static_pointer_cast<scene::IGameActor>( obj );
                    if( actor )
                    {
                        m_lightmapEntities.push_back( actor );
                    }
                }
            }
        }
        else
        {
            // Get all entities
            if( sceneManager )
            {
                auto scene = sceneManager->getCurrentScene();
                if( scene )
                {
                    auto actors = scene->getActors();
                    for( auto &actor : actors )
                    {
                        m_lightmapEntities.push_back( actor );
                    }
                }
            }
        }

        WP_LOG( "Started generating lightmaps" );

        auto timer = applicationManager->getTimer();
        m_startTime = static_cast<u32>( timer->getTime() );

        ++LastGenerateRequest;
    }

    String Lightmapper::createMaterial( const String &curMaterialName, const String &newMaterialName )
    {
        auto applicationManager = core::IApplicationManager::instance();
        auto graphicsSystem = applicationManager->getGraphicsSystem();
        auto materialManager = graphicsSystem->getMaterialManager();

        if( !materialManager )
        {
            return {};
        }

        // Remove existing material if it exists
        // if( materialManager->hasMaterial( newMaterialName ) )
        //{
        //    materialManager->removeMaterial( newMaterialName );
        //}

        // Clone the current material or create new one
        SmartPtr<render::IMaterial> material;

        // if( m_showDebugLightmaps )
        //{
        //     material = materialManager->createMaterial( newMaterialName );
        // }
        // else
        //{
        //     auto prevMaterial = materialManager->getMaterial( curMaterialName );
        //     if( prevMaterial )
        //     {
        //         material = prevMaterial->clone( newMaterialName );
        //     }
        //     else
        //     {
        //         material = materialManager->createMaterial( newMaterialName );
        //     }
        // }

        if( !material )
        {
            return String( "" );
        }

        // Add lightmap texture pass
        // auto technique = material->getTechnique( 0 );
        // if( technique )
        //{
        //    auto pass = technique->getPass( "lightmap" );
        //    if( !pass )
        //    {
        //        pass = technique->createPass();
        //        pass->setName( "lightmap" );
        //        pass->setLightingEnabled( false );
        //        pass->setSceneBlending( render::SceneBlendType::Modulate );
        //    }

        //    // Create texture unit for lightmap
        //    auto textureUnit = pass->createTextureUnitState( newMaterialName + m_imageExtension );
        //    textureUnit->setTextureAddressingMode( render::TextureAddressingMode::Clamp );
        //}

        return newMaterialName;
    }
}  // namespace workphone
